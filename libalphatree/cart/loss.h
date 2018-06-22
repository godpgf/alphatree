//
// Created by godpgf on 18-3-15.
// 自定义损失函数
//

#ifndef ALPHATREE_LOSS_H
#define ALPHATREE_LOSS_H

#include <math.h>
#include "../base/iterator.h"

//对数损失函数
void binaryLogisticLoss(OBaseIterator<float>* g, OBaseIterator<float>* h, IBaseIterator<float>* target, IBaseIterator<float>* lastPred){
    while (target->isValid()){
        float y = **target;
        float ypred = **lastPred;
        g->setValue(expf(ypred) / (1 + expf(ypred)) - y);
        h->setValue(expf(ypred) / powf(1 + expf(ypred),2));
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

void regressionLoss(OBaseIterator<float>* g, OBaseIterator<float>* h, IBaseIterator<float>* target, IBaseIterator<float>* lastPred){
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
