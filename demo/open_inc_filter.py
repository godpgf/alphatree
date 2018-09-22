import configparser
import os
from stdb import *
from pyalphatree import *
import math

conf_path = 'conf'
data_path = "data"
from_feature_path = "formula"
in_feature_path = "output"

delta_return_name = "open_down_returns"
delta_return_line = "(((delay(close, -%d) / delay(open, -1)) - 1) - (1 - (delay(ts_min(low, %d), -%d) / delay(open, -1))))"
valid_sign_name = "valid_sign"
valid_sign_line = '(delay(((volume > 0) & (abs(returns) < 0.09)), -1) & (volume > 0))'


def pred(config, industry):
    market = config.get('info','market')
    # download_industry(config.get('info','code').split(','), market, data_path + "/" + industry)
    cache_base(data_path + "/" + industry)
    daybefore = int(config.get('feature', 'daybefore'))
    # daybefore = 0
    base_feature_path = "%s/base_%s_feature.txt"%(from_feature_path, industry)
    inc_feature_path = in_feature_path + "/" + config.get('feature', 'in_feature')
    to_feature_path = "%s/inc_%s_feature.txt"%(from_feature_path, industry)
    feature_sample_size = int(config.get('feature', 'sample_size'))
    feature_sample_time = int(config.get('feature', 'sample_time'))
    feature_support = float(config.get('feature','support'))
    feature_inc_auc = float(config.get('feature', 'inc_auc'))
    sample_group = int(config.get('feature', 'sample_group'))
    hold_days = int(config.get('feature', 'hold_day'))
    delta_return = float(config.get('feature', 'delta_return'))
    max_corr = float(config.get('feature', 'corr'))
    sample_group = int(config.get('feature', 'sample_group'))

    def update_features(bi, line, cur_dist, features):
        # 如果有相关性很强的数据，就不再添加。如果此数据得分很高，就
        corr_line = None
        for key, value in features.items():
            corr = abs(bi.get_correlation(key, line))
            if corr_line is None:
                corr_line = (key, value, corr)
            elif corr > corr_line[2]:
                corr_line = (key, value, corr)

        if corr_line is None or corr_line[2] < (1 - max_corr):
            # 没有相关性特别强的历史特征
            features[line] = cur_dist
            print(line, cur_dist)
            return True
        elif corr_line[1] < cur_dist:
            # 相关性特别强的历史特征比自己差
            features.pop(corr_line[0])
            features[line] = cur_dist
            print(line, cur_dist)
            return True
        return False

    def read_features(bi_list, feature_list):
        features = {}
        try:
            with open(to_feature_path, 'r') as f:
                line = f.readline()
                while line:
                    line = line[:-1]
                    for id, bi in enumerate(bi_list):
                        max_inc = bi.get_discrimination_inc(line, feature_list)
                        if id < len(bi_list) - 1:
                            if max_inc < 0.0:
                                break

                    # print("cur_dist1:%.4f"%cur_dist1)
                    if max_inc < feature_inc_auc:
                        line = f.readline()
                        continue
                    update_features(bi, line, max_inc, features)
                    line = f.readline()

            with open(to_feature_path, 'w') as f:
                for line, d in features.items():
                    f.write("%s\n" % line)
        except:
            pass
        return features

    def insert_line(bi_list, line, features, feature_list):
        dist1 = 0
        for id, bi in enumerate(bi_list):
            if id < len(bi_list) - 1:
                dist1 = bi.get_discrimination_inc(line, feature_list)
                if dist1 < 0.0:
                    break
            else:
                dist1 = bi.get_discrimination_inc(line, feature_list)

        if dist1 > feature_inc_auc * 0.5:
            print(dist1)
        if dist1 > feature_inc_auc and not math.isinf(dist1) and not math.isnan(dist1):
            line = bi_list[-1].optimize_discrimination_inc(line, feature_list)
            dist = bi_list[-1].get_discrimination_inc(line, feature_list)
            assert dist > feature_inc_auc

            if line not in features:
                # features[line] = dist
                if update_features(bi_list[-1], line, dist, features):
                    print("dist %.4f" % (dist))
                    with open(to_feature_path, 'w') as f:
                        for line, d in features.items():
                            f.write("%s\n" % line)

    with AlphaForest() as af:
        af.load_db(data_path + "/" + industry)
        codes = af.get_stock_codes()
        codes.sort()
        if len(codes) == 0:
            codes = af.get_market_codes()
        af.cache_alpha(delta_return_name, delta_return_line % (hold_days,hold_days-1,hold_days))
        af.cache_codes_sign(valid_sign_name, valid_sign_line, codes)

        bi_list = []
        if sample_group > 1:
            for i in range(sample_group):
                min_value = i / float(sample_group);
                max_value = (i + 1) / float(sample_group);
                tmp = "((rand > %.4f) & (rand < %.4f))"%(min_value, max_value)
                af.cache_codes_sign("%s_%d"%(valid_sign_name, i),"(%s & %s)"%(valid_sign_line, tmp), codes)
                af.load_sign("%s_%d"%(valid_sign_name, i))
                bi_list.append(AlphaBI("%s_%d"%(valid_sign_name, i), daybefore + hold_days, feature_sample_size, feature_sample_time, feature_support * sample_group, delta_return, "rand", delta_return_name))
        af.load_feature('miss')
        af.load_feature('rand')
        af.load_feature('date')
        af.load_feature(delta_return_name)
        af.load_sign(valid_sign_name)
        bi_list.append(AlphaBI(valid_sign_name, daybefore + hold_days, feature_sample_size, feature_sample_time,
                               feature_support, delta_return, "rand", delta_return_name))


        feature_list = []
        with open(base_feature_path, 'r') as f:
            line = f.readline()
            index = 0
            while line:
                feature_list.append("t%d"%index)
                af.cache_alpha(feature_list[-1], line[:-1])
                af.load_feature(feature_list[-1])
                index += 1
                line = f.readline()
            features = read_features(bi_list, feature_list)

        def pred_fun(line):
            if af.get_max_history_days(line) <= 75 and line not in features:
                insert_line(bi_list, line, features, feature_list)

        process_file_name = data_path + "/" + industry + "/inc_process.txt"
        try:
            pf = open(process_file_name, 'r')
            line_num = int(pf.read())
            pf.close()
        except Exception as e:
            line_num = 0

        cur_line_num = 0
        with open(inc_feature_path, 'r') as f:
            line = f.readline()
            while line:
                if cur_line_num >= line_num:
                    pred_fun(line[:-1])
                cur_line_num += 1
                line = f.readline()
                if cur_line_num % 100 == 0:
                    if cur_line_num % 10000 == 0:
                        print(cur_line_num)
                    with open(process_file_name, 'w') as pf:
                        pf.write("%d"%cur_line_num)


if __name__ == '__main__':
    files = os.listdir(conf_path)
    for file in files:
        if not os.path.isdir(file) and file.endswith(".ini"):
            config = configparser.ConfigParser()
            config.read(conf_path + "/" + file)
            pred(config, file[:-4])
