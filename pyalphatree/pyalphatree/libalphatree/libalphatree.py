# coding=utf-8
# author=godpgf
import ctypes
from ctypes import c_void_p, c_float

import os
curr_path = os.path.dirname(os.path.abspath(os.path.expanduser(__file__)))
lib_path = curr_path + "/../../lib/"

try:
    alphatree = ctypes.cdll.LoadLibrary(lib_path + "libalphatree_api.so")
except OSError,e:
    lib_path = curr_path + "/../../../lib/"
    alphatree = ctypes.cdll.LoadLibrary(lib_path + "libalphatree_api.so")
