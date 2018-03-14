# coding=utf-8
# author=godpgf

from stdb import *
from pyalphatree import *
import numpy as np
import math
import time
import os
import json
import math


def rank(x):
    if len(x) == 1:
        return np.array([1.0] * len(x[0]))
    # 从小到大的索引
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

def cov_def(x, y):
    return (x * y).mean() - x.mean() * y.mean()

def pearson_def(x, y, is_print=False):
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
        print "%.10f %.10f %.10f %.10f %.10f" % (avg_x, avg_y, xdiff2, ydiff2, diffprod)

    return 1 if xdiff2 < 0.000000001 or ydiff2 < 0.000000001 else diffprod / math.sqrt(xdiff2) / math.sqrt(ydiff2)


# 将csv数据转化成程序可以识别的格式
def csv_2_binary(af):
    af.csv2binary("../data", "date")
    af.csv2binary("../data", "open")
    af.csv2binary("../data", "high")
    af.csv2binary("../data", "low")
    af.csv2binary("../data", "close")
    af.csv2binary("../data", "volume")
    af.csv2binary("../data", "vwap")
    af.csv2binary("../data", "returns")
    af.csv2binary("../data", "amount")
    af.csv2binary("../data", "turn")
    af.csv2binary("../data", "tcap")
    af.csv2binary("../data", "mcap")
    print "finish convert"


# 测试基本计算的正确性
def test_base_cal(data_proxy, codes):
    float_equal = lambda x, y: math.fabs(x - y) < 0.01

    print "sum(high, -2)"
    alpha = AlphaArray(codes, ["s = sum(high, -2)"], "s", 2, 2)
    # 今天、昨天、前天数据一共3天
    data = alpha[:]
    for id, code in enumerate(codes):
        print code
        assert float_equal(data_proxy.get_all_Data(code)[-1][2] + data_proxy.get_all_Data(code)[-2][2] +
                           data_proxy.get_all_Data(code)[-3][2], data[-1][id])
        assert float_equal(data_proxy.get_all_Data(code)[-2][2] + data_proxy.get_all_Data(code)[-3][2] +
                           data_proxy.get_all_Data(code)[-4][2], data[-2][id])

    print "test target"
    alpha = AlphaArray(codes[0], ["close_ = delay(close, 1)", "returns_ = returns"], ["close_", "returns_"], 9, 22)
    # 过去10天的数据再往前取20天
    data = alpha[1:-1]
    assert len(data) == 20
    for i in xrange(20):
        assert float_equal(data[i][0], data_proxy.get_all_Data(codes[0])[-31 + i][4])
        assert float_equal(data[i][1], data_proxy.get_all_Data(codes[0])[-30 + i][7])

    print "test delay(returns, -2)"
    alpha = AlphaArray(codes, ["r=delay(returns, -2)"], "r", 2, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal(data[0][id], data_proxy.get_all_Data(code)[-2][7])
        assert float_equal(data[1][id], data_proxy.get_all_Data(code)[-1][7])

    print "sum(high, -2)"
    alpha = AlphaArray(codes, ["s = sum(high, -2)"], "s", 2, 2)
    # 今天、昨天、前天数据一共3天
    data = alpha[:]
    for id, code in enumerate(codes):
        print code
        print data[-1][id], data[-2][id],data_proxy.get_all_Data(code)[-1][2] + data_proxy.get_all_Data(code)[-2][2] + data_proxy.get_all_Data(code)[-3][2],data_proxy.get_all_Data(code)[-2][2] + data_proxy.get_all_Data(code)[-3][2] + data_proxy.get_all_Data(code)[-4][2]
        assert float_equal(data_proxy.get_all_Data(code)[-1][2] + data_proxy.get_all_Data(code)[-2][2] +
                           data_proxy.get_all_Data(code)[-3][2], data[-1][id])
        assert float_equal(data_proxy.get_all_Data(code)[-2][2] + data_proxy.get_all_Data(code)[-3][2] +
                           data_proxy.get_all_Data(code)[-4][2], data[-2][id])

    print "product(low, 1)"
    alpha = AlphaArray(codes, ["s = product(low, 1)"], "s", 0, 4)
    data = alpha[2:]
    for id, code in enumerate(codes):
        assert float_equal(data_proxy.get_all_Data(code)[-1][3] * data_proxy.get_all_Data(code)[-2][3] / 100,
                           data[-1][id] / 100)
        assert float_equal(data_proxy.get_all_Data(code)[-2][3] * data_proxy.get_all_Data(code)[-3][3] / 100,
                           data[-2][id] / 100)

    print "mean(close, 1)"
    alpha = AlphaArray(codes, ["s = mean(close, 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][4] + data_proxy.get_all_Data(code)[-2][4]) / 2,
                           data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][4] + data_proxy.get_all_Data(code)[-3][4]) / 2,
                           data[-2][id])

    print "lerp(high, low, 0.4)"
    alpha = AlphaArray(codes, ["s = lerp(high, low, 0.4)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][2] * 0.4 + data_proxy.get_all_Data(code)[-1][3] * 0.6),
                           data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][2] * 0.4 + data_proxy.get_all_Data(code)[-2][3] * 0.6),
                           data[-2][id])

    print "delta(open, 5)"
    alpha = AlphaArray(codes, ["s = delta(open, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(data_proxy.get_all_Data(code)[-1][1] - data_proxy.get_all_Data(code)[-6][1], data[-1][id])
        assert float_equal(data_proxy.get_all_Data(code)[-2][1] - data_proxy.get_all_Data(code)[-7][1], data[-2][id])

    print "pre_rise(open, 5)"
    alpha = AlphaArray(codes, ["s = pre_rise(open, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal((data_proxy.get_all_Data(code)[-1][1] - data_proxy.get_all_Data(code)[-6][1]) / data_proxy.get_all_Data(code)[-1][1], data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][1] - data_proxy.get_all_Data(code)[-7][1]) / data_proxy.get_all_Data(code)[-2][1], data[-2][id])

    print "rise(open, -5)"
    alpha = AlphaArray(codes, ["s = rise(open, -5)"], "s", 5, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal((data_proxy.get_all_Data(code)[-6][1] - data_proxy.get_all_Data(code)[-1][1]) / data_proxy.get_all_Data(code)[-1][1], data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-7][1] - data_proxy.get_all_Data(code)[-2][1]) / data_proxy.get_all_Data(code)[-2][1], data[-2][id])

    print "mean_rise(open, 5)"
    alpha = AlphaArray(codes, ["s = mean_rise(open, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        float_equal((data_proxy.get_all_Data(code)[-1][1] - data_proxy.get_all_Data(code)[-6][1]) / 5, data[-1][id])
        float_equal((data_proxy.get_all_Data(code)[-2][1] - data_proxy.get_all_Data(code)[-7][1]) / 5, data[-2][id])

    print "(low / high)"
    alpha = AlphaArray(codes, ["s = (low / high)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(data_proxy.get_all_Data(code)[-1][3] / data_proxy.get_all_Data(code)[-1][2], data[-1][id])
        assert float_equal(data_proxy.get_all_Data(code)[-2][3] / data_proxy.get_all_Data(code)[-2][2], data[-2][id])

    print "div_from:(100 / low)"
    alpha = AlphaArray(codes, ["s = (100 / low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(100 / data_proxy.get_all_Data(code)[-1][3], data[-1][id])
        assert float_equal(100 / data_proxy.get_all_Data(code)[-2][3], data[-2][id])

    print "div_to:(low / 2)"
    alpha = AlphaArray(codes, ["s = (low / 2)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(data_proxy.get_all_Data(code)[-1][3], data[-1][id] * 2)
        assert float_equal(data_proxy.get_all_Data(code)[-2][3], data[-2][id] * 2)

    print "mean_ratio(close, 1)"
    alpha = AlphaArray(codes, ["s = mean_ratio(close, 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal(data_proxy.get_all_Data(code)[-1][4] / (
                data_proxy.get_all_Data(code)[-1][4] + data_proxy.get_all_Data(code)[-2][4]) * 2, data[-1][id])
        assert float_equal(data_proxy.get_all_Data(code)[-2][4] / (
                data_proxy.get_all_Data(code)[-2][4] + data_proxy.get_all_Data(code)[-3][4]) * 2, data[-2][id])

    print "(high + low)"
    alpha = AlphaArray(codes, ["s = (high + low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][2] + data_proxy.get_all_Data(code)[-1][3]), data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][2] + data_proxy.get_all_Data(code)[-2][3]), data[-2][id])

    print "add_from:(1 + low)"
    alpha = AlphaArray(codes, ["s = (1 + low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 + data_proxy.get_all_Data(code)[-1][3]), data[-1][id])
        assert float_equal((1 + data_proxy.get_all_Data(code)[-2][3]), data[-2][id])

    print "add_to:(high + 1)"
    alpha = AlphaArray(codes, ["s = (high + 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][2] + 1), data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][2] + 1), data[-2][id])

    print "reduce:(high - low)"
    alpha = AlphaArray(codes, ["s = (high - low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][2] - data_proxy.get_all_Data(code)[-1][3]), data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][2] - data_proxy.get_all_Data(code)[-2][3]), data[-2][id])

    print "reduce_from:(1000 - low)"
    alpha = AlphaArray(codes, ["s = (1000 - low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1000 - data_proxy.get_all_Data(code)[-1][3]), data[-1][id])
        assert float_equal((1000 - data_proxy.get_all_Data(code)[-2][3]), data[-2][id])

    print "reduce_to:(high - 1)"
    alpha = AlphaArray(codes, ["s = (high - 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][2] - 1), data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][2] - 1), data[-2][id])

    print "(high * low)"
    alpha = AlphaArray(codes, ["s = (high * low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][2] * data_proxy.get_all_Data(code)[-1][3]) / 10,
                           data[-1][id] / 10)
        assert float_equal((data_proxy.get_all_Data(code)[-2][2] * data_proxy.get_all_Data(code)[-2][3]) / 10,
                           data[-2][id] / 10)

    print "mul_from:(2 * low)"
    alpha = AlphaArray(codes, ["s = (2 * low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((2 * data_proxy.get_all_Data(code)[-1][3]), data[-1][id])
        assert float_equal((2 * data_proxy.get_all_Data(code)[-2][3]), data[-2][id])

    print "mul_to:(high * 3)"
    alpha = AlphaArray(codes, ["s = (high * 3)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][2] * 3), data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][2] * 3), data[-2][id])

    print "mid(high, low)"
    alpha = AlphaArray(codes, ["s = mid(high, low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][2] + data_proxy.get_all_Data(code)[-1][3]) * 0.5,
                           data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][2] + data_proxy.get_all_Data(code)[-2][3]) * 0.5,
                           data[-2][id])

    print "stddev(open, 2)"
    alpha = AlphaArray(codes, ["s = stddev(open, 2)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        res = np.array([data_proxy.get_all_Data(code)[-3][1], data_proxy.get_all_Data(code)[-2][1],
                        data_proxy.get_all_Data(code)[-1][1]])
        float_equal(res.std(), data[-1][id])
        res = np.array([data_proxy.get_all_Data(code)[-4][1], data_proxy.get_all_Data(code)[-3][1],
                        data_proxy.get_all_Data(code)[-2][1]])
        float_equal(res.std(), data[-2][id])

    print "up(close, 2)"
    alpha = AlphaArray(codes, ["s = up(close, 2)"], "s", 0, 1)
    data = alpha[:]
    for id, code in enumerate(codes):
        res = np.array([data_proxy.get_all_Data(code)[-3][4], data_proxy.get_all_Data(code)[-2][4],
                        data_proxy.get_all_Data(code)[-1][4]])
        assert float_equal(res.std() + res.mean(), data[-1][id])

    print "down(returns, 2)"
    alpha = AlphaArray(codes, ["s = down(returns, 2)"], "s", 0, 1)
    data = alpha[:]
    for id, code in enumerate(codes):
        res = np.array([data_proxy.get_all_Data(code)[-3][7], data_proxy.get_all_Data(code)[-2][7],
                        data_proxy.get_all_Data(code)[-1][7]])
        assert float_equal(res.mean() - res.std(), data[-1][id])

    print "rank(open)"
    alpha = AlphaArray(codes, ["s = rank(open)"], "s", 0, 1)
    data = alpha[:]
    v_open = []
    for code in codes:
        d = data_proxy.get_all_Data(code)[-1]
        v_open.append(d[1])
    v_open = np.array(v_open)
    r_open = rank(v_open)
    c_id = 0
    for i in xrange(len(data[-1])):
        if data[-1][i] >= 0:
            float_equal(r_open[c_id], data[-1][i])
            c_id += 1
    assert c_id == len(r_open)

    print "power_mid(delay(open, 25), high)"
    alpha = AlphaArray(codes, ["s = power_mid(delay(open, 25), high)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        value = data_proxy.get_all_Data(code)[-26][1] * data_proxy.get_all_Data(code)[-1][2]
        assert float_equal(math.sqrt(value), data[-1][id])
        value = data_proxy.get_all_Data(code)[-27][1] * data_proxy.get_all_Data(code)[-2][2]
        # print value
        assert float_equal(math.sqrt(value), data[-2][id])

    print "signed_power_to:(returns ^ 2)"
    alpha = AlphaArray(codes, ["s = (returns ^ 2)"], "s", 0, 1)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][7] * data_proxy.get_all_Data(code)[-1][7]),
                           data[-1][id])

    print "ts_rank(returns, 5)"
    alpha = AlphaArray(codes, ["s = ts_rank(returns, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        returns = np.array([data_proxy.get_all_Data(code)[i - 6][7] for i in xrange(6)])
        assert float_equal(ts_rank(returns, 5)[-1], data[-1][id])
        returns = np.array([data_proxy.get_all_Data(code)[i - 6 - 1][7] for i in xrange(6)])
        assert float_equal(ts_rank(returns, 5)[-1], data[-2][id])

    print "delay(open, 5)"
    alpha = AlphaArray(codes, ["s = delay(open, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(data_proxy.get_all_Data(code)[-6][1], data[-1][id])
        assert float_equal(data_proxy.get_all_Data(code)[-7][1], data[-2][id])

    print "correlation(delay(returns, 3), returns, 10)"
    alpha = AlphaArray(codes, ["s = correlation(delay(returns, 3), returns, 10)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        returns = np.array([data_proxy.get_all_Data(code)[i - 11][7] for i in xrange(11)])
        d_returns = np.array([data_proxy.get_all_Data(code)[i - 14][7] for i in xrange(11)])
        # print id
        v1 = pearson_def(d_returns, returns)
        v2 = data[-1][id]
        assert float_equal(v1 * 0.1, v2 * 0.1)

        returns = np.array([data_proxy.get_all_Data(code)[i - 12][7] for i in xrange(11)])
        d_returns = np.array([data_proxy.get_all_Data(code)[i - 15][7] for i in xrange(11)])
        v1 = pearson_def(d_returns, returns)
        v2 = data[-2][id]
        assert float_equal(v1 * 0.1, v2 * 0.1)

    print "covariance(delay(returns, 3), returns, 10)"
    alpha = AlphaArray(codes, ["s = covariance(delay(returns, 3), returns, 10)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        returns = np.array([data_proxy.get_all_Data(code)[i - 11][7] for i in xrange(11)])
        d_returns = np.array([data_proxy.get_all_Data(code)[i - 14][7] for i in xrange(11)])
        # print id
        v1 = cov_def(d_returns, returns)
        v2 = data[-1][id]
        assert float_equal(v1 * 0.1, v2 * 0.1)

        returns = np.array([data_proxy.get_all_Data(code)[i - 12][7] for i in xrange(11)])
        d_returns = np.array([data_proxy.get_all_Data(code)[i - 15][7] for i in xrange(11)])
        v1 = cov_def(d_returns, returns)
        v2 = data[-2][id]
        assert float_equal(v1 * 0.1, v2 * 0.1)

    print "scale(delay(open, 1))"
    alpha = AlphaArray(codes, ["s = scale(delay(open, 1))"], "s", 0, 1)
    data = alpha[:]
    v_open = []
    for code in codes:
        d = data_proxy.get_all_Data(code)[-2]
        v_open.append(d[1])

    v_open = np.array(v_open)
    v_open /= v_open.sum()
    for i in xrange(len(data[-1])):
        assert float_equal(v_open[i], data[-1][i])

    print "decay_linear(high, 2)"
    alpha = AlphaArray(codes, ["s = decay_linear(high, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(data_proxy.get_all_Data(code)[-1][2] * 3 + data_proxy.get_all_Data(code)[-2][2] * 2 +
                           data_proxy.get_all_Data(code)[-3][2], data[-1][id] * 6)
        assert float_equal(data_proxy.get_all_Data(code)[-2][2] * 3 + data_proxy.get_all_Data(code)[-3][2] * 2 +
                           data_proxy.get_all_Data(code)[-4][2], data[-2][id] * 6)

    print "ts_min(high, 2)"
    alpha = AlphaArray(codes, ["s = ts_min(high, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(min([data_proxy.get_all_Data(code)[-1][2], data_proxy.get_all_Data(code)[-2][2],
                                data_proxy.get_all_Data(code)[-3][2]]), data[-1][id])
        assert float_equal(min([data_proxy.get_all_Data(code)[-2][2], data_proxy.get_all_Data(code)[-3][2],
                                data_proxy.get_all_Data(code)[-4][2]]),
                           data[-2][id])

    print "ts_max(high, 2)"
    alpha = AlphaArray(codes, ["s = ts_max(high, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(max([data_proxy.get_all_Data(code)[-1][2], data_proxy.get_all_Data(code)[-2][2],
                                data_proxy.get_all_Data(code)[-3][2]]), data[-1][id])
        assert float_equal(max([data_proxy.get_all_Data(code)[-2][2], data_proxy.get_all_Data(code)[-3][2],
                                data_proxy.get_all_Data(code)[-4][2]]),
                           data[-2][id])

    print "min(delay(before_high, 1), before_high)"
    alpha = AlphaArray(codes, ["before_high = delay(high, 2)", "s = min(delay(before_high, 1), before_high)"], "s", 0,
                       2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(min([data_proxy.get_all_Data(code)[-4][2], data_proxy.get_all_Data(code)[-3][2]]),
                           data[-1][id])
        assert float_equal(min([data_proxy.get_all_Data(code)[-5][2], data_proxy.get_all_Data(code)[-4][2]]),
                           data[-2][id])

    print "max(high, before_high)"
    alpha = AlphaArray(codes, ["before_high = delay(high, 50)", "s = max(high, before_high)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(max([data_proxy.get_all_Data(code)[-1][2], data_proxy.get_all_Data(code)[-51][2]]),
                           data[-1][id])
        assert float_equal(max([data_proxy.get_all_Data(code)[-2][2], data_proxy.get_all_Data(code)[-52][2]]),
                           data[-2][id])

    print "ts_argmin(volume, 2)"
    alpha = AlphaArray(codes, ["s = ts_argmin(volume, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        if data_proxy.get_all_Data(code)[-3][5] != data_proxy.get_all_Data(code)[-2][5] and \
                data_proxy.get_all_Data(code)[-2][5] != data_proxy.get_all_Data(code)[-1][5] and \
                data_proxy.get_all_Data(code)[-3][5] != data_proxy.get_all_Data(code)[-1][5]:
            assert float_equal(
                np.argmin(np.array([data_proxy.get_all_Data(code)[-3][5], data_proxy.get_all_Data(code)[-2][5],
                                    data_proxy.get_all_Data(code)[-1][5]])), 2 - data[-1][id])
        if data_proxy.get_all_Data(code)[-4][5] != data_proxy.get_all_Data(code)[-3][5] and \
                data_proxy.get_all_Data(code)[-3][5] != data_proxy.get_all_Data(code)[-2][5] and \
                data_proxy.get_all_Data(code)[-4][5] != data_proxy.get_all_Data(code)[-2][5]:
            assert float_equal(
                np.argmin(np.array([data_proxy.get_all_Data(code)[-4][5], data_proxy.get_all_Data(code)[-3][5],
                                    data_proxy.get_all_Data(code)[-2][5]])), 2 - data[-2][id])

    print "ts_argmax(volume, 2)"
    alpha = AlphaArray(codes, ["s = ts_argmax(volume, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        if data_proxy.get_all_Data(code)[-3][5] != data_proxy.get_all_Data(code)[-2][5] and \
                data_proxy.get_all_Data(code)[-2][5] != data_proxy.get_all_Data(code)[-1][5] and \
                data_proxy.get_all_Data(code)[-3][5] != data_proxy.get_all_Data(code)[-1][5]:
            assert float_equal(
                np.argmax(np.array([data_proxy.get_all_Data(code)[-3][5], data_proxy.get_all_Data(code)[-2][5],
                                    data_proxy.get_all_Data(code)[-1][5]])), 2 - data[-1][id])
        if data_proxy.get_all_Data(code)[-4][5] != data_proxy.get_all_Data(code)[-3][5] and \
                data_proxy.get_all_Data(code)[-3][5] != data_proxy.get_all_Data(code)[-2][5] and \
                data_proxy.get_all_Data(code)[-4][5] != data_proxy.get_all_Data(code)[-2][5]:
            assert float_equal(
                np.argmax(np.array([data_proxy.get_all_Data(code)[-4][5], data_proxy.get_all_Data(code)[-3][5],
                                    data_proxy.get_all_Data(code)[-2][5]])), 2 - data[-2][id])

    print "sign(returns)"
    alpha = AlphaArray(codes, ["s = sign(returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        v = data_proxy.get_all_Data(code)[-1][7]
        assert float_equal(1 if v > 0 else (-1 if v < 0 else 0), data[-1][id])
        v = data_proxy.get_all_Data(code)[-2][7]
        assert float_equal(1 if v > 0 else (-1 if v < 0 else 0), data[-2][id])

    print "abs(returns)"
    alpha = AlphaArray(codes, ["s = abs(returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal(abs(data_proxy.get_all_Data(code)[-1][7]), data[-1][id])
        assert float_equal(abs(data_proxy.get_all_Data(code)[-2][7]), data[-2][id])

    print "(open ^ returns)"
    alpha = AlphaArray(codes, ["s = (open ^ returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal(math.pow(data_proxy.get_all_Data(code)[-1][1], data_proxy.get_all_Data(code)[-1][7]),
                           data[-1][id])
        assert float_equal(math.pow(data_proxy.get_all_Data(code)[-2][1], data_proxy.get_all_Data(code)[-2][7]),
                           data[-2][id])

    print "less:(delay(returns, 1) < returns)"
    alpha = AlphaArray(codes, ["s = (delay(returns, 1) < returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if data_proxy.get_all_Data(code)[-2][7] < data_proxy.get_all_Data(code)[-1][7] else 0),
                           data[-1][id])
        assert float_equal((1 if data_proxy.get_all_Data(code)[-3][7] < data_proxy.get_all_Data(code)[-2][7] else 0),
                           data[-2][id])

    print "less_to:(returns < 0)"
    alpha = AlphaArray(codes, ["s = (returns < 0)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if data_proxy.get_all_Data(code)[-1][7] < 0 else 0), data[-1][id])
        assert float_equal((1 if data_proxy.get_all_Data(code)[-2][7] < 0 else 0), data[-2][id])

    print "less_from:(0 < returns)"
    alpha = AlphaArray(codes, ["s = (0 < returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if 0 < data_proxy.get_all_Data(code)[-1][7] else 0), data[-1][id])
        assert float_equal((1 if 0 < data_proxy.get_all_Data(code)[-2][7] else 0), data[-2][id])

    print "((delay(returns, 1) < returns) ? open : close)"
    alpha = AlphaArray(codes, ["s = ((delay(returns, 1) < returns) ? open : close)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][1] if data_proxy.get_all_Data(code)[-2][7] <
                                                                    data_proxy.get_all_Data(code)[-1][7] else
        data_proxy.get_all_Data(code)[-1][4]), data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][1] if data_proxy.get_all_Data(code)[-3][7] <
                                                                    data_proxy.get_all_Data(code)[-2][7] else
        data_proxy.get_all_Data(code)[-2][4]), data[-2][id])

    print "if_to:((delay(returns, 1) < returns) ? 1 : close)"
    alpha = AlphaArray(codes, ["s = ((delay(returns, 1) < returns) ? 1 : close)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if data_proxy.get_all_Data(code)[-2][7] < data_proxy.get_all_Data(code)[-1][7] else
        data_proxy.get_all_Data(code)[-1][4]), data[-1][id])
        assert float_equal((1 if data_proxy.get_all_Data(code)[-3][7] < data_proxy.get_all_Data(code)[-2][7] else
        data_proxy.get_all_Data(code)[-2][4]), data[-2][id])

    print "else_to:((delay(returns, 1) < returns) ? open : 1)"
    alpha = AlphaArray(codes, ["s = ((delay(returns, 1) < returns) ? open : 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_Data(code)[-1][1] if data_proxy.get_all_Data(code)[-2][7] <
                                                                    data_proxy.get_all_Data(code)[-1][7] else 1),
                           data[-1][id])
        assert float_equal((data_proxy.get_all_Data(code)[-2][1] if data_proxy.get_all_Data(code)[-3][7] <
                                                                    data_proxy.get_all_Data(code)[-2][7] else 1),
                           data[-2][id])

    print "if_to_else_to:((delay(returns, 1) < returns) ? 1 : -1)"
    alpha = AlphaArray(codes, ["s = ((delay(returns, 1) < returns) ? 1 : -1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if data_proxy.get_all_Data(code)[-2][7] < data_proxy.get_all_Data(code)[-1][7] else -1),
                           data[-1][id])
        assert float_equal((1 if data_proxy.get_all_Data(code)[-3][7] < data_proxy.get_all_Data(code)[-2][7] else -1),
                           data[-2][id])

    print "indneutralize:(returns - indneutralize(returns, IndClass.market))"
    alpha = AlphaArray(codes, ["s = (returns - indneutralize(returns, IndClass.market))"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        if code[0] == '0':
            market = '0000001'
        else:
            market = '1399001'
        assert float_equal(data_proxy.get_all_Data(code)[-1][7] - data_proxy.get_all_Data(market)[-1][7], data[-1][id])
        assert float_equal(data_proxy.get_all_Data(code)[-2][7] - data_proxy.get_all_Data(market)[-2][7], data[-2][id])

    print "finish test"

def test_cache(af, data_proxy, codes):
    float_equal = lambda x, y: math.fabs(x - y) < 0.01

    # print "test atr15"
    af.cache_alpha("atr15",
                   "mean(max(max(((high - low) / low), ((high - delay(close, 1)) / delay(close, 1))), ((delay(close, 1) - low)/ low)), 14)")
    # alpha = AlphaArray(codes, ["s = delay(atr, -5)"], "s", 10, 2)
    # data = alpha[:]
    # for index, code in enumerate(codes):
    #     print code
    #     close = data_proxy.get_all_Data(code)[-21][4]
    #     if data_proxy.get_all_Data(code)[-1][5] == 0:
    #         continue
    #     sum = 0
    #     for i in xrange(15):
    #         tr = max((data_proxy.get_all_Data(code)[-15 + i][2] - data_proxy.get_all_Data(code)[-15 + i][3]) /
    #                  data_proxy.get_all_Data(code)[-15 + i][3],
    #                  (data_proxy.get_all_Data(code)[-15 + i][2] - close) / close,
    #                  (close - data_proxy.get_all_Data(code)[-15 + i][3]) / data_proxy.get_all_Data(code)[-15 + i][3])
    #         sum += tr
    #         close = data_proxy.get_all_Data(code)[-15 + i][4]
    #
    #     print sum/15, data[-1][index]
    #     assert float_equal(sum / 15, data[-1][index])

    print "test mfe_5 mae_5"
    af.cache_alpha("mfe_5", "(((delay(ts_max(high, 4), -5) - close) / close) / delay(atr15, -5))")
    af.cache_alpha("mae_5", "(((close - delay(ts_min(low, 4), -5)) / close) / delay(atr15, -5))")
    mfe_alpha = AlphaArray(codes, ["s = mfe_5"], "s", 10, 2)
    mae_alpha = AlphaArray(codes, ["s = mae_5"], "s", 10, 2)
    data_mfe = mfe_alpha[:]
    data_mae = mae_alpha[:]
    for id, code in enumerate(codes):
        has_loss = False
        for i in xrange(22):
            if data_proxy.get_all_Data(code)[-1 - i][5] == 0:
                has_loss = True
                break
        if has_loss:
            continue
        print code
        close_10 = data_proxy.get_all_Data(code)[-11][4]
        close_11 = data_proxy.get_all_Data(code)[-12][4]

        close = data_proxy.get_all_Data(code)[-21][4]
        sum = 0
        for i in xrange(15):
            tr = max((data_proxy.get_all_Data(code)[-20 + i][2] - data_proxy.get_all_Data(code)[-20 + i][3]) /
                     data_proxy.get_all_Data(code)[-20 + i][3],
                     (data_proxy.get_all_Data(code)[-20 + i][2] - close) / close,
                     (close - data_proxy.get_all_Data(code)[-20 + i][3]) / data_proxy.get_all_Data(code)[-20 + i][3])
            sum += tr
            close = data_proxy.get_all_Data(code)[-20 + i][4]
        tr = sum / 15
        mfe5 = max([(data_proxy.get_all_Data(code)[-10 + i][2] - close_10) / close_10 for i in xrange(5)])
        mae5 = max([(close_10 - data_proxy.get_all_Data(code)[-10 + i][3]) / close_10 for i in xrange(5)])
        #print mfe5, tr, mfe5/tr, data_mfe[-1][id]
        assert float_equal(mfe5 / tr, data_mfe[-1][id])
        assert float_equal(mae5 / tr, data_mae[-1][id])

        close = data_proxy.get_all_Data(code)[-22][4]
        sum = 0
        for i in xrange(15):
            tr = max((data_proxy.get_all_Data(code)[-21 + i][2] - data_proxy.get_all_Data(code)[-21 + i][3]) /
                     data_proxy.get_all_Data(code)[-21 + i][3],
                     (data_proxy.get_all_Data(code)[-21 + i][2] - close) / close,
                     (close - data_proxy.get_all_Data(code)[-21 + i][3]) / data_proxy.get_all_Data(code)[-21 + i][3])
            sum += tr
            close = data_proxy.get_all_Data(code)[-21 + i][4]
        tr = sum / 15
        mfe5 = max([(data_proxy.get_all_Data(code)[-11 + i][2] - close_11) / close_11 for i in xrange(5)])
        mae5 = max([(close_11 - data_proxy.get_all_Data(code)[-11+ i][3]) / close_11 for i in xrange(5)])
        assert float_equal(mfe5 / tr, data_mfe[-2][id])
        assert float_equal(mae5 / tr, data_mae[-2][id])


if __name__ == "__main__":
    with AlphaForest() as af:
        af.load_db("../data")
        # csv_2_binary(af)

        codes = af.get_stock_codes()
        data_proxy = LocalDataProxy(cache_path="../data", is_offline=True)
        test_cache(af, data_proxy, codes)
        test_base_cal(data_proxy, codes)