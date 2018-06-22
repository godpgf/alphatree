//
// Created by yanyu on 2017/7/20.
//

#define ML

#include "alphaforest.h"
#include <iostream>

#ifdef ML
#include "alphagbdt.h"
#endif

using namespace std;



#ifndef WIN32 // or something like that...
#define DLLEXPORT
#else
#define DLLEXPORT __declspec(dllexport)
#endif

//#define MAX_TREE_SIZE 32768
//#define MAX_SUB_TREE_SIZE 16
//#define MAX_NODE_SIZE 64
//#define MAX_STOCK_SIZE 3600
//#define MAX_HISTORY_DAYS 250
//#define MAX_SAMPLE_DAYS 2500
//#define MAX_FUTURE_DAYS 80
//#define CODE_LEN 64


extern "C"
{

void DLLEXPORT initializeAlphaforest(int cacheSize) {
    AlphaForest::initialize(cacheSize);
}

void DLLEXPORT releaseAlphaforest() {
    AlphaForest::release();
}

void DLLEXPORT loadDataBase(const char *path) {
    AlphaForest::getAlphaforest()->getAlphaDataBase()->loadDataBase(path);
}

void DLLEXPORT csv2binary(const char *path, const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->csv2binary(path, featureName);
}

void DLLEXPORT cacheMiss(){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->miss2binary();
}

void DLLEXPORT cacheBoolHMM(const char* featureName, int hideStateNum, size_t seqLength, const char* codes, int codesNum, int epochNum = 8){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->boolhmm2binary(featureName, hideStateNum, seqLength, codes, codesNum, epochNum);
}

void DLLEXPORT loadFeature(const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->loadFeature(featureName);
}

void DLLEXPORT releaseFeature(const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->releaseFeature(featureName);
}

void DLLEXPORT updateFeature(const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->updateFeature(featureName);
}

void DLLEXPORT releaseAllFeature(){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->releaseAllFeature();
}

void DLLEXPORT loadSign(const char* signName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->loadSign(signName);
}

void DLLEXPORT releaseAllSign(){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->releaseAllSign();
}

int DLLEXPORT createAlphatree() {
    return AlphaForest::getAlphaforest()->useAlphaTree();
}

void DLLEXPORT releaseAlphatree(int alphatreeId) {
    AlphaForest::getAlphaforest()->releaseAlphaTree(alphatreeId);
}

int DLLEXPORT useCache() {
    return AlphaForest::getAlphaforest()->useCache();
}

void DLLEXPORT releaseCache(int cacheId) {
    AlphaForest::getAlphaforest()->releaseCache(cacheId);
}

int DLLEXPORT encodeAlphatree(int alphatreeId, const char *rootName, char *out) {
    const char *res = AlphaForest::getAlphaforest()->encodeAlphaTree(alphatreeId, rootName, out);
    return strlen(res);
}

void DLLEXPORT decodeAlphatree(int alphaTreeId, const char *rootName, const char *line) {
    AlphaForest::getAlphaforest()->decode(alphaTreeId, rootName, line);
}

int DLLEXPORT getStockCodes(char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getStockCodes(codes);
}

int DLLEXPORT getStockIds(int dayBefore, int sampleSize, const char* signName, int* dst){
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getStockIds(dayBefore, sampleSize, signName, dst);
}

int DLLEXPORT getCode(int id, char* codes){
    strcpy(codes, AlphaForest::getAlphaforest()->getAlphaDataBase()->getCode(id));
    return strlen(codes);
}

int DLLEXPORT getMarketCodes(const char *marketName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getMarketCodes(marketName, codes);
}

int DLLEXPORT getIndustryCodes(const char *industryName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getIndustryCodes(industryName, codes);
}

int DLLEXPORT getMaxHistoryDays(int alphaTreeId) { return AlphaForest::getAlphaforest()->getMaxHistoryDays(alphaTreeId); }

int DLLEXPORT getSignNum(int dayBefore, int sampleSize, const char* signName){
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
}


void DLLEXPORT calAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, const char *codes, int stockSize) {
    AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, codes, stockSize);
}

void DLLEXPORT calSignAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, int startIndex, int signNum, int signHistoryDays, const char* signName){
    AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, startIndex, signNum, signHistoryDays, signName);
}

void DLLEXPORT cacheAlpha(int alphaTreeId, int cacheId, const char* featureName) {
    AlphaForest::getAlphaforest()->cacheAlpha(alphaTreeId, cacheId, featureName);
}

void DLLEXPORT cacheSign(int alphaTreeId, int cacheId, const char* signName){
    AlphaForest::getAlphaforest()->cacheSign(alphaTreeId, cacheId, signName);
}

void DLLEXPORT cacheCodesSign(int alphaTreeId, int cacheId, const char* signName, const char* codes, int codesNum){
    AlphaForest::getAlphaforest()->cacheSign(alphaTreeId, cacheId, signName, codes, codesNum);
}


float DLLEXPORT optimizeAlpha(int alphaTreeId, int cacheId, const char *rootName, int dayBefore, int sampleSize, const char *codes, size_t stockSize, float exploteRatio, int errTryTime){
    return AlphaForest::getAlphaforest()->optimizeAlpha(alphaTreeId, cacheId, rootName, dayBefore, sampleSize, codes, stockSize, exploteRatio, errTryTime);
}

int DLLEXPORT getAlpha(int alphaTreeId, const char *rootName, int cacheId, float *alpha) {
    const float *res = AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, rootName, cacheId);
    auto *cache = AlphaForest::getAlphaforest()->getCache(cacheId);
    int dataSize = cache->getAlphaDays() * cache->stockSize;
    memcpy(alpha, res, dataSize * sizeof(float));
    return dataSize;
}

void DLLEXPORT synchroAlpha(int alphaTreeId, int cacheId){
    AlphaForest::getAlphaforest()->synchroAlpha(alphaTreeId, cacheId);
}

void DLLEXPORT getAlphaSum(int alphaTreeId, const char *rootName, int cacheId, float* alpha){
    const float* res = AlphaForest::getAlphaforest()->getAlphaSum(alphaTreeId, rootName, cacheId);
    memcpy(alpha, res, 2 * sizeof(float));
}

void DLLEXPORT getAlphaSmooth(int alphaTreeId, const char *rootName, int cacheId, int smoothNum, float* smooth){
    return AlphaForest::getAlphaforest()->getAlphaSmooth(alphaTreeId, rootName, cacheId, smoothNum, smooth);
}

void DLLEXPORT getReturns(const char* codes, int stockSize, int dayBefore, int sampleSize,  const char* buySignList, int buySignNum, const char* sellSignList, int sellSignNum, float maxReturn, float maxDrawdown, int maxHoldDays, float* returns, const char* price = "close"){
    AlphaForest::getAlphaforest()->getReturns(codes, stockSize, dayBefore, sampleSize, buySignList, buySignNum, sellSignList, sellSignNum, maxReturn, maxDrawdown, maxHoldDays, returns, price);
}

void DLLEXPORT getBag(const char* codes, int stockSize, const char* feature, const char* signName, int dayBefore, int sampleSize, int bagNum, float* bags){
    int alphatreeId = AlphaForest::getAlphaforest()->useAlphaTree();
    AlphaForest::getAlphaforest()->decode(alphatreeId,"sign",feature);
    size_t signNum = AlphaForest::getAlphaforest()->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
    AlphaSignIterator asi(AlphaForest::getAlphaforest(), "sign", signName, alphatreeId, dayBefore, sampleSize, 0, signNum);
    getBags(&asi, bags, bagNum);
    AlphaForest::getAlphaforest()->releaseAlphaTree(alphatreeId);
}

#ifdef ML
void DLLEXPORT initializeAlphaGBDT(const char* alphatreeList, int alphatreeNum, float gamma, float lambda, int threadNum, const char* lossFunName = "binary:logistic") {
    AlphaGBDT::initialize(AlphaForest::getAlphaforest(), alphatreeList, alphatreeNum, gamma, lambda, threadNum, lossFunName);
}

void DLLEXPORT releaseAlphaGBDT(){
    AlphaGBDT::release();
}

int DLLEXPORT testReadGBDT(int daybefore, int sampleSize, const char* target, const char* signName, float* pFeature, float* pTarget, int cacheSize = 4096){
    return AlphaGBDT::getAlphaGBDT()->testReadData(daybefore, sampleSize, target, signName, pFeature, pTarget, cacheSize);
}

void DLLEXPORT getFirstFeatureGainsGBDT(int daybefore, int sampleSize, const char* weight, const char* target, const char* signName, float* gains,
                              int barSize, int cacheSize){
    AlphaGBDT::getAlphaGBDT()->getFirstFeatureGains(daybefore, sampleSize, weight, target, signName, gains, barSize, cacheSize);
}

void DLLEXPORT trainAlphaGBDT(int daybefore, int sampleSize, const char* weight, const char* target, const char* signName,
                    int barSize, float minWeight, int maxDepth, float samplePercent, float featurePercent, int boostNum, float boostWeightScale, int cacheSize){
    AlphaGBDT::getAlphaGBDT()->train(daybefore, sampleSize, weight, target, signName, barSize, minWeight, maxDepth, samplePercent, featurePercent, boostNum, boostWeightScale, cacheSize);
}

void DLLEXPORT predAlphaGBDT(int daybefore, int sampleSize, const char* signName, float* predOut, int cacheSize){
    AlphaGBDT::getAlphaGBDT()->pred(daybefore, sampleSize, signName, predOut, cacheSize);
}

void DLLEXPORT saveGBDTModel(const char* path){
    AlphaGBDT::getAlphaGBDT()->saveModel(path);
}

void DLLEXPORT loadGBDTModel(const char* path){
    AlphaGBDT::getAlphaGBDT()->loadModel(path);
}

int DLLEXPORT alphaGBDT2String(char* pout){
    return AlphaGBDT::getAlphaGBDT()->tostring(pout);
}

#endif
}
