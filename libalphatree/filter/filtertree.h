//
// Created by yanyu on 2017/11/24.
//

#ifndef ALPHATREE_FILTERTREE_H
#define ALPHATREE_FILTERTREE_H

#include <stddef.h>
#include "basefiltertree.h"
#include "filterhistogram.h"
#import "../base/pareto.h"

namespace fb {
    const size_t MAX_SAMPLE_SIZE = 1024 * 1024;
    const size_t MAX_FEATURE_SIZE = 512;
    const size_t MAX_FEATURE_NAME_LEN = 128;
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
        return fsqrt(s / n);
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
        bool isSplit() { return leftElementNum > 0 && rightElementNum > 0; }

        float gain(float gamma, float lambda) {
            return 0.5f * ((gl * gl) / (hl + lambda) + (gr * gr) / (hr + lambda) -
                           (gl + gr) * (gl + gr) / (hl + hr + lambda)) - gamma;
        }

        int featureId;
        float gl, gr, hl, hr;
        float slpitValue;
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
        }

        virtual ~FilterCache() {
            BaseFilterCache::~BaseFilterCache();
            delete[]featureFlag;
            delete[]g;
            delete[]h;
            delete[]treeFlag;
            delete[]splitRes;
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
        size_t maxIteratorNum;
        //每一棵回归树最大深度
        size_t maxDepth;
        size_t maxLeafSize;
        //叶节点最少样本权重和（所有节点的权重和等于样本数量）
        float minChildWeight;
        float gamma;
        float lambda;
        size_t maxAdjWeightTime;
        float adjWeightRule;
        size_t maxBarSize;
        float maxBarPercent;

        //监控分裂线程
        std::shared_future<SplitRes> *splitRes = {nullptr};

        virtual void initialize(size_t sampleSize, size_t featureSize, size_t maxLeafSize) {
            this->maxLeafSize = maxLeafSize;
            if (maxLeafSize > maxLeafSize_) {
                delete[]splitRes;
                splitRes = new std::shared_future<SplitRes>[maxLeafSize * MAX_TREE_SIZE];
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
        void train(FilterCache *data, int treeId, ThreadPool *threadPool) {
            //标注此树取样的训练数据
            size_t treeFlag = (((size_t) 1) << treeId);
            initialize(data, treeId);

            int lastRootId = -1;
            for (size_t i = 0; i < maxIteratorNum; ++i) {
                //计算上次迭代的预测
                boostAdjustPred(data, treeId, treeFlag);
                //计算梯度
                calGradient(data, treeId);
                //创建第t棵树
                int rootId = doBoost(data, treeId, threadPool);
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
        }

        float *pred(FilterCache *data, int treeId, int nodeId, size_t treeFlag) {

        }

        float eval(FilterCache *data, int treeId, size_t treeFlag) {

        }

    protected:
        static void initialize(FilterCache *data, int treeId) {
            //初始化
            size_t dataSize = data->sampleSize * MAX_TREE_SIZE;
            memset(data->trainRaw.pred + treeId * dataSize, 0, sizeof(float) * dataSize);
            float *curWeight = data->trainRaw.weight + treeId * dataSize;
            for (size_t i = 0; i < dataSize; ++i)
                data->trainRaw.weight[i] = 1.f;
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
            float *curPred = data->pred + treeId * dataSize;
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
            for (size_t i = 0; i < data->maxDepth; ++i) {
                if (elementSize == 0)
                    break;
                //不断的调整权重，分裂节点
                for (size_t wt = 0; wt < data->maxAdjWeightTime; ++wt) {
                    //如果有新节点加入
                    if (nodeList_.getSize() > startIndex + elementSize) {
                        //将叶节点的预测加到之前t-1棵树的结果上
                        for (size_t j = 0; j < dataSize; ++j)
                            curPred[j] += nodeList_[curNodeId[j]].getWeight();
                        //刷新样本权重
                        refreshWeight(curPred, curWeight, dataSize);
                        //恢复当前预测
                        for (size_t j = 0; j < dataSize; ++j)
                            curPred[j] -= nodeList_[curNodeId[j]].getWeight();
                        //恢复节点所属于的叶节点
                        for (size_t j = 0; j < dataSize; ++j)
                            if (curNodeId[j] >= startIndex + elementSize) {
                                curNodeId[j] = nodeList_[curNodeId[j]].preId;
                            }
                        //删除新节点
                        nodeList_.resize(startIndex + elementSize);
                    }

                    //得到划分
                    for (size_t j = 0; j < elementSize; ++j) {
                        int cmpNodeId = j + startIndex;
                        splitRes[j] = threadPool->enqueue([data, treeId, cmpNodeId] {
                            return split(data, treeId, cmpNodeId);
                        }).share();
                    }

                    //尝试创建节点
                    for (size_t j = 0; j < elementSize; ++j) {
                        if (splitRes[j].get().isValid()) {
                            int preId = j + startIndex;
                            int leftId = createNode(-splitRes[j].get().gl / (splitRes[j].get().hl + data->lambda));
                            int rightId = createNode(-splitRes[j].get().gr / (splitRes[j].get().gl + data->lambda));
                            nodeList_[preId].setup(splitRes[j].get().slpitValue, data->featureName +
                                                                                 splitRes[j].get().featureIndex *
                                                                                 MAX_FEATURE_NAME_LEN);
                            nodeList_[preId].leftId = leftId;
                            nodeList_[preId].rightId = rightId;
                            nodeList_[leftId].preId = preId;
                            nodeList_[rightId].preId = preId;
                            //重新划分样本归属的叶节点
                            float *curFeature = data->feature + splitRes[j].get().featureIndex * data->sampleSize;
                            for (size_t k = 0; k < data->sampleSize; ++k) {
                                if (curNodeId[k] == preId) {
                                    if (curFeature[k] < splitRes[j].get().slpitValue)
                                        curNodeId[k] = leftId;
                                    else
                                        curNodeId[k] = rightId;
                                }
                            }
                        }
                    }

                    //确定新加入的节点
                    startIndex = startIndex + elementSize;
                    elementSize = nodeList_.getSize() - startIndex;
                }
            }

            //将叶节点的预测加到之前t-1棵树的结果上
            for (size_t j = 0; j < dataSize; ++j)
                curPred[j] += nodeList_[curNodeId[j]].getWeight();
            //返回子树
            return rootId;
        }

    protected:
        static void refreshWeight(const float *pred, float *weight, size_t sampleSize, size_t weightLevelNum = 128) {
            float avgValue = mean(pred, sampleSize);
            float stdValue = std(pred, avgValue, sampleSize);
            float stdScale = normsinv(1.0 - 1.0 / weightLevelNum);
            float startValue = avgValue - stdValue * stdScale;
            float deltaStd = stdValue * stdScale / (0.5f * (weightLevelNum - 2));
            size_t *ranking = new size_t[weightLevelNum];
            float *rankingWeight = new float[weightLevelNum];

            for (size_t i = 0; i < sampleSize; ++i) {
                int index = pred[i] < startValue ? 0 : min((int) ((pred[i] - startValue) / deltaStd) + 1,
                                                           weightLevelNum - 1);
                ++ranking[index];
            }
            distributionWeightPr(ranking, weightLevelNum, rankingWeight);
            for (size_t i = 0; i < sampleSize; ++i) {
                int index = pred[i] < startValue ? 0 : min((int) ((pred[i] - startValue) / deltaStd) + 1,
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
                                         data->gamma, data->lambda, data->maxBarSize, data->mergeBarPercent);
                    if (res.isValid()) {
                        float gain = res.gain(data->gamma, data->lambda);
                        if (gain > maxGain) {
                            maxGain = gain;
                            bestRes = res;
                            bestRes.featureId = i;
                        }
                    }
                }
            }
            return bestRes;
        }

        static SplitRes
        split(const float *feature, const float *target, const float *g, const float *h, const float *w,
              const int *nodeId, int cmpNodeId,
              size_t sampleSize, float gamma, float lambda, size_t maxBarSize, float mergeBarPercent) {
            HistogrmBar *bars = new HistogrmBar[maxBarSize];
            size_t barSize = createHistogrmBars(feature, nodeId, target, cmpNodeId, g, h, w, sampleSize, bars,
                                                maxBarSize,
                                                mergeBarPercent);
            for (size_t i = 1; i < barSize; ++i) {
                bars[i].g += bars[i - 1].g;
                bars[i].h += bars[i - 1].h;
                bars[i].dataNum += bars[i - 1].dataNum;
            }
            SplitRes res;

            float maxGain = 0;
            size_t bestSplit = -1;
            for (size_t i = 1; i < barSize - 1; ++i) {
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
