//
// Created by godpgf on 18-2-27.
//

#ifndef ALPHATREE_SAMPLE_H
#define ALPHATREE_SAMPLE_H

#include "alpha.h"

//振幅取样，保证2/3是数据振幅超过coff，1/3数据是振幅不超过coff的，这样保证数据在正负样本和中立样本的无偏性
void* amplitudeSample(void** pars, float coff, int historySize, int stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    int curIndex = 0;
    int moreCoffCnt = 0;
    int lessCoffCnts[] = {0,0,0,0,0,0,0,0};
    for(int i = 0; i < historySize; ++i) {
        for(int j = 0; j < stockSize; ++j){
            curIndex = i * stockSize + j;
            float a = abs(pleft[curIndex]);
            if(a > coff)
                ++moreCoffCnt;
            else{
                for(int k = 7; k >= 0; --k){
                    if(a <= coff)
                        ++lessCoffCnts[k];
                    a *= 2;
                }
            }
        }
    }
    //控制中立数据比例
    moreCoffCnt /= 2.0f;

    if(lessCoffCnts[7] <= moreCoffCnt){
        for(int i = 0; i < historySize; ++i) {
            for(int j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                pout[curIndex] = 1;
            }
        }
    } else {
        float p = 0;
        float minCoff = coff;
        for(int k = 0; k < 8; ++k){
            if(lessCoffCnts[k] > moreCoffCnt){
                minCoff *= pow(0.5f,7.0f-k);
                p = moreCoffCnt / (float)lessCoffCnts[k];
                break;
            }
        }

        srand((unsigned)time(0));
        for(int i = 0; i < historySize; ++i) {
            for(int j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                float v = abs(pleft[curIndex]);
                if(v > coff){
                    pout[curIndex] = 1;
                } else{
                    if(v <= minCoff){
                        if(((double)rand())/RAND_MAX <= p){
                            pout[curIndex] = 1;
                        } else {
                            pout[curIndex] = 0;
                        }
                    }else{
                        pout[curIndex] = 0;
                    }
                }

            }
        }
    }

    return pout;
}

#endif //ALPHATREE_SAMPLE_H
