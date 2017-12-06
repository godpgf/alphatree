# coding=utf-8
# author=godpgf
from ctypes import *

#todo delete later

class AlphaNode(object):
    def __init__(self, name, coff = '', avg = 0, std = 0):
        self.name = name
        self.coff = coff
        self.avg = avg
        self.std = std
        self.parent = None
        self.children = []

    #自己是第几个孩子
    def pre_child_index(self):
        if self.parent:
            for i,c in enumerate(self.parent.children):
                if c == self:
                    return i
        return -1

    def add_child(self, child):
        self.children.append(child)
        child.parent = self


class AlphaTree(object):
    def __init__(self, root):
        # if root.name == 'none':
        #     root = root.children[0]
        #     root.parent = None
        root.parent = None
        self.root = root
        #处理一下
        self._pre_process(self.root)

    def encode(self):
        encode_cache = list()
        self.encode_node(self.root, encode_cache)
        return "".join(encode_cache)


    def encode_node(self, node, encode_cache):
        if node.name == 'rank':
            name_list = list("rank_scale(rank_sort(")
            end_list = list('))')
        else:
            name_list = list(node.name)
            if len(node.children) > 0 or len(node.coff) > 0:
                name_list.append('(')
                end_list = list(')')
            else:
                end_list = list()

        encode_cache.extend(name_list)

        par_count = 0
        for child in node.children:
            if par_count > 0:
                encode_cache.append(',')
                encode_cache.append(' ')
            self.encode_node(child, encode_cache)
            par_count += 1

        if len(node.coff) > 0:
            if par_count > 0:
                encode_cache.append(',')
                encode_cache.append(' ')
            encode_cache.extend(list(node.coff))

        encode_cache.extend(end_list)

    def _pre_process(self, node):
        if node.name == 'rank_scale':
            node.name = 'rank'
            node.children[0].children[0].parent = node
            node.children[0] = node.children[0].children[0]
        for child in node.children:
            self._pre_process(child)


