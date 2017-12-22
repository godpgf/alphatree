//
// Created by yanyu on 2017/7/20.
//

#include "alphaforest.h"
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

void decodeAlphatree(int alphaTreeId, const char *rootName, const char *line, bool isLocal) {
    AlphaForest::getAlphaforest()->decode(alphaTreeId, rootName, line, isLocal);
}

int encodeProcess(int alphatreeId, const char *processName, char *out) {
    const char *res = AlphaForest::getAlphaforest()->encodeProcess(alphatreeId, processName, out);
    return strlen(res);
}

void decodeProcess(int alphaTreeId, const char *processName, const char *line) {
    AlphaForest::getAlphaforest()->decodeProcess(alphaTreeId, processName, line);
}

void learnFilterForest(int alphatreeId, int cacheId, const char *features, int featureSize, int treeSize,
                       int iteratorNum, float gamma, float lambda, int maxDepth,
                       int maxLeafSize, int maxAdjWeightTime, float adjWeightRule,
                       int maxBarSize, float mergeBarPercent, float subsample,
                       float colsampleBytree, const char *buySign, const char *sellSign,
                       const char *targetValue) {
    AlphaForest::getAlphaforest()->learnFilterForest(alphatreeId, cacheId, features, featureSize, treeSize,
                                                     iteratorNum, gamma, lambda, maxDepth, maxLeafSize,
                                                     maxAdjWeightTime, adjWeightRule, maxBarSize, mergeBarPercent,
                                                     subsample, colsampleBytree, buySign, sellSign, targetValue);

}

void loadDataBase(const char *path) {
    AlphaForest::getAlphaforest()->getAlphaDataBase()->loadDataBase(path);
}

//    void loadStockMeta(const char* codes, int* marketIndex, int* industryIndex, int* conceptIndex, int size, int days){
//        AlphaForest::getAlphaforest()->getAlphaDataBase()->loadStockMeta(codes, marketIndex, industryIndex, conceptIndex, size, days);
//    }
//
//    void loadDataElement(const char* elementName, const float* data, int needDays){
//        AlphaForest::getAlphaforest()->getAlphaDataBase()->loadDataElement(elementName, data, needDays);
//    }

int getStockCodes(char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getStockCodes(codes);
}

int getMarketCodes(const char *marketName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getMarketCodes(marketName, codes);
}

int getIndustryCodes(const char *industryName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getIndustryCodes(industryName, codes);
}

int getConceptCodes(const char *conceptName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getConceptCodes(conceptName, codes);
}

int getMaxHistoryDays(int alphaTreeId) { return AlphaForest::getAlphaforest()->getMaxHistoryDays(alphaTreeId); }

void flagAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, const char *codes, int stockSize,
               bool *sampleFlag = nullptr, bool isCalAllNode = false) {
    AlphaForest::getAlphaforest()->flagAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, codes, stockSize, sampleFlag,
                                             isCalAllNode);
}

void calAlpha(int alphaTreeId, int cacheId) {
    AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId);
}

void cacheAlpha(int alphaTreeId, int cacheId, bool isFeature) {
    AlphaForest::getAlphaforest()->cacheAlpha(alphaTreeId, cacheId, isFeature);
}

void processAlpha(int alphaTreeId, int cacheId) {
    AlphaForest::getAlphaforest()->processAlpha(alphaTreeId, cacheId);
}

int getRootAlpha(int alphaTreeId, const char *rootName, int cacheId, float *alpha) {
    const float *res = AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, rootName, cacheId);
    auto *cache = AlphaForest::getAlphaforest()->getCache(cacheId);
    int dataSize = cache->sampleDays * cache->stockSize;
    memcpy(alpha, res, dataSize * sizeof(float));
    return dataSize;
}

int getNodeAlpha(int alphaTreeId, int nodeId, int cacheId, float *alpha) {
    const float *res = AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, nodeId, cacheId);
    auto *cache = AlphaForest::getAlphaforest()->getCache(cacheId);
    int dataSize = cache->sampleDays * cache->stockSize;
    memcpy(alpha, res, dataSize * sizeof(float));
    return dataSize;
}

int getProcess(int alphaTreeId, const char *processName, int cacheId, char *result) {
    const char *res = AlphaForest::getAlphaforest()->getProcess(alphaTreeId, processName, cacheId);
    strcpy(result, res);
    return strlen(result);
}

//总结子公式
int summarySubAlphaTree(const int *alphatreeIds, int len, int minDepth, char *subAlphatreeStr) {
    return AlphaForest::getAlphaforest()->summarySubAlphaTree(alphatreeIds, len, minDepth, subAlphatreeStr);
}

}
