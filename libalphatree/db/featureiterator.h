//
// Created by godpgf on 18-2-26.
//

//todo delete

#ifndef ALPHATREE_FEATUREITERATOR_H
#define ALPHATREE_FEATUREITERATOR_H
#include "../base/iterator.h"
#include "stockcache.h"
#include <string.h>



class FeatureIterator : public IBaseIterator<float*>{
public:

    FeatureIterator(const char* featureName, size_t dayBefore, size_t sampleDays, size_t stockNum, const char* codes, bool isCache, StockCache* cache)
            : dayBefore_(dayBefore), sampleDays_(sampleDays), stockNum_(stockNum), curIndex_(0), isCache_(isCache),  cache_(cache){
        codes_ = new char[64 * stockNum];
        char* curDstCode = codes_;
        const char* curSrcCode = codes;
        for (size_t i = 0; i < stockNum; ++i) {
            strcpy(curDstCode, curSrcCode);
            int codeLen = strlen(curSrcCode);
            curSrcCode += (codeLen + 1);
            curDstCode += (codeLen + 1);
        }

        featureName_ = new char[strlen(featureName) + 1];
        strcpy(this->featureName_, featureName);
        if(isCache){
            feature_ = new float[sampleDays * stockNum];
            curFeature_ = feature_;
            fill_(dayBefore_, sampleDays_, stockNum_, featureName_, codes_, feature_);
        }
        else{
            feature_ = new float[stockNum];
            curFeature_ = feature_;
            fill_(dayBefore_ + sampleDays_ - 1, 1, stockNum_, featureName_, codes_, feature_);
        }

    }

    virtual ~FeatureIterator(){
        delete []codes_;
        delete []featureName_;
        delete []feature_;
    }

    virtual void operator++() {
        ++curIndex_;
        if(curIndex_ < sampleDays_){
            if(isCache_)
                curFeature_ = feature_ + curIndex_ * stockNum_;
            else
                fill_(dayBefore_ + sampleDays_ - curIndex_ - 1, 1, stockNum_, featureName_, codes_, feature_);
        }
    }

    virtual void skip(long size, bool isRelative = true){
        if(isRelative){
            curIndex_ += size;
            if(curIndex_ < sampleDays_){
                if(isCache_)
                    curFeature_ = feature_ + curIndex_ * stockNum_;
                else
                    fill_(dayBefore_ + sampleDays_ - curIndex_ - 1, 1, stockNum_, featureName_, codes_, feature_);
            }
        } else {
            curIndex_ = size;
            if(curIndex_ < sampleDays_){
                if(isCache_)
                    curFeature_ = feature_ + curIndex_ * stockNum_;
                else
                    fill_(dayBefore_ + sampleDays_ -  curIndex_ - 1, 1, stockNum_, featureName_, codes_, feature_);
            }
        }
    }
    virtual bool isValid(){ return curIndex_ < sampleDays_;}
    virtual float*&& getValue() {
        return std::move(curFeature_);
    }
    virtual long size(){ return sampleDays_;}

    virtual int getStockNum(){
        return stockNum_;
    }
protected:

    size_t dayBefore_;
    size_t sampleDays_;
    size_t stockNum_;
    size_t curIndex_;
    bool isCache_;
    char* codes_;
    char* featureName_;
    float* feature_;
    float* curFeature_;
    StockCache* cache_;

    void fill_(size_t dayBefore, size_t sampleDays, size_t stockNum, const char* featureName, const char* codes, float* dst){
        const char* curCode = codes;
        for(size_t i = 0; i < stockNum; ++i){
            cache_->fill(dst, dayBefore, sampleDays, curCode, featureName, i, stockNum);
            curCode = curCode + strlen(curCode) + 1;
        }
    }
};

#endif //ALPHATREE_FEATUREITERATOR_H
