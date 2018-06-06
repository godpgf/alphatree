#coding=utf-8
#author=godpgf
from ctypes import *
from pyalphatree.libalphatree import alphatree
import numpy as np
import math


class AlphaArray(object):

    def __init__(self, name, formula_list, data_column, day_before, sample_days, sign_history = None):
        if sign_history:
            #信号名字只能是字符串
            assert isinstance(name, str)
        cur_code_index = 0
        if isinstance(name, list) or isinstance(name, np.ndarray):
            self.code_cache = (c_char * (len(name) * 64))()
            cur_code_index = 0
            for code in name:
                code_list = list(code)
                for c in code_list:
                    self.code_cache[cur_code_index] = c
                    cur_code_index += 1
                self.code_cache[cur_code_index] = '\0'
                cur_code_index += 1
        else:
            code_list = list(name.encode('utf-8'))
            self.code_cache = (c_char * (len(code_list) + 1))()
            for c in code_list:
                self.code_cache[cur_code_index] = c
                cur_code_index += 1
            self.code_cache[cur_code_index] = b'\0'

        self.name = name
        self.formula_list = formula_list
        self.data_column = data_column
        self.day_before = day_before
        self.sample_days = sample_days
        self.sign_history = sign_history
        self.alphatree_id = alphatree.createAlphatree()
        self.cache_id = alphatree.useCache()
        self.data_num = alphatree.getSignNum(day_before, sample_days, c_char_p(name.encode('utf-8'))) if sign_history else sample_days
        for formula in self.formula_list:
            tmp = formula.split('=')
            alphatree.decodeAlphatree(self.alphatree_id, c_char_p(tmp[0].strip().encode('utf-8')), c_char_p(("" if len(tmp) == 1 else tmp[1].strip()).encode('utf-8')))

    def __del__(self):
        alphatree.synchroAlpha(self.alphatree_id, self.cache_id)
        alphatree.releaseCache(self.cache_id)
        alphatree.releaseAlphatree(self.alphatree_id)

    def __len__(self):
        return self.data_num

    def __getitem__(self, item):
        if isinstance(item, int):
            if isinstance(self.name, list) or isinstance(self.name, np.ndarray):
                alpha_cache = (c_float * len(self.name))
                alphatree.calAlpha(self.alphatree_id, self.cache_id, self.day_before + self.sample_days - 1 - item, 1, self.code_cache, len(self.name))

                if isinstance(self.data_column, list):
                    data = []
                    for root_name in self.data_column:
                        alphatree.getAlpha(self.alphatree_id, c_char_p(root_name.encode('utf-8')), self.cache_id, alpha_cache)
                        data.append([alpha_cache[i] for i in range(len(self.name))])
                    return np.array(data)
                else:
                    alphatree.getAlpha(self.alphatree_id, c_char_p(self.data_column.encode('utf-8')), self.cache_id, alpha_cache)
                    return np.array([alpha_cache[i] for i in range(len(self.name))])
            else:
                if self.sign_history:
                    alpha_cache = (c_float * (self.sign_history))()
                    alphatree.calSignAlpha(self.alphatree_id, self.cache_id, self.day_before, self.sample_days, item, 1, self.sign_history, self.code_cache)
                else:
                    alpha_cache = (c_float * (1))()
                    alphatree.calAlpha(self.alphatree_id, self.cache_id, self.day_before + self.sample_days - 1 - item, 1, self.code_cache, 1)
                if isinstance(self.data_column, list):
                    data = []
                    if not self.sign_history or self.sign_history == 1:
                        for root_name in self.data_column:
                            alphatree.getAlpha(self.alphatree_id, c_char_p(root_name.encode('utf-8')), self.cache_id, alpha_cache)
                            data.append(alpha_cache[0])
                    else:
                        for root_name in self.data_column:
                            alphatree.getAlpha(self.alphatree_id, c_char_p(root_name.encode('utf-8')), self.cache_id, alpha_cache)
                            data.append([alpha_cache[i] for i in range(len(self.sign_history))])
                    return np.array(data)
                else:
                    alphatree.getAlpha(self.alphatree_id, c_char_p(self.data_column.encode('utf-8')), self.cache_id, alpha_cache)
                    if not self.sign_history or self.sign_history == 1:
                        return alpha_cache[0]
                    else:
                        return np.array([alpha_cache[i] for i in range(len(self.sign_history))])
        elif isinstance(item, slice):
            stop = item.stop if item.stop is not None else len(self)
            start = item.start if item.start is not None else 0
            stop = len(self) + stop if stop < 0 else stop
            start = len(self) + start if start < 0 else start
            if isinstance(self.name, list) or isinstance(self.name, np.ndarray):
                alpha_cache = (c_float * ((stop - start) * len(self.name)))()
                alphatree.calAlpha(self.alphatree_id, self.cache_id, self.day_before + self.sample_days - stop, stop - start, self.code_cache, len(self.name))
                if isinstance(self.data_column, list):
                    #天、列、股票
                    data_list = [[[] for j in range(len(self.data_column))] for i in range(stop - start)]
                    for id, root_name in enumerate(self.data_column):
                        alphatree.getAlpha(self.alphatree_id, c_char_p(root_name.encode('utf-8')), self.cache_id, alpha_cache)
                        for i in range(stop - start):
                            data_list[i][id].extend([alpha_cache[i * len(self.name) + j] for j in range(len(self.name))])
                    return np.array(data_list)
                else:
                    data_list = []
                    alphatree.getAlpha(self.alphatree_id, c_char_p(self.data_column.encode('utf-8')), self.cache_id, alpha_cache)
                    for i in range(stop - start):
                        data_list.append([alpha_cache[i * len(self.name) + j] for j in range(len(self.name))])
                    return np.array(data_list)
            else:
                if self.sign_history:
                    alpha_cache = (c_float * ((stop - start) * self.sign_history))()
                    alphatree.calSignAlpha(self.alphatree_id, self.cache_id, self.day_before, self.sample_days, start, stop - start, self.sign_history, self.code_cache)
                else:
                    alpha_cache = (c_float * (stop - start))()
                    alphatree.calAlpha(self.alphatree_id, self.cache_id, self.day_before + self.sample_days - stop, stop - start, self.code_cache, 1)

                if isinstance(self.data_column, list):
                    data_list = [[] for i in range(stop - start)]
                    for root_name in self.data_column:
                        alphatree.getAlpha(self.alphatree_id, c_char_p(root_name.encode('utf-8')), self.cache_id, alpha_cache)
                        if not self.sign_history or self.sign_history == 1:
                            for i in range(stop - start):
                                data_list[i].append(alpha_cache[i])
                        else:
                            for i in range(stop - start):
                                data_list[i].append([alpha_cache[j * (stop - start) + i] for j in range(self.sign_history)])
                    return np.array(data_list)
                else:
                    alphatree.getAlpha(self.alphatree_id, c_char_p(self.data_column.encode('utf-8')), self.cache_id, alpha_cache)
                    if not self.sign_history or self.sign_history == 1:
                        return np.array([alpha_cache[i] for i in range(stop - start)])
                    else:
                        return np.array([[alpha_cache[j * (stop - start) + i] for j in range(self.sign_history)] for i in range(stop - start)])

    def normalize(self, batch_size = 4096):
        assert self.is_sign
        alpha_cache = (c_float * (2))()
        start_index = 0
        if isinstance(self.data_column, list):
            sum_value = [0 for i in range(len(self.data_column))]
            sqr_sum_value = [0 for i in range(len(self.data_column))]
        else:
            sum_value = 0
            sqr_sum_value = 0
        data_len = len(self)
        while start_index < data_len:
            data_size = min(data_len - start_index, batch_size)
            alphatree.calSignAlpha(self.alphatree_id, self.cache_id, self.day_before, self.sample_days, start_index, data_size, 1, self.code_cache)
            if isinstance(self.data_column, list):
                for id, root_name in enumerate(self.data_column):
                    alphatree.getAlphaSum(self.alphatree_id, c_char_p(root_name.encode('utf-8')), self.cache_id, alpha_cache)
                    sum_value[id] += alpha_cache[0]
                    sqr_sum_value[id] += alpha_cache[1]
            else:
                alphatree.getAlpha(self.alphatree_id, c_char_p(self.data_column.encode('utf-8')), self.cache_id, alpha_cache)
                sum_value += alpha_cache[0]
                sqr_sum_value += alpha_cache[1]
            start_index += data_size

        if isinstance(self.data_column, list):
            avg = np.array(sum_value) / data_len
            std = np.sqrt(np.array(sqr_sum_value) / data_len - avg * avg)
        else:
            avg = sum_value / data_len
            std = math.sqrt(sqr_sum_value / data_len - avg * avg)
        return avg, std

    def smooth(self, ratio = 0.001, batch_size = 4096):
        assert self.is_sign
        smooth_num = int(len(self) * ratio)
        start_index = 0
        alpha_cache = (c_float * (smooth_num * 2))()
        if isinstance(self.data_column, list):
            left_smooth_value = [[] for i in range(len(self.data_column))]
            right_smooth_value = [[] for i in range(len(self.data_column))]
        else:
            left_smooth_value = []
            right_smooth_value = []
        data_len = len(self)
        while start_index < data_len:
            data_size = min(data_len - start_index, batch_size)
            alphatree.calSignAlpha(self.alphatree_id, self.cache_id, self.day_before, self.sample_days, start_index, data_size, 1, self.code_cache)
            if isinstance(self.data_column, list):
                for id, root_name in enumerate(self.data_column):
                    alphatree.getAlphaSmooth(self.alphatree_id, c_char_p(root_name.encode('utf-8')), self.cache_id, smooth_num, alpha_cache)
                    left_smooth_value[id], right_smooth_value[id] = self._fill_smooth_value(left_smooth_value[id], right_smooth_value[id], alpha_cache, smooth_num , min(smooth_num, data_size))
            else:
                alphatree.getAlphaSmooth(self.alphatree_id, c_char_p(self.data_column.encode('utf-8')), self.cache_id, smooth_num, alpha_cache)
                left_smooth_value[id], right_smooth_value[id] = self._fill_smooth_value(left_smooth_value, right_smooth_value, alpha_cache, smooth_num, min(smooth_num, data_size))
            start_index += data_size

        if isinstance(self.data_column, list):
            left_smooth = []
            right_smooth = []
            for i in range(len(self.data_column)):
                left_smooth.append(left_smooth_value[i][-1])
                right_smooth.append(right_smooth_value[i][-1])
            left_smooth = np.array(left_smooth)
            right_smooth = np.array(right_smooth)
        else:
            left_smooth = left_smooth_value[-1]
            right_smooth = right_smooth_value[-1]
        return left_smooth, right_smooth

    @classmethod
    def _fill_smooth_value(cls, left_smooth, right_smooth, smooth_cache, smooth_num, cache_smooth_num):
        new_left_smooth = []
        from_id = 0
        to_id = 0
        while from_id < len(left_smooth) or to_id < cache_smooth_num:
            if len(new_left_smooth) == smooth_num:
                break
            if from_id < len(left_smooth) and to_id < cache_smooth_num:
                if left_smooth[from_id] < smooth_cache[to_id]:
                    new_left_smooth.append(left_smooth[from_id])
                    from_id += 1
                else:
                    new_left_smooth.append(smooth_cache[to_id])
                    to_id += 1
                continue
            if from_id < len(left_smooth):
                new_left_smooth.append(left_smooth[from_id])
                from_id += 1
            if to_id < cache_smooth_num:
                new_left_smooth.append(smooth_cache[to_id])
                to_id += 1

        new_right_smooth = []
        from_id = 0
        to_id = 0
        while from_id < len(right_smooth) or to_id < cache_smooth_num:
            if len(new_right_smooth) == smooth_num:
                break
            if from_id < len(right_smooth) and to_id < cache_smooth_num:
                if right_smooth[from_id] > smooth_cache[to_id + smooth_num]:
                    new_right_smooth.append(right_smooth[from_id])
                    from_id += 1
                else:
                    new_right_smooth.append(smooth_cache[to_id + smooth_num])
                    to_id += 1
                continue
            if from_id < len(right_smooth):
                new_right_smooth.append(right_smooth[from_id])
                from_id += 1
            if to_id < cache_smooth_num:
                new_right_smooth.append(smooth_cache[to_id + smooth_num])
                to_id += 1
        return new_left_smooth, new_right_smooth

