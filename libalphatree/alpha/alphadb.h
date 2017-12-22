//
// Created by yanyu on 2017/7/29.
//

#ifndef ALPHATREE_ALPHADB_H
#define ALPHATREE_ALPHADB_H

#include <map>
#include <stdlib.h>
#include <string>
#include <math.h>
#include <vector>
#include "base.h"
#include <iostream>
#include <fstream>
#include "../base/hashmap.h"
#include "db/stock.pb.h"
using namespace std;

//返回中间结果和取样数据所需要考虑的所有天数,比如delate(close,3)取样5天,就返回3+5-1天的数据,这些天以外的数据计算时不会涉及到的.
#define GET_ELEMEMT_SIZE(historyDays, sampleDays) (historyDays + sampleDays - 1)

class AlphaDB{
    public:
        ~AlphaDB(){
            clean();
        }

        bool loadDataBase(const char* path){
            clean();
            db = new stp::StockDB();
            ifstream in(path, ios::in);
            db->ParseFromIstream(&in);
            in.close();
            return true;
        }

        size_t getAllCodes(char* codes){
            char* curCode = codes;
            for(size_t i = 0; i < getStockNum(); ++i){
                strcpy(curCode, db->metas(i).code().data());
                curCode = curCode + strlen(curCode) + 1;
            }
            return getStockNum();
        }

        size_t getStockCodes(char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(size_t i = 0; i < getStockNum(); ++i){
                if(db->metas(i).stocktype() == stp::StockMeta::STOCK){
                    strcpy(curCode, db->metas(i).code().data());
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        size_t getMarketCodes(const char* marketName, char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(size_t i = 0; i < getStockNum(); ++i){
                if(
                        (marketName != nullptr && db->metas(i).stocktype() == stp::StockMeta::STOCK && db->metas(db->metas(i).marketindex()).code().compare(marketName) == 0) ||
                        (marketName == nullptr && db->metas(i).stocktype() == stp::StockMeta::MARKET)){
                    strcpy(curCode, db->metas(i).code().data());
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        size_t getIndustryCodes(const char* industryName, char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(size_t i = 0; i < getStockNum(); ++i){
                if(
                        (industryName != nullptr && db->metas(i).stocktype() == stp::StockMeta::STOCK && db->metas(db->metas(i).industryindex()).code().compare(industryName) == 0) ||
                        (industryName == nullptr && db->metas(i).stocktype() == stp::StockMeta::INDUSTRY)){
                    strcpy(curCode, db->metas(i).code().data());
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        size_t getConceptCodes(const char* conceptName, char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(size_t i = 0; i < getStockNum(); ++i){
                if(
                        (conceptName != nullptr && db->metas(i).stocktype() == stp::StockMeta::STOCK && db->metas(db->metas(i).conceptindex()).code().compare(conceptName) == 0) ||
                        (conceptName == nullptr && db->metas(i).stocktype() == stp::StockMeta::CONCEPT)){
                    strcpy(curCode, db->metas(i).code().data());
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        float* getStock(size_t dayBefore, size_t historyNum, size_t sampleNum, size_t stockNum, const char* name, const char* leafDataClass,
                        float* dst, const char* codes){
            const char* curCode = codes;

            size_t dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
            size_t needDay = dayBefore + dayNum;
            if(needDay > getDays())
                return nullptr;

            string elementName;
            if(strcmp(name,"cap") == 0 || strcmp(name,"pe") == 0){
                elementName = "close";
                leafDataClass = nullptr;
            }else{
                elementName = name;
            }


            auto element = db->elements().find(elementName)->second;
            for(size_t i = 0; i < stockNum; ++i){
                int stockIndex = getStockIndex(curCode, leafDataClass);
                for(size_t j = 0; j < dayNum; ++j){
                    dst[j * stockNum + i] = element.data(db->days() * stockIndex + db->days() - needDay + j);
                }
                curCode = curCode + strlen(curCode) + 1;
            }

            curCode = codes;
            if(strcmp(name,"cap") == 0){
                for(size_t i = 0; i < stockNum; ++i){
                    int stockIndex = getStockIndex(curCode, leafDataClass);
                    for(size_t j = 0; j < dayNum; ++j){
                        dst[j * stockNum + i] *= db->metas(stockIndex).totals();
                    }
                    curCode = curCode + strlen(curCode) + 1;
                }
            } else if(strcmp(name,"pe") == 0){
                for(size_t i = 0; i < stockNum; ++i){
                    int stockIndex = getStockIndex(curCode, leafDataClass);
                    for(size_t j = 0; j < dayNum; ++j){
                        dst[j * stockNum + i] /= db->metas(stockIndex).earningratios();
                    }
                    curCode = curCode + strlen(curCode) + 1;
                }
            }



            return dst;
        }

        bool* getFlag(size_t dayBefore, size_t historyNum, size_t sampleNum, size_t stockNum, const char* name, bool* dst, const char* codes){
            const char* curCode = codes;

            size_t dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
            size_t needDay = dayBefore + dayNum;
            if(needDay > getDays())
                return nullptr;

            auto element = db->elements().find(name)->second;
            if(element.flag_size() != element.data_size()) {
                element = db->elements().find("volume")->second;
                for(size_t i = 0; i < stockNum; ++i){
                    int stockIndex = getStockIndex(curCode);
                    for(size_t j = 0; j < dayNum; ++j){
                        dst[j * stockNum + i] = (element.data(db->days() * stockIndex + db->days() - needDay + j) > 0);
                    }
                    curCode = curCode + strlen(curCode) + 1;
                }
            } else {
                for(size_t i = 0; i < stockNum; ++i){
                    int stockIndex = getStockIndex(curCode);
                    for(size_t j = 0; j < dayNum; ++j){
                        dst[j * stockNum + i] = element.flag(db->days() * stockIndex + db->days() - needDay + j);
                    }
                    curCode = curCode + strlen(curCode) + 1;
                }
            }
            return dst;
        }

        void setElement(string name, const char* line, int needDay, float* alpha, bool* flag, bool isFeature = false){
            auto map = db->mutable_elements();
            (*map)[name].set_needday(needDay);
            (*map)[name].set_line(line);
            for(size_t j = 0; j < db->stocksize(); ++j){
                for(size_t i = 0; i < db->days(); ++i){
                    (*map)[name].add_data(alpha[i * db->stocksize() + j]);
                    (*map)[name].add_flag(flag[i * db->stocksize() + j]);
                }
            }
            if(isFeature){
                featureIndex.add(name.data(), featureSize);
                ++featureSize;
            }
        }

        void getFeature(size_t dayBefore, size_t sampleDays, size_t stockSize, const char* codes, const float* buy, const float* sell,
                          float* featureValue, size_t sampleSize, const char* features, size_t featureSize){
            const char* curFeature = features;
            size_t needDay = sampleDays + dayBefore;
            for(size_t fId = 0; fId < featureSize; ++fId){
                float* feature = featureValue + fId * sampleSize;
                auto element = db->elements().find(curFeature)->second;
                size_t sampleIndex = 0;
                for(size_t i = 0; i < stockSize; ++i){
                    const char* curCode = codes;
                    int lastBuyDay = -1;
                    int stockIndex = getStockIndex(curCode);
                    for(size_t j = 0; j < sampleDays; ++j){
                        size_t index = j * stockSize + i;
                        if(lastBuyDay >= 0 && sell[index] > 0){
                            feature[sampleIndex++] = element.data(db->days() * stockIndex + db->days() - needDay + j);
                            lastBuyDay = -1;
                        }
                        if(lastBuyDay == -1 && buy[index] > 0)
                            lastBuyDay = index;
                    }
                    curCode = curCode + strlen(curCode) + 1;
                }
                curFeature = curFeature + strlen(curFeature) + 1;
            }
        }


//        bool* getFlag(int dayIndex, size_t dayBefore, size_t historyNum, size_t sampleNum, size_t stockNum,
//                      bool* dst, const char* codes){
//            const char* curCode = codes;
//
//            size_t dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
//            size_t needDay = dayBefore + dayNum;
//            if(needDay > getDays())
//                return nullptr;
//
//            auto element = db->elements().find("volume")->second;
//            for(size_t i = 0; i < stockNum; ++i){
//                int stockIndex = getStockIndex(curCode);
//                dst[i] = element.data(db->days() * stockIndex + db->days() - needDay + dayIndex);
//                curCode = curCode + strlen(curCode) + 1;
//            }
//            return dst;
//        }

        size_t getStockNum(){ return db->metas_size();}
        size_t getDays(){ return db->days();}
        size_t getFeatureSize(){ return featureSize;}
    //stp::StockDB* testGet(){ return db;}
//        int gi(const char *curCode){
//        return getStockIndex(curCode);
//        }
    protected:
        void clean(){
            if(db != nullptr)
                delete db;
            db = nullptr;
        }

        int getStockIndex(const char *curCode, const char *leafDataClass = nullptr){
            string code = curCode;
            int stockIndex = db->stockindex().find(code)->second;
            if(db->metas(stockIndex).stocktype() != stp::StockMeta::STOCK || leafDataClass == nullptr){
                return stockIndex;
            }
            if(strcmp(leafDataClass,"IndClass.market") == 0){
                stockIndex = db->metas(stockIndex).marketindex();
            } else if(strcmp(leafDataClass,"IndClass.industry") == 0){
                stockIndex = db->metas(stockIndex).industryindex();
            } else if(strcmp(leafDataClass,"IndClass.concept") == 0){
                stockIndex = db->metas(stockIndex).conceptindex();
            }
            return stockIndex;
        }

    //todo delete
        size_t getReturnsAndRisk(size_t dayBefore, size_t sampleDays, size_t stockSize, const char* codes, const float* buy, const float* sell, float* returns, float* risk, int* chooseIndex){
            const char* curCode = codes;
            int signCount = 0;

            auto high = db->elements().find("high")->second;
            auto low = db->elements().find("low")->second;
            auto close = db->elements().find("close")->second;
            auto atr = db->elements().find("atr")->second;
            size_t needDay = sampleDays + dayBefore;
            for(size_t j = 0; j < stockSize; ++j){
                int stockIndex = getStockIndex(curCode);
                int buyIndex = -1;
                float maxPrice = 0;
                float maxDrawDown = 0;
                int curIndex = 0;
                int dataIndex = 0;
                for(size_t i = 1; i < sampleDays; ++i){
                    curIndex = i * stockSize + j;
                    dataIndex = db->days() * stockIndex + db->days() - needDay + i;
                    if(buyIndex == -1 && buy[curIndex] > 0 && sell[curIndex] == 0){
                        buyIndex = i;
                        maxPrice = close.data(dataIndex);
                        continue;
                    }
                    if(buyIndex != -1){
                        //已经买入,等待卖出
                        maxPrice = max(maxPrice, high.data(dataIndex));
                        maxDrawDown = max(maxDrawDown,maxPrice - low.data(dataIndex));
                        if(sell[curIndex] > 0){
                            int lastDataIndex = db->days() * stockIndex + db->days() - needDay + buyIndex;
                            returns[signCount] = (close.data(dataIndex) - close.data(lastDataIndex))/close.data(lastDataIndex) / atr.data(lastDataIndex);
                            risk[signCount] = maxDrawDown / maxPrice / atr.data(lastDataIndex);
                            chooseIndex[signCount] = lastDataIndex;

                            maxPrice = 0;
                            maxDrawDown = 0;
                            ++signCount;
                        }
                    }
                }
                curCode = curCode + strlen(curCode) + 1;
            }
            return signCount;
        }

        /*void fillFeature(size_t dayBefore, size_t sampleNum, size_t stockSize, const char* codes, const float* buy, const float* sell, size_t signCount){
            const char* curCode = codes;
            auto close = db->elements().find("close")->second;
            auto atr = db->elements().find("atr")->second;
            size_t needDay = sampleNum + dayBefore;
            for(size_t j = 0; j < stockSize; ++j){
                int stockIndex = getStockIndex(curCode);
                int buyIndex = -1;
                int curIndex = 0;
                int dataIndex = 0;
                for(size_t i = 1; i < sampleNum; ++i){
                    curIndex = i * stockSize + j;
                    dataIndex = db->days() * stockIndex + db->days() - needDay + i;
                    if(!isnan(buy[curIndex]) &&
                       !isnan(sell[curIndex]) &&
                       !isnan(close.data(dataIndex)) &&
                       !isnan(close.data(dataIndex-1)) &&
                       !isnan(atr.data(dataIndex))){
                        if(buyIndex == -1 && buy[curIndex] > 0 && sell[curIndex] == 0){
                            buyIndex = i;
                            continue;
                        }
                        if(buyIndex != -1){
                            //已经买入,等待卖出
                            if(sell[curIndex] > 0){
                                int lastDataIndex = db->days() * stockIndex + db->days() - needDay + buyIndex;
                                //开始填写特征

                                ++signCount;
                            }
                        }
                    }
                }
                curCode = curCode + strlen(curCode) + 1;
            }
        }*/

        HashMap<size_t> featureIndex;
        stp::StockDB* db = {nullptr};
        size_t featureSize = {0};
};

/*const size_t CODE_LEN = 64;
const size_t CODE_NUM = 4096;

class DataElement{
    public:
        DataElement(const float* src, size_t size, size_t needDays = 0){
            this->needDays = needDays;
            float* dst = new float[size];
            memcpy(dst, src, size * sizeof(float));
            data = dst;
        }
        ~DataElement(){delete[] data;}
        size_t needDays = {0};
        const float* data;
};

class StockMeta{
    public:
        char code[CODE_LEN];
        int marketIndex = {-1};
        int industryIndex = {-1};
        int conceptIndex = {-1};
        bool isStock(){ return marketIndex >= 0;}
        bool isMarket(){ return marketIndex == -2;}
        bool isIndustry(){ return industryIndex == -2;}
        bool isConcept(){ return conceptIndex == -2;}
};

class AlphaDB{
    public:
        void load(const char* path){
            ifstream in(path, ios::in);
            stp::StockDB sdb;
            sdb.ParseFromIstream(&in);
            in.close();
        }

        void loadStockMeta(const char* codes, int* marketIndex, int* industryIndex, int* conceptIndex, size_t size, size_t days){
            days_ = days;
            stockMeta_.resize(size);
            cleanDataElement();

            const char* pcode = codes;
            for(size_t i = 0; i < size; ++i){
                strcpy(stockMeta_[i].code,pcode);
                stockIndex_[pcode] = i;
                pcode = pcode + strlen(pcode) + 1;
                stockMeta_[i].marketIndex = marketIndex[i];
                stockMeta_[i].industryIndex = industryIndex[i];
                stockMeta_[i].conceptIndex = conceptIndex[i];
            }
        }

        void loadDataElement(const char* elementName, const float* data, size_t needDays){
            dataElement_[elementName] = new DataElement(data, stockMeta_.getSize() * days_, needDays);
        }

        size_t getStockCodes(char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(size_t i = 0; i < getStockNum(); ++i){
                if(stockMeta_[i].marketIndex >= 0){
                    strcpy(curCode, stockMeta_[i].code);
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        size_t getMarketCodes(const char* marketName, char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(size_t i = 0; i < getStockNum(); ++i){
                if(
                        (marketName != nullptr && stockMeta_[i].isStock() && strcmp(stockMeta_[stockMeta_[i].marketIndex].code, marketName) == 0) ||
                        (marketName == nullptr && stockMeta_[i].isMarket())){
                    strcpy(curCode, stockMeta_[i].code);
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        size_t getIndustryCodes(const char* industryName, char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(size_t i = 0; i < getStockNum(); ++i){
                if(
                        (industryName != nullptr && stockMeta_[i].isStock() && strcmp(stockMeta_[stockMeta_[i].industryIndex].code, industryName) == 0) ||
                        (industryName == nullptr && stockMeta_[i].isIndustry())){
                    strcpy(curCode, stockMeta_[i].code);
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        size_t getConceptCodes(const char* conceptName, char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(size_t i = 0; i < getStockNum(); ++i){
                if(
                        (conceptName != nullptr && stockMeta_[i].isStock() && strcmp(stockMeta_[stockMeta_[i].conceptIndex].code, conceptName) == 0) ||
                        (conceptName == nullptr && stockMeta_[i].isConcept())){
                    strcpy(curCode, stockMeta_[i].code);
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        float* getStock(size_t dayBefore, size_t historyNum, size_t sampleNum, size_t stockNum, const char* name, const char* leafDataClass,
                        float* dst, const char* codes){

            const char* curCode = codes;

            size_t dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
            size_t needDay = dayBefore + dayNum;
            if(needDay > getDays())
                return nullptr;

            const float* src = dataElement_[name]->data;
            for(size_t i = 0; i < stockNum; ++i){
                int stockIndex = getStockIndex(curCode, leafDataClass);
                const float* data = src + days_ * stockIndex;

                for(size_t j = 0; j < dayNum; ++j){
                    dst[j * stockNum + i] = data[days_ - needDay + j];
                }

                curCode = curCode + strlen(curCode) + 1;
            }
            return dst;
        }

        bool* getFlag(int dayIndex, size_t dayBefore, size_t historyNum, size_t sampleNum, size_t stockNum,
                      bool* dst, const char* codes){
            const char* curCode = codes;

            size_t dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
            size_t needDay = dayBefore + dayNum;
            if(needDay > getDays())
                return nullptr;

            const float* src = dataElement_["volume"]->data;
            for(size_t i = 0; i < stockNum; ++i){
                int stockIndex = getStockIndex(curCode, "");
                const float* data = src + days_ * stockIndex;

                dst[i] = (data[days_ - needDay + dayIndex] > 0);
                curCode = curCode + strlen(curCode) + 1;
            }
            return dst;
        }

        size_t getStockNum(){ return stockMeta_.getSize();}
        size_t getDays(){ return days_;}

    protected:
        DArray<StockMeta, CODE_NUM> stockMeta_;
        HashMap<int> stockIndex_;
        HashMap<DataElement*> dataElement_;
        size_t days_;

        void cleanDataElement(){
            for(auto i = 0; i < dataElement_.getSize(); ++i)
                delete dataElement_[i];
            dataElement_.clear();
        }

        const int getStockIndex(const char *curCode, const char *leafDataClass){
            int stockIndex = stockIndex_[curCode];
            if(!stockMeta_[stockIndex].isStock() || leafDataClass == nullptr){
                return stockIndex;
            }
            if(strcmp(leafDataClass,"IndClass.market") == 0){
                stockIndex = stockMeta_[stockIndex].marketIndex;
            } else if(strcmp(leafDataClass,"IndClass.industry") == 0){
                stockIndex = stockMeta_[stockIndex].industryIndex;
            } else if(strcmp(leafDataClass,"IndClass.concept") == 0){
                stockIndex = stockMeta_[stockIndex].conceptIndex;
            }
            return stockIndex;
        }
};*/

/*
//const size_t MAX_STOCK_SIZE = 4096;
const int NONE = -1;


const int MAX_DATA_TYPE = 256;

//const size_t MAX_STOCK_CACHE = 1024 * 4096 * 4;
//const size_t ATR_SIZE = 14;



enum class LeafDataType{
    OPEN = 0,
    HIGH = 1,
    LOW = 2,
    CLOSE = 3,
    VOLUME = 4,
    VWAP = 5,
    RETURNS = 6,
    TR = 7,
};

//LeafDataType str2LeafDataType(const char* str){
//    if(strcmp(str,"open") == 0){
//        return LeafDataType::OPEN;
//    } else if(strcmp(str,"high") == 0){
//        return LeafDataType::HIGH;
//    } else if(strcmp(str,"low") == 0){
//        return LeafDataType::LOW;
//    } else if(strcmp(str,"close") == 0){
//        return LeafDataType::CLOSE;
//    } else if(strcmp(str,"volume") == 0){
//        return LeafDataType::VOLUME;
//    } else if(strcmp(str,"vwap") == 0){
//        return LeafDataType::VWAP;
//    } else if(strcmp(str,"returns") == 0){
//        return LeafDataType::RETURNS;
//    } else if(strcmp(str,"tr") == 0){
//        return LeafDataType::TR;
//    }else {
//        throw "IndClass Error!";
//    }
//}

class Stock{
    public:
        Stock(const char* code, const char* market, const char* industry, const char*concept,
              const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
              size_t size, int totals
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

//            copyData(this->open, open, size);
//            copyData(this->high, high, size);
//            copyData(this->low, low, size);
//            copyData(this->close, close, size);
//            copyData(this->volume, volume, size);
//            copyData(this->vwap, vwap, size);
//            copyData(this->returns, returns, size);
            setData("open", open);
            setData("high", high);
            setData("low", low);
            setData("close", close);
            setData("volume", volume);
            setData("vwap", vwap);
            setData("returns", returns);
            calTR();

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
            for(size_t i = 0; i < stockNum; ++i){
                maxLength = max(stocks[i]->size, maxLength);
                allWeight += stocks[i]->totals * stocks[i]->data["close"][stocks[i]->size-1];
            }
            this->size = maxLength;

            //以最后一天的市值来作为各个股票所在成分
            float* weight = new float[stockNum];
            bool* yestodayIsUse = new bool[stockNum];
            for(size_t i = 0; i < stockNum; ++i){
                weight[i] = stocks[i]->totals * stocks[i]->data["close"][stocks[i]->size-1] / allWeight;
            }


            float* open = new float[maxLength];
            float* high = new float[maxLength];
            float* low = new float[maxLength];
            float* close = new float[maxLength];
            float* volume = new float[maxLength];
            float* vwap = new float[maxLength];
            float* returns = new float[maxLength];

            for(auto i = 0; i < maxLength; ++i){

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
                    for(size j = 0; j < stockNum; ++j){
                        int curIndex = stocks[j]->size - (maxLength - i);
                        if(yestodayIsUse[j] && stocks[j]->data["volume"][curIndex] > 0){
                            lastOpenPrice += stocks[j]->data["open"][curIndex-1] * weight[j];
                            openPrice += stocks[j]->data["open"][curIndex] * weight[j];
                        }
                    }


                    if(lastOpenPrice != 0)
                        realOpenPrice = openPrice * open[i-1]/lastOpenPrice;
                    else
                        //当遇到计算不出的情况取个近似
                        realOpenPrice = close[i-1];
                }

                //计算为了得到这个开盘价所需要的缩放
                for(size j = 0; j < stockNum; ++j){
                    yestodayIsUse[j] = false;
                    if(stocks[j]->size >= maxLength-i){
                        int curIndex = stocks[j]->size - (maxLength - i);
                        if(stocks[j]->data["volume"][curIndex] > 0){
                            openPrice += stocks[j]->data["open"][curIndex] * weight[j];
                            highPrice += stocks[j]->data["high"][curIndex] * weight[j];
                            lowPrice += stocks[j]->data["low"][curIndex] * weight[j];
                            closePrice += stocks[j]->data["close"][curIndex] * weight[j];
                            volumeValue += stocks[j]->data["volume"][curIndex] * weight[j];
                            vwapPrice += stocks[j]->data["vwap"][curIndex] * weight[j];
                            returnsValue += stocks[j]->data["returns"][curIndex] * weight[j];
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
            data["open"] = open;
            data["high"] = high;
            data["low"] = low;
            data["close"] = close;
            data["volume"] = volume;
            data["vwap"] = vwap;
            data["returns"] = returns;
            delete []weight;
            calTR();
        }

        ~Stock(){
            for(auto i = 0; i < data.getSize(); ++i)
                delete data[i];
            data.clear();
//            delete []open;
//            delete []high;
//            delete []low;
//            delete []close;
//            delete []volume;
//            delete []vwap;
//
//            delete []returns;
//            delete []tr;
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

        inline void setData(const char* name, const float* src){
            float* v = new float[size];
            memcpy(v, src, size * sizeof(float));
            data[name] = v;
        }

        const float* getData(const char* name){
            return data[name];
//            switch (leafDataType){
//                case LeafDataType::OPEN:
//                    return open;
//                case LeafDataType::HIGH:
//                    return high;
//                case LeafDataType::LOW:
//                    return low;
//                case LeafDataType::CLOSE:
//                    return close;
//                case LeafDataType::VOLUME:
//                    return volume;
//                case LeafDataType::VWAP:
//                    return vwap;
//                case LeafDataType::RETURNS:
//                    return returns;
//                case LeafDataType::TR:
//                    return tr;
//            }
        }



        //bool isRealStock(){ return totals != 0;}

        //用相对收益作为target
//        void calRelativeAvgReturns(int futureNum, const float* marketReturns){
//            target = new float[futureNum * size];
//            memset(target, 0, futureNum * size * sizeof(float));
//            for(int fId = 1; fId <= futureNum; ++fId){
//                float* curTarget = target + (fId-1)*size;
//                for(auto i = 0; i < size - fId; ++i){
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
        size_t size;
        //交易数据,注意不包含日期是因为所有日期都是对齐的!
//        float* open = {nullptr};
//        float* high = {nullptr};
//        float* low = {nullptr};
//        float* close = {nullptr};
//        float* volume = {nullptr};
//        float* vwap = {nullptr};
//        float* returns = {nullptr};
//        float* tr = {nullptr};
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

        inline void calTR(){
            float* tr = new float[size];
            const float* high = data["high"];
            const float* low = data["low"];
            const float* close = data["close"];
            for(size_t i = 0; i < size; ++i){
                tr[i] = high[i] - low[i];
                if(i > 0){
                    tr[i] = fmaxf(tr[i], fmaxf(high[i]-close[i-1],close[i-1] - low[i]));
                }
            }
            data["tr"] = tr;
        }
        HashMap<const float*, 4 * MAX_DATA_TYPE, MAX_DATA_TYPE> data;
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

        void calMAE_MFE(size_t maxFutureSize){
            MAE = new float[size * maxFutureSize];
            MFE = new float[size * maxFutureSize];
            float* tr = stockCache;
            calTR(tr);
            float* sumTR = stockCache + size;
            sumTR[0] = tr[0];
            for(int i = 1; i < size; ++i)
                sumTR[i] = sumTR[i-1] + tr[i];
            for(int fIndex = 0; fIndex < maxFutureSize; ++fIndex){
                for(auto i = 0; i < size-fIndex-1; ++i){
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
            for(size_t i = 0; i < size; ++i){
                tr[i] = high[i] - low[i];
                if(i > 0){
                    tr[i] = fmaxf(tr[i], fmaxf(high[i]-close[i-1],close[i-1] - low[i]));
                }
            }
        }

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
            for(auto i = 0; i < stocks.getSize(); ++i){
                delete stocks[i];
            }
            stocks.clear();
        }

        void addStock(const char* code, const char* market, const char* industry, const char* concept,
                      const float* open, const float* high, const float* low, const float* close, const float* volume, const float* vwap, const float* returns,
                      int size, int totals
        ){
            Stock* stock = new Stock(code, market, industry, concept, open, high, low, close, volume, vwap, returns, size, totals);

//            if(stock->getMarket()){
//                Stock* market = getStock(stock->getMarket());
//                stock->calTarget(MAX_FUTURE_DAYS);
//            } else {
//                stock->calTarget(MAX_FUTURE_DAYS);
//            }
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

            for(auto i = 0; i < stocks.getSize(); ++i){
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
//
//            if(isConcept){
//                Stock * s =getStock(getStock("0603738")->getConcept());
//                if(s == nullptr)
//                    cout<<getStock("0603738")->getConcept()<<" "<<"NULL\n";
//                else
//                    cout<<s->code<<":"<<s->size<<"/"<<getStock("0603738")->size<<" "<<s->volume[s->size-44]<<"ddd"<<endl;
//            }
//

        }

//        static size_t getElementSize(size_t historyNum, size_t sampleNum, size_t watchFutureNum){
//            return historyNum + sampleNum - 1 + watchFutureNum;
//        }


//        size_t getStocks(size_t dayBefore, size_t watchFutureNum, size_t historyNum, size_t sampleNum, Stock** pstocks){
//            return getStocks_(dayBefore, watchFutureNum, historyNum, sampleNum,[pstocks](const char* code, Stock* curStock, size_t stockNum){
//                pstocks[stockNum] = curStock;
//            });
//        }

        size_t getCodes(size_t dayBefore, size_t historyNum, size_t sampleNum, char* codes){
            char* curCodes = codes;
            char** pCurCodes = &curCodes;
            return getStocks_(dayBefore,historyNum,sampleNum,[pCurCodes](Stock* curStock, size_t stockNum){
                strcpy((*pCurCodes), curStock->code);
                (*pCurCodes) = (*pCurCodes) + strlen((*pCurCodes)) + 1;
            });
        }

        size_t getCodes(char* codes){
            char* curCodes = codes;
            char** pCurCodes = &curCodes;
            return getStocks_([pCurCodes](Stock* curStock, size_t stockNum){
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
        float* getStock(size_t dayBefore, size_t historyNum, size_t sampleNum, size_t stockNum, const char* name, const char* leafDataClass,
                      float* dst, const char* codes){

            const char* curCodes = codes;

            int dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
            int needDay = dayBefore + dayNum;

            for(size_t i = 0; i < stockNum; ++i){
                Stock* curStock = getStock(curCodes, leafDataClass);

                if(curStock == nullptr){
                    for(size_t j = 0; j < dayNum; ++j){
                        dst[j * stockNum + i] = 0;
                    }
                }
                else{
                    if(strcmp(name, "volume"))
                        fillData_(dst, curStock->getData(name), dayNum, curStock->size - needDay, i, stockNum, 0);
                    else
                        fillData_(dst, curStock->getData(name), dayNum, curStock->size - needDay, i, stockNum, curStock->getData("open")[0]);
                }

                curCodes = nextCode_(curCodes);
            }
            return dst;
        }

//        float getMAE(Stock* stock, int id, int watchFutureNum){
//            return stock->MAE[id * MAX_FUTURE_DAYS + watchFutureNum - 1];
//        }
//
//        float getMFE(Stock* stock, int id, int watchFutureNum){
//            return stock->MFE[id * MAX_FUTURE_DAYS + watchFutureNum - 1];
//        }

    protected:
        //仅能在主线程调用,stockMap_不是线程安全的,即使只读
        template<class F>
        size_t getStocks_(size_t dayBefore, size_t historyNum, size_t sampleNum, F&& f){
            int stockNum = 0;

            int dayNum = GET_ELEMEMT_SIZE(historyNum, sampleNum);
            int needDay = dayBefore + dayNum;

            for(auto i = 0; i < stocks.getSize(); ++i){
                Stock* curStock = stocks[i];
                const float* volume = curStock->getData("volume");
                if(curStock->getMarket()){
                    if(curStock->size >= needDay){
                        const float* startVolume = volume + (curStock->size - needDay);
                        bool hasEmptyData = false;
                        for(auto i = 0; i < dayNum; i++){
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

        template<class F>
        size_t getStocks_(F&& f){
            int stockNum = 0;
            for(auto i = 0; i < stocks.getSize(); ++i){
                Stock* curStock = stocks[i];
                if(curStock->getMarket()){
                    f(curStock, stockNum);
                    ++stockNum;
                }
            }
            return stockNum;
        }

        inline void fillData_(float* dst, const float* src, int dayNum, int srcStartIndex, int stockIndex, int stockNum, float defaultValue = 0){
            for(size_t j = 0; j < dayNum; ++j){
                if(srcStartIndex + j < 0)
                    dst[j * stockNum + stockIndex] = defaultValue;
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


        HashMap<>
};*/

#endif //ALPHATREE_ALPHADB_H
