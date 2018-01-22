//
// Created by godpgf on 18-1-19.
//

#ifndef ALPHATREE_RANDOMCHOOSE_H
#define ALPHATREE_RANDOMCHOOSE_H

#include <string.h>
#include <math.h>
#include <random>

class RandomChoose{
public:
    RandomChoose(int elementSize):elementSize_(elementSize){
        elementList_ = new int[elementSize];
        memset(elementList_, 0, elementSize * sizeof(int));
    }

    virtual ~RandomChoose(){
        delete[] elementList_;
    }

    static bool isExplote(float exploteRatio){
        return rand() / (float)RAND_MAX < exploteRatio;
    }

    void add(int elementId){++elementList_[elementId];}
    void reduce(int elementId){--elementList_[elementId];}
    int choose(){
        float sumP = 0;
        for(int i = 0; i < elementSize_; ++i){
            sumP += sigmoid(elementList_[i]);
        }

        float rValue = rand() / (float)RAND_MAX;
        rValue *= sumP;
        for(int i = 0; i < elementSize_; ++i){
            rValue -= sigmoid(elementList_[i]);
            if(rValue <= 0){
                return i;
            }
        }
        return elementSize_-1;
    }
protected:
    static inline float sigmoid(float x)
    {
        return (1 / (1 + exp(-x)));
    }
    int* elementList_;
    int elementSize_;
};

#endif //ALPHATREE_RANDOMCHOOSE_H
