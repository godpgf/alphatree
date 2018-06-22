//
// Created by godpgf on 18-3-15.
//

#ifndef ALPHATREE_ACT_H
#define ALPHATREE_ACT_H

#include <math.h>
#include "../base/iterator.h"

void binaryLogisticAct(IOBaseIterator<float>* pred){
    while (pred->isValid()){
        pred->setValue( 1.0f / (1.0f + expf(-(**pred))) );
        ++*pred;
    }
    pred->skip(0, false);
}

void regressionAct(IOBaseIterator<float>* pred){

}

#endif //ALPHATREE_ACT_H
