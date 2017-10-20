from .util import AlphaForest
from .util import AlphaTree, AlphaNode
import re

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