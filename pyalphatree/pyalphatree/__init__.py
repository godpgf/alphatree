# coding=utf-8
# author=godpgf
from .util import AlphaForest, AlphaArray, AlphaFilter, AlphaGBDT, AlphaPic
import re

def cache_base(data_path):
    with AlphaForest() as af:
        af.load_db(data_path)
        af.csv2binary(data_path, "date")
        af.csv2binary(data_path, "open")
        af.csv2binary(data_path, "high")
        af.csv2binary(data_path, "low")
        af.csv2binary(data_path, "close")
        af.csv2binary(data_path, "volume")
        af.csv2binary(data_path, "vwap")
        af.csv2binary(data_path, "returns")
        af.csv2binary(data_path, "amount")
        af.csv2binary(data_path, "turn")
        af.csv2binary(data_path, "tcap")
        af.csv2binary(data_path, "mcap")
        af.cache_miss()

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


