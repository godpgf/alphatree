//
// Created by yanyu on 2017/7/20.
//

#include "alphaforest.h"
#include "alpharft.h"
#include "alphagbdt.h"
#include <iostream>

using namespace std;

#define MAX_TREE_SIZE 32768
#define MAX_SUB_TREE_SIZE 16
#define MAX_NODE_SIZE 64
#define MAX_STOCK_SIZE 3600
#define MAX_HISTORY_DAYS 250
#define MAX_SAMPLE_DAYS 2500
#define MAX_FUTURE_DAYS 80
#define CODE_LEN 64


extern "C"
{
void initializeAlphaforest(int cacheSize) {
    AlphaForest::initialize(cacheSize);
}

void releaseAlphaforest() {
    AlphaForest::release();
}

void loadDataBase(const char *path) {
    AlphaForest::getAlphaforest()->getAlphaDataBase()->loadDataBase(path);
}

void csv2binary(const char *path, const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->csv2binary(path, featureName);
}

void cacheFeature(const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->cacheFeature(featureName);
}

int createAlphatree() {
    return AlphaForest::getAlphaforest()->useAlphaTree();
}

void releaseAlphatree(int alphatreeId) {
    AlphaForest::getAlphaforest()->releaseAlphaTree(alphatreeId);
}

int useCache() {
    return AlphaForest::getAlphaforest()->useCache();
}

void releaseCache(int cacheId) {
    AlphaForest::getAlphaforest()->releaseCache(cacheId);
}

int encodeAlphatree(int alphatreeId, const char *rootName, char *out) {
    const char *res = AlphaForest::getAlphaforest()->encodeAlphaTree(alphatreeId, rootName, out);
    return strlen(res);
}

void decodeAlphatree(int alphaTreeId, const char *rootName, const char *line) {
    AlphaForest::getAlphaforest()->decode(alphaTreeId, rootName, line);
}

int getStockCodes(char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getStockCodes(codes);
}

int getMarketCodes(const char *marketName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getMarketCodes(marketName, codes);
}

int getIndustryCodes(const char *industryName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getIndustryCodes(industryName, codes);
}

int getMaxHistoryDays(int alphaTreeId) { return AlphaForest::getAlphaforest()->getMaxHistoryDays(alphaTreeId); }

int getSignNum(int dayBefore, int sampleSize, const char* signName){
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
}


void calAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, const char *codes, int stockSize) {
    AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, codes, stockSize);
}

void calSignAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, int startIndex, int signNum, int signHistoryDays, const char* signName){
    AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, startIndex, signNum, signHistoryDays, signName);
}

void cacheAlpha(int alphaTreeId, int cacheId, const char* featureName) {
    AlphaForest::getAlphaforest()->cacheAlpha(alphaTreeId, cacheId, featureName);
}

void cacheSign(int alphaTreeId, int cacheId, const char* signName){
    AlphaForest::getAlphaforest()->cacheSign(alphaTreeId, cacheId, signName);
}

void cacheCodesSign(int alphaTreeId, int cacheId, const char* signName, const char* codes, int codesNum){
    AlphaForest::getAlphaforest()->cacheSign(alphaTreeId, cacheId, signName, codes, codesNum);
}


float optimizeAlpha(int alphaTreeId, int cacheId, const char *rootName, int dayBefore, int sampleSize, const char *codes, size_t stockSize, float exploteRatio, int errTryTime){
    return AlphaForest::getAlphaforest()->optimizeAlpha(alphaTreeId, cacheId, rootName, dayBefore, sampleSize, codes, stockSize, exploteRatio, errTryTime);
}

int getAlpha(int alphaTreeId, const char *rootName, int cacheId, float *alpha) {
    const float *res = AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, rootName, cacheId);
    auto *cache = AlphaForest::getAlphaforest()->getCache(cacheId);
    int dataSize = cache->getAlphaDays() * cache->stockSize;
    float a = 0;
    for(int i = 0; i < dataSize; ++i){
        a += res[i];
    }
    for(int i = 0; i < dataSize; ++i){
        alpha[i] = 0;
    }
    memcpy(alpha, res, dataSize * sizeof(float));
    return dataSize;
}

void getAlphaSum(int alphaTreeId, const char *rootName, int cacheId, float* alpha){
    const float* res = AlphaForest::getAlphaforest()->getAlphaSum(alphaTreeId, rootName, cacheId);
    memcpy(alpha, res, 2 * sizeof(float));
}

void getAlphaSmooth(int alphaTreeId, const char *rootName, int cacheId, int smoothNum, float* smooth){
    return AlphaForest::getAlphaforest()->getAlphaSmooth(alphaTreeId, rootName, cacheId, smoothNum, smooth);
}

//机器学习----------------------------------------------------------------------------------
void initializeAlphaRFT(const char* alphatreeList, int alphatreeNum){
    AlphaRFT::initialize(AlphaForest::getAlphaforest(), alphatreeList, alphatreeNum);
}

void releaseAlphaRFT(){
    AlphaRFT::release();
}

void trainAlphaRFT(int daybefore, int sampleSize, int pruneDaybefore, int pruneSampleSize,const char* target, const char* signName,
                float gamma, float lambda, float minWeight = 1024, int epochNum = 30000,
                bool isConsiderRisk = false, int splitNum = 64, int cacheSize = 4096,
                const char* lossFunName = "binary:logistic", int epochPerGC = 4096, float gcThreshold = 0.06f,
                float corrOtherPercent = 0.32f, float corrAllPercent = 0.64f, float lr = 0.1f, float step = 0.8f, float tiredCoff = 0.016
){
    AlphaRFT::getAlphaRFT()->train(daybefore, sampleSize, pruneDaybefore, pruneSampleSize, target, signName, gamma, lambda, minWeight, epochNum, isConsiderRisk, splitNum, cacheSize, lossFunName, epochPerGC, gcThreshold, corrOtherPercent, corrAllPercent, lr, step, tiredCoff);
}

void evalAlphaRFT(int daybefore, int sampleSize, const char* target, const char* signName, int cacheSize){
    AlphaRFT::getAlphaRFT()->eval(daybefore, sampleSize, target, signName, cacheSize);
}

void saveRFTModel(const char* path){
    AlphaRFT::getAlphaRFT()->saveModel(path);
}

void loadRFTModel(const char* path){
    AlphaRFT::getAlphaRFT()->loadModel(path);
}

void cleanAlphaRFT(float threshold, bool isConsiderRisk){
    AlphaRFT::getAlphaRFT()->cleanTree(threshold, isConsiderRisk);
}

int alphaRFT2String(char* pout, bool isConsiderRisk){
    return AlphaRFT::getAlphaRFT()->tostring(pout, isConsiderRisk);
}

void predAlphaRFT(int daybefore, float* predOut, const char *codes, int stockSize, bool isConsiderRisk){
    AlphaRFT::getAlphaRFT()->pred(daybefore, predOut, codes, stockSize, isConsiderRisk);
}


void initializeAlphaGBDT(const char* alphatreeList, int alphatreeNum, float gamma, float lambda, int threadNum, const char* lossFunName = "binary:logistic") {
    AlphaGBDT::initialize(AlphaForest::getAlphaforest(), alphatreeList, alphatreeNum, gamma, lambda, threadNum, lossFunName);
}

void releaseAlphaGBDT(){
    AlphaGBDT::release();
}

void trainAlphaGBDT(int daybefore, int sampleSize, const char* target, const char* signName,
                    int barSize, float minWeight, int maxDepth, int boostNum, float boostWeightScale, int cacheSize){
    AlphaGBDT::getAlphaGBDT()->train(daybefore, sampleSize, target, signName, barSize, minWeight, maxDepth, boostNum, boostWeightScale, cacheSize);
}

void predAlphaGBDT(int daybefore, int sampleSize, const char* signName, float* predOut, int cacheSize){
    AlphaGBDT::getAlphaGBDT()->pred(daybefore, sampleSize, signName, predOut, cacheSize);
}

void saveGBDTModel(const char* path){
    AlphaGBDT::getAlphaGBDT()->saveModel(path);
}

void loadGBDTModel(const char* path){
    AlphaGBDT::getAlphaGBDT()->loadModel(path);
}

int alphaGBDT2String(char* pout){
    return AlphaGBDT::getAlphaGBDT()->tostring(pout);
}

}
