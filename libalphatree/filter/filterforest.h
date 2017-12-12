//
// Created by yanyu on 2017/11/26.
//

#ifndef ALPHATREE_FILTERFOREST_H
#define ALPHATREE_FILTERFOREST_H

#include "filtertree.h"
#include "../base/dcache.h"
#include "../base/cache.h"
#include "../base/threadpool.h"

namespace fb {
    class FilterForest{
    public:
        void clean(){
            treeNum = 0;
        }
        size_t treeNum = {0};
        int filterTrees[MAX_TREE_SIZE];
    };

    class FilterMachine {
    public:
        FilterMachine(int cacheSize, ThreadPool *threadPool) : threadPool_(threadPool) {
            filterTreeCache_ = DCache<FilterTree>::create();
            filterForestCache_ = DCache<FilterForest>::create();
            filterCache_ = Cache<FilterCache>::create(cacheSize + 1);
            char *p = filterCache_->getBuff();
            for (auto i = 0; i != filterCache_->getMaxCacheSize(); ++i) {
                new(p)FilterCache();
                p += sizeof(FilterCache);
            }
        }

        ~FilterMachine() {
            DCache<FilterTree>::release(filterTreeCache_);
            DCache<FilterForest>::release(filterForestCache_);
            for (auto i = 0; i != filterCache_->getMaxCacheSize(); ++i)
                filterCache_->getCacheMemory(i)->~FilterCache();
            Cache<FilterCache>::release(filterCache_);
        }

        int useCache() {
            return filterCache_->useCacheMemory();
        }

        FilterCache *getCache(int cacheId) {
            return filterCache_->getCacheMemory(cacheId);
        }

        void releaseCache(int cacheId) {
            filterCache_->releaseCacheMemory(cacheId);
        }

        void releaseFilterForest(int id) {
            FilterForest* f = getFilterForest(id);
            for(size_t i = 0; i < f->treeNum; ++i) {
                getFilterTree(f->filterTrees[i])->clean();
                filterTreeCache_->releaseCacheMemory(f->filterTrees[i]);
            }

            getFilterForest(id)->clean();
            filterForestCache_->releaseCacheMemory(id);
        }

        int useFilterForest() {
            return filterForestCache_->useCacheMemory();
        }

        void decode(int forestId, const char* line, size_t size){

        }
        const char* encode(int forestId, char* pout){

        }

        int train(int cacheId, int forestId) {

        }

    protected:
        FilterTree *getFilterTree(int id) {
            return &filterTreeCache_->getCacheMemory(id);
        }

        FilterForest *getFilterForest(int id){
            return &filterForestCache_->getCacheMemory(id);
        }

        //filtertree的内存空间
        DCache<FilterTree> *filterTreeCache_ = {nullptr};
        DCache<FilterForest> *filterForestCache_ = {nullptr};
        Cache<FilterCache> *filterCache_ = {nullptr};
        //计算中间结果对应的线程
        ThreadPool *threadPool_;
    };
}

#endif //ALPHATREE_FILTERFOREST_H
