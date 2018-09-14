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
            string code, market, industry, price, days;
            getline(ss, code, ',');
            getline(ss, market, ',');
            getline(ss, industry, ',');
            getline(ss, price, ',');
            getline(ss, days, ',');

            StockMeta sm;
            strcpy(sm.code, code.c_str());
            sm.days = atoi(days.c_str());
			//godpgf
//			cout << code << " " << sm.days << endl;
            //offset += sm.days;
            //cout<<market<<" "<<market.length()<<" "<<industry<<" "<<industry.length()<<endl;
            if(market.length() != 0 && industry.length() == 0){
                sm.stockType = StockMeta::StockType::MARKET;
                strcpy(sm.marker, market.c_str());
                strcpy(sm.industry, market.c_str());
            } else {
                if(market.length() == 0 && industry.length() != 0){
                    sm.stockType = StockMeta::StockType::INDUSTRY;
                    strcpy(sm.marker, industry.c_str());
                    strcpy(sm.industry, industry.c_str());
                } else {
                    sm.stockType = StockMeta::StockType::STOCK;
                    strcpy(sm.marker, market.c_str());
                    strcpy(sm.industry, industry.c_str());
                }
            }
            //cout<<sm.code<<" "<<(int)sm.stockType<<endl;
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

    int offset2index(size_t curOffset){
        if (curOffset >= stockMetas[stockMetas.getSize()-1].offset)
            return stockMetas.getSize() - 1;
        int l = 0, r = stockMetas.getSize() - 2;

        do{
            int mid = (l + r) >> 1;
            if(curOffset >= stockMetas[mid].offset){
                if(curOffset < stockMetas[mid+1].offset)
                    return mid;
                l = mid + 1;
            } else {
                r = mid - 1;
            }
        }while (l < r);
        if(!(curOffset >= stockMetas[l].offset && curOffset < stockMetas[l + 1].offset)){
            cout<<"找不到偏移\n";
            throw "error";
        }
        //cout<<"curoffset:"<<curOffset<<" find:"<<stockMetas[l].code<<" offset:"<<stockMetas[l].offset<<" days:"<<stockMetas[l].days<<endl;
        return l;
    }

    HashMap<StockMeta> stockMetas;
    size_t offset;
    //标记最主要的股票，用来做日历
    int mainStock;
};

#endif //ALPHATREE_STOCKDES_H
