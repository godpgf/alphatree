//
// Created by godpgf on 18-6-11.
//

#ifndef ALPHATREE_BAG_H
#define ALPHATREE_BAG_H

#include <string.h>
#include <math.h>
#include <float.h>
#include "normal.h"
#include "iterator.h"

struct DataBag{
    //数据的和
    float dataSum;
    //数据的平方的和
    float dataSqrSum;
    //权重的和
    float weightSum;
    //max value of data in this bag
    float maxValue;
};

void getBags(IBaseIterator<float>* feature, float* bags, int bagNum){
    //先计算需要统计柱状图的数据的均值和标准差
    double sum = 0;
    double sumSqr = 0;
    double dataCount = feature->size();
    while (feature->isValid()){
        float value = (**feature);
        sum += value / dataCount;
        sumSqr += value * value / dataCount;
        feature->skip(1);
    }
    feature->skip(0, false);

    float avg = sum;
    float std = sqrtf(sumSqr  - avg * avg);

    if(std == 0 || isnan(std)){
        for(int i = 0; i < bagNum; ++i)
            bags[i] = 0;
        return;
    }

    //初始化柱状图每个柱子的范围
    float stdScale =  normsinv(1.0 - 1.0 / (bagNum + 1));
    float startValue = avg - std * stdScale;
    float deltaStd = std * stdScale * 2 / fmaxf((bagNum - 1), 0.0001f);
    bags[0] = startValue;
    for(int i = 1; i < bagNum; ++i)
        bags[i] = startValue + deltaStd * i;
}

DataBag* createBags(IBaseIterator<float>* feature, int* skip, int skipLen, float minWeight, int maxBagNum = 8){
    if(maxBagNum == 0)
        return nullptr;
    //先计算需要统计柱状图的数据的均值和标准差
    double sum = 0;
    double sumSqr = 0;

    double dataCount = skipLen;

    for(int i = 0; i < skipLen; ++i){
        feature->skip(skip[i]);
        //在这里先做除法是为了防止数据溢出
        float value = (**feature);
        sum += value / dataCount;
        sumSqr += value * value / dataCount;
    }

    feature->skip(0, false);

    float avg = sum;
    float std = sqrtf(sumSqr  - avg * avg);

    if(std == 0 || isnan(std))
        return nullptr;
    //初始化柱状图每个柱子的范围
    float stdScale =  normsinv(1.0 - 1.0 / maxBagNum);
    float startValue = avg - std * stdScale;
    float deltaStd = std * stdScale / fmaxf(0.5f * (maxBagNum - 2), 0.0001f);

    //create data bag
    DataBag* bags = new DataBag[maxBagNum];
    memset(bags, 0, maxBagNum * sizeof(DataBag));
    bags[0].maxValue = startValue;
    bags[maxBagNum-1].maxValue = FLT_MAX;
    for(int i = 1; i < maxBagNum - 1; ++i){
        bags[i].maxValue = startValue + i * deltaStd;
    }

    for(int i = 0; i < skipLen; ++i){
        feature->skip(skip[i]);

        int index = (**feature) < startValue ? 0 : std::min((int)(((**feature) - startValue) / deltaStd) + 1, maxBagNum - 1);
        bags[index].dataSum += (**feature);
        bags[index].dataSqrSum += powf((**feature), 2);
        bags[index].weightSum += 1;
    }

    int curIndex = 0;
    int nextIndex = 1;
    while (curIndex < maxBagNum){
        while (bags[curIndex].weightSum < minWeight && nextIndex < maxBagNum){
            bags[curIndex].dataSum += bags[nextIndex].dataSum;
            bags[curIndex].dataSqrSum += bags[nextIndex].dataSqrSum;
            bags[curIndex].weightSum += bags[nextIndex].weightSum;
            bags[curIndex].maxValue = bags[nextIndex].maxValue;
            ++nextIndex;
        }
        ++curIndex;
        int leftIndex = curIndex;
        if(leftIndex != nextIndex){
            while (nextIndex < maxBagNum){
                bags[leftIndex].dataSum = bags[nextIndex].dataSum;
                bags[leftIndex].dataSqrSum = bags[nextIndex].dataSqrSum;
                bags[leftIndex].weightSum = bags[nextIndex].weightSum;
                bags[leftIndex].maxValue = bags[nextIndex].maxValue;
                ++nextIndex;
                ++leftIndex;
            }
            maxBagNum = leftIndex;
        }
        nextIndex = curIndex+1;
    }
    if(maxBagNum > 1 && bags[maxBagNum-1].weightSum < minWeight){
        --maxBagNum;
        bags[maxBagNum - 1].dataSum += bags[maxBagNum].dataSum;
        bags[maxBagNum - 1].dataSqrSum += bags[maxBagNum].dataSqrSum;
        bags[maxBagNum - 1].weightSum += bags[maxBagNum].weightSum;
        bags[maxBagNum - 1].maxValue = bags[maxBagNum].maxValue;
    }

    feature->skip(0, false);
    return bags;
}

void destroyBags(DataBag* bags){
    if(bags)
        delete[] bags;
}


#endif //ALPHATREE_BAG_H
