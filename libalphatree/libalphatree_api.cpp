//
// Created by yanyu on 2017/7/20.
//

#define ML

#include "alphaforest.h"
#include "alphabi.h"
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


extern "C"
{

/*
 * 股票数据在做加减乘除等等计算时会有很多中间结果，需要巨大的缓存空间，这里先申请cacheSize个缓存空间
 * 这样就可以用多线程同时计算cacheSize个公式集合
 */
void DLLEXPORT initializeAlphaforest(int cacheSize) {
    AlphaForest::initialize(cacheSize);
    AlphaBI::initialize();
}


void DLLEXPORT releaseAlphaforest() {
    AlphaBI::release();
    AlphaForest::release();
}

/*
 * 加载股票的描述文件，只要包括股票代码是什么，对应的市场代码是什么，包含多少条数据等基本信息
 * */
void DLLEXPORT loadDataBase(const char *path) {
    AlphaForest::getAlphaforest()->getAlphaDataBase()->loadDataBase(path);
}

/*
 * 将多个csv文件中的某一列数据变成一个二进制文件作为一个“特征”，比如日期，比如收盘价。
 * 文件格式很简单，就是按照loadDataBase中加载的股票顺序，将每个条数据再按先后写入。
 * 比如有三中股票收盘价分别是，000001【1,2,3,4】,000002[2,3,2,1】，000003[7,2】
 * 二进制文件中的数据就是【1,2,3,4,2,3,2,1,7,2】
 * */
void DLLEXPORT csv2binary(const char *path, const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->csv2binary(path, featureName);
}

/*
 * 缓存缺失数据描述，首元素的值是0，如果当前数据前n天的数据都是完整的，文件中值就是n，如果缺失了m天，特征就是-m
 * 比如有三中股票收盘价分别是，000001【1,2,x,x,3,4】,000002[2,3,2,x,1】，000003[7,2】（x表示缺失）
 * 二进制文件数据就是[0,1,-2,1,0,1,2,-1,0,1]
 * */
void DLLEXPORT cacheMiss(){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->miss2binary();
}

/*
 * 每个股票每天的数据分配一个随机数，用来取样
 * */
void DLLEXPORT cacheRand(){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->rand2binary();
}

/*
 * 把一个“特征”加载到内存（比如收盘价或者某些高级的特征），公式中如果用到了这个特征就可以不用读文件了
 * */
void DLLEXPORT loadFeature(const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->loadFeature(featureName);
}

/*
 * 把一个特征从内存中卸载掉
 * */
void DLLEXPORT releaseFeature(const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->releaseFeature(featureName);
}


//更新内存中的特征
void DLLEXPORT updateFeature(const char* featureName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->updateFeature(featureName);
}

//卸载所有特征
void DLLEXPORT releaseAllFeature(){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->releaseAllFeature();
}

//创建一个alphatree（公式集），并得到它的id
int DLLEXPORT createAlphatree() {
    return AlphaForest::getAlphaforest()->useAlphaTree();
}

//回收一个alphatree，下次再需要用到时还可以再次使用，不会反复申请释放内存
void DLLEXPORT releaseAlphatree(int alphatreeId) {
    AlphaForest::getAlphaforest()->releaseAlphaTree(alphatreeId);
}

/*
 * 给alphatree添加一条公式，比如rootName="f1",line="((close - open) / 2)"
 * alphatree会将这条公式解码成一棵公式树，“f1”就是这棵树的名字。alphatree中可以有多棵公式树。
 * */
void DLLEXPORT decodeAlphatree(int alphaTreeId, const char *rootName, const char *line) {
    AlphaForest::getAlphaforest()->decode(alphaTreeId, rootName, line);
}

//将一课公式树重新编码成字符串
int DLLEXPORT encodeAlphatree(int alphatreeId, const char *rootName, char *out) {
    const char *res = AlphaForest::getAlphaforest()->encodeAlphaTree(alphatreeId, rootName, out);
    return strlen(res);
}

//创建缓存，返回它的id。缓存是用了记录alphatree各种运算的中间结果的内存空间。
int DLLEXPORT useCache() {
    return AlphaForest::getAlphaforest()->useCache();
}

//回收缓存（不会释放，下次还可以接着使用）
void DLLEXPORT releaseCache(int cacheId) {
    AlphaForest::getAlphaforest()->releaseCache(cacheId);
}

/*
 * 计算alphatree中所有的公式，并将结果保持到缓存中。
 * alphaTreeId：需要计算的alphatree
 * cacheId：分配给alphatree用来存放中间结果的缓存
 * dayBefore：计算多少天前的数据
 * sampleSize：计算多少天的数据
 * codes：所有需要计算的股票码，比如"000001\0000002\0000003\0"每个股票码以“\0”结尾
 * */
void DLLEXPORT calAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, const char *codes, int stockSize) {
    AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, codes, stockSize);
}

/*
 * 得到某个公式的计算结果
 * alphaTreeId：公式所在的alphatree（公式集）
 * rootName：公式名
 * cacheId：分配给alphatree用来存放中间结果的缓存
 * alpha：输出计算结果的内存。
 * 计算结果的格式是：第一天所有股票的计算结果[c1_1,c2_1,c3_1,...,cn_1]，第二天所有股票的计算结果[c1_2,c2_2,c3_2,...,cn_2],...
 * */
int DLLEXPORT getAlpha(int alphaTreeId, const char *rootName, int cacheId, float *alpha) {
    const float *res = AlphaForest::getAlphaforest()->getAlpha(alphaTreeId, rootName, cacheId);
    auto *cache = AlphaForest::getAlphaforest()->getCache(cacheId);
    int dataSize = cache->getAlphaDays() * cache->stockSize;
    memcpy(alpha, res, dataSize * sizeof(float));
    return dataSize;
}

//等待所有线程把alphatree中的所有公式计算完成
void DLLEXPORT synchroAlpha(int alphaTreeId, int cacheId){
    AlphaForest::getAlphaforest()->synchroAlpha(alphaTreeId, cacheId);
}

/*
 * 某个公式对于所有股票所有日期的计算结果保存到二进制文件，变成一个“特征”（类似之前的收盘价也是一个特征）
 * alphaTreeId：这个公式所在的alphatree
 * cacheId：用来保存中间结果的缓存
 * featureName：公式名，见decodeAlphatree注释中的“f1”，同时也是缓存成特征的特征名
 * */
void DLLEXPORT cacheAlpha(int alphaTreeId, int cacheId, const char* featureName) {
    AlphaForest::getAlphaforest()->cacheAlpha(alphaTreeId, cacheId, featureName);
}

/*
 * 将公式的计算结果保存成信号。
 * 信号是用来过滤数据的，比如某个公式f2=(close > open)
 * 把f2保存成信号我们就能把每天收盘大于开盘的股票数据筛选出来，并在此基础上再做别的运算。
 * alphaTreeId：这个公式所在的alphatree
 * cacheId：用来保存中间结果的缓存
 * signName：公式名，同时也是需要保存成信号二进制文件的文件名
 * 信号文件的格式比较复杂，如下：
 * 文件头：（有多少个交易日就有多少数据）
 * [第1个交易日和它之前所有信号数量,第2个交易日和它之前所有信号数量,......]
 * 文件体：
 * [第1个交易日第1个信号的文件偏移，第1个交易日第2个信号的文件偏移，……，第n个交易日第m个信号的文件偏移]
 * 文件偏移定义：
 * 就是数据在特征文件中的序号，见csv2binary中的注释
 * */
void DLLEXPORT cacheSign(int alphaTreeId, int cacheId, const char* signName){
    AlphaForest::getAlphaforest()->cacheSign(alphaTreeId, cacheId, signName);
}

//同上，只不过指定了能发出信号的股票代码
void DLLEXPORT cacheCodesSign(int alphaTreeId, int cacheId, const char* signName, const char* codes, int codesNum){
    AlphaForest::getAlphaforest()->cacheSign(alphaTreeId, cacheId, signName, codes, codesNum);
}

/*
 * 计算在某个信号过滤之下，alphatree的所有公式
 * alphaTreeId：需要计算的alphatree
 * cacheId：分配给alphatree用来存放中间结果的缓存
 * dayBefore：计算多少天前的数据
 * sampleSize：计算多少天的数据
 * startIndex：信号中有很多数据，从第几条开始计算
 * signNum：一共计算多少条数据
 * signHistoryDays：同时计算信号发生前多少天的数据
 * signName：信号名
 *
 * 注意，经过计算后，getAlpha的到的数据结构就是：
 * 第1天：[信号1的数据计算结果，信号2的数据计算结果，...],第2天：[信号1的数据计算结果，信号2的数据计算结果，...],...第signHistoryDays天[...]
 * */
void DLLEXPORT calSignAlpha(int alphaTreeId, int cacheId, int dayBefore, int sampleSize, int startIndex, int signNum, int signHistoryDays, const char* signName){
    AlphaForest::getAlphaforest()->calAlpha(alphaTreeId, cacheId, dayBefore, sampleSize, startIndex, signNum, signHistoryDays, signName);
}

//将信号加载到内存
void DLLEXPORT loadSign(const char* signName){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->loadSign(signName);
}

//从内存中卸载所有信号
void DLLEXPORT releaseAllSign(){
    AlphaForest::getAlphaforest()->getAlphaDataBase()->releaseAllSign();
}

/*
 * 得到信号数量
 * dayBefore：计算多少天前的数据
 * sampleSize：计算多少天的数据
 * */
int DLLEXPORT getSignNum(int dayBefore, int sampleSize, const char* signName){
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
}


//得到所有股票代码，并返回数量
int DLLEXPORT getStockCodes(char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getStockCodes(codes);
}

//得到某段时间发出信号的股票id
int DLLEXPORT getStockIds(int dayBefore, int sampleSize, const char* signName, int* dst){
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getStockIds(dayBefore, sampleSize, signName, dst);
}

//得到股票id对应的股票代码
int DLLEXPORT getCode(int id, char* codes){
    strcpy(codes, AlphaForest::getAlphaforest()->getAlphaDataBase()->getCode(id));
    return strlen(codes);
}

//得到所有市场的股票代码，比如上证就是0000001
int DLLEXPORT getMarketCodes(const char *marketName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getMarketCodes(marketName, codes);
}

//得到所有行业的股票代码
int DLLEXPORT getIndustryCodes(const char *industryName, char *codes) {
    return AlphaForest::getAlphaforest()->getAlphaDataBase()->getIndustryCodes(industryName, codes);
}

//得到某个alphatree计算时最多使用的历史数据天数
int DLLEXPORT getMaxHistoryDays(int alphaTreeId) { return AlphaForest::getAlphaforest()->getMaxHistoryDays(alphaTreeId); }

//得到所有交易日的数量
int DLLEXPORT getAllDays(){ return AlphaForest::getAlphaforest()->getAlphaDataBase()->getDays();}


//void DLLEXPORT getBag(const char* codes, int stockSize, const char* feature, const char* signName, int dayBefore, int sampleSize, int bagNum, float* bags){
//    int alphatreeId = AlphaForest::getAlphaforest()->useAlphaTree();
//    AlphaForest::getAlphaforest()->decode(alphatreeId,"sign",feature);
//    size_t signNum = AlphaForest::getAlphaforest()->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
//    AlphaSignIterator asi(AlphaForest::getAlphaforest(), "sign", signName, alphatreeId, dayBefore, sampleSize, 0, signNum);
//    getBags(&asi, bags, bagNum);
//    AlphaForest::getAlphaforest()->releaseAlphaTree(alphatreeId);
//}


/*
 * 得到某个信号下，a、b两个特征的相关性
 * daybefore：使用多少天前的数据
 * sampleSize：计算相关性使用的天数
 * sampleTime：一共计算多少次（使用相关性最大的一次作为结果）
 * 比如：
 * sampleSize=128，sampleTime=2，表示第一个128天计算一个相关性，往前推128天后再计算第二个128天的相关性，返回最大的那个
 * */
float DLLEXPORT getCorrelation(const char* signName, const char* a, const char* b, int daybefore, int sampleSize, int sampleTime){
    return AlphaBI::getAlphaBI()->getCorrelation(signName, a, b, daybefore, sampleSize, sampleTime);
}

/*
 * 计算某个信号下(signName)，某个特征(feature)对于某个分类(target)的区分度
 * daybefore：使用多少天前的数据
 * sampleSize：计算相关性使用的天数
 * sampleTime：一共计算多少次（使用贡献度第allowFailTime小的一次作为结果）
 * allowFailTime：选择第几小的数据作为返回
 * 比如：
 * sampleSize=128，sampleTime=2，表示第一个128天计算一个贡献度，往前推128天后再计算第二个128天的贡献度
 * */
float DLLEXPORT getDiscrimination(const char* signName, const char* feature, const char* target, int daybefore, int sampleSize, int sampleTime, float support){
    return AlphaBI::getAlphaBI()->getDiscrimination(signName, feature, target, daybefore, sampleSize, sampleTime, support);
}

//优化feature中的参数，使得贡献度最大
int DLLEXPORT optimizeDiscrimination(const char* signName, const char* feature, const char* target, int daybefore, int sampleSize, int sampleTime, float support,  char* outFeature, int maxHistoryDays = 75, float exploteRatio = 0.1f, int errTryTime = 64){
    return AlphaBI::getAlphaBI()->optimizeDiscrimination(signName, feature, target, daybefore, sampleSize, sampleTime, support, outFeature, maxHistoryDays, exploteRatio, errTryTime);
}

/*
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
*/

//将xgboost集成进来，由于老大坚持用第三方xgb，所以暂时没用，不过是可用的，并已经测试过------------------------------------------------------------------------
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

#endif
}
