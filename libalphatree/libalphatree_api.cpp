//
// Created by yanyu on 2017/7/20.
//

#include "alphaforest.h"
#include "alphaexaminer.h"
#include <iostream>
using namespace std;

extern "C"
{
    void initializeAlphaforest(int maxHistorySize, int maxStoctSize, int maxSampleSize, int maxWatchSize, int maxCacheSize, int maxAlphaTreeSize, int maxNodeSize, int maxSubTreeNum, int maxAchievementSize, int sampleBetaSize);

    void releaseAlphaforest();

    int createAlphatree();

    void releaseAlphatree(int alphatreeId);

    int encodeAlphatree(int alphatreeId, char *out);

    void decodeAlphatree(int alphaTreeId, const char *line);

    void decodeSubAlphatree(int alphaTreeId, const char *name, const char *line);

    int getRoot(int alphaTreeId);

    int getNodeDes(int alphaTreeId, int nodeId, char* name, char* coff, int* childList);

    void addStock(const char* code, const char* market, const char* industry, const char* concept,
                  const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
                  int size, int totals);

    void calClassifiedData();

    int getCodes(int dayBefore, int watchFutureNum, int historyNum, int sampleNum, char* codes);

    int getHistoryDays(int alphaTreeId){ return AlphaForest::getAlphaforest()->getHistoryDays(alphaTreeId);}

    int getFutureDays(int alphaTreeId){ return AlphaForest::getAlphaforest()->getFutureDays(alphaTreeId);}

    void calAlpha(int alphaTreeId, int cacheMemoryId, int flagCacheId, int dayBefore, int sampleSize, const char* codes, int stockSize, float* alpha);

    void getAlpha(int alphaTreeId, int nodeId, int cacheMemoryId, size_t sampleSize, size_t stockSize, float* alpha);

    int useCacheMemory();

    void releaseCacheMemory(int cacheMemoryId);

    int useFlagCache();

    void releaseFlagCache(int flagCacheId);

    int useCodesCache();

    void releaseCodesCache(int codesCacheId);

    //总结子公式
    int summarySubAlphaTree(const int* alphatreeIds, int len, int minDepth, char* subAlphatreeStr);

    //验证-----------------------------------------------------------------------------------------------
    void initializeAlphaexaminer();

    void releaseAlphaexaminer();

    float evalAlphaTree(int alphatreeId,int cacheMemoryId, int flagCacheId, int codesId, int futureNum, int evalTime, int trainDateSize, int testDateSize, int minSieveNum, int maxSieveNum, int punishNum);

    void getTarget(int futureIndex, int dayBefore, int sampleNum, int stockNum, float* dst, const char* codes);
}

void initializeAlphaforest(int maxHistorySize, int maxStoctSize, int maxSampleSize, int maxFutureSize, int maxCacheSize, int maxAlphaTreeSize, int maxNodeSize, int maxSubTreeNum, int maxAchievementSize, int sampleBetaSize){
    //cout<<maxHistorySize<<" "<<maxStoctSize<<" "<< maxSampleSize<<" "<<maxWatchSize<<" "<<maxCacheSize<<" "<<maxAlphaTreeSize<<" "<<maxNodeSize<<" "<<maxAchievementSize<<endl;
    //cout<<"start initializeAlphaforest"<<endl;
    AlphaForest::initialize(maxHistorySize, maxStoctSize, maxSampleSize, maxFutureSize, maxCacheSize, maxAlphaTreeSize, maxNodeSize, maxSubTreeNum, maxAchievementSize, sampleBetaSize);
    //cout<<"finish initializeAlphaforest"<<endl;
}

void releaseAlphaforest(){
    //cout<<"start releaseAlphaforest"<<endl;
    AlphaForest::release();
    //cout<<"finish releaseAlphaforest"<<endl;
}

int createAlphatree(){
    //cout<<"start createAlphatree"<<endl;
    int alphaTreeId = AlphaForest::getAlphaforest()->useAlphaTree();
    //cout<<"finish createAlphatree"<<endl;
    return alphaTreeId;
}

void releaseAlphatree(int alphatreeId){
    //cout<<"start releaseAlphatree"<<endl;
    AlphaForest::getAlphaforest()->releaseAlphaTree(alphatreeId);
    //cout<<"finish releaseAlphatree"<<endl;
}

int encodeAlphatree(int alphatreeId, char* out){
    //cout<<"start encodeAlphatree"<<endl;
    AlphaForest::getAlphaforest()->encodeAlphaTree(alphatreeId, out);
    //cout<<"finish encodeAlphatree"<<endl;
    return strlen(out);
}

void decodeAlphatree(int alphaTreeId, const char *line){
    //cout<<"start decodeAlphatree"<<endl;
    AlphaForest::getAlphaforest()->decode(alphaTreeId, line);
    //cout<<"finish decodeAlphatree"<<endl;
}

void decodeSubAlphatree(int alphaTreeId, const char *name, const char *line){
    //cout<<"start decodeSubAlphatree"<<endl;
    AlphaForest::getAlphaforest()->decode(alphaTreeId, line, name);
    //cout<<"finish decodeSubAlphatree"<<endl;
}

int getRoot(int alphaTreeId){
    return AlphaForest::getAlphaforest()->getRoot(alphaTreeId);
}

int getNodeDes(int alphaTreeId, int nodeId, char* name, char* coff, int* childList){
    return AlphaForest::getAlphaforest()->getNodeDes(alphaTreeId, nodeId, name, coff, childList);
}

void addStock(const char* code, const char* market, const char* industry, const char* concept,
              const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
              int size, int totals){
    //cout<<"start addStock"<<endl;
    //if(market)
    //    std::cout<<code<<" "<<market<<" "<<industry<<" "<<concept<<" open("<<open[0]<<"~"<<open[size-1]<<") volume("<<volume[0]<<"~"<<volume[size-1]<<") vwap("<<vwap[0]<<"~"<<vwap[size-1]<<") returns("<<returns[0]<<"~"<<returns[size-1]<<")"<<endl;
    AlphaForest::getAlphaforest()->addStock(code, market, industry, concept, open, high, low, close, volume, vwap, returns, size, totals);
    //cout<<"finish addStock"<<endl;
}

void calClassifiedData(){
    //cout<<"start calClassifiedData"<<endl;
    AlphaForest::getAlphaforest()->calClassifiedData();
    //cout<<"finish calClassifiedData"<<endl;
}

int getCodes(int dayBefore, int watchFutureNum, int historyNum, int sampleNum, char* codes){
    //cout<<"start getCodes"<<endl;
    int codeNum = AlphaForest::getAlphaforest()->getCodes(dayBefore, watchFutureNum, historyNum, sampleNum, codes);
    //cout<<"finish getCodes"<<endl;
    return codeNum;
}

//void getAlpha(int alphaTreeId, int dayBefore, int sampleSize, float* alpha, const char* codes, int stockSize){
//    float* res = AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, dayBefore, sampleSize, codes, stockSize);
//    memcpy(alpha, res, sampleSize * stockSize * sizeof(float));
//}

void calAlpha(int alphaTreeId, int cacheMemoryId, int flagCacheId, int dayBefore, int sampleSize, const char* codes, int stockSize, float* alpha){
    //cout<<"start calAlpha"<<endl;
    const float* res = AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheMemoryId, flagCacheId, dayBefore, sampleSize, codes, stockSize);
    memcpy(alpha, res, sampleSize * stockSize * sizeof(float));
    //cout<<"finish calAlpha"<<endl;
}


void getAlpha(int alphaTreeId, int nodeId, int cacheMemoryId, size_t sampleSize, size_t stockSize, float* alpha){
    //cout<<"start getAlpha"<<endl;
    const float* res = AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, nodeId, cacheMemoryId, sampleSize, stockSize);
    memcpy(alpha, res, sampleSize * stockSize * sizeof(float));
    //cout<<"finish getAlpha"<<endl;
}

int useCacheMemory(){
    //cout<<"start useCacheMemory"<<endl;
    int memoryId = AlphaForest::getAlphaforest()->getAlphaTreeMemoryCache()->useCacheMemory();
    //cout<<"finish useCacheMemory"<<endl;
    return memoryId;
}

void releaseCacheMemory(int cacheMemoryId){
    //cout<<"start releaseCacheMemory"<<endl;
    AlphaForest::getAlphaforest()->getAlphaTreeMemoryCache()->releaseCacheMemory(cacheMemoryId);
    //cout<<"finish releaseCacheMemory"<<endl;
}

int useFlagCache(){
    //cout<<"start useFlagCache"<<endl;
    int flagId = AlphaForest::getAlphaforest()->getFlagCache()->useCacheMemory();
    //cout<<"finish useFlagCache"<<endl;
    return flagId;
}

void releaseFlagCache(int flagCacheId){
    //cout<<"start releaseFlagCache"<<endl;
    AlphaForest::getAlphaforest()->getFlagCache()->releaseCacheMemory(flagCacheId);
    //cout<<"finish releaseFlagCache"<<endl;
}

int useCodesCache(){
    //cout<<"start useCodesCache"<<endl;
    int codeCacheId = AlphaForest::getAlphaforest()->getCodesCache()->useCacheMemory();
    //cout<<"finish useCodesCache"<<endl;
    return codeCacheId;
}

void releaseCodesCache(int codesCacheId){
    //cout<<"start releaseCodesCache"<<endl;
    AlphaForest::getAlphaforest()->getCodesCache()->releaseCacheMemory(codesCacheId);
    //cout<<"finish releaseCodesCache"<<endl;
}

int summarySubAlphaTree(const int* alphatreeIds, int len, int minDepth, char* subAlphatreeStr){
    //cout<<"start summarySubAlphaTree"<<endl;
    int subAlphaTreeNum = AlphaForest::getAlphaforest()->summarySubAlphaTree(alphatreeIds, len, minDepth, subAlphatreeStr);
    //cout<<"finish summarySubAlphaTree"<<endl;
    return subAlphaTreeNum;
}

void initializeAlphaexaminer(){
    //cout<<"start initializeAlphaexaminer"<<endl;
    AlphaExaminer::initialize(AlphaForest::getAlphaforest());
    //cout<<"finish initializeAlphaexaminer"<<endl;
}

void releaseAlphaexaminer(){
    //cout<<"start releaseAlphaexaminer"<<endl;
    AlphaExaminer::release();
    //cout<<"finish releaseAlphaexaminer"<<endl;
}

float evalAlphaTree(int alphatreeId,int cacheMemoryId, int flagCacheId, int codesId, int futureNum, int evalTime, int trainDateSize, int testDateSize, int minSieveNum, int maxSieveNum, int punishNum){
    //cout<<"start evalAlphaTree"<<endl;
    float score = AlphaExaminer::getAlphaexaminer()->eval(alphatreeId, cacheMemoryId, flagCacheId, codesId, futureNum, evalTime, trainDateSize,testDateSize, minSieveNum, maxSieveNum, punishNum).get();
    //cout<<"finish evalAlphaTree"<<endl;
    return score;
}

void getTarget(int futureIndex, int dayBefore, int sampleNum, int stockNum, float* dst, const char* codes){
    //cout<<"start getTarget"<<endl;
    AlphaForest::getAlphaforest()->getAlphaDataBase()->getTarget(futureIndex, dayBefore, sampleNum, stockNum, dst, codes);
    //cout<<"finish getTarget"<<endl;
}