//
// Created by yanyu on 2017/11/24.
//

#ifndef ALPHATREE_FILTERTREE_H
#define ALPHATREE_FILTERTREE_H

#include <stddef.h>
#include "basefiltertree.h"
#include "filterhistogram.h"

namespace fb {
    const size_t MAX_SAMPLE_SIZE = 1024 * 1024;
    const size_t MAX_FEATURE_SIZE = 512;
    const size_t MAX_FEATURE_NAME_LEN = 128;
    const size_t MAX_TREE_SIZE = 64;
    const size_t MAX_LEAF_SIZE = 1024;

    inline float mean(const float* v, size_t n){
        float avg = 0;
	for(size_t i = 0; i < n; ++i)
	    avg += v[i];
	return avg / n;
    }
    inline float std(const float* v, float avg, size_t n){
        float s = 0;
	for(size_t i = 0; i < n; ++i)
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
        TrainCache cvTrainRaw;

        //记录一个特征会被哪些树用到
        size_t *featureFlag = {nullptr};
        //二级泰勒展开的中间项
        float *g = {nullptr};
        float *h = {nullptr};
        //记录样本被哪棵树取样（可以同时属于多个树）
        size_t *treeFlag = {nullptr};
        size_t maxDepth;
	size_t maxLeafSize;
        float minChildWeight;
        float gamma;
        float lambda;
        size_t maxAdjWeightTime;
        float adjWeightRule;
	size_t maxBarSize;
	float maxBarPercent;

	//监控分裂线程
        std::shared_future<SplitRes>* splitRes = {nullptr};

        virtual void initialize(size_t sampleSize, size_t featureSize, size_t maxLeafSize) {
	    this->maxLeafSize = maxLeafSize;
	    if(maxLeafSize > maxLeafSize_){
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
        void train(FilterCache *data, int treeId, size_t maxIteratorNum = 2, size_t maxAdjWeightTime = 4,
                   float adjWeightRule = 0.2f, size_t maxDepth = 6, float minChildWeight = 1, float lambda = 1,
                   int cvTreeId = -1) {
            //标注此树取样的训练数据
            size_t treeFlag = (((size_t) 1) << treeId);
            int lastTreeLen = 0;

            //初始化
            size_t dataSize = data->sampleSize * MAX_TREE_SIZE;
            memset(data->trainRaw.pred + treeId * dataSize, 0, sizeof(float) * dataSize);
            memset(data->cvTrainRaw.pred + treeId * dataSize, 0, sizeof(float) * dataSize);
            float *curWeight = data->trainRaw.weight + treeId * dataSize;
            for (size_t i = 0; i < dataSize; ++i)
                data->trainRaw.weight[i] = 1.f;


            //标注交叉验证使用的样本，0表示不就行交叉验证。交叉验证用来决定迭代次数，不做交叉验证就用最大迭代次数
            size_t cvTreeFlag = (cvTreeId == -1 ? 0 : ((size_t) 1) << cvTreeId);
            //上次交叉验证的损失
            float lastCVLoss = FLT_MAX;


            for (size_t i = 0; i < maxIteratorNum; ++i) {
                //计算上次迭代的预测
                boostAdjustPred(data, treeId, treeFlag, 1, false);
                //计算梯度
                calGradient(data, treeId);
                //创建第t棵树
                doBoost(data, treeId, maxDepth, minChildWeight, lambda, maxAdjWeightTime, adjWeightRule);

                //进行一次最优迭代
                for (size_t j = 0; j < maxAdjWeightTime; ++j) {

                    //调整当前预测值
                    boostAdjustPred(data, treeId, treeFlag, 1, false);
                    //调整当前权重
                    bool isAdjWeight = boostAdjustWeight(data, treeId, treeFlag, adjWeightRule, false);

                    if (isAdjWeight == false || j == maxAdjWeightTime - 1)
                        break;

                    //把新建的节点删掉
                    boostAdjustPred(data, treeId, treeFlag, -1, false);
                    nodeList_.resize(lastTreeLen);
                }

                //如果需要，进行交叉验证，判断是否过拟合
                if (cvTreeId >= 0) {
                    //决定交叉验证的样本属于哪个叶节点
                    refreshNodeId(data, treeId, treeFlag, true);
                    boostAdjustPred(data, treeId, cvTreeFlag, 1, true);
                    boostAdjustWeight(data, treeId, cvTreeFlag, adjWeightRule, true);
                    float cvLoss = evalLoss(data, treeId, true);
                    if (cvLoss > lastCVLoss) {
                        //有可能过拟合，不再继续循环
                        break;
                    }
                }
            }
        }

        float *pred(FilterCache *data, int treeId, int nodeId, size_t treeFlag) {

        }

        float eval(FilterCache *data, int treeId, size_t treeFlag) {

        }

    protected:
        //决定
        void refreshNodeId(FilterCache *data, int treeId, size_t treeFlag, bool isCV) {

        }

        //用某个节点的预测值修正真实预测值
        void boostAdjustPred(FilterCache *data, int treeId, size_t treeFlag, float weightScale, bool isCV) {
            size_t dataSize = data->sampleSize * MAX_TREE_SIZE;
            float *curPred = (isCV ? data->cvTrainRaw.pred : data->trainRaw.pred) + treeId * dataSize;

            if (nodeList_.getSize() == 0) {
                memset(curPred, 0, sizeof(float) * dataSize);
            } else {
                int *curNodeId = (isCV ? data->cvTrainRaw.nodeId : data->trainRaw.nodeId) + treeId * dataSize;
                for (size_t i = 0; i < data->sampleSize; ++i) {
                    if (data->treeFlag[i] & treeFlag) {
                        curPred[i] += nodeList_[curNodeId[i]].getWeight() * weightScale;
                    }
                }
            }
        }

        //调整权重，返回权重是否有变动
        bool boostAdjustWeight(FilterCache *data, int treeId, size_t treeFlag, float adjWeightRule, bool isCV) {

        }

        void calGradient(FilterCache *data, size_t treeId) {
            size_t dataSize = data->sampleSize * MAX_TREE_SIZE;
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

        void doBoost(FilterCache *data, size_t treeId, ThreadPool *threadPool) {
            size_t dataSize = data->sampleSize;
            int *curNodeId = data->trainRaw.nodeId + treeId * dataSize;
	    float* curPred = data->pred + treeId * dataSize;
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
	    std::shared_future<SplitRes>* splitRes = data->splitRes + treeId * data->maxLeafSize;
            for (size_t i = 0; i < data->maxDepth; ++i) {
                //尝试分裂
		if(elementSize == 0)
		    break;
		for(size_t j = 0; j < elementSize; ++j){
		    int cmpNodeId = j + startIndex;
		    splitRes[j] = threadPool->enqueue([data, treeId, cmpNodeId]{
		        return split(data, treeId, cmpNodeId);
		    }).share();
		}
		//尝试创建节点
		for(size_t j = 0; j < elementSize; ++j){
		    if(splitRes[j].get().isValid()){
			int preId = j + startIndex;
	                int leftId = createNode(-splitRes[j].get().gl/(splitRes[j].get().hl + data->lambda));
			int rightId = createNode(-splitRes[j].get().gr/(splitRes[j].get().gl + data->lambda));
			nodeList_[preId].setup(splitRes[j].get().slpitValue, data->featureName + splitRes[j].get().featureIndex * MAX_FEATURE_NAME_LEN);
			nodeList_[preId].leftId = leftId;
			nodeList_[preId].rightId = rightId;
			nodeList_[leftId].preId = preId;
			nodeList_[rightId].preId = preId;
                        //重新划分样本归属的叶节点
			float* curFeature = data->feature + splitRes[j].get().featureIndex * data->sampleSize;
			for(size_t k = 0; k < data->sampleSize; ++k){
			    if(curNodeId[k] == preId){
		                if(curFeature[k] < splitRes[j].get().slpitValue)
				    curNodeId[k] = leftId;
				else
				    curNodeId[k] = rightId;
			    }
			}
		    }
		}
		//将叶节点的预测加到之前t-1棵树的结果上
                for(size_t j = 0; j < dataSize; ++j)
		    curPred[j] += nodeList_[curNodeId[j]].getWeight();
		//刷新样本权重


            }
        }

    protected:
	static void refreshWeight(const float* pred, float* weight, size_t sampleSize, size_t weightLevelNum = 128){
	   float avgValue = mean(pred, sampleSize);
	   float stdValue = std(pred, avgValue, sampleSize);

	}
        /*
         * 找到最好的分裂，返回分裂和gain:
         * feature
         * */
        static SplitRes
        split(FilterCache *data,  size_t treeId, int cmpNodeId){
            SplitRes bestRes;
            size_t treeFlag = (((size_t) 1) << treeId);
            float maxGain = 0;
            for(size_t i = 0; i < data->featureSize; ++i){
                if(data->featureFlag[i] & treeFlag){
	            float* curFeature = data->feature + data->sampleSize * i;
		    SplitRes res = split(curFeature, data->target, data->g, data->h, data->trainRaw.weight+treeId*data->sampleSize, data->trainRaw.nodeId + treeId*data->sampleSize, cmpNodeId, data->sampleSize, data->gamma, data->lambda, data->maxBarSize, data->mergeBarPercent);
                    if(res.isValid()){
		        float gain = res.gain(data->gamma, data->lambda);
			if(gain > maxGain){
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
        split(const float *feature, const float *target, const float* g, const float* h, const float *w, const int *nodeId, int cmpNodeId,
              size_t sampleSize, float gamma, float lambda, size_t maxBarSize, float mergeBarPercent) {
            HistogrmBar *bars = new HistogrmBar[maxBarSize];
            size_t barSize = createHistogrmBars(feature, nodeId, target, cmpNodeId, g, h, w, sampleSize, bars, maxBarSize,
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
            } else {
                res.leftElementNum = 0;
                res.rightElementNum = 0;
            }
            return res;
        }
    };
}

#endif //ALPHATREE_FILTERTREE_H
