//
// Created by yanyu on 2017/7/29.
//

#ifndef ALPHATREE_ALPHADB_H
#define ALPHATREE_ALPHADB_H

#include <map>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <set>
#include <vector>
#include "atom/base.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "../base/hashmap.h"
#include "../db/stockdes.h"
#include "../db/stockcache.h"
using namespace std;

//返回中间结果和取样数据所需要考虑的所有天数,比如delate(close,3)取样5天,就返回3+5-1天的数据,这些天以外的数据计算时不会涉及到的.
#define GET_HISTORY_SIZE(historyDays, sampleDays) (historyDays + (sampleDays) - 1)


class AlphaDB{
    public:
        ~AlphaDB(){
            clean();
        }

        bool loadDataBase(const char* path){
            clean();

            des_ = new StockDes(path);
            cache_ = new StockCache(path, des_);
            return true;
        }

        void csv2binary(const char* path, const char* featureName){
            if(strcmp(featureName,"date") == 0){
                StockCache::csv2feature<long>(path, featureName, des_);
                //StockCache::testfeature<long>(path, featureName, des_);
            } else {
                StockCache::csv2feature<float>(path, featureName, des_);
            }
        }

        size_t getAllCodes(char* codes){
            char* curCode = codes;
            for(int i = 0; i < des_->stockMetas.getSize(); ++i){
                strcpy(curCode, des_->stockMetas[i].code);
                curCode = curCode + strlen(curCode) + 1;
            }

            return des_->stockMetas.getSize();
        }

        void getCodesFlag(bool* flag, const char* codes, int stockNum){
            struct comp{
                bool operator()(const char* a, const char* b){
                    return strcmp(a, b) < 0;
                }
            };
            set<const char*, comp> stocks;
            for(int i = 0; i < stockNum; ++i){
                stocks.insert(codes);
                codes += (strlen(codes) + 1);
            }
            for(int i = 0; i < des_->stockMetas.getSize(); ++i){
                if(stocks.find(des_->stockMetas[i].code) != stocks.end()){
                    //cout<<"match "<<des_->stockMetas[i].code<<endl;
                    flag[i] = true;
                } else {
                    flag[i] = false;
                }
            }
        }

        const char* testGetCode(int id){
            return des_->stockMetas[id].code;
        }

        size_t getStockCodes(char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(int i = 0; i < des_->stockMetas.getSize(); ++i){
                if(des_->stockMetas[i].stockType == StockMeta::StockType::STOCK){
                    strcpy(curCode, des_->stockMetas[i].code);
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }

            return codeNum;
        }

        size_t getMarketCodes(const char* marketName, char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(int i = 0; i < des_->stockMetas.getSize(); ++i){
                if(
                        (marketName != nullptr && des_->stockMetas[i].stockType == StockMeta::StockType::STOCK && strcmp(des_->stockMetas[i].marker, marketName) == 0) ||
                        (marketName == nullptr && des_->stockMetas[i].stockType == StockMeta::StockType::MARKET)){
                    strcpy(curCode, des_->stockMetas[i].code);
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        size_t getIndustryCodes(const char* industryName, char* codes){
            char* curCode = codes;
            size_t codeNum = 0;
            for(int i = 0; i < des_->stockMetas.getSize(); ++i){
                if(
                        (industryName != nullptr && des_->stockMetas[i].stockType == StockMeta::StockType::STOCK && strcmp(des_->stockMetas[i].industry, industryName) == 0) ||
                        (industryName == nullptr && des_->stockMetas[i].stockType == StockMeta::StockType::INDUSTRY)){
                    strcpy(curCode, des_->stockMetas[i].code);
                    curCode = curCode + strlen(curCode) + 1;
                    ++codeNum;
                }
            }
            return codeNum;
        }

        //得到所有股票数据，注意futureNum<=0
        float* getStock(size_t dayBefore, int historyNum, int futureNum, size_t sampleNum, size_t stockNum, const char* name, const char* leafDataClass,
                        float* dst, const char* codes){
            const char* curCode = codes;
            int dayNum = GET_HISTORY_SIZE(historyNum, sampleNum);
            int needDay = dayBefore + dayNum;
            if(needDay > des_->stockMetas[des_->mainStock].days || dayBefore < (size_t)-futureNum)
                return nullptr;

            for(size_t i = 0; i < stockNum; ++i){
                const char* code = getCode(curCode, leafDataClass);
                cache_->fill(dst, dayBefore + futureNum, dayNum - futureNum, code, name, i, stockNum);
                curCode = curCode + strlen(curCode) + 1;
            }
            return dst;
        }

        //得到信号发生时的股票数据
        float* getStock(size_t dayBefore, int historyNum, int futureNum, size_t sampleNum, size_t signHistoryDays, size_t startIndex, size_t signNum, const char* name, const char* signName, float* dst){
            int dayNum = GET_HISTORY_SIZE(historyNum, signHistoryDays);
            cache_->fill(dst, dayBefore, sampleNum, dayNum, futureNum, startIndex, signNum, signName, name);
            return dst;
        }

        size_t getSignNum(size_t dayBefore, size_t sampleSize, const char* signName){
            return cache_->getSignNum(dayBefore, sampleSize, signName);
        }


//        //将满足要求的特征写入内存或文件，可以分段写入
//        void fillFeature(size_t dayBefore, size_t historyNum, size_t sampleNum, size_t stockNum, const char* name, const char* leafDataClass,
//                         const float* sign, Iterator<float>& featureOut, const char* codes){
//            const char* curCode = codes;
//            size_t dayNum = GET_HISTORY_SIZE(historyNum, sampleNum);
//            //size_t needDay = dayBefore + dayNum;
//
//            for(size_t i = 0; i < stockNum; ++i){
//                const char* code = getCode(curCode, leafDataClass);
//                cache_->fill(featureOut, sign, dayBefore, dayNum, code, name, i, stockNum);
//                curCode = curCode + strlen(curCode) + 1;
//            }
//        }
//
//        void fillFeature(size_t historyNum, size_t sampleNum, size_t stockNum, const float* sign,
//                         Iterator<float>& featureOut, size_t sampleSize, float* feature){
//            size_t dayNum = GET_HISTORY_SIZE(historyNum, sampleNum);
//            for(size_t i = 0; i < stockNum; ++i){
//                for(size_t j = 0; j < dayNum; ++j){
//                    size_t index = j * stockNum + i;
//                    if(sign[index] > 0){
//                        *featureOut = feature[index];
//                        ++featureOut;
//                    }
//                }
//            }
//        }

        ofstream* createCacheFile(const char* featureName){
            return cache_->createCacheFile(featureName);
        }

        template<class T>
        void invFill2File(const float* cache, size_t dayBefore, size_t daySize, const char* featureName, ofstream* file,
                          bool isWritePreData = false, bool isWriteLastData = false){
            for(int i = 0; i < des_->stockMetas.getSize(); ++i)
                cache_->invFill2File<T>(cache, dayBefore, daySize, des_->stockMetas[i].code, featureName, i, des_->stockMetas.getSize(), file, isWritePreData, isWriteLastData);
        }

        void invFill2Sign(const float* cache, size_t daySize, const char* featureName, ofstream* file, size_t& preDayNum, size_t& preSignCnt, const bool* stockFlag = nullptr){
            cache_->invFill2Sign(cache, daySize, des_->stockMetas.getSize(), file, getDays(), preDayNum, preSignCnt, stockFlag);
        }

        void testInvFill2Sign(const float* cache, const float* testCache, size_t daySize, const char* featureName, ofstream* file, size_t& preDayNum, size_t& preSignCnt){
            cache_->testInvFill2Sign(cache, testCache, daySize, des_->stockMetas.getSize(), file, getDays(), preDayNum, preSignCnt);
        }

        void releaseCacheFile(ofstream * file){
            cache_->releaseCacheFile(file);
        }

        void cacheFeature(const char* featureName){
            cache_->cacheFeature(featureName);
        }

//        IBaseIterator<float>* createSignFeatureIter(const char* signName, const char* featureName, size_t dayBefore, size_t sampleDays, int offset){
//            return cache_->createSignFeatureIter(signName, featureName, dayBefore, sampleDays, offset);
//        }
//
//        IBaseIterator<float*>* createFeatureIter(const char* featureName, size_t dayBefore, size_t sampleDays, size_t stockNum, const char* codes, bool isCache){
//            return new FeatureIterator(featureName, dayBefore, sampleDays, stockNum, codes, isCache, cache_);
//        }

        size_t getStockNum(){ return des_->stockMetas.getSize();}
        size_t getDays(){
            //cout<<des_->stockMetas.getSize()<<endl;
            //cout<<des_->mainStock<<endl;
            return des_->stockMetas[des_->mainStock].days;
        }

    protected:
        inline const char* getCode(const char* code, const char* leafDataClass){
            if(leafDataClass != nullptr){
                if(strcmp(leafDataClass,"IndClass.market") == 0){
                    code = des_->stockMetas[code].marker;
                } else if(strcmp(leafDataClass, "IndClass.industry") == 0){
                    code = des_->stockMetas[code].industry;
                }
            }
            return code;
        }

        void clean(){
            if(cache_ != nullptr)
                delete cache_;
            cache_ = nullptr;
            if(des_ != nullptr)
                delete des_;
            des_ = nullptr;
        }

        StockDes* des_ = {nullptr};
        StockCache* cache_ = {nullptr};

};


#endif //ALPHATREE_ALPHADB_H
