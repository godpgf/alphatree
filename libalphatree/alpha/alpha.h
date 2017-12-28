//
// Created by yanyu on 2017/7/12.
// 实现所有计算,规定所有运算参数小于等于2个
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

enum class CacheFlag{
    CAN_NOT_FLAG = -1,
    NO_FLAG = 0,
    NEED_CAL = 1,
    HAS_CAL = 2,
};

const float* valid(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    for(size_t i = 0; i < historySize * stockSize; ++i){
        pout[i] = pStockFlag[i] ? 1 : 0;
    }
    return pout;
}

//计算前几天的数据累加
const float* sum(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    size_t d = (size_t)roundf(coff);
    for(size_t i = 1; i < historySize; ++i){
        _add((pout+i*stockSize),(pout + (i-1)*stockSize), stockSize);
        if(i > d){
            _reduce((pout+i*stockSize),(pleft + (i-1-d) * stockSize), stockSize);
        }
    }
    return pout;
}

//计算前几天数据的累积
const float* product(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    size_t d = (size_t)roundf(coff);
    for(size_t i = 1; i < historySize; ++i){
        _mulNoZero((pout + i * stockSize), (pout + (i - 1) * stockSize), stockSize);
        if(i > d){
            _divNoZero((pout + i * stockSize), (pleft + (i - 1 - d) * stockSize), stockSize);
        }
    }
    return pout;
}


const float* mean(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    sum(pleft, pright, coff, historySize, stockSize, pflag, pStockFlag,pout,psign);

    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL){
            _div(pout + i * stockSize, roundf(coff) + 1, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }

    return pout;
}

const float* lerp(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _lerp(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* delta(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    size_t d = (size_t)roundf(coff);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL && i >= d){
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce(pout + i * stockSize, pleft + (i - d) * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* meanRise(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    coff = roundf(coff);
    size_t d = (size_t)roundf(coff);
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL && i >= d){
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce(pout + i * stockSize, pleft + (i - d) * stockSize, stockSize);
            _div(pout + i * stockSize, roundf(coff), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* div(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    //memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _div((pout + i * stockSize), (pleft + i * stockSize), (pright + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* meanRatio(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    mean(pleft, pright, coff, historySize, stockSize, pflag, pStockFlag,pout,psign);
    return div(pleft, pout, coff, historySize, stockSize, pflag, pStockFlag,pout,psign);
}

const float* add(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), (pright + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* addFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* reduce(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce((pout + i * stockSize), (pright + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* reduceFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce(coff, (pout + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* reduceTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce((pout + i * stockSize), coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }

    return pout;
}

const float* mul(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _mul((pout + i * stockSize), (pright + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* mulFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _mul((pout + i * stockSize), coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* divFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _div(coff, pout + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* divTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _div(pout + i * stockSize, coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* signAnd(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _signAnd((pout + i * stockSize), (pleft + i * stockSize), (pright + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* signOr(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _signOr((pout + i * stockSize), (pleft + i * stockSize), (pright + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* mid(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), (pright + i * stockSize), stockSize);
            _div(pout + i * stockSize, 2.0f, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

//标准差相关------------------------------------------
const float* calStddevData(const float* pleft,const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign, int dataType){
    const float* pmean = (pright != nullptr ? pright : mean(pleft, pright, coff, historySize, stockSize, pflag, pStockFlag, pout, psign));

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
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }

    }
    return pout;
}
const float* stddev(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    return calStddevData(pleft, pright, coff, historySize, stockSize, pflag, pStockFlag, pout, psign, 0);
}
const float* up(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    return calStddevData(pleft, pright, coff, historySize, stockSize, pflag, pStockFlag, pout, psign, 1);
}
const float* down(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    return calStddevData(pleft, pright, coff, historySize, stockSize, pflag, pStockFlag, pout, psign, 2);
}
//----------------------------------------------------------------------
const float* rankSort(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            float* curOut = pout + i * stockSize;
            //flagNan(curOut, curStockFlag, stockSize);
            _ranksort(curOut, (pleft + i * stockSize), (pStockFlag + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* rankScale(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            _rankscale((pout + i * stockSize), (pleft + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}


//const float* ranking(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
//    for(size_t i = 0; i < historySize; ++i){
//        _ranking((pout + i * stockSize), (pcache + i * stockSize), (pleft + i * stockSize), stockSize);
//    }
//    return pout;
//}

const float* powerMid(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            _powerMid(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* tsRank(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
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

const float* delay(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    size_t d = (size_t)roundf(coff);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL && i >= d){
            memcpy(pout + i * stockSize, pleft + (i - d) * stockSize, stockSize * sizeof(float));
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* correlation(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    size_t d = (size_t)roundf(coff);
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL && i >= d){
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
            }
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* scale(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    for(size_t i = 0; i < historySize; ++i)
        if(pflag[i] == CacheFlag::NEED_CAL){
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _scale(pout + i * stockSize, (pStockFlag + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    return pout;
}

const float* decayLinear(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    memset(pout, 0, sizeof(float)*historySize*stockSize);
    size_t d = (size_t)roundf(coff) + 1;
    float allWeight = (d + 1) * d * 0.5;
    for(size_t i = d-1; i < historySize; i++){
        if(pflag[i] == CacheFlag::NEED_CAL){
            for(size_t j = 0; j < d; j++){
                _addAndMul(pout + i * stockSize, pleft + (i - j) * stockSize, (d - j) / allWeight, stockSize);
            }
        }
    }
    return pout;
}

const float* tsMin(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
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

const float* tsMax(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
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

const float* min(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _min(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* max(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _max(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* tsArgMin(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    size_t d = (size_t)roundf(coff);
    _tsMinIndex(pout, pleft, historySize, stockSize, d);
    for(size_t i = 0; i < historySize; ++i){
        for(size_t j = 0; j < stockSize; ++j){
            int minId = (int)pout[i * stockSize + j];
            if(minId + d >= i)
                pout[i * stockSize + j] = minId + d - i;
            else
                pout[i * stockSize + j] = 0;
        }
    }
    return pout;
}

const float* tsArgMax(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    size_t d = (size_t)roundf(coff);
    _tsMaxIndex(pout, pleft, historySize, stockSize, d);
    for(size_t i = 0; i < historySize; ++i){
        for(size_t j = 0; j < stockSize; ++j){
            int maxId = (int)pout[i * stockSize + j];
            if(maxId + d >= i)
                pout[i * stockSize + j] = maxId + d - i;
            else
                pout[i * stockSize + j] = 0;
        }
    }
    return pout;
}

const float* sign(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _sign((pout + i * stockSize), (pleft + i * stockSize), stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* abs(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _abs(pout + i * stockSize, pleft + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }

    return pout;
}

const float* log(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    float logmax = logf(0.0001);

    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _log(pout + i * stockSize, pleft + i * stockSize, stockSize, logmax);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* signedPower(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _pow(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);

        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* signedPowerFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _pow(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* signedPowerTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _pow(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* lessCond(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _lessCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* moreCond(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    return lessCond(pright, pleft, coff, historySize, stockSize, pflag, pStockFlag, pout, psign);
}

const float* lessCondFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _lessCond(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* lessCondTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _lessCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* moreCondFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _moreCond(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* moreCondTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _moreCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* elseCond(const float *pleft, const float *pright, float coff, size_t historySize, size_t stockSize,
                      CacheFlag* pflag, bool* pStockFlag, float *pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _elseCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* elseCondTo(const float *pleft, const float *pright, float coff, size_t historySize, size_t stockSize,
                        CacheFlag* pflag, bool* pStockFlag, float *pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _elseCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* ifCond(const float *pleft, const float *pright, float coff, size_t historySize, size_t stockSize,
                    CacheFlag* pflag, bool* pStockFlag, float *pout, int* psign){

    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _ifCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* ifCondTo(const float *pleft, const float *pright, float coff, size_t historySize, size_t stockSize,
                      CacheFlag* pflag, bool* pStockFlag, float *pout, int* psign){
    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL) {
            _ifCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* indneutralize(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* kd(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    int curIndex = 0;
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL && i > 0) {
            for(size_t j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                pout[curIndex] = pout[curIndex - stockSize] * (2.f / 3.f) + pleft[curIndex] * (1.f / 3.f);
            }
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* cross(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    int curIndex = 0;
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL && i > 0) {
            for(size_t j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                pout[curIndex] = (pleft[curIndex - stockSize] < pright[curIndex - stockSize] && pleft[curIndex] > pright[curIndex]) ? 1 : 0;
            }
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* crossFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    int curIndex = 0;
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL && i > 0) {
            for(size_t j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                pout[curIndex] = (coff < pleft[curIndex - stockSize] && coff > pleft[curIndex]) ? 1 : 0;
            }
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* crossTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign) {
    int curIndex = 0;
    for(size_t i = 0; i < historySize; ++i) {
        if(pflag[i] == CacheFlag::NEED_CAL && i > 0) {
            for(size_t j = 0; j < stockSize; ++j){
                curIndex = i * stockSize + j;
                pout[curIndex] = (pleft[curIndex - stockSize] < coff && pleft[curIndex] > coff) ? 1 : 0;
            }
        } else {
            memset(pout + i * stockSize, 0, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* match(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    mulFrom(pleft, pright, -1, historySize, stockSize, pflag, pStockFlag, pout, psign);
    int curIndex = 0;
    for(size_t j = 0; j < stockSize; ++j){
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
    return pout;
}

const float* fillSign(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    size_t dataSize = historySize*stockSize;
    memcpy(pout,pright,sizeof(float)*dataSize);
    memset(psign, 0, sizeof(int) * dataSize);
    int curIndex = 0;

    for(size_t j = 0; j < stockSize; ++j){
        for(size_t i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                curIndex = i * stockSize + j;

                if(pleft[curIndex] < 0)
                    psign[curIndex] = (int)pleft[curIndex];
                if(pleft[curIndex] > 0){
                    if(i + (int)pleft[curIndex] < historySize){
                        psign[curIndex] = (int)pleft[curIndex];
                    }
                    else
                        psign[curIndex] = -1;
                }
            }
        }
    }
    return pout;
}


const float* ftSharp(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* psign){
    size_t dataSize = historySize*stockSize;
    memset(pout, 0, sizeof(float)*dataSize);
    int curIndex = 0;
    float maxDropdown = 0;
    float maxPrice = 0;
    float meanDropdown = 0;
    float sumSqrDropdown = 0;
    size_t signCount = 0;
    for(size_t j = 0; j < stockSize; ++j){
        for(size_t i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                curIndex = i * stockSize + j;
                if(psign[curIndex] > 0 ){
                    maxPrice = pleft[curIndex];
                    maxDropdown = 0;
                    for(size_t k = 1; k <= psign[curIndex]; ++k){
                        maxPrice = max(pleft[curIndex + k * stockSize],maxPrice);
                        maxDropdown = max((maxPrice - pright[curIndex + k * stockSize]) / maxPrice, maxDropdown);
                    }
                    pout[curIndex] = maxDropdown;
                    meanDropdown += maxDropdown;
                    ++signCount;
                }
            }
        }
    }
    meanDropdown /= signCount;
    for(size_t j = 0; j < stockSize; ++j){
        for(size_t i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                curIndex = i * stockSize + j;
                if(psign[curIndex] > 0 ){
                    sumSqrDropdown += powf(pout[curIndex]-meanDropdown, 2);
                }
            }
        }
    }
    //dropdown的不确定性
    float std = sqrtf(sumSqrDropdown / signCount);
    for(size_t j = 0; j < stockSize; ++j){
        for(size_t i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                curIndex = i * stockSize + j;
                if(psign[curIndex] > 0 ){
                    //风险=dropdown的不确定性+dropdown本身
                    pout[curIndex] = (pleft[curIndex + psign[curIndex] * stockSize] - pleft[curIndex])/pleft[curIndex]/(pout[curIndex]+std);
                }
            }
        }
    }
    return pout;
}

#endif //ALPHATREE_ALPHA_H
