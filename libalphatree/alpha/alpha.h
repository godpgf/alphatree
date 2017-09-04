//
// Created by yanyu on 2017/7/12.
// 实现所有计算,规定所有运算参数小于等于2个
//

#ifndef ALPHATREE_ALPHA_H
#define ALPHATREE_ALPHA_H

#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "base.h"
//测试
#include <iostream>
using namespace std;

//计算前几天的数据累加
const float* sum(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    int d = (int) roundf(coff);
    for(int i = 1; i < historySize; i++){
        _add((pout+i*stockSize),(pout + (i-1)*stockSize), stockSize);
        if(i > d){
            _reduce((pout+i*stockSize),(pleft + (i-1-d) * stockSize), stockSize);
        }
    }
    return pout;
}

//计算前几天数据的累积
const float* product(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    int d = (int) roundf(coff);
    for(int i = 1; i < historySize; i++){
        _mul((pout+i*stockSize),(pout + (i-1)*stockSize), stockSize);
        if(i > d){
            _div((pout+i*stockSize),(pleft + (i-1-d) * stockSize), stockSize);
        }
    }
    return pout;
}



const float* mean(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    sum(pleft, pright, coff, historySize, stockSize, pflag, pout);
    for(int i = 0; i < historySize; ++i) {
        if(pflag[i]){
            _div(pout + i * stockSize, roundf(coff) + 1, stockSize);
        }
    }
    return pout;
}

const float* lerp(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if (pflag[i]) {
            _lerp(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

const float* delta(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    int d = (int) roundf(coff);
    for(int i = d; i < historySize; i++) {
        if(pflag[i]){
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce(pout + i * stockSize, pleft + (i - d) * stockSize, stockSize);
        }
    }
    return pout;
}

const float* meanRise(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    coff = roundf(coff);
    int d = (int) roundf(coff);
    for(int i = d; i < historySize; i++) {
        if(pflag[i]){
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce(pout + i * stockSize, pleft + (i - d) * stockSize, stockSize);
            _div(pout + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

const float* div(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    //memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            _div((pout + i * stockSize), (pleft + i * stockSize), (pright + i * stockSize), stockSize);
        }
    }
    return pout;
}

const float* meanRatio(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    mean(pleft, pright, coff, historySize, stockSize, pflag, pout);
    return div(pleft, pout, coff, historySize, stockSize, pflag, pout);
}

const float* add(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), (pright + i * stockSize), stockSize);
        }
    }
    return pout;
}

const float* addFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), coff, stockSize);
        }
    }
    return pout;
}

const float* reduce(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce((pout + i * stockSize), (pright + i * stockSize), stockSize);
        }
    }
    return pout;
}

const float* reduceFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce(coff, (pout + i * stockSize), stockSize);
        }
    }
    return pout;
}

const float* reduceTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _reduce((pout + i * stockSize), coff, stockSize);
        }
    }

    return pout;
}

const float* mul(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _mul((pout + i * stockSize), (pright + i * stockSize), stockSize);
        }
    }
    return pout;
}

const float* mulFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _mul((pout + i * stockSize), coff, stockSize);
        }
    }
    return pout;
}

const float* divFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; ++i) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _div(coff, pout + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* divTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; ++i) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _div(pout + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

const float* mid(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
            _add((pout + i * stockSize), (pright + i * stockSize), stockSize);
            _div(pout + i * stockSize, 2.0f, stockSize);
        }
    }
    return pout;
}

//标准差相关------------------------------------------
const float* calStddevData(const float* pleft,const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout, int dataType){
    const float* pmean = (pright != nullptr ? pright : mean(pleft, pright, coff, historySize, stockSize, pflag, pout));

    int d = (int)roundf(coff);
    //int blockSize = historySize * stockSize;
    for(int i = d; i < historySize; i++){
        if(pflag[i]){
            for(int j = 0; j < stockSize; j++){
                float sum = 0;
                float meanValue = pmean[i * stockSize + j];
                for(int k = 0; k <= d; k++){
                    float tmp = pleft[(i-k) * stockSize + j] - meanValue;
                    sum += tmp * tmp;
                }
                switch(dataType){
                    case 0:
                        //stddev
                        pout[i * stockSize + j] = sqrtf(sum / (d+1));
                        break;
                    case 1:
                        //up
                        pout[i * stockSize + j] = sqrtf(sum / (d+1)) + meanValue;
                        break;
                    case 2:
                        //down
                        pout[i * stockSize + j] = meanValue - sqrtf(sum / (d+1));
                        break;
                }
            }
        }

    }
    return pout;
}
const float* stddev(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    return calStddevData(pleft, pright, coff, historySize, stockSize, pflag, pout, 0);
}
const float* up(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    return calStddevData(pleft, pright, coff, historySize, stockSize, pflag, pout, 1);
}
const float* down(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    return calStddevData(pleft, pright, coff, historySize, stockSize, pflag, pout, 2);
}
//----------------------------------------------------------------------
const float* rankSort(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if(pflag[i]){
            _ranksort((pout + i * stockSize), (pleft + i * stockSize), stockSize);
        }
    }
    return pout;
}

const float* rankScale(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if(pflag[i]){
            _rankscale((pout + i * stockSize), (pleft + i * stockSize), stockSize);
        }
    }
    return pout;
}


//const float* ranking(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
//    for(int i = 0; i < historySize; i++){
//        _ranking((pout + i * stockSize), (pcache + i * stockSize), (pleft + i * stockSize), stockSize);
//    }
//    return pout;
//}

const float* powerMid(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    for(int i = 0; i < historySize; i++){
        if(pflag[i]){
            _powerMid(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* tsRank(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    memset(pout, 0, sizeof(float)*historySize*stockSize);
    int d = (int)roundf(coff);
    for(int i = 1; i < d + 1; ++i){
        for(int j = i; j <historySize; ++j){
            int curBlockIndex = j * stockSize;
            int beforeBlockIndex = (j-i) * stockSize;
            if(pflag[j]){
                _tsRank(pout + curBlockIndex, pleft + curBlockIndex, pleft + beforeBlockIndex, stockSize);
            }
        }
    }
    size_t size = historySize * stockSize;
    for(int i = 0; i < size; i++)
        pout[i] /= d;
    return pout;
}

const float* delay(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    int d = (int)roundf(coff);
    for(int i = d; i < historySize; i++){
        if(pflag[i]){
            memcpy(pout + i * stockSize, pleft + (i - d) * stockSize, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* future(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    int d = (int)roundf(coff);
    for(int i = 0; i < historySize-d; i++){
        if(pflag[i]){
            memcpy(pout + i * stockSize, pleft + (i + d) * stockSize, stockSize * sizeof(float));
        }
    }
    return pout;
}

const float* correlation(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    int d = (int)roundf(coff);
    for(int i = d; i < historySize; ++i){
        if(pflag[i]){
            //计算第日股票前d天的相关系数
            for(int j = 0; j < stockSize; ++j){
                //计算当前股票的均值和方差
                float meanLeft = 0;
                float meanRight = 0;
                float sumSqrLeft = 0;
                float sumSqrRight = 0;
                for(int k = 0; k <= d; ++k){
                    int curBlock = (i - k) * stockSize;
                    meanLeft += pleft[curBlock + j];
                    sumSqrLeft += pleft[curBlock + j] * pleft[curBlock + j];
                    meanRight += pright[curBlock + j];
                    sumSqrRight += pright[curBlock + j] * pright[curBlock + j];
                }
                meanLeft /= (d+1);
                meanRight /= (d+1);

                float cov = 0;
                for(int k = 0; k <= d; ++k){
                    int curBlock = (i - k) * stockSize;
                    cov += (pleft[curBlock + j] - meanLeft) * (pright[curBlock + j] - meanRight);
                }
                //cov /= (d+1);
                float xDiff2 = (sumSqrLeft - meanLeft*meanLeft*(d+1));
                float yDiff2 = (sumSqrRight - meanRight*meanRight*(d+1));
                if(xDiff2 < 0.000000001 || yDiff2 < 0.000000001)
                    pout[i * stockSize + j] = 1;
                else
                    pout[i * stockSize + j] = cov / (sqrtf(xDiff2) * sqrtf(yDiff2));
            }
        }
    }
    return pout;
}

const float* scale(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    for(int i = 0; i < historySize; ++i)
        if(pflag[i])
            _scale(pout + i * stockSize, pleft + i * stockSize, stockSize);
    return pout;
}

const float* decayLinear(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    memset(pout, 0, sizeof(float)*historySize*stockSize);
    int d = (int) roundf(coff) + 1;
    float allWeight = (d + 1) * d * 0.5;
    for(int i = d-1; i < historySize; i++){
        if(pflag[i]){
            for(int j = 0; j < d; j++){
                _addAndMul(pout + i * stockSize, pleft + (i - j) * stockSize, (d - j) / allWeight, stockSize);
            }
        }
    }
    return pout;
}

const float* tsMin(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    int d = (int)roundf(coff);
    _tsMinIndex(pout, pleft, historySize, stockSize, d);
    for(int i = 0; i < historySize; i++){
        if(pflag[i]){
            for(int j = 0; j < stockSize; j++){
                int minId = (int)pout[i * stockSize + j];
                pout[i * stockSize + j] = pleft[minId * stockSize + j];
            }
        }

    }
    return pout;
}

const float* tsMax(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    int d = (int)roundf(coff);
    _tsMaxIndex(pout, pleft, historySize, stockSize, d);
    for(int i = 0; i < historySize; i++){
        if(pflag[i]){
            for(int j = 0; j < stockSize; j++){
                int maxId = (int)pout[i * stockSize + j];
                pout[i * stockSize + j] = pleft[maxId * stockSize + j];
            }
        }

    }
    return pout;
}

const float* min(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if (pflag[i]) {
            _min(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* max(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _max(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* tsArgMin(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    int d = (int)roundf(coff);
    _tsMinIndex(pout, pleft, historySize, stockSize, d);
    for(int i = 0; i < historySize; i++){
        for(int j = 0; j < stockSize; j++){
            int minId = (int)pout[i * stockSize + j];
            pout[i * stockSize + j] = minId - (i - d);
        }
    }
    return pout;
}

const float* tsArgMax(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    int d = (int)roundf(coff);
    _tsMaxIndex(pout, pleft, historySize, stockSize, d);
    for(int i = 0; i < historySize; i++){
        for(int j = 0; j < stockSize; j++){
            int maxId = (int)pout[i * stockSize + j];
            pout[i * stockSize + j] = maxId - (i - d);
        }
    }
    return pout;
}

const float* sign(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++) {
        if (pflag[i]) {
            _sign((pout + i * stockSize), (pleft + i * stockSize), stockSize);
        }
    }
    return pout;
}

const float* abs(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _abs(pout + i * stockSize, pleft + i * stockSize, stockSize);
        }
    }

    return pout;
}

const float* log(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    float logmax = logf(0.0001);
    float logmin = logf(-0.0001);
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _log(pout + i * stockSize, pleft + i * stockSize, stockSize, logmin, logmax);
        }
    }
    return pout;
}

const float* signedPower(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _pow(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* signedPowerFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _pow(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* signedPowerTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _pow(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

const float* lessCond(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _lessCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* lessCondFrom(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _lessCond(pout + i * stockSize, coff, pleft + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* lessCondTo(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _lessCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

const float* elseCond(const float *pleft, const float *pright, float coff, size_t historySize, size_t stockSize,
                      bool* pflag, float *pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _elseCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* elseCondTo(const float *pleft, const float *pright, float coff, size_t historySize, size_t stockSize,
                        bool* pflag, float *pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _elseCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

const float* ifCond(const float *pleft, const float *pright, float coff, size_t historySize, size_t stockSize,
                    bool* pflag, float *pout){

    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _ifCond(pout + i * stockSize, pleft + i * stockSize, pright + i * stockSize, stockSize);
        }
    }
    return pout;
}

const float* ifCondTo(const float *pleft, const float *pright, float coff, size_t historySize, size_t stockSize,
                      bool* pflag, float *pout){
    for(int i = 0; i < historySize; i++){
        if (pflag[i]) {
            _ifCond(pout + i * stockSize, pleft + i * stockSize, coff, stockSize);
        }
    }
    return pout;
}

const float* indneutralize(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, bool* pflag, float* pout) {
    for(int i = 0; i < historySize; i++) {
        if(pflag[i]) {
            memcpy(pout + i * stockSize, pleft + i * stockSize, stockSize * sizeof(float));
        }
    }
    return pout;
}

#endif //ALPHATREE_ALPHA_H
