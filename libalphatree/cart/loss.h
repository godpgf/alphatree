//
// Created by godpgf on 18-3-15.
// 自定义损失函数
//

#ifndef ALPHATREE_LOSS_H
#define ALPHATREE_LOSS_H

#include <math.h>
#include "../base/iterator.h"

//对数损失函数
float binaryLogisticLoss(IBaseIterator<float>* target, IBaseIterator<float>* lastPred){
    float errSum = 0;
    int right_cnt[] = {0,0,0,0,0};
    int all_cnt[] = {0,0,0,0,0};
    while (target->isValid()){
        float y = **target;
        float ypred = **lastPred;
        errSum += y * logf((1.0f + expf(-ypred))) + (1.f - y) * logf(1.f + expf(ypred));
        ++*lastPred;
        ++*target;
        for(int j = 0; j < 5; ++j){
            float p = 1.0f / (1.0f + expf(-ypred));
            if((p - 0.5f) > j * 0.1f){
                if((y - 0.5f) > 0.001f)
                    right_cnt[j] += 1;
                all_cnt[j] += 1;
            }

        }

    }
    cout<<"test win percent:";
    for(int j = 0; j < 5; ++j){
        cout<<right_cnt[j]<<"/"<<all_cnt[j]<<"="<<float(right_cnt[j])/all_cnt[j]<<" ";
    }
    cout<<endl;
    lastPred->skip(0, false);
    target->skip(0, false);
    return errSum / target->size();
}

void binaryLogisticLossGrad(OBaseIterator<float>* g, OBaseIterator<float>* h, IBaseIterator<float>* target, IBaseIterator<float>* lastPred){

    int right_cnt = 0;
    while (target->isValid()){
        float y = **target;
        float ypred = **lastPred;
        g->setValue(expf(ypred) / (1 + expf(ypred)) - y);
        h->setValue(expf(ypred) / powf(1 + expf(ypred),2));
        ++*g;
        ++*h;
        ++*lastPred;
        ++*target;
        if((1.0f / (1.0f + expf(-ypred)) - 0.5f) * (y - 0.5f) > 0)
            right_cnt += 1;
    }
    cout<<"train win percent:"<<float(right_cnt)/target->size()<<endl;
    g->skip(0, false);
    h->skip(0, false);
    lastPred->skip(0, false);
    target->skip(0, false);
}

float regressionLoss(IBaseIterator<float>* target, IBaseIterator<float>* lastPred){
    float errSum = 0;
    while (target->isValid()){
        float y = **target;
        float ypred = **lastPred;
        errSum += powf(y - ypred, 2.0f);
        ++*lastPred;
        ++*target;
    }
    lastPred->skip(0, false);
    target->skip(0, false);
    return errSum / target->size();
}

void regressionLossGrad(OBaseIterator<float>* g, OBaseIterator<float>* h, IBaseIterator<float>* target, IBaseIterator<float>* lastPred){
    float errSum = 0;
    while (target->isValid()){
        float y = **target;
        float ypred = **lastPred;
        g->setValue((ypred - y) * 2);
        h->setValue(2);
        ++*g;
        ++*h;
        ++*lastPred;
        ++*target;
    }

    g->skip(0, false);
    h->skip(0, false);
    lastPred->skip(0, false);
    target->skip(0, false);
}


#endif //ALPHATREE_LOSS_H
