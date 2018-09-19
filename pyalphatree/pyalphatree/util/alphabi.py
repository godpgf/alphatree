from ctypes import *
from pyalphatree.libalphatree import alphatree


class AlphaBI(object):
    def __init__(self, sign_name, daybefore, sample_size,
                                 sample_time, support, expect_return, rand_feature = None, returns = None):
        self.id = alphatree.useBIGroup(c_char_p(sign_name.encode('utf-8')),
                                    c_int32(daybefore),c_int32(sample_size),
                                    c_int32(sample_time),c_float(support),c_float(expect_return))
        if rand_feature:
            alphatree.pluginControlBIGroup(c_int32(self.id), c_char_p(rand_feature.encode('utf-8')), c_char_p(returns.encode('utf-8')))
        self.max_alpha_tree_str_len = 4096;
        self.encode_cache = (c_char * self.max_alpha_tree_str_len)()

    def __del__(self):
        alphatree.releaseBIGroup(c_int32(self.id))
        #alphatree.releaseAlphaforest()

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        # alphatree.releaseAlphaGraph()
        # alphatree.releaseBIGroup(c_int32(self.id))
        pass

    def get_correlation(self, a, b):
        return alphatree.getCorrelation(c_int32(self.id), c_char_p(a.encode('utf-8')), c_char_p(b.encode('utf-8')))

    def get_discrimination(self, feature, min_rand_percent = 0.32, std_scale = 2.0):
        return alphatree.getDiscrimination(c_int32(self.id), c_char_p(feature.encode('utf-8')),
                                           c_float(min_rand_percent), c_float(std_scale))

    def optimize_discrimination(self, feature, min_rand_percent = 0.32, std_scale = 2.0, max_history_days = 75,
                                explote_ratio = 0.1, err_try_time = 64):
        str_len = alphatree.optimizeDiscrimination(c_int32(self.id), c_char_p(feature.encode()), self.encode_cache, c_float(min_rand_percent), c_float(std_scale), c_int32(max_history_days), c_float(explote_ratio), c_int32(err_try_time))
        str_list = [self.encode_cache[i].decode() for i in range(str_len)]
        return "".join(str_list)

    def get_discrimination_inc(self, inc_feature, base_features, std_scale = 2.0):
        inc = 0
        for feature in base_features:
            inc = max(inc, alphatree.getDiscriminationInc(c_int32(self.id), c_char_p(inc_feature.encode('utf-8')), c_char_p(feature.encode('utf-8')),
                                           c_float(std_scale)))
        return inc

    def optimize_discrimination_inc(self, inc_feature, base_features, std_scale = 2.0, max_history_days = 75,
                                explote_ratio = 0.1, err_try_time = 64):
        inc = 0
        for feature in base_features:
            str_len = alphatree.optimizeDiscriminationInc(c_int32(self.id), c_char_p(inc_feature.encode('utf-8')), c_char_p(feature.encode()), self.encode_cache, c_float(std_scale), c_int32(max_history_days), c_float(explote_ratio), c_int32(err_try_time))
            line = "".join([self.encode_cache[i].decode() for i in range(str_len)])
            cur_inc = self.get_discrimination_inc(line, [feature], std_scale)
            if cur_inc > inc:
                res = line
        return res