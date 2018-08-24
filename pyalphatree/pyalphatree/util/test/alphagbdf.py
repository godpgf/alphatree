#coding=utf-8
#author=godpgf
from ctypes import *
from pyalphatree.libalphatree import alphatree
import numpy as np


class AlphaGBDF(object):
    def __init__(self, alphatree_list, forest_size, gamma_value = 0.02, lambda_value = 0.02, thread_num = 4, loss_fun_name = "binary:logistic"):
        alphatree_char_num = 0
        for at in alphatree_list:
            alphatree_char_num += len(at) + 1

        alphatree_cache = (c_char * alphatree_char_num)()
        cur_alpha_index = 0
        for at in alphatree_list:
            code_list = list(at)
            for c in code_list:
                alphatree_cache[cur_alpha_index] = c
                cur_alpha_index += 1
            alphatree_cache[cur_alpha_index] = '\0'
            cur_alpha_index += 1

        alphatree.initializeAlphaGBDF(alphatree_cache, len(alphatree_list), forest_size, c_float(gamma_value), c_float(lambda_value), thread_num, c_char_p(loss_fun_name))

    def __del__(self):
        pass
        #alphatree.releaseAlphaGBDT()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        alphatree.releaseAlphaGBDT()

    def testRead(self, daybefore, sample_size, feature_num, target, sign_name, cache_size = 2048):
        sign_num = alphatree.getSignNum(daybefore, sample_size, c_char_p(sign_name))
        target_cache = (c_float * sign_num)()
        feature_cache = (c_float * (sign_num * feature_num))()
        data_num = alphatree.testReadGBDF(daybefore, sample_size, c_char_p(target), c_char_p(sign_name), feature_cache, target_cache, cache_size)
        assert data_num == sign_num
        t = [target_cache[i] for i in range(sign_num)]
        f = []
        for i in range(sign_num):
            for j in range(feature_num):
                f.append(feature_cache[i * feature_num + j])
        #f = [[feature_cache[i * feature_num + j] for j in range(feature_num)] for i in range(sign_num)]
        return np.array(f), np.array(t)

    def train(self, daybefore, sample_size, weight, target, sign_name, sample_percent, feature_percent, bar_size = 32, min_weight = 64.0, max_depth = 8, boost_num = 2, boost_weight_scale = 1, cache_size = 2048):
        alphatree.trainAlphaGBDF(daybefore, sample_size, c_char_p(weight) if weight else None, c_char_p(target), c_char_p(sign_name), c_float(sample_percent), c_float(feature_percent), bar_size, c_float(min_weight), max_depth, boost_num, c_float(boost_weight_scale), cache_size)

    def pred(self, daybefore, sample_size, sign_name, cache_size = 1024):
        sign_num = alphatree.getSignNum(daybefore, sample_size, c_char_p(sign_name))
        alpha_cache = (c_float * sign_num)()
        alphatree.predAlphaGBDF(daybefore, sample_size, c_char_p(sign_name), alpha_cache, cache_size)
        return np.array([alpha_cache[i] for i in range(sign_num)])

    def save_model(self, path):
        alphatree.saveGBDFModel(c_char_p(path))

    def load_model(self, path):
        alphatree.loadGBDFModel(c_char_p(path))

    def __str__(self):
        alphatree_cache = (c_char * 131072)()
        char_num = alphatree.alphaGBDF2String(alphatree_cache)
        return "".join([alphatree_cache[i] for i in range(char_num)])

    def __repr__(self):
        return str(self)