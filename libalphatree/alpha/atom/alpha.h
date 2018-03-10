//
// Created by yanyu on 2017/7/12.
//

#ifndef ALPHATREE_ALPHA_H
#define ALPHATREE_ALPHA_H

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include "base.h"
//测试
#include <iostream>
using namespace std;

//todo 改bool
enum class CacheFlag{
    CAN_NOT_FLAG = -1,
    NO_FLAG = 0,
    NEED_CAL = 1,
    HAS_CAL = 2,
};


//计算前几天的数据累加
void* sum(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)pars[0];
    size_t d = (size_t)roundf(coff) + 1;

    for(size_t i = 1; i < historySize; ++i){
        _add((pout+i*stockSize),(pout + (i-1)*stockSize), stockSize);
    }
    for(size_t i = historySize-1; i >= d; --i){
        _reduce((pout + i * stockSize), (pout + (i-d) * stockSize), stockSize);
    }
    return pout;
}

//计算前几天数据的累积
void* product(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)(pars[1]);
    float* pleft = (float*)(pars[0]);
    memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    size_t d = (size_t)roundf(coff);
    for(size_t i = d; i < historySize; ++i){
        for(int j = 1; j <= d; ++j)
            _mul(pout + i * stockSize, pleft + (i-j) * stockSize, stockSize);
        /*_mulNoZero((pout + i * stockSize), (pout + (i - 1) * stockSize), stockSize);
        if(i > d){
            _divNoZero((pout + i * stockSize), (pleft + (i - 1 - d) * stockSize), stockSize);
        }*/
    }
    return pout;
}


void* mean(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    sum(pars, coff, historySize, stockSize, pflag);
    float* pout = (float*)pars[0];
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL){
            _div(pout + i * stockSize, roundf(coff) + 1, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }

    return pout;
}

void* lerp(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _lerp(pleft + i * stockSize, pleft + i * stockSize, pright + i * stockSize, coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pleft;
}

void* delta(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    size_t d = (size_t)roundf(coff);
    float* pout = (float*)pars[0];
    for(size_t i = historySize-1; i >= d; --i) {
        if(pflag[i] == CacheFlag::NEED_CAL){
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce(pout + i * stockSize, pout + (i - d) * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* preRise(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    size_t d = (size_t)roundf(coff);
    float* pout = (float*)pars[0];
    for(size_t i = historySize-1; i >= d; --i) {
        if(pflag[i] == CacheFlag::NEED_CAL){
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _preRise(pout + i * stockSize, pout + (i - d) * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* rise(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    size_t d = (size_t)roundf(coff);
    float* pout = (float*)pars[0];
    for(size_t i = historySize-1; i >= d; --i) {
        if(pflag[i] == CacheFlag::NEED_CAL){
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _rise(pout + i * stockSize, pout + (i - d) * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* meanRise(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    coff = roundf(coff);
    size_t d = (size_t)roundf(coff);
    for(size_t i = historySize-1; i >= d; --i) {
        if(pflag[i] == CacheFlag::NEED_CAL){
            _reduce(pout + i * stockSize, pout + (i - d) * stockSize, stockSize);
            _div(pout + i * stockSize, roundf(coff), stockSize);
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            //_reduce(pout + i * stockSize, pleft + (i - d) * stockSize, stockSize);
            //_div(pout + i * stockSize, roundf(coff), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* div(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    //memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    float* pout = pleft;
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _div((pout + i * stockSize), (pleft + i * stockSize), (pright + i * stockSize), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* meanRatio(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[1]);
    float* pleft = (float*)(pars[0]);
    memcpy(pout,pleft,stockSize*historySize* sizeof(float));
    mean(pars+1, coff, historySize, stockSize, pflag);
    return div(pars, coff, historySize, stockSize, pflag);
}

void* add(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    //float* pleft = (float*)(pars[0]);
    float* pright = (float*)(pars[1]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), (pright + i * stockSize), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* addFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    //float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* reduce(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    //float* pleft = (float*)(pars[0]);
    float* pright = (float*)(pars[1]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce((pout + i * stockSize), (pright + i * stockSize), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* reduceFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    //float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce(coff, (pout + i * stockSize), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* reduceTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    //float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce((pout + i * stockSize), coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }

    return pout;
}

void* mul(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pright = (float*)pars[1];
//float* pout = pleft;
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _mul((pout + i * stockSize), (pright + i * stockSize), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* mulFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
                float* pout = (float*)pars[0];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _mul((pout + i * stockSize), coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* divFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _div(coff, pout + i * stockSize, stockSize);
        }
        //else {
         //   memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* divTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
     float* pout = (float*)pars[0];
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _div(pout + i * stockSize, coff, stockSize);
        }
        //else {
          //  memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* signAnd(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];


    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _signAnd((pout + i * stockSize), (pleft + i * stockSize), (pright + i * stockSize), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* signOr(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _signOr((pout + i * stockSize), (pleft + i * stockSize), (pright + i * stockSize), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* mid(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), (pright + i * stockSize), stockSize);
            _div(pout + i * stockSize, 2.0f, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

//标准差相关------------------------------------------
void* calStddevData(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, int dataType){

    float* pmean = (float*)pars[1];
    float* pout = pmean;
    float* pleft = (float*)pars[0];
    memcpy(pmean,pleft,stockSize * historySize * sizeof(float));
    mean(pars+1, coff, historySize, stockSize, pflag);

    size_t d = (size_t)roundf(coff);
    //int blockSize = historySize * stockSize;
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL && i >= d){
            for(size_t j = 0; j < stockSize; ++j){
                float sum = 0;
                float meanValue = pmean[i * stockSize + j];
                for(size_t k = 0; k <= d; ++k){
                    float tmp = pleft[(i-k) * stockSize + j] - meanValue;
                    sum += tmp * tmp;
                }
                sum /= (d+1);
                switch(dataType){
                    case 0:
                        //stddev
                        pout[i * stockSize + j] = sqrtf(sum);
                        break;
                    case 1:
                        //up
                        pout[i * stockSize + j] = sqrtf(sum) + meanValue;
                        break;
                    case 2:
                        //down
                        pout[i * stockSize + j] = meanValue - sqrtf(sum);
                        break;
                }
            }
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}

    }
    return pout;
}
void* stddev(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    return calStddevData(pars, coff, historySize, stockSize, pflag, 0);
}
void* up(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    return calStddevData(pars, coff, historySize, stockSize, pflag, 1);
}
void* down(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    return calStddevData(pars, coff, historySize, stockSize, pflag, 2);
}
//----------------------------------------------------------------------
void* ranking(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = pout;
    float* pindex = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            float* curIndex = pindex + i * stockSize;
            //flagNan(curOut, curStockFlag, stockSize);
            _ranksort(curIndex, (pleft + i * stockSize), stockSize);
            _rankscale((pout + i * stockSize), curIndex, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}
//void* rankSort(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
//    for(size_t i = 0; i < historySize; ++i){
//        if(pflag[i] == CacheFlag::NEED_CAL){
//            float* curOut = pout + i * stockSize;
//            //flagNan(curOut, curStockFlag, stockSize);
//            _ranksort(curOut, (pleft + i * stockSize), (pStockFlag + i * stockSize), stockSize);
//        } else {
//            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
//        }
//    }
//    return pout;
//}
//
//void* rankScale(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
//    for(size_t i = 0; i < historySize; ++i){
//        if(pflag[i] == CacheFlag::NEED_CAL){
//            _rankscale((pout + i * stockSize), (pleft + i * stockSize), stockSize);
//        } else {
//            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
//        }
//    }
//    return pout;
//}



void* powerMid(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            _powerMid(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* tsRank(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)(pars[1]);
    float* pleft = (float*)(pars[0]);
    memset(pout, 0, sizeof(float)*historySize*stockSize);
    size_t d = (size_t)roundf(coff);
    for(size_t i = 1; i < d + 1; ++i){
        for(size_t j = i; j <historySize; ++j){
            int curBlockIndex = j * stockSize;
            int beforeBlockIndex = (j-i) * stockSize;
            if(pflag[j] == CacheFlag::NEED_CAL){
                _tsRank(pout + curBlockIndex, pleft + curBlockIndex, pleft + beforeBlockIndex, stockSize);
            }
        }
    }
    size_t size = historySize * stockSize;
    for(size_t i = 0; i < size; ++i)
        pout[i] /= d;
    return pout;
}

void* delay(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    size_t d = (size_t)roundf(coff);
    for(size_t i = historySize-1; i >= d; --i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            //cout<<(pout + i * stockSize)[19]<<":"<<(pleft + (i-d) * stockSize)[19]<<endl;
            memcpy(pout + i * stockSize, pleft + (i - d) * stockSize, stockSize * sizeof(float));
            //cout<<(pout + i * stockSize)[19]<<endl;
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* correlation(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    float* pright = (float*)(pars[1]);
    size_t d = (size_t)roundf(coff);
    for(size_t i = historySize-1; i >= d; --i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            //计算第日股票前d天的相关系数
            for(size_t j = 0; j < stockSize; ++j){
                //计算当前股票的均值和方差
                float meanLeft = 0;
                float meanRight = 0;
                float sumSqrLeft = 0;
                float sumSqrRight = 0;
                for(size_t k = 0; k <= d; ++k){
                    int curBlock = (i - k) * stockSize;
                    meanLeft += pleft[curBlock + j];
                    sumSqrLeft += pleft[curBlock + j] * pleft[curBlock + j];
                    meanRight += pright[curBlock + j];
                    sumSqrRight += pright[curBlock + j] * pright[curBlock + j];
                }
                meanLeft /= (d+1);
                meanRight /= (d+1);


                float cov = 0;
                for(size_t k = 0; k <= d; ++k){
                    int curBlock = (i - k) * stockSize;
                    cov += (pleft[curBlock + j] - meanLeft) * (pright[curBlock + j] - meanRight);
                }
                //cov /= (d+1);
                float xDiff2 = (sumSqrLeft - meanLeft*meanLeft*(d+1));
                float yDiff2 = (sumSqrRight - meanRight*meanRight*(d+1));
                if(xDiff2 < 0.000000001 || yDiff2 < 0.000000001)
                    pout[i * stockSize + j] = 1;
                else
                    pout[i * stockSize + j] = cov / sqrtf(xDiff2) / sqrtf(yDiff2);

                /*float test_ydiff2 =0;
                for(size_t k = 0; k <= d; ++k){
                    int curBlock = (i - k) * stockSize;
                    test_ydiff2 += (pright[curBlock + j] - meanRight) * (pright[curBlock + j] - meanRight);
                }
                cout<<j<<":"<<meanLeft<<" "<<meanRight<<" "<<xDiff2<<" "<<yDiff2<<" "<<test_ydiff2<<" "<<cov<<endl;*/
            }
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* covariance(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)(pars[2]);
    float* pleft = (float*)(pars[0]);
    float* pright = (float*)(pars[1]);

    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _mul((pout + i * stockSize), (pright + i * stockSize), stockSize);
        }
    }
    void *childMemory[1];
    childMemory[0] = pout;
    sum(childMemory, coff, historySize, stockSize, pflag);
    childMemory[0] = pleft;
    sum(childMemory, coff, historySize, stockSize, pflag);
    childMemory[0] = pright;
    sum(childMemory, coff, historySize, stockSize, pflag);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL){
            _div(pout + i * stockSize, roundf(coff) + 1, stockSize);
            _div(pleft + i * stockSize, roundf(coff) + 1, stockSize);
            _div(pright + i * stockSize, roundf(coff) + 1, stockSize);
        }
    }

    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _mul((pleft + i * stockSize), (pright + i * stockSize), stockSize);
        }
    }

    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce((pout + i * stockSize), (pleft + i * stockSize), stockSize);
        }
    }

    return pout;
}

void* scale(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)(pars[0]);
    //float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i)
        if(pflag[i] == CacheFlag::NEED_CAL){
            //memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _scale(pout + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    return pout;
}

void* decayLinear(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    //memset(pout, 0, sizeof(float)*historySize*stockSize);
    float* pout = (float*)(pars[0]);
    size_t d = (size_t)roundf(coff) + 1;
    float allWeight = (d + 1) * d * 0.5;
    for(size_t i = historySize; i >= d-1; --i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            for(size_t j = 0; j < d; j++){
                if(j == 0)
                    _mul(pout + i * stockSize, (d - j) / allWeight, stockSize);
                else
                    _addAndMul(pout + i * stockSize, pout + (i - j) * stockSize, (d - j) / allWeight, stockSize);
            }
        }
    }
    return pout;
}

void* tsMin(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[1]);
    float* pleft = (float*)(pars[0]);
    size_t d = (size_t)roundf(coff);
    _tsMinIndex(pout, pleft, historySize, stockSize, d);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            for(size_t j = 0; j < stockSize; ++j){
                int minId = (int)pout[i * stockSize + j];
                pout[i * stockSize + j] = pleft[minId * stockSize + j];
            }
        }

    }
    return pout;
}

void* tsMax(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[1]);
    float* pleft = (float*)(pars[0]);
    size_t d = (size_t)roundf(coff);
    _tsMaxIndex(pout, pleft, historySize, stockSize, d);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            for(size_t j = 0; j < stockSize; ++j){
                int maxId = (int)pout[i * stockSize + j];
                pout[i * stockSize + j] = pleft[maxId * stockSize + j];
            }
        }

    }
    return pout;
}

void* min(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    float* pright = (float*)(pars[1]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _min(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
    }
    return pout;
}

void* minFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _minFrom(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        }
    }
    return pout;
}

void* minTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _minTo(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

void* max(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    float* pright = (float*)(pars[1]);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _max(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}


void* maxFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _maxFrom(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        }
    }
    return pout;
}

void* maxTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _maxTo(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

//返回最小元素的下标，当前元素的下标是0,上一个元素下标是1
void* tsArgMin(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[1]);
    float* pleft = (float*)(pars[0]);
    size_t d = (size_t)roundf(coff);
    _tsMinIndex(pout, pleft, historySize, stockSize, d);
    for(size_t i = 0; i < historySize; ++i){
        for(size_t j = 0; j < stockSize; ++j){
            int minId = (int)pout[i * stockSize + j];
            if(minId + d >= i)
                pout[i * stockSize + j] = i - minId;
            else
                pout[i * stockSize + j] = d;
        }
    }
    return pout;
}

void* tsArgMax(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[1]);
    float* pleft = (float*)(pars[0]);
    size_t d = (size_t)roundf(coff);
    _tsMaxIndex(pout, pleft, historySize, stockSize, d);
    for(size_t i = 0; i < historySize; ++i){
        for(size_t j = 0; j < stockSize; ++j){
            int maxId = (int)pout[i * stockSize + j];
            if(maxId + d >= i)
                pout[i * stockSize + j] = i - maxId;
            else
                pout[i * stockSize + j] = d;
        }
    }
    return pout;
}

void* sign(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _sign((pout + i * stockSize), (pleft + i * stockSize), stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* abs(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _abs(pout + i * stockSize, pleft + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }

    return pout;
}

void* log(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float logmax = logf(0.0001);
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _log(pout + i * stockSize, pleft + i * stockSize, stockSize, logmax);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* signedPower(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    float* pright =(float*)(pars[1]);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _pow(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);

        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* signedPowerFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _pow(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* signedPowerTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _pow(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* lessCond(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _lessCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* moreCond(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    lessCond(pars, coff, historySize, stockSize, pflag);
    return reduceFrom(pars, 1, historySize, stockSize, pflag);
}

void* lessCondFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    //float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _lessCond(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* lessCondTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    //float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _lessCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* moreCondFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    //float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _moreCond(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* moreCondTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    //float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _moreCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* elseCond(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _elseCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* elseCondTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _elseCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* ifCond(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _ifCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* ifCondTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _ifCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* indneutralize(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    /*for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }*/
    return pars[0];
}



void* kd(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    int curIndex = 0;
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            for(size_t j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                if(i > 0)
                    pout[curIndex] = pout[curIndex - stockSize] * (2.f / 3.f) + pleft[curIndex] * (1.f / 3.f);
                else
                    pout[curIndex] = 0;
            }
        }
    }
    return pout;
}

void* cross(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    int curIndex = 0;
    size_t d = (size_t)roundf(coff);
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t i = historySize-1; i >= d; --i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            for(size_t j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                pout[curIndex] = (pleft[curIndex - d * stockSize] < pright[curIndex - d * stockSize] && pleft[curIndex] > pright[curIndex]) ? 1 : 0;
            }
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* crossFrom(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    int curIndex = 0;
    size_t d = (size_t)roundf(coff);
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    for(size_t i = historySize-1; i >= d; --i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            for(size_t j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                pout[curIndex] = (coff < pleft[curIndex - d*stockSize] && coff > pleft[curIndex]) ? 1 : 0;
            }
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* crossTo(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag) {
    int curIndex = 0;
    size_t d = (size_t)roundf(coff);
    float* pout = (float*)pars[0];
    float* pleft = (float*)pars[0];
    for(size_t i = historySize-1; i >= d; --i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            for(size_t j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                pout[curIndex] = (pleft[curIndex - d*stockSize] < coff && pleft[curIndex] > coff) ? 1 : 0;
            }
        }
        //else {
        //    memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        //}
    }
    return pout;
}

void* match(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    //cout<<"a\n";
    mulFrom(pars, -1, historySize, stockSize, pflag);
    int curIndex = 0;
    float* pout = (float*)pars[0];
    //float* pleft = (float*)pars[0];
    float* pright = (float*)pars[1];
    for(size_t j = 0; j < stockSize; ++j){
        //cout<<j<<endl;
        int buyIndex = -1;
        for(size_t i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                curIndex = i * stockSize + j;
                if(buyIndex > 0 && pright[curIndex] > 0){
                    pout[buyIndex] = (curIndex - buyIndex) / stockSize;
                    buyIndex = -1;
                }
                if(pout[curIndex] < 0 && buyIndex < 0){
                    buyIndex = curIndex;
                }
            }
        }
    }
    //cout<<"f\n";
    return pout;
}






#endif //ALPHATREE_ALPHA_H
