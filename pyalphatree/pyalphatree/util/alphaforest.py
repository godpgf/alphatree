# coding=utf-8
# author=godpgf
from ctypes import *
from pyalphatree.libalphatree import alphatree
from .alphatree import AlphaNode, AlphaTree
from stdb import *
import numpy as np
import math

class AlphaForest(object):

    #子公式中的节点打平加上公式中的节点数量不能超过max_node_size
    def __init__(self, max_history_size = 251, max_stoct_size = 3600, max_sample_size = 120, max_watch_size = 5, max_cache_size = 4, max_alpha_tree_size = 1024, max_node_size = 64, max_subtree_num = 16, max_achievement_size = 4096, sample_beta_size = 20):
        self.max_node_size = max_node_size
        alphatree.initializeAlphaforest(max_history_size, max_stoct_size, max_sample_size, max_watch_size, max_cache_size, max_alpha_tree_size, max_node_size, max_subtree_num, max_achievement_size, sample_beta_size)
        alphatree.initializeAlphaexaminer()
        self.encode_cache = (c_char * 512)()
        self.code_cache = (c_char * (max_stoct_size * 64))()
        self.alpha_cache = (c_float * (max_stoct_size * max_sample_size))()
        self.sub_alphatree_str_cache = (c_char * (max_alpha_tree_size * max_subtree_num * 128))()
        self.alphatree_id_cache = (c_int * max_alpha_tree_size)()
        self.name_cache = (c_char * 512)()
        self.coff_cache = (c_char * 512)()
        self.children_cache = (c_int * 2)()

    def __del__(self):
        alphatree.releaseAlphaexaminer()
        alphatree.releaseAlphaforest()

    #读取数据
    def load_data(self, cache_path = "data", is_offline = False, sample_beta_len = 20):
        codeProxy = LocalCodeProxy(cache_path, is_offline)
        dataProxy = LocalDataProxy(cache_path, is_offline)
        classifiedProxy = LocalClassifiedProxy(cache_path, is_offline)
        codes = codeProxy.get_codes()

        data_size = len(dataProxy.trading_calender_int)
        open = (c_float * data_size)()
        high = (c_float * data_size)()
        low = (c_float * data_size)()
        close = (c_float * data_size)()
        volume = (c_float * data_size)()
        vwap = (c_float * data_size)()
        returns = (c_float * data_size)()

        data = dataProxy.get_all_Data('0000001')
        self._add_stock('sh',None,None,None,data,open,high,low,close, volume,vwap,returns,0)
        data = dataProxy.get_all_Data('1399001')
        self._add_stock('sz', None, None, None, data, open, high, low, close, volume, vwap, returns, 0)

        for code in codes:
            data = dataProxy.get_all_Data(code[0])
            if data is not None:
                totals = int(code[2]/code[1])
                industry = classifiedProxy.industry_info(code[0])
                concept = classifiedProxy.concept_info(code[0])
                if industry is None or len(industry) == 0:
                    industry = '其他'
                else:
                    industry = industry.encode('utf8')
                if concept is None or len(concept) == 0:
                    concept = '其他'
                else:
                    concept = concept.encode('utf8')
                if code[0][0] == '0':
                    market = 'sh'
                else:
                    market = 'sz'

                self._add_stock(code[0], market, industry, concept, data, open, high, low, close, volume, vwap, returns, totals)

        alphatree.calClassifiedData()

    def get_codes(self, day_before, watch_future_num, history_num, sample_size):
        stock_size = alphatree.getCodes(day_before, watch_future_num, history_num, sample_size, self.code_cache)
        return np.array(self._read_codes(stock_size))

    def cal_stock_alpha(self, alphatree_id, codes, day_before = 0, sample_size = 1):
        self._write_codes(codes)
        stock_size = len(codes)

        cache_id = alphatree.useCacheMemory()
        flag_id = alphatree.useFlagCache()
        alphatree.calAlpha(alphatree_id, flag_id, cache_id, day_before, sample_size, self.code_cache, stock_size, self.alpha_cache)
        alpha_list = self._read_alpha(sample_size, stock_size)
        alphatree.releaseCacheMemory(cache_id)
        alphatree.releaseFlagCache(flag_id)
        return np.array(alpha_list)

    def cal_alphatree(self, alphatree_id, day_before = 0, sample_size = 1):
        histort_num = alphatree.getHistoryDays(alphatree_id)
        future_num = alphatree.getFutureDays(alphatree_id)
        stock_size = alphatree.getCodes(day_before, future_num, histort_num, sample_size, self.code_cache)
        cache_id = alphatree.useCacheMemory()
        flag_id = alphatree.useFlagCache()
        alphatree.calAlpha(alphatree_id, cache_id, flag_id, day_before, sample_size, self.code_cache, stock_size, self.alpha_cache)
        alphatree.releaseCacheMemory(cache_id)
        alphatree.releaseFlagCache(flag_id)
        alpha_list = self._read_alpha(sample_size, stock_size)
        codes_list = self._read_codes(stock_size)
        return np.array(alpha_list), np.array(codes_list)

    def eval_alphatree(self, alphatree_id, future_num = 5, eval_time = 16, train_date_size = 32, test_date_size = 32, min_sieve_num = 16, max_sieve_num = 512, punish_num = 6):
        cache_id = alphatree.useCacheMemory()
        flag_id = alphatree.useFlagCache()
        code_id = alphatree.useCodesCache()

        score = alphatree.evalAlphaTree(alphatree_id, cache_id, flag_id, code_id, future_num, eval_time, train_date_size, test_date_size, min_sieve_num, max_sieve_num, punish_num)

        alphatree.releaseCacheMemory(cache_id)
        alphatree.releaseFlagCache(flag_id)
        alphatree.releaseCodesCache(code_id)
        return math.exp(score) - 1

    def cal_alpha(self, line, day_before = 0, sample_size = 1, sub_alpha_dict = None):
        alphatree_id = self.create_alphatree()
        #解码
        if sub_alpha_dict:
            for name, value in sub_alpha_dict.items():
                alphatree.decodeSubAlphatree(alphatree_id, c_char_p(name), c_char_p(value))
        alphatree.decodeAlphatree(alphatree_id, c_char_p(line))
        alpha_list, codes_list = self.cal_alphatree(alphatree_id, day_before, sample_size)
        self.release_alphatree(alphatree_id)
        return alpha_list, codes_list

    #future_index=0表示未来一天，1表示未来2天
    def get_target(self, future_index, day_before, codes, sample_size = 1):
        self._write_codes(codes)
        stock_size = len(codes)
        alphatree.getTarget(future_index, day_before, sample_size, stock_size, self.alpha_cache, self.code_cache)
        return self._read_alpha(sample_size, stock_size)

    def load_alphatree(self, alphatree_id):
        root = self._load_alphatree_node(alphatree_id)
        return AlphaTree(root)

    def _load_alphatree_node(self, alphatree_id, node_id=None):
        if node_id is None:
            node_id = alphatree.getRoot(alphatree_id)
        child_num = alphatree.getNodeDes(alphatree_id, node_id, self.name_cache, self.coff_cache, self.children_cache)
        name = self._read_str(self.name_cache)
        coff = self._read_str(self.coff_cache)
        node = AlphaNode(name, coff)
        leftId = self.children_cache[0] if child_num > 0 else None
        rightId = self.children_cache[1] if child_num > 1 else None
        if leftId is not None:
            node.children.append(self._load_alphatree_node(alphatree_id, leftId))
            node.children[-1].parent = node
        if rightId is not None:
            node.children.append(self._load_alphatree_node(alphatree_id, rightId))
            node.children[-1].parent = node
        return node


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
    def _read_str(cls, str_cache):
        code = list()
        cur_index = 0
        while str_cache[cur_index] != '\0':
            code.append(str_cache[cur_index])
            cur_index += 1
        return "".join(code)

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

    def _write_alphatree_ids(self, alphatree_list):
        cur_index = 0
        for id in alphatree_list:
            self.alphatree_id_cache[cur_index] = id
            cur_index += 1

    def _read_sub_alphatree(self, sub_alphatree_num):
        return self._read_str_list(self.sub_alphatree_str_cache, sub_alphatree_num)

    def _add_stock(self, code, market, industry, concept, data, open, high, low, close, volume, vwap, returns, totals):
        startIndex = 0
        while data[startIndex][5] == 0:
            startIndex += 1

        length = len(data) - startIndex

        # 只看上市超过大半年的
        if length > 160:
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


    def create_alphatree(self, line = None, sub_alpha_dict = None):
        alphatree_id = alphatree.createAlphatree()
        # 解码
        if sub_alpha_dict:
            for name, value in sub_alpha_dict.items():
                alphatree.decodeSubAlphatree(alphatree_id, c_char_p(name), c_char_p(value))
        if line:
            alphatree.decodeAlphatree(alphatree_id, c_char_p(line))
        return alphatree_id

    def get_history_num(self, alphatree_id):
        return alphatree.getHistoryDays(alphatree_id)

    def get_future_num(self, alphatree_id):
        return alphatree.getFutureDays(alphatree_id)

    def release_alphatree(self, alphatree_id):
        alphatree.releaseAlphatree(alphatree_id)

    def encode_alphatree(self, alphatree_id):
        str_len = alphatree.encodeAlphatree(alphatree_id, self.encode_cache)
        str_list = [self.encode_cache[i] for i in xrange(str_len)]
        return "".join(str_list)

    def summary_sub_alphatree(self, alphatree_list, min_depth = 3):
        self._write_alphatree_ids(alphatree_list)
        sub_alphatree_num = alphatree.summarySubAlphaTree(self.alphatree_id_cache, len(alphatree_list), min_depth, self.sub_alphatree_str_cache)
        return self._read_sub_alphatree(sub_alphatree_num)



