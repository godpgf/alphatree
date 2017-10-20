//
// Created by yanyu on 2017/10/3.
//

#ifndef ALPHATREE_COLLECTWORK_H
#define ALPHATREE_COLLECTWORK_H

#include "alphawork.h"

class CollectWork : public AlphaWork{
    public:
        CollectWork(AlphaForest* alphaForest, size_t groupNum, size_t groupOffset = 0)
                :AlphaWork(alphaForest), groupNum_(groupNum), groupOffset_(groupOffset){

        }

        virtual size_t work(int alphatreeId, int cacheMemoryId, int flagCacheId, int codesId, AlphaGroup* cacheGroup, int stockSize = 0){
            size_t willGroupSize = AlphaWork::work(alphatreeId, cacheMemoryId, flagCacheId, codesId, cacheGroup, stockSize);
            if(willGroupSize > 0){
                AlphaGroup* curAlphaGroup = cacheGroup + groupOffset_;
                size_t startIndex = 0;
                while(startIndex < willGroupSize && curAlphaGroup[startIndex].flag)
                    ++startIndex;
                size_t id = startIndex+1;
                while (startIndex < willGroupSize){
                    while (curAlphaGroup[id].flag == false){
                        ++id;
                        if(id >= willGroupSize)
                            throw "err";
                    }
                    curAlphaGroup[startIndex++] = curAlphaGroup[id++];
                }
            }
            return willGroupSize;
        }

    protected:
        size_t groupNum_;
        size_t groupOffset_;
};

#endif //ALPHATREE_COLLECTWORK_H
