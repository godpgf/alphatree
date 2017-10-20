//
// Created by yanyu on 2017/7/29.
//

#ifndef ALPHATREE_ALPHADB_H
#define ALPHATREE_ALPHADB_H

#include <map>
#include <stdlib.h>
#include <string>
#include <vector>
#include "base.h"
#include <iostream>
#include "../base/hashmap.h"
using namespace std;

//const size_t MAX_STOCK_SIZE = 4096;
const int NONE = -1;

const size_t CODE_LEN = 64;
//const size_t MAX_STOCK_CACHE = 1024 * 4096 * 4;
//const size_t ATR_SIZE = 14;

//返回中间结果和取样数据所需要考虑的所有天数,比如delate(close,3)取样5天,就返回3+5-1天的数据,这些天以外的数据计算时不会涉及到的.
#define GET_ELEMEMT_SIZE(historyDays, sampleDays) (historyDays + sampleDays - 1)

enum class LeafDataType{
    OPEN = 0,
    HIGH = 1,
    LOW = 2,
    CLOSE = 3,
    VOLUME = 4,
    VWAP = 5,
    RETURNS = 6,
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
    } else {
        throw "IndClass Error!";
    }
}

class Stock{
    public:
        Stock(const char* code, const char* market, const char* industry, const char*concept,
              const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
              int size, int totals
        ){
            strcpy(this->code, code);
            if(market != nullptr)
                strcpy(this->market, market);
            else
                this->market[0] = 0;
            if(industry != nullptr){
                strcpy(this->industry,"i:");
                strcpy(this->industry+2, industry);
            }
            else
                this->industry[0] = 0;
            if(concept != nullptr){
                strcpy(this->concept, "c:");
                strcpy(this->concept+2, concept);
            }
            else
                this->concept[0] = 0;
            this->size = size;
            this->totals = max(totals,1);

            copyData(this->open, open, size);
            copyData(this->high, high, size);
            copyData(this->low, low, size);
            copyData(this->close, close, size);
            copyData(this->volume, volume, size);
            copyData(this->vwap, vwap, size);
            copyData(this->returns, returns, size);
            //calMAE_MFE(futureNum);
        }

        Stock(const char* code, vector<Stock*>& stocks){
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


                    if(lastOpenPrice != 0)
                        realOpenPrice = openPrice * open[i-1]/lastOpenPrice;
                    else
                        //当遇到计算不出的情况取个近似
                        realOpenPrice = close[i-1];
                }

                //计算为了得到这个开盘价所需要的缩放
                for(int j = 0; j < stockNum; ++j){
                    yestodayIsUse[j] = false;
                    if(stocks[j]->size >= maxLength-i){
                        int curIndex = stocks[j]->size - (maxLength - i);
                        if(stocks[j]->volume[curIndex] > 0){
                            openPrice += stocks[j]->open[curIndex] * weight[j];
                            highPrice += stocks[j]->high[curIndex] * weight[j];
                            lowPrice += stocks[j]->low[curIndex] * weight[j];
                            closePrice += stocks[j]->close[curIndex] * weight[j];
                            volumeValue += stocks[j]->volume[curIndex] * weight[j];
                            vwapPrice += stocks[j]->vwap[curIndex] * weight[j];
                            returnsValue += stocks[j]->returns[curIndex] * weight[j];
                            yestodayIsUse[j] = true;
                        }
                    }
                }


                if(realOpenPrice != NONE){
                    if(volumeValue == 0){
                        openPrice = close[i-1];
                        highPrice = close[i-1];
                        lowPrice = close[i-1];
                        closePrice = close[i-1];
                        volumeValue = 0;
                        vwapPrice = 0;
                        returnsValue = 0;
                    }else{
                        float k = realOpenPrice / openPrice;
                        openPrice *= k;
                        highPrice *= k;
                        lowPrice *= k;
                        closePrice *= k;
                        volumeValue *= k;
                        vwapPrice *= k;
                        returnsValue *= k;
                    }

                }

                open[i] = openPrice;
                high[i] = highPrice;
                low[i] = lowPrice;
                close[i] = closePrice;
                volume[i] = volumeValue;
                vwap[i] = vwapPrice;
                returns[i] = returnsValue;

            }

            //calMAE_MFE(futureNum);
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
//            if(alpha)
//                delete []alpha;
//            if(beta)
//                delete []beta;
//            if(MAE)
//                delete []MAE;
//            if(MFE)
//                delete []MFE;

//            if(target)
//                delete []target;
        }

        const char* getMarket(){ return strlen(market) == 0 ? nullptr : market;}
        const char* getIndustry(){ return strlen(industry) == 0 ? nullptr : industry;}
        const char* getConcept(){ return strlen(concept) == 0 ? nullptr : concept;}

        const float* getData(LeafDataType leafDataType){
            switch (leafDataType){
                case LeafDataType::OPEN:
                    return open;
                case LeafDataType::HIGH:
                    return high;
                case LeafDataType::LOW:
                    return low;
                case LeafDataType::CLOSE:
                    return close;
                case LeafDataType::VOLUME:
                    return volume;
                case LeafDataType::VWAP:
                    return vwap;
                case LeafDataType::RETURNS:
                    return returns;
            }
        }

        //void calTarget(/*const float* marketReturns, size_t sampleBetaSize, */size_t maxFutureSize){
            //calCAMP(marketReturns, sampleBetaSize);
            //calMAE_MFE(maxFutureSize);
        //}

        //bool isRealStock(){ return totals != 0;}

        //用相对收益作为target
//        void calRelativeAvgReturns(int futureNum, const float* marketReturns){
//            target = new float[futureNum * size];
//            memset(target, 0, futureNum * size * sizeof(float));
//            for(int fId = 1; fId <= futureNum; ++fId){
//                float* curTarget = target + (fId-1)*size;
//                for(int i = 0; i < size - fId; ++i){
//                    for(int j = 1; j <= fId; ++j)
//                        if(marketReturns != nullptr)
//                            curTarget[i] += logf((returns[i+j] - marketReturns[i+j])+1.f);
//                        else
//                            curTarget[i] += logf(returns[i+j]+1.f);
//                    curTarget[i] /= fId;
//                }
//            }
//        }


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
        float* open = {nullptr};
        float* high = {nullptr};
        float* low = {nullptr};
        float* close = {nullptr};
        float* volume = {nullptr};
        float* vwap = {nullptr};
        float* returns = {nullptr};
        //float* alpha = {nullptr};
        //float* beta = {nullptr};
        //float* MAE = {nullptr};
        //float* MFE = {nullptr};
        //float* target;
    protected:
        inline static void copyData(float*& dst, const float* src, int length){
            dst = new float[length];
            memcpy(dst, src, length * sizeof(float));
        }

        //因为是短线,所以忽略无风险shouy
        /*void calCAMP(const float* marketReturns, size_t sampleBetaSize){
            beta = new float[size];
            alpha = new float[size];
            memset(beta, 1, size * sizeof(float));
            memset(alpha, 0, size * sizeof(float));
            for(int i = sampleBetaSize-1; i < size; ++i){
                int offset = i - (sampleBetaSize-1);
                lstsq(marketReturns+offset, returns+offset, sampleBetaSize, alpha[i], beta[i]);
            }
        }*/

        /*void calMAE_MFE(size_t maxFutureSize){
            MAE = new float[size * maxFutureSize];
            MFE = new float[size * maxFutureSize];
            float* tr = stockCache;
            calTR(tr);
            float* sumTR = stockCache + size;
            sumTR[0] = tr[0];
            for(int i = 1; i < size; ++i)
                sumTR[i] = sumTR[i-1] + tr[i];
            for(int fIndex = 0; fIndex < maxFutureSize; ++fIndex){
                for(int i = 0; i < size-fIndex-1; ++i){
                    float downRatio = (close[i] - low[i+fIndex+1])/close[i];
                    float upRatio = (high[i + fIndex+1] - close[i])/close[i];
                    int targetIndex = i * maxFutureSize + fIndex;
                    MAE[targetIndex] = fIndex == 0 ? downRatio : fmaxf(MAE[targetIndex-1], downRatio);
                    MFE[targetIndex] = fIndex == 0 ? upRatio : fmaxf(MFE[targetIndex-1], upRatio);
                    float ATR = (i <= ATR_SIZE ? sumTR[i] / (i+1) : (sumTR[i] - sumTR[i - ATR_SIZE - 1]) / (ATR_SIZE+1));
                    MAE[targetIndex] /= ATR;
                    MFE[targetIndex] /= ATR;
                }
            }

        }

        inline void calTR(float* tr){
            for(int i = 0; i < size; ++i){
                tr[i] = high[i] - low[i];
                if(i > 0){
                    tr[i] = fmaxf(tr[i], fmaxf(high[i]-close[i-1],close[i-1] - low[i]));
                }
            }
        }*/

        //static float stockCache[MAX_STOCK_CACHE];
};


class AlphaDataBase{
    public:
        AlphaDataBase()
        {
        }

        ~AlphaDataBase(){
            clear();
        }

        void clear(){
            for(int i = 0; i < stocks.getSize(); ++i){
                delete stocks[i];
            }
            stocks.clear();
        }

        void addStock(const char* code, const char* market, const char* industry, const char* concept,
                      const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
                      int size, int totals
        ){
            Stock* stock = new Stock(code, market, industry, concept, open, high, low, close, volume, vwap, returns, size, totals);

            /*if(stock->getMarket()){
                Stock* market = getStock(stock->getMarket());
                stock->calTarget(MAX_FUTURE_DAYS);
            } else {
                stock->calTarget(MAX_FUTURE_DAYS);
            }*/
            //stock->calTarget(MAX_FUTURE_DAYS);

            addStock(stock);
        }
        void addStock(Stock* stock){
            //if(strcmp(stock->code,"次新股") == 0)
            //    cout<<stock->size<<" LLLLLLLLLLLLLLLLLLLL\n";
            stocks[stock->code] = stock;
        }

        void calClassifiedData(bool isConcept){
            map<const char*, vector<Stock*>, ptrCmp> subindustryMap;

            for(int i = 0; i < stocks.getSize(); ++i){
                Stock* curStock = stocks[i];
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

            /*if(isConcept){
                Stock * s =getStock(getStock("0603738")->getConcept());
                if(s == nullptr)
                    cout<<getStock("0603738")->getConcept()<<" "<<"NULL\n";
                else
                    cout<<s->code<<":"<<s->size<<"/"<<getStock("0603738")->size<<" "<<s->volume[s->size-44]<<"ddd"<<endl;
            }
             */

        }

//        static size_t getElementSize(size_t historyNum, size_t sampleNum, size_t watchFutureNum){
//            return historyNum + sampleNum - 1 + watchFutureNum;
//        }


        /*size_t getStocks(size_t dayBefore, size_t watchFutureNum, size_t historyNum, size_t sampleNum, Stock** pstocks){
            return getStocks_(dayBefore, watchFutureNum, historyNum, sampleNum,[pstocks](const char* code, Stock* curStock, size_t stockNum){
                pstocks[stockNum] = curStock;
            });
        }*/

        size_t getCodes(size_t dayBefore, size_t historyNum, size_t sampleNum, char* codes){
            char* curCodes = codes;
            char** pCurCodes = &curCodes;
            return getStocks_(dayBefore,historyNum,sampleNum,[pCurCodes](/*const char* code, */Stock* curStock, size_t stockNum){
                strcpy((*pCurCodes), curStock->code);
                (*pCurCodes) = (*pCurCodes) + strlen((*pCurCodes)) + 1;
            });
        }

        Stock* getStock(const char* curCodes){
            auto iter = stocks.find(curCodes);
            if(*iter == nullptr)
                return nullptr;
            return stocks[(*iter)->id];
        }

        Stock* getStock(const char *curCodes, const char *leafDataClass){
            Stock* curStock = getStock(curCodes);
            if(curStock == nullptr){
                return nullptr;
            }
            if(leafDataClass == nullptr){
                return curStock;
            }
            if(strcmp(leafDataClass,"IndClass.market") == 0){
                curStock = getStock(curStock->getMarket());
            } else if(strcmp(leafDataClass,"IndClass.industry") == 0){
                curStock = getStock(curStock->getIndustry());
            } else if(strcmp(leafDataClass,"IndClass.concept") == 0){
                curStock = getStock(curStock->getConcept());
                //cout<<curStock->code<<" "<<strlen(curCodes)<<" "<<dayNum<<" "<<needDay<<" "<<curStock->size<<endl;
            } else {
                curStock = getStock(leafDataClass);
            }
            return curStock;
        }

        //读出数据,如果不能读出补0
        float* getStock(size_t dayBefore, size_t historyNum, size_t sampleNum, size_t stockNum, LeafDataType leafDataType, const char* leafDataClass,
                      float* dst, const char* codes){

            const char* curCodes = codes;

            int dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
            int needDay = dayBefore + dayNum;

            for(int i = 0; i < stockNum; i++){
                Stock* curStock = getStock(curCodes, leafDataClass);

                if(curStock == nullptr){
                    for(int j = 0; j < dayNum; j++){
                        dst[j * stockNum + i] = 0;
                    }
                }
                else{
                    fillData_(dst, curStock->getData(leafDataType), dayNum, curStock->size - needDay, i, stockNum);
                }

                curCodes = nextCode_(curCodes);
            }
            return dst;
        }

        /*float getMAE(Stock* stock, int id, int watchFutureNum){
            return stock->MAE[id * MAX_FUTURE_DAYS + watchFutureNum - 1];
        }

        float getMFE(Stock* stock, int id, int watchFutureNum){
            return stock->MFE[id * MAX_FUTURE_DAYS + watchFutureNum - 1];
        }*/

    protected:
        //仅能在主线程调用,stockMap_不是线程安全的,即使只读
        template<class F>
        size_t getStocks_(size_t dayBefore, size_t historyNum, size_t sampleNum, F&& f){
            int stockNum = 0;

            int dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
            int needDay = dayBefore + dayNum;

            for(int i = 0; i < stocks.getSize(); ++i){
                Stock* curStock = stocks[i];
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
                            f(curStock, stockNum);
                            ++stockNum;
                        }
                    }
                }
            }
            return stockNum;
        }

        inline void fillData_(float* dst, const float* src, int dayNum, int srcStartIndex, int stockIndex, int stockNum){
            for(int j = 0; j < dayNum; j++){
                if(srcStartIndex + j < 0)
                    dst[j * stockNum + stockIndex] = 0;
                else{
                    dst[j * stockNum + stockIndex] = src[srcStartIndex + j];
                }
            }
        }

        const char* nextCode_(const char* code){
            return code + (strlen(code) + 1);
        }

        HashMap<Stock*> stocks;
        //HashName stockHashName;
        //Stock* stocks[MAX_STOCK_SIZE];

};

#endif //ALPHATREE_ALPHADB_H
