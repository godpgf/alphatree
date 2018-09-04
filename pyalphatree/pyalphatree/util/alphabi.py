from ctypes import *
from pyalphatree.libalphatree import alphatree


class AlphaBI(object):
    def __init__(self, sign_name, daybefore, sample_size,
                                 sample_time, support, rand_feature = None, returns = None):
        self.id = alphatree.useBIGroup(c_char_p(sign_name.encode('utf-8')),
                                    c_int32(daybefore),c_int32(sample_size),
                                    c_int32(sample_time),c_float(support))
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

    def get_discrimination(self, feature, target, min_rand_percent = 0.05, min_R2 = 0.32):
        return alphatree.getDiscrimination(c_int32(self.id), c_char_p(feature.encode('utf-8')),
                                           c_char_p(target.encode('utf-8')),
                                           c_float(min_rand_percent), c_float(min_R2))

    def optimize_discrimination(self, feature, target, min_rand_percent = 0.05, min_R2 = 0.32, max_history_days = 75,
                                explote_ratio = 0.1, err_try_time = 64):
        str_len = alphatree.optimizeDiscrimination(c_int32(self.id), c_char_p(feature.encode()), c_char_p(target.encode()), self.encode_cache, c_float(min_rand_percent), c_float(min_R2), c_int32(max_history_days), c_float(explote_ratio), c_int32(err_try_time))
        str_list = [self.encode_cache[i].decode() for i in range(str_len)]
        return "".join(str_list)