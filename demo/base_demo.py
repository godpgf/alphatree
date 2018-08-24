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

    for i in range(len(y)):
        y[arg_index[i]] = len(y) - i - 1

    return y / float(len(y) - 1)


def ts_rank(x, d):
    y = np.zeros(x.shape)
    for i in range(1, d + 1):
        for j in range(i, len(y)):
            z = (x[j] > x[j - i])
            y[j] += z.astype(int)
    return np.array(y) / float(d)

def cov_def(x, y):
    #print "%.4f - %.4f"%((x * y).mean(), x.mean() * y.mean())
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
        print("%.10f %.10f %.10f %.10f %.10f" % (avg_x, avg_y, xdiff2, ydiff2, diffprod))

    return 1 if xdiff2 < 0.000000001 or ydiff2 < 0.000000001 else diffprod / math.sqrt(xdiff2) / math.sqrt(ydiff2)


# 将csv数据转化成程序可以识别的格式
def csv_2_binary(af):
    af.load_db("data")
    af.csv2binary("data", "date")
    af.csv2binary("data", "open")
    af.csv2binary("data", "high")
    af.csv2binary("data", "low")
    af.csv2binary("data", "close")
    af.csv2binary("data", "volume")
    af.csv2binary("data", "vwap")
    af.csv2binary("data", "returns")
    af.csv2binary("data", "amount")
    af.csv2binary("data", "turn")
    af.csv2binary("data", "tcap")
    af.csv2binary("data", "mcap")
    print("finish convert")


# 测试基本计算的正确性
def test_base_cal(data_proxy, codes):
    float_equal = lambda x, y: math.fabs(x - y) < 0.01

    print("test target")
    alpha = AlphaArray(codes[0], ["close_ = delay(close, 1)", "returns_ = returns"], ["close_", "returns_"], 9, 22)
    # 过去10天的数据再往前取20天
    data = alpha[1:-1]
    assert len(data) == 20
    for i in range(20):
        assert float_equal(data[i][0], data_proxy.get_all_data(codes[0])[-31 + i][4])
        assert float_equal(data[i][1], data_proxy.get_all_data(codes[0])[-30 + i][7])

    print("test delay(returns, -2)")
    alpha = AlphaArray(codes, ["r=delay(returns, -2)"], "r", 2, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal(data[0][id], data_proxy.get_all_data(code)[-2][7])
        assert float_equal(data[1][id], data_proxy.get_all_data(code)[-1][7])

    print("sum(high, -2)")
    alpha = AlphaArray(codes, ["s = sum(high, -2)"], "s", 2, 2)
    # 今天、昨天、前天数据一共3天
    data = alpha[:]
    for id, code in enumerate(codes):
        # print(code)
        # print(data[-1][id], data[-2][id],data_proxy.get_all_data(code)[-1][2] + data_proxy.get_all_data(code)[-2][2] + data_proxy.get_all_data(code)[-3][2],data_proxy.get_all_data(code)[-2][2] + data_proxy.get_all_data(code)[-3][2] + data_proxy.get_all_data(code)[-4][2])
        assert float_equal(data_proxy.get_all_data(code)[-1][2] + data_proxy.get_all_data(code)[-2][2] +
                           data_proxy.get_all_data(code)[-3][2], data[-1][id])
        assert float_equal(data_proxy.get_all_data(code)[-2][2] + data_proxy.get_all_data(code)[-3][2] +
                           data_proxy.get_all_data(code)[-4][2], data[-2][id])

    print("product(low, 1)")
    alpha = AlphaArray(codes, ["s = product(low, 1)"], "s", 0, 4)
    data = alpha[2:]
    for id, code in enumerate(codes):
        assert float_equal(data_proxy.get_all_data(code)[-1][3] * data_proxy.get_all_data(code)[-2][3] / 100,
                           data[-1][id] / 100)
        assert float_equal(data_proxy.get_all_data(code)[-2][3] * data_proxy.get_all_data(code)[-3][3] / 100,
                           data[-2][id] / 100)

    print("ma(close, 1)")
    alpha = AlphaArray(codes, ["s = ma(close, 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][4] + data_proxy.get_all_data(code)[-2][4]) / 2,
                           data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][4] + data_proxy.get_all_data(code)[-3][4]) / 2,
                           data[-2][id])

    print("lerp(high, low, 0.4)")
    alpha = AlphaArray(codes, ["s = lerp(high, low, 0.4)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][2] * 0.4 + data_proxy.get_all_data(code)[-1][3] * 0.6),
                           data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][2] * 0.4 + data_proxy.get_all_data(code)[-2][3] * 0.6),
                           data[-2][id])

    print("delta(open, 5)")
    alpha = AlphaArray(codes, ["s = delta(open, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(data_proxy.get_all_data(code)[-1][1] - data_proxy.get_all_data(code)[-6][1], data[-1][id])
        assert float_equal(data_proxy.get_all_data(code)[-2][1] - data_proxy.get_all_data(code)[-7][1], data[-2][id])

    print("pre_rise(open, 5)")
    alpha = AlphaArray(codes, ["s = pre_rise(open, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal((data_proxy.get_all_data(code)[-1][1] - data_proxy.get_all_data(code)[-6][1]) / data_proxy.get_all_data(code)[-1][1], data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][1] - data_proxy.get_all_data(code)[-7][1]) / data_proxy.get_all_data(code)[-2][1], data[-2][id])

    print("rise(open, -5)")
    alpha = AlphaArray(codes, ["s = rise(open, -5)"], "s", 5, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal((data_proxy.get_all_data(code)[-6][1] - data_proxy.get_all_data(code)[-1][1]) / data_proxy.get_all_data(code)[-1][1], data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-7][1] - data_proxy.get_all_data(code)[-2][1]) / data_proxy.get_all_data(code)[-2][1], data[-2][id])

    print("mean_rise(open, 5)")
    alpha = AlphaArray(codes, ["s = mean_rise(open, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        float_equal((data_proxy.get_all_data(code)[-1][1] - data_proxy.get_all_data(code)[-6][1]) / 5, data[-1][id])
        float_equal((data_proxy.get_all_data(code)[-2][1] - data_proxy.get_all_data(code)[-7][1]) / 5, data[-2][id])

    print("(low / high)")
    alpha = AlphaArray(codes, ["s = (low / high)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(data_proxy.get_all_data(code)[-1][3] / data_proxy.get_all_data(code)[-1][2], data[-1][id])
        assert float_equal(data_proxy.get_all_data(code)[-2][3] / data_proxy.get_all_data(code)[-2][2], data[-2][id])

    print("div_from:(100 / low)")
    alpha = AlphaArray(codes, ["s = (100 / low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(100 / data_proxy.get_all_data(code)[-1][3], data[-1][id])
        assert float_equal(100 / data_proxy.get_all_data(code)[-2][3], data[-2][id])

    print("div_to:(low / 2)")
    alpha = AlphaArray(codes, ["s = (low / 2)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(data_proxy.get_all_data(code)[-1][3], data[-1][id] * 2)
        assert float_equal(data_proxy.get_all_data(code)[-2][3], data[-2][id] * 2)

    print("mean_ratio(close, 1)")
    alpha = AlphaArray(codes, ["s = mean_ratio(close, 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal(data_proxy.get_all_data(code)[-1][4] / (
                data_proxy.get_all_data(code)[-1][4] + data_proxy.get_all_data(code)[-2][4]) * 2, data[-1][id])
        assert float_equal(data_proxy.get_all_data(code)[-2][4] / (
                data_proxy.get_all_data(code)[-2][4] + data_proxy.get_all_data(code)[-3][4]) * 2, data[-2][id])

    print("(high + low)")
    alpha = AlphaArray(codes, ["s = (high + low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][2] + data_proxy.get_all_data(code)[-1][3]), data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][2] + data_proxy.get_all_data(code)[-2][3]), data[-2][id])

    print("add_from:(1 + low)")
    alpha = AlphaArray(codes, ["s = (1 + low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 + data_proxy.get_all_data(code)[-1][3]), data[-1][id])
        assert float_equal((1 + data_proxy.get_all_data(code)[-2][3]), data[-2][id])

    print("add_to:(high + 1)")
    alpha = AlphaArray(codes, ["s = (high + 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][2] + 1), data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][2] + 1), data[-2][id])

    print("reduce:(high - low)")
    alpha = AlphaArray(codes, ["s = (high - low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][2] - data_proxy.get_all_data(code)[-1][3]), data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][2] - data_proxy.get_all_data(code)[-2][3]), data[-2][id])

    print("reduce_from:(1000 - low)")
    alpha = AlphaArray(codes, ["s = (1000 - low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1000 - data_proxy.get_all_data(code)[-1][3]), data[-1][id])
        assert float_equal((1000 - data_proxy.get_all_data(code)[-2][3]), data[-2][id])

    print("reduce_to:(high - 1)")
    alpha = AlphaArray(codes, ["s = (high - 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][2] - 1), data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][2] - 1), data[-2][id])

    print("(high * low)")
    alpha = AlphaArray(codes, ["s = (high * low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][2] * data_proxy.get_all_data(code)[-1][3]) / 10,
                           data[-1][id] / 10)
        assert float_equal((data_proxy.get_all_data(code)[-2][2] * data_proxy.get_all_data(code)[-2][3]) / 10,
                           data[-2][id] / 10)

    print("mul_from:(2 * low)")
    alpha = AlphaArray(codes, ["s = (2 * low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((2 * data_proxy.get_all_data(code)[-1][3]), data[-1][id])
        assert float_equal((2 * data_proxy.get_all_data(code)[-2][3]), data[-2][id])

    print("mul_to:(high * 3)")
    alpha = AlphaArray(codes, ["s = (high * 3)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][2] * 3), data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][2] * 3), data[-2][id])

    print("mid(high, low)")
    alpha = AlphaArray(codes, ["s = mid(high, low)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][2] + data_proxy.get_all_data(code)[-1][3]) * 0.5,
                           data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][2] + data_proxy.get_all_data(code)[-2][3]) * 0.5,
                           data[-2][id])

    print("stddev(open, 2)")
    alpha = AlphaArray(codes, ["s = stddev(open, 2)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        res = np.array([data_proxy.get_all_data(code)[-3][1], data_proxy.get_all_data(code)[-2][1],
                        data_proxy.get_all_data(code)[-1][1]])
        float_equal(res.std(), data[-1][id])
        res = np.array([data_proxy.get_all_data(code)[-4][1], data_proxy.get_all_data(code)[-3][1],
                        data_proxy.get_all_data(code)[-2][1]])
        float_equal(res.std(), data[-2][id])

    print("up(close, 2)")
    alpha = AlphaArray(codes, ["s = up(close, 2)"], "s", 0, 1)
    data = alpha[:]
    for id, code in enumerate(codes):
        res = np.array([data_proxy.get_all_data(code)[-3][4], data_proxy.get_all_data(code)[-2][4],
                        data_proxy.get_all_data(code)[-1][4]])
        assert float_equal(res.std() + res.mean(), data[-1][id])

    print("down(returns, 2)")
    alpha = AlphaArray(codes, ["s = down(returns, 2)"], "s", 0, 1)
    data = alpha[:]
    for id, code in enumerate(codes):
        res = np.array([data_proxy.get_all_data(code)[-3][7], data_proxy.get_all_data(code)[-2][7],
                        data_proxy.get_all_data(code)[-1][7]])
        assert float_equal(res.mean() - res.std(), data[-1][id])

    print("rank(open)")
    alpha = AlphaArray(codes, ["s = rank(open)"], "s", 0, 1)
    data = alpha[:]
    v_open = []
    for code in codes:
        d = data_proxy.get_all_data(code)[-1]
        v_open.append(d[1])
    v_open = np.array(v_open)
    r_open = rank(v_open)
    c_id = 0
    for i in range(len(data[-1])):
        if data[-1][i] >= 0:
            float_equal(r_open[c_id], data[-1][i])
            c_id += 1
    assert c_id == len(r_open)

    print("power_mid(delay(open, 25), high)")
    alpha = AlphaArray(codes, ["s = power_mid(delay(open, 25), high)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        value = data_proxy.get_all_data(code)[-26][1] * data_proxy.get_all_data(code)[-1][2]
        assert float_equal(math.sqrt(value), data[-1][id])
        value = data_proxy.get_all_data(code)[-27][1] * data_proxy.get_all_data(code)[-2][2]
        # print value
        assert float_equal(math.sqrt(value), data[-2][id])

    print("signed_power_to:(returns ^ 2)")
    alpha = AlphaArray(codes, ["s = (returns ^ 2)"], "s", 0, 1)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][7] * data_proxy.get_all_data(code)[-1][7]),
                           data[-1][id])

    print("ts_rank(returns, 5)")
    alpha = AlphaArray(codes, ["s = ts_rank(returns, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        returns = np.array([data_proxy.get_all_data(code)[i - 6][7] for i in range(6)])
        assert float_equal(ts_rank(returns, 5)[-1], data[-1][id])
        returns = np.array([data_proxy.get_all_data(code)[i - 6 - 1][7] for i in range(6)])
        assert float_equal(ts_rank(returns, 5)[-1], data[-2][id])

    print("delay(open, 5)")
    alpha = AlphaArray(codes, ["s = delay(open, 5)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # 往后推1天就是-2，所以是-6
        assert float_equal(data_proxy.get_all_data(code)[-6][1], data[-1][id])
        assert float_equal(data_proxy.get_all_data(code)[-7][1], data[-2][id])

    print("correlation(delay(returns, 3), returns, 10)")
    alpha = AlphaArray(codes, ["s = correlation(delay(returns, 3), returns, 10)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        returns = np.array([data_proxy.get_all_data(code)[i - 11][7] for i in range(11)])
        d_returns = np.array([data_proxy.get_all_data(code)[i - 14][7] for i in range(11)])
        # print id
        v1 = pearson_def(d_returns, returns)
        v2 = data[-1][id]
        assert float_equal(v1 * 0.1, v2 * 0.1)

        returns = np.array([data_proxy.get_all_data(code)[i - 12][7] for i in range(11)])
        d_returns = np.array([data_proxy.get_all_data(code)[i - 15][7] for i in range(11)])
        v1 = pearson_def(d_returns, returns)
        v2 = data[-2][id]
        assert float_equal(v1 * 0.1, v2 * 0.1)

    print("covariance(delay(returns, 3), returns, 10)")
    alpha = AlphaArray(codes, ["s = covariance(delay(returns, 3), returns, 10)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        returns = np.array([data_proxy.get_all_data(code)[i - 11][7] for i in range(11)])
        d_returns = np.array([data_proxy.get_all_data(code)[i - 14][7] for i in range(11)])
        # print id
        v1 = cov_def(d_returns, returns)
        v2 = data[-1][id]
        assert float_equal(v1 * 0.1, v2 * 0.1)

        returns = np.array([data_proxy.get_all_data(code)[i - 12][7] for i in range(11)])
        d_returns = np.array([data_proxy.get_all_data(code)[i - 15][7] for i in range(11)])
        v1 = cov_def(d_returns, returns)
        v2 = data[-2][id]
        assert float_equal(v1 * 0.1, v2 * 0.1)

    print("scale(delay(open, 1))")
    alpha = AlphaArray(codes, ["s = scale(delay(open, 1))"], "s", 0, 1)
    data = alpha[:]
    v_open = []
    for code in codes:
        d = data_proxy.get_all_data(code)[-2]
        v_open.append(d[1])

    v_open = np.array(v_open)
    v_open /= v_open.sum()
    for i in range(len(data[-1])):
        assert float_equal(v_open[i], data[-1][i])

    print("wma(high, 2)")
    alpha = AlphaArray(codes, ["s = wma(high, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(data_proxy.get_all_data(code)[-1][2] * 3 + data_proxy.get_all_data(code)[-2][2] * 2 +
                           data_proxy.get_all_data(code)[-3][2], data[-1][id] * 6)
        assert float_equal(data_proxy.get_all_data(code)[-2][2] * 3 + data_proxy.get_all_data(code)[-3][2] * 2 +
                           data_proxy.get_all_data(code)[-4][2], data[-2][id] * 6)

    print("ts_min(high, 2)")
    alpha = AlphaArray(codes, ["s = ts_min(high, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(min([data_proxy.get_all_data(code)[-1][2], data_proxy.get_all_data(code)[-2][2],
                                data_proxy.get_all_data(code)[-3][2]]), data[-1][id])
        assert float_equal(min([data_proxy.get_all_data(code)[-2][2], data_proxy.get_all_data(code)[-3][2],
                                data_proxy.get_all_data(code)[-4][2]]),
                           data[-2][id])

    print("ts_max(high, 2)")
    alpha = AlphaArray(codes, ["s = ts_max(high, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(max([data_proxy.get_all_data(code)[-1][2], data_proxy.get_all_data(code)[-2][2],
                                data_proxy.get_all_data(code)[-3][2]]), data[-1][id])
        assert float_equal(max([data_proxy.get_all_data(code)[-2][2], data_proxy.get_all_data(code)[-3][2],
                                data_proxy.get_all_data(code)[-4][2]]),
                           data[-2][id])

    print("min(delay(before_high, 1), before_high)")
    alpha = AlphaArray(codes, ["before_high = delay(high, 2)", "s = min(delay(before_high, 1), before_high)"], "s", 0,
                       2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(min([data_proxy.get_all_data(code)[-4][2], data_proxy.get_all_data(code)[-3][2]]),
                           data[-1][id])
        assert float_equal(min([data_proxy.get_all_data(code)[-5][2], data_proxy.get_all_data(code)[-4][2]]),
                           data[-2][id])

    print("max(high, before_high)")
    alpha = AlphaArray(codes, ["before_high = delay(high, 50)", "s = max(high, before_high)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        assert float_equal(max([data_proxy.get_all_data(code)[-1][2], data_proxy.get_all_data(code)[-51][2]]),
                           data[-1][id])
        assert float_equal(max([data_proxy.get_all_data(code)[-2][2], data_proxy.get_all_data(code)[-52][2]]),
                           data[-2][id])

    print("ts_argmin(volume, 2)")
    alpha = AlphaArray(codes, ["s = ts_argmin(volume, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        if data_proxy.get_all_data(code)[-3][5] != data_proxy.get_all_data(code)[-2][5] and \
                data_proxy.get_all_data(code)[-2][5] != data_proxy.get_all_data(code)[-1][5] and \
                data_proxy.get_all_data(code)[-3][5] != data_proxy.get_all_data(code)[-1][5]:
            assert float_equal(
                np.argmin(np.array([data_proxy.get_all_data(code)[-3][5], data_proxy.get_all_data(code)[-2][5],
                                    data_proxy.get_all_data(code)[-1][5]])), 2 - data[-1][id])
        if data_proxy.get_all_data(code)[-4][5] != data_proxy.get_all_data(code)[-3][5] and \
                data_proxy.get_all_data(code)[-3][5] != data_proxy.get_all_data(code)[-2][5] and \
                data_proxy.get_all_data(code)[-4][5] != data_proxy.get_all_data(code)[-2][5]:
            assert float_equal(
                np.argmin(np.array([data_proxy.get_all_data(code)[-4][5], data_proxy.get_all_data(code)[-3][5],
                                    data_proxy.get_all_data(code)[-2][5]])), 2 - data[-2][id])

    print("ts_argmax(volume, 2)")
    alpha = AlphaArray(codes, ["s = ts_argmax(volume, 2)"], "s", 0, 2)
    data = alpha[:]
    # 今天、昨天、前天数据一共3天
    for id, code in enumerate(codes):
        if data_proxy.get_all_data(code)[-3][5] != data_proxy.get_all_data(code)[-2][5] and \
                data_proxy.get_all_data(code)[-2][5] != data_proxy.get_all_data(code)[-1][5] and \
                data_proxy.get_all_data(code)[-3][5] != data_proxy.get_all_data(code)[-1][5]:
            assert float_equal(
                np.argmax(np.array([data_proxy.get_all_data(code)[-3][5], data_proxy.get_all_data(code)[-2][5],
                                    data_proxy.get_all_data(code)[-1][5]])), 2 - data[-1][id])
        if data_proxy.get_all_data(code)[-4][5] != data_proxy.get_all_data(code)[-3][5] and \
                data_proxy.get_all_data(code)[-3][5] != data_proxy.get_all_data(code)[-2][5] and \
                data_proxy.get_all_data(code)[-4][5] != data_proxy.get_all_data(code)[-2][5]:
            assert float_equal(
                np.argmax(np.array([data_proxy.get_all_data(code)[-4][5], data_proxy.get_all_data(code)[-3][5],
                                    data_proxy.get_all_data(code)[-2][5]])), 2 - data[-2][id])

    print("sign(returns)")
    alpha = AlphaArray(codes, ["s = sign(returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        v = data_proxy.get_all_data(code)[-1][7]
        assert float_equal(1 if v > 0 else (-1 if v < 0 else 0), data[-1][id])
        v = data_proxy.get_all_data(code)[-2][7]
        assert float_equal(1 if v > 0 else (-1 if v < 0 else 0), data[-2][id])

    print("abs(returns)")
    alpha = AlphaArray(codes, ["s = abs(returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal(abs(data_proxy.get_all_data(code)[-1][7]), data[-1][id])
        assert float_equal(abs(data_proxy.get_all_data(code)[-2][7]), data[-2][id])

    print("(open ^ returns)")
    alpha = AlphaArray(codes, ["s = (open ^ returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal(math.pow(data_proxy.get_all_data(code)[-1][1], data_proxy.get_all_data(code)[-1][7]),
                           data[-1][id])
        assert float_equal(math.pow(data_proxy.get_all_data(code)[-2][1], data_proxy.get_all_data(code)[-2][7]),
                           data[-2][id])

    print("less:(delay(returns, 1) < returns)")
    alpha = AlphaArray(codes, ["s = (delay(returns, 1) < returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if data_proxy.get_all_data(code)[-2][7] < data_proxy.get_all_data(code)[-1][7] else 0),
                           data[-1][id])
        assert float_equal((1 if data_proxy.get_all_data(code)[-3][7] < data_proxy.get_all_data(code)[-2][7] else 0),
                           data[-2][id])

    print("less_to:(returns < 0)")
    alpha = AlphaArray(codes, ["s = (returns < 0)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if data_proxy.get_all_data(code)[-1][7] < 0 else 0), data[-1][id])
        assert float_equal((1 if data_proxy.get_all_data(code)[-2][7] < 0 else 0), data[-2][id])

    print("less_from:(0 < returns)")
    alpha = AlphaArray(codes, ["s = (0 < returns)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if 0 < data_proxy.get_all_data(code)[-1][7] else 0), data[-1][id])
        assert float_equal((1 if 0 < data_proxy.get_all_data(code)[-2][7] else 0), data[-2][id])

    print("((delay(returns, 1) < returns) ? open : close)")
    alpha = AlphaArray(codes, ["s = ((delay(returns, 1) < returns) ? open : close)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][1] if data_proxy.get_all_data(code)[-2][7] <
                                                                    data_proxy.get_all_data(code)[-1][7] else
        data_proxy.get_all_data(code)[-1][4]), data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][1] if data_proxy.get_all_data(code)[-3][7] <
                                                                    data_proxy.get_all_data(code)[-2][7] else
        data_proxy.get_all_data(code)[-2][4]), data[-2][id])

    print("if_to:((delay(returns, 1) < returns) ? 1 : close)")
    alpha = AlphaArray(codes, ["s = ((delay(returns, 1) < returns) ? 1 : close)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if data_proxy.get_all_data(code)[-2][7] < data_proxy.get_all_data(code)[-1][7] else
        data_proxy.get_all_data(code)[-1][4]), data[-1][id])
        assert float_equal((1 if data_proxy.get_all_data(code)[-3][7] < data_proxy.get_all_data(code)[-2][7] else
        data_proxy.get_all_data(code)[-2][4]), data[-2][id])

    print("else_to:((delay(returns, 1) < returns) ? open : 1)")
    alpha = AlphaArray(codes, ["s = ((delay(returns, 1) < returns) ? open : 1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((data_proxy.get_all_data(code)[-1][1] if data_proxy.get_all_data(code)[-2][7] <
                                                                    data_proxy.get_all_data(code)[-1][7] else 1),
                           data[-1][id])
        assert float_equal((data_proxy.get_all_data(code)[-2][1] if data_proxy.get_all_data(code)[-3][7] <
                                                                    data_proxy.get_all_data(code)[-2][7] else 1),
                           data[-2][id])

    print("if_to_else_to:((delay(returns, 1) < returns) ? 1 : -1)")
    alpha = AlphaArray(codes, ["s = ((delay(returns, 1) < returns) ? 1 : -1)"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        assert float_equal((1 if data_proxy.get_all_data(code)[-2][7] < data_proxy.get_all_data(code)[-1][7] else -1),
                           data[-1][id])
        assert float_equal((1 if data_proxy.get_all_data(code)[-3][7] < data_proxy.get_all_data(code)[-2][7] else -1),
                           data[-2][id])

    print("indneutralize:(returns - indneutralize(returns, IndClass.market))")
    alpha = AlphaArray(codes, ["s = (returns - indneutralize(returns, IndClass.market))"], "s", 0, 2)
    data = alpha[:]
    for id, code in enumerate(codes):
        # if code[0] == '0':
        #     market = '0000001'
        # else:
        #     market = '1399001'
        market = '1399005'
        assert float_equal(data_proxy.get_all_data(code)[-1][7] - data_proxy.get_all_data(market)[-1][7], data[-1][id])
        assert float_equal(data_proxy.get_all_data(code)[-2][7] - data_proxy.get_all_data(market)[-2][7], data[-2][id])

    print("finish test")

def test_cache(af, data_proxy):
    float_equal = lambda x, y: math.fabs(x - y) < 0.01

    print("test stock sign")
    af.cache_codes_sign("test_market_sign", "(returns > 0.02)", [ "1399005"])
    data = AlphaArray("test_market_sign",["s = returns"], "s", 2, 100, 1)[:]
    assert af.get_sign_num(2, 100, "test_market_sign") == len(data)
    index = 0
    for i in range(100):
        value = data_proxy.get_all_data("1399005")[-2-100+i][7]
        if value > 0.02:
            float_equal(value * 10, data[index] * 10)
            index += 1

def test_sign(af, data_proxy):
    float_equal = lambda x, y: math.fabs(x - y) < 0.01

    codes = af.get_stock_codes()
    af.cache_miss()
    #以成交量大于0作为信号
    af.cache_codes_sign("test_codes_sign", "(volume > 0)", codes)
    test_days = 200
    #得到最后一天满足信号的所有股票前250天的数据
    alpha = AlphaArray("test_codes_sign",["v = abs(returns)"], "v", 0, 1, test_days)[:]
    #得到最后一天满足信号的所有股票id
    sign_code_ids = af.get_stock_ids(0, 1, "test_codes_sign")
    #每个信号有test_days天的数据，这里便利每个信号
    for index, cid in enumerate(sign_code_ids):
        data = alpha[index]
        assert len(data) == test_days
        #得到当前信号对应的股票代码
        code = af.get_code(cid)
        # if code == '1000677':
        #     print("d")
        real_data = data_proxy.get_all_data(code)
        #确保当前股票最后一天的成交量是大于0的，满足信号要求
        assert real_data[-1][5] > 0
        float_equal(data[-1], math.fabs(real_data[-1][7]))
        for i in range(test_days):
            rid = -1 - i
            if not float_equal(data[rid], math.fabs(real_data[rid][7])):
                print('start')
                for j in range(test_days):
                    print(real_data[-1-j][7])
                print(code, data[rid], real_data[rid][7])
                assert float_equal(data[rid], math.fabs(real_data[rid][7]))

    af.cache_sign("test_returns_sign", "((delay(returns, 1) > 0.01) & (volume > 0))")
    alpha = AlphaArray("test_returns_sign", ["r = delay(returns, 1)","v = volume"], ["r","v"], 2, 5, 1)
    data = alpha[:]
    for d in data:
        assert d[0] > 0.01
        assert d[1] > 0
        
    af.cache_sign("test_open_close", "((delay(close, -2) > delay(open, -1))  & (volume > 0))")
    #alpha = AlphaArray("test_open_close", ["v1 = delay(open, -1)", "v2 = delay(close, -2)"], ["v1", "v2"], 2, 1, 1)
    ids = af.get_stock_ids(2, 0, "test_open_close")
    codes = [af.get_code(id) for id in ids]
    for id, code in enumerate(codes):
        open_1 = data_proxy.get_all_data(code)[-2][1]
        close_2 = data_proxy.get_all_data(code)[-1][4]
        assert close_2 > open_1




def test_alpha_beta(af, data_proxy, codes):
    float_equal = lambda x, y: math.fabs(x - y) < 0.01
    alpha = AlphaArray(codes, ["alpha_5_", "beta_5_", "lstsq_ = lstsq(close, indneutralize(close, IndClass.market), alpha_5_, beta_5_, -5)", "alpha_5 = wait(lstsq_, alpha_5_)", "beta_5 = wait(lstsq_, beta_5_)"], ["alpha_5", "beta_5"], 10, 2)
    #af.cache_alpha_list(["alpha_5_", "beta_5_", "lstsq_ = lstsq(close, indneutralize(close, IndClass.market), alpha_5_, beta_5_, -5)", "alpha_5 = wait(lstsq_, alpha_5_)", "beta_5 = wait(lstsq_, beta_5_)"], ["alpha_5", "beta_5"])
    data = alpha[:]
    alpha2 = AlphaArray(codes, ["a5 = alpha_5", "b5 = beta_5"],["a5", "b5"], 10, 2)
    data2 = alpha2[:]
    for id, code in enumerate(codes):
        print(code)
        # if code[0] == '0':
        #     market = '0000001'
        # else:
        #     market = '1399001'
        market = '1399005'

        market_close = data_proxy.get_all_data(market)[-11][4]
        close = data_proxy.get_all_data(code)[-11][4]
        market_list = []
        local_list = []

        volume_mul = 1
        for i in range(12):
            volume_mul *= data_proxy.get_all_data(market)[-i - 1][5]
        if volume_mul == 0:
            continue

        for i in range(5):
            market_list.append(data_proxy.get_all_data(market)[-10 + i][4] / market_close)
            local_list.append(data_proxy.get_all_data(code)[-10 + i][4] / close)

        m, c = np.linalg.lstsq(np.vstack([np.array(market_list), np.ones(len(market_list))]).T, np.array(local_list))[0]
        float_equal(data2[-1][1][id], data[-1][1][id])
        float_equal(data2[-1][0][id], data[-1][0][id])
        float_equal(m, data[-1][1][id])
        float_equal(c, data[-1][0][id])

        market_close = data_proxy.get_all_data(market)[-12][4]
        close = data_proxy.get_all_data(code)[-12][4]
        market_list = []
        local_list = []
        for i in range(5):
            market_list.append(data_proxy.get_all_data(market)[-11 + i][4] / market_close)
            local_list.append(data_proxy.get_all_data(code)[-11 + i][4] / close)
        m, c = np.linalg.lstsq(np.vstack([np.array(market_list), np.ones(len(market_list))]).T, np.array(local_list))[0]
        float_equal(data2[-2][1][id], data[-2][1][id])
        float_equal(data2[-2][0][id], data[-2][0][id])
        float_equal(m, data[-2][1][id])
        float_equal(c, data[-2][0][id])


def cache_feature(af, feature_path):
    fn = 0
    with open(feature_path, 'r') as f:
        line = f.readline()
        while line:
            af.cache_alpha("t%d"%fn, line[:-1].split('#')[0])
            fn += 1
            line = f.readline()
    return fn

if __name__ == "__main__":
    #下载所有股票数据
    # download_stock_data()
    #将下载下来的csv变成二进制特征
    # with AlphaForest() as af:
    #    csv_2_binary(af)

    with AlphaForest() as af:
        #加载数据描述
        af.load_db("data")
        codes = af.get_stock_codes()

        #得到最后一天所有股票的收益
        r = AlphaArray(codes,["v1=returns"],"v1", 0, 1)[0]
        data_proxy = LocalDataProxy(cache_path="data", is_offline=True)
        #测试基本运算
        test_base_cal(data_proxy, codes)
        #测试信号运算
        test_sign(af, data_proxy)
        test_cache(af, data_proxy)
        #测试计算alpha beta
        test_alpha_beta(af, data_proxy, codes)
