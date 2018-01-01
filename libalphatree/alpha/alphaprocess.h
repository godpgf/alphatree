//
// Created by yanyu on 2017/10/21.
// process 是后处理,作用是将结果总结出一个结论,返回json
//

#ifndef ALPHATREE_ALPHAPROCESS_H
#define ALPHATREE_ALPHAPROCESS_H

#include <string.h>
#include "process.h"

class IAlphaProcess{
public:
    virtual const char* getName() = 0;
    //返回孩子数量
    virtual int getChildNum(){ return 0;}
    //传入左右孩子的参数,系数,输出最后的处理结果,以json格式返回
    virtual const char* process(AlphaDB* alphaDataBase, DArray<const float*, MAX_PROCESS_BLOCK>& childRes, const int* psign, DArray<char[MAX_PROCESS_COFF_STR_LEN],MAX_PROCESS_BLOCK>& coff, size_t sampleSize, const char* codes, size_t stockSize, char* pout) = 0;
};

class AlphaProcess : public IAlphaProcess{
    public:
        AlphaProcess(
                const char* name,
                const char* (*opt) (AlphaDB* alphaDataBase, DArray<const float*, MAX_PROCESS_BLOCK>& childRes, const int* psign, DArray<char[MAX_PROCESS_COFF_STR_LEN],MAX_PROCESS_BLOCK>& coff, size_t sampleSize, const char* codes, size_t stockSize, char* pout),
                int childNum
        ):name_(name),opt_(opt),childNum_(childNum){}

        const char* getName(){ return name_;}
        int getChildNum(){ return childNum_;}
        const char* process(AlphaDB* alphaDataBase, DArray<const float*, MAX_PROCESS_BLOCK>& childRes,const int* psign, DArray<char[MAX_PROCESS_COFF_STR_LEN],MAX_PROCESS_BLOCK>& coff, size_t sampleSize, const char* codes, size_t stockSize, char* pout){
            return opt_(alphaDataBase, childRes, psign, coff, sampleSize, codes, stockSize, pout);
        }

        static AlphaProcess alphaProcessList[];
    protected:
        const char*name_;
        const char* (*opt_) (AlphaDB* alphaDataBase, DArray<const float*, MAX_PROCESS_BLOCK>& childRes, const int* psign, DArray<char[MAX_PROCESS_COFF_STR_LEN],MAX_PROCESS_BLOCK>& coff, size_t sampleSize, const char* codes, size_t stockSize, char* pout);
        int childNum_;
};

AlphaProcess AlphaProcess::alphaProcessList[] = {
        //参数:买入卖出信号,close,atr
        AlphaProcess("eratio", eratio, 2),
};

#endif //ALPHATREE_ALPHAPROCESS_H
