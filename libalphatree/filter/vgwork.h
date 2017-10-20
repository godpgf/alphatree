//
// Created by yanyu on 2017/10/1.
//

#ifndef ALPHATREE_VALUEGROUPWORK_H
#define ALPHATREE_VALUEGROUPWORK_H

#include <float.h>
#include "alphawork.h"

class ValueGroupEvalWork: public AlphaWork{
    public:
        ValueGroupEvalWork(AlphaForest* alphaForest, size_t dayBefore, size_t minFutureDays, size_t maxFutureDays, size_t groupSize, size_t sampleSize, size_t groupOffset)
            :AlphaWork(alphaForest), dayBefore_(dayBefore), minFutureDays_(minFutureDays), maxFutureDays_(maxFutureDays), groupSize_(groupSize), sampleSize_(sampleSize), groupOffset_(groupOffset)
            {}

        virtual size_t work(int alphatreeId, int cacheMemoryId, int flagCacheId, int codesId, AlphaGroup* cacheGroup, int stockSize = 0){
            float* cacheMemory = alphaForest_->getAlphaTreeMemoryCache()->getCacheMemory(cacheMemoryId);
            int rootId = alphaForest_->getRoot(alphatreeId);
            const float* res = alphaForest_->getAlpha(alphatreeId, rootId, cacheMemoryId, sampleSize_, stockSize);
            AlphaGroup* curAlphaGroup = cacheGroup + groupOffset_;
            char* codeCache = alphaForest_->getCodesCache()->getCacheMemory(codesId);

            float startAlpha = curAlphaGroup[0].groupPars[1];
            float endAlpha = curAlphaGroup[groupSize_-1].groupPars[0];
            float deltaAlpha = (endAlpha - startAlpha) / (groupSize_ - 2);

            int gNum = getGroupNum();
            for(int i = 0; i < gNum; ++i){
                curAlphaGroup[i].MFE = 0;
                curAlphaGroup[i].MAE = 0;
                curAlphaGroup[i].marketMAE = 0;
                curAlphaGroup[i].marketMFE = 0;
                curAlphaGroup[i].count = 0;
            }

            char* curCodes = codeCache;
            for(int i = 0; i <  stockSize; ++i){
                Stock* stock = alphaForest_->getAlphaDataBase()->getStock(curCodes);
                Stock* market = alphaForest_->getAlphaDataBase()->getStock(stock->getMarket());
                for(int k = 0; k < sampleSize_; ++k){
                    int resIndex = k * stockSize + i;
                    int stockIndex = stock->size-alphaForest_->getFutureDays(alphatreeId) - sampleSize_ + k;
                    int marketIndex = market->size-alphaForest_->getFutureDays(alphatreeId) - sampleSize_ + k;
                    int index = res[resIndex] < startAlpha ? 0 : ( res[resIndex] >= endAlpha ? groupSize_ - 1 :1 + (int)(res[resIndex] - startAlpha)/deltaAlpha);
                    for(int j = minFutureDays_; j <= maxFutureDays_; ++j){
                        int srcId = (j - minFutureDays_) * groupSize_ + index;
                        curAlphaGroup[srcId].count++;
                        curAlphaGroup[srcId].MAE += alphaForest_->getAlphaDataBase()->getMAE(stock, stockIndex, j);
                        curAlphaGroup[srcId].MFE += alphaForest_->getAlphaDataBase()->getMFE(stock, stockIndex, j);
                        curAlphaGroup[srcId].marketMAE += alphaForest_->getAlphaDataBase()->getMAE(market, marketIndex, j);
                        curAlphaGroup[srcId].marketMFE += alphaForest_->getAlphaDataBase()->getMFE(market, marketIndex, j);
                    }
                }
                curCodes = curCodes + (strlen(curCodes) + 1);
            }

            return AlphaWork::work(alphatreeId, cacheMemoryId, flagCacheId, codesId, cacheGroup, stockSize);
        }

        int getGroupNum(){
            return (maxFutureDays_ - minFutureDays_ + 1) * groupSize_;
        }

    protected:
        size_t dayBefore_;
        size_t minFutureDays_;
        size_t maxFutureDays_;
        size_t groupSize_;
        size_t sampleSize_;
        size_t groupOffset_;
};

class ValueGroupWork: public ValueGroupEvalWork{
    public:
        ValueGroupWork(AlphaForest* alphaForest, size_t dayBefore, size_t minFutureDays, size_t maxFutureDays, size_t groupSize, size_t sampleSize, size_t groupOffset)
                :ValueGroupEvalWork(alphaForest, dayBefore, minFutureDays, maxFutureDays, groupSize, sampleSize, groupOffset)
        {}

        virtual size_t work(int alphatreeId, int cacheMemoryId, int flagCacheId, int codesId, AlphaGroup* cacheGroup, int stockSize = 0){
            float* cacheMemory = alphaForest_->getAlphaTreeMemoryCache()->getCacheMemory(cacheMemoryId);
            int rootId = alphaForest_->getRoot(alphatreeId);
            const float* res = alphaForest_->getAlpha(alphatreeId, rootId, cacheMemoryId, sampleSize_, stockSize);
            int dateSize = AlphaDataBase::getElementSize(alphaForest_->getHistoryDays(alphatreeId), sampleSize_, alphaForest_->getFutureDays(alphatreeId));
            int nodeNum = alphaForest_->getNodeNum(alphatreeId);
            if(nodeNum != rootId + 1)
                throw "err";
            float *targetCache = AlphaTree::getNodeCacheMemory(nodeNum, dateSize, stockSize, cacheMemory);

            //找到最大最小的数据
            int gribSize = sampleSize_ * stockSize / groupSize_;
            float* minDataCache = targetCache;
            float* maxDataCache = targetCache + gribSize;
            for(int i = 0; i < gribSize; ++i) {
                minDataCache[i] = FLT_MAX;
                maxDataCache[i] = -FLT_MAX;
            }
            int index;
            for(int i = 0; i < sampleSize_ * stockSize; ++i){
                index = gribSize-1;
                if (res[i] < minDataCache[index]){
                    while(index >= 1 && res[i] < minDataCache[index-1]){
                        minDataCache[index] = minDataCache[index-1];
                        index--;
                    }
                    minDataCache[index] = res[i];
                }
                index = gribSize-1;
                if(res[i] > maxDataCache[index]){
                    while (index >= 1 && res[i] > maxDataCache[index-1]){
                        maxDataCache[index] = maxDataCache[index-1];
                        index--;
                    }
                    maxDataCache[index] = res[i];
                }
            }

            float startAlpha = minDataCache[gribSize-1];
            float endAlpha = maxDataCache[gribSize-1];
            float deltaAlpha = (endAlpha - startAlpha) / (groupSize_ - 2);

            AlphaGroup* curAlphaGroup = cacheGroup + groupOffset_;
            for(int i = 0; i < groupSize_; ++i){
                for(int j = minFutureDays_; j <= maxFutureDays_; ++j){
                    int srcId = (j - minFutureDays_) * groupSize_ + i;
                    curAlphaGroup[srcId].flag = true;
                    curAlphaGroup[srcId].count = 0;
                    curAlphaGroup[srcId].futureDays = j;
                    curAlphaGroup[srcId].groupType = AlphaGroupType::VALUE_GROUP;
                    curAlphaGroup[srcId].MAE = 0;
                    curAlphaGroup[srcId].MFE = 0;
                    curAlphaGroup[srcId].groupPars[0] = (i == 0) ? -FLT_MAX : startAlpha + (i - 1) * deltaAlpha;
                    curAlphaGroup[srcId].groupPars[1] = (i == groupSize_ - 1) ? FLT_MAX : i * deltaAlpha;
                }
            }

            return ValueGroupEvalWork::work(alphatreeId, cacheMemoryId, flagCacheId, codesId, cacheGroup, stockSize);
        }



};


#endif //ALPHATREE_VALUEGROUPWORK_H
