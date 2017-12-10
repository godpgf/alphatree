//
// Created by yanyu on 2017/11/24.
//

#ifndef ALPHATREE_FILTERTREE_H
#define ALPHATREE_FILTERTREE_H

#include <stddef.h>
#include "basefiltertree.h"

namespace fb {
    const size_t MAX_SAMPLE_SIZE = 1024 * 1024;
    const size_t MAX_FEATURE_SIZE = 512;
    const size_t MAX_FEATURE_NAME_LEN = 128;
    const size_t MAX_TREE_SIZE = 64;

    class BaseFilterCache {
    public:
        BaseFilterCache(){
            target = new float[MAX_SAMPLE_SIZE];
            feature = new float[MAX_SAMPLE_SIZE * MAX_FEATURE_SIZE];
            featureName = new char[MAX_FEATURE_NAME_LEN * MAX_FEATURE_SIZE];
        }
        virtual ~BaseFilterCache(){
            delete[]target;
            delete[]feature;
            delete[]featureName;
        }

        virtual void initialize(size_t sampleSize, size_t featureSize) {
            this->featureSize = featureSize;
            this->sampleSize = sampleSize;
            if (featureSize > maxFeatureSize_ || sampleSize > maxSampleSize_) {
                delete[]feature;
                size_t featureDataSize = max(featureSize, maxFeatureSize_) * max(sampleSize, maxSampleSize_);
                feature = new float[featureDataSize];
                if (sampleSize > maxSampleSize_) {
                    delete[]target;
                    target = new float[sampleSize];
                }
                if (featureSize > maxFeatureSize_) {
                    delete[]featureName;
                    featureName = new char[featureSize * MAX_FEATURE_NAME_LEN];
                }
                maxSampleSize_ = max(maxSampleSize_, sampleSize);
                maxFeatureSize_ = max(maxFeatureSize_, featureSize);
            }
        }
        //特征数
        size_t featureSize = {0};
        //样本数
        size_t sampleSize = {0};
        //记录所有目标值
        float *target = {nullptr};
        //记录所有特征
        float *feature = {nullptr};
        char *featureName = {nullptr};
    protected:
        size_t maxSampleSize_ = {MAX_SAMPLE_SIZE};
        size_t maxFeatureSize_ = {MAX_FEATURE_SIZE};
    };

    class TrainCache{
    public:
        TrainCache(){
            pred = new float[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
            weight = new float[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
            nodeId = new int[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
        }
        virtual ~TrainCache(){
            delete[]pred;
            delete[]weight;
            delete[]nodeId;
        }
        virtual void initialize(size_t sampleSize) {
            delete[]pred;
            delete[]weight;
            delete[]nodeId;
            delete[]treeFlag;
            pred = new float[sampleSize * MAX_TREE_SIZE];
            weight = new float[sampleSize * MAX_TREE_SIZE];
            nodeId = new int[sampleSize * MAX_TREE_SIZE];
        }
        //记录预测值
        float *pred = {nullptr};
        //记录样本权重
        float *weight = {nullptr};
        //记录样本属于那个节点的标记（只能属于一个节点）
        int *nodeId = {nullptr};
    };

    class FilterCache : public BaseFilterCache{
    public:
        FilterCache() : BaseFilterCache() {
            featureFlag = new size_t[MAX_SAMPLE_SIZE];
            g = new float[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
            h = new float[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
            treeFlag = new size_t[MAX_SAMPLE_SIZE];
        }

        virtual ~FilterCache() {
            BaseFilterCache::~BaseFilterCache();
            delete[]featureFlag;
            delete[]g;
            delete[]h;
            delete[]treeFlag;

        }

        //训练和交叉验证的中间数据
        TrainCache trainRaw;
        TrainCache cvTrainRaw;

        //记录一个特征会被哪些树用到
        size_t *featureFlag = {nullptr};
        //二级泰勒展开的中间项
        float *g = {nullptr};
        float *h = {nullptr};
        //记录样本被哪棵树取样（可以同时属于多个树）
        size_t *treeFlag = {nullptr};

        virtual void initialize(size_t sampleSize, size_t featureSize) {
            if (featureSize > maxFeatureSize_ || sampleSize > maxSampleSize_) {
                if (sampleSize > maxSampleSize_) {
                    delete[]g;
                    delete[]h;
                    delete[]treeFlag;
                    g = new float[sampleSize * MAX_TREE_SIZE];
                    h = new float[sampleSize * MAX_TREE_SIZE];
                    treeFlag = new size_t[sampleSize];
                    trainRaw.initialize(sampleSize);
                    cvTrainRaw.initialize(sampleSize);
                }
                if (featureSize > maxFeatureSize_) {
                    delete[]featureFlag;
                    featureFlag = new size_t[featureSize];
                }
                maxSampleSize_ = max(maxSampleSize_, sampleSize);
                maxFeatureSize_ = max(maxFeatureSize_, featureSize);
            }
            BaseFilterCache::initialize(sampleSize, featureSize);
        }


    };

    class FilterTree : public BaseFilterTree {
    public:
        //训练treeFlag标注的样本数据，交叉验证cvTreeFlag标注的样本数据(用来寻找最优迭代次数，0表示不使用交叉验证)
        /*
         * data: 所有训练数据
         * treeId: 标注此树序号
         * maxIteratorNum: 最大迭代次数（boosting用到的回归树数量），迭代次数多了的boosting会过拟合，但配合bagging又可以缓解它
         * maxAdjWeightTime: 
         * adjWeightRule:
         * maxDepth: 每一棵回归树最大深度
         * minChildWeight: 叶节点最少样本权重和（所有节点的权重和等于样本数量）
         * cvTreeId: 交叉验证的树id, -1表示不做交叉验证
         * lambda: 正则项权重
         * */
        void train(FilterCache *data, int treeId, size_t maxIteratorNum = 2, size_t maxAdjWeightTime = 4, float adjWeightRule = 0.2f, size_t maxDepth = 6, float minChildWeight = 1, float lambda = 1, int cvTreeId = -1) {
            //标注此树取样的训练数据
            size_t treeFlag = (((size_t)1) << treeId);
            int lastTreeLen = 0;

            //初始化
            size_t dataSize = data->sampleSize * MAX_TREE_SIZE;
            memset(data->trainRaw.pred + treeId * dataSize, 0, sizeof(float) * dataSize);
            memset(data->cvTrainRaw.pred + treeId * dataSize, 0, sizeof(float) * dataSize);
            float* curWeight = data->trainRaw.weight + treeId * dataSize;
            for(size_t i = 0; i < dataSize; ++i)
                data->trainRaw.weight[i] = 1.f;


            //标注交叉验证使用的样本，0表示不就行交叉验证。交叉验证用来决定迭代次数，不做交叉验证就用最大迭代次数
            size_t cvTreeFlag = (cvTreeId == -1 ? 0 : ((size_t)1) << cvTreeId);
            //上次交叉验证的损失
            float lastCVLoss = FLT_MAX;


            for(size_t i = 0; i < maxIteratorNum; ++i){
                //计算上次迭代的预测
                boostAdjustPred(data, treeId, treeFlag, 1, false);
                //计算梯度
                calGradient(data, treeId);
                //创建第t棵树
                doBoost(data, treeId, maxDepth, minChildWeight, lambda, maxAdjWeightTime, adjWeightRule);

                //进行一次最优迭代
                for(size_t j = 0; j < maxAdjWeightTime; ++j){

                    //调整当前预测值
                    boostAdjustPred(data, treeId, treeFlag, 1, false);
                    //调整当前权重
                    bool isAdjWeight = boostAdjustWeight(data, treeId, treeFlag, adjWeightRule, false);

                    if(isAdjWeight == false || j == maxAdjWeightTime - 1)
                        break;

                    //把新建的节点删掉
                    boostAdjustPred(data, treeId, treeFlag, -1, false);
                    nodeList_.resize(lastTreeLen);
                }

                //如果需要，进行交叉验证，判断是否过拟合
                if(cvTreeId >= 0){
                    //决定交叉验证的样本属于哪个叶节点
                    refreshNodeId(data, treeId, treeFlag, true);
                    boostAdjustPred(data, treeId, cvTreeFlag, 1, true);
                    boostAdjustWeight(data, treeId, cvTreeFlag, adjWeightRule, true);
                    float cvLoss = evalLoss(data, treeId, true);
                    if(cvLoss > lastCVLoss){
                        //有可能过拟合，不再继续循环
                        break;
                    }
                }
            }
        }

        float* pred(FilterCache *data, int treeId, int nodeId, size_t treeFlag){

        }

        float eval(FilterCache *data, int treeId, size_t treeFlag){

        }
    protected:
        //决定
        void refreshNodeId(FilterCache *data, int treeId, size_t treeFlag, bool isCV){

        }

        //用某个节点的预测值修正真实预测值
        void boostAdjustPred(FilterCache *data, int treeId, size_t treeFlag, float weightScale, bool isCV){
            size_t dataSize = data->sampleSize * MAX_TREE_SIZE;
            float* curPred = (isCV ? data->cvTrainRaw.pred : data->trainRaw.pred) + treeId * dataSize;

            if(nodeList_.getSize() == 0){
                memset(curPred, 0, sizeof(float) * dataSize);
            } else {
                int* curNodeId = (isCV ? data->cvTrainRaw.nodeId : data->trainRaw.nodeId) + treeId * dataSize;
                for(size_t i = 0; i < data->sampleSize; ++i){
                    if(data->treeFlag[i] & treeFlag){
                        curPred[i] += nodeList_[curNodeId[i]].getWeight() * weightScale;
                    }
                }
            }
        }

        //调整权重，返回权重是否有变动
        bool boostAdjustWeight(FilterCache *data, int treeId, size_t treeFlag, float adjWeightRule, bool isCV){

        }

        void calGradient(FilterCache *data, size_t treeId){
            size_t dataSize = data->sampleSize * MAX_TREE_SIZE;
            size_t treeFlag = (((size_t)1) << treeId);
            float* g = data->g + treeId * dataSize;
            float* h = data->h + treeId * dataSize;
            float* pred = data->trainRaw.pred + treeId * dataSize;
            for(size_t i = 0; i < data->sampleSize; ++i){
                if(data->treeFlag[i] & treeFlag){
                    g[i] = 2 * (pred[i] - data->target[i]);
                    h[i] = 2;
                }
            }
        }

        void doBoost(FilterCache *data, size_t treeId, size_t maxDepth, float minChildWeight, float lambda, size_t maxAdjWeightTime, float adjWeightRule){
            size_t dataSize = data->sampleSize * MAX_TREE_SIZE;
            size_t treeFlag = (((size_t)1) << treeId);
            int* curNodeId = data->trainRaw.nodeId + treeId * dataSize;
            //先把所有样本分配到当前节点名下
            for(size_t i = 0; i < data->sampleSize; ++i){
                if(data->treeFlag[i] & treeFlag){
                    curNodeId[i] = nodeList_.getSize();
                } else {
                    curNodeId[i] = -1;
                }
            }

            for(size_t i = 0; i < maxDepth; ++i){

            }
        }

    protected:
        struct SplitRes{
            float gain;
            float slpitValue;
            int leftElementNum;
            int rightElementNum;
        };
        /*
         * 找到最好的分裂，返回分裂和gain:
         * feature
         * */
        static SplitRes split(const float* feature, const float* target, const float* weight, const int* nodeId, ){

        }
    };
}

#endif //ALPHATREE_FILTERTREE_H
