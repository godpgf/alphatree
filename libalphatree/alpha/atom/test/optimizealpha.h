//
// Created by godpgf on 18-3-13.
//

#ifndef ALPHATREE_OPTIMIZEALPHA_H
#define ALPHATREE_OPTIMIZEALPHA_H

#include "normal.h"
#include "alpha.h"

#define MIN_DATA_NUM_IN_BAR 5

void* noiseValid(void** pars, float coff, int historySize, int stockSize, CacheFlag* pflag){
    //memcpy(pout, pleft, historySize * stockSize * sizeof(float));
    float* src = (float*)pars[0];
    float* t0 = (float*)pars[1];
    float* t1 = (float*)pars[2];
    float* pout = (float*)pars[3];
    int barSize = (int)coff;

    memset(pout, 0, barSize * 3 * sizeof(float));
    float* t0_out = pout;
    float* t1_out = pout + barSize;
    float* cnt_out = pout + 2 * barSize;

    //统计柱状图
    float sum = 0;
    float sumSqr = 0;
    int dataNum = 0;
    for(int i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            ++dataNum;
        }
    }
    dataNum *= stockSize;
    int realDataNum = 0;
    for(int i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            int curIndex = i * stockSize;
            for(int j = 0; j < stockSize; ++j){
                if(t0[curIndex + j] * t1[curIndex + j] > 0){
                    float data = src[curIndex + j];
                    sum += data / dataNum;
                    sumSqr += powf(data, 2.0f) / dataNum;
                    ++realDataNum;
                }
            }
        }
    }
    sum *= ((float)dataNum / (float)realDataNum);
    sumSqr *= ((float)dataNum / (float)realDataNum);
    dataNum = realDataNum;

    float avg = sum;
    float std = sqrtf(sumSqr  - avg * avg);
    if(std == 0 || isnan(std) || isinf(std)){
        cout<<"数据居然全部一样！\n";
        /*for(int i = 0; i < historySize; ++i){
            if(pflag[i] == CacheFlag::NEED_CAL){
                int curIndex = i * stockSize;
                for(int j = 0; j < stockSize; ++j){
                    cout<<" "<< src[curIndex + j];
                }
            }
            cout<<endl;
        }
        throw "err";*/
        memset(pout,0,historySize * stockSize * sizeof(float));
        return pout;
    }
    //初始化柱状图每个柱子的范围
    float stdScale =  normsinv(1.0 - 1.0 / barSize);
    float startValue = avg - std * stdScale;
    float deltaStd = std * stdScale / (0.5f * (barSize - 2));

    float avg_t0 = 0;
    float avg_t1 = 0;
    for(int i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            int curIndex = i * stockSize;
            for(int j = 0; j < stockSize; ++j){
                //保证未来数据不缺失并且有上下波动
                if(t0[curIndex + j] * t1[curIndex + j] > 0){
                    int index = src[curIndex + j] < startValue ? 0 : std::min((int)((src[curIndex + j] - startValue) / deltaStd) + 1, barSize - 1);
                    t0_out[index] += t0[curIndex + j];
                    t1_out[index] += t1[curIndex + j];
                    cnt_out[index] += 1;
                    avg_t0 += t0[curIndex + j] / dataNum;
                    avg_t1 += t1[curIndex + j] / dataNum;
                }
            }
        }
    }

    /*{
        for(int i = 0; i < barSize; ++i){
            cout<<t0_out[i]<<" ";
        }
        cout<<endl;

        for(int i = 0; i < barSize; ++i){
            cout<<t1_out[i]<<" ";
        }
        cout<<endl;
    }*/


    float avgValue = avg_t0 / avg_t1;
    float stdValue = 0;
    for(int i = 0; i < barSize; ++i){
        float curValue = abs(t1_out[i]) < 0.0001f || cnt_out[i] < MIN_DATA_NUM_IN_BAR ? avgValue : t0_out[i] / t1_out[i];
        //cout<<curValue<<" "<<cnt_out[i]<<" "<<t0_out[i]<<" "<<t1_out[i]<<endl;
        stdValue += (curValue - avgValue) * (curValue - avgValue) * cnt_out[i] / dataNum;
    }

    stdValue = sqrtf(stdValue);
    for(int i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            int curIndex = i * stockSize;
            for(int j = 0; j < stockSize; ++j){
                pout[curIndex + j] = stdValue;
            }
        }
    }

    return pout;
}

void* alphaCorrelation(void** pars, float coff, int historySize, int stockSize, CacheFlag* pflag) {
    float* pout = (float*)(pars[0]);
    float* pleft = (float*)(pars[0]);
    float* pright = (float*)(pars[1]);

    int dataNum = 0;
    for(int i = historySize-1; i >= 0; --i) {
        if (pflag[i] == CacheFlag::NEED_CAL)
            ++dataNum;
    }
    dataNum *= stockSize;

    //计算当前股票的均值和方差
    float meanLeft = 0;
    float meanRight = 0;
    float sumSqrLeft = 0;
    float sumSqrRight = 0;
    for(int i = historySize-1; i >= 0; --i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            //计算第日股票前d天的相关系数
            for(int j = 0; j < stockSize; ++j){
                int curBlock = i * stockSize;
                meanLeft += pleft[curBlock + j] / dataNum;
                sumSqrLeft += pleft[curBlock + j] * pleft[curBlock + j] / dataNum;
                meanRight += pright[curBlock + j] / dataNum;
                sumSqrRight += pright[curBlock + j] * pright[curBlock + j] / dataNum;
            }
        }
    }

    float cov = 0;
    for(int i = historySize-1; i >= 0; --i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            //计算第日股票前d天的相关系数
            for(int j = 0; j < stockSize; ++j){
                int curBlock = i * stockSize;
                cov += (pleft[curBlock + j] - meanLeft) * (pright[curBlock + j] - meanRight) / dataNum;
            }
        }
    }

    //cov /= (d+1);
    float xDiff2 = (sumSqrLeft - meanLeft*meanLeft);
    float yDiff2 = (sumSqrRight - meanRight*meanRight);
    float res;
    if(xDiff2 < 0.000000001 || yDiff2 < 0.000000001)
        res = 0;
    else
        res = cov / sqrtf(xDiff2) / sqrtf(yDiff2);

    for(int i = 0; i < historySize; ++i){
        if(pflag[i] == CacheFlag::NEED_CAL){
            int curIndex = i * stockSize;
            for(int j = 0; j < stockSize; ++j){
                pout[curIndex + j] = res;
            }
        }
    }
    return pout;
}
#endif //ALPHATREE_OPTIMIZEALPHA_H
