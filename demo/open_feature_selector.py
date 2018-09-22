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
    sample_size = feature_sample_time * feature_sample_size
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
        feature_list = ["%d=%s"%(id,line) for id, line in enumerate(mid_feature_list)]
        feature_name = ["%d"%id for id in range(len(mid_feature_list))]
        x = AlphaArray(valid_sign_name, feature_list, feature_name, daybefore, sample_size, 1)[:]
        y = AlphaArray(valid_sign_name, ["t=(%s > %.4f)" % (target_returns_name, delta_returns)], "t", daybefore, sample_size, 1)[:]
        # 归一化
        scaler = StandardScaler()
        x = scaler.fit_transform(x)

        left_alpha = .0
        right_alpha = 1.0

        while left_alpha + 0.0001 < right_alpha:
            cur_alpha = (left_alpha + right_alpha) * 0.5
            lasso = Lasso(alpha=cur_alpha)
            lasso.fit(x, y)
            coff_cnt = 0
            for id in range(len(mid_feature_list) - 1):
                if abs(lasso.coef_[id]) > 0.001:
                    coff_cnt += 1
            if coff_cnt > out_feature_num:
                left_alpha = cur_alpha
            elif coff_cnt < out_feature_num:
                right_alpha = cur_alpha
            else:
                break

        selection = []
        for id in range(len(mid_feature_list) - 1):
            if abs(lasso.coef_[id]) > 0.001:
                selection.append(id)
        with open(feature_path, 'w') as f:
            for sel in selection:
                print(bi.get_discrimination(mid_feature_list[sel]))
                f.write(mid_feature_list[sel] + '\n')


if __name__ == '__main__':
    files = os.listdir(conf_path)
    for file in files:
        if not os.path.isdir(file) and file.endswith(".ini"):
            config = configparser.ConfigParser()
            config.read(conf_path + "/" + file)
            select(config, file[:-4])