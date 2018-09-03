# coding=utf-8
# author=godpgf
from .util import AlphaForest, AlphaArray, AlphaGBDT, AlphaBI
import re

def cache_base(data_path, titles = ["date","open","high","low","close","volume","vwap","returns","amount","turn","tcap","mcap"]):
    with AlphaForest() as af:
        af.load_db(data_path)
        for title in titles:
            af.csv2binary(data_path, title)
        af.cache_miss()
        af.cache_rand()

#缓存各种指标
def cache_indicator(indicator_path, data_path):
    with AlphaForest() as af:
        af.load_db(data_path)
        with open(indicator_path, 'r') as f:
            line = f.readline()
            while line and len(line) > 1:
                line = line[:-1]
                print(line)
                if ';' in line:
                    #缓存多个
                    factor_list = []
                    name_list = []
                    if line.startswith('['):
                        tmp = line[1:].split(']')
                        line = tmp[1].split(';')
                        tmp = tmp[0].split(';')
                        for t in tmp:
                            factor_list.append(t.strip())
                    else:
                        line = line.split(';')
                    for t in line:
                        factor_list.append(t.strip())
                        name_list.append(factor_list[-1].split('=')[0].strip())
                    af.cache_alpha_list(factor_list, name_list)
                else:
                    #缓存一个
                    tmp = line.split('=')
                    af.cache_alpha(tmp[0].strip(), tmp[1].strip())
                line = f.readline()


def read_alpha_tree_list(path):
    return read_alpha_list(path, lambda line: re.search(r"(?P<alpha>\w+): (?P<content>.*)", line).group('content'))


def write_alpha_tree_list(alphatree_list, path):
    try:
        f = open(path,'w')
        for id, alphatree in enumerate(alphatree_list):
            f.write("Alpha#%d: %s\n"%(id+1,alphatree))
    finally:
        f.close()

def filter_corr_alphatree_list(alphatree_score_list, filter_list, corr_fun, start_index = 0, cur_depth = 0, max_corr = 0.32, is_use_hero_feature = True):
    max_depth = len(filter_list)
    if cur_depth == max_depth:
        #print(filter_list)
        return filter_list[:]
    best_filter_list = None
    for index in range(start_index, len(alphatree_score_list) - (max_depth - cur_depth - 1)):

        is_corr = False
        cur_corr = 0
        for depth in range(0, cur_depth):
            cur_corr = max(cur_corr, corr_fun(filter_list[depth][0], alphatree_score_list[index][0]))
            if cur_corr >= max_corr:
                is_corr = True
                break
        print("%s%d/%d corr:%.4f" % (
        ''.join([" " for i in range(cur_depth)]), index, len(alphatree_score_list) - (max_depth - cur_depth - 1), cur_corr))
        if not is_corr:
            filter_list[cur_depth] = alphatree_score_list[index]
            cur_filter_list = filter_corr_alphatree_list(alphatree_score_list, filter_list, corr_fun, index + 1, cur_depth + 1, max_corr, is_use_hero_feature)
            if best_filter_list is None:
                best_filter_list = cur_filter_list
                # if cur_depth == 0:
                #     print(best_filter_list)
            elif cur_filter_list is not None:
                delta_score = 0
                for i in range(max_depth):
                    delta_score += best_filter_list[i][1] - cur_filter_list[i][1]
                if delta_score < 0:
                    best_filter_list = cur_filter_list
            if best_filter_list is not None and (is_use_hero_feature or cur_depth == max_depth - 1):
                return best_filter_list
                    # if cur_depth == 0:
                    #     print(best_filter_list)
    return best_filter_list




def read_alpha_tree_dict(path):
    alpha_tree = {}
    try:
        f = open(path,'rU')
        while True:
            line = f.readline()
            if line and len(line) > 0:
                tmp = line.split('=')
                key = tmp[0][:-1]
                line = tmp[1][1:-1]
                alpha_tree[key] = line
            else:
                break
    finally:
        f.close()
    return alpha_tree

def write_alpha_tree_dict(sub_alpha_dict, path):
    try:
        f = open(path,'w')
        for key, value in sub_alpha_dict.items():
            f.write("%s = %s\n"%(key, value))
    finally:
        f.close()

def read_alpha_list(path, line_format_function):
    alpha_tree = []
    try:
        f = open(path,'rU')
        while True:
            line = f.readline()
            if line and len(line) > 0:
                line = line_format_function(line)
                alpha_tree.append(line)
            else:
                break
    finally:
        f.close()
    return alpha_tree

def get_sub_alphatree(alphatree_Str, subalphatree_dict):
    sub_alpha_dict = {}
    for key, value in subalphatree_dict.items():
        if key in alphatree_Str:
            sub_alpha_dict[key] = value
    return sub_alpha_dict

#day pic-------------------------------------------------------------------------------------------------------------------
# def ini_day_price_pic(daybefore, sample_size, sign_name):
#     return AlphaPic(sign_name, ["(high / delay(close, 1))", "(low / delay(close, 1))"],
#              daybefore, sample_size)


# def get_day_price_pic(daybefore, sample_size, element_daybefore, element_days, sign_name, alpha_pic, column, max_std_scale):
#     open_elements = ["(delay(open, %d) / delay(close, %d))" % (-i, -i + 1) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     high_elements = ["(delay(high, %d) / delay(close, %d))" % (-i, -i + 1) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     low_elements = ["(delay(low, %d) / delay(close, %d))" % (-i, -i + 1) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     close_elements = ["(delay(close, %d) / delay(close, %d))" % (-i, -i + 1) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     return alpha_pic.get_k_line(sign_name, open_elements, high_elements, low_elements, close_elements, daybefore, sample_size,
#                                 column, max_std_scale)


# def ini_day_volume_pic(daybefore, sample_size, sign_name):
#     return AlphaPic(sign_name, ["(((delay(volume, 1) > 0) & (volume > 0)) ? (volume / delay(volume, 1)) : 1)"],
#              daybefore, sample_size)


# def get_day_volume_pic(daybefore, sample_size, element_daybefore, element_days, sign_name, alpha_pic, column, max_std_scale):
#     elements = ["delay((((delay(volume, 1) > 0) & (volume > 0)) ? (volume / delay(volume, 1)) : 1), %d)" % (-i) for i in range(-element_daybefore, -element_daybefore + element_days)]
#     return alpha_pic.get_trend(sign_name, elements, daybefore, sample_size, column, max_std_scale)


#week pic-------------------------------------------------------------------------------------------------------------------
# def ini_week_price_pic(daybefore, sample_size, sign_name):
#     return AlphaPic(sign_name, ["(ts_max(high, 4) / delay(close, 5))", "(ts_min(low, 4) / delay(close, 5))"],
#              daybefore, sample_size)
#
# def get_week_price_pic(daybefore, sample_size, element_daybefore, element_days, sign_name, alpha_pic,  column, max_std_scale):
#     open_elements = ["(delay(open, %d) / delay(close, %d))" % ((1 - i) * 5 - 1, (1 - i) * 5) for i
#                      in range(-element_daybefore, -element_daybefore + element_days)]
#     high_elements = ["delay((ts_max(high, 4) / delay(close, 5)), %d)" % (- i * 5) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     low_elements = ["delay((ts_min(low, 4) / delay(close, 5)), %d)" % (- i * 5) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     close_elements = ["delay((close / delay(close, 5)), %d)" % (- i * 5) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     return alpha_pic.get_k_line(sign_name, open_elements, high_elements, low_elements, close_elements, daybefore, sample_size,
#                                 column, max_std_scale)

# def ini_week_volume_pic(daybefore, sample_size, sign_name):
#     return AlphaPic(sign_name, ["(((delay(sum(volume, 4), 5) > 0) & (sum(volume, 4) > 0)) ? (sum(volume, 4) / delay(sum(volume, 4), 5)) : 1)"],
#              daybefore, sample_size)
#

# def get_week_volume_pic(daybefore, sample_size, element_daybefore, element_days, sign_name, alpha_pic, column, max_std_scale):
#     elements = ["delay((((delay(sum(volume, 4), 5) > 0) & (sum(volume, 4) > 0)) ? (sum(volume, 4) / delay(sum(volume, 4), 5)) : 1), %d)" % (- i * 5) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     return alpha_pic.get_trend(sign_name, elements, daybefore, sample_size, column, max_std_scale)
#
#month pic-------------------------------------------------------------------------------------------------------------------
# def ini_month_price_pic(daybefore, sample_size, sign_name):
#     return AlphaPic(sign_name, ["(ts_max(high, 24) / delay(close, 25))", "(ts_min(low, 24) / delay(close, 25))"],
#              daybefore, sample_size)
#
# def get_month_price_pic(daybefore, sample_size, element_daybefore, element_days, sign_name, alpha_pic,  column, max_std_scale):
#     open_elements = ["(delay(open, %d) / delay(close, %d))" % ((1 - i) * 25 - 1, (1 - i) * 25) for i
#                      in range(-element_daybefore, -element_daybefore + element_days)]
#     high_elements = ["delay((ts_max(high, 24) / delay(close, 25)), %d)" % (- i * 25) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     low_elements = ["delay((ts_min(low, 24) / delay(close, 25)), %d)" % (- i * 25) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     close_elements = ["delay((close / delay(close, 25)), %d)" % (- i * 25) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     return alpha_pic.get_k_line(sign_name, open_elements, high_elements, low_elements, close_elements, daybefore, sample_size,
#                                 column, max_std_scale)
#
#
# def ini_month_volume_pic(daybefore, sample_size, sign_name):
#     return AlphaPic(sign_name, ["(((delay(sum(volume, 24), 25) > 0) & (sum(volume, 24) > 0)) ? (sum(volume, 24) / delay(sum(volume, 24), 25)) : 1)"],
#              daybefore, sample_size)
#
#
# def get_month_volume_pic(daybefore, sample_size, element_daybefore, element_days, sign_name, alpha_pic, column, max_std_scale):
#     elements = ["delay((((delay(sum(volume, 24), 25) > 0) & (sum(volume, 24) > 0)) ? (sum(volume, 24) / delay(sum(volume, 24), 25)) : 1), %d)" % (- i * 25) for i in
#                      range(-element_daybefore, -element_daybefore + element_days)]
#     return alpha_pic.get_trend(sign_name, elements, daybefore, sample_size, column, max_std_scale)
