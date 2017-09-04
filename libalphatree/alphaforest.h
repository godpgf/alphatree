//
// Created by yanyu on 2017/7/13.
// 观察者模式,负责所有技术面的特征提取,考核
//

#ifndef ALPHATREE_ALPHAFOREST_H
#define ALPHATREE_ALPHAFOREST_H

#include "alpha/alphatree.h"
#include "base/cache.h"
#include "base/threadpool.h"
#include "alpha/alphadb.h"
#include <map>
#include <set>
#include <vector>
#include <iostream>
using namespace std;

class AlphaForest{
    public:
        static void initialize(size_t maxHistorySize, size_t maxStoctSize, size_t maxSampleSize, size_t maxFutureSize, size_t maxCacheSize, size_t maxAlphaTreeSize, size_t maxNodeSize, size_t maxSubTreeNum, size_t maxAchievementSize, int sampleBetaSize);

        static void release();

        static AlphaForest* getAlphaforest();

        void initAlphaElement(){
            int alphaAtomNum = sizeof(AlphaAtom::alphaAtomList) / sizeof(AlphaAtom);
            for(int i = 0; i < alphaAtomNum; i++){
                alphaElementMap_[AlphaAtom::alphaAtomList[i].getName()] = &AlphaAtom::alphaAtomList[i];
            }
            int alphaParNum = sizeof(AlphaPar::alphaParList) / sizeof(AlphaPar);
            for(int i = 0; i < alphaParNum; i++){
                alphaElementMap_[AlphaPar::alphaParList[i].getName()] = &AlphaPar::alphaParList[i];
            }
        }

        void addStock(const char* code, const char* market, const char* industry, const char* concept,
                      const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
                      int size, int totals
        ){
            alphaDataBase_->addStock(code, market, industry, concept, open, high, low, close, volume, vwap, returns, size, totals);
        }

        AlphaDataBase* getAlphaDataBase(){ return alphaDataBase_;}

        void calClassifiedData(){
            alphaDataBase_->calClassifiedData(true);
            alphaDataBase_->calClassifiedData(false);
        }

        //从分配的alphatree中拿出一个
        int useAlphaTree(const char* line = nullptr){
            int treeId = alphaTreeCache_->useCacheMemory();
            if(line != nullptr)
                getAlphaTree(treeId)->decode(line, alphaElementMap_);
            return treeId;
        }

        void decode(int treeId, const char* line, const char* name = nullptr){
            if(name == nullptr)
                getAlphaTree(treeId)->decode(line, alphaElementMap_);
            else
                getAlphaTree(treeId)->decode(name, line, alphaElementMap_);
        }

        //归还一个alphatree
        void releaseAlphaTree(int id){
            getAlphaTree(id)->clean();
            alphaTreeCache_->releaseCacheMemory(id);
        }

        const char* encodeAlphaTree(int id, char* pout){
            return getAlphaTree(id)->encode(pout);
        }


        int addLeftChild(int alphaTreeId, int parentId,const char* opt, double coff){
            int childId = getAlphaTree(alphaTreeId)->createNode(alphaElementMap_[opt], coff);
            getAlphaTree(alphaTreeId)->addLeftChild(parentId, childId);
            return childId;
        }

        int addRightChild(int alphaTreeId, int parentId,const char* opt, double coff){
            int childId = getAlphaTree(alphaTreeId)->createNode(alphaElementMap_[opt], coff);
            getAlphaTree(alphaTreeId)->addRightChild(parentId, childId);
            return childId;
        }

        int getLeftChild(int alphaTreeId, int parentId){ return getAlphaTree(alphaTreeId)->getLeftChild(parentId);}
        int getRightChild(int alphaTreeId, int parentId){ return getAlphaTree(alphaTreeId)->getRightChild(parentId);}

        int getRoot(int alphaTreeId){ return getAlphaTree(alphaTreeId)->getNodeNum()-1;}
        int getNodeDes(int alphaTreeId, int nodeId, char* name, char* coff, int* childList){
            strcpy(name, getAlphaTree(alphaTreeId)->getName(nodeId));
            auto node = getAlphaTree(alphaTreeId)->getNode(nodeId);

            //如果是子树
            if(getAlphaTree(alphaTreeId)->isSubtreeRoot(nodeId)){
                coff[0] = 0;
                return 0;
            }

            node->getCoffStr(coff);
            int childNum = node->getChildNum();
            if(childNum > 0)
                childList[0] = node->leftId;
            if(childNum > 1)
                childList[1] = node->rightId;
            return childNum;
        }
        //const char* getNodeName(int alphaTreeId, int nodeId){return getAlphaTree(alphaTreeId)->getNode(nodeId)->getName();}
        //float getNodeCoff(int alphaTreeId, int nodeId){ return getAlphaTree(alphaTreeId)->getNode(nodeId)->getCoff();}

        size_t getCodes(size_t dayBefore, size_t watchFutureNum, size_t historyNum, size_t sampleNum, char* codes){
            return alphaDataBase_->getCodes(dayBefore,watchFutureNum,historyNum,sampleNum,codes);
        }

        Cache<float>* getAlphaTreeMemoryCache(){ return alphaTreeMemoryCache_;}
        Cache<bool>* getFlagCache(){ return flagCache_;}
        Cache<char>* getCodesCache(){ return codesCache_;}

        ThreadPool* getThreadPool(){ return &threadPool_;}

//        float* getLastMemoryCache(int alphaTreeId, int cacheMemoryId, int futureId, int sampleSize, int stockSize){
//            return getAlphaTree(alphaTreeId)->getLastCacheMemory(sampleSize, stockSize, futureId, alphaTreeFutureMemoryCache_->getCacheMemory(cacheMemoryId));
//        }

        const float* calAlpha(int alphaTreeId, int cacheMemoryId, int flagCacheId, size_t dayBefore, size_t sampleSize, const char* codes, size_t stockSize){
            return calAlpha(alphaTreeId, flagCache_->getCacheMemory(flagCacheId), alphaTreeMemoryCache_->getCacheMemory(cacheMemoryId), codes, dayBefore, sampleSize, stockSize).get();
        }

        std::shared_future<const float*> calAlpha(int alphaTreeId, bool *flagCache, float *cacheMemory, const char *codes, size_t dayBefore, size_t sampleSize, size_t stockSize){
            return getAlphaTree(alphaTreeId)->calAlpha(alphaDataBase_, dayBefore, sampleSize,
                                                       flagCache, cacheMemory, codes,
                                                       stockSize, &threadPool_);
        }

        const float* getAlpha(int alphaTreeId, int nodeId, int cacheMemoryId, size_t sampleSize, size_t stockSize){
            return getAlphaTree(alphaTreeId)->getAlpha(nodeId, sampleSize,
                                                       alphaTreeMemoryCache_->getCacheMemory(cacheMemoryId), stockSize);
        }

        int getHistoryDays(int alphaTreeId){
            return (int)getAlphaTree(alphaTreeId)->getHistoryDays();
        }

        int getNodeNum(int alphaTreeId){
            return getAlphaTree(alphaTreeId)->getNodeNum();
        }

        int getFutureDays(int alphaTreeId){
            return (int)getAlphaTree(alphaTreeId)->getFutureDays();
        }

//        void examAlphatrees(int* alphaTreeIds, size_t batchSize, IExam* exam){
//
//            //刷新之前记录的achievement
//            auto iter = exam->getRecordList()->begin();
//            while(iter != exam->getRecordList()->end()){
//                if(exam->getLastScore(alphaAchievement_->getCacheMemory(*iter)) <= 0){
//                    alphaAchievement_->releaseCacheMemory(*iter);
//                    exam->getRecordList()->erase(iter++);
//                } else{
//                    ++iter;
//                }
//            }
//
//            vector<std::future<int>> v;
//            for(size_t i = 0; i < batchSize; i++){
//                AlphaTree* alphaTree = getAlphaTree(alphaTreeIds[i]);
//
//                auto alphaTreeMemoryCache = alphaTreeMemoryCache_;
//                auto alphaTreeFutureMemoryCache = alphaTreeFutureMemoryCache_;
//                auto codesCache = codesCache_;
//                auto alphaAchievement = alphaAchievement_;
//
//                v.push_back(threadPool_.enqueue([alphaTree, exam, alphaAchievement, alphaTreeMemoryCache, alphaTreeFutureMemoryCache, codesCache]{
//                    int cacheMemoryId = alphaTreeMemoryCache->useCacheMemory();
//                    int cacheFutureMemoryId = alphaTreeFutureMemoryCache->useCacheMemory();
//                    int cacheCodesId = codesCache->useCacheMemory();
//                    int achievementId = alphaAchievement->useCacheMemory();
//                    exam->exam(alphaTree, alphaTreeMemoryCache->getCacheMemory(cacheMemoryId), alphaTreeFutureMemoryCache->getCacheMemory(cacheFutureMemoryId), codesCache->getCacheMemory(cacheCodesId), alphaAchievement->getCacheMemory(achievementId));
//                    alphaTreeMemoryCache->releaseCacheMemory(cacheMemoryId);
//                    alphaTreeFutureMemoryCache->releaseCacheMemory(cacheFutureMemoryId);
//                    codesCache->releaseCacheMemory(cacheCodesId);
//                    return achievementId;
//                }));
//            }
//
//            for(auto& res : v){
//                int achievementId = res.get();
//                //用最新的数据去更新考试阈值
//                exam->adjust(alphaAchievement_->getCacheMemory(achievementId));
//                //记录通过考试的成就
//                if(exam->getLastScore(alphaAchievement_->getCacheMemory(achievementId)) > 0)
//                    exam->record(achievementId);
//                else
//                    alphaAchievement_->releaseCacheMemory(achievementId);
//            }
//        }


        int summarySubAlphaTree(const int* alphatreeIds, int len, int minDepth, char* subAlphatreeStr){
            set<const char*, ptrCmp> substrSet;
            char subAlphaTreeStrList[MAX_SUB_ALPHATREE_STR_NUM * MAX_ALPHATREE_STR_LEN];
            int curSubAlphaTreeIndex = 0;
            char cmpLine[MAX_ALPHATREE_STR_LEN];
            int allNum = 0;
            for(int i = 0; i < len; ++i){
                getAlphaTree(alphatreeIds[i])->encode(cmpLine);
                for(int j = 0; j < len; ++j){
                    int minTime = 1;
                    if(i == j){
                        minTime = 2;
                    }
                    int num = getAlphaTree(alphatreeIds[j])->searchPublicSubstring(cmpLine,subAlphaTreeStrList+curSubAlphaTreeIndex, minTime, minDepth);
                    while(num-- > 0){
                        curSubAlphaTreeIndex += strlen(subAlphaTreeStrList + curSubAlphaTreeIndex) + 1;
                        ++allNum;
                    }
                }
            }
            //去重
            curSubAlphaTreeIndex = 0;
            while (allNum-- > 0){
                auto it = substrSet.find(subAlphaTreeStrList+curSubAlphaTreeIndex);
                if(it == substrSet.end()){
                    substrSet.insert(subAlphaTreeStrList+curSubAlphaTreeIndex);
                }
                curSubAlphaTreeIndex += strlen(subAlphaTreeStrList + curSubAlphaTreeIndex) + 1;
            }
            //写回
            allNum = 0;
            for(auto it = substrSet.begin(); it != substrSet.end(); ++it){
                strcpy(subAlphatreeStr, *it);
                subAlphatreeStr += (strlen(*it)+1);
                ++allNum;
            }
            return allNum;
        }

//        float* getTarget(size_t futureIndex, size_t dayBefore, size_t sampleNum, size_t stockNum, float* dst, const char* codes){
//            return alphaDataBase_->getTarget(futureIndex, dayBefore, sampleNum, stockNum, dst, codes);
//        }
    protected:
        AlphaForest(size_t maxHistorySize, size_t maxStoctSize, size_t maxSampleSize, size_t maxFutureSize, size_t maxCacheSize, size_t maxAlphaTreeSize, size_t maxNodeSize, size_t maxSubTreeNum, size_t maxAchievementSize, int sampleBetaSize)
        : threadPool_(maxCacheSize){

            //申请alphatree的内存
            alphaTreeCache_ = Cache<AlphaTree>::create(maxAlphaTreeSize);
            char* p = alphaTreeCache_->getBuff();
            for(int i = 0; i != alphaTreeCache_->getMaxCacheSize(); ++i){
                new (p)AlphaTree(maxNodeSize, maxSubTreeNum);
                p += sizeof(AlphaTree);
            }
//            alphaAchievement_ = Cache<AlphaAchievement>::create(maxAchievementSize);
//            p = alphaAchievement_->getBuff();
//            for(int i = 0; i != alphaAchievement_->getMaxCacheSize(); ++i){
//                new (p)AlphaAchievement();
//                p += sizeof(AlphaAchievement);
//            }

            //申请中间结果的内存,有多少个maxCacheSize就可以同时计算几个alphatree
            alphaTreeMemoryCache_ = Cache<float>::create(maxCacheSize, maxNodeSize * AlphaDataBase::getElementSize(maxHistorySize, maxSampleSize,maxFutureSize) * maxStoctSize);
            codesCache_ = Cache<char>::create(maxCacheSize, maxStoctSize * CODE_LEN);
            flagCache_ = Cache<bool>::create(maxCacheSize, maxNodeSize * (maxHistorySize + maxSampleSize + maxFutureSize-1));

            alphaDataBase_ = new AlphaDataBase(sampleBetaSize, maxFutureSize);
            initAlphaElement();
        }

        ~AlphaForest(){
            for(int i = 0; i != alphaTreeCache_->getMaxCacheSize(); ++i)
                alphaTreeCache_->getCacheMemory(i)->~AlphaTree();
            Cache<AlphaTree>::release(alphaTreeCache_);
            cout<<"release alphatree"<<endl;
            //Cache<AlphaAchievement>::release(alphaAchievement_);
            Cache<float>::release(alphaTreeMemoryCache_);
            cout<<"release alphatree memory"<<endl;
            Cache<char>::release(codesCache_);
            cout<<"release codes memory"<<endl;
            Cache<bool>::release(flagCache_);
            cout<<"release flag memory"<<endl;
            delete alphaDataBase_;
            cout<<"release AlphaForest"<<endl;
        }
        AlphaTree* getAlphaTree(int id){
            return alphaTreeCache_->getCacheMemory(id);
        }

        //alphatree的内存空间
        Cache<AlphaTree>* alphaTreeCache_;
        //Cache<AlphaAchievement>* alphaAchievement_;

        //中间结果的内存空间
        Cache<float>*alphaTreeMemoryCache_;
        Cache<char>*codesCache_;
        Cache<bool>*flagCache_;
        //计算中间结果对应的线程
        ThreadPool threadPool_;

        AlphaDataBase*alphaDataBase_;
        map<const char*,IAlphaElement*, ptrCmp> alphaElementMap_;


        static AlphaForest*alphaForest_;
};


void AlphaForest::initialize(size_t maxHistorySize, size_t maxStoctSize, size_t maxSampleSize, size_t maxFutureSize, size_t maxCacheSize, size_t maxAlphaTreeSize, size_t maxNodeSize, size_t maxSubTreeNum, size_t maxAchievementSize, int sampleBetaSize){
    release();
    alphaForest_ = new AlphaForest(maxHistorySize, maxStoctSize, maxSampleSize, maxFutureSize, maxCacheSize, maxAlphaTreeSize, maxNodeSize, maxSubTreeNum, maxAchievementSize, sampleBetaSize);
}

void AlphaForest::release(){
    if(alphaForest_)
        delete alphaForest_;
}

AlphaForest* AlphaForest::getAlphaforest(){ return alphaForest_;}

AlphaForest* AlphaForest::alphaForest_ = nullptr;

#endif //ALPHATREE_ALPHAFOREST_H
