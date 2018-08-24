//
// Created by godpgf on 18-1-30.
// random filter tree
// 多线程多批次操作base random filter tree
//

#ifndef ALPHATREE_RFT_H
#define ALPHATREE_RFT_H

#include "hashmap.h"
#include "threadpool.h"
#include "../loss.h"
#include "rftbase.h"
#include <iostream>

//不考虑稳定性
float baseRewardFun(NodeTimeStep* preNode, NodeTimeStep* curNode, int stepNum){
    float preTargetSum = 0;
    float preWeightSum = 0;
    float curTargetSum = 0;
    float curWeightSum = 0;
    for(int i = 0; i < stepNum; ++i){
        if(preNode != nullptr){
            preTargetSum += preNode[i].targetSum;
            preWeightSum += preNode[i].weightSum;
        }
        curTargetSum += curNode[i].targetSum;
        curWeightSum += curNode[i].weightSum;
    }
    if(curWeightSum < 0.00001)
        return 0;
    return curTargetSum / curWeightSum - preTargetSum / std::max(preWeightSum, 0.0001f);
}

//考虑稳定性
float stableRewardFun(NodeTimeStep* preNode, NodeTimeStep* curNode, int stepNum){
    float preSum = 0;
    float curSum = 0;
    float preSqrSum = 0;
    float curSqrSum = 0;
    for(int i = 0; i < stepNum; ++i){
        float tmp = preNode[i].targetSum/std::max(preNode[i].weightSum,0.00001);
        preSum += tmp / stepNum;
        preSqrSum += tmp * tmp / stepNum;
        tmp = curNode[i].targetSum / std::max(curNode[i].weightSum, 0.00001);
        curSum += tmp / stepNum;
        curSqrSum += tmp * tmp / stepNum;
    }
    if(preSqrSum - preSum * preSum <= 0)
        return 0;
    if(curSqrSum - curSum * curSum <= 0)
        return 0;
    float reward = curSum / sqrtf(curSqrSum - curSum * curSum) - preSum / sqrtf(preSqrSum - preSum * preSum);
    //cout<<curSum<<"/"<<sqrtf(curSqrSum - curSum * curSum)<<" "<<preSum<<"/"<<sqrtf(preSqrSum - preSum * preSum)<<"="<<reward<<endl;
    return reward;
}

template <int DTREE_BLOCK_SIZE = 32>
class RFT : public BaseRFT<DTREE_BLOCK_SIZE>{
public:
    static RFT<DTREE_BLOCK_SIZE>* create(){ return new RFT<DTREE_BLOCK_SIZE>();}
    static void release(RFT<DTREE_BLOCK_SIZE> * rft){delete rft;}

    void initialize(IBaseIterator<float>* weight, IBaseIterator<float>* target, IOBaseIterator<float>* lastPred, IOBaseIterator<float>* g, IOBaseIterator<float>* h, const char* lossFunName = "binary:logistic", int stepNum = 1){
        lastPred->initialize(0);
        //计算h、g
        auto opt = lossFun[lossFunName];
        opt(g, h, target, lastPred);

        /*{
            flag->initialize(0);
            //测试分类分裂的准确性
            for(int i = 0; i < featureList->getSize(); ++i){
                IBaseIterator<float>* feature = (*featureList)[i];
                int barId = BaseRFT<DTREE_BLOCK_SIZE>::splitBars_->useCacheMemory();
                SplitBarList& bars = BaseRFT<DTREE_BLOCK_SIZE>::splitBars_->getCacheMemory(barId);
                bars.initialize(barSize_);
                if(!fillBars(feature, weight, g, h, flag, 0, bars.bars, barSize_, bars.startValue, bars.deltaStd)){
                    BaseRFT<DTREE_BLOCK_SIZE>::splitBars_->releaseCacheMemory(barId);
                    cout<<"不能分裂\n";
                    return;
                }
                auto res = splitBars(bars.bars, barSize_, bars.startValue, bars.deltaStd, gamma, lambda);
                auto testRes = testSplitValue2LogLoss(bars.bars, barSize_, bars.startValue, bars.deltaStd, feature, weight, lastPred, target, flag, 0);
                cout<<res.splitValue<<" "<<testRes.splitValue<<endl;
                BaseRFT<DTREE_BLOCK_SIZE>::splitBars_->releaseCacheMemory(barId);
            }

        }*/

        rootIds_.resize(0);
        int rootId = BaseRFT< DTREE_BLOCK_SIZE>::createRoot(weight, target, stepNum);
        rootIds_[0] = rootId;
    }

    /*
     * featureList是所有特征
     * splitWeight是每个特征向左分裂的权重+每个特征向右分裂的权重。比如某个人的年龄是个特征，发现年龄越大target越大，向左分裂的权重就是0.1,向右分裂的权重就是0.9,即更有可能选择右边的数据块
     * weight是每个元素的权重，暂时都填1,以后再优化
     * target是目标值，我们的目标就算尽量切割出某个数据块，让target的均值最大化
     * lastPred是上棵树的预测，目前只支持一棵树，就全填0就行
     * tmpG、tmpH、tmpFlagList是中间结果的内存，这部分内存可能很大，所以不让底层代码管理它
     * minWeight是筛选数据集合的最小权重和，小于它将不再被筛
     * iteratorNum是反复过滤的次数，目前只能是一，以后有时间再优化
     * lossFunName决定算法的损失函数
     * epochPerGC决定多少是epoch做一次垃圾回收
     * gcThreshold
     */
//    void train(IVector<IBaseIterator<float>*>* featureList, IVector<float>* splitWeight, IBaseIterator<float>* weight, IBaseIterator<float>* target,
//               IOBaseIterator<float>* lastPred, IOBaseIterator<float>* g, IOBaseIterator<float>* h, IOBaseIterator<int>* flag,
//               float gamma, float lambda, float minWeight = 256, int epochNum = 30000, int iteratorNum = 1, const char* lossFunName = "binary:logistic",
//               int epochPerGC = 128, float gcThreshold = 0.0f
//#ifdef Q_LEARNING
//            ,float lr = 0.1f, float step = 0.8f
//#endif
//    ){
//        initizlize();
//        int remainGCTime = epochPerGC;
//        while (epochNum > 0){
//            if(epochNum % 10 == 0)
//                cout<<epochNum<<endl;
//#ifdef Q_LEARNING
//            BaseRFT<DTREE_BLOCK_SIZE>::update(rootId, featureList, splitWeight, weight, target, g, h, flag, barSize_, gamma, lambda, minWeight, isConsiderRisk_, lr, step);
//#else
//            BaseRFT<DTREE_BLOCK_SIZE>::update(rootId, featureList, splitWeight, weight, target, g, h, flag, barSize_, gamma, lambda, minWeight);
//#endif
//            --epochNum;
//
//            垃圾回收，节省内存
//            --remainGCTime;
//            if(remainGCTime <= 0){
//                remainGCTime = epochPerGC;
//                BaseRFT< DTREE_BLOCK_SIZE>::cleanTree(rootId, gcThreshold, isConsiderRisk_);
//            }
//        }


//    }

    void train(IVector<IBaseIterator<float>*>* featureList, IVector<IVector<float>*>* splitWeights, IBaseIterator<float>* weight, IBaseIterator<float>* target,
               IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, int splitBarSize,
               float gamma, float lambda, float minWeight, int epochNum = 30000, const char* rewardFunName = "base", int stepNum = 1
#ifdef Q_LEARNING
            ,float lr = 0.1f, float step = 0.8f, float tiredCoff = 0.016
#endif
    ){
        auto reward = rewardFun[rewardFunName];
        ThreadPool threadPool(splitWeights->getSize());
        std::shared_future<bool>* res = new std::shared_future<bool>[splitWeights->getSize()];

        Vector<Vector<IBaseIterator<float>*>*> featureListClone( splitWeights->getSize());
        Vector<IBaseIterator<float>*> weightClone(splitWeights->getSize());
        Vector<IBaseIterator<float>*> targetClone(splitWeights->getSize());
        Vector<IBaseIterator<float>*> gClone(splitWeights->getSize());
        Vector<IBaseIterator<float>*> hClone(splitWeights->getSize());

        for(int i = 0; i < splitWeights->getSize(); ++i){
            featureListClone[i] = new Vector<IBaseIterator<float>*>( featureList->getSize());
            for(int j = 0; j < featureList->getSize(); ++j){
                (*featureListClone[i])[j] = (*featureList)[j]->clone();
            }
            weightClone[i] = weight->clone();
            targetClone[i] = target->clone();
            gClone[i] = g->clone();
            hClone[i] = h->clone();
        }

        while (epochNum > 0){
            cout<<epochNum<<endl;
            for(int i = 0; i < splitWeights->getSize(); ++i){
                IVector<IBaseIterator<float>*>* curFeatureList = featureListClone[i];
                IBaseIterator<float>* curWeight = weightClone[i];
                IBaseIterator<float>* curTarget = targetClone[i];
                IBaseIterator<float>* curG = gClone[i];
                IBaseIterator<float>* curH = hClone[i];
                IVector<float>* curSplitWeight = (*splitWeights)[i];
                int* curSkip = skip + i * curWeight->size();
                auto* rft = this;
                res[i] = threadPool.enqueue([rft, curFeatureList, curSplitWeight, curWeight, curTarget, curG, curH, curSkip, splitBarSize, gamma, lambda, minWeight, reward, stepNum, lr, step, tiredCoff]{
                    rft->updateTree(0, curFeatureList, curSplitWeight, curWeight, curTarget, curG, curH, curSkip, splitBarSize, gamma, lambda, minWeight, reward, stepNum, lr, step, tiredCoff);
                    return true;
                }).share();
            }
            for(int i = 0; i < splitWeights->getSize(); ++i){
                res[i].get();
            }
            epochNum -= splitWeights->getSize();
        }
        delete []res;

        for(int i = 0; i < splitWeights->getSize(); ++i){
            for(int j = 0; j < featureList->getSize(); ++j){
                delete (*featureListClone[i])[j];
            }
            delete featureListClone[i];
            delete weightClone[i];
            delete targetClone[i];
            delete gClone[i];
            delete hClone[i];
        }
    }

    template <class F>
    void updateTree(int treeIndex, IVector<IBaseIterator<float>*>* featureList, IVector<float>* splitWeight, IBaseIterator<float>* weight, IBaseIterator<float>* target,
                    IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, int splitBarSize,
                    float gamma, float lambda, float minWeight, F&& reward, int stepNum = 1
#ifdef Q_LEARNING
            ,float lr = 0.1f, float step = 0.8f, float tiredCoff = 0.016
#endif
    ){

#ifdef Q_LEARNING
            BaseRFT<DTREE_BLOCK_SIZE>::update(rootIds_[treeIndex], featureList, splitWeight, weight, target, g, h, skip, splitBarSize, gamma, lambda, minWeight, reward, stepNum, lr, step, tiredCoff);
#else
            BaseRFT<DTREE_BLOCK_SIZE>::update(rootIds_[treeIndex], featureList, splitWeight, weight, target, g, h, flag, barSize_, gamma, lambda, minWeight);
#endif
    }

    void cleanTree(int treeId, float threshold, const char* rewardFunName = "base", int startStepIndex = 0, int stepNum = 1){
        auto reward = rewardFun[rewardFunName];
        BaseRFT< DTREE_BLOCK_SIZE>::cleanTree(rootIds_[treeId], threshold, reward, startStepIndex, stepNum);
    }

    virtual bool cleanCorrelation(int treeId, IVector<IBaseIterator<float>*>* featureList, IBaseIterator<float>* weight, float corrPercent = 0.36f, bool isCorrAll = false){
        return BaseRFT< DTREE_BLOCK_SIZE>::cleanCorrelation(rootIds_[treeId], featureList, weight, corrPercent, isCorrAll);
    }

    void eval(IVector<IBaseIterator<float>*>* featureList, IBaseIterator<float>* weight, IBaseIterator<float>* target, int stepIndex = 1){
        for(int i = 0; i < rootIds_.getSize(); ++i) {
            BaseRFT< DTREE_BLOCK_SIZE>::eval(rootIds_[i], featureList, weight, target, stepIndex);
        }
    }

    void pred(IVector<IBaseIterator<float>*>* featureList, float* predOut, const char* rewardFunName = "base", int startStepIndex = 0, int stepNum = 1){
        auto reward = rewardFun[rewardFunName];
        memset(predOut,0, sizeof(float) * (*featureList)[0]->size());
        for(int i = 0; i < rootIds_.getSize(); ++i)
            BaseRFT<DTREE_BLOCK_SIZE>::curPred(rootIds_[i], predOut, featureList, reward, startStepIndex, stepNum);
    }

    /*
    virtual float pred(int treeId, IVector<float>* feature, const char* rewardFunName = "base", int startStepIndex = 0, int stepNum = 1){
        auto reward = rewardFun[rewardFunName];
        return BaseRFT< DTREE_BLOCK_SIZE>::curPred(rootIds_[treeId], rootIds_[treeId], feature, reward, startStepIndex, stepNum);
    }*/

    string tree2str(IVector<string>* nameList, const char* rewardFunName = "base", int startStepIndex = 0, int stepNum = 1){
        string str = "";
        auto f = rewardFun[rewardFunName];
        for(int i = 0; i < rootIds_.getSize(); ++i){
            string curStr= "";
            string spaceStr = "\t";

            tostring(curStr, rootIds_[i], rootIds_[i], nameList, 0, spaceStr, f, startStepIndex, stepNum);
            str += curStr;
        }
        return str;
    }

    void saveModel(const char* path){
        ofstream file;
        file.open(path, ios::binary);
        //1、写入树数
        int treeNum = rootIds_.getSize();
        file.write(reinterpret_cast<const char * >( &treeNum ), sizeof(int));
        for(int i = 0; i < treeNum; ++i){
            int nodeNum = BaseRFT< DTREE_BLOCK_SIZE>::tree_->getNodeNum(rootIds_[i]);
            file.write(reinterpret_cast<const char * >( &nodeNum ), sizeof(int));
            int nodeIndex = 0;
            saveTree(file, rootIds_[i], nodeIndex);
        }
        file.close();
    }

    void loadModel(const char* path){
        BaseRFT< DTREE_BLOCK_SIZE>::tree_->releaseAll();
        rootIds_.clear();
        ifstream file;
        file.open(path, ios::binary);
        int treeNum;
        file.read(reinterpret_cast< char* >( &treeNum ), sizeof( int ));
        int offset = 0;
        for(int i = 0; i < treeNum; ++i){
            int nodeNum;
            file.read(reinterpret_cast< char* >( &nodeNum ), sizeof( int ));

            for(int j = 0; j < nodeNum; ++j){
                int nodeId = BaseRFT< DTREE_BLOCK_SIZE>::tree_->createNode();

                int childNum;
                file.read(reinterpret_cast< char* >( &childNum ), sizeof( int ));
                for(int k = 0; k < childNum; ++k){
                    int childIndex;
                    file.read(reinterpret_cast< char* >( &childIndex ), sizeof( int ));
                    BaseRFT< DTREE_BLOCK_SIZE>::tree_->addChild(nodeId, offset + childIndex);
                }
                file.read(reinterpret_cast< char* >( &(*BaseRFT<DTREE_BLOCK_SIZE>::tree_)[nodeId].featureIndex ), sizeof( int ));
                file.read(reinterpret_cast< char* >( &(*BaseRFT<DTREE_BLOCK_SIZE>::tree_)[nodeId].splitValue ), sizeof( float ));
                file.read(reinterpret_cast< char* >( &(*BaseRFT<DTREE_BLOCK_SIZE>::tree_)[nodeId].isLeft ), sizeof( bool ));
                for(int i = 0; i < MAX_TIME_STEP; ++i){
                    file.read(reinterpret_cast< char* >( &(*BaseRFT<DTREE_BLOCK_SIZE>::tree_)[nodeId].timeStep[i].targetSum ), sizeof(double ));
                    file.read(reinterpret_cast< char* >( &(*BaseRFT<DTREE_BLOCK_SIZE>::tree_)[nodeId].timeStep[i].targetSqrSum ), sizeof( double));
                    file.read(reinterpret_cast< char* >( &(*BaseRFT<DTREE_BLOCK_SIZE>::tree_)[nodeId].timeStep[i].weightSum ), sizeof( double ));
                }

                file.read(reinterpret_cast< char* >( &(*BaseRFT<DTREE_BLOCK_SIZE>::tree_)[nodeId].expectReturn ), sizeof( float ));

                if(j == nodeNum - 1){
                    rootIds_[i] = nodeId;
                }
            }
            offset += nodeNum;
        }
        file.close();
    }
protected:
    RFT():BaseRFT<DTREE_BLOCK_SIZE>(){
        if(lossFun.getSize() == 0){
            //加入损失函数
            lossFun.add("binary:logistic", binaryLogisticLossGrad);
            lossFun.add("regression", regressionLossGrad);
            rewardFun.add("base", baseRewardFun);
            rewardFun.add("stable", stableRewardFun);
        }
        ++objNum_;
    }

    virtual ~RFT(){
        --objNum_;
        if(objNum_ == 0){
            lossFun.clear();
            rewardFun.clear();
        }
    }

    template <class F>
    void tostring(string& str, int rootId, int nodeId, IVector<string>*nameList, int space, const string& spaceStr, F&& f, int stepStartIndex, int stepNum){
        for(int i = 0; i < space; ++i)
            str += spaceStr;
        if((*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[nodeId].featureIndex == -1){
            str += "tree:";
        } else {
            int featureIndex = (*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[nodeId].featureIndex;
            str += (*nameList)[featureIndex];
            if((*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[nodeId].isLeft){
                str += " < ";
            } else {
                str += " >= ";
            }
            str += std::to_string((*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[nodeId].splitValue);

            if(BaseRFT< DTREE_BLOCK_SIZE>::tree_->getChildNum(nodeId) == 0){
                str += " weight sum: ";
                float weightSum = 0;
                for(int i = 0; i < stepNum; ++i){
                    weightSum += (*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[nodeId].timeStep[i + stepStartIndex].weightSum;
                }
                str += std::to_string(weightSum);
                str += " mean: ";
                //str += std::to_string(f((*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[rootId].timeStep + stepStartIndex, (*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[nodeId].timeStep + stepStartIndex, stepNum));
                float score = FLT_MAX;
                for(int i = 0; i <= stepStartIndex; ++i){
                    score = fminf(f(nullptr, (*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[nodeId].timeStep + i, 1), score);
                }

                str += std::to_string(score);
            }

        }

        str += "\n";
        for(int i = 0;i < BaseRFT<DTREE_BLOCK_SIZE>::tree_->getChildNum(nodeId); ++i){
            tostring(str, rootId, BaseRFT< DTREE_BLOCK_SIZE>::tree_->getChild(nodeId, i), nameList, space + 1, spaceStr, f, stepStartIndex, stepNum);
        }
    }

    int saveTree(ofstream& file, int rootId, int& nodeIndex){
        int childNum = BaseRFT< DTREE_BLOCK_SIZE>::tree_->getChildNum(rootId);
        int* children = new int[childNum];
        for(int i = 0; i < childNum; ++i){
            children[i] = saveTree(file, BaseRFT< DTREE_BLOCK_SIZE>::tree_->getChild(rootId, i), nodeIndex);
        }

        //写入节点
        file.write(reinterpret_cast<const char * >( &childNum ), sizeof(int));
        for(int i = 0; i < childNum; ++i){
            file.write(reinterpret_cast<const char * >( &children[i] ), sizeof(int));
        }
        delete[] children;
        file.write(reinterpret_cast<const char * >( &(*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[rootId].featureIndex), sizeof(int));
        file.write(reinterpret_cast<const char * >( &(*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[rootId].splitValue), sizeof(float));
        file.write(reinterpret_cast<const char * >( &(*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[rootId].isLeft ), sizeof(bool));
        for(int i = 0; i < MAX_TIME_STEP; ++i){
            file.write(reinterpret_cast<const char * >( &(*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[rootId].timeStep[i].targetSum ), sizeof(double));
            file.write(reinterpret_cast<const char * >( &(*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[rootId].timeStep[i].targetSqrSum ), sizeof(double));
            file.write(reinterpret_cast<const char * >( &(*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[rootId].timeStep[i].weightSum ), sizeof(double));
        }


        file.write(reinterpret_cast<const char * >( &(*BaseRFT< DTREE_BLOCK_SIZE>::tree_)[rootId].expectReturn ), sizeof(float));
        return nodeIndex++;
    }

    DArray<int, 4> rootIds_;

    static HashMap<void(*)(OBaseIterator<float>*, OBaseIterator<float>*, IBaseIterator<float>*, IBaseIterator<float>*), 8, 16> lossFun;
    static HashMap<float(*)(NodeTimeStep*, NodeTimeStep*, int), 8, 16> rewardFun;
    static int objNum_;
};

template <int DTREE_BLOCK_SIZE>
HashMap<void(*)(OBaseIterator<float>*, OBaseIterator<float>*, IBaseIterator<float>*, IBaseIterator<float>*), 8, 16> RFT<DTREE_BLOCK_SIZE>::lossFun;

template <int DTREE_BLOCK_SIZE>
HashMap<float(*)(NodeTimeStep*, NodeTimeStep*, int), 8, 16> RFT<DTREE_BLOCK_SIZE>::rewardFun;

template <int DTREE_BLOCK_SIZE>
int RFT<DTREE_BLOCK_SIZE>::objNum_ = 0;

#endif //CART_RFT_H