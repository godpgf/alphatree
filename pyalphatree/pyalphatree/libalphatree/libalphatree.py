# coding=utf-8
# author=godpgf
import ctypes
import platform
from ctypes import c_void_p, c_float, c_bool, c_int32

sysstr = platform.system()

import os
curr_path = os.path.dirname(os.path.abspath(os.path.expanduser(__file__)))
lib_path = curr_path + "/../../lib/"

try:
    alphatree = ctypes.windll.LoadLibrary(lib_path + 'libalphatree_api.dll') if sysstr =="Windows" else ctypes.cdll.LoadLibrary(lib_path + 'libalphatree_api.so')
except OSError as e:
    lib_path = curr_path + "/../../../lib/"
    alphatree = ctypes.windll.LoadLibrary(
        lib_path + 'libalphatree_api.dll') if sysstr == "Windows" else ctypes.cdll.LoadLibrary(
        lib_path + 'libalphatree_api.so')

alphatree.getMaxHistoryDays.restype = c_int32
alphatree.getAllDays.restype = c_int32
alphatree.getSignNum.restype = c_int32
alphatree.trainAndEvalAlphaGBDT.restype = c_float
alphatree.evalAlphaGBDT.restype = c_float
alphatree.getDistinguish.restype = c_float
alphatree.getConfidence.restype = c_float
alphatree.getCorrelation.restype = c_float

alphatree.optimizeDistinguish.restype = c_int32
alphatree.optimizeConfidence.restype = c_int32
