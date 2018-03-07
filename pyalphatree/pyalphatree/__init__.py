from .util import AlphaForest, AlphaArray, AlphaTransaction, Transaction
import re
from stdb import *

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

"""
def write_stock_data(path, codeProxy, dataProxy, classifiedProxy):
    data_size = len(dataProxy.trading_calender_int)
    stock_dict, market_dict, industry_dict, concept_dict = load_all_stock_flat(codeProxy, dataProxy,
                                                                               classifiedProxy)
    stock_size = len(stock_dict) + len(market_dict) + len(industry_dict) + len(concept_dict)

    sdb = stock_pb2.StockDB()
    sdb.days = data_size
    sdb.stockSize = stock_size

    element_name = ["open","high","low","close","volume","vwap","returns","amount"]
    for name in element_name:
        #se = stock_pb2.StockElement()
        #se.needDay = 0
        sdb.elements[name].needDay = 0

    stock_index = 0

    def fill_element(element_dict, names, bar):
        for name in names:
            b = bar[name]
            element = element_dict[name]
            for v in b:
                element.data.append(v)

    def fill_all(sdb, data_dict, element_dict, element_name, stock_type, name_2_index, stock_index):
        for key, value in data_dict.items():
            name_2_index[key] = stock_index
            stock_index += 1
            meta = sdb.metas.add()
            meta.code = key
            meta.stockType = stock_type
            fill_element(element_dict, element_name, value.bar)
        return stock_index

    stock_index = fill_all(sdb, market_dict, sdb.elements, element_name, stock_pb2.StockMeta.MARKET, sdb.stockIndex, stock_index)
    stock_index = fill_all(sdb, industry_dict, sdb.elements, element_name, stock_pb2.StockMeta.INDUSTRY, sdb.stockIndex, stock_index)
    stock_index = fill_all(sdb, concept_dict, sdb.elements, element_name, stock_pb2.StockMeta.CONCEPT, sdb.stockIndex, stock_index)
    for key, value in stock_dict.items():
        meta = sdb.metas.add()
        meta.code = key
        meta.stockType = stock_pb2.StockMeta.STOCK
        meta.marketIndex = sdb.stockIndex[value.market]
        meta.industryIndex = sdb.stockIndex[value.industry]
        meta.conceptIndex = sdb.stockIndex[value.concept]
        meta.totals = value.totals
        meta.earningRatios = value.earning_ratios
        fill_element(sdb.elements, element_name, value.bar)
        sdb.stockIndex[key] = stock_index
        stock_index += 1

    with open(path,'wb')as f:
        f.write(sdb.SerializeToString())
        """
