//
// Created by yanyu on 2017/10/9.
//

#ifndef ALPHATREE_DCACHE_H
#define ALPHATREE_DCACHE_H
#include "darray.h"

#define DCACHE_BLOCK_SIZE 512

template <class T>
class DCache{

public:
    static DCache<T>* create(){ return new DCache<T>();}
    static void release(DCache<T> * cache){delete cache;}

    T& getCacheMemory(int cacheIndex){
        return cacheMemoryBuf_[cacheIndex];
    }

    int useCacheMemory(){
        //通过互斥遍历保证以下语句一个时间只有一个线程在执行,这时会mutex_.lock()
        std::unique_lock<std::mutex> lock{mutex_};
        if(curFreeCacheMemoryId_ == cacheMemoryBuf_.getSize()){
            cacheMemoryBuf_[curFreeCacheMemoryId_];
            freeCacheMemoryIds_.add(curFreeCacheMemoryId_);
        }
        return freeCacheMemoryIds_[curFreeCacheMemoryId_++];
    }

    void releaseCacheMemory(int id){
        std::lock_guard<std::mutex> lock{mutex_};
        freeCacheMemoryIds_[--curFreeCacheMemoryId_] = id;
    }


protected:
    DCache(){ }

    ~DCache(){
        cacheMemoryBuf_.clear();
        freeCacheMemoryIds_.clear();
    }

    //中间结果的内存空间
    DArray<T, DCACHE_BLOCK_SIZE> cacheMemoryBuf_;
    DArray<int, DCACHE_BLOCK_SIZE> freeCacheMemoryIds_;
    //curFreeCacheMemoryId_以后的数据都是闲置的,以前的数据是用过的(没意义)
    int curFreeCacheMemoryId_ = {0};


    //线程安全
    std::mutex mutex_;
};

#endif //ALPHATREE_DCACHE_H
