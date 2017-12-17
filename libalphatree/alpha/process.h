//
// Created by yanyu on 2017/10/21.
//

#ifndef ALPHATREE_PROCESS_H
#define ALPHATREE_PROCESS_H

#include <string.h>
#include <math.h>
#include "alphadb.h"
#include "../base/darray.h"

const size_t MAX_PROCESS_BLOCK = 8;
const size_t MAX_PROCESS_COFF_STR_LEN = 2048;

const char* eratio(AlphaDB* alphaDataBase, DArray<const float*, MAX_PROCESS_BLOCK>& childRes, DArray<char[MAX_PROCESS_COFF_STR_LEN],MAX_PROCESS_BLOCK>& coff, size_t sampleSize, const char* codes, size_t stockSize, char* pout){
    float MAE = 0;
    float MFE = 0;
    const float* buy = childRes[0];
    const float* sell = childRes[1];
    const float* high = childRes[2];
    const float* low = childRes[3];
    const float* close = childRes[4];
    const float* atr = childRes[5];
    int signCount = 0;

    float holdDay = 0;
    float yearReturns = 0;

    for(size_t j = 0; j < stockSize; ++j){
        int buyIndex = -1;
        float maxPrice = 0;
        float minPrice = FLT_MAX;
        int curIndex = 0;



        for(size_t i = 1; i < sampleSize; ++i){
            curIndex = i * stockSize + j;
            if(buyIndex == -1 && buy[curIndex] > 0 && sell[curIndex] == 0){
                buyIndex = curIndex;
                maxPrice = close[buyIndex];
                minPrice = close[buyIndex];
                continue;
            }
            if(buyIndex != -1){
                //已经买入,等待卖出
                maxPrice = max(maxPrice, high[curIndex]);
                minPrice = min(minPrice, low[curIndex]);
                if(sell[curIndex] > 0){

                    MFE += ((maxPrice - close[buyIndex]) / atr[buyIndex]);
                    MAE += ((close[buyIndex] - minPrice) / atr[buyIndex]);
                    yearReturns += (close[curIndex] - close[buyIndex]) / close[buyIndex] / ((curIndex - buyIndex) / stockSize);
                    holdDay += (curIndex - buyIndex) / stockSize;

                    buyIndex = -1;
                    maxPrice = 0;
                    minPrice = FLT_MAX;
                    ++signCount;
                }
            }
        }
    }
    yearReturns = powf(1 + (yearReturns / signCount), 250) - 1;
    sprintf(pout,"{\"eratio\" : %.4f, \"sign_cnt\" : %d, \"year_return\" : %.4f, \"hold_day\" : %.4f}", MAE == 0 ? 0 : MFE / MAE, signCount, yearReturns, holdDay / signCount);
    //cout<<MAE<<" "<<MFE<<" "<<pout<<endl;
    return pout;
}

#endif //ALPHATREE_PROCESS_H
