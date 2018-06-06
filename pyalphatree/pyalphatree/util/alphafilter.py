# coding=utf-8
# author=godpgf
# 对alpha做初步筛选，保留好的并且不怎么相关的alpha
from ctypes import *
from pyalphatree.libalphatree import alphatree
import math

class AlphaFilter(object):
    def __init__(self, alphaforest, sign_name, test_daybefore, test_sample_size, max_alpha_num = 64, max_corr_coff = 0.3, eval_formula = "noise_valid(%s, mfe_5, mae_5, 64)", test_corr_days = 25):
        self.alphaforest = alphaforest
        self.sign_name = sign_name
        self.test_daybefore = test_daybefore
        self.test_sample_size = test_sample_size
        self.max_alpha_num = max_alpha_num
        self.max_corr_coff = max_corr_coff
        self.eval_formula = eval_formula
        self.alpha_tree_list = []
        self.alpha_tree_score = []
        self.min_score = None
        #测试相关性的天数，为了效率，最好就用默认即可
        self.test_corr_days = min(test_corr_days, test_sample_size)

    def __del__(self):
        pass

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        pass

    def add_alpha(self, line):
        score = self.alphaforest.process_alpha(self.eval_formula%line, self.test_daybefore, self.test_sample_size, self.sign_name)
        if len(self.alpha_tree_list) < self.max_alpha_num:
            self.alpha_tree_list.append(line)
            self.alpha_tree_score.append(score)
            if self.min_score is None or self.min_score > score:
                self.min_score = score
            return True
        elif score > self.min_score:
            max_corr_value = self.max_corr_coff
            max_corr_id = -1
            is_similar = False
            min_score_id = -1
            for id, alpha_tree in enumerate(self.alpha_tree_list):
                corr_value = math.fabs(self.alphaforest.process_alpha("alpha_correlation(%s, %s)"%(alpha_tree, line), self.test_daybefore, self.test_sample_size, self.sign_name))
                if corr_value > max_corr_value:
                    is_similar = True
                    if score > self.alpha_tree_score[id]:
                        max_corr_value = corr_value
                        max_corr_id = id
                if min_score_id == -1 or self.alpha_tree_score[id] < self.alpha_tree_score[min_score_id]:
                    min_score_id = id
            if is_similar:
                #替换相关性最大的
                if max_corr_id != -1:
                    self.alpha_tree_list[max_corr_id] = line
                    if self.alpha_tree_score[max_corr_id] == self.min_score:
                        self.alpha_tree_score[max_corr_id] = score
                        self._refresh_min_score()
                    else:
                        self.alpha_tree_score[max_corr_id] = score
                    return True
            else:
                #替换分数最低的
                self.alpha_tree_list[min_score_id] = line
                self.alpha_tree_score[min_score_id] = score
                self._refresh_min_score()
                return True
        return False

    def _refresh_min_score(self):
        self.min_score = None
        for score in self.alpha_tree_score:
            if self.min_score is None or self.min_score > score:
                self.min_score = score