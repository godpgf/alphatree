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
        alphatree.initializeAlphaGraph()

        self.code_cache = (c_char * (max_stock_num * 64))()
        self.cur_stock_size = 0
        self.alpha_cache = (c_float * (max_stock_num * max_day_num))()
        self.feature_cache = (c_char * (max_feature_size * 64))()

        self.max_alpha_tree_str_len = 4096;
        self.sub_alphatree_str_cache = (c_char * (self.max_alpha_tree_str_len * 1024))()
        self.alphatree_id_cache = (c_int * max_stock_num)()

        self.encode_cache = (c_char * self.max_alpha_tree_str_len)()
        self.process_cache = (c_char * (4096 * 64))()


    def __del__(self):
        pass
        #alphatree.releaseAlphaforest()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        alphatree.releaseAlphaGraph()
        alphatree.releaseAlphaforest()


    # 读取数据
    def load_db(self, path):
        alphatree.loadDataBase(c_char_p(path.encode('utf-8')))

    def csv2binary(self, path, feature_name):
        alphatree.csv2binary(c_char_p(path.encode('utf-8')), c_char_p(feature_name.encode('utf-8')))

    #缓存缺失数据的描述，以便快速知道数据的缺失情况
    def cache_miss(self):
        alphatree.cacheMiss()

    #将某个公式的计算结果保持在文件
    def cache_alpha(self, name, line):
        alphatree_id = self.create_alphatree()
        cache_id = self.use_cache()
        self.decode_alphatree(alphatree_id, name, line)
        alphatree.cacheAlpha(alphatree_id, cache_id, c_char_p(name.encode('utf-8')))
        self.release_cache(cache_id)
        self.release_alphatree(alphatree_id)

    def cache_alpha_list(self, formula_list, name_list):
        alphatree_id = self.create_alphatree()
        cache_id = self.use_cache()
        for formula in formula_list:
            tmp = formula.split('=')
            alphatree.decodeAlphatree(alphatree_id, c_char_p(tmp[0].strip().encode('utf-8')), c_char_p(("" if len(tmp) == 1 else tmp[1].strip()).encode('utf-8')))
        for name in name_list:
            alphatree.cacheAlpha(alphatree_id, cache_id, c_char_p(name.encode('utf-8')))
        self.release_cache(cache_id)
        self.release_alphatree(alphatree_id)

    #将某个信号保存在文件
    def cache_sign(self, name, line):
        alphatree_id = self.create_alphatree()
        cache_id = self.use_cache()
        self.decode_alphatree(alphatree_id, name, line)
        alphatree.cacheSign(alphatree_id, cache_id, c_char_p(name.encode('utf-8')))
        self.release_cache(cache_id)
        self.release_alphatree(alphatree_id)

    #仅仅保存某些股票的信号
    def cache_codes_sign(self, name, line, codes):
        stock_size = len(codes)
        self._write_codes(codes)
        alphatree_id = self.create_alphatree()
        cache_id = self.use_cache()
        self.decode_alphatree(alphatree_id, name, line)
        alphatree.cacheCodesSign(alphatree_id, cache_id, c_char_p(name.encode('utf-8')), self.code_cache, stock_size)
        self.release_cache(cache_id)
        self.release_alphatree(alphatree_id)

    def load_feature(self, feature_name):
        alphatree.loadFeature(c_char_p(feature_name.encode('utf-8')))

    def release_feature(self, feature_name):
        alphatree.releaseFeature(c_char_p(feature_name.encode('utf-8')))

    def update_feature(self, feature_name):
        alphatree.updateFeature(c_char_p(feature_name.encode('utf-8')))

    def release_all_feature(self):
        alphatree.releaseAllFeature()

    def load_sign(self, sign_name):
        alphatree.loadSign(c_char_p(sign_name.encode('utf-8')))

    def release_all_sign(self):
        alphatree.releaseAllSign()

    def get_stock_codes(self):
        stock_size = alphatree.getStockCodes(self.code_cache)
        return np.array(self._read_str_list(self.code_cache, stock_size))

    def get_market_codes(self):
        stock_size = alphatree.getMarketCodes(None, self.code_cache)
        return np.array(self._read_str_list(self.code_cache, stock_size))

    def get_stock_ids(self, daybefore, sample_size, sign_name):
        stock_num = alphatree.getStockIds(daybefore, sample_size, c_char_p(sign_name.encode('utf-8')), self.alphatree_id_cache)
        return np.array([self.alphatree_id_cache[i] for i in range(stock_num)])

    def get_code(self, stock_id):
        l = alphatree.getCode(c_int32(stock_id), self.code_cache)
        return ''.join([self.code_cache[i].decode() for i in range(l)])

    def fill_codes(self, codes):
        self.cur_stock_size = len(codes)
        self._write_codes(codes)

    def process_alpha(self, line, daybefore, sample_size, sign_name):
        alphatree_id = alphatree.createAlphatree()
        alphatree.decodeAlphatree(alphatree_id, c_char_p(b"_process"), c_char_p(line.encode('utf-8')))
        cache_id = alphatree.useCache()
        alphatree.calSignAlpha(alphatree_id, cache_id, daybefore, sample_size, 0, self.get_sign_num(daybefore, sample_size, sign_name), 1, c_char_p(sign_name.encode('utf-8')))
        alphatree.getAlpha(alphatree_id, c_char_p(b"_process"), cache_id, self.alpha_cache)
        alphatree.releaseAlphatree(alphatree_id)
        alphatree.releaseCache(cache_id)
        return self.alpha_cache[0]

    def create_alphatree(self):
        return alphatree.createAlphatree()

    def release_alphatree(self, alphatree_id):
        alphatree.releaseAlphatree(alphatree_id)

    def use_cache(self):
        return alphatree.useCache()

    def release_cache(self, cache_id):
        alphatree.releaseCache(cache_id)

    def encode_alphatree(self, alphatree_id, root_name):
        str_len = alphatree.encodeAlphatree(alphatree_id, c_char_p(root_name.encode('utf-8')), self.encode_cache)
        str_list = [self.encode_cache[i] for i in range(str_len)]
        return "".join(str_list)

    def decode_alphatree(self, alphatree_id, root_name, line):
        alphatree.decodeAlphatree(alphatree_id, c_char_p(root_name.encode('utf-8')), c_char_p(line.encode('utf-8')))

    def get_max_history_days(self, line):
        alphatree_id = self.create_alphatree()
        self.decode_alphatree(alphatree_id,"t",line)
        days = alphatree.getMaxHistoryDays(alphatree_id)
        self.release_alphatree(alphatree_id)
        return days

    def get_all_days(self):
        return alphatree.getAllDays()

    def get_sign_num(self, day_before, sample_days, sign_name):
        return alphatree.getSignNum(day_before, sample_days, c_char_p(sign_name.encode('utf-8')))


    def cache_bool_hmm(self, feature_name, codes, hide_state_num = 3, seq_len = 750, epoch_num = 64):
        stock_size = len(codes)
        self._write_codes(codes)
        alphatree.cacheBoolHMM(c_char_p(feature_name.encode('utf-8')), hide_state_num, seq_len, self.code_cache, stock_size, epoch_num)


    def cal_alpha(self, alphatree_id, cache_id, daybefore, sample_size, codes):
        stock_size = len(codes)
        # self.check_alpha_size(sample_size, stock_size)
        self._write_codes(codes)
        alphatree.calAlpha(alphatree_id, cache_id, daybefore, sample_size, self.code_cache, stock_size)

    def cal_sign_alpha(self, alphatree_id, cache_id, daybefore, sample_size, sign_history_days, sign_name):
        alphatree.calSignAlpha(alphatree_id, cache_id, daybefore, sample_size, sign_history_days, c_char_p(sign_name.encode('utf-8')))


    # def optimize_alpha(self, alphatree_id, cache_id, root_name, daybefore, sample_size, codes, exploteRatio = 0.1, errTryTime = 64):
    #     stock_size = len(codes)
    #     self._write_codes(codes)
    #     return alphatree.optimizeAlpha(alphatree_id, cache_id, c_char_p(root_name.encode('utf-8')), daybefore, sample_size, self.code_cache, stock_size, c_float(exploteRatio), errTryTime)

    def get_root_alpha(self, alphatree_id, root_name, cache_id, sample_size):
        data_size = alphatree.getAlpha(alphatree_id, c_char_p(root_name.encode('utf-8')), cache_id, self.alpha_cache)
        stock_size = data_size / sample_size
        return self._read_alpha(sample_size, stock_size)

    def get_returns(self, codes, daybefore, sample_size, buysign_list, sellsign_list, max_return, max_drawdown, max_holdday, price = "close"):
        stock_size = len(codes)
        self._write_codes(codes)
        alphatree.getReturns(self.code_cache, stock_size, daybefore, sample_size, self.alphalist2char(buysign_list), len(buysign_list), self.alphalist2char(sellsign_list), len(sellsign_list), c_float(max_return), c_float(max_drawdown), c_int32(max_holdday), self.alpha_cache, c_char_p(price.encode('utf-8')))
        returns = []
        for i in range(sample_size):
            returns.append(self.alpha_cache[i])
        return np.array(returns)

    def get_bag(self, codes, daybefore, sample_size, bag_size, sign_name, feature_name):
        stock_size = len(codes)
        self._write_codes(codes)
        alphatree.getBag(self.code_cache, stock_size, c_char_p(feature_name.encode()), c_char_p(sign_name.encode()), c_int32(daybefore), c_int32(sample_size), c_int32(bag_size), self.alpha_cache)
        bags = []
        for i in range(bag_size):
            bags.append(self.alpha_cache[i])
        return np.array(bags)

    def get_node_alpha(self, alphatree_id, node_id, cache_id, sample_size):
        data_size = alphatree.getNodeAlpha(alphatree_id, node_id, cache_id, sample_size)
        stock_size = data_size / sample_size
        return self._read_alpha(sample_size, stock_size)

    #def process_alpha(self, alphatree_id, cache_id):
    #    alphatree.processAlpha(alphatree_id, cache_id)

    def get_process(self, alphatree_id, process_name, cache_id):
        alphatree.getRootProcess(alphatree_id, c_char_p(process_name.encode('utf-8')), cache_id, self.process_cache)
        return self._read_str(self.process_cache)

    def get_distinguish(self, sign_name, feature, target, daybefore, sample_size, sample_time):
        return alphatree.getDistinguish(c_char_p(sign_name.encode()), c_char_p(feature.encode()), c_char_p(target.encode()), daybefore, sample_size, sample_time)

    def get_confidence(self, sign_name, feature, target, daybefore, sample_size, sample_time, support, std_scale = 2.0):
        return alphatree.getConfidence(c_char_p(sign_name.encode()), c_char_p(feature.encode()), c_char_p(target.encode()), daybefore, sample_size, sample_time, c_float(support), c_float(std_scale))

    def get_correlation(self, sign_name, a, b, daybefore, sample_size, sample_time):
        return alphatree.getCorrelation(c_char_p(sign_name.encode()), c_char_p(a.encode()), c_char_p(b.encode()), daybefore, sample_size, sample_time)

    def optimize_distinguish(self, sign_name, feature, target, daybefore, sample_size, sample_time, max_history_days = 75, explote_ratio = 0.1, err_try_time = 64):
        str_len = alphatree.optimizeDistinguish(c_char_p(sign_name.encode()), c_char_p(feature.encode()), c_char_p(target.encode()), daybefore, sample_size, sample_time, self.encode_cache, c_int32(max_history_days), c_float(explote_ratio), c_int32(err_try_time))
        str_list = [self.encode_cache[i].decode() for i in range(str_len)]
        return "".join(str_list)

    def optimize_confidence(self, sign_name, feature, target, daybefore, sample_size, sample_time, support, std_scale = 2.0, explote_ratio = 0.1, err_try_time = 64):
        str_len = alphatree.optimizeConfidence(c_char_p(sign_name.encode()), c_char_p(feature.encode()), c_char_p(target.encode()), daybefore, sample_size, sample_time, c_float(support), c_float(std_scale), self.encode_cache, c_float(explote_ratio), c_int32(err_try_time))
        str_list = [self.encode_cache[i].decode() for i in range(str_len)]
        return "".join(str_list)

    def _read_alpha(self, sample_size, stock_size):
        alpha_list = list()
        cur_alpha_index = 0
        for i in range(sample_size):
            alpha = list()
            for j in range(stock_size):
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
        for i in range(str_num):
            code = list()
            while str_cache[cur_index] != b'\x00':
                code.append(str_cache[cur_index].decode())
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

    @classmethod
    def alphalist2char(cls, alphalist):
        alphatree_char_num = 0
        for at in alphalist:
            alphatree_char_num += len(at) + 1

        alphatree_cache = (c_char * alphatree_char_num)()
        cur_alpha_index = 0
        for at in alphalist:
            code_list = list(at)
            for c in code_list:
                alphatree_cache[cur_alpha_index] = c.encode()
                cur_alpha_index += 1
            alphatree_cache[cur_alpha_index] = '\0'.encode()
            cur_alpha_index += 1
        return alphatree_cache

    def _read_codes(self, stock_size):
        return self._read_str_list(self.code_cache, stock_size)

    def _write_codes(self, codes):
        cur_code_index = 0
        for code in codes:
            code_list = list(code)
            for c in code_list:
                self.code_cache[cur_code_index] = c.encode()
                cur_code_index += 1
            self.code_cache[cur_code_index] = '\0'.encode()
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



