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
    void initializeAlphaforest(int cacheSize){
        AlphaForest::initialize(cacheSize);
    }

    void releaseAlphaforest(){
        AlphaForest::release();
    }

    int createAlphatree(){
        return AlphaForest::getAlphaforest()->useAlphaTree();
    }

    void releaseAlphatree(int alphatreeId){
        AlphaForest::getAlphaforest()->releaseAlphaTree(alphatreeId);
    }

    int useCache(){
        return AlphaForest::getAlphaforest()->useCache();
    }

    void releaseCache(int cacheId){
        AlphaForest::getAlphaforest()->releaseCache(cacheId);
    }

    int encodeAlphatree(int alphatreeId, const char* rootName, char *out){
        const char* res = AlphaForest::getAlphaforest()->encodeAlphaTree(alphatreeId, rootName, out);
        return strlen(res);
    }

    void decodeAlphatree(int alphaTreeId, const char* rootName, const char *line, bool isLocal){
        AlphaForest::getAlphaforest()->decode(alphaTreeId, rootName, line, isLocal);
    }

    int encodeProcess(int alphatreeId, const char* processName, char* out){
        const char* res = AlphaForest::getAlphaforest()->encodeProcess(alphatreeId, processName, out);
        return strlen(res);
    }

    void decodeProcess(int alphaTreeId, const char* processName, const char* line){
        AlphaForest::getAlphaforest()->decodeProcess(alphaTreeId, processName, line);
    }

    void addStock(const char* code, const char* market, const char* industry, const char* concept,
                  const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
                  int size, int totals){
        AlphaForest::getAlphaforest()->addStock(code, market, industry, concept, open, high, low, close, volume, vwap, returns, size, totals);
    }

    void clearAllStock(){
        AlphaForest::getAlphaforest()->getAlphaDataBase()->clear();
    }

    void calClassifiedData(){
        AlphaForest::getAlphaforest()->getAlphaDataBase()->calClassifiedData(true);
        AlphaForest::getAlphaforest()->getAlphaDataBase()->calClassifiedData(false);
    }

    int getCodes(int dayBefore, int historyNum, int sampleNum, char* codes){
        return AlphaForest::getAlphaforest()->getCodes(dayBefore, historyNum, sampleNum, codes);
    }

    int getAllCodes(char* codes){
        return AlphaForest::getAlphaforest()->getCodes(codes);
    }

    int getMaxHistoryDays(int alphaTreeId){ return AlphaForest::getAlphaforest()->getMaxHistoryDays(alphaTreeId);}

    void flagAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, const char* codes, int stockSize, bool* sampleFlag = nullptr, bool isCalAllNode = false){
        AlphaForest::getAlphaforest()->flagAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, codes, stockSize, sampleFlag, isCalAllNode);
    }

    void calAlpha(int alphaTreeId, int cacheId){
        AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId);
    }

    void processAlpha(int alphaTreeId, int cacheId){
        AlphaForest::getAlphaforest()->processAlpha(alphaTreeId, cacheId);
    }

    int getRootAlpha(int alphaTreeId, const char* rootName, int cacheId, float* alpha){
        const float* res =AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, rootName, cacheId);
        auto* cache = AlphaForest::getAlphaforest()->getCache(cacheId);
        int dataSize = cache->sampleDays * cache->stockSize;
        memcpy(alpha, res, dataSize * sizeof(float));
        return dataSize;
    }

    int getNodeAlpha(int alphaTreeId, int nodeId, int cacheId, float* alpha){
        const float* res =AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, nodeId, cacheId);
        auto* cache = AlphaForest::getAlphaforest()->getCache(cacheId);
        int dataSize = cache->sampleDays * cache->stockSize;
        memcpy(alpha, res, dataSize * sizeof(float));
        return dataSize;
    }

    int getProcess(int alphaTreeId, const char* processName, int cacheId, char* result){
        const char* res = AlphaForest::getAlphaforest()->getProcess(alphaTreeId, processName, cacheId);
        strcpy(result, res);
        return strlen(result);
    }

    //总结子公式
    int summarySubAlphaTree(const int* alphatreeIds, int len, int minDepth, char* subAlphatreeStr){
        return AlphaForest::getAlphaforest()->summarySubAlphaTree(alphatreeIds, len, minDepth, subAlphatreeStr);
    }

}
