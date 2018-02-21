//
// Created by godpgf on 18-2-9.
//

#ifndef ALPHATREE_STOCKDES_H
#define ALPHATREE_STOCKDES_H
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include "../base/hashmap.h"
#include "../base/darray.h"

//定义股票的数据结构------------------------------
const size_t CODE_LEN = 64;
class StockMeta{
public:
    enum class StockType{
        MARKET = 0,
        INDUSTRY = 1,
        STOCK = 2,
    };
    char code[CODE_LEN];
    char marker[CODE_LEN];
    char industry[CODE_LEN];
    StockType stockType;
    int offset;
    int days;
};

#define STOCK_BLOCK 1024
class StockDes{
public:
    StockDes(const char* path){
        auto filepath = [path] (string file) {
            string filePath = path;
            filePath += "/";
            filePath += file;
            filePath += ".csv";
            return filePath;
        };

        ifstream inFile = ifstream(filepath("codes"), ios::in);
        string lineStr;
        getline(inFile, lineStr);

        while (getline(inFile, lineStr)){
            stringstream ss(lineStr);
            string code, market, industry, price, cap, pe, days;
            getline(ss, code, ',');
            getline(ss, market, ',');
            getline(ss, industry, ',');
            getline(ss, price, ',');
            getline(ss, cap, ',');
            getline(ss, pe, ',');
            getline(ss, days, ',');

            StockMeta sm;
            strcpy(sm.code, code.c_str());
            sm.days = atoi(days.c_str());
            //offset += sm.days;
            //cout<<market<<market.length()<<industry<<industry.length()<<endl;
            if(market.length() != 0 && industry.length() == 0){
                sm.stockType = StockMeta::StockType::MARKET;
            } else {
                if(market.length() == 0 && industry.length() != 0){
                    sm.stockType = StockMeta::StockType::INDUSTRY;
                } else {
                    sm.stockType = StockMeta::StockType::STOCK;
                    strcpy(sm.marker, market.c_str());
                    strcpy(sm.industry, industry.c_str());
                }
            }

            stockMetas[sm.code] = sm;
        }

        int maxDays = 0;
        offset = 0;
        for(int i = 0; i < stockMetas.getSize();++i){
            //cout<<offset<<" "<<stockMetas[i].code<<endl;
            stockMetas[i].offset = offset;
            offset += stockMetas[i].days;
            //cout<<(int)stockMetas[i].stockType<<" "<<stockMetas[i].days<<endl;
            if(stockMetas[i].days > maxDays){
                maxDays = stockMetas[i].days;
                mainStock = i;
            }
        }
        //cout<<stockMetas.getSize()<<" init "<<mainStock<<endl;
    }

    HashMap<StockMeta> stockMetas;
    size_t offset;
    //标记最主要的股票，用来做日历
    int mainStock;
};

#endif //ALPHATREE_STOCKDES_H
