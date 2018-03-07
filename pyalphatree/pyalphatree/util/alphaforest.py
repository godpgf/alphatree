# coding=utf-8
# author=godpgf
from ctypes import *
from pyalphatree.libalphatree import alphatree
from stdb import *
import numpy as np
import math
import json


class AlphaForest(object):
    def __init__(self, cache_size=4, max_stock_num=4096, max_day_num=4096, max_feature_size=2048):
        alphatree.initializeAlphaforest(cache_size)

        self.code_cache = (c_char * (max_stock_num * 64))()
        self.alpha_cache = (c_float * (max_stock_num * max_day_num))()
        self.feature_cache = (c_char * (max_feature_size * 64))()

        self.max_alpha_tree_str_len = 4096;
        self.sub_alphatree_str_cache = (c_char * (self.max_alpha_tree_str_len * 1024))()
        self.alphatree_id_cache = (c_int * 1024)()

        self.encode_cache = (c_char * self.max_alpha_tree_str_len)()
        self.process_cache = (c_char * (4096 * 64))()


    def __del__(self):
        alphatree.releaseAlphaforest()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def create_alphatree(self):
        return alphatree.createAlphatree()

    def release_alphatree(self, alphatree_id):
        alphatree.releaseAlphatree(alphatree_id)

    def use_cache(self):
        return alphatree.useCache()

    def release_cache(self, cache_id):
        alphatree.releaseCache(cache_id)

    def encode_alphatree(self, alphatree_id, root_name):
        str_len = alphatree.encodeAlphatree(alphatree_id, c_char_p(root_name), self.encode_cache)
        str_list = [self.encode_cache[i] for i in xrange(str_len)]
        return "".join(str_list)

    def decode_alphatree(self, alphatree_id, root_name, line):
        alphatree.decodeAlphatree(alphatree_id, c_char_p(root_name), c_char_p(line))

    def get_max_history_days(self, alphatree_id):
        return alphatree.getMaxHistoryDays(alphatree_id)

    def get_sign_num(self, day_before, sample_days, sign_name):
        return alphatree.getSignNum(day_before, sample_days, c_char_p(sign_name))

    #将某个公式的计算结果保持在文件
    def cache_alpha(self, name, line):
        alphatree_id = self.create_alphatree()
        cache_id = self.use_cache()
        self.decode_alphatree(alphatree_id, name, line)
        alphatree.cacheAlpha(alphatree_id, cache_id, c_char_p(name))
        self.release_cache(cache_id)
        self.release_alphatree(alphatree_id)

    #将某个信号保存在文件
    def cache_sign(self, name, line):
        alphatree_id = self.create_alphatree()
        cache_id = self.use_cache()
        self.decode_alphatree(alphatree_id, name, line)
        alphatree.cacheSign(alphatree_id, cache_id, c_char_p(name))
        self.release_cache(cache_id)
        self.release_alphatree(alphatree_id)

    # 读取数据
    def load_db(self, path):
        alphatree.loadDataBase(c_char_p(path))

    def csv2binary(self, path, feature_name):
        alphatree.csv2binary(c_char_p(path), c_char_p(feature_name))

    def cache_feature(self, feature_name):
        alphatree.cacheFeature(c_char_p(feature_name))


    def get_stock_codes(self):
        stock_size = alphatree.getStockCodes(self.code_cache)
        return np.array(self._read_str_list(self.code_cache, stock_size))


    def cal_alpha(self, alphatree_id, cache_id, daybefore, sample_size, codes):
        stock_size = len(codes)
        # self.check_alpha_size(sample_size, stock_size)
        self._write_codes(codes)
        alphatree.calAlpha(alphatree_id, cache_id, daybefore, sample_size, self.code_cache, stock_size)

    def cal_sign_alpha(self, alphatree_id, cache_id, daybefore, sample_size, sign_history_days, sign_name):
        alphatree.calSignAlpha(alphatree_id, cache_id, daybefore, sample_size, sign_history_days, c_char_p(sign_name))


    def optimize_alpha(self, alphatree_id, cache_id, root_name, daybefore, sample_size, codes, exploteRatio = 0.1, errTryTime = 64):
        stock_size = len(codes)
        self._write_codes(codes)
        return alphatree.optimizeAlpha(alphatree_id, cache_id, c_char_p(root_name), daybefore, sample_size, self.code_cache, stock_size, c_float(exploteRatio), errTryTime)

    def get_root_alpha(self, alphatree_id, root_name, cache_id, sample_size):
        data_size = alphatree.getAlpha(alphatree_id, c_char_p(root_name), cache_id, self.alpha_cache)
        stock_size = data_size / sample_size
        return self._read_alpha(sample_size, stock_size)

    def get_node_alpha(self, alphatree_id, node_id, cache_id, sample_size):
        data_size = alphatree.getNodeAlpha(alphatree_id, node_id, cache_id, sample_size)
        stock_size = data_size / sample_size
        return self._read_alpha(sample_size, stock_size)

    #def process_alpha(self, alphatree_id, cache_id):
    #    alphatree.processAlpha(alphatree_id, cache_id)

    def get_process(self, alphatree_id, process_name, cache_id):
        alphatree.getRootProcess(alphatree_id, c_char_p(process_name), cache_id, self.process_cache)
        return self._read_str(self.process_cache)

    def _read_alpha(self, sample_size, stock_size):
        alpha_list = list()
        cur_alpha_index = 0
        for i in xrange(sample_size):
            alpha = list()
            for j in xrange(stock_size):
                alpha.append(self.alpha_cache[cur_alpha_index])
                cur_alpha_index += 1
            alpha_list.append(alpha)
        return alpha_list

    @classmethod
    def _write_code(cls, code, code_cache, cur_index):
        code_list = list(code)
        for c in code_list:
            code_cache[cur_index] = c
            cur_index += 1
        code_cache[cur_index] = '\0'
        cur_index += 1;
        return cur_index

    @classmethod
    def _read_str_list(clf, str_cache, str_num):
        cur_index = 0
        str_list = list()
        for i in xrange(str_num):
            code = list()
            while str_cache[cur_index] != '\0':
                code.append(str_cache[cur_index])
                cur_index += 1
            cur_index += 1
            str_list.append("".join(code))
        return str_list;


    @classmethod
    def _read_str(cls, str_cache):
        code = list()
        cur_index = 0
        while str_cache[cur_index] != '\0':
            code.append(str_cache[cur_index])
            cur_index += 1
        return "".join(code)

    @classmethod
    def _read_str(cls, str_cache):
        cur_index = 0
        str = list()
        while str_cache[cur_index] != '\0':
            str.append(str_cache[cur_index])
            cur_index += 1
        return "".join(str)

    def _read_codes(self, stock_size):
        return self._read_str_list(self.code_cache, stock_size)

    def _write_codes(self, codes):
        cur_code_index = 0
        for code in codes:
            code_list = list(code)
            for c in code_list:
                self.code_cache[cur_code_index] = c
                cur_code_index += 1
            self.code_cache[cur_code_index] = '\0'
            cur_code_index += 1

    def _write_features(self, features):
        cur_feature_index = 0
        for feature in features:
            f_list = list(feature)
            for c in f_list:
                self.feature_cache[cur_feature_index] = c
                cur_feature_index += 1
            self.feature_cache[cur_feature_index] = '\0'
            cur_feature_index += 1



