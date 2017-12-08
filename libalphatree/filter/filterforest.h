//
// Created by yanyu on 2017/11/26.
//

#ifndef ALPHATREE_FILTERFOREST_H
#define ALPHATREE_FILTERFOREST_H

#include "filtertree.h"
#include "../base/dcache.h"
#include "../base/cache.h"
#include "../base/threadpool.h"

namespace fb{
    class FilterForest{
        public:
            FilterForest(int cacheSize, ThreadPool* threadPool):threadPool_(threadPool){
                filterTreeCache_ = DCache<FilterTree>::create();
                filterCache_ = Cache<FilterCache>::create(cacheSize+1);
                char* p = filterCache_->getBuff();
                for(auto i = 0; i != filterCache_->getMaxCacheSize(); ++i){
                    new (p)FilterCache();
                    p += sizeof(FilterCache);
                }
            }
            ~FilterForest(){
                DCache<FilterTree>::release(filterTreeCache_);
                for(auto i = 0; i != filterCache_->getMaxCacheSize(); ++i)
                    filterCache_->getCacheMemory(i)->~FilterCache();
                Cache<FilterCache>::release(filterCache_);
            }

            int useCache(){
                return filterCache_->useCacheMemory();
            }

            FilterCache* getCache(int cacheId){
                return filterCache_->getCacheMemory(cacheId);
            }

            void releaseCache(int cacheId){
                filterCache_->releaseCacheMemory(cacheId);
            }

            void releaseFilterTree(int id){
                getFilterTree(id)->clean();
                filterTreeCache_->releaseCacheMemory(id);
            }

            int useFilterTree(){
                return filterTreeCache_->useCacheMemory();
            }

            
            int train(int cacheId, float lambda, ){
                //TODO
            }
        protected:
            FilterTree* getFilterTree(int id){
                return &filterTreeCache_->getCacheMemory(id);
            }
            //filtertree的内存空间
            DCache<FilterTree>* filterTreeCache_ = {nullptr};
            Cache<FilterCache>* filterCache_ = {nullptr};
            //计算中间结果对应的线程
            ThreadPool* threadPool_;
    };
}

#endif //ALPHATREE_FILTERFOREST_H
