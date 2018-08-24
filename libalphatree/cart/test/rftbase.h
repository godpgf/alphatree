//
// Created by godpgf on 18-3-15.
// base random filter tree
// 随机过滤树的基本操作
// 解释：为什么用树不用图？因为很小概率会遇到相同的状态（所有特征切割相同特征也相同的情况在不同的特征选择次序下很难遇到，偶然遇见了也是少数情况，不影响大局）
//

#ifndef ALPHATREE_RFTBASE_H
#define ALPHATREE_RFTBASE_H

#include <mutex>
#include <thread>
#include <map>
#include "../split.h"
#include "vector.h"
#include "dtree.h"
#include "dcache.h"
#include <float.h>
#include <random>

#define Q_LEARNING

#define MAX_TIME_STEP 32

struct NodeTimeStep{
    double targetSum;
    double targetSqrSum;
    double weightSum;
};

//DTREE_BLOCK_SIZE树的节点数每次动态增加的数量
template <int DTREE_BLOCK_SIZE = 32>
class BaseRFT{
public:

    struct Node{
        int featureIndex;
        float splitValue;
        bool isLeft;
        bool isSuccess;
        NodeTimeStep timeStep[MAX_TIME_STEP];
//        inline float getTargetAvg(){
//            return targetSum / weightSum;
//        }
//        inline float getTargetStd(){
//            return sqrtf(targetSqrSum / weightSum - powf(getTargetAvg(), 2));
//        }
//        inline float getReward(Node* preNode, bool isConsiderRisk = false){
//            return isConsiderRisk ? getTargetAvg() / getTargetStd() - preNode->getTargetAvg() / preNode->getTargetStd() : getTargetAvg() - preNode->getTargetAvg();
//        }
#ifdef Q_LEARNING
        //回报期望
        float expectReturn;
#endif
    };

public:
    BaseRFT(){
        splitBars_ = DCache<SplitBarList>::create();
        //孩子数是所有特征的2倍，因为特征可以左右分，每个分裂方式都可以作为孩子
        tree_ = DTree<Node, DTREE_BLOCK_SIZE>::create();
    }

    virtual ~BaseRFT(){
        DCache<SplitBarList>::release(splitBars_);
        DTree<Node, DTREE_BLOCK_SIZE>::release(tree_);
    }

    int createRoot(IBaseIterator<float>* weight, IBaseIterator<float>* target, int stepNum){
        int rootId = tree_->createNode();

        (*tree_)[rootId].featureIndex = -1;
        (*tree_)[rootId].isSuccess = true;

        for(int i = 0; i < stepNum; ++i){
            (*tree_)[rootId].timeStep[i].targetSum = 0;
            (*tree_)[rootId].timeStep[i].targetSqrSum = 0;
            (*tree_)[rootId].timeStep[i].weightSum = 0;
        }

        float timeSpan = target->size() / float(stepNum);

        int index = 0;
        while (target->isValid()){
            int sid = (int)(index / timeSpan);
            (*tree_)[rootId].timeStep[sid].targetSum += (**weight) * (**target);
            (*tree_)[rootId].timeStep[sid].targetSqrSum += powf((**target), 2) * (**weight);
            (*tree_)[rootId].timeStep[sid].weightSum += (**weight);
            ++*weight;
            ++*target;
            ++index;
        }

        weight->skip(0, false);
        target->skip(0, false);

        return rootId;
    }

    template <class F>
    void update(int rootId, IVector<IBaseIterator<float>*>* featureList, IVector<float>* splitWeight, IBaseIterator<float>* weight, IBaseIterator<float>* target,
                IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip,
                int barSize, float gamma, float lambda, float minWeight, F&& f, int stepNum = 1
#ifdef Q_LEARNING
            ,float lr = 0.1f, float step = 0.8f, float tiredCoff = 0.016f
#endif
    ){
        for(int i = 1; i < target->size(); ++i)
            skip[i] = 1;
        skip[0] = 0;

        int leafId = grow(rootId, featureList, splitWeight, weight, target, g, h, skip, target->size(), barSize, gamma, lambda, minWeight, stepNum);
#ifdef Q_LEARNING
        mendScore(leafId, lr, step, tiredCoff, f, stepNum);
#endif
    }

    virtual void eval(int rootId, IVector<IBaseIterator<float>*>* featureList, IBaseIterator<float>* weight, IBaseIterator<float>* target, int stepIndex = 1){
        cleanTarger(rootId, stepIndex);

        while (target->isValid()){
            curEval(rootId, featureList, weight, target, stepIndex);
            ++*weight;
            ++*target;
            for(size_t i = 0; i < featureList->getSize(); ++i)
                ++*(*featureList)[i];
        }

        for(size_t i = 0; i < featureList->getSize(); ++i)
            (*featureList)[i]->skip(0, false);
        weight->skip(0, false);
        target->skip(0, false);
    }

    template <class F>
    float cleanTree(int rootId, float threshold, F&& f, int startStepIndex, int stepNum = 1){
        return cleanTree(rootId, rootId, threshold, f, startStepIndex, stepNum);
    }

    //(isCorrAll=false)当某个叶子节点中的数据中有corrPercent与另外某个叶子节点重合,(isCorrAll=true)或者有corrPercent的数据与其他某些叶子节点重合，就删掉它
    virtual bool cleanCorrelation(int rootId, IVector<IBaseIterator<float>*>* featureList, IBaseIterator<float>* weight, float corrPercent = 0.36f, bool isCorrAll = false){
        //给叶节点标号，记录映射
        map<int, int> node2index;
        map<int, int> index2node;
        int leafNum = 0;
        createLeafIndex(rootId, node2index, index2node, leafNum);


        //得到节点相关性矩阵 和 每个叶节点数量
        float* corrMat = new float[leafNum * leafNum];
        memset(corrMat, 0, leafNum * leafNum * sizeof(float));
        float* weightSum = new float[leafNum];
        memset(weightSum, 0, leafNum * sizeof(float));
        fillCorrMat(rootId, node2index, featureList, weight, corrMat, weightSum);

        //删掉最差的节点
        int delNodeIndex = -1;
        float maxPercent = 0;
        if(isCorrAll){
            for(int i = 0; i < leafNum; ++i){
                float corrSumWeight = 0;
                for(int j = 0; j < leafNum; ++j){
                    corrSumWeight += corrMat[i * leafNum + j];
                }
                float p = corrSumWeight / weightSum[i];
                if(p > corrPercent){
                    if(p > maxPercent){
                        maxPercent = p;
                        delNodeIndex = i;
                    }
                }
            }
        } else{
            for(int i = 0; i < leafNum; ++i){
                for(int j = 0; j < leafNum; ++j){
                    if(weightSum[i] > 0.01f){
                        float p = corrMat[i * leafNum + j] / weightSum[i];
                        if(p > corrPercent){
                            if(p > maxPercent){
//                                cout<<p<<endl;
                                maxPercent = p;
                                delNodeIndex = i;
                            }
                        }
                    }
                }
            }
        }

        delete []corrMat;
        delete []weightSum;

        if(delNodeIndex >= 0){
            int delId = index2node[delNodeIndex];
            int parentId = tree_->getParent(delId);
            while (parentId != -1){
                for(int i = 0; i < tree_->getChildNum(parentId); ++i){
                    if(tree_->getChild(parentId, i) == delId){
                        tree_->removeChild(parentId, i);
                        if(tree_->getChildNum(parentId) == 0){
                            delId = parentId;
                            parentId = tree_->getParent(delId);
                        } else {
                            parentId = -1;
                        }

                        break;
                    }
                }
            }
            return true;
        }

        return false;
    }
protected:
    //在某个节点继续生长
    int grow(int rootId, IVector<IBaseIterator<float>*>*featureList, IVector<float>* splitWeight, IBaseIterator<float>* weight, IBaseIterator<float>* target,
             IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, int skipLen,
             int barSize, float gamma, float lambda, float minWeight, int stepNum = 1){
#ifdef Q_LEARNING
        initFeatureWeight(rootId, splitWeight);
#endif
        int featureIndex = randomChooseSplit(splitWeight);
        bool isLeft = featureIndex < (int)(splitWeight->getSize() >> 1);
        if(!isLeft)
            featureIndex -= (splitWeight->getSize() >> 1);
        int newNodeId = growFeature(rootId, featureList, featureIndex, isLeft, weight, target, g, h, skip, skipLen, barSize, gamma, lambda, minWeight, stepNum);
        if(newNodeId == -1)
            return rootId;
        return grow(newNodeId, featureList, splitWeight, weight, target, g, h, skip, skipLen, barSize, gamma, lambda, minWeight, stepNum);
    }

#ifdef Q_LEARNING
    void initFeatureWeight(int rootId, IVector<float>* splitWeight, float explorationRate = 0.1f){
        int childNum = tree_->getChildNum(rootId);

        float sumR = 0;
        float sqrSumR = 0;
        float minR = FLT_MAX;

        for(int i = 0; i < childNum; ++i){
            if((*tree_)[tree_->getChild(rootId, i)].isSuccess){
                sumR += (*tree_)[tree_->getChild(rootId, i)].expectReturn;
                sqrSumR += (*tree_)[tree_->getChild(rootId, i)].expectReturn * (*tree_)[tree_->getChild(rootId, i)].expectReturn;
                minR = std::min((*tree_)[tree_->getChild(rootId, i)].expectReturn, minR);
            }

        }

        float avgR = childNum == 0 ? 0 : sumR / childNum;
        float stdR =  childNum == 0 ? 0 : sqrtf(sqrSumR / childNum - avgR * avgR);
        float minWeight = childNum == 0 ? 1 : stdR * explorationRate;

        splitWeight->initialize(minWeight);
        for(int i = 0; i < childNum; ++i){
            int splitIndex = (*tree_)[tree_->getChild(rootId, i)].featureIndex;
            if((*tree_)[tree_->getChild(rootId, i)].isSuccess){
                if(!(*tree_)[tree_->getChild(rootId, i)].isLeft)
                    splitIndex += (splitWeight->getSize() >> 1);
                float deltaR = (*splitWeight)[splitIndex];
                deltaR += ((*tree_)[tree_->getChild(rootId, i)].expectReturn - minR);
                (*splitWeight)[splitIndex] = deltaR;
            } else {
                (*splitWeight)[splitIndex] = 0;
            }
        }
    }

    template <class F>
    void mendScore(int leafId, float lr, float step, float tiredCoff, F&& f, int stepNum){
        std::lock_guard<std::mutex> lock{mutex_};
        int preId = tree_->getParent(leafId);
        //当某个叶节点被尝试了很多次，就会疲劳，于是给它扣分，以便算法能有更多精力尝试别的
        int currentLeafId = leafId;
        while(preId != -1){
            int chidNum = tree_->getChildNum(preId);
            //float r = (*tree_)[leafId].getReward(&(*tree_)[preId], isConsiderRisk);
            float r = f((*tree_)[preId].timeStep, (*tree_)[leafId].timeStep, stepNum);
            float nextR = -FLT_MAX;
            for(int i = 0; i < chidNum; ++i){
                nextR = std::max((*tree_)[tree_->getChild(preId, i)].expectReturn, nextR);
            }
            (*tree_)[preId].expectReturn = (*tree_)[preId].expectReturn + lr * (r + step * nextR - (*tree_)[preId].expectReturn);
            leafId = preId;
            preId = tree_->getParent(leafId);
        }
        (*tree_)[currentLeafId].expectReturn -= tiredCoff;
    }
#endif

    int growFeature(int rootId, IVector<IBaseIterator<float>*>* featureList, int featureIndex, bool isLeft,
                    IBaseIterator<float>* weight, IBaseIterator<float>* target, IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, int& skipLen,
                    int barSize, float gamma, float lambda, float minWeight, int stepNum){
        int childNum = tree_->getChildNum(rootId);
        int newNodeId = -1;
        for(int i = 0; i < childNum; ++i){
            if((*tree_)[tree_->getChild(rootId, i)].featureIndex == featureIndex && (*tree_)[tree_->getChild(rootId, i)].isLeft == isLeft){
                newNodeId = tree_->getChild(rootId, i);
                //之前尝试分裂过这个节点，但是失败了，现在已经没有尝试必要
                if(!(*tree_)[newNodeId].isSuccess)
                    return -1;
                updateSkip(newNodeId, featureList, skip, skipLen);
                return newNodeId;
            }
        }

        //之前没有分裂过，开始分裂--------------------------------------------------------------------
        //如果有线程正在分裂当前这个节点，返回失败，一个节点不能同时被多个线程分裂！（他孩子的分裂不受影响）
        if(!tree_->lock(rootId))
            return -1;
        //无论是否成功都加一个节点
        newNodeId = tree_->createNode();
        (*tree_)[newNodeId].isSuccess = false;
        IBaseIterator<float>* feature = (*featureList)[featureIndex];
        int barId = splitBars_->useCacheMemory();
        SplitBarList& bars = splitBars_->getCacheMemory(barId);
        bars.initialize(barSize);
        (*tree_)[newNodeId].featureIndex = featureIndex;
        if(!fillBars(feature, weight, g, h, skip, skipLen, bars.bars, barSize, bars.startValue, bars.deltaStd)){
            splitBars_->releaseCacheMemory(barId);
            tree_->addChild(rootId, newNodeId);
            //别忘记解锁！！！！！！！
            tree_->unlock(rootId);
            return -1;
        }
        auto res = splitBars(bars.bars, barSize, bars.startValue, bars.deltaStd, gamma, lambda);
        splitBars_->releaseCacheMemory(barId);
        if(!res.isSlpit()){
            tree_->addChild(rootId, newNodeId);
            //别忘记解锁！！！！！！！
            tree_->unlock(rootId);
            return -1;
        }


        if(isLeft){
            if(res.leftWeightSum < minWeight){
                tree_->addChild(rootId, newNodeId);
                //别忘记解锁！！！！！！！
                tree_->unlock(rootId);
                return -1;
            }

        } else {
            if(res.rightWeightSum < minWeight){
                tree_->addChild(rootId, newNodeId);
                //别忘记解锁！！！！！！！
                tree_->unlock(rootId);
                return -1;
            }
        }


        (*tree_)[newNodeId].splitValue = res.splitValue;
        (*tree_)[newNodeId].isLeft = isLeft;
        updateSkip(newNodeId, featureList, skip, skipLen);
        updateTarget(newNodeId, skip, skipLen, weight, target, stepNum);
#ifdef Q_LEARNING
        (*tree_)[newNodeId].expectReturn = 0;
#endif
        (*tree_)[newNodeId].isSuccess = true;
        tree_->addChild(rootId, newNodeId);
        tree_->unlock(rootId);
        return newNodeId;
    }

    void updateSkip(int toNodeId, IVector<IBaseIterator<float> *> *featureList, int* skip, int& skipLen){
        int featureIndex = (*tree_)[toNodeId].featureIndex;
        bool isLeft = (*tree_)[toNodeId].isLeft;
        float splitValue = (*tree_)[toNodeId].splitValue;
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

    void updateTarget(int nodeId, int* skip, int skipLen, IBaseIterator<float>* weight, IBaseIterator<float>* target, int stepNum){
        for(int i = 0; i < stepNum; ++i){
            (*tree_)[nodeId].timeStep[i].targetSum = 0;
            (*tree_)[nodeId].timeStep[i].targetSqrSum = 0;
            (*tree_)[nodeId].timeStep[i].weightSum = 0;
        }

        float timeSpan = target->size() / float(stepNum);

        int index = 0;
        for(int i = 0; i < skipLen; ++i){
            weight->skip(skip[i]);
            target->skip(skip[i]);
            index += skip[i];
            int sid = (int)(index / timeSpan);
            (*tree_)[nodeId].timeStep[sid].targetSum += (**weight) * (**target);
            (*tree_)[nodeId].timeStep[sid].targetSqrSum += powf((**weight) * (**target), 2);
            (*tree_)[nodeId].timeStep[sid].weightSum += (**weight);
        }

        //while (target->isValid()){
        //    if(**flag == nodeId){
        //        int sid = (int)(index / timeSpan);
        //        (*tree_)[nodeId].timeStep[sid].targetSum += (**weight) * (**target);
        //        (*tree_)[nodeId].timeStep[sid].targetSqrSum += powf((**weight) * (**target), 2);
        //        (*tree_)[nodeId].timeStep[sid].weightSum += (**weight);
                //(*tree_)[nodeId].targetSum += (**weight) * (**target);
                //(*tree_)[nodeId].targetSqrSum += powf((**weight) * (**target), 2);
                //(*tree_)[nodeId].weightSum += (**weight);
        //    }
        //    ++*weight;
        //    ++*flag;
        //    ++*target;
        //    ++index;
        //}

        weight->skip(0, false);
        target->skip(0, false);
        //cout<<(*tree_)[nodeId].targetSum/(*tree_)[nodeId].weightSum<<" "<<(*tree_)[nodeId].weightSum<<endl;
    }

    void cleanTarger(int nodeId, int stepIndex){
        for(int i = 0; i < tree_->getChildNum(nodeId); ++i)
            cleanTarger(tree_->getChild(nodeId, i), stepIndex);
        (*tree_)[nodeId].timeStep[stepIndex].targetSum = 0;
        (*tree_)[nodeId].timeStep[stepIndex].targetSqrSum = 0;
        (*tree_)[nodeId].timeStep[stepIndex].weightSum = 0;
    }

    template <class F>
    float cleanTree(int rootId, int nodeId, float threshold, F&& f, int startStepIndex, int stepNum){
        //cout<<"clean "<<nodeId<<endl;
        //float curReward = (*tree_)[nodeId].getReward(&(*tree_)[rootId], isConsiderRisk);
        float curReward = f((*tree_)[rootId].timeStep + startStepIndex, (*tree_)[nodeId].timeStep + startStepIndex, stepNum);
        float maxReward = curReward;
        //cout<<curReward<<" "<<tree_->getChildNum(nodeId)<<endl;
        for(int i = tree_->getChildNum(nodeId) - 1; i >= 0; --i){
            float reward = cleanTree(rootId, tree_->getChild(nodeId, i), threshold, f, startStepIndex, stepNum);
            int childId = tree_->getChild(nodeId, i);

            float weight = 0;
            for(int j = 0; j < stepNum; ++j)
                weight += (*tree_)[childId].timeStep[j + startStepIndex].weightSum;
            //孩子数据太少没办法验证
            if(weight == 0){
                tree_->removeChild(nodeId, i);
                continue;
            }

            //孩子提升不够或者一代不如一代
            if(reward < threshold || reward < curReward){
                tree_->removeChild(nodeId, i);
                continue;
            }


            maxReward = std::max(maxReward, reward);
        }
        return maxReward;
    }

    void curEval(int rootId, IVector<IBaseIterator<float>*>* feature, IBaseIterator<float>* weight, IBaseIterator<float>* target, int stepIndex){

        float featureValue = 0;
        int featureIndex = (*tree_)[rootId].featureIndex;
        if(featureIndex >=0 )//如果不是根节点
            featureValue = **(*feature)[featureIndex];

        auto targetValue = *(*target);
        auto weightValue = *(*weight);
        if( featureIndex < 0 ||
            (featureValue < (*tree_)[rootId].splitValue && (*tree_)[rootId].isLeft) ||
            (featureValue >= (*tree_)[rootId].splitValue && !(*tree_)[rootId].isLeft)){
            (*tree_)[rootId].timeStep[stepIndex].targetSum += targetValue * weightValue;
            (*tree_)[rootId].timeStep[stepIndex].targetSqrSum += powf(targetValue * weightValue, 2);
            (*tree_)[rootId].timeStep[stepIndex].weightSum += weightValue;
            for(int i = 0; i < tree_->getChildNum(rootId); ++i){
                curEval(tree_->getChild(rootId, i), feature, weight, target, stepIndex);
            }
        }

    }

    template <class F>
    void curPred(int rootId, float* pred, IVector<IBaseIterator<float>*>* feature, F&& f, int startStepIndex, int stepNum){
        int id = 0;
        while ((*feature)[0]->isValid()){
            pred[id++] += curPred(rootId, rootId, feature, f, startStepIndex, stepNum);
            for(int i = 0; i < feature->getSize(); ++i){
                (*feature)[i]->skip(1);
            }
        }
        for(int i = 0; i < feature->getSize(); ++i){
            (*feature)[i]->skip(0, false);
        }
    }

    template <class F>
    float curPred(int rootId, int nodeId, IVector<IBaseIterator<float>*>* feature, F&& f, int startStepIndex, int stepNum){
        float featureValue = 0;
        int featureIndex = (*tree_)[nodeId].featureIndex;
        if(featureIndex >= 0)//如果不是根节点
            featureValue = **(*feature)[featureIndex];

        if( featureIndex < 0 ||
            (featureValue < (*tree_)[nodeId].splitValue && (*tree_)[nodeId].isLeft) ||
            (featureValue >= (*tree_)[nodeId].splitValue && !(*tree_)[nodeId].isLeft)){

            float predSum = 0;
            float predNum = 0;
            for(int i = 0; i < tree_->getChildNum(nodeId); ++i){
                int childId = tree_->getChild(nodeId, i);
                float pred = curPred(rootId, childId, feature, f, startStepIndex, stepNum);
                if(pred > 0){
                    predSum += pred;
                    predNum += 1;
                }
            }
            //return tree_->getChildNum(nodeId) == 0 ? f((*tree_)[rootId].timeStep + startStepIndex, (*tree_)[nodeId].timeStep + startStepIndex, stepNum) : predSum / max(predNum, 0.001f);
            if(tree_->getChildNum(nodeId) == 0){
                float score = FLT_MAX;
                for(int i = 0; i <= startStepIndex; ++i){
                    score = fminf(f(nullptr, (*tree_)[nodeId].timeStep + i, 1), score);
                }
                return score;
            }
            return predSum / max(predNum, 0.001f);
        }
        return 0;
    }

    /*
    template <class F>
    float curPred(int rootId, int nodeId, IVector<float>* feature, F&& f, int startStepIndex, int stepNum){
        float featureValue = 0;
        int featureIndex = (*tree_)[nodeId].featureIndex;
        if(featureIndex >= 0)//如果不是根节点
            featureValue = (*feature)[featureIndex];

        if( featureIndex < 0 ||
            (featureValue < (*tree_)[nodeId].splitValue && (*tree_)[nodeId].isLeft) ||
            (featureValue >= (*tree_)[nodeId].splitValue && !(*tree_)[nodeId].isLeft)){

            float predSum = 0;
            float predNum = 0;
            for(int i = 0; i < tree_->getChildNum(nodeId); ++i){
                int childId = tree_->getChild(nodeId, i);
                float pred = curPred(rootId, childId, feature, f, startStepIndex, stepNum);
                if(pred > 0){
                    predSum += pred;
                    predNum += 1;
                }
            }
            return tree_->getChildNum(nodeId) == 0 ? f((*tree_)[rootId].timeStep + startStepIndex, (*tree_)[nodeId].timeStep + startStepIndex, stepNum) : predSum / max(predNum, 0.001f);
        }
        return 0;
    }*/

    //给叶节点标号
    void createLeafIndex(int nodeId, map<int, int>& node2index, map<int, int>& index2node, int& curIndex){
        if(tree_->getChildNum(nodeId) == 0){
            node2index[nodeId] = curIndex;
            index2node[curIndex] = nodeId;
            ++curIndex;
        } else {
            for(int i = 0; i < tree_->getChildNum(nodeId); ++i){
                createLeafIndex(tree_->getChild(nodeId, i), node2index, index2node, curIndex);
            }
        }
    }

    //生成相关矩阵
    void fillCorrMat(int rootId, map<int, int>& node2index, IVector<IBaseIterator<float>*>* feature, IBaseIterator<float>* weight, float* outMat, float* weightSum){
        int leafNum = node2index.size();
        bool* flag = new bool[leafNum];
        float* curFeature = new float[feature->getSize()];

        while (weight->isValid()){
            memset(flag, 0, leafNum * sizeof(bool));
            float curWeight = **weight;
            weight->skip(1);
            for(int i = 0; i < feature->getSize(); ++i){
                curFeature[i] = **(*feature)[i];
                (*feature)[i]->skip(1);
            }
            fillFlag(rootId, node2index, curFeature, flag);

//            if(leafNum < 5){
//                for(int i = 0; i < leafNum; ++i){
//                    cout<<flag[i]<<" ";
//                }
//                cout<<endl;
//            }

            for(int i = 0; i < leafNum; ++i)
                if(flag[i])
                    weightSum[i] += curWeight;

            for(int i = 0; i < leafNum -1; ++i){
                if(flag[i]){
                    for(int j = i + 1; j < leafNum; ++j){
                        if(flag[j]){
                            outMat[i * leafNum + j] += curWeight;
                            outMat[j * leafNum + i] += curWeight;
                        }
                    }
                }
            }
        }

//        if(leafNum < 5){
//            for(int i = 0; i < leafNum; ++i){
//                for(int j = 0; j < leafNum; ++j){
//                    cout<<outMat[i * leafNum + j]<<" ";
//                }
//                cout<<endl;
//            }
//        }


        weight->skip(0, false);
        for(int i = 0; i < feature->getSize(); ++i)
            (*feature)[i]->skip(0, false);
        delete []flag;
        delete []curFeature;
    }
    void fillFlag(int nodeId, map<int, int>& node2index, const float* feature, bool* flag){
        float featureValue = 0;
        int featureIndex = (*tree_)[nodeId].featureIndex;
        if(featureIndex >= 0)//如果不是根节点
            featureValue = feature[featureIndex];

        if( featureIndex < 0 ||
            (featureValue < (*tree_)[nodeId].splitValue && (*tree_)[nodeId].isLeft) ||
            (featureValue >= (*tree_)[nodeId].splitValue && !(*tree_)[nodeId].isLeft)){

            if(tree_->getChildNum(nodeId) > 0){
                for(int i = 0; i < tree_->getChildNum(nodeId); ++i){
                    int childId = tree_->getChild(nodeId, i);
                    fillFlag(childId, node2index, feature, flag);
                }
            } else {
                int index = node2index[nodeId];
                flag[index] = true;
            }
        }
    }


    static int randomChooseSplit(IVector<float>* splitWeight){
        float sumP = 0;
        for(size_t i = 0; i < splitWeight->getSize(); ++i){
            sumP += (*splitWeight)[i];
        }
        float rValue = rand() / (float)RAND_MAX;
        rValue *= sumP;
        int curIndex = 0;
        for(size_t i = 0; i < splitWeight->getSize(); ++i){
            rValue -= (*splitWeight)[i];
            if(rValue <= 0)
                return curIndex;
            ++curIndex;
        }
        return std::min(curIndex - 1, (int)splitWeight->getSize() - 1);
    }

    DCache<SplitBarList>* splitBars_ = {nullptr};
    DTree<Node, DTREE_BLOCK_SIZE>* tree_ = {nullptr};
    //线程安全
    std::mutex mutex_;
};

#endif //ALPHATREE_RFTBASE_H
