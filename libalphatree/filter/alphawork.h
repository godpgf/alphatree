//
// Created by yanyu on 2017/10/1.
//

#ifndef ALPHATREE_ALPHAWORK_H
#define ALPHATREE_ALPHAWORK_H

#include "../alphaforest.h"
#include "alphagroup.h"

const size_t MAX_SUB_WORK_NUM = 4;

class AlphaWork{
    public:
        AlphaWork(AlphaForest* alphaForest):alphaForest_(alphaForest){

        }
        virtual ~AlphaWork(){};

        bool addSubWork(AlphaWork* subWork){
            if(subWorkNum < MAX_SUB_WORK_NUM){
                subWorks_[subWorkNum++] = subWork;
                return true;
            }
            return false;
        }
        virtual size_t work(int alphatreeId, int cacheMemoryId, int flagCacheId, int codesId, AlphaGroup* cacheGroup, int stockSize = 0){
            size_t willGroupSize = 0;
            for(int i = 0; i < subWorkNum; ++i){
                willGroupSize += subWorks_[i]->work(alphatreeId, cacheMemoryId, flagCacheId, codesId, cacheGroup, stockSize);
            }
            return willGroupSize;
        }

        AlphaForest* getAlphaForest(){ return alphaForest_;}

    protected:
        AlphaForest* alphaForest_;
        AlphaWork* subWorks_[MAX_SUB_WORK_NUM];
        size_t subWorkNum={0};
};

class AlphatreeWork : public AlphaWork{
    public:
        AlphatreeWork(AlphaForest* alphaForest, size_t dayBefore, size_t watchFutureNum, size_t sampleSize):
                AlphaWork(alphaForest), dayBefore_(dayBefore), watchFutureNum_(watchFutureNum),sampleSize_(sampleSize){

        }

        virtual size_t work(int alphatreeId, int cacheMemoryId, int flagCacheId, int codesId, AlphaGroup* cacheGroup, int stockSize = 0){
            char* codes = alphaForest_->getCodesCache()->getCacheMemory(codesId);
            stockSize = alphaForest_->getCodes(dayBefore_, watchFutureNum_,alphaForest_->getHistoryDays(alphatreeId),sampleSize_,codes);
            alphaForest_->calAlpha(alphatreeId, cacheMemoryId, flagCacheId, dayBefore_ + watchFutureNum_, sampleSize_, codes, stockSize);
            return AlphaWork::work(alphatreeId, cacheMemoryId, flagCacheId, codesId, cacheGroup, stockSize);
        }

    protected:
        size_t dayBefore_;
        size_t watchFutureNum_;
        size_t sampleSize_;
};


#endif //ALPHATREE_ALPHAWORK_H