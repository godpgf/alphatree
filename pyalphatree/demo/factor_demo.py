# coding=utf-8
# author=godpgf

from stdb import *
from pyalphatree import *
import numpy as np
import math
import time
import os
import json
import math


def cal_factor(line, codes):
    alpha = AlphaArray(codes, ["r=%s"%line], "r", 0, 20)
    data = alpha[:]
    value = np.mean(data.reshape(-1))
    if math.isnan(value) or math.fabs(value) < 0.000001:
        print line
        print value


if __name__ == '__main__':
    rootdir = "../../doc"
    with AlphaForest() as af:
        af.load_db("../data")
        codes = af.get_stock_codes()
        af.fill_codes(codes)
        list = os.listdir(rootdir)
        for file_name in list:
            path = os.path.join(rootdir, file_name)
            if os.path.isfile(path):
                with open(path, "r") as f:
                    line = f.readline()
                    last_factor = None
                    while line:
                        factor = line[:-1].split('#')[0].lstrip().rstrip()
                        cal_factor(factor, codes)
                        print af.process_alpha("noise_valid(%s, mfe_5, mae_5, 64)"%factor, 10, 20)
                        if last_factor:
                            print af.process_alpha("alpha_correlation(%s, %s)"%(factor, last_factor), 10, 20)
                        last_factor = factor
                        line = f.readline()