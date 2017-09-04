# 股票技术面特征提取和直接验证

## 安装

make all

## 技术公式

####编码和解码公式并计算出结果

最全的使用方法可以参见pyalphatree下的test.py

```python
# coding=utf-8
# author=godpgf
from stdb import *
from pyalphatree import AlphaForest

#先加载互联网上的股票数据
af = AlphaForest()
af.load_data()

#解码字符串创建公式树并得到id,程序里面叫做alpha tree
alphatree_id = af.create_alphatree("min(delay(before_high, 1), before_high)", {"before_high":"delay(high, 2)"})
#编码公式树成字符串
encodestr = af.encode_alphatree(alphatree_id)
#多线程开始计算数据并得到计算结果, res是计算结果, code是参与计算的股票, day_before表示我们使用的是几天前的数据开始运算, sample_size计算结果中需要包含多少天的数据
res,code = af.cal_alphatree(alphatree_id,day_before = 0, sample_size = 1)
```

## 验证有效性

```python
print "score %.4f"%af.eval_alphatree(alphatree_id)
```