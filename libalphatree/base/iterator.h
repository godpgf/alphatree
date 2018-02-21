//
// Created by godpgf on 18-1-25.
//

#ifndef ALPHATREE_ITERATOR_H
#define ALPHATREE_ITERATOR_H

#include <fstream>
#include <sstream>

template <class T>
class BaseIterator{
public:
    virtual ~BaseIterator(){}
    virtual BaseIterator& operator++() { return *this;}
    //跳过指定位置，isRelative如果是false就是从起点开始计算，否则是从当前开始计算
    virtual BaseIterator& skip(long size, bool isRelative = true){ return *this;}
    virtual bool isValid(){ return false;}
    virtual T& operator*() { return (T&)(*this);}
    virtual int size(){ return 0;}
    //假设数据是有序的，跳到首次某个元素，这个元素刚好最后一次小于value
    virtual int jumpTo(T value){ return jumpTo(value, 0, size());}
    virtual int jumpTo(T value, int start, int length){ return -1;}
};

template <class T>
class IteratorClient{
public:
    virtual BaseIterator<T>* createIter() = 0;
    virtual void cleanIter(BaseIterator<T>* iter){delete iter;}
};

template <class T>
class Iterator : public BaseIterator<T>{
public:
    Iterator(IteratorClient<T>* owner):owner_(owner){
        iter_ = owner->createIter();
    }

    Iterator(BaseIterator<T> *iter):owner_(nullptr),iter_(iter){}

    //禁止拷贝
    Iterator(const Iterator&) = delete;
    //禁止赋值
    Iterator &operator = (const Iterator &) = delete;


    Iterator():owner_(nullptr),iter_(nullptr){}
    virtual ~Iterator(){
        if(owner_)
            owner_->cleanIter(iter_);
        else if(iter_)
            delete iter_;
    }
    virtual Iterator& operator++() {
        ++(*iter_);
        return *this;
    }
    virtual Iterator& skip(long size, bool isRelative = true){
        iter_->skip(size, isRelative);
        return *this;
    }
    virtual bool isValid(){ return iter_->isValid();}
    virtual T& operator*() { return **iter_;}
    virtual int size(){ return iter_->size();}
    virtual int jumpTo(T value){
        return iter_->jumpTo(value);
    }
    virtual int jumpTo(T value, int start, int length){
        return iter_->jumpTo(value, start, length);
    }
protected:
    IteratorClient<T>* owner_;
    BaseIterator<T> *iter_;
};

template <class T>
class MemoryIterator : public BaseIterator<T>{
public:
    MemoryIterator(T* curMemory, int size){
        curMemory_ = curMemory;
        endMemory_ = curMemory + size;
    }
    virtual BaseIterator<T>& operator++() {
        curMemory_ = curMemory_ + 1;
        return *this;
    }
    virtual BaseIterator<T>& skip(long size, bool isRelative = true){
        if(isRelative)
            curMemory_ += size;
        else
            curMemory_ = endMemory_ - this->size() + size;
        return *this;
    }
    virtual bool isValid(){ return curMemory_ < endMemory_;}
    virtual T& operator*() { return *curMemory_;}
    virtual int size(){ return endMemory_ - curMemory_;}
    virtual int jumpTo(T value, int start, int length){
        int dataNum = length;
        T* startMemory = endMemory_ - size() + start;
        if(startMemory[0] >= value){
            curMemory_ = startMemory;
            return -1;
        } else if(startMemory[dataNum-1] < value){
            curMemory_ = endMemory_ - size() + start + length - 1;
            return dataNum - 1;
        }
        int l = 0, r = dataNum-2;
        int curId = ((l + r) >> 1);
        while (l < r){
            if(curMemory_[curId] >= value)
                r = curId - 1;
            else if(curMemory_[curId+1] < value)
                l = curId + 1;
            else
                break;
            curId = ((l + r) >> 1);
        }
        curMemory_ = startMemory + curId;
        return curId;
    }
    //virtual Iterator<> clone(){ return MemoryIterator(curMemory_, size());}
protected:
    T* curMemory_ = {nullptr};
    T* endMemory_ = {nullptr};
};

template <class T>
class CSVIterator : public BaseIterator<T>{
public:
    CSVIterator(const char* path, const char* column):inFile_(path, ios::in){
        columnIndex_ = -1;
        getline(inFile_, curLine_);
        stringstream ss(curLine_);
        isValid_ = false;
        while(getline(ss, curData_, ',')){
            ++columnIndex_;
            if(strcmp(curData_.c_str(), column) == 0){
                isValid_ = true;
                break;
            }

        }
        if(isValid_)
            ++(*this);
    }

    virtual ~CSVIterator(){
        inFile_.close();
    }

    virtual BaseIterator<T>& operator++() {
        isValid_ = getline(inFile_, curLine_) ? true : false;
        return *this;
    }
    virtual BaseIterator<T>& skip(long size, bool isRelative = true){
        if(isRelative == false)
            throw "不支持！";
        while (size && isValid_){
            isValid_ = getline(inFile_, curLine_) ? true : false;
            size--;
        }
        return *this;
    }
    virtual bool isValid(){ return isValid_;}
    virtual T& operator*() {
        stringstream ss(curLine_);
        for(int i = 0; i <= columnIndex_; ++i)
            getline(ss, curData_, ',');
        readData(realData_);
        return realData_;
    }
    virtual int size(){ return -1;}
    virtual int jumpTo(T value, int start, int length){
        throw "不支持";
    }
protected:
    void readData(long& data){
        data = atol(curData_.c_str());
    }
    void readData(int& data){
        data = atoi(curData_.c_str());
    }
    void readData(float& data){
        data = atof(curData_.c_str());
    }
    ifstream inFile_;
    int columnIndex_;
    string curLine_;
    string curData_;
    T realData_;
    bool isValid_;
};

template <class T>
class BinaryRangeIterator : public BaseIterator<T>{
public:
    BinaryRangeIterator(const char* path, size_t start, size_t size):start_(start), size_(size){
        file_.open(path,ios::binary|ios::in);
        file_.seekg(sizeof(T) * start,ios::beg);
        //file_>>realData_;
        file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
        //if(start == 0)
        //cout<<path<<" "<<realData_<<endl;
    }

    virtual ~BinaryRangeIterator(){
        file_.close();
    }

    virtual BaseIterator<T>& operator++() {
        if(isValid()){
            file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
            ++curIndex_;
        }
        return *this;
    }

    virtual BaseIterator<T>& skip(long size, bool isRelative = true){
        if(isRelative){
            if(size > 0){
                file_.seekg(sizeof(T) * (size-1),ios::cur);
                curIndex_ += size;
            } else {
                return *this;
            }
        }
        else{
            file_.seekg(sizeof(T) * (size + start_),ios::beg);
            curIndex_ = size;
        }

        file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );

        return *this;
    }

    virtual bool isValid(){
        return curIndex_ < size_;
    }

    virtual T& operator*() {
        return realData_;
    }

    virtual int size(){
        return size_;
    }

    virtual int jumpTo(T value, int start, int length){
        int dataNum = length;
        file_.seekg((start_ + start) * sizeof(T), ios::beg);
        file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
        if(realData_ >= value){
            curIndex_ = 0;
            return -1;
        } else{
            file_.seekg((start_ + start + dataNum-1) * sizeof(T), ios::beg);
            file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
            if(realData_ < value){
                curIndex_ = dataNum-1;
                return curIndex_;
            }
        }
        int l = 0, r = dataNum-2;
        curIndex_ = ((l + r) >> 1);
        while (l < r){
            file_.seekg((start_ + start + curIndex_) * sizeof(T), ios::beg);
            file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
            if(realData_ >= value)
                r = curIndex_ - 1;
            else{
                //再往后读一个元素
                file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
                if(realData_ < value)
                    l = curIndex_ + 1;
                else
                    break;

            }
            curIndex_ = ((l + r) >> 1);
        }
        file_.seekg((start_ + start + curIndex_) * sizeof(T), ios::beg);
        file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
        return curIndex_;
    }
protected:
    size_t curIndex_ = {0};
    T realData_;
    size_t start_;
    size_t size_;
    ifstream file_;
};

#endif //ALPHATREE_ITERATOR_H
