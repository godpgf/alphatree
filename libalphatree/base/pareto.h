//
// Created by godpgf on 17-12-11.
//

#ifndef ALPHATREE_PARETO_H
#define ALPHATREE_PARETO_H

#include<math.h>
#include<iostream>

//对于一个随机变量X来说,xm是X能取到的最小值,求X>x的概率
float pr(float x, float xm, float alpha) {
    if (x < xm)
        return 1;
    return powf(xm / x, alpha);
}

//ranking后ruleWeight比例的人拥有1-ruleWeight的权重，按照这个规则分配权重
void distributionWeightPr(size_t *ranking, size_t size, float *outWeight, float ruleWeight = 0.382f) {
    for (size_t i = 1; i < size; ++i)
        ranking[i] += ranking[i - 1];
    size_t all_num = ranking[size - 1];
    float x = ranking[size - 1] * ruleWeight;
    //换底公式log(1/x,0.2)=ln(0.2) / ln(1/x)

    float alpha = logf(ruleWeight) / logf(1 / x);

    for (size_t i = size - 1; i > 0; --i)
        ranking[i] -= ranking[i - 1];

    float lastWeight = 0;
    float allWeight = 0;
    size_t sum = 0;
    for (size_t i = size - 1; i > 0; --i) {
        if(ranking[i] > 0){
            sum += ranking[i];
            float weight = max(1.0 - pr(sum, 1, alpha),0.0);
            outWeight[i] = (weight - lastWeight) * all_num;
            allWeight += (weight - lastWeight);
            cout<<"num:"<<ranking[i]<<" "<<"weight:"<<outWeight[i]<<"w/n:"<<outWeight[i]/ranking[i]<<endl;
            outWeight[i] /= ranking[i];
            lastWeight = weight;
        } else {
            outWeight[i] = 0;
        }
    }
    if(ranking[0] > 0)
        outWeight[0] = 1 - lastWeight;
    //修正权重
    if(allWeight > 0){
        for(size_t i = 0; i < size; ++ i)
            outWeight[i] /= allWeight;
    }
}

#endif //ALPHATREE_PARETO_H
