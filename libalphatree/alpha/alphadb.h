//
// Created by yanyu on 2017/7/29.
//

#ifndef ALPHATREE_ALPHADB_H
#define ALPHATREE_ALPHADB_H

#include <map>
#include <stdlib.h>
#include <string>
#include <map>
#include <vector>
#include "base.h"
#include <iostream>
using namespace std;

const size_t CODE_LEN = 64;

enum class LeafDataType{
    OPEN = 0,
    HIGH = 1,
    LOW = 2,
    CLOSE = 3,
    VOLUME = 4,
    VWAP = 5,
    RETURNS = 6,
    ALPHA = 7,
    BETA = 8,
};

LeafDataType str2LeafDataType(const char* str){
    if(strcmp(str,"open") == 0){
        return LeafDataType::OPEN;
    } else if(strcmp(str,"high") == 0){
        return LeafDataType::HIGH;
    } else if(strcmp(str,"low") == 0){
        return LeafDataType::LOW;
    } else if(strcmp(str,"close") == 0){
        return LeafDataType::CLOSE;
    } else if(strcmp(str,"volume") == 0){
        return LeafDataType::VOLUME;
    } else if(strcmp(str,"vwap") == 0){
        return LeafDataType::VWAP;
    } else if(strcmp(str,"returns") == 0){
        return LeafDataType::RETURNS;
    } else if (strcmp(str,"alpha") == 0){
        return LeafDataType::ALPHA;
    } else if (strcmp(str,"beta") == 0){
        return LeafDataType::BETA;
    } else {
        throw "IndClass Error!";
    }
}

class Stock{
    public:
        Stock(const char* code, const char* market, const char* industry, const char*concept,
              const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
              int size, int totals
        ): alpha(nullptr), beta(nullptr), target(nullptr){
            strcpy(this->code, code);
            if(market != nullptr)
                strcpy(this->market, market);
            else
                this->market[0] = 0;
            if(industry != nullptr)
                strcpy(this->industry, industry);
            else
                this->industry[0] = 0;
            if(concept != nullptr)
                strcpy(this->concept, concept);
            else
                this->concept[0] = 0;
            this->size = size;
            this->totals = totals;

            copyData(this->open, open, size);
            copyData(this->high, high, size);
            copyData(this->low, low, size);
            copyData(this->close, close, size);
            copyData(this->volume, volume, size);
            copyData(this->vwap, vwap, size);
            copyData(this->returns, returns, size);
        }

        Stock(const char* code, vector<Stock*>& stocks): alpha(nullptr), beta(nullptr), target(nullptr){
            strcpy(this->code, code);
            this->market[0] = 0;
            this->industry[0] = 0;
            this->concept[0] = 0;
            int maxLength = 0;
            float allWeight = 0;
            auto stockNum = stocks.size();
            for(int i = 0; i < stockNum; ++i){
                maxLength = max(stocks[i]->size, maxLength);
                allWeight += stocks[i]->totals * stocks[i]->close[stocks[i]->size-1];
            }
            this->size = maxLength;

            //以最后一天的市值来作为各个股票所在成分
            float* weight = new float[stockNum];
            bool* yestodayIsUse = new bool[stockNum];
            for(int i = 0; i < stockNum; ++i){
                weight[i] = stocks[i]->totals * stocks[i]->close[stocks[i]->size-1] / allWeight;
            }


            open = new float[maxLength];
            high = new float[maxLength];
            low = new float[maxLength];
            close = new float[maxLength];
            volume = new float[maxLength];
            vwap = new float[maxLength];
            returns = new float[maxLength];

            for(int i = 0; i < maxLength; ++i){

                float openPrice = 0;
                float highPrice = 0;
                float lowPrice = 0;
                float closePrice = 0;
                float volumeValue = 0;
                float vwapPrice = 0;
                float returnsValue = 0;

                //先计算今天的开盘
                float realOpenPrice = NONE;
                if(i > 0){
                    //先用昨天有数据今天还有数据的股票计算出今天的开盘价
                    float lastOpenPrice = 0;
                    for(int j = 0; j < stockNum; ++j){
                        int curIndex = stocks[j]->size - (maxLength - i);
                        if(yestodayIsUse[j] && stocks[j]->volume[curIndex] > 0){
                            lastOpenPrice += stocks[j]->open[curIndex-1] * weight[j];
                            openPrice += stocks[j]->open[curIndex] * weight[j];
                        }
                    }
                    realOpenPrice = openPrice * open[i-1]/lastOpenPrice;
                }

                //计算为了得到这个开盘价所需要的缩放
                for(int j = 0; j < stockNum; ++j){
                    if(stocks[j]->size >= maxLength-i){
                        int curIndex = stocks[j]->size - (maxLength - i);
                        openPrice += stocks[j]->open[curIndex] * weight[j];
                        highPrice += stocks[j]->high[curIndex] * weight[j];
                        lowPrice += stocks[j]->low[curIndex] * weight[j];
                        closePrice += stocks[j]->close[curIndex] * weight[j];
                        volumeValue += stocks[j]->volume[curIndex] * weight[j];
                        vwapPrice += stocks[j]->vwap[curIndex] * weight[j];
                        returnsValue += stocks[j]->returns[curIndex] * weight[j];
                        yestodayIsUse[j] = true;
                    } else {
                        yestodayIsUse[j] = false;
                    }
                }

                if(realOpenPrice != NONE){
                    float k = realOpenPrice / openPrice;
                    openPrice *= k;
                    highPrice *= k;
                    lowPrice *= k;
                    closePrice *= k;
                    volumeValue *= k;
                    vwapPrice *= k;
                    returnsValue *= k;
                }

                open[i] = openPrice;
                high[i] = highPrice;
                low[i] = lowPrice;
                close[i] = closePrice;
                volume[i] = volumeValue;
                vwap[i] = vwapPrice;
                returns[i] = returnsValue;
            }

            delete []weight;
        }

        ~Stock(){
            delete []open;
            delete []high;
            delete []low;
            delete []close;
            delete []volume;
            delete []vwap;
            delete []returns;
            if(beta)
                delete []beta;
            if(alpha)
                delete []alpha;
            if(target)
                delete []target;
        }

        const char* getMarket(){ return strlen(market) == 0 ? nullptr : market;}
        const char* getIndustry(){ return strlen(industry) == 0 ? nullptr : industry;}
        const char* getConcept(){ return strlen(concept) == 0 ? nullptr : concept;}

        bool isRealStock(){ return totals != 0;}

        //因为是短线,所以忽略无风险shouy
        void calCAMP(const float* marketReturns, size_t sampleBetaSize){
            beta = new float[size];
            alpha = new float[size];
            memset(beta, 1, size * sizeof(float));
            memset(alpha, 0, size * sizeof(float));
            for(int i = sampleBetaSize-1; i < size; ++i){
                int offset = i - (sampleBetaSize-1);
                lstsq(marketReturns+offset, returns+offset, sampleBetaSize, alpha[i], beta[i]);
            }
        }

        //用相对收益作为target
        void calRelativeAvgReturns(int futureNum, const float* marketReturns){
            target = new float[futureNum * size];
            memset(target, 0, futureNum * size * sizeof(float));
            for(int fId = 1; fId <= futureNum; ++fId){
                float* curTarget = target + (fId-1)*size;
                for(int i = 0; i < size - fId; ++i){
                    for(int j = 1; j <= fId; ++j)
                        if(marketReturns != nullptr)
                            curTarget[i] += logf((returns[i+j] - marketReturns[i+j])+1.f);
                        else
                            curTarget[i] += logf(returns[i+j]+1.f);
                    curTarget[i] /= fId;
                }
            }
        }


        //股票码(可以使用行业)
        char code[CODE_LEN];
        //上证还是深圳
        char market[CODE_LEN];
        //行业
        char industry[CODE_LEN];
        //概念
        char concept[CODE_LEN];
        //总股本
        float totals{0};
        //数据长度
        int size;
        //交易数据,注意不包含日期是因为所有日期都是对齐的!
        float* open;
        float* high;
        float* low;
        float* close;
        float* volume;
        float* vwap;
        float* returns;

        float* alpha;
        float* beta;

        float* target;
    protected:
        inline static void copyData(float*& dst, const float* src, int length){
            dst = new float[length];
            memcpy(dst, src, length * sizeof(float));
        }
};

class AlphaDataBase{
    public:
        AlphaDataBase(int sampleBetaSize, int watchFutureNum):sampleBetaSize_(sampleBetaSize),watchFutureNum_(watchFutureNum){}

        ~AlphaDataBase(){
            for(auto iter = stockMap_.begin(); iter != stockMap_.end(); iter++){
                delete iter->second;
            }
        }
        void addStock(const char* code, const char* market, const char* industry, const char* concept,
                      const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
                      int size, int totals
        ){
            Stock* stock = new Stock(code, market, industry, concept, open, high, low, close, volume, vwap, returns, size, totals);
            if(stock->getMarket()){
                Stock* market = stockMap_[stock->getMarket()];
                stock->calCAMP(market->returns + (market->size - stock->size), sampleBetaSize_);
                stock->calRelativeAvgReturns(watchFutureNum_, market->returns+(market->size-stock->size));
            } else {
                stock->calCAMP(stock->returns, sampleBetaSize_);
                stock->calRelativeAvgReturns(watchFutureNum_, nullptr);
            }
            addStock(stock);
        }
        void addStock(Stock* stock){
            stockMap_[stock->code] = stock;
        }
//        void calFutureData(int sampleBetaSize){
//            for(auto iter = stockMap_.begin(); iter != stockMap_.end(); iter++){
//                Stock* curStock = iter->second;
//                if(curStock->getMarket()){
//                    Stock* market = stockMap_[iter->second->getMarket()];
//                    curStock->calCAMP(market->returns + (market->size - curStock->size), sampleBetaSize);
//                    curStock->calFutureLogAlpha(market->returns + (market->size - curStock->size), sampleBetaSize, maxWatchSize_);
//                }
//            }
//        }
        void calClassifiedData(bool isConcept){
            map<const char*, vector<Stock*>, ptrCmp> subindustryMap;

            for(auto iter = stockMap_.begin(); iter != stockMap_.end(); iter++){
                Stock* curStock = iter->second;

                const char* cKey = isConcept ? curStock->getConcept() : curStock->getIndustry();
                if(cKey){
                    //std::cout<<curStock->code<<" "<<strlen(cKey)<<" ";
                    //std::cout<<cKey<<std::endl;
                    auto iter = subindustryMap.find(cKey);
                    if(iter == subindustryMap.end()){
                        vector<Stock*> v;
                        v.push_back(curStock);
                        subindustryMap[cKey] = v;
                    } else {
                        iter->second.push_back(curStock);
                    }
                }
            }

            for(auto iter = subindustryMap.begin(); iter != subindustryMap.end(); iter++){
                Stock* stock = new Stock(iter->first, iter->second);
                addStock(stock);
            }
            //std::cout<<"finish"<<std::endl;
        }

        static size_t getElementSize(size_t historyNum, size_t sampleNum, size_t watchFutureNum){
            return historyNum + sampleNum - 1 + watchFutureNum;
        }

        size_t getCodes(size_t dayBefore, size_t watchFutureNum, size_t historyNum, size_t sampleNum, char* codes){
            int stockNum = 0;
            char* curCodes = codes;

            int dayNum = getElementSize(historyNum, sampleNum, watchFutureNum);
            int needDay = dayBefore + dayNum;


            for(auto iter = stockMap_.begin(); iter != stockMap_.end(); iter++){
                Stock* curStock = iter->second;
                if(curStock->getMarket()){
                    if(curStock->size >= needDay){
                        float* startVolume = curStock->volume + (curStock->size - needDay);
                        bool hasEmptyData = false;
                        for(int i = 0; i < dayNum; i++){
                            if(startVolume[i] == 0)
                            {
                                hasEmptyData = true;
                                break;
                            }
                        }
                        if(!hasEmptyData){
                            ++stockNum;
                            strcpy(curCodes, iter->first);
                            curCodes = curCodes + strlen(curCodes) + 1;
                        }
                    }
                }
            }
            return stockNum;
        }

        float* getStock(size_t dayBefore, size_t watchFutureNum, size_t historyNum, size_t sampleNum, size_t stockNum, LeafDataType leafDataType, const char* leafDataClass,
                      bool* flag, float* dst, const char* codes){
            const char* curCodes = codes;

            int dayNum = getElementSize(historyNum, sampleNum, watchFutureNum);
            int needDay = dayBefore + dayNum;

            for(int i = 0; i < stockNum; i++){
                Stock* curStock = nullptr;

                if(leafDataClass == nullptr){
                    curStock = stockMap_[curCodes];
                } else if(strcmp(leafDataClass,"IndClass.market") == 0){
                    curStock = stockMap_[stockMap_[curCodes]->getMarket()];
                } else if(strcmp(leafDataClass,"IndClass.industry") == 0){
                    curStock = stockMap_[stockMap_[curCodes]->getIndustry()];
                } else if(strcmp(leafDataClass,"IndClass.concept") == 0){
                    curStock = stockMap_[stockMap_[curCodes]->getConcept()];
                    //cout<<curStock->code<<" "<<strlen(curCodes)<<" "<<dayNum<<" "<<needDay<<" "<<curStock->size<<endl;
                } else {
                    curStock = stockMap_[leafDataClass];
                }

                curCodes = curCodes + (strlen(curCodes) + 1);
                for(int j = 0; j < dayNum; j++){
                    if(flag[j] == false)
                        continue;
                    int targetId = j * stockNum + i;
                    int srcId = curStock->size - needDay + j;
                    switch (leafDataType){
                        case LeafDataType::OPEN:
                            dst[targetId] = curStock->open[srcId];
                            break;
                        case LeafDataType::HIGH:
                            dst[targetId] = curStock->high[srcId];
                            break;
                        case LeafDataType::LOW:
                            dst[targetId] = curStock->low[srcId];
                            break;
                        case LeafDataType::CLOSE:
                            dst[targetId] = curStock->close[srcId];
                            break;
                        case LeafDataType::VOLUME:
                            dst[targetId] = curStock->volume[srcId];
                            break;
                        case LeafDataType::VWAP:
                            dst[targetId] = curStock->vwap[srcId];
                            break;
                        case LeafDataType::RETURNS:
                            dst[targetId] = curStock->returns[srcId];
                            break;
                        case LeafDataType::ALPHA:
                            dst[targetId] = curStock->alpha[srcId];
                            break;
                        case LeafDataType::BETA:
                            dst[targetId] = curStock->beta[srcId];
                            break;
                    }
                }
            }
            return dst;
        }

        float* getTarget(size_t futureIndex, size_t dayBefore, size_t sampleNum, size_t stockNum, float* dst, const char* codes){
            const char* curCodes = codes;
            for(int i = 0; i < stockNum; i++){
                Stock* curStock = nullptr;
                curStock = stockMap_[curCodes];
                int startIndex = curStock->size * (futureIndex+1)  - sampleNum - dayBefore;
                for(int j = 0; j < sampleNum; ++j){
                    int targetId = j * stockNum + i;
                    dst[targetId] = curStock->target[startIndex + j];
                }

                curCodes = curCodes + (strlen(curCodes) + 1);
            }
            return dst;
        }

    protected:
        map<const char*, Stock*, ptrCmp> stockMap_;
        int sampleBetaSize_;
        int watchFutureNum_;
};

#endif //ALPHATREE_ALPHADB_H
