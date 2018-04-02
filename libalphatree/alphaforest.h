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
#include <set>
#include <vector>
#include <iostream>
#include <string.h>

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

    float optimizeAlpha(int alphaTreeId, int cacheId, const char *rootName, size_t dayBefore, size_t sampleSize, const char *codes, size_t stockSize, float exploteRatio, int errTryTime){
        return getAlphaTree(alphaTreeId)->optimizeAlpha(rootName, &alphaDataBase_, dayBefore, sampleSize, codes, stockSize, alphaCache_->getCacheMemory(cacheId), &threadPool_, exploteRatio, errTryTime);
    }

    const float *getAlpha(int alphaTreeId, const char *rootName, int cacheId) {
        auto curCache = alphaCache_->getCacheMemory(cacheId);
        return getAlphaTree(alphaTreeId)->getAlpha(rootName, curCache);
    }

    const float *getAlphaSum(int alphaTreeId, const char *rootName, int cacheId){
        auto curCache = alphaCache_->getCacheMemory(cacheId);
        return getAlphaTree(alphaTreeId)->getAlphaSum(rootName, curCache);
    }

    void getAlphaSmooth(int alphaTreeId, const char *rootName, int cacheId, int smoothNum, float* smooth){
        auto curCache = alphaCache_->getCacheMemory(cacheId);
        return getAlphaTree(alphaTreeId)->getAlphaSmooth(rootName, curCache, smoothNum, smooth);
    }

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
        //申请alphatree的内存
        alphaTreeCache_ = DCache<AlphaTree>::create();
        //申请中间结果的内存,有多少个cacheSize就可以同时计算几个alphatree
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

    void initAlphaElement() {
        int alphaAtomNum = sizeof(AlphaAtom::alphaAtomList) / sizeof(AlphaAtom);
        for (auto i = 0; i < alphaAtomNum; i++) {
            alphaElementMap_[AlphaAtom::alphaAtomList[i].getName()] = &AlphaAtom::alphaAtomList[i];
        }
    }

    AlphaTree *getAlphaTree(int id) {
        return &alphaTreeCache_->getCacheMemory(id);
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
            af_(af), alphaTreeId_(alphaTreeId), daybefore_(daybefore), sampleSize_(sampleSize), startIndex_(startIndex), signNum_(signNum), cacheSize_(min(cacheSize, signNum)), curBlockIndex_(signNum){
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
            af_->calAlpha(alphaTreeId_, cacheId, daybefore_, sampleSize_, startIndex_ +  curBlockIndex_, min(cacheSize_, signNum_ - curBlockIndex_), 1, signName_);
            const float* alpha = af_->getAlpha(alphaTreeId_, rootName_, cacheId);
            memcpy(cache_, alpha, min(cacheSize_, signNum_ - curBlockIndex_) * sizeof(float));
            af_->releaseCache(cacheId);
        }
        return std::move(cache_[curIndex_ % cacheSize_]);
    }

    virtual void operator++(){if(curIndex_ < signNum_)++curIndex_;}
    //跳过指定位置，isRelative如果是false就是从起点开始计算，否则是从当前开始计算
    virtual void skip(long size, bool isRelative = true){
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
    virtual long size(){
        return signNum_;
    }

    int getAlphaTreeId(){
        return alphaTreeId_;
    }
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
