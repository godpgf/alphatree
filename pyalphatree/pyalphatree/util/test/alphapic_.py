#coding=utf-8
#author=godpgf
from ctypes import *
from pyalphatree.libalphatree import alphatree
import numpy as np


class AlphaPic(object):
    def __init__(self, sign_name, feature_list, daybefore, sample_size):
        features_cache = self.decode_features(feature_list)
        self.picid = alphatree.useAlphaPic(c_char_p(sign_name.encode('utf-8')), features_cache, len(feature_list), daybefore, sample_size)

    @classmethod
    def decode_features(cls, feature_list):
        feature_len = 0
        for feature in feature_list:
            feature_len += len(feature) + 1
        features_cache = (c_char * feature_len)()
        cur_feature_index = 0
        for feature in feature_list:
            name_list = list(feature.encode('utf-8'))
            for c in name_list:
                features_cache[cur_feature_index] = c
                cur_feature_index += 1
            features_cache[cur_feature_index] = b'\0'
            cur_feature_index += 1
        return features_cache


    def get_k_line(self, sign_name, open_elements, high_elements, low_elements, close_elements, daybefore, sample_size, column, max_std_scale):
        open_cache = self.decode_features(open_elements)
        high_cache = self.decode_features(high_elements)
        low_cache = self.decode_features(low_elements)
        close_cache = self.decode_features(close_elements)
        pic_size = (column * len(open_elements) * 3 * alphatree.getSignNum(daybefore, sample_size, c_char_p(sign_name.encode('utf-8'))))
        pic_cache = (c_float * pic_size)()
        alphatree.getKLinePic(self.picid, c_char_p(sign_name.encode('utf-8')), open_cache, high_cache, low_cache, close_cache, len(open_elements), daybefore, sample_size, pic_cache, column, c_float(max_std_scale))
        # kline = np.zeros((pic_size,))
        # for i in range(pic_size):
        #     kline[i] = pic_cache[i]
        kline = np.array(pic_cache)
        return kline.reshape([-1, column, len(open_elements) * 3, 1])

    def get_trend(self, sign_name, elements, daybefore, sample_size, column, max_std_scale):
        element_cache = self.decode_features(elements)
        pic_size = (column * len(elements) * 3 * alphatree.getSignNum(daybefore, sample_size, c_char_p(sign_name.encode('utf-8'))))
        pic_cache = (c_float * pic_size)()
        alphatree.getTrendPic(self.picid, c_char_p(sign_name.encode('utf-8')), element_cache, len(elements), daybefore, sample_size, pic_cache, column, c_float(max_std_scale))
        # trend = np.zeros((pic_size,))
        # for i in range(pic_size):
        #     trend[i] = pic_cache[i]
        trend = np.array(pic_cache)
        return trend.reshape([-1, column, len(elements) * 3, 1])