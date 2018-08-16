//
// Created by yanyu on 2017/7/20.
//

#define ML

#include "alphaforest.h"
#include <iostream>
#include "alphagraph.h"

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

int DLLEXPORT getAllDays(){ return AlphaForest::getAlphaforest()->getAlphaDataBase()->getDays();}

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


//float DLLEXPORT optimizeAlpha(int alphaTreeId, int cacheId, const char *rootName, int dayBefore, int sampleSize, const char *codes, size_t stockSize, float exploteRatio, int errTryTime){
//    return AlphaForest::getAlphaforest()->optimizeAlpha(alphaTreeId, cacheId, rootName, dayBefore, sampleSize, codes, stockSize, exploteRatio, errTryTime);
//}

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

//draw graph------------------------------------------------------
void DLLEXPORT initializeAlphaGraph(){
    AlphaGraph::initialize(AlphaForest::getAlphaforest());
}


int DLLEXPORT useAlphaPic(const char* signName, const char* features, int featureSize, int dayBefore, int sampleSize){
    return AlphaGraph::getAlphaGraph()->useAlphaPic(signName, features, featureSize, dayBefore, sampleSize);
}

void DLLEXPORT getKLinePic(int picId, const char* signName, const char* openElements, const char* highElements, const char* lowElements, const char* closeElements, int elementNum, int dayBefore, int sampleSize, float* outPic, int column, float maxStdScale){
    AlphaGraph::getAlphaGraph()->getKLinePic(picId, signName, openElements, highElements, lowElements, closeElements, elementNum, dayBefore, sampleSize, outPic, column, maxStdScale);
}

void DLLEXPORT getTrendPic(int picId, const char* signName, const char* elements, int elementNum, int dayBefore, int sampleSize, float* outPic, int column, float maxStdScale){
    AlphaGraph::getAlphaGraph()->getTrendPic(picId, signName, elements, elementNum, dayBefore, sampleSize, outPic, column, maxStdScale);
}

void DLLEXPORT releaseAlphaPic(int id){
    AlphaGraph::getAlphaGraph()->releaseAlphaPic(id);
}

void DLLEXPORT releaseAlphaGraph(){
    AlphaGraph::release();
}

float DLLEXPORT getCorrelation(const char* signName, const char* a, const char* b, int daybefore, int sampleSize, int sampleTime){
    AlphaForest* af = AlphaForest::getAlphaforest();
    int aId = af->useAlphaTree();
    int bId = af->useAlphaTree();
    af->decode(aId, "t", a);
    af->decode(bId, "t", b);

    float corr = 0;
    for(int i = 0; i < sampleTime; ++i){
        AlphaSignIterator afeature(af, "t", signName, aId, daybefore + i * sampleSize, sampleSize, 0, af->getAlphaDataBase()->getSignNum(daybefore + i * sampleSize, sampleSize, signName));
        AlphaSignIterator bfeature(af, "t", signName, bId, daybefore + i * sampleSize, sampleSize, 0, af->getAlphaDataBase()->getSignNum(daybefore + i * sampleSize, sampleSize, signName));

        float curCorr = AlphaSignIterator::getCorrelation(&afeature, &bfeature);
        //cout<<curCorr<<"/"<<corr<<endl;
        if(fabsf(curCorr) > fabsf(corr)){
            corr = curCorr;
        }
    }

    af->releaseAlphaTree(aId);
    af->releaseAlphaTree(bId);
    //cout<<endl;
    return corr;
}

float DLLEXPORT getDistinguish(const char* signName, const char* feature, const char* target, int daybefore, int sampleSize, int sampleTime){
    AlphaForest* af = AlphaForest::getAlphaforest();
    int alphatreeId = af->useAlphaTree();
    int targetId = af->useAlphaTree();

    af->decode(alphatreeId, "t", feature);
    af->decode(targetId, "t", target);
    float distinguish = AlphaSignIterator::getDistinguish(af, signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime);
    af->releaseAlphaTree(alphatreeId);
    af->releaseAlphaTree(targetId);
    return distinguish;
}

int DLLEXPORT optimizeDistinguish(const char* signName, const char* feature, const char* target, int daybefore, int sampleSize, int sampleTime,  char* outFeature, int maxHistoryDays = 75, float exploteRatio = 0.1f, int errTryTime = 64){
    AlphaForest* af = AlphaForest::getAlphaforest();
    int alphatreeId = af->useAlphaTree();
    auto* alphatree = af->getAlphaTree(alphatreeId);
    int targetId = af->useAlphaTree();
    af->decode(alphatreeId, "t", feature);
    af->decode(targetId, "t", target);

    float* bestCoffList = new float[alphatree->getCoffSize()];
    for(int i = 0; i < alphatree->getCoffSize(); ++i){
        bestCoffList[i] = alphatree->getCoff(i);
    }

    float bestRes = AlphaSignIterator::getDistinguish(af, signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime);

    if(alphatree->getCoffSize() > 0){
        RandomChoose rc = RandomChoose(2 * alphatree->getCoffSize());

        auto curErrTryTime = errTryTime;
        while (curErrTryTime > 0){
            //修改参数
            float lastCoffValue = NAN;
            int curIndex = 0;
            bool isAdd = false;
            while(isnan(lastCoffValue)){
                curIndex = rc.choose();
                isAdd = curIndex < alphatree->getCoffSize();
                curIndex = curIndex % alphatree->getCoffSize();
                if(isAdd && alphatree->getCoff(curIndex) < alphatree->getMaxCoff(curIndex)){
                    lastCoffValue = alphatree->getCoff(curIndex);
                    float curCoff = lastCoffValue;
                    if(alphatree->getCoffUnit(curIndex) == CoffUnit::COFF_VAR){
                        curCoff += 0.016f;
                    } else {
                        curCoff += 1.f;
                    }
                    alphatree->setCoff(curIndex, std::min(curCoff, alphatree->getMaxCoff(curIndex)));
                    if(alphatree->getMaxHistoryDays() > maxHistoryDays){
                        alphatree->setCoff(curIndex, lastCoffValue);
                    }
                }
                if(!isAdd && alphatree->getCoff(curIndex) > alphatree->getMinCoff(curIndex)){
                    lastCoffValue = alphatree->getCoff(curIndex);
                    float curCoff = lastCoffValue;
                    if(alphatree->getCoffUnit(curIndex) == CoffUnit::COFF_VAR){
                        curCoff -= 0.016f;
                    } else {
                        curCoff -= 1;
                    }
                    alphatree->setCoff(curIndex, std::max(curCoff, alphatree->getMinCoff(curIndex)));
                }
                if(isnan(lastCoffValue)){
                    curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                    rc.reduce(curIndex);
                }

            }

            float res = AlphaSignIterator::getDistinguish(af, signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime);

            if(res > bestRes){
                //cout<<"best res "<<res<<endl;
                curErrTryTime = errTryTime;
                bestRes = res;
                for(int i = 0; i < alphatree->getCoffSize(); ++i){
                    bestCoffList[i] = alphatree->getCoff(i);
                }
                //根据当前情况决定调整该参数的概率
                curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                rc.add(curIndex);
            } else{
                --curErrTryTime;
                if(!rc.isExplote(exploteRatio)){
                    //恢复现场
                    alphatree->setCoff(curIndex, lastCoffValue);
                }
                curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                rc.reduce(curIndex);
            }

        }

        for(int i = 0; i < alphatree->getCoffSize(); ++i){
            alphatree->setCoff(i, bestCoffList[i]);
        }
    }
    alphatree->encode("t", outFeature);

    delete[] bestCoffList;
    af->releaseAlphaTree(alphatreeId);
    af->releaseAlphaTree(targetId);
    return strlen(outFeature);
}

float DLLEXPORT getConfidence(const char* signName, const char* feature, const char* target, int daybefore, int sampleSize, int sampleTime, float support, float stdScale){
    AlphaForest* af = AlphaForest::getAlphaforest();
    int alphatreeId = af->useAlphaTree();
    int targetId = af->useAlphaTree();
    af->decode(alphatreeId, "t", feature);
    af->decode(targetId, "t", target);

    float confidence = AlphaSignIterator::getConfidence(af, signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime, support, stdScale);

    af->releaseAlphaTree(alphatreeId);
    af->releaseAlphaTree(targetId);
    return confidence;
}

int DLLEXPORT optimizeConfidence(const char* signName, const char* feature, const char* target, int daybefore, int sampleSize, int sampleTime, float support, float stdScale,  char* outFeature, float exploteRatio = 0.1f, int errTryTime = 64){
    AlphaForest* af = AlphaForest::getAlphaforest();
    int alphatreeId = af->useAlphaTree();
    auto* alphatree = af->getAlphaTree(alphatreeId);
    int targetId = af->useAlphaTree();
    af->decode(alphatreeId, "t", feature);
    af->decode(targetId, "t", target);

    float* bestCoffList = new float[alphatree->getCoffSize()];
    for(int i = 0; i < alphatree->getCoffSize(); ++i){
        bestCoffList[i] = alphatree->getCoff(i);
    }

    float bestRes = AlphaSignIterator::getConfidence(af, signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime, support, stdScale);

    if(alphatree->getCoffSize() > 0){
        RandomChoose rc = RandomChoose(2 * alphatree->getCoffSize());

        auto curErrTryTime = errTryTime;
        while (curErrTryTime > 0){
            //修改参数
            float lastCoffValue = NAN;
            int curIndex = 0;
            bool isAdd = false;
            while(isnan(lastCoffValue)){
                curIndex = rc.choose();
                isAdd = curIndex < alphatree->getCoffSize();
                curIndex = curIndex % alphatree->getCoffSize();
                if(isAdd && alphatree->getCoff(curIndex) < alphatree->getMaxCoff(curIndex)){
                    lastCoffValue = alphatree->getCoff(curIndex);
                    float curCoff = lastCoffValue;
                    if(alphatree->getCoffUnit(curIndex) == CoffUnit::COFF_VAR){
                        curCoff += 0.016f;
                    } else {
                        curCoff += 1.f;
                    }
                    alphatree->setCoff(curIndex, std::min(curCoff, alphatree->getMaxCoff(curIndex)));
                }
                if(!isAdd && alphatree->getCoff(curIndex) > alphatree->getMinCoff(curIndex)){
                    lastCoffValue = alphatree->getCoff(curIndex);
                    float curCoff = lastCoffValue;
                    if(alphatree->getCoffUnit(curIndex) == CoffUnit::COFF_VAR){
                        curCoff -= 0.016f;
                    } else {
                        curCoff -= 1;
                    }
                    alphatree->setCoff(curIndex, std::max(curCoff, alphatree->getMinCoff(curIndex)));
                }
                if(isnan(lastCoffValue)){
                    curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                    rc.reduce(curIndex);
                }

            }

            float res = AlphaSignIterator::getConfidence(af, signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime, support, stdScale);

            if(res > bestRes){
                //cout<<"best res "<<res<<endl;
                curErrTryTime = errTryTime;
                bestRes = res;
                for(int i = 0; i < alphatree->getCoffSize(); ++i){
                    bestCoffList[i] = alphatree->getCoff(i);
                }
                //根据当前情况决定调整该参数的概率
                curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                rc.add(curIndex);
            } else{
                --curErrTryTime;
                if(!rc.isExplote(exploteRatio)){
                    //恢复现场
                    alphatree->setCoff(curIndex, lastCoffValue);
                }
                curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                rc.reduce(curIndex);
            }

        }

        for(int i = 0; i < alphatree->getCoffSize(); ++i){
            alphatree->setCoff(i, bestCoffList[i]);
        }
    }
    alphatree->encode("t", outFeature);

    delete[] bestCoffList;
    af->releaseAlphaTree(alphatreeId);
    af->releaseAlphaTree(targetId);
    return strlen(outFeature);
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

void DLLEXPORT defaultTrainAlphaGBDT(int daybefore, int sampleSize, const char* target, const char* signName){
    AlphaGBDT::getAlphaGBDT()->train(daybefore, sampleSize, nullptr, target, signName, 32, 64, 8, 1, 1, 2, 1, 2048);
}

float DLLEXPORT trainAndEvalAlphaGBDT(int daybefore, int sampleSize, int evalDaybefore, int evalSampleSize, const char* weight, const char* target, const char* signName,
                             int barSize, float minWeight, int maxDepth, float samplePercent, float featurePercent, int boostNum, float boostWeightScale, int cacheSize){
    return AlphaGBDT::getAlphaGBDT()->trainAndEval(daybefore, sampleSize, evalDaybefore, evalSampleSize, weight, target, signName, barSize, minWeight, maxDepth, samplePercent, featurePercent, boostNum, boostWeightScale, cacheSize);
}

float DLLEXPORT evalAlphaGBDT(int evalDaybefore, int evalSampleSize, const char* target, const char* signName,int cacheSize){
    return AlphaGBDT::getAlphaGBDT()->eval(evalDaybefore, evalSampleSize, target, signName, cacheSize);
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

//void DLLEXPORT initializeAlphaFilter(const char *alphatreeList, const int* alphatreeFlag,  int alphatreeNum, const char *target, const char *open){
//    AlphaFilter::initialize(AlphaForest::getAlphaforest(), alphatreeList, alphatreeFlag, alphatreeNum, target, open);
//}
//
//void DLLEXPORT releaseAlphaFilter(){
//    AlphaFilter::release();
//}
//
//void DLLEXPORT trainAlphaFilter(const char* signName, int daybefore, int sampleSize, int sampleTime, float support, float confidence, float firstHeroConfidence, float secondHeroConfidence){
//    AlphaFilter::getAlphaFilter()->train(signName, daybefore, sampleSize, sampleTime, support, confidence, firstHeroConfidence, secondHeroConfidence);
//}
//
//int DLLEXPORT predAlphaFilter(const char* signName, int daybefore, int sampleSize, float* predOut, float* openMinValue, float* openMaxValue){
//    return AlphaFilter::getAlphaFilter()->pred(signName, daybefore, sampleSize, predOut, openMinValue, openMaxValue);
//}
//
//void DLLEXPORT saveFilterModel(const char* path){
//    AlphaFilter::getAlphaFilter()->saveModel(path);
//}
//
//void DLLEXPORT loadFilterModel(const char* path){
//    AlphaFilter::getAlphaFilter()->loadModel(path);
//}
//
//int DLLEXPORT alphaFilter2String(char* pout){
//    return AlphaFilter::getAlphaFilter()->tostring(pout);
//}
#endif
}
