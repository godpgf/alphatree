//
// Created by yanyu on 2017/7/12.
//
#ifndef ALPHATREE_ALPHATREE_H
#define ALPHATREE_ALPHATREE_H

#include "basealphatree.h"
#include "../base/threadpool.h"

#define MAX_PROCESS_STR_LEN 4096 * 64
const size_t CODE_LEN = 64;

class AlphaCache{
    public:
        AlphaCache(){
            nodeCacheSize = MAX_NODE_BLOCK;
            processCacheSize = MAX_PROCESS_BLOCK;
            sampleCacheSize = SAMPLE_DAYS;
            stockCacheSize = MAX_NODE_BLOCK * GET_ELEMEMT_SIZE(HISTORY_DAYS, SAMPLE_DAYS) *
                             STOCK_SIZE;
            nodeRes = new std::shared_future<float*>[nodeCacheSize];
            flagRes = new std::shared_future<bool*>[nodeCacheSize];
            processRes = new std::shared_future<const char*>[processCacheSize];
            sampleFlag = new bool[sampleCacheSize];
            result = new float[stockCacheSize];
            resultFlag = new bool[stockCacheSize];
            process = new char[processCacheSize * MAX_PROCESS_STR_LEN];
            //stockFlag = new CacheFlag[stockCacheSize];
            dayCacheSize = MAX_NODE_BLOCK * GET_ELEMEMT_SIZE(HISTORY_DAYS, SAMPLE_DAYS);
            dayFlag = new CacheFlag[dayCacheSize];
            nodeFlag = new bool[nodeCacheSize];
            codeCacheSize = STOCK_SIZE * CODE_LEN;
            codes = new char[codeCacheSize];
        }
        ~AlphaCache(){
            delete []nodeRes;
            delete []flagRes;
            delete []processRes;
            delete []sampleFlag;
            delete []result;
            delete []resultFlag;
            //delete []stockFlag;
            delete []dayFlag;
            delete []nodeFlag;
            delete []codes;
        }

        void initialize(size_t nodeSize, size_t processSize, size_t historyDays, size_t dayBefore, bool* sampleFlag, size_t sampleDays, const char *codes, size_t stockSize, bool isCalAllNode = false){
            const char* curSrcCode = codes;
            char* curDstCode = this->codes;
            for(size_t i = 0; i < stockSize; ++i){
                strcpy(curDstCode, curSrcCode);
                int codeLen = strlen(curSrcCode);
                curSrcCode += (codeLen+1);
                curDstCode += (codeLen+1);
            }
            initialize(nodeSize, processSize, historyDays, dayBefore, sampleFlag, sampleDays, stockSize, isCalAllNode);
        }

        void initialize(size_t nodeSize, size_t processSize, size_t historyDays, AlphaDB* alphaDatabase){
            size_t sampleDays = alphaDatabase->getDays() - historyDays + 1;
            initialize(nodeSize, processSize, historyDays, 0, nullptr, sampleDays, alphaDatabase->getStockNum(), true);
            this->stockSize = alphaDatabase->getAllCodes(this->codes);
        }


        //监控某个节点在多线程中的计算状态
        std::shared_future<float*>* nodeRes = {nullptr};
        std::shared_future<bool*>* flagRes = {nullptr};
        //监控某个后处理在多线程中的计算状态
        std::shared_future<const char*>* processRes = {nullptr};
        //取样的天数中哪些部分是需要计算的
        bool* sampleFlag = {nullptr};
        //保存中间计算结果
        float* result = {nullptr};
        //中间结果是否有效的标记
        bool* resultFlag = {nullptr};
        //保存处理结果
        char* process = {nullptr};
        //保存某只股票某日是否需要计算
        //CacheFlag* stockFlag = {nullptr};
        //保存某日所有股票是否需要计算
        CacheFlag* dayFlag = {nullptr};
        //保存某个节点股票是否需要计算
        bool* nodeFlag = {nullptr};
        //保存需要计算的股票代码
        char* codes = {nullptr};
        //记录各个缓存当前大小,如果某个计算要求的大小超过了就需要重新分配内存
        size_t sampleCacheSize = {0};
        size_t nodeCacheSize = {0};
        size_t processCacheSize = {0};
        size_t stockCacheSize = {0};
        size_t dayCacheSize = {0};
        size_t codeCacheSize = {0};

        size_t dayBefore = {0};
        size_t sampleDays = {0};
        size_t stockSize = {0};
        bool isCalAllNode = {false};

    protected:
        void initialize(size_t nodeSize, size_t processSize, size_t historyDays, size_t dayBefore, bool* sampleFlag, size_t sampleDays, size_t stockSize, bool isCalAllNode = false){
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
                delete []flagRes;
                delete []nodeFlag;
                nodeRes = new std::shared_future<float*>[nodeSize];
                flagRes = new std::shared_future<bool*>[nodeSize];
                nodeFlag = new bool[nodeSize];
                nodeCacheSize = nodeSize;
            }

            if(processSize > processCacheSize){
                delete []processRes;
                delete []process;
                processRes = new std::shared_future<const char*>[processSize];
                process = new char[processSize * MAX_PROCESS_STR_LEN];
                processCacheSize = processSize;
            }

            size_t scs = nodeSize * GET_ELEMEMT_SIZE(historyDays, sampleDays) * stockSize;
            if(scs > stockCacheSize){
                delete []result;
                delete []resultFlag;
                //delete []stockFlag;
                result = new float[scs];
                //stockFlag = new CacheFlag[scs];
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

            //更新某个日期是否需要计算的标记
            memset(dayFlag, 0, dcs * sizeof(CacheFlag));

            if(sampleFlag == nullptr){
                memset(this->sampleFlag, 1, sampleDays * sizeof(bool));
            }else{
                memcpy(this->sampleFlag, sampleFlag, sampleDays * sizeof(bool));
            }
        }
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
                for(auto i = 0; i < subtreeList_.getSize(); ++i){
                    if(!subtreeList_[i].isLocal){
                        int historyDays = getMaxHistoryDays(subtreeList_[i].rootId) + 1;
                        if(historyDays > maxHistoryDay_)
                            maxHistoryDay_ = historyDays;
                    }

                }
            }
            return maxHistoryDay_;
        }

        void modifyCoff(int coffId, double coff, AlphaCache* cache){
            //改完系数后某些节点就会过期,需要重新计算,先找到这些节点
            for(auto i = 0; i < nodeList_.getSize(); ++i)
                if(coffId == nodeList_[i].getExternalCoffId()){
                    cache->nodeFlag[i] = false;
                }

            coffList_[coffId] = coff;
        }

        //修改系数后需要重新标记一下哪些数据需要计算
        void reflagAlpha(AlphaDB *alphaDataBase, AlphaCache* cache){
            //标记所有收到影响需要重新计算的节点
            bool hasNewFlag = true;
            while(hasNewFlag){
                hasNewFlag = false;
                for(auto i = 0; i < nodeList_.getSize(); ++i){
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
            for(auto i = 0; i < nodeList_.getSize(); ++i){
                if(cache->nodeFlag[i] == false){
                    CacheFlag* curFlag = getNodeFlag(i, dateSize, cache->dayFlag);
                    memset(curFlag, 0, sizeof(CacheFlag) * dateSize);
                    //curFlag = getNodeCacheMemory(i, dateSize, cache->stockSize, cache->stockFlag);
                    //memset(curFlag, 0, sizeof(CacheFlag) * dateSize * cache->stockSize);
                }
            }

            //重新标记所有节点
            flagAllNode(cache);
        }

        //在主线程中标记需要计算的数据,sampleFlag表示在取样期内哪些数据需要计算,哪些不需要
        void flagAlpha(AlphaDB *alphaDataBase, size_t dayBefore, size_t sampleSize, const char *codes, size_t stockSize, AlphaCache* cache, ThreadPool *threadPool, bool* sampleFlag = nullptr, bool isFlagStock = true, bool isCalAllNode = false){
            //如果缓存空间不够,就重新申请内存
            cache->initialize(nodeList_.getSize(), processList_.getSize(), getMaxHistoryDays(), dayBefore, sampleFlag, sampleSize, codes, stockSize, isCalAllNode);

            //重新标记所有节点
            flagAllNode(cache);
            flagAllStock(alphaDataBase, cache, threadPool, isFlagStock);
        }

        bool* flag(AlphaDB* alphaDataBase, int nodeId, AlphaCache* cache, bool isFlagStock){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            bool* curResultFlag = getNodeCacheMemory(nodeId, dateSize, cache->stockSize, cache->resultFlag);
            CacheFlag * curDayFlagCache = getNodeFlag(nodeId, dateSize, cache->dayFlag);
            if(isFlagStock){
                if(nodeList_[nodeId].getChildNum() == 0){
                    alphaDataBase->getFlag(cache->dayBefore,
                                           getMaxHistoryDays(),
                                           cache->sampleDays,
                                           cache->stockSize,
                                           nodeList_[nodeId].getName(),
                                           curResultFlag,
                                           cache->codes);
                }else{
                    bool* leftFlag = cache->flagRes[nodeList_[nodeId].leftId].get();
                    bool* rightFlag = (nodeList_[nodeId].getChildNum() == 1) ? nullptr : cache->flagRes[nodeList_[nodeId].rightId].get();
                    flag(nodeList_[nodeId].getElement()->getDateRange(), leftFlag, rightFlag, (size_t)nodeList_[nodeId].getNeedBeforeDays(coffList_), dateSize, cache->stockSize, curDayFlagCache, curResultFlag);
                }
            } else{
                memset(curResultFlag, (int)true, dateSize * cache->stockSize * sizeof(bool));
            }

            return curResultFlag;
        }



        //计算alpha并返回,注意返回的是全部数据,要想使用必须加上偏移res + (int)((alphaTree->getHistoryDays() - 1) * stockSize
        void calAlpha(AlphaDB *alphaDataBase, AlphaCache* cache, ThreadPool *threadPool){
            //int rootId = getSubtreeRootId(rootName);
            for(auto i = 0; i < nodeList_.getSize(); ++i) {
                int nodeId = i;
                AlphaTree* alphaTree = this;
                cache->nodeRes[i] = threadPool->enqueue([alphaTree, alphaDataBase, nodeId, cache]{
                    return alphaTree->cast(alphaDataBase, nodeId, cache);
                }).share();
            }
        }

        float* cast(AlphaDB* alphaDataBase, int nodeId, AlphaCache* cache){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            float* curResultCache = getNodeCacheMemory(nodeId, dateSize, cache->stockSize, cache->result);
            bool* curResultFlag = cache->flagRes[nodeId].get();
            CacheFlag * curDayFlagCache = getNodeFlag(nodeId, dateSize, cache->dayFlag);

            if(nodeList_[nodeId].getChildNum() == 0) {
                alphaDataBase->getStock(cache->dayBefore,
                                        getMaxHistoryDays(),
                                        cache->sampleDays,
                                        cache->stockSize,
                                        nodeList_[nodeId].getName(),
                                        //((AlphaPar *) nodeList_[nodeId].getElement())->leafDataType,
                                        nodeList_[nodeId].getWatchLeafDataClass(),
                                        curResultCache,
                                        cache->codes);
//                float s = 0;
//                for(int i = 0; i < 20; ++i)
//                {
//                    float v = curResultCache[cache->stockSize * (dateSize - i - 1) + 201 ];
//                    cout<<v<<" ";
//                    s += v;
//                }
//                cout<<endl;
//                cout<<s/20<<endl;
            }
            else{
                float* leftRes = cache->nodeRes[nodeList_[nodeId].leftId].get();
                float* rightRes = (nodeList_[nodeId].getChildNum() == 1) ? nullptr : cache->nodeRes[nodeList_[nodeId].rightId].get();

                nodeList_[nodeId].getElement()->cast(leftRes, rightRes, nodeList_[nodeId].getCoff(coffList_), dateSize, cache->stockSize,
                                                    curDayFlagCache, curResultFlag, curResultCache);

//                for(size_t i = 0; i < dateSize * cache->stockSize; ++i)
//                    if(isnan(curResultCache[i])){
//                        cout<<"error "<<nodeList_[nodeId].getName()<<endl;
//                        cout<<"left "<<leftRes[i]<<endl;
//                        if(rightRes)
//                            cout<<"right "<<rightRes[i]<<endl;
//                    }

            }
            for(auto i = 0; i < dateSize; ++i){
                if(curDayFlagCache[i] == CacheFlag::NEED_CAL)
                    curDayFlagCache[i] = CacheFlag::HAS_CAL;
            }

            return curResultCache;
        }

        void processAlpha(AlphaDB *alphaDataBase, AlphaCache* cache, ThreadPool *threadPool){
            for(auto i = 0; i < processList_.getSize(); ++i){
                int processId = i;
                AlphaTree* alphaTree = this;
                cache->processRes[i] = threadPool->enqueue([alphaTree, alphaDataBase, processId, cache]{
                    return alphaTree->process(alphaDataBase, processId, cache);
                }).share();
            }
        }

        const char* process(AlphaDB *alphaDataBase, int processId, AlphaCache* cache){
            for(auto i = 0; i < processList_[processId].chilIndex.getSize(); ++i)
                processList_[processId].childRes[i] = getAlpha(subtreeList_[processList_[processId].chilIndex[i]].rootId, cache);
            return processList_[processId].getProcess()->process(alphaDataBase, processList_[processId].childRes, processList_[processId].coff, cache->sampleDays, cache->codes, cache->stockSize, cache->process + (processId * MAX_PROCESS_STR_LEN));
        }

    //保存alphatree到alphaDB,isFeature表示是否作为机器学习的特征
        void cacheAlpha(AlphaDB *alphaDataBase, AlphaCache* cache, ThreadPool *threadPool, bool isFeature = false){
            cache->initialize(nodeList_.getSize(), processList_.getSize(), getMaxHistoryDays(), alphaDataBase);
            //int dateSize = alphaDataBase->getDays();
            //重新标记所有节点
            flagAllNode(cache);
            flagAllStock(alphaDataBase, cache, threadPool, true);
            calAlpha(alphaDataBase, cache, threadPool);
            for(auto i = 0; i < subtreeList_.getSize(); ++i) {
                if (!subtreeList_[i].isLocal) {
                    string name = subtreeList_[i].name;
                    float *alpha = cache->nodeRes[subtreeList_[i].rootId].get();
                    bool *flag = cache->flagRes[subtreeList_[i].rootId].get();
                    int needDay = getMaxHistoryDays() - 1;

                    char des[MAX_SUB_ALPHATREE_STR_NUM];
                    encode(subtreeList_[i].name, des);
                    alphaDataBase->setElement(name, des, needDay, alpha, flag, isFeature);
                }
            }
        }

        const float* getAlpha(const char* rootName, AlphaCache* cache){
            return getAlpha(getSubtreeRootId(rootName), cache);
        }

        const char* getProcess(const char* processName, AlphaCache* cache){
            for(auto i = 0; i < processList_.getSize(); ++i){
                if(strcmp(processList_[i].name, processName) == 0){
                    return cache->processRes[i].get();
                }
            }
            return nullptr;
        }

        //读取已经计算好的alpha
        const float* getAlpha(int nodeId, AlphaCache* cache){
            //int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            //float* result = AlphaTree::getNodeCacheMemory(nodeId, dateSize, stockSize, resultCache);
            float* result = cache->nodeRes[nodeId].get();
            //CacheFlag* flag = AlphaTree::getNodeCacheMemory(nodeId, dateSize, cache->stockSize, cache->stockFlag);
            //flagResult(result, flag, dateSize * cache->stockSize);
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
            return nodeList_[nodeId].getNeedBeforeDays() + maxDays;
        }


        /*void flagAllNode(AlphaDB *alphaDataBase, AlphaCache* cache){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            const char* curCode = cache->codes;

            for(int stockIndex = 0; stockIndex < cache->stockSize; ++ stockIndex){
                //cout<<curCode<<":";
                for(size_t dayIndex = 0; dayIndex < cache->sampleDays; ++dayIndex){
                    if(cache->sampleFlag[dayIndex]){
                        for(auto i = 0; i < subtreeList_.getSize(); ++i){
                            if(!subtreeList_[i].isLocal) {
                                int curIndex = dayIndex + getMaxHistoryDays() - 1;
                                if(stockIndex == 0)
                                    flagNodeDay(subtreeList_[i].rootId, curIndex, dateSize, cache->sampleDays, cache->dayFlag, cache->isCalAllNode);
                                flagNodeStock(subtreeList_[i].rootId, curIndex, stockIndex, alphaDataBase, cache->dayBefore,
                                              cache->sampleDays, cache->resultFlag, curCode, cache->stockSize, cache->isCalAllNode);
                            }
                        }
                    }

                }
                curCode = curCode + (strlen(curCode) + 1);
            }

            for(auto i = 0; i < nodeList_.getSize(); ++i)
                cache->nodeFlag[i] = true;
        }*/
        void flagAllStock(AlphaDB *alphaDataBase, AlphaCache* cache, ThreadPool* threadPool, bool isFlagStock){
            for(auto i = 0; i < nodeList_.getSize(); ++i) {
                int nodeId = i;
                AlphaTree* alphaTree = this;
                cache->flagRes[i] = threadPool->enqueue([alphaTree, alphaDataBase, nodeId, cache, isFlagStock]{
                    return alphaTree->flag(alphaDataBase, nodeId, cache, isFlagStock);
                }).share();
                //alphaTree->flag(alphaDataBase, nodeId, cache);
            }
        }

        void flagAllNode(AlphaCache* cache){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), cache->sampleDays);
            for(size_t dayIndex = 0; dayIndex < cache->sampleDays; ++dayIndex){
                if(cache->sampleFlag[dayIndex]){
                    for(auto i = 0; i < subtreeList_.getSize(); ++i){
                        if(!subtreeList_[i].isLocal) {
                            int curIndex = dayIndex + getMaxHistoryDays() - 1;
                            flagNodeDay(subtreeList_[i].rootId, curIndex, dateSize, cache);
                        }
                    }
                }
            }

            for(auto i = 0; i < nodeList_.getSize(); ++i)
                cache->nodeFlag[i] = true;
        }

        void flagNodeDay(int nodeId, int dayIndex, int dateSize, AlphaCache* cache){
            CacheFlag* curFlag = getNodeFlag(nodeId, dateSize, cache->dayFlag);
            bool* curStockFlag = getNodeCacheMemory(nodeId, dateSize, cache->stockSize, cache->resultFlag) + dayIndex * cache->stockSize;
            if(curFlag[dayIndex] == CacheFlag::NO_FLAG){

                //填写数据
                curFlag[dayIndex] = CacheFlag::NEED_CAL ;
                if(nodeList_[nodeId].getChildNum() > 0){
                    for(size_t i = 0; i < cache->stockSize; ++i)
                        curStockFlag[i] = true;
                    int dayNum = (int)roundf(nodeList_[nodeId].getNeedBeforeDays(coffList_));
                    switch(nodeList_[nodeId].getElement()->getDateRange()){
                        case DateRange::CUR_DAY:
                            flagAllChild(nodeId, dayIndex, dateSize, cache);
                            //flagStock(getLeftFlag(nodeId, dayIndex, dateSize, cache), getRightFlag(nodeId, dayIndex, dateSize, cache), curStockFlag, cache->stockSize);
                            break;
                        case DateRange::BEFORE_DAY:
                            flagAllChild( nodeId, dayIndex - dayNum, dateSize, cache);
                            //flagStock(getLeftFlag(nodeId, dayIndex - dayNum, dateSize, cache), getRightFlag(nodeId, dayIndex - dayNum, dateSize, cache), curStockFlag, cache->stockSize);
                            break;
                        case DateRange::CUR_AND_BEFORE_DAY:
                            flagAllChild( nodeId, dayIndex, dateSize, cache);
                            flagAllChild( nodeId, dayIndex - dayNum, dateSize, cache);
                            //flagStock(getLeftFlag(nodeId, dayIndex, dateSize, cache), getRightFlag(nodeId, dayIndex, dateSize, cache), curStockFlag, cache->stockSize);
                            //flagStock(getLeftFlag(nodeId, dayIndex - dayNum, dateSize, cache), getRightFlag(nodeId, dayIndex - dayNum, dateSize, cache), curStockFlag, cache->stockSize);
                            break;
                        case DateRange::ALL_DAY:
                            for(auto i = 0; i <= dayNum; ++i){
                                flagAllChild(nodeId, dayIndex - i, dateSize, cache);
                                //flagStock(getLeftFlag(nodeId, dayIndex - dayNum, dateSize, cache), getRightFlag(nodeId, dayIndex - i, dateSize, cache), curStockFlag, cache->stockSize);
                            }
                            break;
                    }
                    if(cache->isCalAllNode){
                        for(size_t i = getMaxHistoryDays() - 1; i < getMaxHistoryDays() + cache->sampleDays - 1; ++i){
                            flagAllChild( nodeId, i, dateSize, cache);
                        }
                    }
                }
//                else{
//                    alphaDataBase->getFlag(dayIndex, cache->dayBefore, getMaxHistoryDays(), cache->sampleDays, cache->stockSize, curStockFlag, cache->codes);
//                }
            }
        }

        /*void flagNodeStock(int nodeId, int dayIndex, int stockIndex, AlphaDB *alphaDataBase, size_t dayBefore,
                           size_t sampleSize, bool *flagCache,
                           const char *curCode, size_t stockSize, bool isCalAllNode = false){
            int dateSize = GET_ELEMEMT_SIZE(getMaxHistoryDays(), sampleSize);
            CHECK(dayIndex >= 0 && dayIndex < dateSize, "标记错误");
            bool * curFlagCache = getNodeCacheMemory(nodeId, dateSize, stockSize, flagCache);
            int deltaSpace = dateSize + dayBefore - dayIndex;
            //只需要标记没有标记过的
            int curIndex = dayIndex * stockSize + stockIndex;
            if(curFlagCache[curIndex] == false){
                if(nodeList_[nodeId].getChildNum() == 0){
                    bool* alphaDataBase->getFlag()
                    Stock* stock = alphaDataBase->getStock(curCode, nodeList_[nodeId].getWatchLeafDataClass());
                    if(stock == nullptr || stock->size < deltaSpace || stock->volume[stock->size - deltaSpace] <= 0){
                        curFlagCache[curIndex] = CacheFlag::CAN_NOT_FLAG;
                        //cout<<stock->code<<" "<<curCode<<" "<<nodeList_[nodeId].getWatchLeafDataClass()<<" "<<stock->size<<"-"<<deltaSpace<<" "<<stockIndex<<" stock err"<<endl;
                    } else{
                        curFlagCache[curIndex] = CacheFlag::NEED_CAL;
                    }
                } else{
                    //先标记孩子
                    int dayNum = (int)roundf(nodeList_[nodeId].getNeedBeforeDays(coffList_));
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
                            for(auto i = 0; i <= dayNum; ++i)
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
                            for(auto i = 0; i <= dayNum; ++i){
                                if(getFlag(leftFlagCache, rightFlagCache, curIndex - i * stockSize) == CacheFlag::CAN_NOT_FLAG){
                                    curFlagCache[curIndex] = CacheFlag::CAN_NOT_FLAG;
                                    return;
                                }
                            }
                            break;
                    }
                }
            }
        }*/

        int maxHistoryDay_;

    private:
        inline static bool* flag(DateRange dataRange, const bool* pleft, const bool* pright, size_t day, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pout){

            if(day > 0){
                memset(pout, 0, day * stockSize * sizeof(bool));
            }
            for(size_t j = 0; j < stockSize; ++j){
                for(size_t i = day; i < historySize; ++i) {
                    if(pflag[i] == CacheFlag::NEED_CAL) {
                        size_t watchIndex = 0;
                        size_t curIndex = i * stockSize + j;
                        switch(dataRange){
                            case DateRange::CUR_DAY:
                                watchIndex = i * stockSize + j;
                                pout[curIndex] = (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]));
                                break;
                            case DateRange::BEFORE_DAY:
                                watchIndex = (i - day) * stockSize + j;
                                pout[curIndex] = (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]));
                                break;
                            case DateRange::CUR_AND_BEFORE_DAY:
                                watchIndex = i * stockSize + j;
                                pout[curIndex] = (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]));
                                if(pout[curIndex]) {
                                    watchIndex = (i - day) * stockSize + j;
                                    pout[curIndex] = (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]));
                                }
                                break;
                            case DateRange::ALL_DAY:
                                if(i > day && pout[curIndex - stockSize]){
                                    watchIndex = i * stockSize + j;
                                    pout[curIndex] = (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]));
                                } else {
                                    watchIndex = (i - day - 1) * stockSize + j;
                                    if(i > day && (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]))){
                                        pout[curIndex] = false;
                                    }
                                    else{
                                        for(size_t k = 0; k <= day; ++k){
                                            watchIndex = (i - k) * stockSize + j;
                                            pout[curIndex] = (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]));
                                            if(pout[curIndex] == false)
                                                break;
                                        }
                                    }

                                }
                                break;
                        }

                    } else {
                        memset(pout + i * stockSize, 0, stockSize * sizeof(bool));
                    }
                }
            }
            return pout;
        }

        inline void flagAllChild(int nodeId, int dayIndex, int dateSize, AlphaCache* cache){
            flagNodeDay(nodeList_[nodeId].leftId, dayIndex, dateSize, cache);
            if(nodeList_[nodeId].getChildNum() > 1)
                flagNodeDay(nodeList_[nodeId].rightId, dayIndex, dateSize, cache);
        }

//        static inline void flagStock(const bool* leftFlag, const bool* rightFlag, bool* resultflag, size_t stockSize){
//            for(size_t i = 0; i < stockSize; ++i){
//                if(resultflag[i]){
//                    if(!leftFlag[i])
//                        resultflag[i] = false;
//                    else if(rightFlag && !rightFlag[i])
//                        resultflag[i] = false;
//                }
//            }
//        }

        static inline CacheFlag* getNodeFlag(int nodeId, int dateSize, CacheFlag* flagCache){
            return flagCache + nodeId * dateSize;
        }

        inline bool* getLeftFlag(int nodeId, int dayIndex, int dateSize, AlphaCache* cache){
            return nodeList_[nodeId].getChildNum() > 0 ? getNodeCacheMemory(nodeList_[nodeId].leftId, dateSize, cache->stockSize, cache->resultFlag) + dayIndex * cache->stockSize: nullptr;
        }
        inline bool* getRightFlag(int nodeId, int dayIndex, int dateSize, AlphaCache* cache){
            return nodeList_[nodeId].getChildNum() > 1 ? getNodeCacheMemory(nodeList_[nodeId].rightId, dateSize, cache->stockSize, cache->resultFlag) + dayIndex * cache->stockSize: nullptr;
        }
};


#endif //ALPHATREE_ALPHATREE_H
