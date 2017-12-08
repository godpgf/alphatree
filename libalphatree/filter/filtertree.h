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

    class FilterCache {
    public:
        FilterCache() {
            target = new float[MAX_SAMPLE_SIZE];
            feature = new float[MAX_SAMPLE_SIZE * MAX_FEATURE_SIZE];
            pred = new float[MAX_SAMPLE_SIZE];
            weight = new float[MAX_SAMPLE_SIZE];
	    flag = new size_t[MAX_SAMPLE_SIZE];
	    featureName = new char[MAX_FEATURE_NAME_LEN * MAX_FEATURE_SIZE];
        }

        virtual ~FilterCache() {
            delete[]target;
            delete[]feature;
            delete[]pred;
            delete[]weight;
	    delete[]flag;
	    delete[]featureName;
        }

        //特征数
        size_t featureSize = {0};
        //样本数
        size_t sampleSize = {0};
        //记录所有目标值
        float *target = {nullptr};
        //记录所有特征
        float *feature = {nullptr};
	//记录预测值
        float *pred = {nullptr};
	//记录样本权重
        float *weight = {nullptr};
	//记录样本属于那个节点的标记
        size_t *nodeFlag = {nullptr};
	//记录样本被哪棵树取样
	size_t *treeFlag = {nullptr};
	char *featureName = {nullptr};

        void initialize(size_t sampleSize, size_t featureSize) {
            this->featureSize = featureSize;
            this->sampleSize = sampleSize;
            if (featureSize > maxFeatureSize_ || sampleSize > maxSampleSize_) {
                delete[]feature;
                size_t featureDataSize = max(featureSize, maxFeatureSize_) * max(sampleSize, maxSampleSize_);
                feature = new float[featureDataSize];
                if (sampleSize > maxSampleSize_) {
                    delete[]target;
                    delete[]pred;
		    delete[]weight;
		    delete[]nodeFlag;
		    delete[]treeFlag;
                    target = new float[sampleSize];
                    pred = new float[sampleSize];
		    weight = new float[samlpeSize];
		    nodeFlag = new size_t[sampleSize];
		    treeFlag = new size_t[sampleSize];
                } 
		if(featureSize > maxFeatureSize_) {
		    delete[]featureName;
		    featureName = new char[featureSize*MAX_FEATURE_NAME_LEN];
		}
                maxSampleSize_ = max(maxSampleSize_, sampleSize);
                maxFeatureSize_ = max(maxFeatureSize_, featureSize);
            }
        }

    private:
        size_t maxSampleSize_ = {MAX_SAMPLE_SIZE};
        size_t maxFeatureSize_ = {MAX_FEATURE_SIZE};
    };

    class FilterTree : public BaseFilterTree {

    };
}

#endif //ALPHATREE_FILTERTREE_H
