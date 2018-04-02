//
// Created by godpgf on 18-3-15.
//

#ifndef ALPHATREE_SPLIT_H
#define ALPHATREE_SPLIT_H

#include <float.h>
#include "bar.h"

struct SplitRes{
public:
    bool isSlpit(){
        return gain > 0;
    }

    float gain = {0};
    float splitValue = {0};
    float leftSum = {0};
    float rightSum = {0};
    float leftSqrSum = {0};
    float rightSqrSum = {0};
    float leftWeightSum = {0};
    float rightWeightSum = {0};
    float gl, gr, hl, hr;
};

inline float gain_(float gl, float gr, float hl, float hr, float gamma, float lambda) {
    return 0.5f * ((gl * gl) / (hl + lambda) + (gr * gr) / (hr + lambda) -
                   (gl + gr) * (gl + gr) / (hl + hr + lambda)) - gamma;
}

//分裂柱状图
SplitRes splitBars(SplitBar* curBars, int barSize, float startValue, float deltaStd, float gamma, float lambda){
    for (int i = 1; i < barSize; ++i) {
        curBars[i].g += curBars[i - 1].g;
        curBars[i].h += curBars[i - 1].h;
        curBars[i].dataSqrSum += curBars[i-1].dataSqrSum;
        curBars[i].dataSum += curBars[i-1].dataSum;
        curBars[i].weightSum += curBars[i-1].weightSum;
    }

    float gl, gr, hl, hr;
    SplitRes res;
    for (int i = 1; i < barSize; ++i) {
        gl = curBars[i - 1].g;
        gr = curBars[barSize - 1].g - gl;
        hl = curBars[i - 1].h;
        hr = curBars[barSize - 1].h - hl;
        float g = gain_(gl, gr, hl, hr, gamma, lambda);
        if (g > res.gain) {
            res.splitValue = (i - 1) * deltaStd + startValue;
            res.gain = g;
            res.leftSum = curBars[i-1].dataSum;
            res.rightSum = curBars[barSize-1].dataSum - res.leftSum;
            res.leftSqrSum = curBars[i-1].dataSqrSum;
            res.rightSqrSum = curBars[barSize-1].dataSqrSum - res.leftSqrSum;
            res.leftWeightSum = curBars[i-1].weightSum;
            res.rightWeightSum = curBars[barSize-1].weightSum - res.leftWeightSum;
            res.gl = gl;
            res.gr = gr;
            res.hl = hl;
            res.hr = hr;
        }
    }

    return res;
}

//测试代码,测试（对数损失函数）分裂的正确性--------------------------------------------------------------------
SplitRes testSplitValue2LogLoss(SplitBar* curBars, int barSize, float startValue, float deltaStd, IBaseIterator<float>* feature, IBaseIterator<float>* weight, IBaseIterator<float>* lastPred, IBaseIterator<float>* target, IBaseIterator<int>* flag, int targetFlag){
    float pAll = 0;
    float pAllCount = 0;
    float lossAll = 0;
    float* pLeft = new float[barSize];
    float* pLeftCount = new float[barSize];
    float* pRight = new float[barSize];
    float* pRightCount = new float[barSize];
    float* loss = new float[barSize];
    memset(pLeft, 0, barSize * sizeof(float));
    memset(pRight, 0, barSize * sizeof(float));
    memset(pLeftCount, 0, barSize * sizeof(float));
    memset(pRightCount, 0, barSize * sizeof(float));
    memset(loss, 0, barSize * sizeof(float));

    //先计算每个分裂的预测概率
    while (feature->isValid()){
        if(**flag == targetFlag){
            pAllCount += (**weight);
            pAll += (**weight) * ((**target) - (**lastPred));
            for(int i = 1; i < barSize; ++i){
                float sv = (i - 1) * deltaStd + startValue;
                if((**feature) < sv){
                    pLeftCount[i] += (**weight);
                    pLeft[i] += (**weight) * ((**target) - (**lastPred));
                } else {
                    pRightCount[i] += (**weight);
                    pRight[i] += (**weight) * ((**target) - (**lastPred));
                }
            }
        }
        ++*feature;
        ++*weight;
        ++*lastPred;
        ++*target;
        ++*flag;
    }
    pAll /= pAllCount;
    for(int i = 1; i < barSize; ++i){
        if(pLeftCount[i] > 0)
            pLeft[i] /= pLeftCount[i];
        if(pRightCount[i] > 0)
            pRight[i] /= pRightCount[i];
    }
    feature->skip(0, false);
    weight->skip(0, false);
    lastPred->skip(0, false);
    target->skip(0, false);
    flag->skip(0, false);


    //开始计算损失
    int minLossBar = -1;
    float minLoss = FLT_MAX;

    while (feature->isValid()){
        if(**flag == targetFlag){
            lossAll += (**target) * logf(pAll) + (1 - **target) * logf(1 - pAll);

            for(int i = 1; i < barSize; ++i){
                float sv = (i - 1) * deltaStd + startValue;
                if((**feature) < sv){
                        loss[i] += (**target) * logf(pLeft[i]) + (1 - **target) * logf(1 - pLeft[i]);
                    } else {
                        loss[i] += (**target) * logf(pRight[i]) + (1 - **target) * logf(1 - pRight[i]);
                    }
            }
        }
            ++*feature;
            ++*weight;
            ++*lastPred;
            ++*target;
            ++*flag;
    }
    if(pAllCount > 0)
        lossAll *= (-1/pAllCount);

    for(int i = 1; i < barSize; ++i)
        if(pLeftCount[i] + pRightCount[i] > 0){
            loss[i] *= (-1/(pLeftCount[i] + pRightCount[i]));
            if(loss[i] < minLoss){
                minLoss = loss[i];
                minLossBar = i;
            }
            cout<<startValue + (i - 1) * deltaStd<<":"<<loss[i]<<"/"<<lossAll<<" ";
        }
    cout<<endl;
    feature->skip(0, false);
    weight->skip(0, false);
    lastPred->skip(0, false);
    target->skip(0, false);
    flag->skip(0, false);


    delete []pLeft;
    delete []pLeftCount;
    delete []pRight;
    delete []pRightCount;

    SplitRes res;
    if(minLossBar >= 0)
        res.splitValue = (minLossBar - 1) * deltaStd + startValue;
    return res;
}

#endif //ALPHATREE_SPLIT_H
