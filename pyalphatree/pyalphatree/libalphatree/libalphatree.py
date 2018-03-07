# coding=utf-8
# author=godpgf
import ctypes
from ctypes import c_void_p, c_float, c_bool, c_int32

import os
curr_path = os.path.dirname(os.path.abspath(os.path.expanduser(__file__)))
lib_path = curr_path + "/../../lib/"

try:
    alphatree = ctypes.cdll.LoadLibrary(lib_path + "libalphatree_api.so")
except OSError,e:
    lib_path = curr_path + "/../../../lib/"
    alphatree = ctypes.cdll.LoadLibrary(lib_path + "libalphatree_api.so")

alphatree.optimizeAlpha.restype = c_float
alphatree.getSignNum.restype = c_int32
alphatree.getBalance.restype = c_float
# alphatree.createSignFeatureIter.restype = c_void_p
# alphatree.createFeatureIter.restype = c_void_p
# alphatree.iterIsValid.restype = c_bool
# alphatree.iterSize.restype = c_int32

#alphatree.iterValue.restype = c_float
# alphatree.iterSmooth.restype = c_float
