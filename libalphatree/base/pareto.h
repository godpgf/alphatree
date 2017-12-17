//
// Created by godpgf on 17-12-11.
//

#ifndef ALPHATREE_PARETO_H
#define ALPHATREE_PARETO_H
#include<math.h>

//对于一个随机变量X来说,xm是X能取到的最小值,求X>x的概率
float pr(float x, float xm, float alpha){
    if(x < xm)
        return 1;
    return powf(xm/x,alpha);
}

//ranking后ruleWeight比例的人拥有1-ruleWeight的权重，按照这个规则分配权重
void distributionWeightPr(size_t* ranking, size_t size, float* outWeight, float ruleWeight = 0.2f){
    for(size_t i = 1; i < size; ++i)
	ranking[i] += ranking[i-1];
    float x = ranking[size-1] * ruleWeight;
    //换底公式log(1/x,0.2)=ln(1/x)/ln(0.2)
    float alpha = logf(1/x)/logf(0.2);
    
    for(size_t i = size - 1; i > 0; --i)
        ranking[i] -= ranking[i-1];

    float lastWeight = 0;
    size_t sum = 0;
    for(size_t i = size - 1; i > 0; --i){
	sum += ranking[i];
        float weight = 1 - pr(sum, 1, alpha);
        outWeight[i] = weight - lastWeight;
	lastWeight = weight;
    }
    outWeight[0] = 1 - lastWeight;
}

#endif //ALPHATREE_PARETO_H
