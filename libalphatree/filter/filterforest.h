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
        void addTree(int treeId){
            filterTrees[treeNum++] = treeId;
        }
        template <class F>
        void decode(const char* line, size_t size, F&& f){
            decode(line, 0, size-1, f);
        }
        template <class F>
        char* encode(char* pout, F&& f){
            int curIndex = encode(pout, 0, 0, f);
            pout[curIndex] = 0;
            return pout;
        }
        size_t treeNum = {0};
        int filterTrees[MAX_TREE_SIZE];
    protected:
        template <class F>
        void decode(const char* line, int l, int r, F&& f){
            int curIndex = l;
            if(line[curIndex] == '(' && line[r] == ')'){
                ++l;
                --r;
                curIndex = l;

                if(getAddIndex(line, l, r, curIndex)){
                    decode(line, l, curIndex-2, f);
                    decode(line, curIndex+2, r, f);
                } else {
                    addTree(f(line, l-1, r+1));
                }
            }
            else {
                CHECK(false, "format error!");
            }
        }
        template <class F>
        int encode(char* pout, int curIndex, int curTreeIndex, F&& f){
            if(curTreeIndex == treeNum - 1){
                return f(filterTrees[curTreeIndex], pout, curIndex);
            } else {
                pout[curIndex++] = '(';
                curIndex = f(filterTrees[curTreeIndex], pout, curIndex);
                strcpy(pout + curIndex, " + ");
                curIndex += 3;
                curIndex = encode(pout, curIndex, curTreeIndex+1, f);
                pout[curIndex++] = ')';
                return curIndex;
            }
        }
        static bool getAddIndex(const char* line, int l, int r, int& curIndex){
            curIndex = l;
            int depth = 0;
            while(curIndex < r){
                if(line[curIndex] == '(')
                    ++depth;
                else if(line[curIndex] == ')')
                    --depth;
                else if(depth == 0 && line[curIndex] == '+')
                    return line[curIndex] == '+';
                ++curIndex;
            }
            throw "format err";
        }
    };

    class FilterMachine {
    public:
        FilterMachine(int cacheSize) : threadPool_(cacheSize) {
            filterTreeCache_ = DCache<FilterTree>::create();
            filterForestCache_ = DCache<FilterForest>::create();
            filterCache_ = Cache<FilterCache>::create(cacheSize);
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
            FilterMachine* fm = this;
            getFilterForest(forestId)->decode(line, size, [fm](const char* line, int l, int r){
                int treeId = fm->filterTreeCache_->useCacheMemory();
                fm->getFilterTree(treeId)->decode(line, l, r);
                return treeId;
            });
        }
        const char* encode(int forestId, char* pout){
            FilterMachine* fm = this;
            getFilterForest(forestId)->encode(pout, [fm](int treeId, char* pout, int curIndex){
                return fm->getFilterTree(treeId)->encode(pout, curIndex);
            });
            return pout;
        }

        void train(int cacheId, int forestId, ThreadPool* forestThreadPoll) {
            FilterForest *forest = getFilterForest(forestId);
            FilterCache* cache = getCache(cacheId);
            cache->sample();
            for(size_t i = 0; i < cache->treeSize; ++i){
                forest->addTree(filterTreeCache_->useCacheMemory());
            }
            for(size_t i = 0; i < cache->treeSize; ++i) {
                FilterTree* ftree = &filterTreeCache_->getCacheMemory(forest->filterTrees[i]);
                int treeIndex = i;
                ThreadPool* tpool = &threadPool_;
                cache->treeRes[i] = forestThreadPoll->enqueue([ftree, cache, treeIndex, tpool]{
                    return ftree->train(cache, treeIndex, tpool);
                }).share();
            }
            for(size_t i = 0; i < cache->treeSize; ++i)
                cache->treeRes[i].get();
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
        ThreadPool threadPool_;
    };
}

#endif //ALPHATREE_FILTERFOREST_H
