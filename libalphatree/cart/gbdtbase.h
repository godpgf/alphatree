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
            tmpElementNum = 0;
        }

        float getWeight(float lambda){
            return -g / (h + lambda);
        }

        float split;
        float weight;
        float g, h;
        int featureIndex;

        int tmpOffset;
        int tmpElementNum;
    };

public:
    BaseGBDT(int threadNum):threadPool_(threadNum), threadNum_(threadNum){
        splitBars_ = DCache<SplitBarList>::create();
        tree_ = DTree<Node, DTREE_BLOCK_SIZE>::create();
    }

    virtual ~BaseGBDT(){
        DCache<SplitBarList>::release(splitBars_);
        DTree<Node, DTREE_BLOCK_SIZE>::release(tree_);
    }



    int boost(IVector<IBaseIterator<float>*>* featureList, IBaseIterator<float>* weight, IBaseIterator<float>* target,
              IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, IBaseIterator<bool>* flag,
              int barSize, float gamma, float lambda, float minWeight, int maxDepth){
        int rootId = tree_->createNode();
        (*tree_)[rootId].clean();


        int* tmpSkip = skip + weight->size();

        for(int i = 0; i < weight->size(); ++i){
            weight->skip(skip[i]);
            flag->skip(skip[i]);
            if(flag->getValue())
                (*tree_)[rootId].weight += weight->getValue();
        }

        weight->skip(0, false);
        flag->skip(0, false);
        (*tree_)[rootId].tmpOffset = 0;
        (*tree_)[rootId].tmpElementNum = weight->size();

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

                //得到当前节点的序列偏移
                int* curSkip = skip + (*tree_)[leafIndex].tmpOffset;
                int curSkipLen = (*tree_)[leafIndex].tmpElementNum;

                //找到最好的特征
                int bestFeatureIndex = -1;
                float maxGain = 0;
                SplitRes bestSplitRes;

                if(threadNum_ > 1){
                    for(int i = 0; i < featureList->getSize(); ++i){
                        if((*featureList)[i] == nullptr)
                            continue;
                        IBaseIterator<float>* featureClone = (*featureList)[i]->clone();
                        IBaseIterator<float>* weightClone = weight->clone();
                        IBaseIterator<bool>* flagClone = flag->clone();
                        IBaseIterator<float>* gClone = g->clone();
                        IBaseIterator<float>* hClone = h->clone();

                        //对每个特征分裂
                        DCache<SplitBarList>* splitBars_ = this->splitBars_;
                        res[i] = threadPool_.enqueue([splitBars_, barSize, featureClone, weightClone, flagClone, gClone, hClone, curSkip, curSkipLen, gamma, lambda]{
                            int barId = splitBars_->useCacheMemory();
                            SplitBarList& bars = splitBars_->getCacheMemory(barId);
                            bars.initialize(barSize);
                            if(!fillBars(featureClone, weightClone, flagClone, gClone, hClone, curSkip, curSkipLen, bars.bars, barSize, bars.startValue, bars.deltaStd)){
                                splitBars_->releaseCacheMemory(barId);
                                SplitRes res;
                                delete featureClone;
                                delete weightClone;
                                delete gClone;
                                delete hClone;
                                return res;
                            } else {
                                auto res = splitBars(bars.bars, barSize, bars.startValue, bars.deltaStd, gamma, lambda);
                                splitBars_->releaseCacheMemory(barId);
                                delete featureClone;
                                delete weightClone;
                                delete gClone;
                                delete hClone;
                                return res;
                            }
                        }).share();
                    }

                    for(int i = 0; i <  featureList->getSize(); ++i){
                        if((*featureList)[i] != nullptr && res[i].get().gain > maxGain){
                            //cout<<" depth: "<<depth<<" feature: "<<i<<" "<<res[i].get().gain<<"/"<<maxGain<<" ";
                            maxGain = res[i].get().gain;
                            bestFeatureIndex = i;
                        }
                    }

                    if(bestFeatureIndex >= 0){
                        bestSplitRes = res[bestFeatureIndex].get();
                    }
                } else{
                    for(int i = 0; i < featureList->getSize(); ++i) {
                        if ((*featureList)[i] == nullptr)
                            continue;
                        int barId = splitBars_->useCacheMemory();
                        SplitBarList& bars = splitBars_->getCacheMemory(barId);
                        bars.initialize(barSize);
                        if(!fillBars((*featureList)[i], weight, flag, g, h, curSkip, curSkipLen, bars.bars, barSize, bars.startValue, bars.deltaStd)){
                            splitBars_->releaseCacheMemory(barId);
                        } else {
                            auto res = splitBars(bars.bars, barSize, bars.startValue, bars.deltaStd, gamma, lambda);
                            splitBars_->releaseCacheMemory(barId);
                            if(res.gain > maxGain){
                                maxGain = res.gain;
                                bestFeatureIndex = i;
                                bestSplitRes = res;
                            }
                        }
                    }
                }
                //cout<<bestFeatureIndex<<endl;

                if(bestFeatureIndex >= 0){
                    //开始分裂
                    (*tree_)[leafIndex].featureIndex = bestFeatureIndex;
                    (*tree_)[leafIndex].split = bestSplitRes.splitValue;
                    int l = tree_->createNode();
                    int r = tree_->createNode();
                    (*tree_)[l].clean();
                    (*tree_)[r].clean();
                    (*tree_)[l].weight = bestSplitRes.leftWeightSum;
                    (*tree_)[r].weight = bestSplitRes.rightWeightSum;
                    (*tree_)[l].g = bestSplitRes.gl;
                    (*tree_)[l].h = bestSplitRes.hl;
                    (*tree_)[r].g = bestSplitRes.gr;
                    (*tree_)[r].h = bestSplitRes.hr;
                    int lSkipLen = updateSkip(leafIndex, (*featureList)[bestFeatureIndex], curSkip, curSkipLen, tmpSkip, true);
                    int rSkipLen = updateSkip(leafIndex, (*featureList)[bestFeatureIndex], curSkip, curSkipLen, tmpSkip + lSkipLen, false);
                    memcpy(curSkip, tmpSkip, lSkipLen * sizeof(int));
                    memcpy(curSkip + lSkipLen, tmpSkip + lSkipLen, rSkipLen * sizeof(int));
                    tree_->addChild(leafIndex, l);
                    tree_->addChild(leafIndex, r);
                    (*tree_)[l].tmpOffset = (*tree_)[leafIndex].tmpOffset;
                    (*tree_)[l].tmpElementNum = lSkipLen;
                    (*tree_)[r].tmpOffset = (*tree_)[leafIndex].tmpOffset + lSkipLen;
                    (*tree_)[r].tmpElementNum = rSkipLen;
                }
            }
            startLeafIndex = lastTreeSize;
        }
        delete []res;
        return rootId;
    }

    void boostPred(int rootId, float lambda, int* skip, IOBaseIterator<float>* pred, float boostWeightScale){
        if(rootId == -1){
            pred->initialize(0);
        } else {
            if(tree_->getChildNum(rootId) == 0){
                float w = (*tree_)[rootId].getWeight(lambda);
                int* curSkip = skip + (*tree_)[rootId].tmpOffset;
                for(int i = 0; i < (*tree_)[rootId].tmpElementNum; ++i){
                    pred->skip(curSkip[i]);
                    float p = pred->getValue();
                    p += w * boostWeightScale;
                    pred->setValue(p);
                }
                pred->skip(0, false);
            } else {
                int offset = 0;
                for(int i = 0; i < tree_->getChildNum(rootId); ++i){
                    int childId = tree_->getChild(rootId,i);
                    boostPred(childId, lambda, skip, pred, boostWeightScale);
                }
            }
        }
    }

protected:
    void updateAllSkip(int nodeId, IVector<IBaseIterator<float>*>* featureList, int* skip, int skipLen, int* tmpSkip){
        if(tree_->getChildNum(nodeId) > 0){
            IBaseIterator<float>* feature = (*featureList)[ (*tree_)[nodeId].featureIndex ];
            int lNum = this->updateSkip(nodeId, feature, skip, skipLen, tmpSkip, true);
            int rNum = this->updateSkip(nodeId, feature, skip, skipLen, tmpSkip + lNum, false);
            memcpy(skip, tmpSkip, lNum * sizeof(int));
            memcpy(skip + lNum,tmpSkip + lNum, rNum * sizeof(int));
            int lid = tree_->getChild(nodeId, 0);
            int rid = tree_->getChild(nodeId, 1);
            (*tree_)[lid].tmpOffset = (*tree_)[nodeId].tmpOffset;
            (*tree_)[lid].tmpElementNum = lNum;
            (*tree_)[rid].tmpOffset = (*tree_)[nodeId].tmpOffset + lNum;
            (*tree_)[rid].tmpElementNum = rNum;
            updateAllSkip(lid,featureList, skip, lNum, tmpSkip);
            updateAllSkip(rid,featureList, skip + lNum, rNum, tmpSkip);
        }
    }

    int updateSkip(int fromNodeId, IBaseIterator<float>* feature, int* skip, int skipLen, int* outSkip, bool isLeft){
        float splitValue = (*tree_)[fromNodeId].split;
        int outSkipLen = 0;
        int curIndex = 0;
        for(int i = 0; i < skipLen; ++i){
            feature->skip(skip[i]);
            curIndex += skip[i];
            if((isLeft && (**feature) < splitValue) ||
               (!isLeft && (**feature) > splitValue)){
                outSkip[outSkipLen++] = curIndex;
            }
        }
        for(int i = outSkipLen - 1; i > 0; --i){
            outSkip[i] -= outSkip[i-1];
        }

        feature->skip(0, false);
        return outSkipLen;
    }
protected:
    DCache<SplitBarList>* splitBars_ = {nullptr};
    DTree<Node, DTREE_BLOCK_SIZE>* tree_ = {nullptr};
    int threadNum_;
    ThreadPool threadPool_;
};

#endif //ALPHATREE_GBDTBASE_H
