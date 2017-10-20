# coding=utf-8
# author=godpgf

from stdb import *
from pyalphatree import AlphaForest, read_alpha_tree_list, read_alpha_tree_dict, write_alpha_tree_list, write_alpha_tree_dict, get_sub_alphatree
import numpy as np
import math
import time


def float_equal(a, b):
    if abs(a - b) >= 0.001:
        print "%.4f %.4f"%(a, b)
    assert abs(a - b) < 0.01


def rank(x):
    if len(x) == 1:
        return np.array([1.0] * len(x[0]))
    #从小到大的索引
    arg_index = x.argsort()
    y = np.zeros(x.shape)

    for i in xrange(len(y)):
        y[arg_index[i]] = len(y) - i - 1

    return y / float(len(y) - 1)


def ts_rank(x, d):
    y = np.zeros(x.shape)
    for i in xrange(1, d + 1):
        for j in xrange(i, len(y)):
            z = (x[j] > x[j - i])
            y[j] += z.astype(int)
    return np.array(y) / float(d)


def pearson_def(x, y, is_print = False):
    assert len(x) == len(y)
    n = len(x)
    assert n > 0
    avg_x = x.mean()
    avg_y = y.mean()
    diffprod = 0
    xdiff2 = 0
    ydiff2 = 0
    for idx in range(n):
        xdiff = x[idx] - avg_x
        ydiff = y[idx] - avg_y
        diffprod += xdiff * ydiff
        xdiff2 += xdiff * xdiff
        ydiff2 += ydiff * ydiff

    if is_print:
        print "%.10f %.10f %.10f %.10f %.10f"%(avg_x, avg_y, xdiff2, ydiff2, diffprod)

    return diffprod / math.sqrt(xdiff2*ydiff2)

def cal_alpha(af, line, sample_size, sub_tree = None, daybefore = 0):
    alphatree_id = af.create_alphatree()
    cache_id = af.use_cache()
    if sub_tree:
        for key, value in sub_tree.items():
            af.decode_alphatree(alphatree_id, key, value)
    af.decode_alphatree(alphatree_id, "alpha", line)
    history_days = af.get_max_history_days(alphatree_id)
    codes = af.get_codes(daybefore, history_days, sample_size)
    af.flag_alpha(alphatree_id, cache_id, daybefore, sample_size, codes)
    af.cal_alpha(alphatree_id, cache_id)
    alpha = af.get_root_alpha(alphatree_id, "alpha", cache_id, sample_size)
    af.release_cache(cache_id)
    af.release_alphatree(alphatree_id)
    return alpha, codes

def test_alphaforest(af, alphatree_list, subalphatree_dict, sample_size = 1):
    alphatree_id_list = []
    for id, at in enumerate(alphatree_list):
        print at
        sub_tree = get_sub_alphatree(at, subalphatree_dict)

        alphatree_id = af.create_alphatree()
        print "aid %d" % alphatree_id
        cache_id = af.use_cache()
        if sub_tree:
            for key, value in sub_tree.items():
                af.decode_alphatree(alphatree_id, key, value)
        af.decode_alphatree(alphatree_id, "alpha", at)
        encodestr = af.encode_alphatree(alphatree_id, "alpha")
        history_days = af.get_max_history_days(alphatree_id)
        print "days %d" % history_days
        codes = af.get_codes(0, history_days, sample_size)
        af.flag_alpha(alphatree_id, cache_id, 0, sample_size, codes)
        print "finish flag"
        af.cal_alpha(alphatree_id, cache_id)
        print "finish cal req"
        alpha = af.get_root_alpha(alphatree_id, "alpha", cache_id, sample_size)
        print "finish cal"

        af.release_cache(cache_id)
        #af.release_alphatree(alphatree_id)

        alphatree_id_list.append(alphatree_id)


        if at != encodestr:
            print encodestr
        assert at == encodestr
        print ">>>>>>>>>>>>>>>>>>>>>>>>>> mean %.4f" % np.array(alpha[-1]).reshape(-1).mean()


    return alphatree_id_list

def test_base_calculate(af, dataProxy):
    print "start test base calculate .................."

    print "returns"
    alpha, codes = cal_alpha(af, "returns", 2)
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][7],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][7],alpha[-2][index])
        
    print "sum(high, 2)"
    alpha, codes = cal_alpha(af, "sum(high, 2)", 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][2]+dataProxy.get_all_Data(code)[-2][2]+dataProxy.get_all_Data(code)[-3][2],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][2]+dataProxy.get_all_Data(code)[-3][2]+dataProxy.get_all_Data(code)[-4][2],alpha[-2][index])

    print "product(low, 1)"
    alpha, codes = cal_alpha(af, "product(low, 1)", 2)
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][3]*dataProxy.get_all_Data(code)[-2][3]/100,alpha[-1][index]/100)
        float_equal(dataProxy.get_all_Data(code)[-2][3]*dataProxy.get_all_Data(code)[-3][3]/100,alpha[-2][index]/100)

    print "mean(close, 1)"
    alpha, codes = cal_alpha(af, "mean(close, 1)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][4]+dataProxy.get_all_Data(code)[-2][4])/2,alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][4]+dataProxy.get_all_Data(code)[-3][4])/2,alpha[-2][index])

    print "lerp(high, low, 0.4)"
    alpha, codes = cal_alpha(af, "lerp(high, low, 0.4)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]*0.4+dataProxy.get_all_Data(code)[-1][3]*0.6),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]*0.4+dataProxy.get_all_Data(code)[-2][3]*0.6),alpha[-2][index])

    print "delta(open, 5)"
    alpha, codes = cal_alpha(af, "delta(open, 5)", 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(dataProxy.get_all_Data(code)[-1][1] - dataProxy.get_all_Data(code)[-6][1],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][1] - dataProxy.get_all_Data(code)[-7][1],alpha[-2][index])

    print "mean_rise(open, 5)"
    alpha, codes = cal_alpha(af, "mean_rise(open, 5)", 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal((dataProxy.get_all_Data(code)[-1][1] - dataProxy.get_all_Data(code)[-6][1])/5,alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][1] - dataProxy.get_all_Data(code)[-7][1])/5,alpha[-2][index])

    print "(low / high)"
    alpha, codes = cal_alpha(af, "(low / high)", 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(dataProxy.get_all_Data(code)[-1][3]/dataProxy.get_all_Data(code)[-1][2],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][3]/dataProxy.get_all_Data(code)[-2][2],alpha[-2][index])

    print "div_from:(100 / low)"
    alpha, codes = cal_alpha(af, "(100 / low)", 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(100 / dataProxy.get_all_Data(code)[-1][3],alpha[-1][index])
        float_equal(100 / dataProxy.get_all_Data(code)[-2][3],alpha[-2][index])

    print "div_to:(low / 2)"
    alpha, codes = cal_alpha(af, "(low / 2)", 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(dataProxy.get_all_Data(code)[-1][3],alpha[-1][index] * 2)
        float_equal(dataProxy.get_all_Data(code)[-2][3],alpha[-2][index] * 2)

    print "mean_ratio(close, 1)"
    alpha, codes = cal_alpha(af, "mean_ratio(close, 1)", 2)
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][4] / (dataProxy.get_all_Data(code)[-1][4]+dataProxy.get_all_Data(code)[-2][4])*2,alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][4] / (dataProxy.get_all_Data(code)[-2][4]+dataProxy.get_all_Data(code)[-3][4])*2,alpha[-2][index])

    print "(high + low)"
    alpha, codes = cal_alpha(af, "(high + low)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]+dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]+dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "add_from:(1 + low)"
    alpha, codes = cal_alpha(af, "(1 + low)", 2)
    for index,code in enumerate(codes):
        float_equal((1+dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((1+dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "add_to:(high + 1)"
    alpha, codes = cal_alpha(af, "(high + 1)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]+1),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]+1),alpha[-2][index])

    print "reduce:(high - low)"
    alpha, codes = cal_alpha(af, "(high - low)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]-dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]-dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "reduce_from:(1000 - low)"
    alpha, codes = cal_alpha(af, "(1000 - low)", 2)
    for index,code in enumerate(codes):
        float_equal((1000-dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((1000-dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "reduce_to:(high - 1)"
    alpha, codes = cal_alpha(af, "(high - 1)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]-1),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]-1),alpha[-2][index])

    print "(high * low)"
    alpha, codes = cal_alpha(af, "(high * low)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]*dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]*dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "mul_from:(2 * low)"
    alpha, codes = cal_alpha(af, "(2 * low)", 2)
    for index,code in enumerate(codes):
        float_equal((2*dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((2*dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "mul_to:(high * 3)"
    alpha, codes = cal_alpha(af, "(high * 3)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]*3),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]*3),alpha[-2][index])

    print "mid(high, low)"
    alpha, codes = cal_alpha(af, "mid(high, low)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]+dataProxy.get_all_Data(code)[-1][3])*0.5,alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]+dataProxy.get_all_Data(code)[-2][3])*0.5,alpha[-2][index])

    print "stddev(open, 2)"
    alpha, codes = cal_alpha(af, "stddev(open, 2)", 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        res = np.array([dataProxy.get_all_Data(code)[-3][1],dataProxy.get_all_Data(code)[-2][1],dataProxy.get_all_Data(code)[-1][1]])
        float_equal(res.std(),alpha[-1][index])
        res = np.array([dataProxy.get_all_Data(code)[-4][1], dataProxy.get_all_Data(code)[-3][1],
                        dataProxy.get_all_Data(code)[-2][1]])
        float_equal(res.std(), alpha[-2][index])

    print "up(close, 2)"
    alpha, codes = cal_alpha(af, "up(close, 2)", 1)
    for index,code in enumerate(codes):
        res = np.array([dataProxy.get_all_Data(code)[-3][4],dataProxy.get_all_Data(code)[-2][4],dataProxy.get_all_Data(code)[-1][4]])
        float_equal(res.std() + res.mean(),alpha[-1][index])

    print "down(returns, 2)"
    alpha, codes = cal_alpha(af, "down(returns, 2)", 1)
    for index,code in enumerate(codes):
        res = np.array([dataProxy.get_all_Data(code)[-3][7],dataProxy.get_all_Data(code)[-2][7],dataProxy.get_all_Data(code)[-1][7]])
        float_equal(res.mean() - res.std(),alpha[-1][index])

    print "rank(open)"
    alpha, codes = cal_alpha(af, "rank(open)", 1)
    v_open = np.array([dataProxy.get_all_Data(code)[-1][1] for code in codes])
    r_open = rank(v_open)
    for i in xrange(len(v_open)):
        float_equal(r_open[i], alpha[-1][i])

    print "power_mid(delay(open, 25), high)"
    alpha, codes = cal_alpha(af, "power_mid(delay(open, 25), high)", 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        #math.sqrt(5)
        value = dataProxy.get_all_Data(code)[-26][1]*dataProxy.get_all_Data(code)[-1][2]
        #print value
        float_equal(math.sqrt(value),alpha[-1][index])
        value = dataProxy.get_all_Data(code)[-27][1]*dataProxy.get_all_Data(code)[-2][2]
        #print value
        float_equal(math.sqrt(value),alpha[-2][index])

    print "signed_power_to:(returns ^ 2)"
    alpha, codes = cal_alpha(af, "(returns ^ 2)", 1)
    for index, code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][7] * dataProxy.get_all_Data(code)[-1][7]),
                    alpha[-1][index])

    print "ts_rank(returns, 5)"
    alpha, codes = cal_alpha(af, "ts_rank(returns, 5)", 2)
    for index, code in enumerate(codes):
        returns = np.array([dataProxy.get_all_Data(code)[i-6][7] for i in xrange(6)])
        float_equal(ts_rank(returns, 5)[-1], alpha[-1][index])
        returns = np.array([dataProxy.get_all_Data(code)[i - 6-1][7] for i in xrange(6)])
        float_equal(ts_rank(returns, 5)[-1], alpha[-2][index])

    print "delay(open, 5)"
    alpha, codes = cal_alpha(af, "delay(open, 5)", 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(dataProxy.get_all_Data(code)[-6][1],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-7][1],alpha[-2][index])

    print "correlation(delay(returns, 3), returns, 10)"
    alpha, codes = cal_alpha(af, "correlation(delay(returns, 3), returns, 10)", 2)
    for index, code in enumerate(codes):
        returns = np.array([dataProxy.get_all_Data(code)[i - 11][7] for i in xrange(11)])
        d_returns = np.array([dataProxy.get_all_Data(code)[i - 14][7] for i in xrange(11)])
        v1 = pearson_def(d_returns, returns)
        v2 = alpha[-1][index]
        float_equal(v1*0.1, v2*0.1)

        returns = np.array([dataProxy.get_all_Data(code)[i - 12][7] for i in xrange(11)])
        d_returns = np.array([dataProxy.get_all_Data(code)[i - 15][7] for i in xrange(11)])
        v1 = pearson_def(d_returns, returns)
        v2 = alpha[-2][index]
        float_equal(v1*0.1, v2*0.1)


    print "scale(delay(returns, 1))"
    alpha, codes = cal_alpha(af, "scale(delay(returns, 1))", 2)
    returns = np.array([dataProxy.get_all_Data(code)[-2][7] for code in codes])
    abs_returns = np.array([abs(r) for r in returns])
    returns /= abs_returns.sum()
    for i in xrange(len(returns)):
        float_equal(returns[i], alpha[-1][i])

    returns = np.array([dataProxy.get_all_Data(code)[-3][7] for code in codes])
    abs_returns = np.array([abs(r) for r in returns])
    returns /= abs_returns.sum()
    for i in xrange(len(returns)):
        float_equal(returns[i], alpha[-2][i])

    print "decay_linear(high, 2)"
    alpha, codes = cal_alpha(af, "decay_linear(high, 2)", 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][2]*3+dataProxy.get_all_Data(code)[-2][2]*2+dataProxy.get_all_Data(code)[-3][2],alpha[-1][index] * 6)
        float_equal(dataProxy.get_all_Data(code)[-2][2]*3+dataProxy.get_all_Data(code)[-3][2]*2+dataProxy.get_all_Data(code)[-4][2],alpha[-2][index] * 6)

    print "ts_min(high, 2)"
    alpha, codes = cal_alpha(af, "ts_min(high, 2)", 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(min([dataProxy.get_all_Data(code)[-1][2],dataProxy.get_all_Data(code)[-2][2],dataProxy.get_all_Data(code)[-3][2]]),alpha[-1][index])
        float_equal(min([dataProxy.get_all_Data(code)[-2][2],dataProxy.get_all_Data(code)[-3][2],dataProxy.get_all_Data(code)[-4][2]]),alpha[-2][index])

    print "ts_max(high, 2)"
    alpha, codes = cal_alpha(af, "ts_max(high, 2)", 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(max([dataProxy.get_all_Data(code)[-1][2],dataProxy.get_all_Data(code)[-2][2],dataProxy.get_all_Data(code)[-3][2]]),alpha[-1][index])
        float_equal(max([dataProxy.get_all_Data(code)[-2][2],dataProxy.get_all_Data(code)[-3][2],dataProxy.get_all_Data(code)[-4][2]]),alpha[-2][index])


    print "min(delay(before_high, 1), before_high)"
    sub_dict = {"before_high":"delay(high, 2)"}
    alpha, codes = cal_alpha(af, "min(delay(before_high, 1), before_high)", 2, sub_dict)
    # 今天、昨天、前天数据一共3天
    for index, code in enumerate(codes):
        float_equal(min([dataProxy.get_all_Data(code)[-4][2], dataProxy.get_all_Data(code)[-3][2]]), alpha[-1][index])
        float_equal(min([dataProxy.get_all_Data(code)[-5][2], dataProxy.get_all_Data(code)[-4][2]]), alpha[-2][index])

    print "max(high, before_high)"
    sub_dict = {"before_high": "delay(high, 50)"}
    alpha, codes = cal_alpha(af, "max(high, before_high)", 2,sub_dict)
    # 今天、昨天、前天数据一共3天
    for index, code in enumerate(codes):
        float_equal(max([dataProxy.get_all_Data(code)[-1][2], dataProxy.get_all_Data(code)[-51][2]]), alpha[-1][index])
        float_equal(max([dataProxy.get_all_Data(code)[-2][2], dataProxy.get_all_Data(code)[-52][2]]), alpha[-2][index])

    print "ts_argmin(volume, 2)"
    alpha, codes = cal_alpha(af, "ts_argmin(volume, 2)", 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(np.argmin(np.array([dataProxy.get_all_Data(code)[-3][5],dataProxy.get_all_Data(code)[-2][5],dataProxy.get_all_Data(code)[-1][5]])),alpha[-1][index])
        float_equal(np.argmin(np.array([dataProxy.get_all_Data(code)[-4][5],dataProxy.get_all_Data(code)[-3][5],dataProxy.get_all_Data(code)[-2][5]])),alpha[-2][index])

    print "ts_argmax(volume, 2)"
    alpha, codes = cal_alpha(af, "ts_argmax(volume, 2)", 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(np.argmax(np.array([dataProxy.get_all_Data(code)[-3][5],dataProxy.get_all_Data(code)[-2][5],dataProxy.get_all_Data(code)[-1][5]])),alpha[-1][index])
        float_equal(np.argmax(np.array([dataProxy.get_all_Data(code)[-4][5],dataProxy.get_all_Data(code)[-3][5],dataProxy.get_all_Data(code)[-2][5]])),alpha[-2][index])

    print "sign(returns)"
    alpha, codes = cal_alpha(af, "sign(returns)", 2)
    for index,code in enumerate(codes):
        v = dataProxy.get_all_Data(code)[-1][7]
        float_equal(1 if v > 0 else (-1 if v < 0 else 0),alpha[-1][index])
        v = dataProxy.get_all_Data(code)[-2][7]
        float_equal(1 if v > 0 else (-1 if v < 0 else 0),alpha[-2][index])

    print "abs(returns)"
    alpha, codes = cal_alpha(af, "abs(returns)", 2)
    for index,code in enumerate(codes):
        float_equal(abs(dataProxy.get_all_Data(code)[-1][7]),alpha[-1][index])
        float_equal(abs(dataProxy.get_all_Data(code)[-2][7]),alpha[-2][index])

    print "(open ^ returns)"
    alpha, codes = cal_alpha(af, "(open ^ returns)", 2)
    for index,code in enumerate(codes):
        float_equal(math.pow(dataProxy.get_all_Data(code)[-1][1], dataProxy.get_all_Data(code)[-1][7]),alpha[-1][index])
        float_equal(math.pow(dataProxy.get_all_Data(code)[-2][1], dataProxy.get_all_Data(code)[-2][7]),alpha[-2][index])

    print "less:(delay(returns, 1) < returns)"
    alpha, codes = cal_alpha(af, "(delay(returns, 1) < returns)", 2)
    for index,code in enumerate(codes):
        float_equal((1 if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else 0),alpha[-1][index])
        float_equal((1 if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else 0),alpha[-2][index])

    print "less_to:(returns < 0)"
    alpha, codes = cal_alpha(af, "(returns < 0)", 2)
    for index,code in enumerate(codes):
        float_equal((1 if dataProxy.get_all_Data(code)[-1][7] < 0 else 0),alpha[-1][index])
        float_equal((1 if dataProxy.get_all_Data(code)[-2][7] < 0 else 0),alpha[-2][index])

    print "less_from:(0 < returns)"
    alpha, codes = cal_alpha(af, "(0 < returns)", 2)
    for index,code in enumerate(codes):
        float_equal((1 if 0 < dataProxy.get_all_Data(code)[-1][7] else 0),alpha[-1][index])
        float_equal((1 if 0 < dataProxy.get_all_Data(code)[-2][7] else 0),alpha[-2][index])

    print "((delay(returns, 1) < returns) ? open : close)"
    alpha, codes = cal_alpha(af, "((delay(returns, 1) < returns) ? open : close)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][1] if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else dataProxy.get_all_Data(code)[-1][4]),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][1] if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else dataProxy.get_all_Data(code)[-2][4]),alpha[-2][index])

    print "if_to:((delay(returns, 1) < returns) ? 1 : close)"
    alpha, codes = cal_alpha(af, "((delay(returns, 1) < returns) ? 1 : close)", 2)
    for index,code in enumerate(codes):
        float_equal((1 if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else dataProxy.get_all_Data(code)[-1][4]),alpha[-1][index])
        float_equal((1 if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else dataProxy.get_all_Data(code)[-2][4]),alpha[-2][index])

    print "else_to:((delay(returns, 1) < returns) ? open : 1)"
    alpha, codes = cal_alpha(af, "((delay(returns, 1) < returns) ? open : 1)", 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][1] if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else 1),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][1] if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else 1),alpha[-2][index])

    print "if_to_else_to:((delay(returns, 1) < returns) ? 1 : -1)"
    alpha, codes = cal_alpha(af, "((delay(returns, 1) < returns) ? 1 : -1)", 2)
    for index,code in enumerate(codes):
        float_equal((1 if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else -1),alpha[-1][index])
        float_equal((1 if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else -1),alpha[-2][index])

    # print "up_mean"
    # alpha, codes = cal_alpha(af, "up_mean(volume, 1)", 2)
    # for index,code in enumerate(codes):
    #     float_equal(1 if dataProxy.get_all_Data(code)[-1][5] >= (dataProxy.get_all_Data(code)[-1][5]+dataProxy.get_all_Data(code)[-2][5])/2 else 0,alpha[-1][index])
    #     float_equal(1 if dataProxy.get_all_Data(code)[-2][5] >= (dataProxy.get_all_Data(code)[-2][5]+dataProxy.get_all_Data(code)[-3][5])/2 else 0,alpha[-2][index])

    print "indneutralize:(returns - indneutralize(returns, IndClass.market))"
    alpha, codes = cal_alpha(af, "(returns - indneutralize(returns, IndClass.market))", 2)
    for index,code in enumerate(codes):
        if code[0] == '0':
            market = '0000001'
        else:
            market = '1399001'
        float_equal(dataProxy.get_all_Data(code)[-1][7] - dataProxy.get_all_Data(market)[-1][7],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][7] - dataProxy.get_all_Data(market)[-2][7],alpha[-2][index])

    print "finish test base calculate ................."

def test_alpha101(af):
    print "test alpha101........................."
    alphatree_list = read_alpha_tree_list("../doc/alpha.txt")
    subalphatree_dict = read_alpha_tree_dict("../doc/subalpha.txt")
    start = time.time()

    alphatree_id_list = test_alphaforest(af, alphatree_list, subalphatree_dict)


    end = time.time()

    print "use time:"
    print end - start

    sub_alphatree = af.summary_sub_alphatree(alphatree_id_list)
    print "---------------------------------"
    for s in sub_alphatree:
        print s

    for aid in alphatree_id_list:
        af.release_alphatree(aid)
    return
    print "start test --------"


if __name__ == '__main__':
    af = AlphaForest()
    af.load_data(is_offline=True)
    dataProxy = LocalDataProxy("data", True)

    test_base_calculate(af, dataProxy)
    test_alpha101(af)
