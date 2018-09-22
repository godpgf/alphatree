import os
import configparser
from pyalphatree import *
import numpy as np
from sklearn.linear_model import Lasso
from sklearn.preprocessing import StandardScaler

conf_path = 'conf'
data_path = "data"
from_feature_path = "formula"

target_returns_name = "open_down_returns"
valid_sign_name = "valid_sign"


def select(config, industry):
    delta_returns = float(config.get('feature', 'delta_return'))
    daybefore = int(config.get('feature', 'daybefore'))
    out_feature_num = int(config.get('feature', 'out_num'))
    mid_feature_path = "%s/mid_%s_feature.txt"%(from_feature_path, industry)
    feature_path = "%s/base_%s_feature.txt"%(from_feature_path, industry)
    feature_sample_size = int(config.get('feature', 'sample_size'))
    feature_sample_time = int(config.get('feature', 'sample_time'))
    feature_train_size = int(config.get('feature', 'train_size'))
    sample_size = feature_sample_time * feature_sample_size + feature_train_size - 1
    feature_support = float(config.get('feature','support'))
    feature_auc = float(config.get('feature', 'auc'))
    with AlphaForest() as af:
        af.load_db(data_path + "/" + industry)
        af.load_feature('miss')
        af.load_feature('date')
        af.load_feature(target_returns_name)
        af.load_sign(valid_sign_name)

        #得到候选特征，并保存在mid_feature_list
        bi = AlphaBI(valid_sign_name, daybefore, feature_sample_size, feature_sample_time,
                     feature_support, delta_returns, "rand", target_returns_name)
        mid_feature_list = []
        line_set = set()
        with open(mid_feature_path, 'r') as f:
            line = f.readline()
            while line:
                line = line[:-1]
                if line not in line_set:
                    dist = bi.get_discrimination(line)
                    if dist > feature_auc:
                        mid_feature_list.append(line)
                    line_set.add(line)
                line = f.readline()
        mid_feature_list.append("(delay(open, -1) / close)")

        #得到候选特征的取值，并保存在x
        feature_list = ["%d=%s" for id, line in enumerate(mid_feature_list)]
        feature_name = ["%d" for id in range(len(mid_feature_list))]
        x = AlphaArray(valid_sign_name, feature_list, feature_name, daybefore, sample_size, 1)[:]
        y = AlphaArray(valid_sign_name, ["t=(%s > %.4f)" % (target_returns_name, delta_returns)], daybefore, sample_size, 1)[:]
        # 归一化
        scaler = StandardScaler()
        x = scaler.fit_transform(x)

        #计算这个特征在每一天的被选中次数
        feature_score = np.zeros((len(mid_feature_list), feature_sample_time * feature_sample_size))
        for index in range(feature_sample_size * feature_sample_time):
            cur_x = x[index:index + feature_train_size]
            cur_y = y[index:index + feature_train_size]
            lasso = Lasso(alpha=.3)
            lasso.fit(cur_x, cur_y)
            #不考虑最后一个必选特征
            for fid in range(len(mid_feature_list) - 1):
                feature_score[fid][index] = lasso.coef_[fid]

        feature_score_sum = np.array([np.sum(feature_score[fid]) for fid in range(len(mid_feature_list))])
        selection = np.argsort(feature_score_sum)[-out_feature_num:]

        with open(feature_path, 'w') as f:
            for sel in selection:
                print(bi.get_discrimination(mid_feature_list[sel]))
                f.write(mid_feature_list[sel] + '\n')