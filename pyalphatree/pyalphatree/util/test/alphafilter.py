# coding=utf-8
# author=godpgf
# 对alpha做初步筛选，保留好的并且不怎么相关的alpha
from ctypes import *
from pyalphatree.libalphatree import alphatree
import numpy as np


class AlphaFilter(object):
    def __init__(self, alphatree_list, target, opentree = None, alpha_tree_flag = None):
        self.is_open_buy = False if opentree is None else True
        alphatree_char_num = 0
        for at in alphatree_list:
            alphatree_char_num += len(at) + 1

        if alpha_tree_flag is None:
            alpha_tree_flag = [3] * len(alphatree_list)

        alphatree_cache = (c_char * alphatree_char_num)()
        cur_alpha_index = 0
        for at in alphatree_list:
            code_list = list(at.encode('utf-8'))
            for c in code_list:
                alphatree_cache[cur_alpha_index] = c
                cur_alpha_index += 1
            alphatree_cache[cur_alpha_index] = b'\0'
            cur_alpha_index += 1
        self.alphatree_num = len(alphatree_list)
        flag_cache = (c_int32 * len(alphatree_list))()
        for id, f in enumerate(alpha_tree_flag):
            flag_cache[id] = f
        alphatree.initializeAlphaFilter(alphatree_cache, flag_cache, len(alphatree_list), c_char_p(target.encode()), None if opentree is None else c_char_p(opentree.encode()))

    def __del__(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        alphatree.releaseAlphaFilter()

    def train(self, sign_name, daybefore, sample_size, sample_time, support, confidence, first_hero_confidence, second_hero_confidence):
        alphatree.trainAlphaFilter(c_char_p(sign_name.encode()), daybefore, sample_size, sample_time, c_float(support), c_float(confidence), c_float(first_hero_confidence), c_float(second_hero_confidence))

    def pred(self, sign_name, daybefore, sample_size):
        sign_num = alphatree.getSignNum(daybefore, sample_size, c_char_p(sign_name.encode()))
        alpha_cache = (c_float * sign_num)()
        min_open_value_cache = (c_float * sign_num)() if self.is_open_buy else None
        max_open_value_cache = (c_float * sign_num)() if self.is_open_buy else None
        alphatree.predAlphaFilter(c_char_p(sign_name.encode()), daybefore, sample_size, alpha_cache, min_open_value_cache, max_open_value_cache)
        #res = np.array([alpha_cache[i] for i in range(sign_num)])
        if self.is_open_buy:
            return np.array(alpha_cache), np.array(min_open_value_cache), np.array(max_open_value_cache)
        return np.array(alpha_cache)

    def save_model(self, path):
        alphatree.saveFilterModel(c_char_p(path.encode()))

    def load_model(self, path):
        alphatree.loadFilterModel(c_char_p(path.encode()))

    def __str__(self):
        alphatree_cache = (c_char * 131072)()
        char_num = alphatree.alphaFilter2String(alphatree_cache)
        return "".join([alphatree_cache[i] for i in range(char_num)])

    def __repr__(self):
        return str(self)
