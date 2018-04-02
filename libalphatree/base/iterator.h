//
// Created by godpgf on 18-1-25.
//

#ifndef ALPHATREE_ITERATOR_H
#define ALPHATREE_ITERATOR_H

#include <fstream>
#include <sstream>

//数据访问器
template <class T>
class BaseIterator{
public:
    virtual ~BaseIterator(){}
    virtual void operator++() = 0;
    //跳过指定位置，isRelative如果是false就是从起点开始计算，否则是从当前开始计算
    virtual void skip(long size, bool isRelative = true) = 0;
    virtual bool isValid() = 0;
    virtual long size() = 0;
};

//只读数据访问器
template <class T>
class IBaseIterator : public BaseIterator<T>{
public:
    virtual T&& operator*() { return getValue();}
    virtual T&& getValue() = 0;
    virtual IBaseIterator<T>* clone() = 0;
};

//写数据访问器
template <class T>
class OBaseIterator : public BaseIterator<T>{
public:
    virtual void setValue(T&& data) = 0;
    virtual void setValue(const T& data) = 0;
    virtual void initialize(T&& data) = 0;
    virtual void initialize(const T& data) = 0;
};

//有序只读数据访问器
template <class T>
class OrderIterator: public IBaseIterator<T>{
public:
    //假设数据是有序的，跳到首次某个元素，这个元素刚好最后一次小于value
    virtual long jumpTo(T value){ return jumpTo(std::move(value), 0, this->size());}
    //virtual long jumpTo(const T& value){ return jumpTo(std::move(value), 0, this->size());}
    virtual long jumpTo(T value, long start, long length){ return -1;}
};

template <class T>
class IOBaseIterator : public OrderIterator<T>, public OBaseIterator<T>{
public:
    virtual void operator++() = 0;
    virtual void initialize(T&& data) = 0;
    virtual void initialize(const T& data) = 0;
    virtual void skip(long size, bool isRelative = true) = 0;
    virtual bool isValid() = 0;
};

template <class T>
class IteratorClient{
public:
    virtual BaseIterator<T>* createIter() = 0;
    virtual void cleanIter(BaseIterator<T>* iter){delete iter;}
};


template <class T>
class Iterator : public OrderIterator<T>{
public:
    Iterator(IteratorClient<T>* owner):owner_(owner){
        iter_ = (IBaseIterator<T>*)owner->createIter();
    }

    Iterator(IBaseIterator<T> *iter):owner_(nullptr),iter_(iter){}

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
    virtual void operator++() {
        ++(*iter_);
    }
    virtual void skip(long size, bool isRelative = true){
        iter_->skip(size, isRelative);
    }
    virtual bool isValid(){ return iter_->isValid();}
    virtual T&& getValue(){ return iter_->getValue();}
    virtual long size(){ return iter_->size();}
    virtual long jumpTo(T value){
        return ((OrderIterator<T>*)iter_)->jumpTo(value);
    }
    virtual long jumpTo(T value, long start, long length){
        return ((OrderIterator<T>*)iter_)->jumpTo(value, start, length);
    }

    virtual IBaseIterator<T>* clone(){
        return nullptr;
    }
protected:
    IteratorClient<T>* owner_;
    IBaseIterator<T> *iter_;
};

template <class T>
class MemoryIterator : public IOBaseIterator<T>{
public:
    MemoryIterator(T* curMemory, int size){
        curMemory_ = curMemory;
        endMemory_ = curMemory + size;
        size_ = size;
    }
    virtual void operator++() {
        curMemory_ = curMemory_ + 1;
    }
    virtual void skip(long size, bool isRelative = true){
        if(isRelative)
            curMemory_ += size;
        else
            curMemory_ = endMemory_ - this->size() + size;
    }

    virtual void initialize(T&& data)
    {
        skip(0, false);
        while (isValid()){
            setValue(std::move(data));
            skip(1);
        }
        skip(0, false);
    }
    virtual void initialize(const T& data){
        skip(0, false);
        while (isValid()){
            setValue(data);
            skip(1);
        }
        skip(0, false);
    }


    virtual bool isValid(){ return curMemory_ < endMemory_;}
    virtual T&& getValue() { return std::move(*curMemory_);}
    virtual void setValue(T&& data){*curMemory_ = data;}
    virtual void setValue(const T& data){*curMemory_ = data;}
    virtual long size(){ return size_;}
    virtual long jumpTo(T value, long start, long length){
        long dataNum = length;
        T* startMemory = endMemory_ - size() + start;
        if(startMemory[0] >= value){
            curMemory_ = startMemory;
            return -1;
        } else if(startMemory[dataNum-1] < value){
            curMemory_ = endMemory_ - size() + start + length - 1;
            return dataNum - 1;
        }
        long l = 0, r = dataNum-2;
        long curId = ((l + r) >> 1);
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
    virtual IBaseIterator<T>* clone(){ return new MemoryIterator<T>(endMemory_ - size(), size());}
protected:
    T* curMemory_ = {nullptr};
    T* endMemory_ = {nullptr};
    long size_;
};

template <class T>
class CSVIterator : public IBaseIterator<T>{
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

        path_ = new char[strlen(path) + 1];
        column_ = new char[strlen(column) + 1];
        strcpy(path_, path);
        strcpy(column_, column);
    }

    virtual ~CSVIterator(){
        inFile_.close();
        delete []path_;
        delete []column_;
    }

    virtual void operator++() {
        isValid_ = getline(inFile_, curLine_) ? true : false;
    }
    virtual void skip(long size, bool isRelative = true){
        if(isRelative == false)
            throw "不支持！";
        while (size && isValid_){
            isValid_ = getline(inFile_, curLine_) ? true : false;
            size--;
        }
    }
    virtual bool isValid(){ return isValid_;}
    virtual T&& getValue(){
        stringstream ss(curLine_);
        for(int i = 0; i <= columnIndex_; ++i)
            getline(ss, curData_, ',');
        readData(realData_);
        return std::move(realData_);
    }
    virtual long size(){ return -1;}

    virtual IBaseIterator<T>* clone(){ return new CSVIterator(path_, column_);}
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
    char* path_;
    char* column_;
};

template <class T>
class BinaryRangeIterator : public OrderIterator<T>{
public:
    BinaryRangeIterator(const char* path, size_t start, size_t size):start_(start), size_(size){
        file_.open(path,ios::binary|ios::in);
        file_.seekg(sizeof(T) * start,ios::beg);
        //file_>>realData_;
        file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
        //if(start == 0)
        //cout<<path<<" "<<realData_<<endl;
        path_ = new char[strlen(path) + 1];
        strcpy(path_, path);
    }

    virtual ~BinaryRangeIterator(){
        file_.close();
        delete []path_;
    }

    virtual IBaseIterator<T>* clone(){ return new BinaryRangeIterator(path_, start_, size_);}

    virtual void operator++() {
        if(isValid()){
            file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
            ++curIndex_;
        }
    }

    virtual void skip(long size, bool isRelative = true){
        if(isRelative){
            if(size > 0){
                file_.seekg(sizeof(T) * (size-1),ios::cur);
                curIndex_ += size;
            } else {
                return;
            }
        }
        else{
            file_.seekg(sizeof(T) * (size + start_),ios::beg);
            curIndex_ = size;
        }

        file_.read( reinterpret_cast< char* >( &realData_ ), sizeof( T ) );
    }

    virtual bool isValid(){
        return curIndex_ < size_;
    }

    virtual T&& getValue() {
        return std::move(realData_);
    }

    virtual long size(){
        return size_;
    }

    virtual long jumpTo(T value, long start, long length){
        long dataNum = length;
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
        long l = 0, r = dataNum-2;
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
    long curIndex_ = {0};
    T realData_;
    long start_;
    long size_;
    ifstream file_;
    char* path_;
};

template<class T>
T smooth(IBaseIterator<T>* piter, float threshold){
    size_t valueLength = threshold < 0.5f ? (size_t)(piter->size() * threshold) : (size_t)(piter->size() * (1 - threshold));
    T* values = new T[valueLength];
    int lastIndex = -1;
    int curIndex = 0;
    piter->skip(0, false);

    T lastValue, curValue;
    while (piter->isValid()){
        curValue = **piter;
        if(lastIndex == -1){
            values[0] = curValue;
            lastIndex = 0;
        } else {
            lastValue = values[lastIndex];
            curIndex = lastIndex;
            if(threshold < 0.5f && curValue < values[lastIndex]){
                while (curIndex > 0 && values[curIndex-1] > curValue){
                    values[curIndex] = values[curIndex-1];
                    --curIndex;
                }
                values[curIndex] = curValue;
                if(lastIndex < valueLength - 1)
                    values[++lastIndex] = lastValue;
            } else if(threshold >= 0.5 && curValue > values[lastIndex]){
                while (curIndex > 0 && values[curIndex-1] < curValue){
                    values[curIndex] = values[curIndex-1];
                    --curIndex;
                }
                values[curIndex] = curValue;
                if(lastIndex < valueLength - 1)
                    values[++lastIndex] = lastValue;
            }

        }
        ++*piter;
    }
    piter->skip(0, false);
    curValue = values[valueLength-1];
    delete []values;
    return curValue;
}

#endif //ALPHATREE_ITERATOR_H
