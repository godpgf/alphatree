//
// Created by yanyu on 2017/11/24.
//

#ifndef ALPHATREE_FILTERTREE_H
#define ALPHATREE_FILTERTREE_H

#include <stddef.h>
#include <stdlib.h>
#include <algorithm>
#include <iostream>
#include "basefiltertree.h"
#include "filterhistogram.h"
#include "pareto.h"
#include "normal.h"
using namespace std;

namespace fb {
    const size_t MAX_SAMPLE_SIZE = 1024 * 1024;
    const size_t MAX_FEATURE_SIZE = 512;
    const size_t MAX_FEATURE_NAME_LEN = 64;
    const size_t MAX_TREE_SIZE = 64;
    const size_t MAX_LEAF_SIZE = 1024;

    inline float mean(const float *v, size_t n) {
        float avg = 0;
        for (size_t i = 0; i < n; ++i)
            avg += v[i];
        return avg / n;
    }

    inline float std(const float *v, float avg, size_t n) {
        float s = 0;
        for (size_t i = 0; i < n; ++i)
            s += (v[i] - avg) * (v[i] - avg);
        return sqrtf(s / n);
    }

    class BaseFilterCache {
    public:
        BaseFilterCache() {
            target = new float[MAX_SAMPLE_SIZE];
            feature = new float[MAX_SAMPLE_SIZE * MAX_FEATURE_SIZE];
            featureName = new char[MAX_FEATURE_NAME_LEN * MAX_FEATURE_SIZE];
        }

        virtual ~BaseFilterCache() {
            delete[]target;
            delete[]feature;
            delete[]featureName;
        }

        void initialize(size_t sampleSize, size_t featureSize) {
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

    class TrainCache {
    public:
        TrainCache() {
            pred = new float[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
            weight = new float[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
            nodeId = new int[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
        }

        virtual ~TrainCache() {
            delete[]pred;
            delete[]weight;
            delete[]nodeId;
        }

        virtual void initialize(size_t sampleSize) {
            delete[]pred;
            delete[]weight;
            delete[]nodeId;
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

    struct SplitRes {
    public:
        bool isSplit() const { return leftElementNum > 0 && rightElementNum > 0; }

        float gain(float gamma, float lambda) {
            return 0.5f * ((gl * gl) / (hl + lambda) + (gr * gr) / (hr + lambda) -
                           (gl + gr) * (gl + gr) / (hl + hr + lambda)) - gamma;
        }

        int featureIndex;
        float gl, gr, hl, hr;
        float splitValue;
        int leftElementNum = {0};
        int rightElementNum = {0};
    };


    class FilterCache : public BaseFilterCache {
    public:
        FilterCache() : BaseFilterCache() {

            featureFlag = new size_t[MAX_FEATURE_SIZE];
            g = new float[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
            h = new float[MAX_SAMPLE_SIZE * MAX_TREE_SIZE];
            treeFlag = new size_t[MAX_SAMPLE_SIZE];
            splitRes = new std::shared_future<SplitRes>[MAX_LEAF_SIZE * MAX_TREE_SIZE];
            sample2nodeRes = new std::shared_future<void>[MAX_LEAF_SIZE * MAX_TREE_SIZE];
            treeRes = new std::shared_future<void>[MAX_TREE_SIZE];
        }

        virtual ~FilterCache() {
            //BaseFilterCache::~BaseFilterCache();
            delete[]featureFlag;
            delete[]g;
            delete[]h;
            delete[]treeFlag;
            delete[]splitRes;
            delete[]sample2nodeRes;
            delete[]treeRes;
        }

        //训练和交叉验证的中间数据
        TrainCache trainRaw;
        //TrainCache cvTrainRaw;

        //记录一个特征会被哪些树用到
        size_t *featureFlag = {nullptr};
        //二级泰勒展开的中间项
        float *g = {nullptr};
        float *h = {nullptr};
        //记录样本被哪棵树取样（可以同时属于多个树）
        size_t *treeFlag = {nullptr};
        //最大迭代次数（boosting用到的回归树数量），迭代次数多了的boosting会过拟合，但配合bagging又可以缓解它
        size_t iteratorNum;
        //随机森林中树的数量
        size_t treeSize;
        //每轮迭代产生回归树的最大深度
        size_t maxDepth = {16};
        //每轮迭代产生回归树的最大叶子节点数
        size_t maxLeafSize = {1024};
        //叶节点最少样本权重和（所有节点的权重和等于样本数量）
        float minChildWeight = {1};
        //xgboost中的gain公式：0.5*(Gl*Gl/(Hl+lambda)+Gr*Gr/(Hr+lambda)+(Gl+Gr)*(Gl+Gr)/(Hl+Hr+lambda))-gamma
        float gamma;
        float lambda;
        //调整权重的规则
        float adjWeightRule;
        //分裂用的柱状图最多有多少
        size_t maxBarSize;

        //随机森林中每棵树样本随机采样占比
        float subsample = {0.6f};
        //随机森林中每棵树特征随机抽取占比
        float colsampleBytree = {0.75f};

        //监控分裂线程
        std::shared_future<SplitRes> *splitRes = {nullptr};
        //监控样本归属叶节点的划分线程
        std::shared_future<void>* sample2nodeRes = {nullptr};
        //监控随机森林中各个树的线程
        std::shared_future<void>* treeRes = {nullptr};

        virtual void initialize(size_t sampleSize, size_t featureSize, size_t maxLeafSize) {
            this->maxLeafSize = maxLeafSize;
            if (maxLeafSize > maxLeafSize_) {
                delete[]splitRes;
                delete[]sample2nodeRes;
                splitRes = new std::shared_future<SplitRes>[maxLeafSize * MAX_TREE_SIZE];
                sample2nodeRes = new std::shared_future<void>[maxLeafSize * MAX_TREE_SIZE];
            }
            if (featureSize > maxFeatureSize_ || sampleSize > maxSampleSize_) {
                if (sampleSize > maxSampleSize_) {
                    delete[]g;
                    delete[]h;
                    delete[]treeFlag;
                    g = new float[sampleSize * MAX_TREE_SIZE];
                    h = new float[sampleSize * MAX_TREE_SIZE];
                    treeFlag = new size_t[sampleSize];
                    trainRaw.initialize(sampleSize);
                    //cvTrainRaw.initialize(sampleSize);
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

        void sample(){
            memset(featureFlag, 0 , sizeof(size_t) * treeSize * featureSize);
            memset(treeFlag, 0, sizeof(size_t) * treeSize * sampleSize);
            //int featureFlagNum = 0, treeFlagNum = 0;
            for(size_t i = 0; i < treeSize; ++i){
                size_t flag = (((size_t)1) << i);
                for(size_t j = 0; j < featureSize; ++j)
                    if(((float)rand())/RAND_MAX <= colsampleBytree){
                        featureFlag[i * featureSize + j] +=  flag;
                        //++featureFlagNum;
                    }

                for(size_t j = 0; j < sampleSize; ++j)
                    if(((float)rand())/RAND_MAX <= subsample){
                        treeFlag[i * sampleSize + j] += flag;
                        //++treeFlagNum;
                    }
            }
            //cout<<featureFlagNum/(float)(treeSize*featureSize)<<" "<<colsampleBytree<<endl;
            //cout<<treeFlagNum/(float)(treeSize * sampleSize)<<" "<<subsample<<endl;
        }

    protected:
        size_t maxLeafSize_ = {MAX_LEAF_SIZE};
    };

    class FilterTree : public BaseFilterTree {
    public:
        //训练treeFlag标注的样本数据
        /*
         * data: 所有训练数据
         * treeId: 标注此树序号
         * */
        void train(FilterCache *data, int treeIndex, ThreadPool *threadPool) {
            //标注此树取样的训练数据
            //size_t treeFlag = (((size_t) 1) << treeIndex);
            initialize(data, treeIndex);

            int lastRootId = -1;
            for (size_t i = 0; i < data->iteratorNum; ++i) {
                //计算上次迭代的预测
                boostAdjustPred(data, treeIndex);
                cout<<"treeIndex:"<<treeIndex<<" "<<"iterator:"<<i<<" l2Loss:"<<l2Loss(data, treeIndex)<<endl;
                //计算梯度
                calGradient(data, treeIndex);
                //创建第t棵树
                int rootId = doBoost(data, treeIndex, threadPool);
                if (i == 0)
                    lastRootId = rootId;
                else {
                    createNode(0, "+");
                    nodeList_[nodeList_.getSize() - 1].leftId = lastRootId;
                    nodeList_[lastRootId].preId = nodeList_.getSize() - 1;
                    nodeList_[nodeList_.getSize() - 1].rightId = rootId;
                    nodeList_[rootId].preId = nodeList_.getSize() - 1;
                    lastRootId = nodeList_.getSize() - 1;
                }
            }
            //随机森林权重在计算平均时会除树数，这里先除了
            for(size_t i = 0; i < nodeList_.getSize(); ++i){
                if(nodeList_[i].getNodeType() == FilterNodeType::LEAF){
                    nodeList_[i].setup(nodeList_[i].getWeight() / data->treeSize);
                }
            }
        }


    protected:
        static void initialize(FilterCache *data, int treeId) {
            //初始化
            size_t dataSize = data->sampleSize;
            memset(data->trainRaw.pred + treeId * dataSize, 0, sizeof(float) * dataSize);
            float *curWeight = data->trainRaw.weight + treeId * dataSize;
            for (size_t i = 0; i < dataSize; ++i)
                curWeight[i] = 1.f;
        }

        static float l2Loss(FilterCache *data, int treeId){
            size_t dataSize = data->sampleSize;
            float *curWeight = data->trainRaw.weight + treeId * dataSize;
            float *curPred = data->trainRaw.pred + treeId * dataSize;
            float l2loss = 0;
            for(size_t i = 0; i < dataSize; ++i) {
                l2loss += curWeight[i] * powf(curPred[i] - data->target[i], 2);
            }
            return l2loss / dataSize;
        }

        //用某个节点的预测值修正真实预测值
        void boostAdjustPred(FilterCache *data, int treeId) {
            size_t dataSize = data->sampleSize;
            float *curPred = data->trainRaw.pred + treeId * dataSize;
            if (nodeList_.getSize() == 0) {
                memset(curPred, 0, sizeof(float) * dataSize);
            } else {
                int *curNodeId = data->trainRaw.nodeId + treeId * dataSize;
                for (size_t i = 0; i < data->sampleSize; ++i) {
                    if (curNodeId[i] >= 0) {
                        curPred[i] += nodeList_[curNodeId[i]].getWeight();
                    }
                }
            }
        }

        void calGradient(FilterCache *data, size_t treeId) {
            size_t dataSize = data->sampleSize;
            size_t treeFlag = (((size_t) 1) << treeId);
            float *g = data->g + treeId * dataSize;
            float *h = data->h + treeId * dataSize;
            float *pred = data->trainRaw.pred + treeId * dataSize;
            for (size_t i = 0; i < data->sampleSize; ++i) {
                if (data->treeFlag[i] & treeFlag) {
                    g[i] = 2 * (pred[i] - data->target[i]);
                    h[i] = 2;
                }
            }
        }

        int doBoost(FilterCache *data, size_t treeId, ThreadPool *threadPool) {
            size_t dataSize = data->sampleSize;
            size_t treeFlag = (((size_t) 1) << treeId);
            int *curNodeId = data->trainRaw.nodeId + treeId * dataSize;
            float *curPred = data->trainRaw.pred + treeId * dataSize;
            float *curWeight = data->trainRaw.weight + treeId * dataSize;

            //先把所有样本分配到当前节点名下
            int rootId = createNode();

            for (size_t i = 0; i < data->sampleSize; ++i) {
                if (data->treeFlag[i] & treeFlag) {
                    curNodeId[i] = rootId;
                } else {
                    curNodeId[i] = -1;
                }
            }

            //尝试把从startIndex开始的elementSize个节点继续分割
            int startIndex = rootId;
            size_t elementSize = 1;
            std::shared_future<SplitRes> *splitRes = data->splitRes + treeId * data->maxLeafSize;
            std::shared_future<void> *sample2NodeRes = data->sample2nodeRes + treeId * data->maxLeafSize;
            for (size_t i = 0; i < data->maxDepth; ++i) {
                if (elementSize == 0)
                    break;

                //得到划分
                for (size_t j = 0; j < elementSize; ++j) {
                    int cmpNodeId = j + startIndex;
                    splitRes[j] = threadPool->enqueue([data, treeId, cmpNodeId] {
                        return split(data, treeId, cmpNodeId);
                    }).share();
                }

                //尝试创建节点
                for (size_t j = 0; j < elementSize; ++j) {
                    if (splitRes[j].get().isSplit()) {
                        int preId = j + startIndex;
                        int leftId = createNode(-splitRes[j].get().gl / (splitRes[j].get().hl + data->lambda));
                        int rightId = createNode(-splitRes[j].get().gr / (splitRes[j].get().hr + data->lambda));
                        nodeList_[preId].setup(splitRes[j].get().splitValue, data->featureName +
                                                                             splitRes[j].get().featureIndex *
                                                                             MAX_FEATURE_NAME_LEN);
                        nodeList_[preId].leftId = leftId;
                        nodeList_[preId].rightId = rightId;
                        nodeList_[leftId].preId = preId;
                        nodeList_[rightId].preId = preId;
                        if(nodeList_[preId].preId != -1)
                            sample2NodeRes[nodeList_[preId].preId].get();
                        float* curFeature = data->feature + splitRes[j].get().featureIndex * data->sampleSize;
                        float slpitValue = splitRes[j].get().splitValue;
                        sample2NodeRes[preId] = threadPool->enqueue([curFeature, slpitValue, preId, leftId, rightId, curNodeId, dataSize]{
                            //重新划分样本归属的叶节点
                            return refreshNodeId(curFeature, slpitValue, preId, leftId, rightId, curNodeId, dataSize);
                        }).share();

                    }
                }

                for (size_t j = 0; j < elementSize; ++j) {
                    if (splitRes[j].get().isSplit()) {
                        int preId = j + startIndex;
                        sample2NodeRes[preId].get();
                    }
                }
                /*cout<<"treeID"<<treeId<<"在第"<<i<<"层得到新节点数量"<<elementSize<<endl;
                //test 测试损失
                {
                    //将叶节点的预测加到之前t-1棵树的结果上
                    for (size_t j = 0; j < dataSize; ++j)
                        curPred[j] += nodeList_[curNodeId[j]].getWeight();

                    //刷新样本权重
                    cout<<"当前分裂后损失："<<l2Loss(data, treeId)<<endl;

                    //恢复当前预测
                    for (size_t j = 0; j < dataSize; ++j)
                        curPred[j] -= nodeList_[curNodeId[j]].getWeight();
                    break;
                }*/
                //确定新加入的节点
                startIndex = startIndex + elementSize;
                elementSize = nodeList_.getSize() - startIndex;
            }

            //将叶节点的预测加到之前t-1棵树的结果上
            for (size_t j = 0; j < dataSize; ++j)
                curPred[j] += nodeList_[curNodeId[j]].getWeight();

            //刷新样本权重
            if(data->adjWeightRule > 0)
                refreshWeight(curPred, curWeight, dataSize, data->adjWeightRule);
            //返回子树
            return rootId;
        }

    protected:
        //刷新样本归属的叶节点
        static void refreshNodeId(const float *curFeature, float slpitValue, int preId, int leftId, int rightId, int *curNodeId, size_t sampleSize){
            for (size_t k = 0; k < sampleSize; ++k) {
                if (curNodeId[k] == preId) {
                    if (curFeature[k] < slpitValue)
                        curNodeId[k] = leftId;
                    else
                        curNodeId[k] = rightId;
                }
            }
        }
        static void refreshWeight(const float *pred, float *weight, size_t sampleSize, float ruleWeight, size_t weightLevelNum = 128) {
            float avgValue = mean(pred, sampleSize);
            float stdValue = std(pred, avgValue, sampleSize);
            float stdScale = normsinv(1.0 - 1.0 / weightLevelNum);
            float startValue = avgValue - stdValue * stdScale;
            float deltaStd = stdValue * stdScale / (0.5f * (weightLevelNum - 2));
            size_t *ranking = new size_t[weightLevelNum];
            float *rankingWeight = new float[weightLevelNum];
            memset(ranking, 0, sizeof(size_t) * weightLevelNum);
            memset(rankingWeight, 0, sizeof(float) * weightLevelNum);

            for (size_t i = 0; i < sampleSize; ++i) {
                int index = pred[i] < startValue ? 0 : std::min((size_t) ((pred[i] - startValue) / deltaStd) + 1,
                                                           weightLevelNum - 1);
                ++ranking[index];
            }

            for(size_t i = 0; i < weightLevelNum; ++i){
                cout<< ranking[i]<<" ";
            }
            cout<<endl;

            distributionWeightPr(ranking, weightLevelNum, rankingWeight, ruleWeight);
            for (size_t i = 0; i < sampleSize; ++i) {
                int index = pred[i] < startValue ? 0 : std::min((size_t) ((pred[i] - startValue) / deltaStd) + 1,
                                                           weightLevelNum - 1);
                weight[i] = rankingWeight[index];
            }

            delete[]ranking;
            delete[]rankingWeight;
        }

        /*
         * 找到最好的分裂，返回分裂和gain:
         * feature
         * */
        static SplitRes
        split(FilterCache *data, size_t treeId, int cmpNodeId) {
            SplitRes bestRes;
            size_t treeFlag = (((size_t) 1) << treeId);
            float maxGain = 0;
            for (size_t i = 0; i < data->featureSize; ++i) {
                if (data->featureFlag[i] & treeFlag) {
                    float *curFeature = data->feature + data->sampleSize * i;
                    SplitRes res = split(curFeature, data->target, data->g, data->h,
                                         data->trainRaw.weight + treeId * data->sampleSize,
                                         data->trainRaw.nodeId + treeId * data->sampleSize, cmpNodeId, data->sampleSize,
                                         data->gamma, data->lambda, data->maxBarSize);
                    if (res.isSplit()) {
                        float gain = res.gain(data->gamma, data->lambda);
                        if (gain > maxGain) {
                            //cout<<data->featureName + i * MAX_FEATURE_NAME_LEN<<" gain:"<<gain<<endl;
                            maxGain = gain;
                            bestRes = res;
                            bestRes.featureIndex = i;
                        }
                    }
                }
            }
            //cout<<data->featureName+bestRes.featureIndex * MAX_FEATURE_NAME_LEN<<" "<<bestRes.splitValue<<endl;
            return bestRes;
        }

        static SplitRes
        split(const float *feature, const float *target, const float *g, const float *h, const float *w,
              const int *nodeId, int cmpNodeId,
              size_t sampleSize, float gamma, float lambda, size_t maxBarSize) {
            HistogrmBar *bars = new HistogrmBar[maxBarSize];
            size_t barSize = createHistogrmBars(feature, nodeId, target, cmpNodeId, g, h, w, sampleSize, bars,
                                                maxBarSize);
            for (int i = 1; i < barSize; ++i) {
                bars[i].g += bars[i - 1].g;
                bars[i].h += bars[i - 1].h;
                bars[i].dataNum += bars[i - 1].dataNum;
            }
            SplitRes res;

            float maxGain = 0;
            int bestSplit = -1;
            for (int i = 1; i < barSize - 1; ++i) {
                res.gl = bars[i - 1].g;
                res.gr = bars[barSize - 1].g - res.gl;
                res.hl = bars[i - 1].h;
                res.hr = bars[barSize - 1].h - res.hl;
                float gain = res.gain(gamma, lambda);
                if (gain > maxGain) {
                    bestSplit = i;
                    maxGain = gain;
                }
            }
            delete[]bars;

            if (bestSplit > 0) {
                res.gl = bars[bestSplit - 1].g;
                res.gr = bars[barSize - 1].g - res.gl;
                res.hl = bars[bestSplit - 1].h;
                res.hr = bars[barSize - 1].h - res.hl;
                res.leftElementNum = bars[bestSplit - 1].dataNum;
                res.rightElementNum = bars[barSize - 1].dataNum - res.leftElementNum;
                res.splitValue = bars[bestSplit].startValue;
            } else {
                res.leftElementNum = 0;
                res.rightElementNum = 0;
            }
            return res;
        }
    };
}

#endif //ALPHATREE_FILTERTREE_H
