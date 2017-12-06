//
// Created by yanyu on 2017/10/1.
// 给数据分组以便验证,比如直接按取值区间分组,或按短周期上交长周期或标准差分组
//

#ifndef ALPHATREE_ALPHAGROUP_H
#define ALPHATREE_ALPHAGROUP_H

//分组中参数的最大个数
const int MAX_GROUP_PARS_NUM = 4;

enum class AlphaGroupType{
    VALUE_GROUP = 0,
    MOVE_AVG_TREND_GROUP = 1,
    BOLL_TREND_GROUP = 2,
    MACD_TREND_GROUP = 3,
    KDJ_TREND_GROUP = 4,
};

struct AlphaGroup{
    AlphaGroupType groupType;
    float groupPars[MAX_GROUP_PARS_NUM];
    float MAE;
    float MFE;
    float marketMAE;
    float marketMFE;
    //分组中的数据量
    int count;
    //未来天数
    int futureDays;
    bool flag;
};

#endif //ALPHATREE_ALPHAGROUP_H
