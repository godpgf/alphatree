#coding=utf-8
#author=godpgf
from ctypes import *
from pyalphatree.libalphatree import alphatree
import numpy as np


class AlphaRFT(object):
    def __init__(self, alphatree_list, reward_fun_name = "base", thread_num = 4):
        self.reward_fun_name = reward_fun_name
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
        alphatree.initializeAlphaRFT(alphatree_cache, len(alphatree_list), thread_num)
        self.start_index = 0
        self.step_num = 0

    def __del__(self):
        pass
        #alphatree.releaseAlphaRFT()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        alphatree.releaseAlphaRFT()

    def testRead(self, daybefore, sample_size, feature_num, target, sign_name, cache_size = 2048):
        sign_num = alphatree.getSignNum(daybefore, sample_size, c_char_p(sign_name))
        target_cache = (c_float * sign_num)()
        feature_cache = (c_float * (sign_num * feature_num))()
        data_num = alphatree.testReadRFT(daybefore, sample_size, c_char_p(target), c_char_p(sign_name), feature_cache, target_cache, cache_size)
        assert data_num == sign_num
        t = [target_cache[i] for i in range(sign_num)]
        f = []
        for i in range(sign_num):
            for j in range(feature_num):
                f.append(feature_cache[i * feature_num + j])
        #f = [[feature_cache[i * feature_num + j] for j in range(feature_num)] for i in range(sign_num)]
        return np.array(f), np.array(t)

    def train(self, daybefore, sample_size, weight, target, sign_name,
                gamma_value, lambda_value, min_weight, epoch_num, step_num = 1,
                split_num = 64, cache_size = 4096,
                loss_fun_name = "binary:logistic", lr = 0.1, step = 0.8, tired_coff = 0.016):
        alphatree.trainAlphaRFT(daybefore, sample_size, c_char_p(weight) if weight else None, c_char_p(target), c_char_p(sign_name), c_float(gamma_value), c_float(lambda_value),
                             c_float(min_weight), epoch_num, c_char_p(self.reward_fun_name), step_num, split_num, cache_size,
                             c_char_p(loss_fun_name), c_float(lr), c_float(step), c_float(tired_coff))

    def eval(self, daybefore, sample_size, target, sign_name, step_index, cache_size = 4096):
        alphatree.evalAlphaRFT(daybefore, sample_size, c_char_p(target), c_char_p(sign_name), step_index, cache_size)


    def pred(self, sign_name, daybefore = 0, step_index = 0, step_num = 1, cache_size = 4096, max_stock_num=4096):
        alpha_cache = (c_float * max_stock_num)()
        stock_num = alphatree.predAlphaRFT(daybefore, alpha_cache, c_char_p(sign_name), c_char_p(self.reward_fun_name), step_index, step_num, cache_size)
        return np.array([alpha_cache[i] for i in range(stock_num)])
    # def pred(self, codes, daybefore = 0, step_index = 0, step_num = 1):
    #     alpha_cache = (c_float * len(codes))()
    #     code_cache = (c_char * (len(codes) * 64))()
    #     cur_code_index = 0
    #     for code in codes:
    #         code_list = list(code)
    #         for c in code_list:
    #             code_cache[cur_code_index] = c
    #             cur_code_index += 1
    #         code_cache[cur_code_index] = '\0'
    #         cur_code_index += 1
    #     alphatree.predAlphaRFT(daybefore, alpha_cache, code_cache, len(codes), c_char_p(self.reward_fun_name), step_index, step_num)
    #     return np.array([alpha_cache[i] for i in range(len(codes))])

    def save_model(self, path):
        alphatree.saveRFTModel(c_char_p(path))

    def load_model(self, path):
        alphatree.loadRFTModel(c_char_p(path))

    def clean_alpha(self, threshold, step_index = 0, step_num = 1):
        alphatree.cleanAlphaRFT(c_float(threshold), c_char_p(self.reward_fun_name), step_index, step_num)

    def clean_correlation(self, daybefore, sample_days, sign_name, corr_other_percent = 0.16, corr_all_percent = 0.32, cache_size = 2048):
        alphatree.cleanAlphaRFTCorrelationLeaf(daybefore, sample_days, c_char_p(sign_name), c_float(corr_other_percent), c_float(corr_all_percent), cache_size)

    def tostring(self, step_index = 0, step_num = 1):
        alphatree_cache = (c_char * 131072)()
        char_num = alphatree.alphaRFT2String(alphatree_cache, c_char_p(self.reward_fun_name), step_index, step_num)
        return "".join([alphatree_cache[i] for i in range(char_num)])
