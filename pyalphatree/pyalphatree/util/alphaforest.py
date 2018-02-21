# coding=utf-8
# author=godpgf
from ctypes import *
from pyalphatree.libalphatree import alphatree
from .alphatree import AlphaNode, AlphaTree
from stdb import *
import numpy as np
import math
import json


class AlphaForest(object):
    def __init__(self, cache_size=4, max_stock_num=4096, max_day_num=4096, max_feature_size=2048):
        alphatree.initializeAlphaforest(cache_size)

        # self.max_sample_size = 0
        # self.max_stoct_size = 0
        self.code_cache = (c_char * (max_stock_num * 64))()
        self.alpha_cache = (c_float * (max_stock_num * max_day_num))()
        self.feature_cache = (c_char * (max_feature_size * 64))()
        # self.max_alpha_tree_size = 0
        # self.check_alpha_size(2500, 3600)
        # self.check_tree_size(1024)

        self.max_alpha_tree_str_len = 4096;
        self.sub_alphatree_str_cache = (c_char * (self.max_alpha_tree_str_len * 1024))()
        self.alphatree_id_cache = (c_int * 1024)()

        self.encode_cache = (c_char * self.max_alpha_tree_str_len)()
        self.process_cache = (c_char * (4096 * 64))()

    # def check_alpha_size(self, sample_size, stock_size):
    #     is_resize_alpha = False
    #     if sample_size > self.max_sample_size:
    #         self.max_sample_size = sample_size
    #         self.sample_flag_cache = (c_bool * self.max_sample_size)
    #         is_resize_alpha = True
    #     if stock_size > self.max_stoct_size:
    #         self.max_stoct_size = stock_size
    #         self.code_cache = (c_char * (self.max_stoct_size * 64))()
    #         is_resize_alpha = True
    #     if is_resize_alpha:
    #         self.alpha_cache = (c_float * (self.max_stoct_size * self.max_sample_size))()

    # def check_tree_size(self, alpha_tree_size):
    #     if alpha_tree_size > self.max_alpha_tree_size:
    #         self.max_alpha_tree_size = alpha_tree_size
    #         self.alphatree_id_cache = (c_int * self.max_alpha_tree_size)()

    def __del__(self):
        alphatree.releaseAlphaforest()

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

    def optimize_alphatree(self):
        pass

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


    def optimize_alpha(self, alphatree_id, cache_id, root_name, daybefore, sample_size, codes, exploteRatio = 0.1, errTryTime = 64):
        stock_size = len(codes)
        self._write_codes(codes)
        return alphatree.optimizeAlpha(alphatree_id, cache_id, c_char_p(root_name), daybefore, sample_size, self.code_cache, stock_size, c_float(exploteRatio), errTryTime)

    def get_root_alpha(self, alphatree_id, root_name, cache_id, sample_size):
        data_size = alphatree.getRootAlpha(alphatree_id, c_char_p(root_name), cache_id, self.alpha_cache)
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

    def summary_sub_alphatree(self, alphatree_list, min_depth=3):
        self._write_alphatree_ids(alphatree_list)
        sub_alphatree_num = alphatree.summarySubAlphaTree(self.alphatree_id_cache, len(alphatree_list), min_depth,
                                                          self.sub_alphatree_str_cache)
        sub_alphatree_list = self._read_str_list(self.sub_alphatree_str_cache, sub_alphatree_num)
        # 按照长度从大到小返回
        sub_alphatree_list.sort(key=lambda x: -len(x))
        return sub_alphatree_list

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
    def _write_data(cls, open, high, low, close, volume, vwap, returns, bar, start_index):
        for id, v in enumerate(bar["open"]):
            open[start_index + id] = v
        for id, v in enumerate(bar["high"]):
            high[start_index + id] = v
        for id, v in enumerate(bar["low"]):
            low[start_index + id] = v
        for id, v in enumerate(bar["close"]):
            close[start_index + id] = v
        for id, v in enumerate(bar["volume"]):
            volume[start_index + id] = v
        for id, v in enumerate(bar["vwap"]):
            vwap[start_index + id] = v
        for id, v in enumerate(bar["rise"]):
            returns[start_index + id] = v

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

    def _write_alphatree_ids(self, alphatree_list):
        cur_index = 0
        for id in alphatree_list:
            self.alphatree_id_cache[cur_index] = id
            cur_index += 1

    def _add_stock(self, code, market, industry, concept, data, open, high, low, close, volume, vwap, returns, totals):
        startIndex = 0
        while data[startIndex][5] == 0:
            startIndex += 1

        length = len(data) - startIndex
        if length > 1:
            for i in xrange(startIndex, len(data)):
                open[i - startIndex] = data[i][1]
                high[i - startIndex] = data[i][2]
                low[i - startIndex] = data[i][3]
                close[i - startIndex] = data[i][4]
                volume[i - startIndex] = data[i][5]
                vwap[i - startIndex] = data[i][6]
                returns[i - startIndex] = data[i][7]
            # print "open(%.4f~%.4f) volume(%.4f~%.4f) vwap(%.4f~%.4f) returns(%.4f~%.4f)"%(data[startIndex][1],data[-1][1],data[startIndex][5],data[-1][5],data[startIndex][6],data[-1][6],data[startIndex][7],data[-1][7])
            alphatree.addStock(c_char_p(code), c_char_p(market), c_char_p(industry), c_char_p(concept),
                               open, high, low, close, volume, vwap, returns, length, totals)
            return True
        return False

    # todo delete later
    # 加载树的结构。如果sample_size==0表示不计算经过各个节点的均值和标准差
    def load_alphatree(self, alphatree_id, day_before=0, sample_size=0):
        if sample_size > 0:
            histort_num = alphatree.getHistoryDays(alphatree_id)
            future_num = alphatree.getFutureDays(alphatree_id)
            stock_size = alphatree.getCodes(day_before, future_num, histort_num, sample_size, self.code_cache)
            cache_id = alphatree.useCacheMemory()
            flag_id = alphatree.useFlagCache()
            alphatree.calAlpha(alphatree_id, cache_id, flag_id, day_before, sample_size, self.code_cache, stock_size,
                               c_void_p(0))
            root = self._load_alphatree_node(alphatree_id, sample_size=sample_size, cache_id=cache_id,
                                             stock_size=stock_size)
            alphatree.releaseCacheMemory(cache_id)
            alphatree.releaseFlagCache(flag_id)
        else:
            root = self._load_alphatree_node(alphatree_id)
        return AlphaTree(root)

    def _load_alphatree_node(self, alphatree_id, node_id=None, sample_size=0, cache_id=None, stock_size=None):
        if node_id is None:
            node_id = alphatree.getRoot(alphatree_id)
        child_num = alphatree.getNodeDes(alphatree_id, node_id, self.name_cache, self.coff_cache, self.children_cache)

        # 计算流过此节点的数据特征
        avg = 0
        std = 0
        if sample_size > 0:
            alphatree.getAlpha(alphatree_id, node_id, cache_id, sample_size, stock_size, self.alpha_cache)
            alpha = np.array(self._read_alpha(sample_size, stock_size))
            alpha.reshape(-1)
            avg = alpha.mean()
            std = alpha.std()
            if math.isnan(std) or math.isinf(std) or std < 0:
                print "alphatreeId:%d nodeId:%d" % (alphatree_id, node_id)
                raise "计算错误!"

        name = self._read_str(self.name_cache)
        coff = self._read_str(self.coff_cache)
        node = AlphaNode(name, coff, avg, std)
        leftId = self.children_cache[0] if child_num > 0 else None
        rightId = self.children_cache[1] if child_num > 1 else None
        if leftId is not None:
            node.add_child(self._load_alphatree_node(alphatree_id, leftId, sample_size, cache_id, stock_size))
        if rightId is not None:
            node.add_child(self._load_alphatree_node(alphatree_id, rightId, sample_size, cache_id, stock_size))
        return node
