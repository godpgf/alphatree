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
const size_t MAX_PROCESS_COFF_STR_LEN = 64;

const char* eratio(AlphaDataBase* alphaDataBase, DArray<const float*, MAX_PROCESS_BLOCK>& childRes, DArray<char[MAX_PROCESS_COFF_STR_LEN],MAX_PROCESS_BLOCK>& coff, size_t sampleSize, const char* codes, size_t stockSize, char* pout){
    float MAE = 0;
    float MFE = 0;
    const float* buy = childRes[0];
    const float* sell = childRes[1];
    const float* close = childRes[2];
    const float* atr = childRes[3];
    int signCount = 0;

    for(int j = 0; j < stockSize; ++j){
        int buyIndex = -1;
        float maxPrice = 0;
        float minPrice = FLT_MAX;
        int curIndex = 0;
        int preIndex = 0;

        for(int i = 1; i < sampleSize; ++i){
            curIndex = i * stockSize + j;
            preIndex = (i-1) * stockSize + j;
            if(!isnan(buy[curIndex]) && !isnan(atr[curIndex] && !isnan(close[curIndex]) && !isnan(sell[curIndex]) && !isnan(close[preIndex]))){
                if(buyIndex == -1 && buy[curIndex] > 0){
                    buyIndex = preIndex;
                    maxPrice = close[buyIndex];
                    minPrice = close[buyIndex];
                    //cout<<buyIndex<<" "<<close[buyIndex]<<endl;
                }
                if(buyIndex != -1){
                    //已经买入,等待卖出
                    maxPrice = max(maxPrice, close[curIndex]);
                    minPrice = min(minPrice, close[curIndex]);
                    if(sell[curIndex] > 0){
                        MFE += ((maxPrice - close[buyIndex]) / close[buyIndex] / atr[curIndex]);
                        MAE += ((close[buyIndex] - minPrice) / close[buyIndex] / atr[curIndex]);
                        //cout<<buyIndex<<" "<<close[buyIndex]<<" "<<close[buyIndex]<<" "<<atr[curIndex]<<" "<<MFE<<" "<<MAE<<" "<<(curIndex - buyIndex) / stockSize<<endl;
                        buyIndex = -1;
                        maxPrice = 0;
                        minPrice = FLT_MAX;
                        ++signCount;
                    }
                }
            }
        }
    }
    sprintf(pout,"{\"eratio\" : %.4f, \"sign_cnt\" : %d}", MAE == 0 ? 0 : MFE / MAE, signCount);
    cout<<pout<<endl;
    return pout;
}

#endif //ALPHATREE_PROCESS_H
