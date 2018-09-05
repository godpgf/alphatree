//
// Created by godpgf on 18-9-4.
//

#ifndef ALPHATREE_BASEBI_H
#define ALPHATREE_BASEBI_H

#include "../base/normal.h"
#include "string.h"
#include <iostream>
using namespace std;


void callstsq_(const float *x, const float *y, int len, float &beta, float &alpha) {
    float sumx = 0.f;
    float sumy = 0.f;
    float sumxy = 0.f;
    float sumxx = 0.f;
    for (int j = 0; j < len; ++j) {
        sumx += x[j];
        sumy += y[j];
        sumxy += x[j] * y[j];
        sumxx += x[j] * x[j];
    }
    float tmp = (len * sumxx - sumx * sumx);
    beta = abs(tmp) < 0.0001f ? 0 : (len * sumxy - sumx * sumy) / tmp;
    alpha = abs(tmp) < 0.0001f ? 0 : sumy / len - beta * sumx / len;
}

float correlation_(const float *a, const float *b, int len) {
    //计算当前股票的均值和方差
    double meanLeft = 0;
    double meanRight = 0;
    double sumSqrLeft = 0;
    double sumSqrRight = 0;
    for (int j = 0; j < len; ++j) {
        meanLeft += a[j];
        sumSqrLeft += a[j] * a[j];
        meanRight += b[j];
        sumSqrRight += b[j] * b[j];
    }
    meanLeft /= len;
    meanRight /= len;

    float cov = 0;
    for (int k = 0; k < len; ++k) {
        cov += (a[k] - meanLeft) * (b[k] - meanRight);
    }

    float xDiff2 = (sumSqrLeft - meanLeft * meanLeft * len);
    float yDiff2 = (sumSqrRight - meanRight * meanRight * len);

    if (isnormal(cov) && isnormal(xDiff2) && isnormal(yDiff2)) {
        float corr = cov / sqrtf(xDiff2) / sqrtf(yDiff2);
        if (isnormal(corr)) {
            return fmaxf(fminf(corr, 1.0f), -1.0f);
        }

        return 1;
    }
    return 1;
}

void quickSort_(const float *src, int *index, int left, int right) {
    if (left >= right)
        return;
    int key = index[left];

    int low = left;
    int high = right;
    while (low < high) {
        //while (low < high && (isnan(src[(int)index[high]]) || src[(int)index[high]] > src[key])){
        while (low < high && src[(int) index[high]] > src[key]) {
            --high;
        }
        if (low < high)
            index[low++] = index[high];
        else
            break;

        //while (low < high && (isnan(src[(int)index[low]]) || src[(int)index[low]] <= src[key])){
        while (low < high && src[(int) index[low]] <= src[key]) {
            ++low;
        }
        if (low < high)
            index[high--] = index[low];
    }
    index[low] = (float) key;

    quickSort_(src, index, left, low - 1);
    quickSort_(src, index, low + 1, right);
}

//给每个时间段的特征排序,排序后数据保存在缓存中
void sortFeature_(const float* cache, int* index, size_t len, size_t sampleTime){
    for(int splitId = 0; splitId < sampleTime; ++splitId){
        int preId = (int)(splitId * len / (float)sampleTime);
        int nextId = (int)((splitId + 1) * len / (float)sampleTime);
        quickSort_(cache, index, preId, nextId-1);
//            for(int j = preId; j < nextId; ++j){
//                cout<<cache[index[j]]<<" ";
//            }
//            cout<<endl;
    }
}

void calReturnsRatioAvgAndStd_(const float* returns, const int* index, size_t len, size_t sampleTime, float support, float* avg, float* std){
    memset(avg, 0, sampleTime * sizeof(float));
    memset(std, 0, sampleTime * sizeof(float));
    for(size_t splitId = 0; splitId < sampleTime; ++splitId){
        size_t nextId = (size_t)((splitId + 1) * len / (float)sampleTime);
        size_t preId = (size_t)(splitId * len / (float)sampleTime);
        size_t supportNextId = preId + (nextId - preId) * support * 0.5f;
        for(int j = preId; j < supportNextId; ++j){
            int lid = index[j];
            int rid = index[nextId - 1 - (j - preId)];
            float v = (returns[rid] + 1.f) / (returns[lid] + 1.f);
//                cout<<v<<"="<<(cache[rid] + 1.f)<<"/"<<(cache[lid] + 1.f)<<endl;
//                v = cache[lid] + cache[rid];
            avg[splitId] += v;
            std[splitId] += v * v;
        }
        avg[splitId] /= (supportNextId - preId);
        std[splitId] = sqrtf(std[splitId] / (supportNextId - preId) - avg[splitId] * avg[splitId]);
        //样本均值的标准差=样本的标准差 / sqrt(样本数量)
        std[splitId] /= sqrtf(supportNextId - preId);
//        cout<<avg[splitId]<<" "<<std[splitId]<<endl;
    }
}

bool getIsDirectlyPropor(const float* feature, const float* returns, int* index, size_t len){
    quickSort_(feature, index, 0, len-1);
    float leftReturns = 0, rightReturns = 0;
    size_t midSize = (len >> 1);
    for(int i = 0; i < midSize; ++i){
        leftReturns += returns[index[i]];
        rightReturns += returns[index[i+midSize]];
    }
//        cout<<leftReturns<<" "<<rightReturns<<endl;
    bool isDirectlyPropor = (rightReturns > leftReturns);
    return isDirectlyPropor;
}

void calFeatureAvg_(const float* cache, const int* index, size_t len, size_t sampleTime, float support, float* featureAvg){
    memset(featureAvg, 0, sampleTime * sizeof(float));
    for(size_t splitId = 0; splitId < sampleTime; ++splitId){
        size_t nextId = (size_t)((splitId + 1) * len / (float)sampleTime);
        size_t preId = (size_t)(splitId * len / (float)sampleTime);
        int supportNextId = preId + (nextId - preId) * support * 0.5f;
        for(int j = preId; j < supportNextId; ++j){
            int lid = index[j];
            int rid = index[nextId - 1 - (j - preId)];
            featureAvg[splitId] += cache[lid];
            featureAvg[splitId] += cache[rid];
        }
        featureAvg[splitId] /= 2 * (supportNextId - preId);
    }
}

void calR2Seq_(const float* xCache, const float* xAvg, const float* yCache, const float* yAvg, const int* index, size_t len, size_t sampleTime, float support, float* r2List){
    cout<<"r2:";
    for(size_t splitId = 0; splitId < sampleTime; ++splitId){
        size_t preId = (size_t)(splitId * len / (float)sampleTime);
        size_t nextId = (size_t)((splitId + 1) * len / (float)sampleTime);
        size_t supportNextId = preId + (nextId - preId) * support * 0.5f;
        float SSR = 0;
        float varX = 0;
        float varY = 0;
        for(int j = preId; j < supportNextId; ++j){
            int lid = index[j];
            int rid = index[nextId - 1 - (j - preId)];
            float x = xCache[lid] - xAvg[splitId];
            float y = yCache[lid] - yAvg[splitId];
            SSR += x * y;
            varX += x * x;
            varY += y * y;
            x = xCache[rid] - xAvg[splitId];
            y = yCache[rid] - yAvg[splitId];
            SSR += x * y;
            varX += x * x;
            varY += y * y;
        }
        float SST = sqrtf(varX * varY);
        r2List[splitId] = SSR / SST;
        cout<<r2List[splitId]<<" ";
    }
    cout<<endl;
}

void calAutoregressive_(const float* timeSeq, const float *data, int len, float stdScale, float &minValue, float &maxValue) {
    float alpha, beta;
    callstsq_(timeSeq, data, len, beta, alpha);

    float stdL = 0, stdR = 0;
    int cntL = 0, cntR = 0;
    for (int i = 0; i < len; ++i) {
        float err = data[i] - (i * beta + alpha);
        if(err >= 0){
            stdR += err * err;
            ++cntR;
        }else{
            stdL += err * err;
            ++cntL;
        }
    }
    stdL = sqrtf(stdL / cntL);
    stdR = sqrtf(stdR / cntR);
    float value = len * beta + alpha;
    minValue = value - stdL * stdScale;
    maxValue = value + stdR * stdScale;
}

void calDiscriminationSeq_(const float* returns, const int* index, size_t len, size_t sampleTime, float support, float expectReturn, float* discList){
    cout<<expectReturn<<":";
    for(size_t splitId = 0; splitId < sampleTime; ++splitId){
        size_t preId = (size_t)(splitId * len / (float)sampleTime);
        size_t nextId = (size_t)((splitId + 1) * len / (float)sampleTime);
        size_t supportNextId = preId + (nextId - preId) * support * 0.5f;
        int leftCnt = 0, rightCnt = 0;
        for(int j = preId; j < supportNextId; ++j){
            int lid = index[j];
            int rid = index[nextId - 1 - (j - preId)];
            if(returns[lid] > expectReturn)
                ++leftCnt;
            if(returns[rid] > expectReturn)
                ++rightCnt;

        }
        discList[splitId] = (leftCnt == 0) ? 1 : rightCnt / (float)leftCnt;
        cout<<rightCnt<<"/"<<leftCnt<<" ";
    }
    cout<<endl;
}
#endif //ALPHATREE_BASEBI_H
