//
// Created by yanyu on 2017/7/12.
//
#ifndef ALPHATREE_ALPHATREE_H
#define ALPHATREE_ALPHATREE_H

#include "basealphatree.h"
#include "../base/threadpool.h"
#include "../base/dcache.h"
#include "../base/randomchoose.h"
#include <math.h>

//#define MAX_PROCESS_STR_LEN 4096 * 64
//在每个节点的内存中附加一部分用来存放一些统计信息
#define ATTACH_NODE_MEMORY 16

class NodeCache {
public:
    NodeCache() {
        //cache = new char[cacheSize];
    }

    template <class T>
    T* initialize(size_t cacheSize){
        if(cacheSize > this->cacheSize){
            clean();
            this->cacheSize = cacheSize;
            cache = new char[cacheSize];
        }
        return (T*)cache;
    }

    ~NodeCache() {
        clean();
    }

    void clean(){
        if(cache)
            delete[] cache;
        cache = nullptr;
    }

    char *cache = {nullptr};
    size_t cacheSize = {0};
};

class AlphaCache {
public:
    AlphaCache() {
        nodeCacheSize = MAX_NODE_BLOCK;
        nodeRes = new std::shared_future<int>[nodeCacheSize];
        result = DCache<NodeCache>::create();
        dayCacheSize = MAX_NODE_BLOCK * GET_HISTORY_SIZE(HISTORY_DAYS, SAMPLE_DAYS);
        dayFlag = new CacheFlag[dayCacheSize];
        codeCacheSize = STOCK_SIZE * CODE_LEN;
        codes = new char[codeCacheSize];
    }

    ~AlphaCache() {
        delete[]nodeRes;
        DCache<NodeCache>::release(result);
        delete[]dayFlag;
        delete[]codes;
    }

    void initialize(size_t nodeSize, int historyDays, int futureDays, size_t dayBefore, size_t sampleDays, const char *codes,
                    size_t stockSize) {
        const char *curSrcCode = codes;
        char *curDstCode = this->codes;
        if(curSrcCode != curDstCode){
            for (size_t i = 0; i < stockSize; ++i) {
                strcpy(curDstCode, curSrcCode);
                int codeLen = strlen(curSrcCode);
                curSrcCode += (codeLen + 1);
                curDstCode += (codeLen + 1);
            }
        }
        initialize(nodeSize, historyDays, futureDays, dayBefore, sampleDays, sampleDays, stockSize);
    }

    void initialize(size_t nodeSize, int historyDays, int futureDays, size_t dayBefore, size_t sampleDays, size_t startIndex, size_t signNum, size_t signHistoryDays, const char* signName){
        initialize(nodeSize, historyDays, futureDays, dayBefore, sampleDays, signHistoryDays, signNum);
        strcpy(this->signName, signName);
        this->startSignIndex = startIndex;
    }

    void initialize(size_t nodeSize, int historyDays, int futureDays, size_t dayBefore, size_t sampleDays, int outputDays, size_t stockSize) {
        this->dayBefore = dayBefore;
        this->sampleDays = sampleDays;
        this->stockSize = stockSize;
        this->signName[0] = 0;
        this->outputDays = outputDays;

        if (nodeSize > nodeCacheSize) {
            delete[]nodeRes;
            //delete[]nodeFlag;
            nodeRes = new std::shared_future<int>[nodeSize];
            //nodeFlag = new bool[nodeSize];
            nodeCacheSize = nodeSize;
        }

        size_t scs = (GET_HISTORY_SIZE(historyDays, outputDays) - futureDays) * stockSize * sizeof(float) + ATTACH_NODE_MEMORY * sizeof(float);
        if(scs > cacheSizePerNode)
            cacheSizePerNode = scs;
        /*if (scs > NodeCache::cacheSize) {
            DCache<NodeCache>::release(result);
            NodeCache::cacheSize = scs;
            result = DCache<NodeCache>::create();
        }*/

        size_t dcs = nodeSize * GET_HISTORY_SIZE(historyDays, outputDays) - futureDays;
        if (dcs > dayCacheSize) {
            delete[]dayFlag;

            dayFlag = new CacheFlag[dcs];
            dayCacheSize = dcs;
        }

        size_t ccs = stockSize * CODE_LEN;
        if (ccs > codeCacheSize) {
            delete[]this->codes;
            this->codes = new char[ccs];
            codeCacheSize = ccs;
        }

        //更新某个日期是否需要计算的标记
        memset(dayFlag, 0, dcs * sizeof(CacheFlag));
        //回收之前用的节点内存
        result->releaseAll();
    }

    bool isSign(){
        return signName[0] > 0;
    }

    int useMemory(){
        return result->useCacheMemory();
    }

    void releaseMemory(int id){
        result->releaseCacheMemory(id);
    }

    template <class T>
    T* getMemort(int memid){
        return result->getCacheMemory(memid).initialize<T>(cacheSizePerNode);
    }

    size_t getAlphaDays(){
        return outputDays;
    }

    //监控某个节点在多线程中的计算状态
    std::shared_future<int> *nodeRes = {nullptr};

    //保存某日所有股票是否需要计算
    CacheFlag *dayFlag = {nullptr};
    //保存需要计算的股票代码
    char *codes = {nullptr};
    //记录各个缓存当前大小,如果某个计算要求的大小超过了就需要重新分配内存
    size_t nodeCacheSize = {0};
    size_t dayCacheSize = {0};
    size_t codeCacheSize = {0};

    size_t dayBefore = {0};
    //取样期
    size_t sampleDays = {0};
    //输出多少天数据
    int outputDays = {0};

    //股票数量，如果是信号就是信号数量
    size_t stockSize = {0};
    //从第几个信号开始计算
    size_t startSignIndex = {0};
    char signName[64];

protected:


    //保存中间计算结果
    DCache<NodeCache> *result = {nullptr};
    //每个节点可以申请的最大内存大小
    size_t cacheSizePerNode = {GET_HISTORY_SIZE(HISTORY_DAYS, SAMPLE_DAYS) *
                            STOCK_SIZE * sizeof(float) + ATTACH_NODE_MEMORY * sizeof(float)};
};


class AlphaTree : public BaseAlphaTree {
public:
    AlphaTree() : BaseAlphaTree(), maxHistoryDay_(-1),maxFutureDay_(1) {}

    virtual void clean() {
        maxHistoryDay_ = -1;
        maxFutureDay_ = 1;
        BaseAlphaTree::clean();
    }

    //返回需要使用的历史天数
    int getMaxHistoryDays() {
        if (maxHistoryDay_ == -1) {
            for (auto i = 0; i < subtreeList_.getSize(); ++i) {
                //今天也算是历史的一天，所以必须加1
                int historyDays = getMaxHistoryDays(subtreeList_[i].rootId) + 1;
                if (historyDays > maxHistoryDay_)
                    maxHistoryDay_ = historyDays;
            }
        }
        return maxHistoryDay_;
    }

    //返回需要使用的未来天数，注意是负数！！！
    int getMaxFutureDays() {
        if (maxFutureDay_ == 1) {
            for (auto i = 0; i < subtreeList_.getSize(); ++i){
                int futureDays = getMaxFutureDays(subtreeList_[i].rootId);
                if (futureDays < maxFutureDay_)
                    maxFutureDay_ = futureDays;
            }
        }
        return maxFutureDay_;
    }

    //计算alpha并返回,注意返回的是全部数据,要想使用必须加上偏移res + (int)((alphaTree->getHistoryDays() - 1) * stockSize
    void calAlpha(AlphaDB *alphaDataBase, size_t dayBefore, size_t sampleSize, const char *codes, size_t stockSize,
                  AlphaCache *cache, ThreadPool *threadPool) {
        //如果缓存空间不够,就重新申请内存
        cache->initialize(nodeList_.getSize(), getMaxHistoryDays(), getMaxFutureDays(), dayBefore, sampleSize, codes, stockSize);
        //重新标记所有节点
        flagAllNode(cache);
        calAlpha(alphaDataBase, cache, threadPool);
    }

    //计算信号发生时的alpha，返还信号数量。sampleSize是信号发生的取样期，signHistoryDays是读取信号发生时signHistoryDays天的数据
    void calAlpha(AlphaDB *alphaDataBase, size_t dayBefore, size_t sampleSize, size_t startIndex, size_t signNum, size_t signHistoryDays, const char *signName, AlphaCache *cache, ThreadPool *threadPool){
//        cout<<"start init\n";
        cache->initialize(nodeList_.getSize(), getMaxHistoryDays(), getMaxFutureDays(), dayBefore, sampleSize, startIndex, signNum, signHistoryDays, signName);
//        cout<<"finish init\n";
        //重新标记所有节点
        flagAllNode(cache);
//        cout<<"start cal\n";
        calAlpha(alphaDataBase, cache, threadPool);
//        cout<<"finish cal \n";
    }


    //将计算结果存在文件中
    template <class T>
    void cacheAlpha(AlphaDB *alphaDataBase, AlphaCache *cache, ThreadPool *threadPool, const char* featureName) {
        int maxHistoryDays = getMaxHistoryDays();
        int maxFutureDays = getMaxFutureDays();
        //cout<<"start feature a "<<featureName<<endl;
        size_t days = alphaDataBase->getDays();
        //cout<<"start feature c "<<featureName<<endl;
        size_t sampleDays = alphaDataBase->getDays() - maxHistoryDays + 1 + maxFutureDays;
        if(sampleDays > 1024)
            sampleDays = 1024;
        //cout<<"start feature b "<<featureName<<endl;
        size_t dayBefore = days - GET_HISTORY_SIZE(maxHistoryDays, sampleDays);
        size_t stockSize = alphaDataBase->getAllCodes(cache->codes);
        bool isFirstWrite = true;
        ofstream* file = alphaDataBase->createCacheFile(featureName);
        //cout<<"start feature "<<featureName<<endl;
        while (true){
            cache->initialize(nodeList_.getSize(), maxHistoryDays, maxFutureDays, dayBefore, sampleDays, sampleDays, stockSize);
            //重新标记所有节点
            flagAllNode(cache);
            calAlpha(alphaDataBase, cache, threadPool);

            synchroAlpha(cache);
            //写入计算结果
            const float* alpha = getAlpha(featureName, cache);

            bool isLastWrite = (dayBefore + maxFutureDays == 0);

            alphaDataBase->invFill2File<T>(alpha, dayBefore, sampleDays, featureName, file, isFirstWrite, isLastWrite);
            isFirstWrite = false;

            if(isLastWrite)
                break;
            if(dayBefore < sampleDays - maxFutureDays)
                sampleDays = dayBefore + maxFutureDays;
            dayBefore -= sampleDays;
        }
        alphaDataBase->releaseCacheFile(file);
        //cout<<"finish cache "<<featureName<<endl;
    }

    //将信号存在文件中
    void cacheSign(AlphaDB *alphaDataBase, AlphaCache *cache, ThreadPool *threadPool, const char* featureName, const char* codes = nullptr, int codesNum = 0){
        size_t maxHistoryDays = getMaxHistoryDays();
        size_t maxFutureDays = getMaxFutureDays();
        size_t days = alphaDataBase->getDays();
        size_t sampleDays = alphaDataBase->getDays() - maxHistoryDays + 1;
        if(sampleDays > 1024)
            sampleDays = 1024;

        size_t dayBefore = days - GET_HISTORY_SIZE(maxHistoryDays, sampleDays);
        size_t stockSize = alphaDataBase->getAllCodes(cache->codes);

        ofstream* file = alphaDataBase->createCacheFile(featureName);
        size_t preSignCnt = 0;
        size_t preDayNum = 0;
        for(preDayNum = 0; preDayNum < maxHistoryDays-1; ++preDayNum){
            file->write(reinterpret_cast<const char* >( &preSignCnt ), sizeof(size_t));
        }

        bool* stockFlag = nullptr;
        if(codes != nullptr){
            stockFlag = new bool[stockSize];
            alphaDataBase->getCodesFlag(stockFlag, codes, codesNum);
        }

        while (true){
            cache->initialize(nodeList_.getSize(), maxHistoryDays, maxFutureDays, dayBefore, sampleDays, sampleDays, stockSize);
            //重新标记所有节点
            flagAllNode(cache);

            calAlpha(alphaDataBase, cache, threadPool);

            //写入计算结果
            const float* alpha = getAlpha(featureName, cache);
            alphaDataBase->invFill2Sign(alpha, sampleDays, featureName, file, preDayNum, preSignCnt, stockFlag);

            if(dayBefore == 0)
                break;
            if(dayBefore < sampleDays)
                sampleDays = dayBefore;
            dayBefore -= sampleDays;
        }
        alphaDataBase->releaseCacheFile(file);
        if(stockFlag){
            delete []stockFlag;
        }
    }

    //test---------------------------------------
    void testCacheSign(AlphaDB *alphaDataBase, AlphaCache *cache, ThreadPool *threadPool, const char* featureName, const char* testFeatureName){
        size_t maxHistoryDays = getMaxHistoryDays();
        size_t maxFutureDays = getMaxFutureDays();
        int days = alphaDataBase->getDays();
        int sampleDays = alphaDataBase->getDays() - maxHistoryDays + 1;
        if(sampleDays > 1024)
            sampleDays = 1024;

        int dayBefore = days - GET_HISTORY_SIZE(maxHistoryDays, sampleDays);
        size_t stockSize = alphaDataBase->getAllCodes(cache->codes);

        ofstream* file = alphaDataBase->createCacheFile(featureName);
        size_t preSignCnt = 0;
        size_t preDayNum = 0;
        for(preDayNum = 0; preDayNum < maxHistoryDays-1; ++preDayNum){
            file->write(reinterpret_cast<const char* >( &preSignCnt ), sizeof(size_t));
        }
        while (true){
            cache->initialize(nodeList_.getSize(), maxHistoryDays, maxFutureDays, dayBefore, sampleDays, sampleDays, stockSize);
            //重新标记所有节点
            flagAllNode(cache);

            calAlpha(alphaDataBase, cache, threadPool);

            //写入计算结果
            const float* alpha = getAlpha(featureName, cache);
            const float* testAlpha = getAlpha(testFeatureName, cache);

            for(int l = 1; l < sampleDays; ++l){
                for(size_t k = 0; k < stockSize; ++k){
                    if(alpha[l * stockSize + k] > 0){
                        if(testAlpha[l * stockSize + k] < 0)
                            cout<<alpha[l * stockSize + k]<<" "<<alphaDataBase->getCode(k)<<" "<<testAlpha[l * stockSize + k]<<endl;
                        if(testAlpha[(l-1) * stockSize + k] < 0)
                            cout<<alpha[l * stockSize + k]<<" "<<alphaDataBase->getCode(k)<<" "<<testAlpha[(l-1) * stockSize + k]<<endl;
                    }
                }
            }

            alphaDataBase->testInvFill2Sign(alpha, testAlpha, sampleDays, featureName, file, preDayNum, preSignCnt);

            if(dayBefore == 0)
                break;
            if(dayBefore < sampleDays)
                sampleDays = dayBefore;
            dayBefore -= sampleDays;
        }
        alphaDataBase->releaseCacheFile(file);
    }

    float optimizeAlpha(const char *rootName, AlphaDB *alphaDataBase, size_t dayBefore, size_t sampleSize, const char *codes, size_t stockSize,
                       AlphaCache *cache, ThreadPool *threadPool, float exploteRatio = 0.1f, int errTryTime = 64) {
        float* bestCoffList = new float[coffList_.getSize()];

        for(int i = 0; i < coffList_.getSize(); ++i){
            bestCoffList[i] = coffList_[i].coffValue;
        }
        calAlpha(alphaDataBase, dayBefore, sampleSize, codes, stockSize, cache, threadPool);
        float bestRes = getAlpha(rootName, cache)[0];
        if(coffList_.getSize() == 0)
            return bestRes;
        //cout<<"start "<<bestRes<<endl;
        RandomChoose rc = RandomChoose(2 * coffList_.getSize());

        auto curErrTryTime = errTryTime;
        while (curErrTryTime > 0){
            //修改参数
            float lastCoffValue = NAN;
            int curIndex = 0;
            bool isAdd = false;
            while(isnan(lastCoffValue)){
                curIndex = rc.choose();
                isAdd = curIndex < coffList_.getSize();
                curIndex = curIndex % coffList_.getSize();
                int srcIndex = coffList_[curIndex].srcNodeIndex;
                if(isAdd && coffList_[curIndex].coffValue < nodeList_[srcIndex].getElement()->getMaxCoff()){
                    lastCoffValue = coffList_[curIndex].coffValue;
                    if(nodeList_[srcIndex].getCoffUnit() == CoffUnit::COFF_VAR){
                        coffList_[curIndex].coffValue += 0.016f;
                    } else {
                        coffList_[curIndex].coffValue += 1;
                    }
                    coffList_[curIndex].coffValue = std::min(coffList_[curIndex].coffValue, nodeList_[srcIndex].getElement()->getMaxCoff());
                }
                if(!isAdd && coffList_[curIndex].coffValue > nodeList_[srcIndex].getElement()->getMinCoff()){
                    lastCoffValue = coffList_[curIndex].coffValue;
                    if(nodeList_[srcIndex].getCoffUnit() == CoffUnit::COFF_VAR){
                        coffList_[curIndex].coffValue -= 0.016f;
                    } else {
                        coffList_[curIndex].coffValue -= 1;
                    }
                    coffList_[curIndex].coffValue = std::max(coffList_[curIndex].coffValue, nodeList_[srcIndex].getElement()->getMinCoff());
                }
                if(isnan(lastCoffValue)){
                    curIndex = isAdd ? curIndex : coffList_.getSize() + curIndex;
                    rc.reduce(curIndex);
                }

            }

            maxFutureDay_ = 1;
            maxHistoryDay_ = -1;
            calAlpha(alphaDataBase, dayBefore, sampleSize, codes, stockSize, cache, threadPool);
            float res = getAlpha(rootName, cache)[0];
            if(res > bestRes){
                cout<<"best res "<<res<<endl;
                curErrTryTime = errTryTime;
                bestRes = res;
                for(int i = 0; i < coffList_.getSize(); ++i){
                    bestCoffList[i] = coffList_[i].coffValue;
                }
                //根据当前情况决定调整该参数的概率
                curIndex = isAdd ? curIndex : coffList_.getSize() + curIndex;
                rc.add(curIndex);
            } else{
                --curErrTryTime;
                if(!rc.isExplote(exploteRatio)){
                    //恢复现场
                    coffList_[curIndex].coffValue = lastCoffValue;
                }
                curIndex = isAdd ? curIndex : coffList_.getSize() + curIndex;
                rc.reduce(curIndex);
            }

        }

        for(int i = 0; i < coffList_.getSize(); ++i){
            coffList_[i].coffValue = bestCoffList[i];
        }
        delete []bestCoffList;
        //cout<<"finish opt"<<bestRes<<endl;
        return bestRes;
    }

    void synchroAlpha(AlphaCache *cache){
        for(auto i = 0; i < subtreeList_.getSize(); ++i)
            getAlpha(subtreeList_[i].rootId, cache);
    }

    const float *getAlpha(const char *rootName, AlphaCache *cache) {
        return getAlpha(getSubtreeRootId(rootName), cache);
    }

    const float *getAlphaSum(const char *rootName, AlphaCache *cache){
        return getAlphaSum(getSubtreeRootId(rootName), cache);
    }

    void getAlphaSmooth(const char *rootName, AlphaCache *cache, int smoothNum, float* smooth){
        return getAlphaSmooth(getSubtreeRootId(rootName), cache, smoothNum, smooth);
    }

//    const char *getProcess(const char *rootName, AlphaCache *cache) {
//        return getProcess(getSubtreeRootId(rootName), cache);
//    }


    //读取已经计算好的alpha
    float *getAlpha(int nodeId, AlphaCache *cache) {
        int memid = cache->nodeRes[nodeId].get();
        float *result = cache->getMemort<float>(memid);
        return result + (int) ((getMaxHistoryDays() - 1) * cache->stockSize);
    }

    //读取alpha的累加和平方累加
    float* getAlphaSum(int nodeId, AlphaCache *cache){
        float *alpha = getAlpha(nodeId, cache);
        float sum = 0;
        float sqrSum = 0;
        for(int i = cache->getAlphaDays() * cache->stockSize - 1; i >=0; --i){
            sum += alpha[i];
            sqrSum += alpha[i] * alpha[i];
        }
        alpha = alpha + cache->getAlphaDays() * cache->stockSize;
        alpha[0] = sum;
        alpha[1] = sqrSum;
        return alpha;
    }

    void getAlphaSmooth(int nodeId, AlphaCache *cache, int smoothNum, float* smooth){
        getAlphaSmooth(nodeId,cache, smoothNum, smooth, true);
        getAlphaSmooth(nodeId,cache,smoothNum,smooth + smoothNum, false);
    }

//    const char *getProcess(int nodeId, AlphaCache *cache) {
//        return cache->getMemort<char>(cache->nodeRes[nodeId].get());
//    }

    static const float *getAlpha(const float *res, size_t sampleIndex, size_t stockSize) {
        return res + (sampleIndex * stockSize);
    }

    template<class T>
    static inline T *getNodeCacheMemory(int nodeId, int dateSize, int stockSize, T *cacheMemory) {
        return cacheMemory + nodeId * dateSize * stockSize;
    }


protected:
    void getAlphaSmooth(int nodeId, AlphaCache *cache, int smoothNum, float* values, bool isLeft){
        int lastIndex = -1;
        int curIndex = 0;

        float* res = getAlpha(nodeId, cache);

        float lastValue, curValue;
        for (int i = cache->getAlphaDays() * cache->stockSize - 1; i >= 0; --i){
            curValue = res[i];
            if(lastIndex == -1){
                values[0] = curValue;
                lastIndex = 0;
            } else {
                lastValue = values[lastIndex];
                curIndex = lastIndex;
                if(isLeft && curValue < values[lastIndex]){
                    while (curIndex > 0 && values[curIndex-1] > curValue){
                        values[curIndex] = values[curIndex-1];
                        --curIndex;
                    }
                    values[curIndex] = curValue;
                    if(lastIndex < smoothNum - 1)
                        values[++lastIndex] = lastValue;
                } else if(!isLeft && curValue > values[lastIndex]){
                    while (curIndex > 0 && values[curIndex-1] < curValue){
                        values[curIndex] = values[curIndex-1];
                        --curIndex;
                    }
                    values[curIndex] = curValue;
                    if(lastIndex < smoothNum - 1)
                        values[++lastIndex] = lastValue;
                }

            }
        }
    }

    void calAlpha(AlphaDB *alphaDataBase, AlphaCache *cache, ThreadPool *threadPool) {

        //int rootId = getSubtreeRootId(rootName);
        for (auto i = 0; i < nodeList_.getSize(); ++i) {
            int nodeId = i;
            AlphaTree *alphaTree = this;
            //cout<<i<<endl;
            cache->nodeRes[i] = threadPool->enqueue([alphaTree, alphaDataBase, nodeId, cache] {
                return alphaTree->cast(alphaDataBase, nodeId, cache);
            }).share();
        }

    }

    int cast(AlphaDB *alphaDataBase, int nodeId, AlphaCache *cache) {
        //cout<<"start "<<nodeId<<" "<<nodeList_[nodeId].getName()<<endl;
        int dateSize = GET_HISTORY_SIZE(getMaxHistoryDays(), cache->getAlphaDays()) - getMaxFutureDays();
        //float* curResultCache = getNodeCacheMemory(nodeId, dateSize, cache->stockSize, cache->result);
        //bool* curResultFlag = cache->flagRes[nodeId].get();
        CacheFlag *curDayFlagCache = getNodeFlag(nodeId, dateSize, cache->dayFlag);

        int outMemoryId = 0;
        if (nodeList_[nodeId].getChildNum() == 0) {
            outMemoryId = cache->useMemory();
            //如果是变量，不需要初始化
            if(!nodeList_[nodeId].isEmpty()){
                if(cache->isSign()){
                    alphaDataBase->getStock(cache->dayBefore,
                                            getMaxHistoryDays(),
                                            getMaxFutureDays(),
                                            cache->sampleDays,
                                            cache->getAlphaDays(),
                                            cache->startSignIndex,
                                            cache->stockSize,
                                            nodeList_[nodeId].getName(),
                                            cache->signName,
                                            cache->getMemort<float>(outMemoryId));
                } else {
                    alphaDataBase->getStock(cache->dayBefore,
                                            getMaxHistoryDays(),
                                            getMaxFutureDays(),
                                            cache->sampleDays,
                                            cache->stockSize,
                                            nodeList_[nodeId].getName(),
                                            nodeList_[nodeId].getWatchLeafDataClass(),
                                            cache->getMemort<float>(outMemoryId),
                                            cache->codes);
                }
            }

        } else {
            int childMemoryIds[MAX_CHILD_NUM];
            void *childMemory[MAX_CHILD_NUM];
            for (int i = 0; i < nodeList_[nodeId].getChildNum(); ++i) {
                int childId = nodeList_[nodeId].childIds[i];
                if (!nodeList_[childId].isEmpty() && nodeList_[childId].isRoot()) {

                    //不能修改子树结果
                    childMemoryIds[i] = cache->useMemory();
                    int subTreeMemoryId = cache->nodeRes[childId].get();
                    //cout<<subTreeMemoryId<<" "<<childMemoryIds[i]<<"create new memory\n";
                    memcpy(cache->getMemort<char>(childMemoryIds[i]),
                           cache->getMemort<char>(subTreeMemoryId),
                           cache->stockSize * dateSize * sizeof(float));
                } else {
                    childMemoryIds[i] = cache->nodeRes[childId].get();
                }
                childMemory[i] = cache->getMemort<char>(childMemoryIds[i]);
            }
            //cout<<nodeList_[nodeId].getElement()->getParNum()<<" " << nodeList_[nodeId].getChildNum()<<endl;
            for (int i = 0; i < nodeList_[nodeId].getElement()->getParNum() - nodeList_[nodeId].getChildNum(); ++i) {
                int newMemId = cache->useMemory();
                childMemoryIds[i + nodeList_[nodeId].getChildNum()] = newMemId;
                childMemory[i + nodeList_[nodeId].getChildNum()] = cache->getMemort<char>(newMemId);
            }

            nodeList_[nodeId].getElement()->cast(childMemory, nodeList_[nodeId].getCoff(coffList_), dateSize,
                                                 cache->stockSize,
                                                 curDayFlagCache);

            //回收内存
            for (int i = 0; i < nodeList_[nodeId].getElement()->getParNum(); ++i) {
                if (i != nodeList_[nodeId].getElement()->getOutParIndex()) {
                    //仅仅自己输出的和作为变量的内存不用回收
                    if(!nodeList_[nodeId].isEmpty())
                        cache->releaseMemory(childMemoryIds[i]);
                } else {
                    outMemoryId = childMemoryIds[i];
                }
            }

        }
        for (auto i = 0; i < dateSize; ++i) {
            if (curDayFlagCache[i] == CacheFlag::NEED_CAL)
                curDayFlagCache[i] = CacheFlag::HAS_CAL;
        }
        //cout<<nodeId<<" "<<nodeList_[nodeId].getName()<<" out "<<outMemoryId<<endl;
        return outMemoryId;
    }

    float getMaxHistoryDays(int nodeId) {
        if (nodeList_[nodeId].getChildNum() == 0)
            return 0;
        float maxDays = 0;
        for (int i = 0; i < nodeList_[nodeId].getChildNum(); ++i) {
            maxDays = std::max(getMaxHistoryDays(nodeList_[nodeId].childIds[i]), maxDays);
        }
        //cout<<nodeList_[nodeId].getName()<<" "<<nodeList_[nodeId].getNeedBeforeDays(coffList_)<<" "<<maxDays<<endl;
        return fmax(roundf(nodeList_[nodeId].getNeedBeforeDays(coffList_)), 0.f) + maxDays;
    }

    //注意返回的是负数
    float getMaxFutureDays(int nodeId){
        if(nodeList_[nodeId].getChildNum() == 0)
            return 0;
        float maxDays = 0;
        for (int i = 0; i < nodeList_[nodeId].getChildNum(); ++i){
            maxDays = std::min(getMaxFutureDays(nodeList_[nodeId].childIds[i]), maxDays);
        }
        return fmin(roundf(nodeList_[nodeId].getNeedBeforeDays(coffList_)),0.f) + maxDays;
    }

    void flagAllNode(AlphaCache *cache) {
        int dateSize = GET_HISTORY_SIZE(getMaxHistoryDays(), cache->getAlphaDays()) - getMaxFutureDays();
        memset(cache->dayFlag, 0, dateSize * nodeList_.getSize() * sizeof(CacheFlag));
        for (size_t dayIndex = 0; dayIndex < cache->getAlphaDays(); ++dayIndex) {
            for (auto i = 0; i < subtreeList_.getSize(); ++i) {
                int curIndex = dayIndex + getMaxHistoryDays() - 1;
                flagNodeDay(subtreeList_[i].rootId, curIndex, dateSize, cache);
            }
        }
    }

    void flagNodeDay(int nodeId, int dayIndex, int dateSize, AlphaCache *cache) {
        CacheFlag *curFlag = getNodeFlag(nodeId, dateSize, cache->dayFlag);
        //bool* curStockFlag = getNodeCacheMemory(nodeId, dateSize, cache->stockSize, cache->resultFlag) + dayIndex * cache->stockSize;
        if (curFlag[dayIndex] == CacheFlag::NO_FLAG) {
            //填写数据
            curFlag[dayIndex] = CacheFlag::NEED_CAL;
            if (nodeList_[nodeId].getChildNum() > 0) {
                //for(size_t i = 0; i < cache->stockSize; ++i)
                //    curStockFlag[i] = true;
                //可以是负数
                int dayNum = (int) roundf(nodeList_[nodeId].getNeedBeforeDays(coffList_));
                switch (nodeList_[nodeId].getElement()->getDateRange()) {
                    case DateRange::CUR_DAY:

                        flagAllChild(nodeId, dayIndex, dateSize, cache);

                        //flagStock(getLeftFlag(nodeId, dayIndex, dateSize, cache), getRightFlag(nodeId, dayIndex, dateSize, cache), curStockFlag, cache->stockSize);
                        break;
                    case DateRange::BEFORE_DAY:
                        flagAllChild(nodeId, dayIndex - dayNum, dateSize, cache);
                        //flagStock(getLeftFlag(nodeId, dayIndex - dayNum, dateSize, cache), getRightFlag(nodeId, dayIndex - dayNum, dateSize, cache), curStockFlag, cache->stockSize);
                        break;
                    case DateRange::CUR_AND_BEFORE_DAY:
                        flagAllChild(nodeId, dayIndex, dateSize, cache);
                        flagAllChild(nodeId, dayIndex - dayNum, dateSize, cache);
                        //flagStock(getLeftFlag(nodeId, dayIndex, dateSize, cache), getRightFlag(nodeId, dayIndex, dateSize, cache), curStockFlag, cache->stockSize);
                        //flagStock(getLeftFlag(nodeId, dayIndex - dayNum, dateSize, cache), getRightFlag(nodeId, dayIndex - dayNum, dateSize, cache), curStockFlag, cache->stockSize);
                        break;
                    case DateRange::ALL_DAY:
                        if(dayNum >= 0){
                            for (auto i = 0; i <= dayNum; ++i) {
                                flagAllChild(nodeId, dayIndex - i, dateSize, cache);
                                //flagStock(getLeftFlag(nodeId, dayIndex - dayNum, dateSize, cache), getRightFlag(nodeId, dayIndex - i, dateSize, cache), curStockFlag, cache->stockSize);
                            }
                        } else {
                            for (int i = dayNum; i <= 0; ++i) {
                                flagAllChild(nodeId, dayIndex - i, dateSize, cache);
                            }
                        }

                        break;
                }

            }
        }

    }

    int maxHistoryDay_;
    int maxFutureDay_;

private:
    inline static bool *
    flag(DateRange dataRange, const bool *pleft, const bool *pright, size_t day, size_t historySize, size_t stockSize,
         CacheFlag *pflag, bool *pout) {

        if (day > 0) {
            memset(pout, 0, day * stockSize * sizeof(bool));
        }
        for (size_t j = 0; j < stockSize; ++j) {
            for (size_t i = day; i < historySize; ++i) {
                if (pflag[i] == CacheFlag::NEED_CAL) {
                    size_t watchIndex = 0;
                    size_t curIndex = i * stockSize + j;
                    switch (dataRange) {
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
                            if (pout[curIndex]) {
                                watchIndex = (i - day) * stockSize + j;
                                pout[curIndex] = (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]));
                            }
                            break;
                        case DateRange::ALL_DAY:
                            if (i > day && pout[curIndex - stockSize]) {
                                watchIndex = i * stockSize + j;
                                pout[curIndex] = (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]));
                            } else {
                                watchIndex = (i - day - 1) * stockSize + j;
                                if (i > day && (pleft[watchIndex] && (pright == nullptr || pright[watchIndex]))) {
                                    pout[curIndex] = false;
                                } else {
                                    for (size_t k = 0; k <= day; ++k) {
                                        watchIndex = (i - k) * stockSize + j;
                                        pout[curIndex] = (pleft[watchIndex] &&
                                                          (pright == nullptr || pright[watchIndex]));
                                        if (pout[curIndex] == false)
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

    inline void flagAllChild(int nodeId, int dayIndex, int dateSize, AlphaCache *cache) {
        for (int i = 0; i < nodeList_[nodeId].getChildNum(); ++i)
            flagNodeDay(nodeList_[nodeId].childIds[i], dayIndex, dateSize, cache);
    }

    static inline CacheFlag *getNodeFlag(int nodeId, int dateSize, CacheFlag *flagCache) {
        return flagCache + nodeId * dateSize;
    }

};


#endif //ALPHATREE_ALPHATREE_H
