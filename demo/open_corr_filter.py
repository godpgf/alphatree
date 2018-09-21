import os
import configparser
from pyalphatree import *

conf_path = 'conf'
data_path = "data"
from_feature_path = "formula"

target_returns_name = "open_down_returns"
valid_sign_name = "valid_sign"


def filter(config, industry):
    delta_returns = float(config.get('feature', 'delta_return'))
    daybefore = int(config.get('feature', 'daybefore'))
    # daybefore = 0
    max_corr = float(config.get('feature', 'corr'))
    out_feature_num = int(config.get('feature', 'out_num'))
    mid_feature_path = "%s/mid_%s_feature.txt"%(from_feature_path, industry)
    feature_path = "%s/base_%s_feature.txt"%(from_feature_path, industry)
    feature_sample_size = int(config.get('feature', 'sample_size'))
    feature_sample_time = int(config.get('feature', 'sample_time'))
    feature_support = float(config.get('feature','support'))
    feature_auc = float(config.get('feature', 'auc'))
    with AlphaForest() as af:
        af.load_db(data_path + "/" + industry)
        af.load_feature('miss')
        af.load_feature('date')
        af.load_feature(target_returns_name)
        af.load_sign(valid_sign_name)

        bi = AlphaBI(valid_sign_name, daybefore, feature_sample_size, feature_sample_time,
                     feature_support, delta_returns, "rand", target_returns_name)
        alphatree_score_list = []
        line_set = set()
        with open(mid_feature_path, 'r') as f:
            line = f.readline()
            while line:
                line = line[:-1]
                if line not in line_set:
                    dist = bi.get_discrimination(line)
                    if dist > feature_auc:
                        alphatree_score_list.append((line, dist))
                    print(line, dist)
                    line_set.add(line)
                line = f.readline()
            alphatree_score_list = sorted(alphatree_score_list, key=lambda elm :elm[1], reverse=True)
        filter_list = [None] * out_feature_num

        def corr_fun(a, b):
            corr = abs(bi.get_correlation(a, b))
            return corr
        filter_list = filter_corr_alphatree_list(alphatree_score_list, filter_list, corr_fun, max_corr=max_corr)
        if filter_list is not None and len(filter_list) and filter_list[-1] is not None:
            with open(feature_path, 'w') as f:
                for i in range(len(filter_list)):
                    # print(filter_list[i][1])
                    print(bi.get_discrimination(filter_list[i][0]))
                    f.write(filter_list[i][0] + '\n')


if __name__ == '__main__':
    files = os.listdir(conf_path)
    for file in files:
        if not os.path.isdir(file) and file.endswith(".ini"):
            config = configparser.ConfigParser()
            config.read(conf_path + "/" + file)
            filter(config, file[:-4])
