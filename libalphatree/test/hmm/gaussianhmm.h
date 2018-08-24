//
// Created by 严宇 on 2018/4/30.
//

#ifndef ALPHATREE_GAUSSIANHMM_H
#define ALPHATREE_GAUSSIANHMM_H

#include "hmmbase.h"
#include <math.h>

class GaussianHMM : public BaseHMM{
public:
    GaussianHMM(unsigned n, unsigned t):BaseHMM(n, 2, t){

    }

protected:
    virtual float getOutProbability(unsigned n, unsigned t, DMatrix<float>* o){
        float mu = (*model_.getB())(n, 0);
        float delta = (*model_.getB())(n, 1);
        return _pi / delta * expf(-0.5 * powf(o->get(t,0) - mu, 2) / powf(delta, 2));
    }
    virtual void init(DMatrix<float>* o){
        model_.getA()->init(1.f / model_.getA()->getDim1());
        //model_.getB()->init(1.f / model_.getB()->getDim2());
        model_.getPi()->init(1.f / model_.getA()->getDim1());
        float mu = 0;
        float mu_2 = 0;
        for(unsigned t = 0; t < o->getDim1(); ++t){
            mu += o->get(t,0);
            mu_2 += powf(o->get(t,0), 2);
        }
        mu /= o->getDim1();
        mu_2 /= o->getDim1();
        float delta = sqrtf(mu_2 - powf(mu, 2));
        for (unsigned i = 0; i < model_.getB()->getDim1(); ++i) {
            (*model_.getB())(i, 0) = mu;
            (*model_.getB())(i, 1) = delta;
        }
    }
    virtual void updateB(DMatrix<float>* gamma, DMatrix<float>* o, DMatrix<float>* b){
        for(unsigned i = 0; i < b->getDim1(); ++i){
            float sumAlphaBetaO = 0;
            float sumAlphaBeta = 0;
            for(unsigned t = 0; t < o->getDim1(); ++t){
                sumAlphaBeta += alpha_(t, i) * beta_(t, i);
                sumAlphaBetaO += alpha_(t, i) * beta_(t, i) * o->get(t,0);
            }
            float mu = sumAlphaBetaO / sumAlphaBeta;

            float sumO_mu = 0;
            for(unsigned t = 0; t < o->getDim1(); ++t){
                sumO_mu += alpha_(t, i) * beta_(t, i) * powf(o->get(t,0) - mu, 2);
            }
            float delta = sqrtf(sumO_mu / sumAlphaBeta);

            (*model_.getB())(i, 0) = mu;
            (*model_.getB())(i, 1) = delta;
        }
    }
    static const float _pi;
};

const float GaussianHMM::_pi = 1.f / sqrtf(M_PI * 2);

#endif //ALPHATREE_GAUSSIANHMM_H
