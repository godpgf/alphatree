//
// Created by godpgf on 18-3-28.
//

#ifndef ALPHATREE_GBDTBASE_H
#define ALPHATREE_GBDTBASE_H

#include <mutex>
#include <thread>
#include "split.h"
#include "../base/vector.h"
#include "../base/dtree.h"
#include "../base/dcache.h"
#include "../base/threadpool.h"
#include <float.h>

template <int DTREE_BLOCK_SIZE = 32>
class BaseGBDT{
protected:
    class Node{
    public:
        void clean(){
            weight = 0;
            featureIndex = -1;
        }

        float getWeight(float lambda){
            return -g / (h + lambda);
        }

        float split;
        float weight;
        float g, h;
        int featureIndex;
    };

public:
    BaseGBDT(int threadNum):threadPool_(threadNum){
        splitBars_ = DCache<SplitBarList>::create();
        tree_ = DTree<Node, DTREE_BLOCK_SIZE>::create(2);
    }

    virtual ~BaseGBDT(){
        DCache<SplitBarList>::release(splitBars_);
        DTree<Node, DTREE_BLOCK_SIZE>::release(tree_);
    }



    int boost(IVector<IBaseIterator<float>*>* featureList, IBaseIterator<float>* weight, IBaseIterator<float>* target,
              IBaseIterator<float>* g, IBaseIterator<float>* h, IOBaseIterator<int>* flag,
              int barSize, float gamma, float lambda, float minWeight, int maxDepth){
        int rootId = tree_->createNode();
        (*tree_)[rootId].clean();
        flag->initialize(rootId);
        while (weight->isValid()){
            (*tree_)[rootId].weight += weight->getValue();
            weight->skip(1);
        }
        weight->skip(0, false);
        int startLeafIndex = rootId;
        std::shared_future<SplitRes>* res = new std::shared_future<SplitRes>[featureList->getSize()];
        //cout<<tree_->getSize()<<" "<<startLeafIndex<<" "<<maxDepth<<" "<<(*tree_)[startLeafIndex].weight<<endl;
        for(int depth = 0; depth < maxDepth; ++depth){
            if(tree_->getSize() == startLeafIndex){
                //当前没有节点可以继续分裂
                break;
            }
            //多线程对所有叶节点分裂
            int lastTreeSize = tree_->getSize();
            for(int leafIndex = startLeafIndex; leafIndex < lastTreeSize; ++leafIndex){
                if((*tree_)[leafIndex].weight < minWeight)
                    continue;
                for(int i = 0; i < featureList->getSize(); ++i){
                    IBaseIterator<float>* featureClone = (*featureList)[i]->clone();
                    IBaseIterator<float>* weightClone = weight->clone();
                    IBaseIterator<float>* gClone = g->clone();
                    IBaseIterator<float>* hClone = h->clone();
                    IBaseIterator<int>* flagClone = flag->clone();

                    //对每个特征分裂
                    DCache<SplitBarList>* splitBars_ = this->splitBars_;
                    res[i] = threadPool_.enqueue([splitBars_, barSize, featureClone, weightClone, gClone, hClone, flagClone, leafIndex, gamma, lambda]{
                        int barId = splitBars_->useCacheMemory();
                        SplitBarList& bars = splitBars_->getCacheMemory(barId);
                        bars.initialize(barSize);
                        if(!fillBars(featureClone, weightClone, gClone, hClone, flagClone, leafIndex, bars.bars, barSize, bars.startValue, bars.deltaStd)){
                            splitBars_->releaseCacheMemory(barId);
                            SplitRes res;
                            delete featureClone;
                            delete weightClone;
                            delete gClone;
                            delete hClone;
                            delete flagClone;
                            return res;
                        } else {
                            auto res = splitBars(bars.bars, barSize, bars.startValue, bars.deltaStd, gamma, lambda);
                            splitBars_->releaseCacheMemory(barId);
                            delete featureClone;
                            delete weightClone;
                            delete gClone;
                            delete hClone;
                            delete flagClone;
                            return res;
                        }
                    }).share();
                }

                //找到最好的特征
                int bestFeatureIndex = -1;
                float maxGain = 0;
                for(int i = 0; i <  featureList->getSize(); ++i){
                    //cout<<res[i].get().gain<<"/"<<maxGain<<" ";
                    if(res[i].get().gain > maxGain){
                        maxGain = res[i].get().gain;
                        bestFeatureIndex = i;
                    }
                }
                //cout<<bestFeatureIndex<<endl;

                if(bestFeatureIndex >= 0){
                    //开始分裂
                    (*tree_)[leafIndex].featureIndex = bestFeatureIndex;
                    (*tree_)[leafIndex].split = res[bestFeatureIndex].get().splitValue;
                    int l = tree_->createNode();
                    int r = tree_->createNode();
                    (*tree_)[l].clean();
                    (*tree_)[r].clean();
                    (*tree_)[l].weight = res[bestFeatureIndex].get().leftWeightSum;
                    (*tree_)[r].weight = res[bestFeatureIndex].get().rightWeightSum;
                    (*tree_)[l].g = res[bestFeatureIndex].get().gl;
                    (*tree_)[l].h = res[bestFeatureIndex].get().hl;
                    (*tree_)[r].g = res[bestFeatureIndex].get().gr;
                    (*tree_)[r].h = res[bestFeatureIndex].get().hr;
                    updateFlag(leafIndex, l, (*featureList)[bestFeatureIndex], flag, true);
                    updateFlag(leafIndex, r, (*featureList)[bestFeatureIndex], flag, false);
                    tree_->addChild(leafIndex, l);
                    tree_->addChild(leafIndex, r);
                }
            }
            startLeafIndex = lastTreeSize;
        }
        delete []res;
        return rootId;
    }

    void boostPred(int rootId, float lambda, IOBaseIterator<int>* flag, IOBaseIterator<float>* pred, float boostWeightScale){
        if(rootId == -1){
            pred->initialize(0);
        } else {
            while (pred->isValid()){
                float p = pred->getValue() * boostWeightScale;
                p += (*tree_)[flag->getValue()].getWeight(lambda);
                pred->setValue(p);
                pred->skip(1);
                flag->skip(1);
            }
            pred->skip(0,false);
            flag->skip(0,false);
        }
    }

protected:
    void updateAllFlag(int nodeId, IVector<IBaseIterator<float>*>* featureList, IOBaseIterator<int>* flag){
        for(int i = 0; i < tree_->getChildNum(nodeId); ++i){
            IBaseIterator<float>* feature = (*featureList)[ (*tree_)[nodeId].featureIndex ];
            this->updateFlag(nodeId, tree_->getChild(nodeId, i), feature, flag, i == 0);
            this->updateAllFlag(tree_->getChild(nodeId, i), featureList, flag);
        }
    }

    void updateFlag(int fromNodeId, int toNodeId, IBaseIterator<float>* feature, IOBaseIterator<int>* flag, bool isLeft){
        float splitValue = (*tree_)[fromNodeId].split;
        while (feature->isValid()){
            if(**flag == fromNodeId){
                if((isLeft && (**feature) < splitValue) ||
                   (!isLeft && (**feature) > splitValue)){
                    flag->setValue(toNodeId);
                }
            }

            ++*feature;
            ++*flag;
        }
        feature->skip(0, false);
        flag->skip(0, false);
    }
protected:
    DCache<SplitBarList>* splitBars_ = {nullptr};
    DTree<Node, DTREE_BLOCK_SIZE>* tree_ = {nullptr};
    ThreadPool threadPool_;
};

#endif //ALPHATREE_GBDTBASE_H
