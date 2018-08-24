//
// Created by godpgf on 18-6-12.
//

#ifndef ALPHATREE_ALPHARLT_H
#define ALPHATREE_ALPHARLT_H

#include "iterator.h"
#include "vector.h"
#include "threadpool.h"
#include "dtree.h"
#include "bag.h"
#include "../alphaforest.h"

class AlphaRLT{
public:
    static void initialize(AlphaForest* af, char* featureList, int featureNum, char* price, char* volume){
        alphaRLT_ = new AlphaRLT(af, featureList, featureNum, price, volume);
    }

    static void release(){
        if(alphaRLT_ != nullptr)
            delete alphaRLT_;
        alphaRLT_ = nullptr;
    }

    static AlphaRLT* getAlphaRLT(){ return alphaRLT_;}

    struct Node{
        int featureIndex;
        float featureSplitValue;
        bool isLessThenFeature;
        //reward of pre node to this node
        float expectReturn;
        float bestSharp;
        //is buy sign or sell sign
        bool isBuy;
    };

    class TrainPars{
    public:
        IVector<IBaseIterator<float>*>* featureList;
        IVector<float>* splitWeight;
        int* skip;
        float minWeight;
        char* buySignList;
        char* sellSignList;
        char* codes;
        int stockNum;
        char* marketCodes;
        int marketNum;
        int daybefore;
        int sampleDays;
        float* returns;
        float* marketReturns;
        int maxBagNum = {8};
        int maxBuyDepth = {4};
        float lr = {0.1f};
        float step = {0.8f};
        float tiredCoff = {0.016f};
        float explorationRate = {0.1f};
    };

    virtual void leaf2str(int leafId, char* buySignList, int& buySignNum, char* sellSignList, int& sellSignNum){
        buySignNum = 0;
        sellSignNum = 0;
        while (tree_->getParent(leafId) != -1){
            int featureIndex = (*tree_)[leafId].featureIndex;
            if(featureIndex != -1){
                if((*tree_)[leafId].isBuy){
                    ++buySignNum;
                    if((*tree_)[leafId].isLessThenFeature){
                        sprintf(buySignList, "(%s < %.4f)", features_[featureIndex], (*tree_)[leafId].featureSplitValue);
                    } else {
                        sprintf(buySignList, "(%s > %.4f)", features_[featureIndex], (*tree_)[leafId].featureSplitValue);
                    }
                    buySignList += (strlen(buySignList) + 1);
                } else {
                    ++sellSignNum;
                    if((*tree_)[leafId].isLessThenFeature){
                        sprintf(sellSignList, "(%s < %.4f)", features_[featureIndex], (*tree_)[leafId].featureSplitValue);
                    } else {
                        sprintf(sellSignList, "(%s > %.4f)", features_[featureIndex], (*tree_)[leafId].featureSplitValue);
                    }
                    sellSignList += (strlen(sellSignList) + 1);
                }
            }
            leafId = tree_->getParent(leafId);
        }
    }

    void train(const char* signName, char* codes, int stockNum, char* marketCodes, int marketNum, int daybefore, int sampleDays, int minSignNum, int threadNum, int epochNum,
               int maxBagNum = 8, int maxBuyDepth = 4, float lr = 0.1f, float step = 0.8f, float tiredCoff = 0.016f, float explorationRate = 0.1f){
        ThreadPool threadPool(threadNum);

        Vector<int> featureIds(features_.getSize());
        for(int i = 0; i < features_.getSize(); ++i) {
            featureIds[i] = af_->useAlphaTree();
            af_->decode(featureIds[i], "sign", features_[i]);
        }

        //填写特征
        Vector<IBaseIterator<float>*> featureSignList(features_.getSize());
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName);
        for(size_t i = 0; i < featureSignList.getSize(); ++i){
            featureSignList[i] = new AlphaSignIterator(af_, "sign", signName, featureIds[i],daybefore,sampleDays, 0, signNum);
        }

        Vector<IVector<IBaseIterator<float>*>*> fs(threadNum);
        Vector<IVector<float>*> splitWeights(threadNum);
        for(int i = 0; i < threadNum; ++i){
            fs[i] = new Vector<IBaseIterator<float>*>(features_.getSize());
            splitWeights[i] = new Vector<float>(4 * features_.getSize() * maxBagNum);
            for(int j = 0; j < features_.getSize(); ++j){
                (*fs[i])[j] = featureSignList[j]->clone();
            }
        }

        int* skip = new int[signNum * threadNum];
        std::shared_future<void>* res = new std::shared_future<void>[threadNum];
        char* buySignList = new char[threadNum * maxFeatureStrLen_];
        char* sellSignList = new char[threadNum * maxFeatureStrLen_];
        float* returns = new float[sampleDays * threadNum];
        float* marketReturns = new float[sampleDays * threadNum];

        auto* rlt = this;

        while (epochNum > 0){
            for(int i = 0; i < threadNum; ++i){
                TrainPars tp;
                tp.featureList = fs[i];
                tp.splitWeight = splitWeights[i];
                tp.skip = skip + i * signNum;
                tp.buySignList = buySignList + (maxFeatureStrLen_ * i);
                tp.sellSignList = sellSignList + (maxFeatureStrLen_ * i);
                tp.returns = returns + (sampleDays * i);
                tp.marketReturns = marketReturns + (sampleDays * i);
                tp.daybefore = daybefore;
                tp.sampleDays = sampleDays;
                tp.maxBagNum = maxBagNum;
                tp.explorationRate = explorationRate;
                tp.lr = lr;
                tp.maxBuyDepth = maxBuyDepth;
                tp.codes = codes;
                tp.stockNum = stockNum;
                tp.marketCodes = marketCodes;
                tp.marketNum = marketNum;
                tp.minWeight = minSignNum;
                tp.tiredCoff = tiredCoff;
                tp.step = step;
                //res[i] = threadPool.enqueue([rlt, tp]{ return rlt->update(tp);}).share();
                rlt->update(tp);
            }

            //for(int i = 0; i < threadNum; ++i)
                //res[i].get();
            epochNum -= threadNum;
        }

        //release
        delete []res;
        delete []skip;
        delete []buySignList;
        delete []sellSignList;
        delete []returns;
        delete []marketReturns;

        for(int i = 0; i < threadNum; ++i){
            for(int j = 0; j < features_.getSize(); ++j){
                delete (*fs[i])[j];
            }
            delete fs[i];
            delete splitWeights[i];
        }

        for(int i = 0; i < features_.getSize(); ++i){
            delete featureSignList[i];
            af_->releaseAlphaTree(featureIds[i]);
        }

    }
protected:
    AlphaRLT(AlphaForest* af, char* featureList, int featureNum, char* price, char* volume):af_(af), features_(featureNum){
        featureList_ = featureList;
        int n = featureNum;
        int len = 0;
        while (n > 0){
            --n;
            len += strlen(featureList_) + 1;
            featureList_ = featureList + len;
        }
        featureList_ = new char[len];
        char* p = featureList_;
        n = 0;
        maxFeatureStrLen_ = 0;
        while (n < featureNum){
            strcpy(p, featureList);
            features_[n] = p;
            ++n;
            len = strlen(featureList) + 1;
            maxFeatureStrLen_ = max(len, maxFeatureStrLen_);
            featureList += len;
            p += len;
        }
        maxFeatureStrLen_ += 16;

        tree_ = DTree<Node>::create();
        rootId_ = tree_->createNode();
        (*tree_)[rootId_].featureIndex = -1;

        price_ = new char[strlen(price) + 1];
        strcpy(price_, price);
        volume_ = new char[strlen(volume) + 1];
        strcpy(volume_, volume);
    }

    virtual ~AlphaRLT(){
        delete[] featureList_;
        delete[] price_;
        delete[] volume_;

        DTree<Node>::release(tree_);
    }

    float getReward(TrainPars& tp, int buySignNum, int sellSignNum){
        af_->getReturns(tp.marketCodes, tp.marketNum, tp.daybefore, tp.sampleDays, volume_, 1, nullptr, 0, FLT_MAX, FLT_MAX, tp.sampleDays, tp.marketReturns, price_);
        const int BASE_TRY_TIME = 4;
        int maxHoldDays[BASE_TRY_TIME] = {1, 3, 5, 8};
        float maxDrawdown[BASE_TRY_TIME] = {0.03f, 0.08f, 0.16f, 1.f};
        float maxReturn[BASE_TRY_TIME] = {0.05f, 0.1f, 0.2f, 1.f};
        float maxSharp = 0;
        float bestSharp = 0;
        for(int i = 0; i < BASE_TRY_TIME; ++i){
            for(int j = 0; j < BASE_TRY_TIME; ++j){
                for(int k = 0; k < BASE_TRY_TIME; ++k){
                    af_->getReturns(tp.codes, tp.stockNum, tp.daybefore, tp.sampleDays, tp.buySignList, buySignNum, tp.sellSignList, sellSignNum, maxReturn[i], maxDrawdown[j], maxHoldDays[k], tp.returns, price_);
                    float maxReturn = -FLT_MAX;
                    float maxDrawdown = 0;
                    for(int l = 0; l < tp.sampleDays; ++l){
                        tp.returns[l] -= tp.marketReturns[l];
                        maxReturn = max(maxReturn, tp.returns[l]);
                        maxDrawdown = max(maxDrawdown, maxReturn - tp.returns[l]);
                    }
                    bestSharp = max(bestSharp, (tp.returns[tp.sampleDays-1]-1.f) / maxDrawdown);
                    //cout<<tp.returns[tp.sampleDays-1]-1.f<<"/"<<maxDrawdown<<"="<<(tp.returns[tp.sampleDays-1]-1.f) / maxDrawdown<<endl;
                }
            }
        }
        cout<<bestSharp<<endl;
        return bestSharp;
    }

    //thread safe
    void update(TrainPars tp){
        for(int i = 1; i < (*tp.featureList)[0]->size(); ++i)
            tp.skip[i] = 1;
        tp.skip[0] = 0;
        int leafId = grow(rootId_, (*tp.featureList)[0]->size(), tp.maxBuyDepth, tp);
        if(leafId > 0){
            mendScore(leafId, tp);
        }
    }

    //在某个节点继续生长
    int grow(int nodeId, int skipLen, int remainBuyDepth, TrainPars& tp){
        //cout<<remainBuyDepth<<" "<<nodeId<<" "<<tree_->getChildNum(nodeId)<<endl;
        //如果有线程正在分裂当前这个节点，返回失败，一个节点不能同时被多个线程分裂！（他孩子的分裂不受影响）
        if(!tree_->lock(nodeId))
            return -1;
        if(tree_->getChildNum(nodeId) == 0){
            if(tree_->getParent(nodeId) == -1){
                spreadBuyNode(nodeId, skipLen, tp);
            } else {
                if((*tree_)[nodeId].isBuy){
                    if(remainBuyDepth > 0)
                        spreadBuyNode(nodeId, skipLen, tp);
                    spreadSellNode(nodeId);
                } else{
                    spreadSellNode(nodeId);
                }
            }
        }
        tree_->unlock(nodeId);

        if(tree_->getChildNum(nodeId) == 0)
            return nodeId;

        initFeatureWeight(nodeId, tp.splitWeight, tp.explorationRate);
        int nextNodeId = tree_->getChild(nodeId, randomChooseSplit(tp.splitWeight, tree_->getChildNum(nodeId)));
        updateSkip(nextNodeId, tp.featureList, tp.skip, skipLen);
        return grow(nextNodeId, skipLen, remainBuyDepth-1, tp);
    }

    void initFeatureWeight(int nodeId, IVector<float>* splitWeight, float explorationRate = 0.1f){
        int childNum = tree_->getChildNum(nodeId);

        float sumR = 0;
        float sqrSumR = 0;
        float minR = FLT_MAX;

        for(int i = 0; i < childNum; ++i){
            sumR += (*tree_)[tree_->getChild(nodeId, i)].expectReturn;
            sqrSumR += (*tree_)[tree_->getChild(nodeId, i)].expectReturn * (*tree_)[tree_->getChild(nodeId, i)].expectReturn;
            minR = std::min((*tree_)[tree_->getChild(nodeId, i)].expectReturn, minR);
        }

        float avgR = sumR / childNum;
        float stdR = sqrtf(sqrSumR / childNum - avgR * avgR);
        float minWeight = stdR == 0.0f ? 1 : stdR * explorationRate;

        for(int i = 0; i < childNum; ++i){
            (*splitWeight)[i] = (*tree_)[tree_->getChild(nodeId, i)].expectReturn - minR + minWeight;
        }
    }


    void mendScore(int leafId, TrainPars& tp){
        //当某个叶节点被尝试了很多次，就会疲劳，于是给它扣分，以便算法能有更多精力尝试别的
        int currentLeafId = leafId;
        if(!tree_->lock(leafId))
            return;
        if((*tree_)[currentLeafId].expectReturn == 0){
            int buySignNum, sellSignNum;
            leaf2str(leafId, tp.buySignList, buySignNum, tp.sellSignList, sellSignNum);
            (*tree_)[currentLeafId].expectReturn = getReward(tp, buySignNum, sellSignNum);
            (*tree_)[currentLeafId].bestSharp = (*tree_)[currentLeafId].expectReturn;
        }
        tree_->unlock(leafId);

        std::lock_guard<std::mutex> lock{mutex_};
        int preId = tree_->getParent(leafId);
        while(preId != -1){
            int chidNum = tree_->getChildNum(preId);
            float nextR = -FLT_MAX;
            for(int i = 0; i < chidNum; ++i){
                nextR = std::max((*tree_)[tree_->getChild(preId, i)].expectReturn, nextR);
            }
            float r = 0;
            (*tree_)[preId].expectReturn = (*tree_)[preId].expectReturn + tp.lr * (r + tp.step * nextR - (*tree_)[preId].expectReturn);
            leafId = preId;
            preId = tree_->getParent(leafId);
        }
        (*tree_)[currentLeafId].expectReturn -= tp.tiredCoff;
    }

    void spreadBuyNode(int nodeId, int skipLen, TrainPars& tp){
        int parentId = tree_->getParent(nodeId);
        bool isConsiderMyself = (parentId != -1 && (*tree_)[parentId].featureIndex != (*tree_)[nodeId].featureIndex);

        int bagIndex = 0;
        float curSplitValue = 0;
        if(isConsiderMyself){
            DataBag* dataBag = createBags((*tp.featureList)[(*tree_)[nodeId].featureIndex], tp.skip, skipLen, tp.minWeight, tp.maxBagNum);
            if(dataBag && dataBag[0].weightSum > tp.minWeight){
                //last bag is no use
                while (dataBag[bagIndex].maxValue < FLT_MAX){
                    if((*tree_)[nodeId].isLessThenFeature && dataBag[bagIndex].maxValue < (*tree_)[nodeId].featureSplitValue){
                        int childId = tree_->createNode();
                        (*tree_)[childId].featureSplitValue = dataBag[bagIndex].maxValue;
                        (*tree_)[childId].isLessThenFeature = false;
                        (*tree_)[childId].featureIndex = (*tree_)[parentId].featureIndex;
                        (*tree_)[childId].expectReturn = 0;
                        (*tree_)[childId].isBuy = true;
                        tree_->addChild(nodeId, childId);
                    } else if(!(*tree_)[nodeId].isLessThenFeature && dataBag[bagIndex].maxValue > (*tree_)[nodeId].featureSplitValue){
                        int childId = tree_->createNode();
                        (*tree_)[childId].featureSplitValue = dataBag[bagIndex].maxValue;
                        (*tree_)[childId].isLessThenFeature = true;
                        (*tree_)[childId].featureIndex = (*tree_)[parentId].featureIndex;
                        (*tree_)[childId].expectReturn = 0;
                        (*tree_)[childId].isBuy = true;
                        tree_->addChild(nodeId, childId);
                    }
                    ++bagIndex;
                }
            }
            destroyBags(dataBag);
        }

        for(int i = (*tree_)[nodeId].featureIndex + 1; i < tp.featureList->getSize(); ++i){
            //cout<<"    "<<i<<endl;
            af_->getAlphaDataBase()->loadFeature(features_[i]);
            DataBag* dataBag = createBags((*tp.featureList)[i], tp.skip, skipLen, tp.minWeight, tp.maxBagNum);
            bagIndex = 0;
            if(dataBag && dataBag[0].weightSum > tp.minWeight){
                //last bag is no use
                while (dataBag[bagIndex].maxValue < FLT_MAX){
                    for(int j = 0; j < 2; ++j){
                        int childId = tree_->createNode();
                        (*tree_)[childId].featureIndex = i;
                        (*tree_)[childId].expectReturn = 0;
                        (*tree_)[childId].isLessThenFeature = (j == 0);
                        (*tree_)[childId].featureSplitValue = dataBag[bagIndex].maxValue;
                        (*tree_)[childId].isBuy = true;
                        tree_->addChild(nodeId, childId);
                    }
                    ++bagIndex;
                }
            }
            destroyBags(dataBag);
            af_->getAlphaDataBase()->releaseFeature(features_[i]);
        }
    }

    void spreadSellNode(int nodeId){
        int sellNum = 0;
        int parentId = nodeId;
        while ((*tree_)[parentId].isBuy == false){
            ++sellNum;
            parentId = tree_->getParent(parentId);
        }
        while (sellNum > 0){
            parentId = tree_->getParent(parentId);
            --sellNum;
        }
        int mirrorId = parentId;
        parentId = tree_->getParent(mirrorId);
        if((*tree_)[mirrorId].featureIndex != -1){
            //jump
            int jumpNodeId = tree_->createNode();
            (*tree_)[jumpNodeId].isBuy = false;
            (*tree_)[jumpNodeId].expectReturn = 0;
            (*tree_)[jumpNodeId].featureIndex = -1;
            tree_->addChild(nodeId, jumpNodeId);

            for(int i = 0; i < tree_->getChildNum(parentId); ++i){
                int childId = tree_->getChild(parentId, i);
                if((*tree_)[childId].featureIndex == (*tree_)[mirrorId].featureIndex &&
                   mirrorId != childId &&
                   (*tree_)[mirrorId].isLessThenFeature != (*tree_)[childId].isLessThenFeature){
                    if(((*tree_)[mirrorId].isLessThenFeature && (*tree_)[childId].featureSplitValue > (*tree_)[mirrorId].featureSplitValue) ||
                       (!(*tree_)[mirrorId].isLessThenFeature && (*tree_)[childId].featureSplitValue < (*tree_)[mirrorId].featureSplitValue)){
                        int newNodeId = tree_->createNode();
                        (*tree_)[newNodeId].isBuy = false;
                        (*tree_)[newNodeId].featureSplitValue = (*tree_)[childId].featureSplitValue;
                        (*tree_)[newNodeId].isLessThenFeature = !(*tree_)[mirrorId].isLessThenFeature;
                        (*tree_)[newNodeId].featureIndex = (*tree_)[mirrorId].featureIndex;
                        (*tree_)[newNodeId].expectReturn = 0;
                        tree_->addChild(nodeId, newNodeId);
                    }
                }
            }
        }
    }

    void updateSkip(int toNodeId, IVector<IBaseIterator<float> *> *featureList, int* skip, int& skipLen){
        int featureIndex = (*tree_)[toNodeId].featureIndex;
        bool isLeft = (*tree_)[toNodeId].isLessThenFeature;
        float splitValue = (*tree_)[toNodeId].featureSplitValue;
        IBaseIterator<float>* feature = (*featureList)[featureIndex];

        int lastSkipId = 0;
        int curIndex = 0;

        for(int i = 0; i < skipLen; ++i){
            feature->skip(skip[i]);
            curIndex += skip[i];
            if((isLeft && (**feature) < splitValue) ||
               (!isLeft && (**feature) >= splitValue)){
                skip[lastSkipId++] = curIndex;
            }
        }

        skipLen = lastSkipId--;
        while (lastSkipId > 0){
            skip[lastSkipId] -= skip[lastSkipId-1];
            --lastSkipId;
        }
        feature->skip(0, false);
    }

    static int randomChooseSplit(IVector<float>* splitWeight, int splitNum){
        float sumP = 0;
        for(size_t i = 0; i < splitNum; ++i){
            sumP += (*splitWeight)[i];
        }
        float rValue = rand() / (float)RAND_MAX;
        rValue *= sumP;
        int curIndex = 0;
        for(size_t i = 0; i < splitNum; ++i){
            rValue -= (*splitWeight)[i];
            if(rValue <= 0)
                return curIndex;
            ++curIndex;
        }
        return std::min(curIndex - 1, splitNum - 1);
    }
protected:
    AlphaForest* af_;
    DTree<Node>* tree_ = {nullptr};
    //线程安全
    std::mutex mutex_;
    char* featureList_;
    Vector<char*> features_;
    int maxFeatureStrLen_;
    int rootId_;
    char* price_;
    char* volume_;

    static AlphaRLT* alphaRLT_;
};

AlphaRLT* AlphaRLT::alphaRLT_ = nullptr;

#endif //ALPHATREE_ALPHARLT_H
