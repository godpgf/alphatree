//
// Created by godpgf on 18-2-27.
//

#ifndef ALPHATREE_BACKTRACE_H
#define ALPHATREE_BACKTRACE_H

#include "alpha.h"

void* ftSharp(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    //size_t dataSize = historySize*stockSize;

    float* pout = (float*)pars[2];
    float* sign = (float*)pars[0];
    float* close = (float*)pars[1];
    //float* pright = (float*)pars[2];

    int curIndex = 0;
    float maxDropdown = 0;
    float maxPrice = 0;
    float meanDropdown = 0;
    float sumSqrDropdown = 0;
    size_t signCount = 0;
    for(size_t j = 0; j < stockSize; ++j){
        for(size_t i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                curIndex = i * stockSize + j;
                if(sign[curIndex] > 0 ){
                    maxPrice = close[curIndex];
                    maxDropdown = 0;
                    for(size_t k = 1; k <= sign[curIndex]; ++k){
                        maxPrice = max(close[curIndex + k * stockSize],maxPrice);
                        maxDropdown = max((maxPrice - close[curIndex + k * stockSize]) / maxPrice, maxDropdown);
                    }
                    pout[curIndex] = maxDropdown;
                    meanDropdown += maxDropdown;
                    ++signCount;
                }
            }
        }
    }
    meanDropdown /= signCount;
    for(size_t j = 0; j < stockSize; ++j){
        for(size_t i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                curIndex = i * stockSize + j;
                if(sign[curIndex] > 0 ){
                    sumSqrDropdown += powf(pout[curIndex]-meanDropdown, 2);
                }
            }
        }
    }
    //dropdown的不确定性
    float std = sqrtf(sumSqrDropdown / signCount);
    for(size_t j = 0; j < stockSize; ++j){
        for(size_t i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                curIndex = i * stockSize + j;
                if(sign[curIndex] > 0 ){
                    //风险=dropdown的不确定性+dropdown本身
                    pout[curIndex] = (close[curIndex + (int)sign[curIndex] * stockSize] - close[curIndex])/close[curIndex]/(pout[curIndex]+std);
                }
            }
        }
    }
    return pout;
}


void* resEratio(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* buy = (float*)pars[0];
    float* close = (float*)pars[1];
    float* atr = (float*)pars[2];
    char* pout = (char*)pars[3];

    float MAE[] = {0,0,0,0,0,0};
    float MFE[] = {0,0,0,0,0,0};
    int signCounts[] = {0,0,0,0,0,0};
    float returns[] = {0,0,0,0,0,0};
    float returnsSqr[] = {0,0,0,0,0,0};
    float holdDay = 0;

    float maxPrice = 0;
    float minPrice = FLT_MAX;

    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            for(size_t j = 0; j < stockSize; ++j){
                auto curIndex = i * stockSize + j;
                if(buy[curIndex] > 0){
                    int buyDay = (int)buy[curIndex];
                    holdDay += buyDay;
                    maxPrice = close[curIndex];
                    minPrice = close[curIndex];
                    for(int k = 1; k <= buyDay; ++k){
                        //已经买入,等待卖出
                        maxPrice = max(maxPrice, close[curIndex + k * stockSize]);
                        minPrice = min(minPrice, close[curIndex + k * stockSize]);

                        if(k <= 5){
                            ++signCounts[k-1];
                            MFE[k-1] += (maxPrice - close[curIndex]) / max(atr[curIndex],0.001f);
                            MAE[k-1] += (close[curIndex] - minPrice) / max(atr[curIndex],0.001f);
                            returns[k-1] += (close[curIndex + k * stockSize] - close[curIndex]) / max(close[curIndex],0.001f);
                            returnsSqr[k-1] += powf((close[curIndex + k * stockSize] - close[curIndex]) / max(close[curIndex],0.001f), 2);
                        }
                    }
                    ++signCounts[5];
                    //cout<<(maxPrice - close[curIndex])<<"/"<<(close[curIndex] - minPrice)<<endl;
                    MFE[5] += (maxPrice - close[curIndex]) / max(atr[curIndex],0.001f);
                    MAE[5] += (close[curIndex] - minPrice) / max(atr[curIndex],0.001f);
                    returns[5] += (close[curIndex + buyDay * stockSize] - close[curIndex]) / max(close[curIndex],0.001f);
                    returnsSqr[5] += powf((close[curIndex + buyDay * stockSize] - close[curIndex]) / max(close[curIndex],0.001f), 2);
                }
            }
        }
    }

    sprintf(pout, "{\"eratio\" : [%.4f, %.4f, %.4f, %.4f, %.4f, %.4f], \"sharp\" : [%.4f, %.4f, %.4f, %.4f, %.4f, %.4f],\"cnt\" : [%d, %d, %d, %d, %d, %d], \"avg_hold_day\" : %.4f}",
            MAE[0] == 0 ? 1 : MFE[0]/MAE[0],
            MAE[1] == 0 ? 1 : MFE[1]/MAE[1],
            MAE[2] == 0 ? 1 : MFE[2]/MAE[2],
            MAE[3] == 0 ? 1 : MFE[3]/MAE[3],
            MAE[4] == 0 ? 1 : MFE[4]/MAE[4],
            MAE[5] == 0 ? 1 : MFE[5]/MAE[5],
            (returns[0] / signCounts[0]) / sqrtf(returnsSqr[0] / signCounts[0] - powf(returns[0] / signCounts[0], 2)),
            (returns[1] / signCounts[1]) / sqrtf(returnsSqr[1] / signCounts[1] - powf(returns[1] / signCounts[1], 2)),
            (returns[2] / signCounts[2]) / sqrtf(returnsSqr[2] / signCounts[2] - powf(returns[2] / signCounts[2], 2)),
            (returns[3] / signCounts[3]) / sqrtf(returnsSqr[3] / signCounts[3] - powf(returns[3] / signCounts[3], 2)),
            (returns[4] / signCounts[4]) / sqrtf(returnsSqr[4] / signCounts[4] - powf(returns[4] / signCounts[4], 2)),
            (returns[5] / signCounts[5]) / sqrtf(returnsSqr[5] / signCounts[5] - powf(returns[5] / signCounts[5], 2)),
            signCounts[0],signCounts[1],signCounts[2],signCounts[3],signCounts[4],signCounts[5],
            signCounts[5] == 0 ? 0 : holdDay / (float)signCounts[5]
    );
    return pout;
}

void* optShape(void** pars, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag){
    float* buy = (float*)pars[0];
    float* close = (float*)pars[1];
    float* pout = (float*)pars[0];


    int signCounts = 0;
    float returns = 0;
    float returnsSqr = 0;

    float maxPrice = 0;
    float minPrice = FLT_MAX;

    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            for(size_t j = 0; j < stockSize; ++j){
                auto curIndex = i * stockSize + j;
                if(buy[curIndex] > 0){
                    int buyDay = (int)buy[curIndex];
                    maxPrice = close[curIndex];
                    minPrice = close[curIndex];
                    for(int k = 1; k <= buyDay; ++k){
                        //已经买入,等待卖出
                        maxPrice = max(maxPrice, close[curIndex + k * stockSize]);
                        minPrice = min(minPrice, close[curIndex + k * stockSize]);
                    }
                    ++signCounts;
                    returns += (close[curIndex + buyDay * stockSize] - close[curIndex]) / max(close[curIndex],0.001f);
                    returnsSqr += powf((close[curIndex + buyDay * stockSize] - close[curIndex]) / max(close[curIndex],0.001f), 2);
                }
            }
        }
    }

    float s = (returns / signCounts) / sqrtf(returnsSqr / signCounts - powf(returns / signCounts, 2));

    for(size_t i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            size_t curIndex = i * stockSize;
            pout[curIndex] = s;
        }
    }

    return pout;
}

#endif //ALPHATREE_BACKTRACE_H
