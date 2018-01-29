//
// Created by godpgf on 18-1-25.
//

#ifndef ALPHATREE_ITERATOR_H
#define ALPHATREE_ITERATOR_H

template <class T>
class BaseIterator{
public:
    virtual BaseIterator& operator++() { return *this;}
    virtual bool isValid(){ return false;}
    virtual T& operator*() { return (T&)(*this);}
    virtual int size(){ return 0;}
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
    /*Iterator(Iterator& other){
        owner_ = other.owner_;
        iter_ = other.iter_;
    }*/
    Iterator():owner_(nullptr),iter_(nullptr){}
    virtual ~Iterator(){
        owner_->cleanIter(iter_);
    }
    virtual Iterator& operator++() {
        ++(*iter_);
        return *this;
    }
    virtual bool isValid(){ return iter_->isValid();}
    virtual T& operator*() { return **iter_;}
    virtual int size(){ return iter_->size();}
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
        curMemory_ += 1;
        return *this;
    }
    virtual bool isValid(){ return curMemory_ < endMemory_;}
    virtual T& operator*() { return *curMemory_;}
    virtual int size(){ return endMemory_ - curMemory_;}
    //virtual Iterator<> clone(){ return MemoryIterator(curMemory_, size());}
protected:
    T* curMemory_ = {nullptr};
    T* endMemory_ = {nullptr};
};

#endif //ALPHATREE_ITERATOR_H
