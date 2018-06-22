//
// Created by godpgf on 18-3-15.
//

#ifndef ALPHATREE_BAR_H
#define ALPHATREE_BAR_H

#include <string.h>
#include <math.h>
#include "../base/normal.h"
#include "../base/iterator.h"

struct SplitBar{
    //数据的和
    float dataSum;
    //数据的平方的和
    float dataSqrSum;
    //权重的和
    float weightSum;
    float g;
    float h;
};

class SplitBarList{
public:
    void initialize(int barSize){
        if(barSize > maxBarSize){
            clean();
            maxBarSize = barSize;
            bars = new SplitBar[maxBarSize];
        }
    }
    virtual ~SplitBarList(){
        clean();
    }
    void clean(){
        if(bars)
            delete []bars;
        maxBarSize = 0;
    }
    SplitBar* bars = {nullptr};
    float startValue;
    float deltaStd;
    int maxBarSize = {0};
};

float getWeightSum_(IBaseIterator<float>* weight, int* skip, int skipLen){
    float weightSum = 0;
    for(int i = 0; i < skipLen; ++i){
        weight->skip(skip[i]);
        weightSum += **weight;
    }
    weight->skip(0, false);
    return weightSum;
}

void fillBars_(SplitBar* curBars, int barSize, IBaseIterator<float>* feature, IBaseIterator<float>* weight, IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, int skipLen, float startValue, float deltaStd){
    memset(curBars, 0, barSize * sizeof(SplitBar));

    for(int i = 0; i < skipLen; ++i){
        feature->skip(skip[i]);
        g->skip(skip[i]);
        h->skip(skip[i]);
        weight->skip(skip[i]);

        int index = (**feature) < startValue ? 0 : std::min((int)(((**feature) - startValue) / deltaStd) + 1, barSize - 1);
        //cout<<index<<" "<<(*feature)<<" "<<deltaStd<<" "<<startValue<<endl;
        curBars[index].dataSum += (**feature) * (**weight);
        curBars[index].dataSqrSum += powf((**feature), 2) * (**weight);
        curBars[index].weightSum += (**weight);
        curBars[index].g += (**g) * (**weight);
        curBars[index].h += (**h) * (**weight);
    }

    feature->skip(0, false);
    g->skip(0, false);
    h->skip(0, false);
    weight->skip(0, false);
}

void fillBars_(SplitBar* curBars, int barSize, IBaseIterator<float>* feature, IBaseIterator<float>* weight, IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, int skipLen, IBaseIterator<float>* cmp, float cmpValue, bool isLeft, float startValue, float deltaStd){
    memset(curBars, 0, barSize * sizeof(SplitBar));

    for(int i = 0; i < skipLen; ++i){
        feature->skip(skip[i]);
        weight->skip(skip[i]);
        g->skip(skip[i]);
        h->skip(skip[i]);
        cmp->skip(skip[i]);

        if((**cmp < cmpValue) == isLeft){
            int index = (**feature) < startValue ? 0 : std::min((int)(((**feature) - startValue) / deltaStd) + 1, barSize - 1);
            //cout<<index<<" "<<(*feature)<<" "<<deltaStd<<" "<<startValue<<endl;
            curBars[index].dataSum += (**feature) * (**weight);
            curBars[index].dataSqrSum += powf((**feature), 2) * (**weight);
            curBars[index].weightSum += (**weight);
            curBars[index].g += (**g) * (**weight);
            curBars[index].h += (**h) * (**weight);
        }
    }

    feature->skip(0, false);
    g->skip(0, false);
    h->skip(0, false);
    weight->skip(0, false);
    cmp->skip(0, false);
}

/*
 * 加权均值：x*=(w1*x1+w2*x2+...+wn*xn)/(w1+w2+...+wn)
 * 加权方差:(w1*(x1-x*)^2+w2*(x2-x*)^2+...+wn*(xn-x*)^2)/(w1+w2+...+wn)
 *         (w1*x1^2+w2*x2^2+...+wn*xn^2)/(w1+w2+...+wn)-2x*(w1*x1+w2*x2+...+wn*xn)/(w1+w2+...+wn)+x*^2
 *         (w1*x1^2+w2*x2^2+...+wn*xn^2)/(w1+w2+...+wn)-x*^2
 */
bool fillBars(IBaseIterator<float>* feature, IBaseIterator<float>* weight, IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, int skipLen, SplitBar* curBars, int barSize, float& startValue, float& deltaStd){
    //先计算需要统计柱状图的数据的均值和标准差
    double sum = 0;
    double sumSqr = 0;

    double dataCount = getWeightSum_(weight, skip, skipLen);

    for(int i = 0; i < skipLen; ++i){
        feature->skip(skip[i]);
        weight->skip(skip[i]);
        //在这里先做除法是为了防止数据溢出
        float value = (**feature);
        sum += value * (**weight) / dataCount;
        sumSqr += value * value * (**weight) / dataCount;
    }

    feature->skip(0, false);
    weight->skip(0, false);

    float avg = sum;
    float std = sqrtf(sumSqr  - avg * avg);
    if(std == 0 || isnan(std))
        return false;
    //初始化柱状图每个柱子的范围
    float stdScale =  normsinv(1.0 - 1.0 / barSize);
    startValue = avg - std * stdScale;
    deltaStd = std * stdScale / (0.5f * (barSize - 2));
    fillBars_(curBars, barSize, feature, weight, g, h, skip, skipLen, startValue, deltaStd);
    return true;
}

bool fillBars(IBaseIterator<float>* feature, IBaseIterator<float>* weight, IBaseIterator<float>* g, IBaseIterator<float>* h, int* skip, int skipLen, IBaseIterator<float>* cmp, float cmpValue, bool isLeft, SplitBar* curBars, int barSize, float& startValue, float& deltaStd){
    //先计算需要统计柱状图的数据的均值和标准差
    double sum = 0;
    double sumSqr = 0;
    double dataCount = getWeightSum_(weight, skip, skipLen);

    for(int i = 0; i < skipLen; ++i){
        feature->skip(skip[i]);
        weight->skip(skip[i]);
        cmp->skip(skip[i]);
        if((**cmp < cmpValue) == isLeft){
            //在这里先做除法是为了防止数据溢出
            sum += ((**feature) * (**weight)) / dataCount;
            sumSqr += powf((**feature), 2) * (**weight) / dataCount;
        }
    }

    feature->skip(0, false);
    weight->skip(0, false);
    cmp->skip(0, false);

    float avg = sum;
    float std = sqrtf(sumSqr  - avg * avg);
    if(std == 0 || isnan(std))
        return false;
    //初始化柱状图每个柱子的范围
    float stdScale =  normsinv(1.0 - 1.0 / barSize);
    startValue = avg - std * stdScale;
    deltaStd = std * stdScale / (0.5f * (barSize - 2));
    fillBars_(curBars, barSize, feature, weight, g, h, skip, skipLen, cmp, cmpValue, isLeft, startValue, deltaStd);
    return true;
}

#endif //ALPHATREE_BAR_H
