# coding=utf-8
# author=godpgf
import re
from stdb import *
from pyalphatree import AlphaForest
import numpy as np
import math
import time

def read_alpha_tree_list(path):
    return read_alpha_list(path, lambda line: re.search(r"(?P<alpha>\w+): (?P<content>.*)", line).group('content'))

def read_alpha_tree_dict(path):
    alpha_tree = {}
    try:
        f = open(path,'rU')
        while True:
            line = f.readline()
            if line and len(line) > 0:
                tmp = line.split('=')
                key = tmp[0][:-1]
                line = tmp[1][1:-1]
                alpha_tree[key] = line
            else:
                break
    finally:
        f.close()
    return alpha_tree

def read_alpha_list(path, line_format_function):
    alpha_tree = []
    try:
        f = open(path,'rU')
        while True:
            line = f.readline()
            if line and len(line) > 0:
                line = line_format_function(line)
                alpha_tree.append(line)
            else:
                break
    finally:
        f.close()
    return alpha_tree

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


def pearson_def(x, y):
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

    return diffprod / math.sqrt(xdiff2 * ydiff2)

if __name__ == '__main__':
    subalphatree_dict = read_alpha_tree_dict("../doc/subalpha.txt")

    af = AlphaForest()
    af.load_data()
    dataProxy = LocalDataProxy("data")

    alphatree_list = read_alpha_tree_list("../doc/alpha.txt")
    alphatree_id_list = []

    start = time.time()

    for id, at in enumerate(alphatree_list):

        sub_alpha_dict = {}
        for key, value in subalphatree_dict.items():
            if key in at:
                sub_alpha_dict[key] = value

        alphatree_id = af.create_alphatree(at, sub_alpha_dict)
        alphatree_id_list.append(alphatree_id)
        encodestr = af.encode_alphatree(alphatree_id)

        print ">>>>>>>>>>>>>>>>>>>>>>>>>>"
        print at
        if at != encodestr:
            print encodestr
        assert at == encodestr

        root = af.load_alphatree(alphatree_id)
        encode_line = root.encode()
        print encode_line
        tmp_alphatree_id = af.create_alphatree(encode_line, sub_alpha_dict)
        encodestr = af.encode_alphatree(tmp_alphatree_id)
        if at != encodestr:
            print encodestr
        assert at == encodestr

        a1,c1 = af.cal_alphatree(alphatree_id)
        a2,c2 = af.cal_alphatree(tmp_alphatree_id)
        for ind in xrange(len(a1[0])):
            assert a1[0][ind] == a2[0][ind]

        af.release_alphatree(tmp_alphatree_id)
        print "score %.4f"%af.eval_alphatree(alphatree_id)
        #af.cal_alphatree(alphatree_id)

    end = time.time()

    print "use time:"
    print end - start

    sub_alphatree = af.summary_sub_alphatree(alphatree_id_list)

    print "start test --------"

    #测试-----------------------------------------------------------
    print "target"
    codes = af.get_codes(0,5,1,32)
    alphatree_id = af.create_alphatree("close")
    alpha = af.cal_stock_alpha(alphatree_id, codes, 5, 32)
    for index, code in enumerate(codes):
        for i in xrange(32):
            float_equal(dataProxy.get_all_Data(code)[-(32+5)+i][4], alpha[i][index])
    for future_index in xrange(5):
        target = af.get_target(future_index, 5, codes, 32)
        for index, code in enumerate(codes):
            if code[0] == '0':
                market = '0000001'
            else:
                market = '1399001'

            for i in xrange(32):
                sumV = 0
                for j in xrange(future_index+1):
                    sumV += math.log((dataProxy.get_all_Data(code)[-(32+5)+i+(j+1)][7] - dataProxy.get_all_Data(market)[-(32+5)+i+(j+1)][7]) + 1)
                sumV /= (future_index+1)
                float_equal(sumV, target[i][index])
    af.release_alphatree(alphatree_id)

    print "returns"
    alpha, codes = af.cal_alpha("returns", 0, 2)
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][7],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][7],alpha[-2][index])

    print "sum(high, 2)"
    alpha, codes = af.cal_alpha("sum(high, 2)", 0, 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][2]+dataProxy.get_all_Data(code)[-2][2]+dataProxy.get_all_Data(code)[-3][2],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][2]+dataProxy.get_all_Data(code)[-3][2]+dataProxy.get_all_Data(code)[-4][2],alpha[-2][index])

    print "product(low, 1)"
    alpha, codes = af.cal_alpha("product(low, 1)", 0, 2)
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][3]*dataProxy.get_all_Data(code)[-2][3]/100,alpha[-1][index]/100)
        float_equal(dataProxy.get_all_Data(code)[-2][3]*dataProxy.get_all_Data(code)[-3][3]/100,alpha[-2][index]/100)

    print "mean(close, 1)"
    alpha, codes = af.cal_alpha("mean(close, 1)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][4]+dataProxy.get_all_Data(code)[-2][4])/2,alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][4]+dataProxy.get_all_Data(code)[-3][4])/2,alpha[-2][index])

    print "lerp(high, low, 0.4)"
    alpha, codes = af.cal_alpha("lerp(high, low, 0.4)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]*0.4+dataProxy.get_all_Data(code)[-1][3]*0.6),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]*0.4+dataProxy.get_all_Data(code)[-2][3]*0.6),alpha[-2][index])

    print "delta(open, 5)"
    alpha, codes = af.cal_alpha("delta(open, 5)", 0, 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(dataProxy.get_all_Data(code)[-1][1] - dataProxy.get_all_Data(code)[-6][1],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][1] - dataProxy.get_all_Data(code)[-7][1],alpha[-2][index])

    print "mean_rise(open, 5)"
    alpha, codes = af.cal_alpha("mean_rise(open, 5)", 0, 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal((dataProxy.get_all_Data(code)[-1][1] - dataProxy.get_all_Data(code)[-6][1])/5,alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][1] - dataProxy.get_all_Data(code)[-7][1])/5,alpha[-2][index])

    print "(low / high)"
    alpha, codes = af.cal_alpha("(low / high)", 0, 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(dataProxy.get_all_Data(code)[-1][3]/dataProxy.get_all_Data(code)[-1][2],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][3]/dataProxy.get_all_Data(code)[-2][2],alpha[-2][index])

    print "div_from:(100 / low)"
    alpha, codes = af.cal_alpha("(100 / low)", 0, 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(100 / dataProxy.get_all_Data(code)[-1][3],alpha[-1][index])
        float_equal(100 / dataProxy.get_all_Data(code)[-2][3],alpha[-2][index])

    print "div_to:(low / 2)"
    alpha, codes = af.cal_alpha("(low / 2)", 0, 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(dataProxy.get_all_Data(code)[-1][3],alpha[-1][index] * 2)
        float_equal(dataProxy.get_all_Data(code)[-2][3],alpha[-2][index] * 2)

    print "mean_ratio(close, 1)"
    alpha, codes = af.cal_alpha("mean_ratio(close, 1)", 0, 2)
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][4] / (dataProxy.get_all_Data(code)[-1][4]+dataProxy.get_all_Data(code)[-2][4])*2,alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][4] / (dataProxy.get_all_Data(code)[-2][4]+dataProxy.get_all_Data(code)[-3][4])*2,alpha[-2][index])

    print "(high + low)"
    alpha, codes = af.cal_alpha("(high + low)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]+dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]+dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "add_from:(1 + low)"
    alpha, codes = af.cal_alpha("(1 + low)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((1+dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((1+dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "add_to:(high + 1)"
    alpha, codes = af.cal_alpha("(high + 1)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]+1),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]+1),alpha[-2][index])

    print "reduce:(high - low)"
    alpha, codes = af.cal_alpha("(high - low)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]-dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]-dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "reduce_from:(1000 - low)"
    alpha, codes = af.cal_alpha("(1000 - low)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((1000-dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((1000-dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "reduce_to:(high - 1)"
    alpha, codes = af.cal_alpha("(high - 1)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]-1),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]-1),alpha[-2][index])

    print "(high * low)"
    alpha, codes = af.cal_alpha("(high * low)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]*dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]*dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "mul_from:(2 * low)"
    alpha, codes = af.cal_alpha("(2 * low)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((2*dataProxy.get_all_Data(code)[-1][3]),alpha[-1][index])
        float_equal((2*dataProxy.get_all_Data(code)[-2][3]),alpha[-2][index])

    print "mul_to:(high * 3)"
    alpha, codes = af.cal_alpha("(high * 3)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]*3),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]*3),alpha[-2][index])

    print "mid(high, low)"
    alpha, codes = af.cal_alpha("mid(high, low)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][2]+dataProxy.get_all_Data(code)[-1][3])*0.5,alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][2]+dataProxy.get_all_Data(code)[-2][3])*0.5,alpha[-2][index])

    print "stddev(open, 2)"
    alpha, codes = af.cal_alpha("stddev(open, 2)", 0, 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        res = np.array([dataProxy.get_all_Data(code)[-3][1],dataProxy.get_all_Data(code)[-2][1],dataProxy.get_all_Data(code)[-1][1]])
        float_equal(res.std(),alpha[-1][index])
        res = np.array([dataProxy.get_all_Data(code)[-4][1], dataProxy.get_all_Data(code)[-3][1],
                        dataProxy.get_all_Data(code)[-2][1]])
        float_equal(res.std(), alpha[-2][index])

    print "up(close, 2)"
    alpha, codes = af.cal_alpha("up(close, 2)", 0, 1)
    for index,code in enumerate(codes):
        res = np.array([dataProxy.get_all_Data(code)[-3][4],dataProxy.get_all_Data(code)[-2][4],dataProxy.get_all_Data(code)[-1][4]])
        float_equal(res.std() + res.mean(),alpha[-1][index])

    print "down(returns, 2)"
    alpha, codes = af.cal_alpha("down(returns, 2)", 0, 1)
    for index,code in enumerate(codes):
        res = np.array([dataProxy.get_all_Data(code)[-3][7],dataProxy.get_all_Data(code)[-2][7],dataProxy.get_all_Data(code)[-1][7]])
        float_equal(res.mean() - res.std(),alpha[-1][index])

    print "rank(open)"
    alpha, codes = af.cal_alpha("rank(open)", 0, 1)
    v_open = np.array([dataProxy.get_all_Data(code)[-1][1] for code in codes])
    r_open = rank(v_open)
    for i in xrange(len(v_open)):
        float_equal(r_open[i], alpha[-1][i])

    print "power_mid(delay(open, 25), high)"
    alpha, codes = af.cal_alpha("power_mid(delay(open, 25), high)", 0, 2)
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
    alpha, codes = af.cal_alpha("(returns ^ 2)", 0, 1)
    for index, code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][7] * dataProxy.get_all_Data(code)[-1][7]),
                    alpha[-1][index])

    print "ts_rank(returns, 5)"
    alpha, codes = af.cal_alpha("ts_rank(returns, 5)", 0, 2)
    for index, code in enumerate(codes):
        returns = np.array([dataProxy.get_all_Data(code)[i-6][7] for i in xrange(6)])
        float_equal(ts_rank(returns, 5)[-1], alpha[-1][index])
        returns = np.array([dataProxy.get_all_Data(code)[i - 6-1][7] for i in xrange(6)])
        float_equal(ts_rank(returns, 5)[-1], alpha[-2][index])

    print "delay(open, 5)"
    alpha, codes = af.cal_alpha("delay(open, 5)", 0, 2)
    for index,code in enumerate(codes):
        #往后推1天就是-2，所以是-6
        float_equal(dataProxy.get_all_Data(code)[-6][1],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-7][1],alpha[-2][index])

    print "future"
    alphatree_id = af.create_alphatree("delay(open, 1)")
    future_alphatree_id = af.create_alphatree("future(open, 2)")
    history_num = af.get_history_num(alphatree_id)
    watch_future_num = af.get_future_num(future_alphatree_id)
    print history_num
    print watch_future_num
    codes = af.get_codes(0,watch_future_num,history_num,2)
    alpha = af.cal_stock_alpha(alphatree_id, codes, 2, 2)
    af.release_alphatree(alphatree_id)

    future_alpha = af.cal_stock_alpha(future_alphatree_id, codes, 0, 2)
    af.release_alphatree(alphatree_id)
    af.release_alphatree(future_alphatree_id)
    for index, code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][1], future_alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][1], future_alpha[-2][index])
        float_equal(dataProxy.get_all_Data(code)[-4][1], alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-5][1], alpha[-2][index])

    print "correlation(delay(returns, 3), returns, 10)"
    alpha, codes = af.cal_alpha("correlation(delay(returns, 3), returns, 10)", 0, 2)
    for index, code in enumerate(codes):
        returns = np.array([dataProxy.get_all_Data(code)[i - 11][7] for i in xrange(11)])
        d_returns = np.array([dataProxy.get_all_Data(code)[i - 14][7] for i in xrange(11)])
        v1 = pearson_def(returns, d_returns)*10
        v2 = alpha[-1][index]*10
        float_equal(v1, v2)

        returns = np.array([dataProxy.get_all_Data(code)[i - 12][7] for i in xrange(11)])
        d_returns = np.array([dataProxy.get_all_Data(code)[i - 15][7] for i in xrange(11)])
        v1 = pearson_def(returns, d_returns)*10
        v2 = alpha[-2][index]*10
        float_equal(v1, v2)


    print "scale(delay(returns, 1))"
    alpha, codes = af.cal_alpha("scale(delay(returns, 1))", 0, 2)
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
    alpha, codes = af.cal_alpha("decay_linear(high, 2)", 0, 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(dataProxy.get_all_Data(code)[-1][2]*3+dataProxy.get_all_Data(code)[-2][2]*2+dataProxy.get_all_Data(code)[-3][2],alpha[-1][index] * 6)
        float_equal(dataProxy.get_all_Data(code)[-2][2]*3+dataProxy.get_all_Data(code)[-3][2]*2+dataProxy.get_all_Data(code)[-4][2],alpha[-2][index] * 6)

    print "ts_min(high, 2)"
    alpha, codes = af.cal_alpha("ts_min(high, 2)", 0, 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(min([dataProxy.get_all_Data(code)[-1][2],dataProxy.get_all_Data(code)[-2][2],dataProxy.get_all_Data(code)[-3][2]]),alpha[-1][index])
        float_equal(min([dataProxy.get_all_Data(code)[-2][2],dataProxy.get_all_Data(code)[-3][2],dataProxy.get_all_Data(code)[-4][2]]),alpha[-2][index])

    print "ts_max(high, 2)"
    alpha, codes = af.cal_alpha("ts_max(high, 2)", 0, 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(max([dataProxy.get_all_Data(code)[-1][2],dataProxy.get_all_Data(code)[-2][2],dataProxy.get_all_Data(code)[-3][2]]),alpha[-1][index])
        float_equal(max([dataProxy.get_all_Data(code)[-2][2],dataProxy.get_all_Data(code)[-3][2],dataProxy.get_all_Data(code)[-4][2]]),alpha[-2][index])

    print "min(delay(before_high, 1), before_high)"
    sub_dict = {"before_high":"delay(high, 2)"}
    alpha, codes = af.cal_alpha("min(delay(before_high, 1), before_high)", 0, 2, sub_dict)
    # 今天、昨天、前天数据一共3天
    for index, code in enumerate(codes):
        float_equal(min([dataProxy.get_all_Data(code)[-4][2], dataProxy.get_all_Data(code)[-3][2]]), alpha[-1][index])
        float_equal(min([dataProxy.get_all_Data(code)[-5][2], dataProxy.get_all_Data(code)[-4][2]]), alpha[-2][index])

    print "max(high, before_high)"
    sub_dict = {"before_high": "delay(high, 50)"}
    alpha, codes = af.cal_alpha("max(high, before_high)", 0, 2,sub_dict)
    # 今天、昨天、前天数据一共3天
    for index, code in enumerate(codes):
        float_equal(max([dataProxy.get_all_Data(code)[-1][2], dataProxy.get_all_Data(code)[-51][2]]), alpha[-1][index])
        float_equal(max([dataProxy.get_all_Data(code)[-2][2], dataProxy.get_all_Data(code)[-52][2]]), alpha[-2][index])

    print "ts_argmin(volume, 2)"
    alpha, codes = af.cal_alpha("ts_argmin(volume, 2)", 0, 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(np.argmin(np.array([dataProxy.get_all_Data(code)[-3][5],dataProxy.get_all_Data(code)[-2][5],dataProxy.get_all_Data(code)[-1][5]])),alpha[-1][index])
        float_equal(np.argmin(np.array([dataProxy.get_all_Data(code)[-4][5],dataProxy.get_all_Data(code)[-3][5],dataProxy.get_all_Data(code)[-2][5]])),alpha[-2][index])

    print "ts_argmax(volume, 2)"
    alpha, codes = af.cal_alpha("ts_argmax(volume, 2)", 0, 2)
    #今天、昨天、前天数据一共3天
    for index,code in enumerate(codes):
        float_equal(np.argmax(np.array([dataProxy.get_all_Data(code)[-3][5],dataProxy.get_all_Data(code)[-2][5],dataProxy.get_all_Data(code)[-1][5]])),alpha[-1][index])
        float_equal(np.argmax(np.array([dataProxy.get_all_Data(code)[-4][5],dataProxy.get_all_Data(code)[-3][5],dataProxy.get_all_Data(code)[-2][5]])),alpha[-2][index])

    print "sign(returns)"
    alpha, codes = af.cal_alpha("sign(returns)", 0, 2)
    for index,code in enumerate(codes):
        v = dataProxy.get_all_Data(code)[-1][7]
        float_equal(1 if v > 0 else (-1 if v < 0 else 0),alpha[-1][index])
        v = dataProxy.get_all_Data(code)[-2][7]
        float_equal(1 if v > 0 else (-1 if v < 0 else 0),alpha[-2][index])

    print "abs(returns)"
    alpha, codes = af.cal_alpha("abs(returns)", 0, 2)
    for index,code in enumerate(codes):
        float_equal(abs(dataProxy.get_all_Data(code)[-1][7]),alpha[-1][index])
        float_equal(abs(dataProxy.get_all_Data(code)[-2][7]),alpha[-2][index])

    print "(open ^ returns)"
    alpha, codes = af.cal_alpha("(open ^ returns)", 0, 2)
    for index,code in enumerate(codes):
        float_equal(math.pow(dataProxy.get_all_Data(code)[-1][1], dataProxy.get_all_Data(code)[-1][7]),alpha[-1][index])
        float_equal(math.pow(dataProxy.get_all_Data(code)[-2][1], dataProxy.get_all_Data(code)[-2][7]),alpha[-2][index])

    print "less:(delay(returns, 1) < returns)"
    alpha, codes = af.cal_alpha("(delay(returns, 1) < returns)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((1 if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else 0),alpha[-1][index])
        float_equal((1 if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else 0),alpha[-2][index])

    print "less_to:(returns < 0)"
    alpha, codes = af.cal_alpha("(returns < 0)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((1 if dataProxy.get_all_Data(code)[-1][7] < 0 else 0),alpha[-1][index])
        float_equal((1 if dataProxy.get_all_Data(code)[-2][7] < 0 else 0),alpha[-2][index])

    print "less_from:(0 < returns)"
    alpha, codes = af.cal_alpha("(0 < returns)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((1 if 0 < dataProxy.get_all_Data(code)[-1][7] else 0),alpha[-1][index])
        float_equal((1 if 0 < dataProxy.get_all_Data(code)[-2][7] else 0),alpha[-2][index])

    print "((delay(returns, 1) < returns) ? open : close)"
    alpha, codes = af.cal_alpha("((delay(returns, 1) < returns) ? open : close)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][1] if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else dataProxy.get_all_Data(code)[-1][4]),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][1] if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else dataProxy.get_all_Data(code)[-2][4]),alpha[-2][index])

    print "if_to:((delay(returns, 1) < returns) ? 1 : close)"
    alpha, codes = af.cal_alpha("((delay(returns, 1) < returns) ? 1 : close)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((1 if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else dataProxy.get_all_Data(code)[-1][4]),alpha[-1][index])
        float_equal((1 if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else dataProxy.get_all_Data(code)[-2][4]),alpha[-2][index])

    print "else_to:((delay(returns, 1) < returns) ? open : 1)"
    alpha, codes = af.cal_alpha("((delay(returns, 1) < returns) ? open : 1)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((dataProxy.get_all_Data(code)[-1][1] if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else 1),alpha[-1][index])
        float_equal((dataProxy.get_all_Data(code)[-2][1] if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else 1),alpha[-2][index])

    print "if_to_else_to:((delay(returns, 1) < returns) ? 1 : -1)"
    alpha, codes = af.cal_alpha("((delay(returns, 1) < returns) ? 1 : -1)", 0, 2)
    for index,code in enumerate(codes):
        float_equal((1 if dataProxy.get_all_Data(code)[-2][7] < dataProxy.get_all_Data(code)[-1][7] else -1),alpha[-1][index])
        float_equal((1 if dataProxy.get_all_Data(code)[-3][7] < dataProxy.get_all_Data(code)[-2][7] else -1),alpha[-2][index])

    # print "up_mean"
    # alpha, codes = af.cal_alpha("up_mean(volume, 1)", 0, 2)
    # for index,code in enumerate(codes):
    #     float_equal(1 if dataProxy.get_all_Data(code)[-1][5] >= (dataProxy.get_all_Data(code)[-1][5]+dataProxy.get_all_Data(code)[-2][5])/2 else 0,alpha[-1][index])
    #     float_equal(1 if dataProxy.get_all_Data(code)[-2][5] >= (dataProxy.get_all_Data(code)[-2][5]+dataProxy.get_all_Data(code)[-3][5])/2 else 0,alpha[-2][index])

    print "indneutralize:(returns - indneutralize(returns, IndClass.market))"
    alpha, codes = af.cal_alpha("(returns - indneutralize(returns, IndClass.market))", 0, 2)
    for index,code in enumerate(codes):
        if code[0] == '0':
            market = '0000001'
        else:
            market = '1399001'
        float_equal(dataProxy.get_all_Data(code)[-1][7] - dataProxy.get_all_Data(market)[-1][7],alpha[-1][index])
        float_equal(dataProxy.get_all_Data(code)[-2][7] - dataProxy.get_all_Data(market)[-2][7],alpha[-2][index])

    print "---------------------------------"
    for s in sub_alphatree:
        print s
    print "finish"