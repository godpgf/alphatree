//
// Created by godpgf on 18-2-9.
//

#ifndef ALPHATREE_STOCKFEATURE_H
#define ALPHATREE_STOCKFEATURE_H

#include "stockdes.h"
#include "../base/iterator.h"

template <class T>
class StockFeature{
public:
    StockFeature(const char* path, int cacheSize = 0){
        strcpy(path_, path);
        if(cacheSize > 0){
            cache_ = new T[cacheSize];
            int curIndex = 0;
            BinaryRangeIterator<T> iter(path_, 0, cacheSize);
            while (iter.isValid()){
                T value = *iter;
                //if(curIndex >= 1175221 && curIndex < 1175221 + 3025)
                //    cout<<value<<endl;
                cache_[curIndex++] = value;
                ++iter;
            }
        }
    }

    BaseIterator<T>* createIter(size_t offset, size_t days){
        if(cache_){
            return new MemoryIterator<T>(cache_ + offset, days);
        }else{
            return new BinaryRangeIterator<T>(path_, offset, days);
        }

    }

    bool isCache(){ return cache_ != nullptr;};

    virtual ~StockFeature(){
        if(cache_)
            delete []cache_;
    }

protected:

    char path_[64];
    T* cache_ = {nullptr};

};

#endif //ALPHATREE_STOCKFEATURE_H
