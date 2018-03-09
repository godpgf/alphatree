#coding=utf-8
#author=godpgf
from pyalphatree import *

def get_min_wave():
    return 0.0004

def get_stop_loss():
    return 0.002

def get_sign_des():
    return "((product(volume, 6) > 0) & amplitude_sample(returns, %.4f))"%get_min_wave()


def get_feature_des():
    formula_list = [
        "close_ = close",
        "close_t = log((close_ / delay(close_, 1)))",
        "high_low_t = (log((high / close_)) - log((low / close_)))",
        "volume_t = volume",
        "askprice_ = log((askprice / close_))",
        "bidprice_ = log((bidprice / close_))",
        "delta_price_t = (bidprice_ - askprice_)",
        "avg_price_t = ((bidprice_ + askprice_) * 0.5)",
        "askvolume_ = askvolume",
        "bidvolume_ = bidvolume",
        "volume_ratio_t = log((askvolume_ / bidvolume_))",
        "volume_depth_t = (askvolume_ + bidvolume_)"
    ]
    column_list = []
    for i in xrange(1, 6):
        column_list.append("close_%d" % i)
        formula_list.append("%s = delay(close_t, %d)" % (column_list[-1], i))
        column_list.append("high_low_%d" % i)
        formula_list.append("%s = delay(high_low_t, %d)" % (column_list[-1], i))
        column_list.append("volume_%d" % i)
        formula_list.append("%s = delay(volume_t, %d)" % (column_list[-1], i))
        column_list.append("delta_price_%d" % i)
        formula_list.append("%s = delay(delta_price_t, %d)" % (column_list[-1], i))
        column_list.append("avg_price_%d" % i)
        formula_list.append("%s = delay(avg_price_t, %d)" % (column_list[-1], i))
        column_list.append("volume_ratio_%d" % i)
        formula_list.append("%s = delay(volume_ratio_t, %d)" % (column_list[-1], i))
        column_list.append("volume_depth_%d" % i)
        formula_list.append("%s = delay(volume_depth_t, %d)" % (column_list[-1], i))
    return formula_list, column_list


def get_target_des():
    formula_list = [
        "return_ = returns",
        "t1 = ((return_ < -%.4f) ? 1 : 0)"%get_min_wave(),
        "t3 = ((return_ > %.4f) ? 1 : 0)"%get_min_wave(),
        "t2 = ((1 - t1) & (1 - t3))"
    ]
    column_list = ["t1", "t2", "t3"]
    return formula_list, column_list


def get_sign_data(sign_name, day_before, sample_num):
    formula_list_f, column_list_f = get_feature_des()
    formula_list_t, column_list_t = get_target_des()
    return AlphaArray(sign_name, formula_list_f, column_list_f, day_before, sample_num, True), \
           AlphaArray(sign_name, formula_list_t, column_list_t, day_before, sample_num, True)


def get_seq_data(code_name, day_before, sample_num):
    formula_list_f, column_list_f = get_feature_des()
    return AlphaArray(code_name, formula_list_f, column_list_f, day_before, sample_num, False), \
           AlphaArray(code_name, ["close_ = delay(close, 1)", "returns_ = returns"], ["close_", "returns_"], day_before, sample_num, False)

