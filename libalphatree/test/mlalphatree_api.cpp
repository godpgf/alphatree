//
// Created by godpgf on 18-6-6.
//

#include "alpharft.h"
#include "alphagbdf.h"
#include "../alphagbdt.h"
#include <iostream>

using namespace std;

extern "C"
{

void initializeAlphaRFT(const char* alphatreeList, int alphatreeNum, int threadNum){
    AlphaRFT::initialize(AlphaForest::getAlphaforest(), alphatreeList, alphatreeNum, threadNum);
}

void releaseAlphaRFT(){
    AlphaRFT::release();
}

int testReadRFT(int daybefore, int sampleSize, const char* target, const char* signName, float* pFeature, float* pTarget, int cacheSize = 4096){
    return AlphaRFT::getAlphaRFT()->testReadData(daybefore, sampleSize, target, signName, pFeature, pTarget, cacheSize);
}

void trainAlphaRFT(int daybefore, int sampleSize, const char* weight, const char* target, const char* signName,
                   float gamma, float lambda, float minWeight = 1024, int epochNum = 30000,
                   const char* rewardFunName = "base", int stepNum = 1, int splitNum = 64, int cacheSize = 4096,
                   const char* lossFunName = "binary:logistic", float lr = 0.1f, float step = 0.8f, float tiredCoff = 0.016f
){
    AlphaRFT::getAlphaRFT()->train(daybefore, sampleSize, weight, target, signName, gamma, lambda, minWeight, epochNum, rewardFunName, stepNum, splitNum, cacheSize, lossFunName, lr, step, tiredCoff);
}

void evalAlphaRFT(int daybefore, int sampleSize, const char* target, const char* signName, int stepIndex, int cacheSize){
    AlphaRFT::getAlphaRFT()->eval(daybefore, sampleSize, target, signName, stepIndex, cacheSize);
}

void saveRFTModel(const char* path){
    AlphaRFT::getAlphaRFT()->saveModel(path);
}

void loadRFTModel(const char* path){
    AlphaRFT::getAlphaRFT()->loadModel(path);
}

void cleanAlphaRFT(float threshold, const char* rewardFunName, int startStepIndex, int stepNum){
    AlphaRFT::getAlphaRFT()->cleanTree(threshold, rewardFunName, startStepIndex, stepNum);
}

void  cleanAlphaRFTCorrelationLeaf(int daybefore, int sampleSize, const char* signName, float corrOtherPercent = 0.16f, float corrAllPercent = 0.32f, int cacheSize = 2048){
    AlphaRFT::getAlphaRFT()->cleanCorrelationLeaf(daybefore, sampleSize, signName, corrOtherPercent, corrAllPercent, cacheSize);
}

int alphaRFT2String(char* pout, const char* rewardFunName, int startIndex, int stepNum){
    return AlphaRFT::getAlphaRFT()->tostring(pout, rewardFunName, startIndex, stepNum);
}

int predAlphaRFT(int daybefore, float* predOut, const char* signName, const char* rewardFunName, int startStepIndex, int stepNum, int cacheSize = 2048){
    return AlphaRFT::getAlphaRFT()->pred(daybefore, predOut, signName, rewardFunName, startStepIndex, stepNum, cacheSize);
    //AlphaRFT::getAlphaRFT()->pred(daybefore, predOut, codes, stockSize, rewardFunName, startStepIndex, stepNum);
}




void initializeAlphaGBDF(const char* alphatreeList, int alphatreeNum, int forestSize, float gamma, float lambda, int threadNum, const char* lossFunName = "binary:logistic"){
    AlphaGBDF::initialize(AlphaForest::getAlphaforest(), alphatreeList, alphatreeNum, forestSize, gamma, lambda, threadNum, lossFunName);
}

void releaseAlphaGBDF(){
    AlphaGBDF::release();
}

int testReadGBDF(int daybefore, int sampleSize, const char* target, const char* signName, float* pFeature, float* pTarget, int cacheSize = 4096){
    return AlphaGBDF::getAlphaGBDF()->testReadData(daybefore, sampleSize, target, signName, pFeature, pTarget, cacheSize);
}

void trainAlphaGBDF(int daybefore, int sampleSize, const char* weight, const char* target, const char* signName,float samplePercent, float featurePercent,
                    int barSize, float minWeight, int maxDepth, int boostNum, float boostWeightScale, int cacheSize){
    AlphaGBDF::getAlphaGBDF()->train(daybefore, sampleSize, weight, target, signName, samplePercent, featurePercent, barSize, minWeight, maxDepth, boostNum, boostWeightScale, cacheSize);
}

void predAlphaGBDF(int daybefore, int sampleSize, const char* signName, float* predOut, int cacheSize){
    AlphaGBDF::getAlphaGBDF()->pred(daybefore, sampleSize, signName, predOut, cacheSize);
}

void saveGBDFModel(const char* path){
    AlphaGBDF::getAlphaGBDF()->saveModel(path);
}

void loadGBDFModel(const char* path){
    AlphaGBDF::getAlphaGBDF()->loadModel(path);
}

void initializeAlphaRL(char* featureList, int featureNum, char* price, char* volume){
    AlphaRLT::initialize(AlphaForest::getAlphaforest(), featureList, featureNum, price, volume);
}

void releaseAlphaRL(){
    AlphaRLT::release();
}

void trainAlphaRL(const char* signName, char* codes, int stockNum, char* marketCodes, int marketNum, int daybefore, int sampleDays, int minSignNum, int threadNum, int epochNum,
                            int maxBagNum, int maxBuyDepth, float lr, float step, float tiredCoff, float explorationRate){
    AlphaRLT::getAlphaRLT()->train(signName, codes, stockNum, marketCodes, marketNum, daybefore, sampleDays, minSignNum, threadNum, epochNum, maxBagNum, maxBuyDepth, lr, step, tiredCoff, explorationRate);
}

}