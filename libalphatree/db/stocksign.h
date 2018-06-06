//
// Created by godpgf on 18-3-4.
//

#ifndef ALPHATREE_STOCKSIGN_H
#define ALPHATREE_STOCKSIGN_H

#include "stockdes.h"
#include "../base/iterator.h"

class MemorySignIterator : public IBaseIterator<size_t>{
public:
    MemorySignIterator(size_t* cache, size_t dayBefore, size_t sampleDays, size_t allDayNum):
            cache_(cache),dayBefore_(dayBefore),sampleDays_(sampleDays),allDays_(allDayNum),preSize_(0),curIndex_(0){
        size_t curDayIndex = allDays_ - dayBefore_ - sampleDays_;
        size_t allSize = cache_[allDays_-dayBefore_-1];
        if(curDayIndex > 0){
            preSize_ = cache_[curDayIndex-1];
        }
        curDataOffset_ = cache_[allDayNum + preSize_];
        size_ = allSize - preSize_;
    }

    virtual void operator++() {
        ++curIndex_;
        curDataOffset_ = cache_[allDays_ + preSize_ + curIndex_];
    }

    virtual void skip(long size, bool isRelative = true){
        if(isRelative)
            curIndex_ += size;
        else
            curIndex_ = size;

        curDataOffset_ = cache_[allDays_ + preSize_ + curIndex_];
        //cout<<":"<<curIndex_<<" "<<size_<<endl;
        //file_.read(reinterpret_cast< char* >( &curDataOffset_ ), sizeof( size_t ));
    }
    virtual bool isValid(){ return curIndex_ < size_;}

    virtual size_t&& getValue(){
        return std::move(curDataOffset_);
    }
    virtual long size(){ return size_;}

    virtual IBaseIterator<size_t >* clone(){
        return new MemorySignIterator(cache_, dayBefore_, sampleDays_, allDays_);
    }
protected:
    size_t* cache_;
    size_t dayBefore_;
    size_t sampleDays_;
    size_t allDays_;
    size_t preSize_;
    //当前是第几个元素
    size_t curIndex_;
    //当前信号数据对应的文件偏移
    size_t curDataOffset_;
    size_t size_;
};

class BinarySignIterator : public IBaseIterator<size_t>{
public:
    //在所有的信号中从dayBefore天前取样sampleDays的信号，机器学习一般都会需要某个信号前几天的数据，offset=-1就表示取信号发生前一天的数据
    BinarySignIterator(const char* path, size_t dayBefore, size_t sampleDays, size_t allDayNum):
            dayBefore_(dayBefore),sampleDays_(sampleDays),allDays_(allDayNum),preSize_(0),curIndex_(0){
        file_.open(path,ios::binary|ios::in);
        size_t curDayIndex = allDays_ - dayBefore_ - sampleDays_;
        size_t allSize = 0;
        file_.seekg(sizeof(size_t) * (allDays_-dayBefore_-1),ios::beg);
        file_.read( reinterpret_cast< char* >( &allSize ), sizeof( size_t ) );
        if(curDayIndex > 0){
            file_.seekg(sizeof(size_t) * (curDayIndex-1),ios::beg);
            file_.read(reinterpret_cast< char* >( &preSize_ ), sizeof( size_t ));
        }
        file_.seekg(sizeof(size_t) * (allDayNum + preSize_), ios::beg);
        file_.read(reinterpret_cast< char* >( &curDataOffset_ ), sizeof( size_t ));
        //file_.read(reinterpret_cast< char* >( &curDataOffset_ ), sizeof( size_t ));

        size_ = allSize - preSize_;

        path_ = new char[strlen(path) + 1];
        strcpy(path_, path);
    }

    virtual ~BinarySignIterator(){
        file_.close();
        delete []path_;
    }

    virtual void operator++() {
        ++curIndex_;
        file_.read(reinterpret_cast< char* >( &curDataOffset_ ), sizeof( size_t ));
        //file_.read(reinterpret_cast< char* >( &curDataOffset_ ), sizeof( size_t ));
    }

    virtual void skip(long size, bool isRelative = true){
        if(isRelative)
            curIndex_ += size;
        else
            curIndex_ = size;

        file_.seekg(sizeof(size_t) * (allDays_ + preSize_ + curIndex_), ios::beg);
        file_.read(reinterpret_cast< char* >( &curDataOffset_ ), sizeof( size_t ));
        //cout<<":"<<curIndex_<<" "<<size_<<endl;
        //file_.read(reinterpret_cast< char* >( &curDataOffset_ ), sizeof( size_t ));
    }
    virtual bool isValid(){ return curIndex_ < size_;}

    virtual size_t&& getValue(){
        return std::move(curDataOffset_);
    }
    virtual long size(){ return size_;}

    virtual IBaseIterator<size_t >* clone(){
        return new BinarySignIterator(path_, dayBefore_, sampleDays_, allDays_);
    }
protected:
    size_t dayBefore_;
    size_t sampleDays_;
    size_t allDays_;
    size_t preSize_;
    //当前是第几个元素
    size_t curIndex_;
    //当前信号数据对应的文件偏移
    size_t curDataOffset_;
    size_t size_;
    ifstream file_;
    char* path_;
};


class StockSign{
public:
    StockSign(const char* path, size_t allDays, bool isCache):allDays_(allDays){
        strcpy(path_, path);
        if(isCache){
            ifstream file;
            file.open(path,ios::binary|ios::in);
            file.seekg(sizeof(size_t) * (allDays_-1),ios::beg);
            size_t allSize = 0;
            file.read( reinterpret_cast< char* >( &allSize ), sizeof( size_t ) );
            cache_ = new size_t[allSize + allDays_];
            file.seekg(0, ios::beg);
            for(int i = 0; i < allSize + allDays_; ++i){
                file.read(reinterpret_cast< char* >( cache_ + i ), sizeof( size_t ));
            }
            file.close();
        }
    }

    IBaseIterator<size_t>* createIter(size_t dayBefore, size_t sampleDays){
        if(isCache()){
            return new MemorySignIterator(cache_, dayBefore, sampleDays, allDays_);
        } else {
            return new BinarySignIterator(path_, dayBefore, sampleDays, allDays_);
        }
    }

    bool isCache(){ return cache_ != nullptr;};

    virtual ~StockSign(){
        if(cache_)
            delete []cache_;
    }

protected:
    char path_[64];
    size_t allDays_;
    size_t* cache_ = {nullptr};
};

#endif //ALPHATREE_STOCKSIGN_H
