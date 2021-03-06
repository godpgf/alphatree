//
// Created by godpgf on 17-12-8.
//

#ifndef ALPHATREE_HISTOGRAM_H
#define ALPHATREE_HISTOGRAM_H

#include "normal.h"
#include <stddef.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <algorithm>

struct HistogrmBar {
    float avgValue;
    float stdValue;
    size_t dataNum;
    float g;
    float h;
    float startValue;
};
/*
#define RANGE_CHECK(a, b, c) fabs(a-b)/fmax(a,b) < c

size_t
meargeBars(const float *x, const int *nodeId, const float *y, size_t cmpNodeId, size_t dataSize, HistogrmBar *outBars,
           size_t maxBarSize, float mergeBarPercent = 0) {
    for (size_t i = 1; i < maxBarSize; ++i) {
        if (!RANGE_CHECK(outBars[i].avgValue, outBars[i - 1].avgValue, mergeBarPercent) ||
            !RANGE_CHECK(outBars[i].stdValue, outBars[i - 1].stdValue, mergeBarPercent)) {
            outBars[i - 1].avgValue =
                    (outBars[i - 1].avgValue * outBars[i - 1].dataNum + outBars[i].avgValue * outBars[i].dataNum) /
                    (outBars[i - 1].dataNum + outBars[i].dataNum);
            outBars[i - 1].dataNum += outBars[i].dataNum;
            outBars[i - 1].g += outBars[i].g;
            outBars[i - 1].h += outBars[i].h;
            for (size_t j = i + 1; j < maxBarSize; ++j)
                outBars[j - 1] = outBars[j];
            --maxBarSize;
            //refresh std
            outBars[i - 1].stdValue = 0;
            for (size_t j = 0; j < dataSize; ++j) {
                if (nodeId[j] == cmpNodeId) {
                    if (x[j] >= outBars[i - 1].startValue && (i >= maxBarSize || x[j] < outBars[i].startValue)) {
                        outBars[i - 1].stdValue += (x[j] - outBars[i - 1].avgValue) * (x[j] - outBars[i - 1].avgValue);
                    }
                }
            }
            outBars[i - 1].stdValue /= outBars[i - 1].dataNum;
            outBars[i - 1].stdValue = sqrtf(outBars[i - 1].stdValue);
            return meargeBars(x, nodeId, y, cmpNodeId, dataSize, outBars, maxBarSize, mergeBarPercent);
        }
    }
    return maxBarSize;
}*/

size_t
createHistogrmBars(const float *x, const int *nodeId, const float *y, size_t cmpNodeId, const float *g, const float *h,
                   const float *w, size_t dataSize,
                   HistogrmBar *outBars, size_t maxBarSize) {
    float stdScale = normsinv(1.0 - 1.0 / maxBarSize);
    float avgValue = 0;
    float stdValue = 0;
    size_t dataCount = 0;
    for (size_t i = 0; i < dataSize; ++i) {
        if (nodeId[i] == cmpNodeId) {
            ++dataCount;
            avgValue += x[i];
        }
    }
    avgValue /= dataCount;
    for (size_t i = 0; i < dataSize; ++i) {
        if (nodeId[i] == cmpNodeId) {
            stdValue += (x[i] - avgValue) * (x[i] - avgValue);
        }
    }
    stdValue /= dataCount;
    stdValue = sqrtf(stdValue);
    float startValue = avgValue - stdValue * stdScale;
    float deltaStd = stdValue * stdScale / (0.5f * (maxBarSize - 2));

    for (size_t i = 0; i < maxBarSize; ++i) {
        outBars[i].avgValue = 0;
        outBars[i].stdValue = 0;
        outBars[i].dataNum = 0;
        outBars[i].g = 0;
        outBars[i].h = 0;
        outBars[i].startValue = (i == 0 ? -FLT_MAX : startValue + (i - 1) * deltaStd);
    }

    for (size_t i = 0; i < dataSize; ++i) {
        if (nodeId[i] == cmpNodeId) {
            int index = x[i] < startValue ? 0 : std::min((size_t) ((x[i] - startValue) / deltaStd) + 1, maxBarSize - 1);
            outBars[index].avgValue += x[i];
            outBars[index].g += g[i] * w[i];
            outBars[index].h += h[i] * w[i];
            ++outBars[index].dataNum;
        }
    }

    for (size_t i = 0; i < maxBarSize; ++i)
        outBars[i].avgValue /= outBars[i].dataNum;

    for (size_t i = 0; i < dataSize; ++i) {
        if (nodeId[i] & cmpNodeId) {
            int index = x[i] < startValue ? 0 : std::min((size_t) ((x[i] - startValue) / deltaStd) + 1, maxBarSize - 1);
            outBars[index].stdValue += (x[i] - outBars[index].avgValue) * (x[i] - outBars[index].avgValue);
        }
    }
    for (size_t i = 0; i < maxBarSize; ++i) {
        outBars[i].stdValue /= outBars[i].dataNum;
        outBars[i].stdValue = sqrtf(outBars[i].stdValue);
    }

    return maxBarSize;
    /*cout<<"look out bars:\n";
    for(size_t i = 0; i < maxBarSize; ++i){
        cout<<outBars[i].dataNum<<":"<<outBars[i].avgValue<<" ";
    }
    cout<<endl;

    return meargeBars(x, nodeId, y, cmpNodeId, dataSize, outBars, maxBarSize, mergeBarPercent);*/
}

#endif //ALPHATREE_HISTOGRAM_H
