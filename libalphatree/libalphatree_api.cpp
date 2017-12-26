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
    FilterMachine::initialize(cacheSize);
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

int learnFilterForest(int alphatreeId, int cacheId, const char *features, int featureSize, int treeSize,
                       int iteratorNum, float gamma, float lambda, int maxDepth,
                       int maxLeafSize, int maxAdjWeightTime, float adjWeightRule,
                       int maxBarSize, float mergeBarPercent, float subsample,
                       float colsampleBytree, const char *buySign, const char *sellSign,
                       const char *targetValue) {
    cout<<"learn filter forest\n";
    AlphaCache *cache = AlphaForest::getAlphaforest()->getCache(cacheId);
    const float *buy = AlphaForest::getAlphaforest()->getAlpha(alphatreeId, buySign, cacheId);
    const float *sell = AlphaForest::getAlphaforest()->getAlpha(alphatreeId, sellSign, cacheId);
    const float *target = AlphaForest::getAlphaforest()->getAlpha(alphatreeId, targetValue, cacheId);
    cout<<"start static\n";

    //计算最大取样数据量
    size_t sampleSize = 0;
    for (size_t i = 0; i < cache->stockSize; ++i) {
        bool isBuy = false;
        for (size_t j = 0; j < cache->sampleDays; ++j) {
            size_t index = j * cache->stockSize + i;
            if (isBuy && sell[index] > 0) {
                ++sampleSize;
                isBuy = false;
            }
            if (isBuy == false && buy[index] > 0)
                isBuy = true;
        }
    }

    cout<<"sample size "<<sampleSize<<endl;
    int filterCacheId = FilterMachine::getFM()->useCache();
    FilterCache *filterCache = FilterMachine::getFM()->getCache(filterCacheId);
    filterCache->initialize(sampleSize, featureSize, maxLeafSize);
    cout<<"finish init fcache\n";

    //填写训练参数
    filterCache->treeSize = treeSize;
    filterCache->iteratorNum = iteratorNum;
    filterCache->maxDepth = maxDepth;
    filterCache->maxLeafSize = maxLeafSize;
    filterCache->gamma = gamma;
    filterCache->lambda = lambda;
    filterCache->maxAdjWeightTime = maxAdjWeightTime;
    filterCache->adjWeightRule = adjWeightRule;
    filterCache->maxBarSize = maxBarSize;
    filterCache->mergeBarPercent = mergeBarPercent;
    filterCache->subsample = subsample;
    filterCache->colsampleBytree = colsampleBytree;

    //填写特征
    const char *curFeature = features;
    AlphaForest::getAlphaforest()->getAlphaDataBase()->getFeature(cache->dayBefore, cache->sampleDays, cache->stockSize, cache->codes, buy, sell,
                              filterCache->feature, sampleSize, features, featureSize);
    cout<<"finish fill feature\n";

    //填写特征名字
    for (size_t fId = 0; fId < featureSize; ++fId) {
        strcpy(filterCache->featureName + fId * MAX_FEATURE_NAME_LEN, curFeature);
        curFeature += strlen(curFeature) + 1;
    }
    cout<<"finish fill feature name\n";

    //填写目标
    size_t sampleIndex = 0;
    for (size_t i = 0; i < cache->stockSize; ++i) {
        bool isBuy = false;
        for (size_t j = 0; j < cache->sampleDays; ++j) {
            size_t index = j * cache->stockSize + i;
            if (isBuy && sell[index] > 0) {
                filterCache->target[sampleIndex++] = target[index];
                isBuy = false;
            }
            if (isBuy == false && buy[index] > 0)
                isBuy = true;
        }
    }
    cout<<"finish fill target\n";

    int forestId = FilterMachine::getFM()->useFilterForest();

    cout<<"train\n";
    FilterMachine::getFM()->train(filterCacheId, forestId);

    FilterMachine::getFM()->releaseCache(filterCacheId);
    return forestId;
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

int getMaxHistoryDays(int alphaTreeId) { return AlphaForest::getAlphaforest()->getMaxHistoryDays(alphaTreeId); }

void flagAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, const char *codes, int stockSize,
               bool *sampleFlag = nullptr, bool isCalAllNode = false) {
    AlphaForest::getAlphaforest()->flagAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, codes, stockSize, sampleFlag,
                                             isCalAllNode);
}

void calAlpha(int alphaTreeId, int cacheId) {
    AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId);
}

void cacheAlpha(int alphaTreeId, int cacheId) {
    AlphaForest::getAlphaforest()->cacheAlpha(alphaTreeId, cacheId);
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
