//
// Created by yanyu on 2017/7/14.
//

#ifndef ALPHATREE_BASE_H
#define ALPHATREE_BASE_H

#include <math.h>
#include <string.h>
//todo 测试
#include <iostream>
#include <map>
using namespace std;

#define CHECK(isSuccess, err) if(!(isSuccess)) throw err

struct ptrCmp
{
    bool operator()( const char * s1, const char * s2 ) const
    {
        return strcmp( s1, s2 ) < 0;
    }
};

inline float _sign(float c){
    if(c > 0)
        return 1.f;
    if(c < 0)
        return -1.f;
    return 0.f;
}

inline void _sign(float *dst, const float *src, int size){
    for(int i = 0; i < size; ++i) dst[i] = _sign(src[i]);
}

inline void _signAnd(float* dst, const float* l, const float* r, int size){
    for(int i = 0; i < size; ++i){
        //cout<<l[i]<<" "<<r[i]<<" "<<endl;
        dst[i] = ((l[i] > 0 && r[i] > 0) ? 1 : 0);
        //cout<<dst[i]<<endl;
    }
}

inline void _signOr(float* dst, const float* l, const float* r, int size){
    for(int i = 0; i < size; ++i)
        dst[i] = ((l[i] > 0 || r[i] > 0) ? 1 : 0);
}

inline void _add(float* dst, const float* src, int size){
    for(int i = 0; i < size; ++i) dst[i] += src[i];
}

inline void _add(float* dst, float src, int size){
    for(int i = 0; i < size; ++i) dst[i] += src;
}

inline void _mulNoZero(float *dst, const float *src, int size){
    for(int i = 0; i < size; ++i){
        if(abs(src[i]) > 0.0001f)
            dst[i] *= src[i];
    }
}

inline void _mul(float *dst, const float *src, int size){
    for(int i = 0; i < size; ++i){
        dst[i] *= src[i];
    }
}

inline void _mul(float* dst, float src, int size){
    for(int i = 0; i < size; ++i){
        dst[i] *= src;
    }
}

inline void _addAndMul(float *dst, const float *src, float w, int size){
    for(int i = 0; i < size; ++i) dst[i] += src[i] * w;
}

inline void _reduce(float* dst,const float* src, int size){
    for(int i = 0; i < size; ++i) dst[i] -= src[i];
}

inline void _reduce(float src,float* dst, int size){
    for(int i = 0; i < size; ++i) dst[i] = src - dst[i];
}

inline void _reduce(float* dst, float src, int size){
    for(int i = 0; i < size; ++i) dst[i] -= src;
}

inline void _preRise(float* dst,const float* src, int size){
    for(int i = 0; i < size; ++i){
        if(abs(dst[i]) > 0.0001f)
            dst[i] = (dst[i] - src[i]) / dst[i];
    }
}

inline void _rise(float* dst,const float* src, int size){
    for(int i = 0; i < size; ++i){
        if(abs(src[i]) > 0.0001f)
            dst[i] = (dst[i] - src[i]) / src[i];
        else
            dst[i] = 0;
    }
}

inline void _divNoZero(float *dst, const float *src, int size){
    for(int i = 0; i < size; ++i){
        if(abs(src[i]) > 0.0001f){
            dst[i] /= src[i];
        }
    }
}

inline void _div(float* dst, float v, int size){
    v = 1.0f / v;
    for(int i = 0; i < size; ++i)
        dst[i] *= v;
}

inline void _div(float v, float* dst, int size){
    for(int i = 0; i < size; ++i){
        if(dst[i] == 0.0f || (dst[i] > 0.0f && dst[i] < 0.0001)){
            dst[i] =v / 0.0001;
        } else if(dst[i] < 0.0f && dst[i] > -0.0001){
            dst[i] = v / -0.0001;
        }
        else{
            dst[i] = v / dst[i];
        }
    }
}

inline void _div(float* dst, const float* a, const float* const b, int size){
    for(int i = 0; i < size; ++i) {
        //cout<<a[i]<<" "<<b[i]<<" "<<size<<endl;
        if (b[i] == 0.0f || (b[i] > 0 && b[i] < 0.0001)) {
            dst[i] = a[i] * 10000;
        } else if( b[i] < 0 and b[i] > -0.0001){
            dst[i] = a[i] * -10000;
        }
        else {
            dst[i] = a[i] / b[i];
        }
    }
}

//inline void _divCoff(float* dst, float coff, int historySize, int elementSize){
//    for(int i = 0; i < historySize; ++i)
//        _div(dst + i * elementSize,coff,elementSize);
//}

inline void _lerp(float* dst, const float* a, const float* b, float c, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = a[i] * c + b[i] * (1.0 - c);
    }
}

inline void _min(float *dst, const float *a, const float *b, int size){
    for(int i = 0; i < size; ++i)
        dst[i] = a[i] < b[i] ? a[i] : b[i];
}

inline void _minFrom(float *dst, const float a, const float *b, int size){
    for(int i = 0; i < size; ++i)
        dst[i] = a < b[i] ? a : b[i];
}

inline void _minTo(float *dst, const float *a, const float b, int size){
    for(int i = 0; i < size; ++i)
        dst[i] = a[i] < b ? a[i] : b;
}

inline void _max(float *dst, const float *a, const float *b, int size){
    for(int i = 0; i < size; ++i)
        dst[i] = a[i] > b[i] ? a[i] : b[i];
}

inline void _maxFrom(float *dst, const float a, const float *b, int size){
    for(int i = 0; i < size; ++i)
        dst[i] = a > b[i] ? a : b[i];
}

inline void _maxTo(float *dst, const float *a, const float b, int size){
    for(int i = 0; i < size; ++i)
        dst[i] = a[i] > b ? a[i] : b;
}

inline void _abs(float *dst, const float *src, int size){
    for(int i = 0; i < size; ++i)
        dst[i] = src[i] > 0 ? src[i] : -src[i];
}

inline void _log(float *dst, const float *src, int size,  float logmax){
    for(int i = 0; i < size; ++i){
        if(src[i] < 0.0001)
            dst[i] = logmax;
        else
            dst[i] = logf(src[i]);
    }
}

inline void _pow(float *dst, const float *a, const float *b, int size){
    for(int i = 0; i < size; ++i)
        if(a[i] < 0 && b[i] < 1)
            dst[i] = 0;
        else
            dst[i] = powf(a[i], b[i]);
}

inline void _pow(float *dst, const float *a, const float b, int size){
    for(int i = 0; i < size; ++i)
        if(a[i] < 0 && b < 1)
            dst[i] = 0;
        else
            dst[i] = powf(a[i], b);
}

inline void _pow(float *dst, const float a, const float *b, int size){
    for(int i = 0; i < size; ++i)
        if(a < 0 && b[i] < 1)
            dst[i] = 0;
        else
            dst[i] = powf(a, b[i]);
}

inline void _lessCond(float *dst, const float *a, const float *b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = (a[i] < b[i]) ? 1 : 0;
    }
}

inline void _lessCond(float *dst, float a, const float *b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = (a < b[i]) ? 1 : 0;
    }
}

inline void _lessCond(float *dst, const float *a, const float b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = (a[i] < b) ? 1 : 0;
        //cout<<dst[i]<<" "<<a[i]<<endl;
    }
}

inline void _moreCond(float *dst, float a, const float *b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = (a > b[i]) ? 1 : 0;
    }
}

inline void _moreCond(float *dst, const float *a, const float b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = (a[i] > b) ? 1 : 0;
    }
}

inline void _elseCond(float *dst, const float *a, const float *b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = isnan(a[i]) ? b[i] : a[i];
    }
}

inline void _elseCond(float *dst, const float *a, const float b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = isnan(a[i]) ? b : a[i];
    }
}


inline void _ifCond(float *dst, const float *a, const float *b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = (a[i] > 0) ? b[i] : NAN;
    }
}

inline void _ifCond(float *dst, const float *a, float b, int size){
    for(int i = 0; i < size; ++i) {
        dst[i] = (a[i] > 0) ? b : NAN;
    }
}

inline void _negativeFlag(float*dst, const float*data,const float* flag, int size){
    for(int i = 0; i < size; ++i){
        dst[i] = (flag[i] < 0.00001) ? data[i] : -data[i];
    }
}


//注意!计算数组src的各个元素排行
void quickSort(const float* src, float* index, int left, int right){
    if(left >= right)
        return;
    int key = (int)index[left];
//
//    if(isnan(src[key])){
//        quickSort(src, index, left + 1, right);
//        cout<<"err nan "<<key<<endl;
//        throw "eee";
//        return;
//    }

    int low = left;
    int high = right;
    while (low < high){
        //while (low < high && (isnan(src[(int)index[high]]) || src[(int)index[high]] > src[key])){
        while (low < high && src[(int)index[high]] > src[key]){
            --high;
        }
        if(low < high)
            index[low++] = index[high];
        else
            break;

        //while (low < high && (isnan(src[(int)index[low]]) || src[(int)index[low]] <= src[key])){
        while (low < high && src[(int)index[low]] <= src[key]){
            ++low;
        }
        if(low < high)
            index[high--] = index[low];
    }
    index[low] = key;

    quickSort(src, index, left, low - 1);
    quickSort(src, index, low+1, right);
}

void _ranksort(float *index, const float *src, int size){
    for(int i = 0; i < size; ++i)
        index[i] = i;
    quickSort(src, index, 0, size-1);
    //for(int i = 0; i < size; ++i){
    //    if(flag[(int)index[i]] == false){
    //        index[i] = -index[i]-1;
    //    }
    //}
}

void _rankscale(float *dst,const float *index, int size){
    int elementSize = 0;
    for(int i = 0; i < size; ++i)
        if(index[i] >= 0)
            ++elementSize;

    int curSortId = 0;
    for(int i = 0; i < size; ++i){
        if(index[i] >= 0){
            int elementId = ((int)index[i]);
            //因为是从小到大排序,而rank是从大到小的,所以在这里转一下
            dst[elementId] = (float)(elementSize - curSortId - 1) / (float)(elementSize-1);
            ++curSortId;
        }else{
            int elementId = ((int)(-index[i] - 1));
            dst[elementId] = -0.001;
        }
    }
}

void _powerMid(float *dst, const float *a, const float *b, int size){
    for(int i = 0; i < size; ++i){
        dst[i] = sqrtf(a[i] * b[i]);
    }
}

void _tsRank(float *dst,const float *curData, const float *beforeData, int size){
    for(int i = 0; i < size; ++i){
        dst[i] += curData[i] > beforeData[i] ? 1.0f : 0.0f;
    }
//    for(int k = 0; k < stockSize; ++k){
//        pout[curBlockIndex+k] += (pleft[curBlockIndex+k] > pleft[beforeBlockIndex+k]) ? 1.0f : 0.0f;
//    }
}

//void _ranking(float *dst, float *index, const float *src, int size){
//    for(int i = 0; i < size; ++i){
//        index[i] = i;
//    }
//    quickSort(src, index, 0, size-1);
//
//    for(int i = 0; i < size; ++i){
//        int elementId = ((int)index[i]);
//        //因为是从小到大排序,而rank是从大到小的,所以在这里转一下
//        dst[elementId] = (float)(size - i - 1) / (float)(size-1);
//    }
//}

inline void _scale(float *dst, int size){
    float sum = 0;
    for(int i = 0; i < size; ++i)
        //if(flag[i])
            sum += dst[i] >= 0 ? dst[i] : -dst[i];
    if(sum < 0.0001)
        memset(dst,0,sizeof(float) * size);
    else
        for(int i = 0; i < size; ++i)
            dst[i] /= sum;
}

//返回range范围内的最小值的id
inline void _tsMinIndex(float *dst, const float *src, int historySize, int elementSize, int range){

    if(range > 0){
        memset(dst,0, sizeof(float) * elementSize);
        for(int i = 1; i < historySize; ++i) {
            for (int j = 0; j < elementSize; ++j) {
                int lastId = (int) dst[(i - 1) * elementSize + j];
                //如果出现相等的情况，后面的id优先
                if (lastId + range >= i) {
                    //上一个元素记录的最小值在自己的视野内
                    dst[i * elementSize + j] = (src[i * elementSize + j] <= src[lastId * elementSize + j]) ? i : lastId;
                } else {
                    int minId = i;
                    for (int k = 1; k <= range; ++k) {
                        if (src[(i - k) * elementSize + j] < src[minId * elementSize + j])
                            minId = i - k;
                    }
                    dst[i * elementSize + j] = minId;
                }
            }
        }
    } else {
        for(int j = 0; j < elementSize; ++j)
            dst[(historySize - 1) * elementSize + j] = historySize - 1;
        for(int i = historySize - 2; i >= 0; --i){
            for(int j = 0; j < elementSize; ++j) {
                int lastId = (int) dst[(i + 1) * elementSize + j];
                if(lastId + range <= i){
                    //在自己的视野内
                    dst[i * elementSize + j] = (src[i * elementSize + j] <= src[lastId * elementSize + j]) ? i : lastId;
                } else {
                    int  minId = i;
                    for(int k = -1; k >= range; --k){
                        if(src[(i - k) * elementSize + j] < src[minId * elementSize + j])
                            minId = i - k;
                    }
                    dst[i * elementSize + j] = minId;
                }
            }
        }
    }

}
inline void _tsMaxIndex(float *dst, const float *src, int historySize, int elementSize, int range){
    if(range > 0){
        memset(dst,0, sizeof(float) * elementSize);
        for(int i = 1; i < historySize; ++i) {
            for (int j = 0; j < elementSize; ++j) {
                int lastId = (int) dst[(i - 1) * elementSize + j];
                //如果出现相等的情况，后面的id优先
                if (lastId + range >= i) {
                    //上一个元素记录的最小值在自己的视野内
                    dst[i * elementSize + j] = (src[i * elementSize + j] >= src[lastId * elementSize + j]) ? i : lastId;
                } else {
                    int minId = i;
                    for (int k = 1; k <= range; ++k) {
                        if (src[(i - k) * elementSize + j] > src[minId * elementSize + j])
                            minId = i - k;
                    }
                    dst[i * elementSize + j] = minId;
                }
            }
        }
    } else {
        for(int j = 0; j < elementSize; ++j)
            dst[(historySize - 1) * elementSize + j] = historySize - 1;
        for(int i = historySize - 2; i >= 0; --i){
            for(int j = 0; j < elementSize; ++j) {
                int lastId = (int) dst[(i + 1) * elementSize + j];
                if(lastId + range <= i){
                    //在自己的视野内
                    dst[i * elementSize + j] = (src[i * elementSize + j] >= src[lastId * elementSize + j]) ? i : lastId;
                } else {
                    int  minId = i;
                    for(int k = -1; k >= range; --k){
                        if(src[(i - k) * elementSize + j] > src[minId * elementSize + j])
                            minId = i - k;
                    }
                    dst[i * elementSize + j] = minId;
                }
            }
        }
    }
}

/*
 *最小二乘法 线性回归
 * y = beta * x + alpha
 * beta = sum(y) / n - alpha * sum(x) / n
 * alpha = ( n * sum( xy ) - sum( x ) * sum( y ) ) / ( n * sum( x^2 ) - sum(x) ^ 2 )
 * */
void lstsq_(const float* x, const float* y, int historySize, float& alpha, float& beta){
    float sumx = 0.f;
    float sumy = 0.f;
    float sumxy = 0.f;
    float sumxx = 0.f;
    for(int j = 0; j < historySize; ++j){
        sumx += x[j];
        sumy += y[j];
        sumxy += x[j] * y[j];
        sumxx += x[j] * x[j];
    }
    float tmp = (historySize * sumxx - sumx * sumx);
    beta = abs(tmp) < 0.0001f ? 0 : (historySize * sumxy - sumx * sumy) / tmp;
    alpha = abs(tmp) < 0.0001f ? 0 : sumy / historySize - beta * sumx / historySize;
}
void lstsq_(float* x, float* y, int historySize, int elementSize, float* alpha, float* beta){
    for(int i = 0; i < elementSize; ++i){
        float sumx = 0.f;
        float sumy = 0.f;
        float sumxy = 0.f;
        float sumxx = 0.f;
        for(int j = 0; j < historySize; ++j){
            sumx += x[j * elementSize + i];
            sumy += y[j * elementSize + i];
            sumxy += x[j * elementSize + i] * y[j * elementSize + i];
            sumxx += x[j * elementSize + i] * x[j * elementSize + i];
        }
        float tmp = (historySize * sumxx - sumx * sumx);
        beta[i] = abs(tmp) < 0.0001f ? 0 : (historySize * sumxy - sumx * sumy) / tmp;
        alpha[i] = abs(tmp) < 0.0001f ? 0 : sumy / historySize - beta[i] * sumx / historySize;
    }
}

/*
 * 计算线性回归的标准差
 * */
void lstd(float* x, float* y, int historySize, int elementSize, const float* alpha, const float* beta, float* alphaSTD,
          float* betaSTD){
    for(int i = 0; i < elementSize; ++i){
        float sumAlphaErr_2 = 0;
        float sumAlphaErr = 0;
        float sumBetaErr_2 = 0;
        float sumBetaErr = 0;
        for(int j = 0; j < historySize; ++j){
            //真实alpha-平均alpha
            float err = (y[j * elementSize + i] - x[j * elementSize + i] * beta[i]) - alpha[i];
            sumAlphaErr += err;
            sumAlphaErr_2 += err * err;

            if(fabsf(x[j * elementSize + i]) < 0.0001) {
                err = 0;
            }
            else{
                err = (y[j * elementSize + i] - alpha[i]) / x[j * elementSize + i] - beta[i];
            }
            sumBetaErr += err;
            sumBetaErr_2 += err * err;
        }
        alphaSTD[i] = sqrtf((sumAlphaErr_2 - sumAlphaErr * sumAlphaErr / historySize) / historySize);
        betaSTD[i] = sqrtf((sumBetaErr_2 - sumBetaErr * sumBetaErr / historySize) / historySize);
    }
}



#endif //ALPHATREE_BASE_H
