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

const char* eratio(AlphaDB* alphaDataBase, DArray<const float*, MAX_PROCESS_BLOCK>& childRes, const int* psign, DArray<char[MAX_PROCESS_COFF_STR_LEN],MAX_PROCESS_BLOCK>& coff, size_t sampleSize, const char* codes, size_t stockSize, char* pout){
    float MAE = 0;
    float MFE = 0;
    float MRE = 0;

    //const float* high = childRes[0];
    //const float* low = childRes[1];
    const float* close = childRes[0];
    const float* atr = childRes[1];
    int signCount = 0;

    float MAEs[5];
    float MFEs[5];
    float MREs[5];
    int signCounts[5];
    float year_returns[5];
    memset(MAEs,0, sizeof(float)*5);
    memset(MFEs,0, sizeof(float)*5);
    memset(MREs,0, sizeof(float)*5);
    memset(year_returns,0, sizeof(float)*5);
    memset(signCounts,0, sizeof(int)*5);

    for(int i = 0; i < 5; ++i)
        MAEs[i] = 0.00000001f;

    float holdDay = 0;
    float yearReturn = 0;

    for(size_t j = 0; j < stockSize; ++j){
        //int buyIndex = -1;
        float maxPrice = 0;
        float minPrice = FLT_MAX;
        int curIndex = 0;

        for(size_t i = 0; i < sampleSize; ++i){
            curIndex = i * stockSize + j;

            if(psign[curIndex] > 0){
                holdDay += psign[curIndex];
                maxPrice = close[curIndex];
                minPrice = close[curIndex];
                if(atr[curIndex] == 0){
                    cout<<"atr:"<<atr[curIndex]<<" close:"<<maxPrice<<endl;
                }

                for(int k = 1; k <= psign[curIndex]; ++k){
                    //已经买入,等待卖出
                    maxPrice = max(maxPrice, close[curIndex + k * stockSize]);
                    minPrice = min(minPrice, close[curIndex + k * stockSize]);

                    if(k <= 5){
                        ++signCounts[k-1];
                        MFEs[k-1] += (maxPrice - close[curIndex]) / atr[curIndex];
                        MAEs[k-1] += (close[curIndex] - minPrice) / atr[curIndex];
                        MREs[k-1] += (close[curIndex + k * stockSize] - close[curIndex]) / atr[curIndex];
                        year_returns[k-1] += (close[curIndex + k * stockSize] - close[curIndex]) / close[curIndex] / k;
                    }
                }

                ++signCount;
                MFE += (maxPrice - close[curIndex]) / atr[curIndex];
                MAE += (close[curIndex] - minPrice) / atr[curIndex];
                MRE += (close[curIndex + psign[curIndex] * stockSize] - close[curIndex]) /atr[curIndex];
                yearReturn += (close[curIndex + psign[curIndex] * stockSize] - close[curIndex]) / close[curIndex] / psign[curIndex];
            }
        }
    }
    holdDay /= signCount;
    yearReturn = powf(1 + (yearReturn / signCount), 250) - 1;
    for(size_t i = 0; i < 5; ++i)
        year_returns[i] = powf(1 + (year_returns[i] / signCount), 250) - 1;
    sprintf(pout,"{\"eratio\" : %.4f, \"sign_cnt\" : %d, \"year_return\" : %.4f, \"hold_day\" : %.4f, \"MAE\" : %.4f, \"MFE\" : %.4f, \"MRE\" : %.4f, ", MAE == 0 ? 0 : MFE / MAE, signCount, yearReturn, holdDay / signCount, MAE / signCount, MFE / signCount, MRE / signCount);
    sprintf(pout + strlen(pout),"\"eratios\" : [%.4f, %.4f, %.4f, %.4f, %.4f], \"MREs\" : [%.4f, %.4f, %.4f, %.4f, %.4f], ", MFEs[0]/MAEs[0], MFEs[1]/MAEs[1], MFEs[2]/MAEs[2], MFEs[3]/MAEs[3], MFEs[4]/MAEs[4], MREs[0]/signCounts[0], MREs[1]/signCounts[1], MREs[2]/signCounts[2], MREs[3]/signCounts[3], MREs[4]/signCounts[4]);
    sprintf(pout + strlen(pout),"\"year_returns\" : [%.4f, %.4f, %.4f, %.4f, %.4f]}", year_returns[0], year_returns[1], year_returns[2], year_returns[3], year_returns[4]);
    //cout<<MAE<<" "<<MFE<<" "<<pout<<endl;
    return pout;
}

#endif //ALPHATREE_PROCESS_H
