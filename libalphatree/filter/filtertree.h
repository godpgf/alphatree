//
// Created by yanyu on 2017/11/24.
//

#ifndef ALPHATREE_FILTERTREE_H
#define ALPHATREE_FILTERTREE_H

#include <stddef.h>
#include "basefiltertree.h"

namespace fb{
    const size_t MAX_SAMPLE_SIZE = 1024 * 1024;
    const size_t MAX_FEATURE_SIZE = 512;
    class FilterCache{
        public:
            FilterCache(){
                returns = new float[MAX_SAMPLE_SIZE];
                risk = new float[MAX_SAMPLE_SIZE];
                feature = new float[MAX_SAMPLE_SIZE * MAX_FEATURE_SIZE];
                featureRank = new int[MAX_SAMPLE_SIZE * MAX_FEATURE_SIZE];
                featureSort = new int[MAX_SAMPLE_SIZE * MAX_FEATURE_SIZE];
            }

            virtual ~FilterCache(){
                delete []returns;
                delete []risk;
                delete []feature;
                delete []featureRank;
                delete []featureSort;
            }

            //特征数
            size_t featureSize = {0};
            //样本数
            size_t sampleSize = {0};
            //记录所有目标值
            float* returns = {nullptr};
            float* risk = {nullptr};
            //记录所有特征
            float* feature = {nullptr};
            //特征对应排名
            int* featureRank = {nullptr};
            //特征的预排序
            int* featureSort = {nullptr};

            void initialize(size_t sampleSize, size_t featureSize){
                this->featureSize = featureSize;
                this->sampleSize = sampleSize;
                if(featureSize > maxFeatureSize_ || sampleSize > maxSampleSize_){
                    delete []feature;
                    delete []featureRank;
                    delete []featureSort;
                    size_t featureDataSize = max(featureSize,maxFeatureSize_) * max(sampleSize, maxSampleSize_);
                    feature = new float[featureDataSize];
                    featureRank = new int[featureDataSize];
                    featureSort = new int[featureDataSize];
                    if(sampleSize > maxSampleSize_){
                        delete []returns;
                        delete []risk;
                        returns = new float[sampleSize];
                        risk = new float[sampleSize];
                    }
                    maxSampleSize_ = max(maxSampleSize_, sampleSize);
                    maxFeatureSize_ = max(maxFeatureSize_, featureSize);
                }
            }
        private:
            size_t maxSampleSize_ = {MAX_SAMPLE_SIZE};
            size_t maxFeatureSize_ = {MAX_FEATURE_SIZE};
    };

    class FilterTree : public BaseFilterTree{

    };
}

#endif //ALPHATREE_FILTERTREE_H
