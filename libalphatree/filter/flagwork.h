//
// Created by yanyu on 2017/10/3.
//

#ifndef ALPHATREE_FLAGWORK_H
#define ALPHATREE_FLAGWORK_H

#include "alphawork.h"

class FlagWork : public AlphaWork{
    public:
        FlagWork(AlphaForest* alphaForest, float minERatio, size_t minCount, size_t groupNum, size_t groupOffset)
            :AlphaWork(alphaForest), minERatio_(minERatio), minCount_(minCount), groupNum_(groupNum), groupOffset_(groupOffset)
            {}

        virtual size_t work(int alphatreeId, int cacheMemoryId, int flagCacheId, int codesId, AlphaGroup* cacheGroup, int stockSize = 0){
            size_t willGroupNum = 0;
            AlphaGroup* curAlphaGroup = cacheGroup + groupOffset_;
            for(int i = 0; i < groupNum_; ++i){
                if(curAlphaGroup[i].flag){
                    if(curAlphaGroup[i].count < minCount_)
                    {
                        curAlphaGroup[i].flag = false;
                    }
                    else{
                        float eRatio = (curAlphaGroup[i].MFE / curAlphaGroup[i].MAE) / (curAlphaGroup[i].marketMFE / curAlphaGroup[i].marketMAE);
                        if(eRatio < minERatio_)
                            curAlphaGroup[i].flag = false;
                        else
                            ++willGroupNum;
                    }
                }
            }
            if(willGroupNum > 0 && subWorkNum > 0){
                return AlphaWork::work(alphatreeId, cacheMemoryId, flagCacheId, codesId, cacheGroup, stockSize);
            }
            return willGroupNum;
        }

    protected:
        float minERatio_;
        size_t minCount_;
        size_t groupNum_;
        size_t groupOffset_;
};

#endif //ALPHATREE_FLAGWORK_H
