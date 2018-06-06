//
// Created by 严宇 on 2018/4/30.
//

#ifndef ALPHATREE_MATRIX_H
#define ALPHATREE_MATRIX_H
#include <string.h>


//稠密矩阵Dense Matrix
template <class T>
class DMatrix{
public:
    DMatrix(unsigned dim1, unsigned dim2, T* cache = nullptr){
        isCache_ = cache != nullptr;
        value_ = cache;
        setSize(dim1, dim2);
    }

    virtual ~DMatrix(){
        if(!isCache_){
            delete[] value_;
        }
    }

    virtual const T& get(int x, int y){
        if(x < 0)
            x += dim1_;
        if(y < 0)
            y += dim2_;
        return value_[x * dim2_ + y];
    }

    virtual void assign(DMatrix<T>& v){
        if((v.dim1_ != dim1_) || (v.dim2_ != dim2_)) setSize(v.dim1_, v.dim2_);
        memcpy(value_, v.value_, sizeof(T) * dim1_ * dim2_);
    }

    virtual void init(const T& value){
        int len = dim1_ * dim2_;
        for(int i = 0; i < len; ++i)
            value_[i] = value;
    }

    virtual void init(T&& value){
        int len = dim1_ * dim2_;
        for(int i = 0; i < len; ++i)
            value_[i] = value;
    }

    T& operator() (int x, int y) {
        //	assert((x < dim1) && (y < dim2));
        if(x < 0)
            x += dim1_;
        if(y < 0)
            y += dim2_;
        return value_[x * dim2_ + y];
    }
    T operator() (int x, int y) const {
        //	assert((x < dim1) && (y < dim2));
        if(x < 0)
            x += dim1_;
        if(y < 0)
            y += dim2_;
        return value_[x * dim2_ + y];
    }

    virtual void setSize(unsigned dim1, unsigned dim2){
        if(dim1 == dim1_ && dim2 == dim2_)
            return;
        dim1_ = dim1;
        dim2_ = dim2;
        if(!isCache_){
            delete[] value_;
            value_ = new T[dim1 * dim2];
        }

    }

    const unsigned& getDim1(){ return dim1_;}
    const unsigned& getDim2(){ return dim2_;}
protected:
    unsigned dim1_ = {0};
    unsigned dim2_ = {0};
    T* value_;
    bool isCache_;
};

#endif //ALPHATREE_MATRIX_H
