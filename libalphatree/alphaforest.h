//
// Created by yanyu on 2017/7/13.
// 观察者模式,负责所有技术面的特征提取,考核
//

#ifndef ALPHATREE_ALPHAFOREST_H
#define ALPHATREE_ALPHAFOREST_H

#include "alpha/alphatree.h"
#include "base/cache.h"
#include "base/dcache.h"
#include "base/threadpool.h"
#include "alpha/alphadb.h"
#include "base/hashmap.h"
#include "base/bag.h"
#include "base/vector.h"
#include <set>
#include <vector>
#include <iostream>
#include <string.h>
#include <math.h>
#include <algorithm>

using namespace std;

class AlphaForest {
public:
    static void initialize(int cacheSize) {
        alphaForest_ = new AlphaForest(cacheSize);
    }

    static void release() {
        if (alphaForest_)
            delete alphaForest_;
        alphaForest_ = nullptr;
    }

    static AlphaForest *getAlphaforest() { return alphaForest_; }

    AlphaDB *getAlphaDataBase() { return &alphaDataBase_; }

    //从分配的alphatree中拿出一个
    int useAlphaTree() {
        return alphaTreeCache_->useCacheMemory();
    }

    void decode(int treeId, const char *name, const char *line) {
        getAlphaTree(treeId)->decode(name, line, alphaElementMap_);
    }

    AlphaTree *getAlphaTree(int id) {
        return &alphaTreeCache_->getCacheMemory(id);
    }

    //归还一个alphatree
    void releaseAlphaTree(int id) {
        getAlphaTree(id)->clean();
        alphaTreeCache_->releaseCacheMemory(id);
    }


    int getSubAlphatreeSize(int id) {
        return getAlphaTree(id)->getSubtreeSize();
    }

    const char *getSubAlphatreeName(int id, int subId) {
        return getAlphaTree(id)->getSubtreeName(subId);
    }

    const char *encodeAlphaTree(int id, const char *rootName, char *pout) {
        return getAlphaTree(id)->encode(rootName, pout);
    }


    int useCache() {
        return alphaCache_->useCacheMemory();
    }

    AlphaCache *getCache(int cacheId) {
        return alphaCache_->getCacheMemory(cacheId);
    }

    void releaseCache(int cacheId) {
        alphaCache_->releaseCacheMemory(cacheId);
    }

    const void calAlpha(int alphaTreeId, int cacheId, size_t dayBefore, size_t sampleSize, const char *codes, size_t stockSize) {
        getAlphaTree(alphaTreeId)->calAlpha(&alphaDataBase_, dayBefore, sampleSize, codes, stockSize, alphaCache_->getCacheMemory(cacheId), &threadPool_);
    }

    const void calAlpha(int alphaTreeId, int cacheId, size_t dayBefore, size_t sampleSize, size_t startIndex, size_t signNum, size_t  signHistoryDays, const char* signName){
        getAlphaTree(alphaTreeId)->calAlpha(&alphaDataBase_, dayBefore, sampleSize, startIndex, signNum, signHistoryDays, signName, alphaCache_->getCacheMemory(cacheId), &threadPool_);
    }

    const void cacheAlpha(int alphaTreeId, int cacheId, const char* featureName) {
        getAlphaTree(alphaTreeId)->cacheAlpha<float>(&alphaDataBase_, alphaCache_->getCacheMemory(cacheId), &threadPool_, featureName);
    }

    const void cacheSign(int alphaTreeId, int cacheId, const char* featureName, const char* codes = nullptr, int codesNum = 0){
        getAlphaTree(alphaTreeId)->cacheSign(&alphaDataBase_, alphaCache_->getCacheMemory(cacheId), &threadPool_, featureName, codes, codesNum);
    }

    const void testCacheSign(int alphaTreeId, int cacheId, const char* featureName, const char* testFeatureName){
        getAlphaTree(alphaTreeId)->testCacheSign(&alphaDataBase_, alphaCache_->getCacheMemory(cacheId), &threadPool_, featureName, testFeatureName);
    }

//    float optimizeAlpha(int alphaTreeId, int cacheId, const char *rootName, size_t dayBefore, size_t sampleSize, const char *codes, size_t stockSize, float exploteRatio, int errTryTime){
//        return getAlphaTree(alphaTreeId)->optimizeAlpha(rootName, &alphaDataBase_, dayBefore, sampleSize, codes, stockSize, alphaCache_->getCacheMemory(cacheId), &threadPool_, exploteRatio, errTryTime);
//    }

    const float *getAlpha(int alphaTreeId, const char *rootName, int cacheId) {
        auto curCache = alphaCache_->getCacheMemory(cacheId);
        return getAlphaTree(alphaTreeId)->getAlpha(rootName, curCache);
    }

    void synchroAlpha(int alphaTreeId, int cacheId){
        auto curCache = alphaCache_->getCacheMemory(cacheId);
        getAlphaTree(alphaTreeId)->synchroAlpha(curCache);
    }

//    const float *getAlphaSum(int alphaTreeId, const char *rootName, int cacheId){
//        auto curCache = alphaCache_->getCacheMemory(cacheId);
//        return getAlphaTree(alphaTreeId)->getAlphaSum(rootName, curCache);
//    }
//
//    void getAlphaSmooth(int alphaTreeId, const char *rootName, int cacheId, int smoothNum, float* smooth){
//        auto curCache = alphaCache_->getCacheMemory(cacheId);
//        return getAlphaTree(alphaTreeId)->getAlphaSmooth(rootName, curCache, smoothNum, smooth);
//    }

//    const char* getProcess(int alphaTreeId, const char *rootName, int cacheId){
//        auto curCache = alphaCache_->getCacheMemory(cacheId);
//        return getAlphaTree(alphaTreeId)->getProcess(rootName, curCache);
//    }

    const float *getAlpha(int alphaTreeId, int nodeId, int cacheId) {
        auto curCache = alphaCache_->getCacheMemory(cacheId);
        return getAlphaTree(alphaTreeId)->getAlpha(nodeId, curCache);
    }

    int getMaxHistoryDays(int alphaTreeId) {
        return (int) getAlphaTree(alphaTreeId)->getMaxHistoryDays();
    }

    /*
    void getReturns(const char* codes, int stockSize, int dayBefore, int sampleSize,  const char* buySignList, int buySignNum, const char* sellSignList, int sellSignNum, float maxReturn, float maxDrawdown, int maxHoldDays, float* returns, const char* price = "close"){
        int maxNameLen = max(buySignNum, sellSignNum) / 10 + 3;
        int maxSignLen = (max(buySignNum, sellSignNum) / 10 + 3 + 6) * max(buySignNum, sellSignNum);
        char* name = new char[maxNameLen];
        char* sign = new char[maxSignLen];
        char* tmp = new char[maxSignLen];

        int buyId = useAlphaTree();
        int buyCacheId = useCache();
        for(int i = 0; i < buySignNum; ++i){
            sprintf(name,"s%d", i);
            if(i == 0)
                strcpy(sign, name);
            else{
                strcpy(tmp, sign);
                sprintf(sign, "(%s & %s)", tmp, name);
            }
            int len = strlen(buySignList);
            //cout<<"buy:"<<buySignList<<endl;
            decode(buyId, name, buySignList);
            buySignList = buySignList + (len + 1);
        }
        decode(buyId, "sign", sign);
        //cout<<sign<<endl;

        int sellId = useAlphaTree();
        int sellCacheId = useCache();
        if(sellSignNum > 0){
            for(int i = 0; i < sellSignNum; ++i){
                sprintf(name, "s%d", i);
                if(i == 0)
                    strcpy(sign, name);
                else{
                    strcpy(tmp, sign);
                    sprintf(sign, "(%s | %s)", tmp, name);
                }
                int len = strlen(sellSignList);
                //cout<<"sell:"<<sellSignList<<endl;
                decode(sellId, name, sellSignList);
                sellSignList = sellSignList + (len + 1);
            }
            decode(sellId, "sign", sign);
            //cout<<sign<<endl;
        }


        int priceId = useAlphaTree();
        int priceCacheId = useCache();
        decode(priceId, "price", price);

        //back trader--------------------------------------------------
        auto maxPrice = Vector<float>(stockSize);
        auto buyPrice = Vector<float>(stockSize);
        auto buyDay = Vector<int>(stockSize);
        buyDay.initialize(-1);
        calAlpha(buyId, buyCacheId, dayBefore, sampleSize, codes, stockSize);
        if(sellSignNum > 0)
            calAlpha(sellId, sellCacheId, dayBefore, sampleSize, codes, stockSize);
        calAlpha(priceId, priceCacheId, dayBefore, sampleSize, codes, stockSize);
        const float* buyAlpha = getAlpha(buyId, "sign", buyCacheId);
        const float* sellAlpha = sellSignNum <= 0 ? nullptr : getAlpha(sellId, "sign", sellCacheId);
        const float* priceAlpha = getAlpha(priceId, "price", priceCacheId);
        returns[0] = 1;
        for(int i = 1; i < sampleSize; ++i){
            float r = 0;
            int cnt = 0;
            for(int j = 0; j < stockSize; ++j){
                int curIndex = i * stockSize + j;
                if(buyDay[j] < 0){
                    if(buyAlpha[curIndex] > 0.5f &&
                       buyAlpha[curIndex - stockSize] < 0.5f &&
                       priceAlpha[curIndex] / priceAlpha[curIndex - stockSize] < 1.09f){
                        maxPrice[j] = priceAlpha[curIndex];
                        buyPrice[j] = priceAlpha[curIndex];
                        buyDay[j] = i;
                    }
                } else {
                    r += priceAlpha[curIndex] / priceAlpha[curIndex - stockSize];
                    ++cnt;
                    //sell
                    if(priceAlpha[curIndex] / buyPrice[j] > 1 + maxReturn){
                        buyDay[j] = -1;
                    } else if(priceAlpha[curIndex] / maxPrice[j] < 1 - maxDrawdown){
                        buyDay[j] = -1;
                    } else if(i - buyDay[j] >= maxHoldDays){
                        buyDay[j] = -1;
                    } else if(sellAlpha != nullptr && sellAlpha[curIndex] > 0.5f){
                        buyDay[j] = -1;
                    }
                    maxPrice[j] = max(maxPrice[j], priceAlpha[curIndex]);
                }
            }
            r = (cnt > 0 ? r / cnt : 1);
            returns[i] = returns[i-1] * r;
        }

        //release------------------------------------------------------
        releaseAlphaTree(buyId);
        releaseCache(buyCacheId);
        releaseAlphaTree(sellId);
        releaseCache(sellCacheId);
        releaseAlphaTree(priceId);
        releaseCache(priceCacheId);
        delete []name;
        delete []sign;
        delete []tmp;
    }*/


//    int summarySubAlphaTree(const int *alphatreeIds, int len, int minDepth, char *subAlphatreeStr) {
//        set<const char *, ptrCmp> substrSet;
//        char subAlphaTreeStrList[MAX_SUB_ALPHATREE_STR_NUM * MAX_NODE_STR_LEN];
//        int curSubAlphaTreeIndex = 0;
//        char cmpLine[MAX_NODE_STR_LEN];
//        int allNum = 0;
//        for (auto i = 0; i < len; ++i) {
//            for (int j = 0; j < getAlphaTree(alphatreeIds[i])->getSubtreeSize(); ++j) {
//                getAlphaTree(alphatreeIds[i])->encode(getAlphaTree(alphatreeIds[i])->getSubtreeName(j), cmpLine);
//                for (int k = 0; k < len; ++k) {
//                    int minTime = 1;
//                    for (int l = 0; l < getAlphaTree(alphatreeIds[k])->getSubtreeSize(); ++l) {
//                        if (i == k && j == l) {
//                            minTime = 2;
//                        }
//                        int num = getAlphaTree(alphatreeIds[k])->searchPublicSubstring(
//                                getAlphaTree(alphatreeIds[k])->getSubtreeName(l), cmpLine,
//                                subAlphaTreeStrList + curSubAlphaTreeIndex, minTime, minDepth);
//                        while (num-- > 0) {
//                            curSubAlphaTreeIndex += strlen(subAlphaTreeStrList + curSubAlphaTreeIndex) + 1;
//                            ++allNum;
//                        }
//                    }
//
//                }
//            }
//
//        }
//        //去重
//        curSubAlphaTreeIndex = 0;
//        while (allNum-- > 0) {
//            //注意子树中不能包含indneutralize
//            if (!strstr(subAlphaTreeStrList + curSubAlphaTreeIndex, "indneutralize")) {
//                auto it = substrSet.find(subAlphaTreeStrList + curSubAlphaTreeIndex);
//                if (it == substrSet.end()) {
//                    substrSet.insert(subAlphaTreeStrList + curSubAlphaTreeIndex);
//                }
//            }
//            curSubAlphaTreeIndex += strlen(subAlphaTreeStrList + curSubAlphaTreeIndex) + 1;
//        }
//        //写回
//        allNum = 0;
//        char *curSubAlphatreeStr = subAlphatreeStr;
//        for (auto it = substrSet.begin(); it != substrSet.end(); ++it) {
//            strcpy(curSubAlphatreeStr, *it);
//            curSubAlphatreeStr += (strlen(*it) + 1);
//            ++allNum;
//        }
//        return allNum;
//    }
//
//    //学习用来过滤信号的随机森林
//    int learnFilterForest(int alphatree, int cacheId, const char *features, size_t featureSize, size_t treeSize = 20,
//                          size_t iteratorNum = 2, float gamma = 0.001f, float lambda = 1.0f, size_t maxDepth = 16,
//                          size_t maxLeafSize = 1024, size_t maxAdjWeightTime = 4, float adjWeightRule = 0.2f,
//                          size_t maxBarSize = 16, float mergeBarPercent = 0.016f, float subsample = 0.6f,
//                          float colsampleBytree = 0.75f, const char *buySign = "buy", const char *sellSign = "sell",
//                          const char *targetValue = "target") {
//
//
//
//    }

protected:
    AlphaForest(int cacheSize) : threadPool_(cacheSize) {
        initAlphaElement();
        //申请alphatree的内存。注意：一个alphatree其实就是一个股票的公式集
        alphaTreeCache_ = DCache<AlphaTree>::create();
        //申请中间结果的内存,有多少个cacheSize就可以同时计算几个alphatree（公式集）
        alphaCache_ = Cache<AlphaCache>::create(cacheSize);

        char *p = alphaCache_->getBuff();
        for (auto i = 0; i != alphaCache_->getMaxCacheSize(); ++i) {
            new(p)AlphaCache();
            p += sizeof(AlphaCache);
        }
    }

    ~AlphaForest() {
        //for(auto i = 0; i != alphaTreeCache_->getMaxCacheSize(); ++i)
        //    alphaTreeCache_->getCacheMemory(i)->~AlphaTree();
        DCache<AlphaTree>::release(alphaTreeCache_);

//        cout << "release alphatree" << endl;
        for (auto i = 0; i != alphaCache_->getMaxCacheSize(); ++i)
            alphaCache_->getCacheMemory(i)->~AlphaCache();
        Cache<AlphaCache>::release(alphaCache_);
//        cout << "release alphacache" << endl;

    }

    /*
     * 将公式名和公式的函数指针做映射
     * */
    void initAlphaElement() {
        int alphaAtomNum = sizeof(AlphaAtom::alphaAtomList) / sizeof(AlphaAtom);
        for (auto i = 0; i < alphaAtomNum; i++) {
            alphaElementMap_[AlphaAtom::alphaAtomList[i].getName()] = &AlphaAtom::alphaAtomList[i];
        }
    }

    //alphatree的内存空间
    DCache<AlphaTree> *alphaTreeCache_ = {nullptr};

    //中间结果的内存空间
    Cache<AlphaCache> *alphaCache_ = {nullptr};

    //计算中间结果对应的线程
    ThreadPool threadPool_;

    AlphaDB alphaDataBase_;
    HashMap<IAlphaElement *> alphaElementMap_;

    static AlphaForest *alphaForest_;
};

AlphaForest *AlphaForest::alphaForest_ = nullptr;

class AlphaSignIterator : public IBaseIterator<float>{
public:
    AlphaSignIterator(AlphaForest* af, const char* rootName, const char* signName, int alphaTreeId, size_t daybefore, size_t sampleSize, size_t startIndex, size_t signNum, size_t cacheSize = 4096):
            af_(af), alphaTreeId_(alphaTreeId), daybefore_(daybefore), sampleSize_(sampleSize), startIndex_(startIndex), signNum_(signNum), cacheSize_(std::min(cacheSize, signNum)), curBlockIndex_(signNum){
        cache_ = new float[cacheSize];
        rootName_ = new char[strlen(rootName) + 1];
        signName_ = new char[strlen(signName) + 1];
        strcpy(signName_, signName);
        strcpy(rootName_, rootName);
    }

    virtual ~AlphaSignIterator(){
        delete []cache_;
        delete []signName_;
        delete []rootName_;

    }

    virtual IBaseIterator<float>* clone(){
        return new AlphaSignIterator(af_, rootName_, signName_, alphaTreeId_, daybefore_, sampleSize_, startIndex_, signNum_, cacheSize_);
    }

    virtual float&& getValue(){
        if(curIndex_ < curBlockIndex_ || curIndex_ >= curBlockIndex_ + cacheSize_){
            int cacheId = af_->useCache();
            curBlockIndex_ = (curIndex_ / cacheSize_) * cacheSize_;
            af_->calAlpha(alphaTreeId_, cacheId, daybefore_, sampleSize_, startIndex_ +  curBlockIndex_, std::min(cacheSize_, signNum_ - curBlockIndex_), 1, signName_);
            const float* alpha = af_->getAlpha(alphaTreeId_, rootName_, cacheId);
            af_->synchroAlpha(alphaTreeId_, cacheId);
            memcpy(cache_, alpha, std::min(cacheSize_, signNum_ - curBlockIndex_) * sizeof(float));
            af_->releaseCache(cacheId);
        }
        return std::move(cache_[curIndex_ % cacheSize_]);
    }

    virtual void operator++(){if(curIndex_ < signNum_)++curIndex_;}
    //跳过指定位置，isRelative如果是false就是从起点开始计算，否则是从当前开始计算
    virtual void skip(int64_t size, bool isRelative = true){
        if(isRelative){
            curIndex_ += size;
        } else {
            curIndex_ = size;
        }
        if(curIndex_ > signNum_)
            curIndex_ = signNum_;
    }
    virtual bool isValid(){
        return curIndex_ < signNum_;
    }
    virtual int64_t size(){
        return signNum_;
    }

    virtual float getAvg(){
        const double maxSubCnt = 1024;
        double avg = 0;
        double cnt = size();
        double subAvg = 0;
        double subCnt = 0;
        while (isValid()){
            subAvg += getValue();
            subCnt += 1;
            if(subCnt > maxSubCnt){
                avg += (subAvg / subCnt) * (subCnt / cnt);
                //cout<<subAvg / subCnt<<endl;
                subAvg = 0;
                subCnt = 0;
            }
            skip(1);
        }
        if(subCnt > 0){
            avg += (subAvg / subCnt) * (subCnt / cnt);
        }
        skip(0, false);
        return avg;
    }

    virtual void getAvgAndStd(float& avg, float& std){
        const double maxSubCnt = 1024;
        const double cnt = size();
        avg = 0;
        std = 0;
        double subX = 0;
        double subXSqr = 0;
        double subCnt = 0;
        while (isValid()){
            subX += getValue();
            subCnt += 1;
            subXSqr += getValue() * getValue();
            if(subCnt > maxSubCnt){
                avg += (subX / subCnt) * (subCnt / cnt);
                std += (subXSqr / subCnt) * (subCnt / cnt);
                subX = 0;
                subXSqr = 0;
            }
            skip(1);
        }
        if(subCnt > 0){
            avg += (subX / subCnt) * (subCnt / cnt);
            std += (subXSqr / subCnt) * (subCnt / cnt);
        }
        skip(0, false);
        std = sqrtf(std - avg * avg);
    }

    int getAlphaTreeId(){
        return alphaTreeId_;
    }

    static float getCorrelation(IBaseIterator<float>* a, IBaseIterator<float>* b){
        //计算当前股票的均值和方差
        double meanLeft = 0;
        double meanRight = 0;
        double sumSqrLeft = 0;
        double sumSqrRight = 0;
        for(int j = 0; j < a->size(); ++j){
            meanLeft += a->getValue();
            sumSqrLeft += a->getValue() * a->getValue();
            meanRight += b->getValue();
            sumSqrRight += b->getValue() * b->getValue();
            a->skip(1);
            b->skip(1);
        }
        a->skip(0, false);
        b->skip(0, false);
        meanLeft /= a->size();
        meanRight /= a->size();

        float cov = 0;
        for(int k = 0; k < a->size(); ++k){
            cov += (a->getValue() - meanLeft) * (b->getValue() - meanRight);
            a->skip(1);
            b->skip(1);
        }
        a->skip(0, false);
        b->skip(0, false);
        float xDiff2 = (sumSqrLeft - meanLeft*meanLeft*a->size());
        float yDiff2 = (sumSqrRight - meanRight*meanRight*a->size());

        if(isnormal(cov) && isnormal(xDiff2) && isnormal(yDiff2))
        {
            float corr = cov / sqrtf(xDiff2) / sqrtf(yDiff2);
            if(isnormal(corr)){
                return fmaxf(fminf(corr, 1.0f), -1.0f);
            }

            return 1;
        }
        return 1;
    }

    static float getDistinguish(AlphaForest* af, const char* signName, int alphatreeId, int targetId, int daybefore, int sampleSize, int sampleTime, int allowFailTime){
        Vector<double> avg(sampleTime);
        double curSum = 0;
        int curCnt = 0;

        float avg_l = 0;
        float cnt_l = 0;
        float avg_r = 0;
        float cnt_r = 0;
        float minTarget = FLT_MAX;
        int sampleDays = sampleSize * sampleTime;
        int curTimeIndex = 0;

        AlphaSignIterator feature(af, "t", signName, alphatreeId, daybefore, sampleDays, 0, af->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName));
        AlphaSignIterator target(af, "t", signName, targetId, daybefore, sampleDays, 0, af->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName));

//        cout<<sampleDays<<"="<<feature.size()<<endl;

        for(int i = 0;i < feature.size(); ++i){
            curSum += *feature;
            ++curCnt;
            feature.skip(1);
            if((int)(i * sampleTime / (feature.size() - 1.0f)) > curTimeIndex){
                avg[curTimeIndex] = curSum / curCnt;
                ++curTimeIndex;
                curSum = 0;
                curCnt = 0;
//                cout<<"avg:"<<avg[curTimeIndex-1]<<endl;
            }
        }
        feature.skip(0, false);

        bool isProportion = false;
        Vector<float> minDist(allowFailTime + 1);
        int curFailTime = 0;
        float distinguish = -1;

        curTimeIndex = 0;
        for(int i = 0; i < feature.size(); ++i){
            if(feature.getValue() > avg[curTimeIndex]){
                avg_r += target.getValue();
                cnt_r += 1;
            } else {
                avg_l += target.getValue();
                cnt_l += 1;
            }
//            if(curTimeIndex == 0){
//                cout<<feature.getValue()<<" "<<avg[curTimeIndex]<<" "<<target.getValue()<<endl;
//            }
            minTarget = min(minTarget, target.getValue());

            feature.skip(1);
            target.skip(1);

            if((int)(i * sampleTime / (feature.size() - 1.0f)) > curTimeIndex){
//                cout<<"i:"<<i<<" "<<i * sampleTime / (feature.size() - 1.0f)<<">"<<curTimeIndex<<endl;
                if(avg_r == 0 || avg_l == 0)
                    return 0;
                if(min(cnt_l, cnt_r) / max(cnt_l, cnt_r) < 0.32f)
                    return 0;
                avg_l = avg_l / cnt_l - minTarget;
                avg_r = avg_r / cnt_r - minTarget;
                float dist = avg_r / avg_l;
                if(dist == 0)
                    return 0;

                if(distinguish < 0){
                    isProportion = (dist > 1);
                    distinguish = isProportion ? dist : 1 / dist;
                    minDist[curFailTime] = distinguish;
                }else  {
                    distinguish = isProportion ? dist : 1 / dist;
                    if(curFailTime < allowFailTime){
                        //insert
                        bool isInsert = false;
                        for(int j = curFailTime; j >= 0; --j){
                            if(distinguish >= minDist[j]){
                                minDist[j+1] = distinguish;
                                isInsert = true;
                                break;
                            } else{
                                minDist[j+1] = minDist[j];
                            }
                        }
                        if(isInsert == false){
                            minDist[0] = distinguish;
                        }
                        ++curFailTime;
                    } else{
                        bool isReplace = false;
                        for(int j = allowFailTime; j >= 0; --j){
                            if(isReplace == false){
                                if(distinguish >= minDist[j])
                                    break;
                                else{
                                    minDist[j] = distinguish;
                                    isReplace = true;
                                }
                            }
                            else if(j > 0 && minDist[j] < minDist[j - 1]){
                                distinguish = minDist[j];
                                minDist[j] = minDist[j-1];
                                minDist[j-1] = distinguish;
                            } else {
                                break;
                            }
                        }
                    }
//                    distinguish = min(distinguish, isProportion ? dist : 1 / dist);
//                    if(distinguish < 1.f){
//                        break;
//                    }
                }

//                if(daybefore > 100)
//                    cout<<"dis"<<distinguish<<endl;
                if(curFailTime == allowFailTime && minDist[curFailTime] < 1.0)
                    return minDist[curFailTime];
                avg_l = 0;
                cnt_l = 0;
                avg_r = 0;
                cnt_r = 0;
                minTarget = FLT_MAX;
                ++curTimeIndex;
            }
        }
        return distinguish;
    }


    static void getConfidence(IBaseIterator<float>* feature, float slpitValue, IBaseIterator<float>* target, bool isMore, float& support, float& confidence){
        int pred_cnt = 0;
        int right_cnt = 0;
        while (feature->isValid()){
            if((feature->getValue() > slpitValue && isMore) || (feature->getValue() < slpitValue && !isMore)){
                ++pred_cnt;
                if(target->getValue() > 0.5f){
                    ++right_cnt;
                }
            }
            feature->skip(1);
            target->skip(1);
        }
        feature->skip(0, false);
        target->skip(0, false);
        support = ((float)pred_cnt) / feature->size();
        confidence = ((float)right_cnt) / pred_cnt;
    }


    /*
    static float getConfidence(AlphaForest* af, const char* signName, int alphatreeId, int targetId, int daybefore, int sampleSize, int sampleTime, float support, float stdScale = 2.0){
        AlphaSignIterator allFeature(af, "t", signName, alphatreeId, daybefore, sampleSize * sampleTime, 0, af->getAlphaDataBase()->getSignNum(daybefore, sampleSize *sampleTime, signName));
        float avg, std;
        allFeature.getAvgAndStd(avg, std);
        AlphaSignIterator allTarget(af, "t", signName, targetId, daybefore, sampleSize * sampleTime, 0, af->getAlphaDataBase()->getSignNum(daybefore, sampleSize *sampleTime, signName));

        float rSupport, rConfidence, lSupport, lConfidence;
        getConfidence(&allFeature, avg + stdScale * std, &allTarget, true, rSupport, rConfidence);
        if(rSupport <= support)
            return 0;
        getConfidence(&allFeature, avg - stdScale * std, &allTarget, false, lSupport, lConfidence);
        if(lSupport <= support)
            return 0;

        bool isProportion = rSupport > lSupport;

        float confidence = isProportion ? rConfidence : lConfidence;

        for(int i = 0; i < sampleTime; ++i){
            AlphaSignIterator feature(af, "t", signName, alphatreeId, daybefore + i * sampleSize, sampleSize, 0, af->getAlphaDataBase()->getSignNum(daybefore + i * sampleSize, sampleSize, signName));
            AlphaSignIterator target(af, "t", signName, targetId, daybefore + i * sampleSize, sampleSize, 0, af->getAlphaDataBase()->getSignNum(daybefore + i * sampleSize, sampleSize, signName));

            if(isProportion){
                getConfidence(&feature, avg + stdScale * std, &target, true, rSupport, rConfidence);
                if(rSupport <= support)
                    return 0;
                confidence = min(confidence, rConfidence);
            } else {
                getConfidence(&feature, avg - stdScale * std, &target, false, lSupport, lConfidence);
                if(lSupport <= support)
                    return 0;
                confidence = min(confidence, lConfidence);
            }
        }
        return confidence;
    }*/
protected:
    AlphaForest* af_;
    char* rootName_;
    char* signName_;
    int alphaTreeId_;
    size_t daybefore_;
    size_t sampleSize_;
    size_t startIndex_;
    size_t signNum_;
    size_t cacheSize_;
    //当前数据块的开始位子
    size_t curBlockIndex_;
    size_t curIndex_ = {0};
    float* cache_ = {nullptr};
};
#endif //ALPHATREE_ALPHAFOREST_H
