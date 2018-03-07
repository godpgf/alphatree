# coding=utf-8
# author=godpgf
from ctypes import *
from pyalphatree.libalphatree import alphatree
import numpy as np


class AlphaTransaction(object):

    def __enter__(self):
        alphatree.initializeAlphaTransaction()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        alphatree.releaseAlphaTransaction()


class Transaction(object):

    def __init__(self, money, fee, stock_num):
        self.tid = alphatree.useTransaction(c_float(money), c_float(fee))
        #self.stock_num = stock_num
        self.tcache = (c_float * (stock_num))()

    def __del__(self):
        alphatree.releaseTransaction(self.tid)

    def buy_stock(self, stock_index, price, weight = 1.0):
        alphatree.buyStock(self.tid, stock_index, c_float(price), c_float(weight))

    def short_stock(self, stock_index, price, weight = 1.0):
        alphatree.shortStock(self.tid, stock_index, c_float(price), c_float(weight))

    def sell_stock(self, stock_index, price, is_sell_buy, weight = 1.0):
        alphatree.sellStock(self.tid, stock_index, c_float(price), c_float(weight), c_bool(is_sell_buy))

    def get_balance(self, cur_price_list):
        for id, price in enumerate(cur_price_list):
            self.tcache[id] = c_float(price)
        return alphatree.getBalance(self.tid, self.tcache)