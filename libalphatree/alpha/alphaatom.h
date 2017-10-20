//
// Created by yanyu on 2017/7/12.
//

#ifndef ALPHATREE_ALPHAATOM_H
#define ALPHATREE_ALPHAATOM_H

#include "alphadb.h"
#include "alpha.h"


//系数单位
enum class CoffUnit{
    COFF_NONE = 0,
    COFF_DAY = 1,
    COFF_CONST = 2,
    COFF_INDCLASS = 3,
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
        virtual int getChildNum(){ return 0;}
        //传入左右孩子的参数,系数,输出内存块   得到运算结果
        virtual const float* cast(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, float* pout) = 0;
        //得到系数类型
        virtual CoffUnit getCoffUnit() { return CoffUnit::COFF_NONE;}
        //得到计算天数
        virtual DateRange getDateRange(){ return DateRange::CUR_DAY; }
        //是否将缺失数据补0去计算
        virtual bool isCalculateLossDataAsZero(){ return true;}
};

//原子alpha
class AlphaAtom: public IAlphaElement{
    public:
        AlphaAtom(
                const char* name,
                const float* (*opt) (const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, float* pout),
                int childNum = 0,
                CoffUnit coffUnit = CoffUnit::COFF_NONE,
                DateRange dateRange = DateRange::CUR_DAY,
                bool isCalLossDataAsZero = true
        ):name_(name), opt_(opt), childNum_(childNum), coffUnit_(coffUnit), dateRange_(dateRange), isCalLossDataAsZero_(isCalLossDataAsZero){
        }

        virtual const char* getName(){ return name_;}

        virtual int getChildNum(){ return childNum_;}

        virtual const float* cast(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, float* pout){
            return opt_(pleft, pright, coff, historySize, stockSize, pflag, pout);
        }

        virtual CoffUnit getCoffUnit() { return coffUnit_;}

        virtual DateRange getDateRange(){ return dateRange_; }

        virtual bool isCalculateLossDataAsZero(){ return isCalLossDataAsZero_;}

        static AlphaAtom alphaAtomList[];

    private:

        const char*name_;

        const float* (*opt_) (const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, float* pout);
        int childNum_;
        //参数单位
        CoffUnit coffUnit_;
        //计算天数
        DateRange dateRange_;
        //是否将缺失数据补0去计算
        bool isCalLossDataAsZero_;
};

AlphaAtom AlphaAtom::alphaAtomList[] = {
        //AlphaAtom("none", none, 1),
        AlphaAtom("mean", mean, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("lerp", lerp, 2, CoffUnit::COFF_CONST),
        AlphaAtom("mean_rise", meanRise, 1, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
        AlphaAtom("mean_ratio", meanRatio, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("mid", mid, 2),
        AlphaAtom("up", up, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("down", down, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("power_mid", powerMid, 2),

        //AlphaAtom("rank", ranking, 1),
        AlphaAtom("rank_scale", rankScale, 1),
        AlphaAtom("rank_sort", rankSort, 1, CoffUnit::COFF_NONE, DateRange::CUR_DAY, false),

        AlphaAtom("ts_rank", tsRank, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("delay", delay, 1, CoffUnit::COFF_DAY, DateRange::BEFORE_DAY),
        //AlphaAtom("future", future, 1, CoffUnit::COFF_FUTURE_DAY, DateRange::FUTURE_DAY),
        AlphaAtom("delta", delta, 1, CoffUnit::COFF_DAY, DateRange::CUR_AND_BEFORE_DAY),
        AlphaAtom("correlation", correlation, 2, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("scale", scale, 1, CoffUnit::COFF_NONE, DateRange::CUR_DAY, false),
        AlphaAtom("decay_linear", decayLinear, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("ts_min", tsMin, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("ts_max", tsMax, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("min", min, 2),
        AlphaAtom("max", max, 2),
        AlphaAtom("ts_argmin", tsArgMin, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("ts_argmax", tsArgMax, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("sum", sum, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("product", product, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("stddev", stddev, 1, CoffUnit::COFF_DAY, DateRange::ALL_DAY),
        AlphaAtom("sign", sign, 1),
        AlphaAtom("abs", abs, 1),
        AlphaAtom("log", log, 1),

        AlphaAtom("add", add, 2),
        AlphaAtom("add_from",addFrom, 1, CoffUnit::COFF_CONST),
        AlphaAtom("add_to",addFrom, 1, CoffUnit::COFF_CONST),
        AlphaAtom("reduce", reduce, 2),
        AlphaAtom("reduce_from",reduceFrom, 1, CoffUnit::COFF_CONST),
        AlphaAtom("reduce_to",reduceTo, 1, CoffUnit::COFF_CONST),
        AlphaAtom("mul", mul, 2),
        AlphaAtom("mul_from",mulFrom, 1, CoffUnit::COFF_CONST),
        AlphaAtom("mul_to",mulFrom, 1, CoffUnit::COFF_CONST),
        AlphaAtom("div", div, 2),
        AlphaAtom("div_from",divFrom, 1, CoffUnit::COFF_CONST),
        AlphaAtom("div_to",divTo, 1, CoffUnit::COFF_CONST),
        AlphaAtom("signed_power", signedPower, 2),
        AlphaAtom("signed_power_from",signedPowerFrom, 1, CoffUnit::COFF_CONST),
        AlphaAtom("signed_power_to",signedPowerTo, 1, CoffUnit::COFF_CONST),
        AlphaAtom("less", lessCond, 2),
        AlphaAtom("less_from",lessCondFrom, 1, CoffUnit::COFF_CONST),
        AlphaAtom("less_to",lessCondTo, 1, CoffUnit::COFF_CONST),

        AlphaAtom("else", elseCond, 2),
        AlphaAtom("else_to", elseCondTo, 1, CoffUnit::COFF_CONST),
        AlphaAtom("if", ifCond, 2),
        AlphaAtom("if_to", ifCondTo, 1, CoffUnit::COFF_CONST),

        AlphaAtom("indneutralize", indneutralize, 1,CoffUnit::COFF_INDCLASS),
        //AlphaAtom("up_mean", upMean, 1, CoffUnit::COFF_DAY),
};

//参数alpha
class AlphaPar:public IAlphaElement{
    public:
        AlphaPar(const char* name) : name_(name){
            this->leafDataType = str2LeafDataType(name);
        }
        LeafDataType leafDataType;

        virtual const char* getName(){ return name_;}

        virtual const float* cast(const float* pleft, const float* pright, float coff, size_t historySize, size_t stockSize, CacheFlag* pflag, float* pout){
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
        AlphaPar("returns")
};

#endif //ALPHATREE_ALPHATREE_H
