//
// Created by yanyu on 2017/7/12.
//
#ifndef ALPHATREE_ALPHATREE_H
#define ALPHATREE_ALPHATREE_H

#include "basealphatree.h"
#include "../base/threadpool.h"

class AlphaCache{
    public:
        AlphaCache(){
            nodeCacheSize = NODE_SIZE;
            sampleCacheSize = SAMPLE_DAYS;
            stockCacheSize = NODE_SIZE * GET_ELEMEMT_SIZE(HISTORY_DAYS, SAMPLE_DAYS) *
                             STOCK_SIZE;
            nodeRes = new std::shared_future<float*>[nodeCacheSize];
            sampleFlag = new bool[sampleCacheSize];
            result = new float[stockCacheSize];
            stockFlag = new CacheFlag[stockCacheSize];
            dayCacheSize = NODE_SIZE * GET_ELEMEMT_SIZE(HISTORY_DAYS, SAMPLE_DAYS);
            dayFlag = new CacheFlag[dayCacheSize];
            nodeFlag = new bool[nodeCacheSize];
            codeCacheSize = STOCK_SIZE * CODE_LEN;
            codes = new char[codeCacheSize];
        }
        ~AlphaCache(){
            delete []nodeRes;
            delete []sampleFlag;
            delete []result;
            delete []stockFlag;
            delete []dayFlag;
            delete []nodeFlag;
            delete []codes;
        }

        void initialize(size_t nodeSize, size_t historyDays, size_t dayBefore, bool* sampleFlag, size_t sampleDays, const char *codes, size_t stockSize, bool isCalAllNode = false){
            this->dayBefore = dayBefore;
            this->sampleDays = sampleDays;
            this->stockSize = stockSize;
            this->isCalAllNode = isCalAllNode;

            if(sampleDays > sampleCacheSize){
                delete[] this->sampleFlag;
                this->sampleFlag = new bool[sampleDays];
                sampleCacheSize = sampleDays;
            }

            if(nodeSize > nodeCacheSize){
                delete []nodeRes;
                delete []nodeFlag;
                nodeRes = new std::shared_future<float*>[nodeSize];
                nodeFlag = new bool[nodeSize];
                nodeCacheSize = nodeSize;
            }

            size_t scs = nodeSize * GET_ELEMEMT_SIZE(historyDays, sampleDays) * stockSize;
            if(scs > stockCacheSize){
                delete []result;
                delete []stockFlag;
                result = new float[scs];
                stockFlag = new CacheFlag[scs];
                stockCacheSize = scs;
            }

            size_t dcs = nodeSize * GET_ELEMEMT_SIZE(historyDays, sampleDays);
            if(dcs > dayCacheSize){
                delete []dayFlag;
                dayFlag = new CacheFlag[dcs];
                dayCacheSize = dcs;
            }

            size_t ccs = stockSize * CODE_LEN;
            if(ccs > codeCacheSize){
                delete []this->codes;
                this->codes = new char[ccs];
                codeCacheSize = ccs;
            }

            const char* curSrcCode = codes;
            char* curDstCode = this->codes;
            for(int i = 0; i < stockSize; ++i){
                strcpy(curDstCode, curSrcCode);
                int codeLen = strlen(curSrcCode);
                curSrcCode += (codeLen+1);
                curDstCode += (codeLen+1);
            }

            if(sampleFlag == nullptr){
                memset(this->sampleFlag, 1, sampleDays * sizeof(bool));
            }else{
                memcpy(this->sampleFlag, sampleFlag, sampleDays * sizeof(bool));
            }
        }


        //监控某个节点在多线程中的计算状态
        std::shared_future<float*>* nodeRes = {nullptr};
        //取样的天数中哪些部分是需要计算的
        bool* sampleFlag = {nullptr};
        //保存中间计算结果
        float* result = {nullptr};
        //保存某只股票某日是否需要计算
        CacheFlag* stockFlag = {nullptr};
        //保存某日所有股票是否需要计算
        CacheFlag* dayFlag = {nullptr};
        //保存某个节点股票是否需要计算
        bool* nodeFlag = {nullptr};
        //保存需要计算的股票代码
        char* codes = {nullptr};
        //记录各个缓存当前大小,如果某个计算要求的大小超过了就需要重新分配内存
        size_t sampleCacheSize = {0};
        size_t nodeCacheSize = {0};
        size_t stockCacheSize = {0};
        size_t dayCacheSize = {0};
        size_t codeCacheSize = {0};

        size_t dayBefore = {0};
        size_t sampleDays = {0};
        size_t stockSize = {0};
        bool isCalAllNode = {false};
};


class AlphaTree : public BaseAlphaTree{
    public:
        AlphaTree():BaseAlphaTree(),maxHistoryDay_(NONE){}

        virtual void clean(){
            maxHistoryDay_ = NONE;
            BaseAlphaTree::clean();
        }

        int getMaxHistoryDays(){
            if(maxHistoryDay_ == NONE){
                for(int i = 0; i < subtreeList_.getSize(); ++i){
                    if(!subtreeList_[i].isLocal){
                        int historyDays = getMaxHistoryDays(subtreeList_[i].rootId) + 1;
                        if(historyDays > maxHistoryDay_)
                            maxHistoryDay_ = historyDays;
                    }

                }
            }
            return maxHistoryDay_;
        }

        //在主线程中标记需要计算的数据,sampleFlag表示在取样期内哪些数据需要计算,哪些不需要
        void flagAlpha(AlphaDataBase *alphaDataBase, size_t dayBefore, size_t sampleSize, const char *codes, size_t stockSize, AlphaCache* cache, bool* sampleFlag = nullptr, bool isCalAllNode = false){
            //如果缓存空间不够,就重新申请内存
            cache->initialize(nodeList_.getSize(), getMaxHistoryDays(), dayBefore, sampleFlag, sampleSize, codes, stockSize, isCalAllNode);
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), sampleSize);

            if(sampleFlag == nullptr)
                memset(cache->sampleFlag, 1, sizeof(bool) * sampleSize);
            else
                memcpy(cache->sampleFlag, sampleFlag, sizeof(bool) * sampleSize);

            //更新某个日期是否需要计算的标记
            memset(cache->dayFlag, 0, dateSize * nodeList_.getSize() *
                                                 sizeof(CacheFlag));
            //更新某个股票的某个日期是否需要计算的标记
            memset(cache->stockFlag, 0, dateSize * stockSize * nodeList_.getSize() *
                                        sizeof(CacheFlag));

            //重新标记所有节点
            flagAllNode(alphaDataBase, cache);
        }

        void modifyCoff(int coffId, double coff, AlphaCache* cache){
            //改完系数后某些节点就会过期,需要重新计算,先找到这些节点
            for(int i = 0; i < nodeList_.getSize(); ++i)
                if(coffId == nodeList_[i].getExternalCoffId()){
                    cache->nodeFlag[i] = false;
                }

            coffList_[coffId] = coff;
        }

        //修改系数后需要重新标记一下哪些数据需要计算
        void reflagAlpha(AlphaDataBase *alphaDataBase, AlphaCache* cache){
            //标记所有收到影响需要重新计算的节点
            bool hasNewFlag = true;
            while(hasNewFlag){
                hasNewFlag = false;
                for(int i = 0; i < nodeList_.getSize(); ++i){
                    if(cache->nodeFlag[i] && nodeList_[i].getChildNum() > 0){
                        if(cache->nodeFlag[nodeList_[i].leftId] == false){
                            hasNewFlag = true;
                            cache->nodeFlag[i] = false;
                        } else if(nodeList_[i].getChildNum() > 1 && cache->nodeFlag[nodeList_[i].rightId] == false){
                            hasNewFlag = true;
                            cache->nodeFlag[i] = false;
                        }
                    }
                }
            }

            //清空需要重新计算的节点标记
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            for(int i = 0; i < nodeList_.getSize(); ++i){
                if(cache->nodeFlag[i] == false){
                    CacheFlag* curFlag = getNodeFlag(i, dateSize, cache->dayFlag);
                    memset(curFlag, 0, sizeof(CacheFlag) * dateSize);
                    curFlag = getNodeCacheMemory(i, dateSize, cache->stockSize, cache->stockFlag);
                    memset(curFlag, 0, sizeof(CacheFlag) * dateSize * cache->stockSize);
                }
            }

            //重新标记所有节点
            flagAllNode(alphaDataBase, cache);
        }

        //计算alpha并返回,注意返回的是全部数据,要想使用必须加上偏移res + (int)((alphaTree->getHistoryDays() - 1) * stockSize
        void calAlpha(AlphaDataBase *alphaDataBase, AlphaCache* cache, ThreadPool *threadPool){
            //int rootId = getSubtreeRootId(rootName);
            for(int i = 0; i < nodeList_.getSize(); ++i) {
                int nodeId = i;
                AlphaTree* alphaTree = this;
                cache->nodeRes[i] = threadPool->enqueue([alphaTree, alphaDataBase, nodeId, cache]{
                    return alphaTree->cast(alphaDataBase, nodeId, cache);
                }).share();
            }
        }

        float* cast(AlphaDataBase* alphaDataBase, int nodeId, AlphaCache* cache){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            float* curResultCache = getNodeCacheMemory(nodeId, dateSize, cache->stockSize, cache->result);
            CacheFlag * curDayFlagCache = getNodeFlag(nodeId, dateSize, cache->dayFlag);

            if(nodeList_[nodeId].getChildNum() == 0) {
                alphaDataBase->getStock(cache->dayBefore,
                                        getMaxHistoryDays(),
                                        cache->sampleDays,
                                        cache->stockSize,
                                        ((AlphaPar *) nodeList_[nodeId].getElement())->leafDataType,
                                        nodeList_[nodeId].getWatchLeafDataClass(),
                                        curResultCache,
                                        cache->codes);

                /*CacheFlag* flag = AlphaTree::getNodeCacheMemory(nodeId, dateSize, cache->sampleDays, cache->stockFlag);
                cout<<(int)flag[(getMaxHistoryDays() - 3) * cache->stockSize]
                <<(int)flag[(getMaxHistoryDays() - 2) * cache->stockSize]
                <<(int)flag[(getMaxHistoryDays() - 1) * cache->stockSize]<<endl;
                cout<<curResultCache[(getMaxHistoryDays() - 3) * cache->stockSize]
                <<curResultCache[(getMaxHistoryDays() - 2) * cache->stockSize]
                <<curResultCache[(getMaxHistoryDays() - 1) * cache->stockSize]<<endl;*/
            }
            else{
                float* leftRes = cache->nodeRes[nodeList_[nodeId].leftId].get();
                float* rightRes = (nodeList_[nodeId].getChildNum() == 1) ? nullptr : cache->nodeRes[nodeList_[nodeId].rightId].get();

                //如果不能把缺失的数据当做0,还原缺失的数据
                if(!nodeList_[nodeId].getElement()->isCalculateLossDataAsZero()){

                    flagResult(leftRes, getNodeCacheMemory(nodeList_[nodeId].leftId, dateSize, cache->stockSize,
                                                           cache->stockFlag), dateSize * cache->stockSize);
                    if(rightRes)
                        flagResult(rightRes, getNodeCacheMemory(nodeList_[nodeId].rightId, dateSize, cache->stockSize,
                                                                cache->stockFlag), dateSize * cache->stockSize);
                }

                nodeList_[nodeId].getElement()->cast(leftRes, rightRes, nodeList_[nodeId].getCoff(coffList_), dateSize, cache->stockSize,
                                                    curDayFlagCache, curResultCache);
                //cout<<curResultCache[(getMaxHistoryDays() - 1) * cache->stockSize]<<endl;
            }
            for(int i = 0; i < dateSize; ++i){
                if(curDayFlagCache[i] == CacheFlag::NEED_CAL)
                    curDayFlagCache[i] = CacheFlag::HAS_CAL;
            }

            return curResultCache;
        }

        const float* getAlpha(const char* rootName, AlphaCache* cache){
            return getAlpha(getSubtreeRootId(rootName), cache);
        }

        //读取已经计算好的alpha
        const float* getAlpha(int nodeId, AlphaCache* cache){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            //float* result = AlphaTree::getNodeCacheMemory(nodeId, dateSize, stockSize, resultCache);
            float* result = cache->nodeRes[nodeId].get();
            CacheFlag* flag = AlphaTree::getNodeCacheMemory(nodeId, dateSize, cache->stockSize, cache->stockFlag);
            flagResult(result, flag, dateSize * cache->stockSize);
            return result + (int)((getMaxHistoryDays() - 1) * cache->stockSize);
        }

        static const float* getAlpha(const float* res, size_t sampleIndex, size_t stockSize){ return res + (sampleIndex * stockSize);}

        template<class T>
        static inline T* getNodeCacheMemory(int nodeId, int dateSize, int stockSize, T* cacheMemory){
            return cacheMemory + nodeId * dateSize * stockSize;
        }


    protected:
        float getMaxHistoryDays(int nodeId){
            if(nodeList_[nodeId].getChildNum() == 0)
                return 0;
            float leftDays = getMaxHistoryDays(nodeList_[nodeId].leftId);
            float rightDays = 0;
            if(nodeList_[nodeId].getChildNum() == 2)
                rightDays = getMaxHistoryDays(nodeList_[nodeId].rightId);
            int maxDays = leftDays > rightDays ? leftDays : rightDays;
            if(nodeList_[nodeId].getCoffUnit() == CoffUnit::COFF_DAY)
                return roundf(nodeList_[nodeId].getCoff()) + maxDays;
            return maxDays;
        }


        void flagAllNode(AlphaDataBase *alphaDataBase, AlphaCache* cache){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            const char* curCode = cache->codes;

            for(int stockIndex = 0; stockIndex < cache->stockSize; ++ stockIndex){
                //cout<<curCode<<":";
                for(int dayIndex = 0; dayIndex < cache->sampleDays; ++dayIndex){
                    if(cache->sampleFlag[dayIndex]){
                        for(int i = 0; i < subtreeList_.getSize(); ++i){
                            if(!subtreeList_[i].isLocal) {
                                int curIndex = dayIndex + getMaxHistoryDays() - 1;
                                if(stockIndex == 0)
                                    flagNodeDay(subtreeList_[i].rootId, curIndex, dateSize, cache->sampleDays, cache->dayFlag, cache->isCalAllNode);
                                flagNodeStock(subtreeList_[i].rootId, curIndex, stockIndex, alphaDataBase, cache->dayBefore,
                                              cache->sampleDays, cache->stockFlag, curCode, cache->stockSize, cache->isCalAllNode);
                            }
                        }
                    }

                }
                curCode = curCode + (strlen(curCode) + 1);
            }

            for(int i = 0; i < nodeList_.getSize(); ++i)
                cache->nodeFlag[i] = true;
        }

        void flagNodeDay(int nodeId, int dayIndex, int dateSize, size_t sampleSize, CacheFlag *flagCache, bool isCalAllNode = false){
            CacheFlag* curFlag = getNodeFlag(nodeId, dateSize, flagCache);
            if(curFlag[dayIndex] == CacheFlag::NO_FLAG){
                curFlag[dayIndex] = CacheFlag::NEED_CAL ;
                if(nodeList_[nodeId].getChildNum() > 0){
                    int dayNum = (int)roundf(nodeList_[nodeId].getCoff(coffList_));
                    switch(nodeList_[nodeId].getElement()->getDateRange()){
                        case DateRange::CUR_DAY:
                            flagAllChild(nodeId, dayIndex, dateSize, sampleSize, flagCache, isCalAllNode);
                            break;
                        case DateRange::BEFORE_DAY:
                            flagAllChild(nodeId, dayIndex - dayNum, dateSize, sampleSize, flagCache, isCalAllNode);
                            break;
                        case DateRange::CUR_AND_BEFORE_DAY:
                            flagAllChild(nodeId, dayIndex, dateSize, sampleSize, flagCache, isCalAllNode);
                            flagAllChild(nodeId, dayIndex - dayNum, dateSize, sampleSize, flagCache, isCalAllNode);
                            break;
                        case DateRange::ALL_DAY:
                            for(int i = 0; i <= dayNum; ++i)
                                flagAllChild(nodeId, dayIndex - i, dateSize, sampleSize, flagCache, isCalAllNode);
                            break;
                    }
                    if(isCalAllNode){
                        for(int i = getMaxHistoryDays() - 1; i < getMaxHistoryDays() + sampleSize - 1; ++i){
                            flagAllChild(nodeId, i, dateSize, sampleSize, flagCache, isCalAllNode);
                        }
                    }
                }
            }
        }

        void flagNodeStock(int nodeId, int dayIndex, int stockIndex, AlphaDataBase *alphaDataBase, size_t dayBefore,
                           size_t sampleSize, CacheFlag *flagCache,
                           const char *curCode, size_t stockSize, bool isCalAllNode = false){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), sampleSize);
            CHECK(dayIndex >= 0 && dayIndex < dateSize, "标记错误");
            CacheFlag * curFlagCache = getNodeCacheMemory(nodeId, dateSize, stockSize, flagCache);
            int deltaSpace = dateSize + dayBefore - dayIndex;
            //只需要标记没有标记过的
            int curIndex = dayIndex * stockSize + stockIndex;
            if(curFlagCache[curIndex] == CacheFlag::NO_FLAG){
                if(nodeList_[nodeId].getChildNum() == 0){
                    Stock* stock = alphaDataBase->getStock(curCode, nodeList_[nodeId].getWatchLeafDataClass());
                    if(stock == nullptr || stock->size < deltaSpace || stock->volume[stock->size - deltaSpace] <= 0){
                        curFlagCache[curIndex] = CacheFlag::CAN_NOT_FLAG;
                        //cout<<stock->code<<" "<<curCode<<" "<<nodeList_[nodeId].getWatchLeafDataClass()<<" "<<stock->size<<"-"<<deltaSpace<<" "<<stockIndex<<" stock err"<<endl;
                    } else{
                        curFlagCache[curIndex] = CacheFlag::NEED_CAL;
                    }
                } else{
                    //先标记孩子
                    int dayNum = (int)roundf(nodeList_[nodeId].getCoff(coffList_));
                    switch(nodeList_[nodeId].getElement()->getDateRange()){
                        case DateRange::CUR_DAY:
                            flagAllChild(nodeId, dayIndex, stockIndex, alphaDataBase, dayBefore, sampleSize, flagCache, curCode, stockSize, isCalAllNode);
                            break;
                        case DateRange::BEFORE_DAY:
                            flagAllChild(nodeId,dayIndex - dayNum, stockIndex, alphaDataBase, dayBefore, sampleSize, flagCache, curCode, stockSize, isCalAllNode);
                            break;
                        case DateRange::CUR_AND_BEFORE_DAY:
                            flagAllChild(nodeId, dayIndex, stockIndex, alphaDataBase, dayBefore, sampleSize, flagCache, curCode, stockSize, isCalAllNode);
                            flagAllChild(nodeId,dayIndex - dayNum, stockIndex, alphaDataBase, dayBefore, sampleSize, flagCache, curCode, stockSize, isCalAllNode);
                            break;
                        case DateRange::ALL_DAY:
                            for(int i = 0; i <= dayNum; ++i)
                                flagAllChild(nodeId,dayIndex - i, stockIndex, alphaDataBase, dayBefore, sampleSize, flagCache, curCode, stockSize, isCalAllNode);
                            break;
                    }
                    if(isCalAllNode){
                        for(int i = getMaxHistoryDays() - 1; i < getMaxHistoryDays() + sampleSize - 1; ++i){
                            flagAllChild(nodeId,i, stockIndex, alphaDataBase, dayBefore, sampleSize, flagCache, curCode, stockSize, isCalAllNode);
                        }
                    }
                    //再标记自己
                    CacheFlag * leftFlagCache = getNodeCacheMemory(nodeList_[nodeId].leftId, dateSize, stockSize, flagCache);
                    CacheFlag * rightFlagCache = nodeList_[nodeId].getChildNum() > 1 ? getNodeCacheMemory(nodeList_[nodeId].rightId, dateSize, stockSize, flagCache) : nullptr;
                    switch(nodeList_[nodeId].getElement()->getDateRange()){
                        case DateRange::CUR_DAY:
                            curFlagCache[curIndex] = getFlag(leftFlagCache, rightFlagCache, curIndex);
                            break;
                        case DateRange::BEFORE_DAY:
                            curFlagCache[curIndex] = getFlag(leftFlagCache, rightFlagCache, curIndex - dayNum * stockSize);
                            break;
                        case DateRange::CUR_AND_BEFORE_DAY:
                            curFlagCache[curIndex] = CacheFlag::NEED_CAL;
                            if(getFlag(leftFlagCache, rightFlagCache, curIndex) == CacheFlag::CAN_NOT_FLAG || getFlag(leftFlagCache, rightFlagCache, curIndex - dayNum * stockSize) == CacheFlag::CAN_NOT_FLAG)
                                curFlagCache[curIndex] = CacheFlag::CAN_NOT_FLAG;
                            break;
                        case DateRange::ALL_DAY:
                            curFlagCache[curIndex] = CacheFlag::NEED_CAL;
                            for(int i = 0; i <= dayNum; ++i){
                                if(getFlag(leftFlagCache, rightFlagCache, curIndex - i * stockSize) == CacheFlag::CAN_NOT_FLAG){
                                    curFlagCache[curIndex] = CacheFlag::CAN_NOT_FLAG;
                                    return;
                                }
                            }
                            break;
                    }
                }
            }
        }

        int maxHistoryDay_;

    private:
        inline void flagAllChild(int nodeId, int dayIndex, int dateSize, size_t sampleSize, CacheFlag* flagCache, bool isCalAllNode = false){
            flagNodeDay(nodeList_[nodeId].leftId, dayIndex, dateSize, sampleSize, flagCache, isCalAllNode);
            if(nodeList_[nodeId].getChildNum() > 1)
                flagNodeDay(nodeList_[nodeId].rightId, dayIndex, dateSize, sampleSize, flagCache, isCalAllNode);
        }

        inline void flagAllChild(int nodeId, int dayIndex, int stockIndex, AlphaDataBase *alphaDataBase, size_t dayBefore, size_t sampleSize, CacheFlag *flagCache,
                                 const char *curCode, size_t stockSize, bool isCalAllNode = false){
            flagNodeStock(nodeList_[nodeId].leftId, dayIndex, stockIndex, alphaDataBase, dayBefore, sampleSize,
                          flagCache, curCode, stockSize, isCalAllNode);
            if(nodeList_[nodeId].getChildNum() > 1)
                flagNodeStock(nodeList_[nodeId].rightId, dayIndex, stockIndex, alphaDataBase, dayBefore, sampleSize,
                              flagCache, curCode, stockSize, isCalAllNode);
        }

        inline void flagResult(float *result, CacheFlag *flag, int size){
            for(int i = 0; i < size; ++i)
                if(flag[i] == CacheFlag::CAN_NOT_FLAG || flag[i] == CacheFlag::NO_FLAG)
                    result[i] = NAN;
        }

        static inline CacheFlag getFlag(CacheFlag* leftFlagCache, CacheFlag* rightFlagCache, int curIndex){
            CHECK(leftFlagCache[curIndex] != CacheFlag::NO_FLAG ,"不可能");
            if(leftFlagCache[curIndex] == CacheFlag::CAN_NOT_FLAG)
                return CacheFlag::CAN_NOT_FLAG;
            if(rightFlagCache && rightFlagCache[curIndex] == CacheFlag::CAN_NOT_FLAG)
                return CacheFlag::CAN_NOT_FLAG;
            CHECK(!rightFlagCache || rightFlagCache[curIndex] != CacheFlag::NO_FLAG,"不可能");
            return CacheFlag::NEED_CAL;
        }

        static inline CacheFlag* getNodeFlag(int nodeId, int dateSize, CacheFlag* flagCache){
            return flagCache + nodeId * dateSize;
        }
};


#endif //ALPHATREE_ALPHATREE_H
