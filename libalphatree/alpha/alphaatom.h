//
// Created by yanyu on 2017/7/12.
//

#ifndef ALPHATREE_ALPHAATOM_H
#define ALPHATREE_ALPHAATOM_H

#include "alphadb.h"
#include "atom/alpha.h"


//系数单位
enum class CoffUnit{
    COFF_NONE = 0,
    COFF_DAY = 1,
    COFF_VAR = 2,
    COFF_CONST = 3,
    COFF_INDCLASS = 4,
    COFF_FORBIT_INDCLASS = 5,
};

//计算的天数范围
enum class DateRange{
    CUR_DAY = 0,
    BEFORE_DAY = 1,
    CUR_AND_BEFORE_DAY = 2,
    ALL_DAY = 3,
};

class IAlphaElement{
    public:
        virtual const char* getName() = 0;
        //返回孩子数量
        //virtual int getChildNum(){ return 0;}
        //返回最小需要考虑的历史天数
        //virtual int getMinHistoryDays(){ return 0;}
        //传入左右孩子的参数,系数,输出内存块   得到运算结果
        //virtual const float* cast(const float* pleft, const float* pright, float coff, int historySize, int stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout, int* sign) = 0;
        //传入孩子的内存和给自己分配的内存，返回结果内存
        virtual void* cast(void** pars, float coff, int historySize, int stockSize, CacheFlag* pflag) = 0;
        //得到系数类型
        virtual CoffUnit getCoffUnit() { return CoffUnit::COFF_NONE;}
        //得到计算天数
        virtual DateRange getDateRange(){ return DateRange::CUR_DAY; }
        //得到需要的参数个数
        virtual int getParNum(){ return 1;}
        //得到需要返回的参数下标
        virtual int getOutParIndex(){ return 0;}

        virtual float getMinCoff(){ return 0;}
        virtual float getMaxCoff(){ return 0;}
};

//原子alpha
class AlphaAtom: public IAlphaElement{
    public:
        AlphaAtom(
                const char* name,
                void* (*opt) (void** pars, float coff, int historySize, int stockSize, CacheFlag* pflag),
                int parNum = 1,
                int outParIndex = 0,
                CoffUnit coffUnit = CoffUnit::COFF_NONE,
                DateRange dateRange = DateRange::CUR_DAY
        ):name_(name), opt_(opt), parNum_(parNum), outParIndex_(outParIndex), coffUnit_(coffUnit), dateRange_(dateRange){
            switch (coffUnit){
                case CoffUnit::COFF_DAY:
                    minCoff_ = 1;
                    maxCoff_ = 50;
                    break;
                case CoffUnit::COFF_VAR:
                    minCoff_ = 0;
                    maxCoff_ = 1;
                    break;
                default:
                    break;
            }
        }

        AlphaAtom* initCoff(float minCoff, float maxCoff){
            minCoff_ = minCoff;
            maxCoff_ = maxCoff;
            return this;
        }
        virtual float getMinCoff(){ return minCoff_;}
        virtual float getMaxCoff(){ return maxCoff_;}


        virtual const char* getName(){ return name_;}



        virtual void* cast(void** pars, float coff, int historySize, int stockSize, CacheFlag* pflag){
            return opt_(pars, coff, historySize, stockSize, pflag);
        }

        virtual CoffUnit getCoffUnit() { return coffUnit_;}

        virtual DateRange getDateRange(){ return dateRange_; }

        //得到需要的参数个数
        virtual int getParNum(){ return parNum_;}
        //得到需要返回的参数下标
        virtual int getOutParIndex(){ return outParIndex_;}

        static AlphaAtom alphaAtomList[];

    private:
        const char* name_;
        void* (*opt_) (void** pars, float coff, int historySize, int stockSize, CacheFlag* pflag);
        int parNum_;
        int outParIndex_;
        float minCoff_, maxCoff_;
        //参数单位
        CoffUnit coffUnit_;
        //计算天数
        DateRange dateRange_;
};

AlphaAtom AlphaAtom::alphaAtomList[] = {
        AlphaAtom("ma", mean, 1, 0, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("lerp", lerp, 2, 0, CoffUnit::COFF_VAR),
        AlphaAtom("mean_rise", meanRise, 1, 0, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
        AlphaAtom("mean_ratio", meanRatio, 2, 0, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("mid", mid, 2),
        AlphaAtom("up", up, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("down", down, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("power_mid", powerMid, 2),

        AlphaAtom("rank", ranking, 2, 0, CoffUnit::COFF_FORBIT_INDCLASS),
        //AlphaAtom("rank_scale", rankScale, 1),
        //AlphaAtom("rank_sort", rankSort, 1, CoffUnit::COFF_NONE, DateRange::CUR_DAY, 1),

        AlphaAtom("ts_rank", tsRank, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("delay", delay, 1, 0, CoffUnit::COFF_DAY, DateRange::BEFORE_DAY),
        //AlphaAtom("future", future, 1, CoffUnit::COFF_FUTURE_DAY, DateRange::FUTURE_DAY),
        AlphaAtom("delta", delta, 1, 0, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
        AlphaAtom("pre_rise", preRise, 1, 0, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
        AlphaAtom("rise", rise, 1, 0, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
        AlphaAtom("correlation", correlation, 2, 0, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("covariance", covariance, 3, 2, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("scale", scale, 1, 0, CoffUnit::COFF_NONE, DateRange::CUR_DAY),
        AlphaAtom("wma", decayLinear, 1, 0, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("ts_min", tsMin, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("ts_max", tsMax, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("min", min, 2),
        AlphaAtom("min_from", minFrom, 1, 0, CoffUnit::COFF_VAR),
        AlphaAtom("min_to", minTo, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("max", max, 2),
        AlphaAtom("max_from", minFrom, 1, 0, CoffUnit::COFF_VAR),
        AlphaAtom("max_to", minTo, 1, 0, CoffUnit::COFF_CONST),

        AlphaAtom("ts_argmin", tsArgMin, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("ts_argmax", tsArgMax, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("sum", sum, 1, 0, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("product", product, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("stddev", stddev, 2, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("sign", sign, 1),
        AlphaAtom("clamp", clamp, 1),
        AlphaAtom("abs", abs, 1),
        AlphaAtom("log", log, 1),

        AlphaAtom("add", add, 2),
        AlphaAtom("add_from",addFrom, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("add_to",addFrom, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("reduce", reduce, 2),
        AlphaAtom("reduce_from",reduceFrom, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("reduce_to",reduceTo, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("mul", mul, 2),
        AlphaAtom("mul_from",mulFrom, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("mul_to",mulFrom, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("div", div, 2),
        AlphaAtom("div_from",divFrom, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("div_to",divTo, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("and",signAnd,2),
        AlphaAtom("or",signOr,2),
        AlphaAtom("signed_power", signedPower, 2),
        AlphaAtom("signed_power_from",signedPowerFrom, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("signed_power_to",signedPowerTo, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("equal", equalCond, 2),
        AlphaAtom("equal_from",equalCondFrom, 1, 0, CoffUnit::COFF_VAR),
        AlphaAtom("equal_to",equalCondTo, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("less", lessCond, 2),
        AlphaAtom("less_from",lessCondFrom, 1, 0, CoffUnit::COFF_VAR),
        AlphaAtom("less_to",lessCondTo, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("more", moreCond, 2),
        AlphaAtom("more_from",moreCondFrom, 1, 0, CoffUnit::COFF_VAR),
        AlphaAtom("more_to",moreCondTo, 1, 0, CoffUnit::COFF_CONST),

        AlphaAtom("else", elseCond, 2),
        AlphaAtom("else_to", elseCondTo, 1, 0, CoffUnit::COFF_CONST),
        AlphaAtom("if", ifCond, 2),
        AlphaAtom("if_to", ifCondTo, 1, 0, CoffUnit::COFF_CONST),

        AlphaAtom("indneutralize", indneutralize, 1, 0, CoffUnit::COFF_INDCLASS),
        AlphaAtom("lstsq", lstsq, 4, 0, CoffUnit::COFF_DAY),
        AlphaAtom("wait", wait, 2, 1),
//        AlphaAtom("noise_valid", noiseValid, 4, 3, CoffUnit::COFF_CONST),
//        AlphaAtom("alpha_correlation", alphaCorrelation, 2, 0, CoffUnit::COFF_NONE),
//
//        AlphaAtom("amplitude_sample", amplitudeSample, 1, 0, CoffUnit::COFF_CONST),
//        AlphaAtom("random_sign", randomSign, 1, 0, CoffUnit::COFF_CONST),
        //AlphaAtom("up_mean", upMean, 1, CoffUnit::COFF_DAY),

        //todo delete later
//        AlphaAtom("kd", kd, 2, 1, CoffUnit::COFF_NONE, DateRange::CUR_AND_BEFORE_DAY),
//        AlphaAtom("cross", cross, 2, 0, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
//        AlphaAtom("cross_from", crossFrom, 1, 0, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
//        AlphaAtom("cross_to", crossTo, 1, 0, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
//        AlphaAtom("match", match, 2),
//        AlphaAtom("ft_sharp", ftSharp, 3, 2),
//        AlphaAtom("res_eratio", resEratio, 4, 3),
//        AlphaAtom("opt_sharp", optShape, 2),
};

//参数alpha
/*class AlphaPar:public IAlphaElement{
    public:
        AlphaPar(const char* name) : name_(name){
            //this->leafDataType = str2LeafDataType(name);
        }
        //LeafDataType leafDataType;

        virtual const char* getName(){ return name_;}

        virtual const float* cast(const float* pleft, const float* pright, float coff, int historySize, int stockSize, CacheFlag* pflag, bool* pStockFlag, float* pout){
            return pleft;
        }

        static AlphaPar alphaParList[];

    private:
        const char*name_;
};

AlphaPar AlphaPar::alphaParList[] = {
        AlphaPar("open"),
        AlphaPar("high"),
        AlphaPar("low"),
        AlphaPar("close"),
        AlphaPar("volume"),
        AlphaPar("vwap"),
        AlphaPar("returns"),
        AlphaPar("tr"),
};*/

#endif //ALPHATREE_ALPHATREE_H
