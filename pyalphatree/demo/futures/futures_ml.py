#coding=utf-8
#author=godpgf
import math
import numpy as np
from pyalphatree import *
from sda import *
from bp import *
import tensorflow as tf
import os

def smooth(low_list, high_list, max_len, value):
    if len(low_list) == 0:
        low_list.append(value)
    elif value < low_list[-1]:
        last_value = low_list[-1]
        cur_index = len(low_list) - 1
        while cur_index > 0 and low_list[cur_index-1] > value:
            low_list[cur_index] = low_list[cur_index-1]
            cur_index -= 1
        low_list[cur_index] = value
        if len(low_list) < max_len:
            low_list.append(last_value)

    if len(high_list) == 0:
        high_list.append(value)
    elif value > high_list[-1]:
        last_value = high_list[-1]
        cur_index = len(high_list) - 1
        while cur_index > 0 and high_list[cur_index-1] < value:
            high_list[cur_index] = high_list[cur_index-1]
            cur_index -= 1
        high_list[cur_index] = value
        if len(high_list) < max_len:
            high_list.append(last_value)


class TargetIter(object):

    def __init__(self, iter):
        self.iter = iter

    def skip(self, offset=1, is_relative=True):
        self.iter.skip(offset, is_relative)

    def is_valid(self):
        return self.iter.is_valid()

    def size(self):
        return self.iter.size()

    def get_data(self, batch_size):
        batch = []
        for _ in xrange(batch_size):
            batch.append([self.get_value()])
            self.skip()
        return np.array(batch)

    def get_value(self):
        return self.iter.get_value()


class AllFeatureIter(object):

    def __init__(self, high, low, close, volume, askprice, askvolume, bidprice, bidvolume, day_num):
        self.high = high
        self.low = low
        self.close = close
        self.volume = volume
        self.askprice = askprice
        self.askvolume = askvolume
        self.bidprice = bidprice
        self.bidvolume = bidvolume
        self.alliter = [high, low, close, volume, askprice, askvolume, bidprice, bidvolume]
        self.day_num = day_num
        self.std = None
        self.avg = None

    def skip(self, offset = 1, is_relative = True):
        for data in self.alliter:
            for value in data:
                value.skip(offset, is_relative)

    def is_valid(self):
        return self.close[0].is_valid()

    def size(self):
        return self.close[0].size()

    def get_data(self, batch_size):
        batch = []
        for _ in xrange(batch_size):
            batch.append(self.get_value())
            self.skip()
        return np.array(batch)

    def get_value(self):
        data = []
        for i in xrange(1, self.day_num):
            #print "%.4f %.4f"%(self.close[i-1].get_value(), self.close[i].get_value())
            high_t = math.log(self.high[i].get_value() / max(self.close[i].get_value(), 0.1))
            low_t = math.log(self.low[i].get_value() / max(self.close[i].get_value(),0.1))
            close_t = math.log(self.close[i].get_value()/max(self.close[i-1].get_value(),0.1))

            askprice_t = math.log(max(self.askprice[i].get_value(),0.1) / max(self.close[i-1].get_value(),0.1))
            bidprice_t = math.log(max(self.bidprice[i].get_value(),0.1) / max(self.close[i-1].get_value(),0.1))
            delta_price_t = bidprice_t - askprice_t
            avg_price_t = (bidprice_t + askprice_t) * 0.5

            volume_t = self.volume[i].get_value()
            askvolume_t = self.askvolume[i].get_value()
            bidvolume_t = self.bidvolume[i].get_value()

            volume_ratio_t = math.log(max(askvolume_t,0.1) / max(bidvolume_t,0.1))
            volume_depth_t = askvolume_t + bidvolume_t
            data.extend([high_t-low_t, close_t, delta_price_t, avg_price_t, volume_t, volume_ratio_t, volume_depth_t])
        data = np.array(data)
        if self.avg is not None and self.std is not None:
            return (data - self.avg) / self.std
        return data

    def normalize(self):
        self.avg = None
        self.std = None
        data_sum = None
        data_sqr_sum = None
        self.skip(0, False)
        while self.is_valid():
            value = self.get_value()
            if data_sum is None:
                data_sum = value
                data_sqr_sum = value * value
            else:
                data_sum += value
                data_sqr_sum += value * value
            self.skip()
        data_sum /= self.size()
        data_sqr_sum /= self.size()
        self.avg = data_sum
        self.std = np.sqrt(data_sqr_sum - data_sum * data_sum)
        self.skip(0, False)


class SDAAllFeatureIter(object):
    def __init__(self, sess, iter, sda):
        self.iter = iter
        self.sda = sda
        self.sess = sess

    def skip(self, offset=1, is_relative=True):
        self.iter.skip(offset, is_relative)

    def is_valid(self):
        self.iter.is_valid()

    def size(self):
        return self.iter.size()

    def get_data(self, batch_size):
        batch = []
        for _ in xrange(batch_size):
            batch.append(self.get_value())
            self.skip()
        x = np.array(batch)
        return self.sda.predict(self.sess, x)

    def get_value(self):
        return self.iter.get_value()



def get_train_iter(day_before, sample_num, day_num = 6):
    #得到平滑后的数据
    high = [AlphaIter("filter", "high", day_before, sample_num, i-day_num).smooth() for i in range(day_num)]
    low = [AlphaIter("filter", "low", day_before, sample_num, i-day_num).smooth() for i in range(day_num)]
    close = [AlphaIter("filter", "close", day_before, sample_num, i-day_num).smooth() for i in range(day_num)]
    volume = [AlphaIter("filter", "volume", day_before, sample_num, i-day_num).smooth() for i in range(day_num)]
    askprice = [AlphaIter("filter", "askprice", day_before, sample_num, i-day_num).smooth() for i in range(day_num)]
    askvolume = [AlphaIter("filter", "askvolume", day_before, sample_num, i-day_num).smooth() for i in range(day_num)]
    bidprice = [AlphaIter("filter", "bidprice", day_before, sample_num, i-day_num).smooth() for i in range(day_num)]
    bidvolume = [AlphaIter("filter", "bidvolume", day_before, sample_num, i-day_num).smooth() for i in range(day_num)]

    #得到取样数据并归一化
    feature_iter = AllFeatureIter(high, low, close, volume, askprice, askvolume, bidprice, bidvolume, day_num)
    feature_iter.normalize()
    return feature_iter, TargetIter(AlphaIter("filter", "target", day_before, sample_num))

def get_test_iter(day_before, sample_num, avg, std, day_num = 6):
    #得到平滑后的数据
    high = [AlphaIter("filter", "high", day_before, sample_num, i-day_num) for i in range(day_num)]
    low = [AlphaIter("filter", "low", day_before, sample_num, i-day_num) for i in range(day_num)]
    close = [AlphaIter("filter", "close", day_before, sample_num, i-day_num) for i in range(day_num)]
    volume = [AlphaIter("filter", "volume", day_before, sample_num, i-day_num) for i in range(day_num)]
    askprice = [AlphaIter("filter", "askprice", day_before, sample_num, i-day_num) for i in range(day_num)]
    askvolume = [AlphaIter("filter", "askvolume", day_before, sample_num, i-day_num) for i in range(day_num)]
    bidprice = [AlphaIter("filter", "bidprice", day_before, sample_num, i-day_num) for i in range(day_num)]
    bidvolume = [AlphaIter("filter", "bidvolume", day_before, sample_num, i-day_num) for i in range(day_num)]

    #得到取样数据并归一化
    feature_iter = AllFeatureIter(high, low, close, volume, askprice, askvolume, bidprice, bidvolume, day_num)
    feature_iter.avg = avg
    feature_iter.std = std
    return feature_iter, TargetIter(AlphaIter("filter", "target", day_before, sample_num))

def get_test_feature_iter(day_before, sample_num, avg, std, day_num = 6, code = "IF"):
    high = [FeatureIter("high", day_before + day_num - i, sample_num, code) for i in range(day_num)]
    low = [FeatureIter("low", day_before + day_num - i, sample_num, code) for i in range(day_num)]
    close = [FeatureIter("close", day_before + day_num - i, sample_num, code) for i in range(day_num)]
    volume = [FeatureIter("volume", day_before + day_num - i, sample_num, code) for i in range(day_num)]
    askprice = [FeatureIter("askprice", day_before + day_num - i, sample_num, code) for i in range(day_num)]
    askvolume = [FeatureIter("askvolume", day_before + day_num - i, sample_num, code) for i in range(day_num)]
    bidprice = [FeatureIter("bidprice", day_before + day_num - i, sample_num, code) for i in range(day_num)]
    bidvolume = [FeatureIter("bidvolume", day_before + day_num - i, sample_num, code) for i in range(day_num)]

    #得到取样数据并归一化
    feature_iter = AllFeatureIter(high, low, close, volume, askprice, askvolume, bidprice, bidvolume, day_num)
    feature_iter.avg = avg
    feature_iter.std = std
    return feature_iter, FeatureIter("returns",day_before, sample_num, code)

def train(sample_days = 250, before_days = 50, encode_epochs = 16, adjust_epochs = 16, training_epochs = 64, sign_ratio = 0.06, path = "save/model.ckpt", is_minute = True):
    scale = 1 if is_minute else 60
    train_x, train_y = get_train_iter(before_days * 4 * 60 * scale, sample_days * 4 * 60 * scale)
    test_x, test_y = get_test_iter(0, before_days * 4 * 60 * scale, train_x.avg, train_x.std)

    #统计样本
    cnt_1, cnt_0_5, cnt_0 = 0, 0, 0
    print "训练数据、测试数据大小:%d、%d"%(train_y.size(),test_y.size())
    while train_y.is_valid():
        v = train_y.get_value()
        train_y.skip()
        if v > 0.9:
            cnt_1 += 1
        elif v < 0.1:
            cnt_0 += 1
        else:
            cnt_0_5 += 1
    train_y.skip(0, False)
    print "训练数据中正、负、中立样本个数分别是%d、%d、%d"%(cnt_1,cnt_0,cnt_0_5)
    cnt_1, cnt_0_5, cnt_0 = 0, 0, 0
    while test_y.is_valid():
        v = test_y.get_value()
        test_y.skip()
        if v > 0.9:
            cnt_1 += 1
        elif v < 0.1:
            cnt_0 += 1
        else:
            cnt_0_5 += 1
    test_y.skip(0, False)
    print "测试数据中正、负、中立样本个数分别是%d、%d、%d" % (cnt_1, cnt_0, cnt_0_5)

    with tf.Session() as sess:
        #sda = SDA(5 * 7, [24, 16])
        bp = BP(35,[18], 1)

        saver = tf.train.Saver()
        fileName = path.split('/')[-1]
        if path and os.path.exists(path.replace(fileName, 'checkpoint')):
            saver.restore(sess, path)
        else:
            # init = tf.initialize_all_variables()
            init = tf.global_variables_initializer()
            sess.run(init)

        #sda.train(sess, train_x, encode_epochs)
        #sda.adjust(sess, train_x, training_epochs=adjust_epochs)
        #bp.train(sess, SDAAllFeatureIter(sess, train_x, sda), train_y, training_epochs)
        bp.train(sess, train_x, train_y, training_epochs)

        #得到发出信号的阈值
        sign_num = int(test_x.size() * sign_ratio)
        low_list = []
        high_list = []

        test_x.skip(0, False)
        while test_x.is_valid():
            x = test_x.get_value()
            test_x.skip()
            smooth(low_list, high_list, sign_num, bp.predict(sess, np.array([x]))[0][0])

        #保存模型
        saver.save(sess, path)
        with open(path.replace(fileName, "config.txt"), 'w') as f:
            avg_str = []
            for avg in train_x.avg:
                avg_str.append("%.16f"%avg)
            std_str = []
            for std in train_x.std:
                std_str.append("%.16f"%std)
            f.write(" ".join(avg_str) + '\n')
            f.write(" ".join(std_str) + '\n')
            f.write("%.16f %.16f\n"%(low_list[-1], high_list[-1]))


def test(test_days = 50, before_days = 0, test_batch = 10, path = "save/model.ckpt", is_minute = True):
    fileName = path.split('/')[-1]
    scale = 1 if is_minute else 60
    with open(path.replace(fileName, "config.txt"), 'r') as f:
        avg_str = f.readline()[:-1].split(' ')
        avg = np.array([float(_) for _ in avg_str])
        std_str = f.readline()[:-1].split(' ')
        std = np.array([float(_) for _ in std_str])
        tmp = f.readline()[:-1].split(' ')
        low_sign, high_sign = float(tmp[0]), float(tmp[1])

        test_x, test_y = get_test_iter(before_days * 4 * 60 * scale, test_days * 4 * 60 * scale, avg, std)
        with tf.Session() as sess:
            #sda = SDA(5 * 7, [24, 16])
            bp = BP(35, [18], 1)
            saver = tf.train.Saver()
            saver.restore(sess, path)
            up_right_cnt = 0
            up_all_cnt = 0
            down_right_cnt = 0
            down_all_cnt = 0
            base_up_right_cnt = 0
            base_up_all_cnt = 0
            base_down_right_cnt = 0
            base_down_all_cnt = 0
            while test_x.is_valid():
                x = test_x.get_data(test_batch)
                y = test_y.get_data(test_batch)
                #pred = bp.predict(sess, sda.predict(sess, np.array(x)))
                pred = bp.predict(sess, np.array(x))
                for i in xrange(len(pred)):
                    p_, y_ = pred[i][0], y[i][0]
                    if pred[i][0] <= low_sign:
                        down_all_cnt += 1
                        if y[i][0] < 0.4:
                            down_right_cnt += 1
                    if pred[i][0] >= high_sign:
                        up_all_cnt += 1
                        if y[i][0] > 0.6:
                            up_right_cnt += 1
                    if pred[i][0] < 0.5:
                        if y[i][0] < 0.5:
                            base_down_right_cnt += 1
                        base_down_all_cnt += 1
                    if pred[i][0] > 0.5:
                        if y[i][0] > 0.5:
                            base_up_right_cnt += 1
                        base_up_all_cnt += 1
            print "信号发出时：正确上涨数/预测上涨数=%d/%d=%.4f"%(up_right_cnt,up_all_cnt,up_right_cnt/float(up_all_cnt))
            print "信号发出时：正确下跌数/预测下跌数=%d/%d=%.4f"%(down_right_cnt,down_all_cnt,down_right_cnt/float(down_all_cnt))
            print "总体上：正确上涨数/预测上涨数=%d/%d=%.4f"%(base_up_right_cnt,base_up_all_cnt,base_up_right_cnt/float(base_up_all_cnt))
            print "总体上：正确下跌数/预测下跌数=%d/%d=%.4f"%(base_down_right_cnt,base_down_all_cnt,base_down_right_cnt/float(base_down_all_cnt))

def back_trace_second(test_days = 50, before_days = 0, test_batch = 100, path = "save/model.ckpt", reduce_returns = -0.0007, empty_returns = -0.002):
    fileName = path.split('/')[-1]

    act_return = 100
    market_return = 100
    last_act_return = act_return

    with open(path.replace(fileName, "config.txt"), 'r') as f:
        avg_str = f.readline()[:-1].split(' ')
        avg = np.array([float(_) for _ in avg_str])
        std_str = f.readline()[:-1].split(' ')
        std = np.array([float(_) for _ in std_str])
        tmp = f.readline()[:-1].split(' ')
        low_sign, high_sign = float(tmp[0]), float(tmp[1])
        with open(path.replace(fileName, "returns.csv"), 'w') as cf:
            cf.write("act_return,market_return\n")
            test_x, test_y = get_test_feature_iter(before_days, test_days * 4 * 60 * 60, avg, std)
            with tf.Session() as sess:
                #sda = SDA(5 * 7, [24, 16])
                bp = BP(35, [18], 1)
                saver = tf.train.Saver()
                saver.restore(sess, path)
                cur_batch = 0
                #-1做空、0持币、1做多
                flag = 0
                while test_x.is_valid():
                    x = test_x.get_data(test_batch)
                    y = test_y.get_data(test_batch)

                    cur_batch += test_batch
                    print "%d / %d %.4f / %.4f"%(cur_batch,test_x.size(),act_return,market_return)

                    #pred = bp.predict(sess, sda.predict(sess, np.array(x)))
                    pred = bp.predict(sess, np.array(x))
                    for i in xrange(len(pred)):
                        #写策略
                        if flag == 0:
                            if pred[i][0] <= low_sign:
                                flag = -1
                                last_act_return = act_return
                            if pred[i][0] >= high_sign:
                                flag = 1
                                last_act_return = act_return
                        elif flag == 1:
                            if pred[i][0] <= low_sign:
                                flag = -1
                                #扣手续费
                                act_return *= (1 + reduce_returns)
                                last_act_return = act_return
                            # elif pred[i][0] <= low_sign + (0.5-low_sign) * 0.032:
                            #     flag = 0
                            #     act_return *= (1 + reduce_returns)
                            #     last_act_return = act_return
                        elif flag == -1:
                            if pred[i][0] >= high_sign:
                                flag = 1
                                act_return *= (1 + reduce_returns)
                                last_act_return = act_return
                            # elif pred[i][0] >= high_sign - (high_sign-0.5)*0.032:
                            #     flag = 0
                            #     act_return *= (1 + reduce_returns)
                            #     last_act_return = act_return

                        market_return *= (1 + y[i])
                        if flag == 1:
                            act_return *= (1 + y[i])
                        elif flag == -1:
                            act_return /= (1 + y[i])
                        elif flag != 0 and act_return < last_act_return * (1 + empty_returns):
                            #止损并扣手续费
                            act_return *= (1 + reduce_returns)
                            flag = 0

                        cf.write("%.4f,%.4f\n"%(act_return, market_return))


if __name__ == '__main__':
    af = AlphaForest()
    af.load_db("../../cffex_if")

    train(training_epochs = 32, sample_days=500, before_days=100)
    # test(test_days=100, before_days=500)
    #back_trace_second()

