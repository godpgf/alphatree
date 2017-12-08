//
// Created by yanyu on 2017/7/13.
// 观察者模式,负责所有技术面的特征提取,考核
//

#ifndef ALPHATREE_ALPHAFOREST_H
#define ALPHATREE_ALPHAFOREST_H

#include "alpha/alphatree.h"
#include "filter/filterforest.h"
#include "base/cache.h"
#include "base/dcache.h"
#include "base/threadpool.h"
#include "alpha/alphadb.h"
#include "base/hashmap.h"
#include <set>
#include <vector>
#include <iostream>
using namespace std;
using namespace fb;

class AlphaForest{
    public:
        static void initialize(int cacheSize){
            alphaForest_ = new AlphaForest(cacheSize);
        }

        static void release(){
            if(alphaForest_)
                delete alphaForest_;
            alphaForest_ = nullptr;
        }

        static AlphaForest* getAlphaforest(){ return alphaForest_;}


//        void addStock(const char* code, const char* market, const char* industry, const char* concept,
//                      const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
//                      int size, int totals
//        ){
//            alphaDataBase_.addStock(code, market, industry, concept, open, high, low, close, volume, vwap, returns, size, totals);
//        }

        AlphaDB* getAlphaDataBase(){ return &alphaDataBase_;}

        //从分配的alphatree中拿出一个
        int useAlphaTree(){
            return alphaTreeCache_->useCacheMemory();
        }



        void decode(int treeId, const char* name, const char* line, bool isLocal = false){
            getAlphaTree(treeId)->decode(name, line, alphaElementMap_, isLocal);
        }

        void decodeProcess(int treeId, const char* name, const char* line){
            getAlphaTree(treeId)->decodeProcess(name, line, alphaProcessMap_, alphaElementMap_);
        }

        //归还一个alphatree
        void releaseAlphaTree(int id){
            getAlphaTree(id)->clean();
            alphaTreeCache_->releaseCacheMemory(id);
        }



        int getSubAlphatreeSize(int id){
            return getAlphaTree(id)->getSubtreeSize();
        }

        const char* getSubAlphatreeName(int id, int subId){
            return getAlphaTree(id)->getSubtreeName(subId);
        }

        const char* encodeAlphaTree(int id, const char* rootName, char* pout){
            return getAlphaTree(id)->encode(rootName, pout);
        }

        const char* encodeProcess(int id, const char* processName, char* pout){
            return getAlphaTree(id)->encodeProcess(processName, pout);
        }

        int useCache(){
            return alphaCache_->useCacheMemory();
        }

        AlphaCache* getCache(int cacheId){
            return alphaCache_->getCacheMemory(cacheId);
        }

        void releaseCache(int cacheId){
            alphaCache_->releaseCacheMemory(cacheId);
        }

        const void flagAlpha(int alphaTreeId, int cacheId, size_t dayBefore, size_t sampleSize, const char* codes, size_t stockSize, bool* sampleFlag = nullptr, bool isFlagStock = false, bool isCalAllNode = false){
            getAlphaTree(alphaTreeId)->flagAlpha(&alphaDataBase_, dayBefore, sampleSize, codes, stockSize, alphaCache_->getCacheMemory(cacheId), &threadPool_, sampleFlag, isFlagStock, isCalAllNode);
        }

        const void calAlpha(int alphaTreeId, int cacheId){
            getAlphaTree(alphaTreeId)->calAlpha(&alphaDataBase_, alphaCache_->getCacheMemory(cacheId), &threadPool_);
        }

        const void cacheAlpha(int alphaTreeId, int cacheId, bool isFeature){
            getAlphaTree(alphaTreeId)->cacheAlpha(&alphaDataBase_, alphaCache_->getCacheMemory(cacheId), &threadPool_, isFeature);
        }

        const void processAlpha(int alphaTreeId, int cacheId){
            getAlphaTree(alphaTreeId)->processAlpha(&alphaDataBase_, alphaCache_->getCacheMemory(cacheId), &threadPool_);
        }

        const float* getAlpha(int alphaTreeId, const char* rootName, int cacheId){
            auto curCache = alphaCache_->getCacheMemory(cacheId);
            return getAlphaTree(alphaTreeId)->getAlpha(rootName, curCache);
        }

        const char* getProcess(int alphaTree, const char* processName, int cacheId){
            auto curCache = alphaCache_->getCacheMemory(cacheId);
            return getAlphaTree(alphaTree)->getProcess(processName, curCache);
        }

        const float* getAlpha(int alphaTreeId, int nodeId, int cacheId){
            auto curCache = alphaCache_->getCacheMemory(cacheId);
            return getAlphaTree(alphaTreeId)->getAlpha(nodeId, curCache);
        }

        int getMaxHistoryDays(int alphaTreeId){
            return (int)getAlphaTree(alphaTreeId)->getMaxHistoryDays();
        }


        int summarySubAlphaTree(const int* alphatreeIds, int len, int minDepth, char* subAlphatreeStr){
            set<const char*, ptrCmp> substrSet;
            char subAlphaTreeStrList[MAX_SUB_ALPHATREE_STR_NUM * MAX_NODE_STR_LEN];
            int curSubAlphaTreeIndex = 0;
            char cmpLine[MAX_NODE_STR_LEN];
            int allNum = 0;
            for(auto i = 0; i < len; ++i){
                for(int j = 0; j < getAlphaTree(alphatreeIds[i])->getSubtreeSize(); ++j){
                    getAlphaTree(alphatreeIds[i])->encode(getAlphaTree(alphatreeIds[i])->getSubtreeName(j), cmpLine);
                    for(int k = 0; k < len; ++k){
                        int minTime = 1;
                        for(int l = 0; l < getAlphaTree(alphatreeIds[k])->getSubtreeSize(); ++l){
                            if(i == k && j == l){
                                minTime = 2;
                            }
                            int num = getAlphaTree(alphatreeIds[k])->searchPublicSubstring(getAlphaTree(alphatreeIds[k])->getSubtreeName(l), cmpLine, subAlphaTreeStrList + curSubAlphaTreeIndex, minTime, minDepth);
                            while(num-- > 0){
                                curSubAlphaTreeIndex += strlen(subAlphaTreeStrList + curSubAlphaTreeIndex) + 1;
                                ++allNum;
                            }
                        }

                    }
                }

            }
            //去重
            curSubAlphaTreeIndex = 0;
            while (allNum-- > 0){
                //注意子树中不能包含indneutralize
                if(!strstr(subAlphaTreeStrList+curSubAlphaTreeIndex, "indneutralize")){
                    auto it = substrSet.find(subAlphaTreeStrList+curSubAlphaTreeIndex);
                    if(it == substrSet.end()){
                        substrSet.insert(subAlphaTreeStrList+curSubAlphaTreeIndex);
                    }
                }
                curSubAlphaTreeIndex += strlen(subAlphaTreeStrList + curSubAlphaTreeIndex) + 1;
            }
            //写回
            allNum = 0;
            char* curSubAlphatreeStr = subAlphatreeStr;
            for(auto it = substrSet.begin(); it != substrSet.end(); ++it){
                strcpy(curSubAlphatreeStr, *it);
                curSubAlphatreeStr += (strlen(*it)+1);
                ++allNum;
            }
            return allNum;
        }

        int learnFilterTree(int alphatree, int cacheId,  int minSampleSize = 30){
            AlphaCache* cache = getCache(cacheId);
            const float* buy = getAlphaTree(alphatree)->getAlpha("buy", cache);
            const float* sell = getAlphaTree(alphatree)->getAlpha("sell", cache);

            //计算最大取样数据量
            size_t dataSize = cache->sampleDays * cache->stockSize;
            size_t maxSampleSize = 0;
            for(size_t i = 0; i < dataSize; ++i)
                if(sell[i] > 0){
                    ++maxSampleSize;
                }

            int filterCacheId = filterForest_.useCache();
            FilterCache* filterCache = filterForest_.getCache(filterCacheId);
            filterCache->initialize(maxSampleSize, alphaDataBase_.getFeatureSize());
            filterCache->sampleSize = alphaDataBase_.getFeature(cache->dayBefore, cache->sampleDays, cache->stockSize, cache->codes, buy, sell, filterCache->feature, filterCache->returns, filterCache->risk, filterCache->weight);

            filterForest_.releaseCache(filterCacheId);
        }

    protected:
        AlphaForest(int cacheSize):threadPool_(cacheSize),filterForest_(cacheSize, &threadPool_) {
            initAlphaElement();

            //申请alphatree的内存
            alphaTreeCache_ = DCache<AlphaTree>::create();

            //申请中间结果的内存,有多少个cacheSize就可以同时计算几个alphatree
            alphaCache_ = Cache<AlphaCache>::create(cacheSize);
            char* p = alphaCache_->getBuff();
            for(auto i = 0; i != alphaCache_->getMaxCacheSize(); ++i){
                new (p)AlphaCache();
                p += sizeof(AlphaCache);
            }
        }

        ~AlphaForest(){
            //for(auto i = 0; i != alphaTreeCache_->getMaxCacheSize(); ++i)
            //    alphaTreeCache_->getCacheMemory(i)->~AlphaTree();
            DCache<AlphaTree>::release(alphaTreeCache_);

            cout<<"release alphatree"<<endl;
            for(auto i = 0; i != alphaCache_->getMaxCacheSize(); ++i)
                alphaCache_->getCacheMemory(i)->~AlphaCache();
            Cache<AlphaCache>::release(alphaCache_);

            cout<<"release alphacache"<<endl;
        }

        void initAlphaElement(){
            int alphaAtomNum = sizeof(AlphaAtom::alphaAtomList) / sizeof(AlphaAtom);
            for(auto i = 0; i < alphaAtomNum; i++){
                alphaElementMap_[AlphaAtom::alphaAtomList[i].getName()] = &AlphaAtom::alphaAtomList[i];
            }

            int alphaProcessNum = sizeof(AlphaProcess::alphaProcessList) / sizeof(AlphaProcess);
            for(auto i = 0; i < alphaProcessNum; ++i){
                alphaProcessMap_[AlphaProcess::alphaProcessList[i].getName()] = &AlphaProcess::alphaProcessList[i];
            }
        }
        AlphaTree* getAlphaTree(int id){
            return &alphaTreeCache_->getCacheMemory(id);
        }


        //alphatree的内存空间
        DCache<AlphaTree>* alphaTreeCache_ = {nullptr};

        //中间结果的内存空间
        Cache<AlphaCache>* alphaCache_ = {nullptr};

        //计算中间结果对应的线程
        ThreadPool threadPool_;

        //机器学习进一步过滤
        FilterForest filterForest_;

        AlphaDB alphaDataBase_;
        HashMap<IAlphaElement*> alphaElementMap_;
        HashMap<IAlphaProcess*> alphaProcessMap_;

        static AlphaForest* alphaForest_;
};

AlphaForest* AlphaForest::alphaForest_ = nullptr;
#endif //ALPHATREE_ALPHAFOREST_H
