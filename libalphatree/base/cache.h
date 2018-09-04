//
// Created by yanyu on 2017/7/27.
//

#ifndef ALPHATREE_CACHE_H
#define ALPHATREE_CACHE_H

class BaseCache{
public:
    template <class T>
    T* initialize(size_t cacheSize){
        if(cacheSize > this->cacheSize){
            clean();
            this->cacheSize = cacheSize;
            cache = new char[cacheSize];
        }
        return (T*)cache;
    }

    ~BaseCache() {
        clean();
    }

    void clean(){
        if(cache)
            delete[] cache;
        cache = nullptr;
    }

    char *cache = {nullptr};
protected:
    size_t cacheSize = {0};
};

template <class T>
class Cache{

    public:
        static Cache<T>* create(size_t maxCacheSize, int cacheBlockNum = 1){ return new Cache<T>(maxCacheSize, cacheBlockNum);}
        static void release(Cache<T> * cache){delete cache;}

        T* getCacheMemory(int cacheIndex){
            T* list = reinterpret_cast<T*>(cacheMemoryBuf_);
            return (list + cacheIndex * cacheBlockNum_);
        }

        char* getBuff(){ return cacheMemoryBuf_;}

        int useCacheMemory(){
            //通过互斥遍历保证以下语句一个时间只有一个线程在执行,这时会mutex_.lock()
            std::unique_lock<std::mutex> lock{mutex_};
            //如果某个缓存被用完,需要等待别人归还后再去使用,如果条件不满足,会mutex_.unlock()然后等待
            //当别人notify_one时会再去检查条件,并mutex_.lock()
            //cout<<curFreeCacheMemoryId_<<"/"<<maxCacheSize_<<endl;
            cv_.wait(lock, [this]{ return curFreeCacheMemoryId_ < maxCacheSize_;});
            return freeCacheMemoryIds_[curFreeCacheMemoryId_++];
        }

        void releaseCacheMemory(int id){
            {
                std::lock_guard<std::mutex> lock{mutex_};
                freeCacheMemoryIds_[--curFreeCacheMemoryId_] = id;
            }
            cv_.notify_one();
        }

        int getMaxCacheSize(){ return maxCacheSize_;}

    protected:
        Cache(size_t maxCacheSize, int cacheBlockNum):maxCacheSize_(maxCacheSize), cacheBlockNum_(cacheBlockNum), curFreeCacheMemoryId_(0){
            cacheMemoryBuf_ = new char[sizeof(T) * maxCacheSize * cacheBlockNum];
            freeCacheMemoryIds_ = new int[maxCacheSize];
            for(size_t i = 0; i < maxCacheSize; ++i) {
                freeCacheMemoryIds_[i] = i;
            }
        }

        ~Cache(){
            delete []cacheMemoryBuf_;
            delete []freeCacheMemoryIds_;
        }

        //中间结果的内存空间
        char* cacheMemoryBuf_;
        size_t maxCacheSize_;
        int cacheBlockNum_;
        int*freeCacheMemoryIds_;
        size_t curFreeCacheMemoryId_;

        //线程安全
        std::mutex mutex_;
        std::condition_variable cv_;
};

#endif //ALPHATREE_CACHE_H
