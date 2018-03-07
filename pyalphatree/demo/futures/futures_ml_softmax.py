#coding=utf-8
#author=godpgf
import math
import numpy as np
from pyalphatree import *
from sda import *
from bp_softmax import *
import tensorflow as tf
import os
from futures_des import *


def smooth(low_list, high_list, max_len, value):
    if len(low_list) == 0:
        low_list.append(value[0])
    elif value[0] > low_list[-1]:
        last_value = low_list[-1]
        cur_index = len(low_list) - 1
        while cur_index > 0 and low_list[cur_index-1] < value[0]:
            low_list[cur_index] = low_list[cur_index-1]
            cur_index -= 1
        low_list[cur_index] = value[0]
        if len(low_list) < max_len:
            low_list.append(last_value)

    if len(high_list) == 0:
        high_list.append(value[2])
    elif value[2] > high_list[-1]:
        last_value = high_list[-1]
        cur_index = len(high_list) - 1
        while cur_index > 0 and high_list[cur_index-1] < value[2]:
            high_list[cur_index] = high_list[cur_index-1]
            cur_index -= 1
        high_list[cur_index] = value[2]
        if len(high_list) < max_len:
            high_list.append(last_value)


def write_array(f, data):
    data_str = []
    for v in data:
        data_str.append("%.16f" % v)
    f.write(" ".join(data_str) + '\n')

def read_array(f):
    data_str = f.readline()[:-1].split(' ')
    return np.array([float(d) for d in data_str])


def train(sample_days = 250, before_days = 50,  training_epochs = 64, sign_ratio = 0.002, path = "save/model.ckpt", time_offset = 0, batch_size = 4096):
    scale = 1 if time_offset == 2 else (6 if time_offset == 1 else 60)

    train_x, train_y = get_sign_data("filter", before_days * 4 * 60 * scale, sample_days * 4 * 60 * scale)
    test_x, test_y = get_sign_data("filter", 0, before_days * 4 * 60 * scale)

    #统计样本
    cnt_1, cnt_0_5, cnt_0 = 0, 0, 0
    assert len(train_x) == len(train_y)
    assert len(test_x) == len(test_y)
    print "训练数据、测试数据大小:%d、%d"%(len(train_y),len(test_y))

    start_index = 0
    data_num = len(train_y)
    while start_index < data_num:
        data_size = min(data_num - start_index, batch_size)
        y = train_y[start_index:start_index + data_size]
        for i in xrange(len(y)):
            if y[i][2] > 0.9:
                cnt_1 += 1
            if y[i][0] > 0.9:
                cnt_0 += 1
            if y[i][1] > 0.9:
                cnt_0_5 += 1
        start_index += data_size

    print "训练数据中正、负、中立样本个数分别是%d、%d、%d"%(cnt_1,cnt_0,cnt_0_5)
    cnt_1, cnt_0_5, cnt_0 = 0, 0, 0
    start_index = 0
    data_num = len(test_y)
    while start_index < data_num:
        data_size = min(data_num - start_index, batch_size)
        y = test_y[start_index:start_index + data_size]
        for i in xrange(len(y)):
            if y[i][2] > 0.9:
                cnt_1 += 1
            if y[i][0] > 0.9:
                cnt_0 += 1
            if y[i][1] > 0.9:
                cnt_0_5 += 1
        start_index += data_size
    print "测试数据中正、负、中立样本个数分别是%d、%d、%d" % (cnt_1, cnt_0, cnt_0_5)

    avg, std = train_x.normalize()
    low, high = train_x.smooth()

    with tf.Session() as sess:
        bp = BPSoftmax(35,[18], 3)

        saver = tf.train.Saver()
        fileName = path.split('/')[-1]
        if path and os.path.exists(path.replace(fileName, 'checkpoint')):
            saver.restore(sess, path)
        else:
            init = tf.global_variables_initializer()
            sess.run(init)

        bp.train(sess, train_x, train_y, test_x, test_y, avg, std, low, high, training_epochs, batch_size)

        #得到发出信号的阈值
        sign_num = int(len(test_x) * sign_ratio)
        low_list = []
        high_list = []

        for i in range(len(test_y)):
            x = test_x[i]
            smooth(low_list, high_list, sign_num, bp.predict(sess, np.array([x]))[0])

        #保存模型
        saver.save(sess, path)
        with open(path.replace(fileName, "config.txt"), 'w') as f:
            write_array(f, avg)
            write_array(f, std)
            write_array(f, low)
            write_array(f, high)
            write_array(f,[low_list[-1], high_list[-1]])

def back_trace(sample_days = 50, before_days = 0,  path = "save/model.ckpt", time_offset = 0, batch_size = 4096):
    scale = 1 if time_offset == 2 else (6 if time_offset == 1 else 60)
    back_x, back_y = get_seq_data("IF", before_days * 4 * 60 * scale, sample_days * 4 * 60 * scale)

    fileName = path.split('/')[-1]
    with open(path.replace(fileName, "config.txt"), 'r') as f:
        avg = read_array(f)
        std = read_array(f)
        low = read_array(f)
        high = read_array(f)
        low_high = read_array(f)

    with AlphaTransaction():
        transaction = Transaction(100000, 0.0002, 1)
        with tf.Session() as sess:
            bp = BPSoftmax(35,[18], 3)
            saver = tf.train.Saver()
            saver.restore(sess, path)

            start_index = 0
            data_num = len(back_y)
            #统计胜率
            all_down_cnt = 0
            real_down_cnt = 0
            all_up_cnt = 0
            real_up_cnt = 0
            #-1做空，0空仓，1做多
            state = 0
            #之前持仓的最高价
            last_act_money = None
            #之前最高价
            pre_max_money = 0
            #最大回撤
            max_drop_down = 0
            while start_index < data_num:
                data_size = min(data_num - start_index, batch_size)
                x = back_x[start_index:start_index + data_size]
                y = back_y[start_index:start_index + data_size]
                pred = bp.predict(sess, (np.minimum(np.maximum(x, low), high) - avg) / std)

                for i in xrange(data_size):
                    money = transaction.get_balance([y[i][0]])
                    #if pred[i][2] > low_high[1]:
                    #    print "%.4f %.4f %.4f %.4f"%(pred[i][0], low_high[0], pred[i][2], low_high[1])
                    if pred[i][0] > low_high[0]:
                        if state != -1:
                            if state == 1:
                                transaction.sell_stock(0, y[i][0], True)
                            #做空
                            transaction.short_stock(0, y[i][0])
                            state = -1
                            #统计
                            all_down_cnt += 1
                            #print "down %.4f"% y[i][1]
                            if y[i][1] < 0:
                            #if y[i][1] <= -get_min_wave():
                            #    print y[i][1]
                                real_down_cnt += 1
                        last_act_money = money
                    elif pred[i][2] > low_high[1]:
                        if state != 1:
                            if state == -1:
                                transaction.sell_stock(0, y[i][0], False)
                            #做多
                            transaction.buy_stock(0, y[i][0])
                            state = 1
                            #统计
                            all_up_cnt += 1
                            #print "up %.4f"% y[i][1]
                            if y[i][1] > 0:
                            #if y[i][1] >= get_min_wave():
                                #print y[i][1]
                                real_up_cnt += 1
                        last_act_money = money
                    else:
                        if state != 0:
                            #止损
                            last_act_money = max(last_act_money, money)
                            if (money - last_act_money) / last_act_money < -get_stop_loss():
                                transaction.sell_stock(0, y[i][0], (state == 1))
                                state = 0

                    pre_max_money = max(pre_max_money, money)
                    max_drop_down = max((pre_max_money - money) / pre_max_money, max_drop_down)
                print "账户余额：%.4f，最大回撤：%.4f，做多胜率%.4f，做空胜率%.4f"%(money, max_drop_down, real_up_cnt / float(max(all_up_cnt,1)), real_down_cnt/float(max(all_down_cnt, 1)))
                start_index += data_size
# def back_trace(test_days = 50, before_days = 0, test_batch = 100, path = "save/model.ckpt", reduce_returns = -0.0002, empty_returns = -0.002, time_offset = 0):
#     fileName = path.split('/')[-1]
#     scale = 1 if time_offset == 2 else (6 if time_offset == 1 else 60)
#     act_return = 100
#     market_return = 100
#     last_act_return = 100
#
#     with open(path.replace(fileName, "config.txt"), 'r') as f:
#         avg_str = f.readline()[:-1].split(' ')
#         avg = np.array([float(_) for _ in avg_str])
#         std_str = f.readline()[:-1].split(' ')
#         std = np.array([float(_) for _ in std_str])
#         tmp = f.readline()[:-1].split(' ')
#         low_sign, high_sign = float(tmp[0]), float(tmp[1])
#         with open(path.replace(fileName, "returns.csv"), 'w') as cf:
#             cf.write("act_return,market_return\n")
#             test_x, test_y = get_test_feature_iter(before_days * 4 * 60 * scale, test_days * 4 * 60 * scale, avg, std)
#             with tf.Session() as sess:
#                 #sda = SDA(5 * 7, [24, 16])
#                 bp = BPSoftmax(35, [18], 3)
#                 saver = tf.train.Saver()
#                 saver.restore(sess, path)
#                 cur_batch = 0
#                 #-1做空、0持币、1做多
#                 flag = 0
#                 all_size = test_y.size()
#                 act_time = 0
#                 while test_x.is_valid():
#                     if test_batch > all_size:
#                         test_batch = all_size
#                     all_size -= test_batch
#                     x = test_x.get_data(test_batch)
#                     y = test_y.get_data(test_batch)
#
#                     cur_batch += test_batch
#                     print "%d(%d): %d / %d %.4f / %.4f"%(act_time, flag, cur_batch,test_x.size(),act_return,market_return)
#
#                     #pred = bp.predict(sess, sda.predict(sess, np.array(x)))
#                     pred = bp.predict(sess, np.array(x))
#                     for i in xrange(len(pred)):
#                         #写策略
#                         if flag == 0:
#                             if pred[i][0] >= low_sign:
#                                 flag = -1
#                                 last_act_return = act_return
#                             if pred[i][2] >= high_sign:
#                                 flag = 1
#                                 last_act_return = act_return
#                         elif flag == 1:
#                             if pred[i][0] >= low_sign:
#                                 flag = -1
#                                 #扣手续费
#                                 act_return *= (1 + reduce_returns)
#                                 last_act_return = act_return
#                                 act_time += 1
#                             elif pred[i][0] >= low_sign - (low_sign-0.5) * 0.06:
#                                 flag = 0
#                                 act_return *= (1 + reduce_returns)
#                                 last_act_return = act_return
#                                 act_time += 1
#                         elif flag == -1:
#                             if pred[i][2] > high_sign:
#                                 flag = 1
#                                 act_return *= (1 + reduce_returns)
#                                 last_act_return = act_return
#                                 act_time += 1
#                             elif pred[i][0] >= high_sign - (high_sign-0.5)*0.06:
#                                 flag = 0
#                                 act_return *= (1 + reduce_returns)
#                                 last_act_return = act_return
#                                 act_time += 1
#
#                         market_return *= (1 + y[i][0])
#                         if flag == 1:
#                             act_return *= (1 + y[i][0])
#                         elif flag == -1:
#                             act_return /= (1 + y[i][0])
#                         if flag != 0 and act_return < last_act_return * (1 + empty_returns):
#                             #止损并扣手续费
#                             act_return *= (1 + reduce_returns)
#                             act_time += 1
#                             flag = 0
#                         if flag != 0 and act_return > last_act_return:
#                             last_act_return = act_return
#
#                         cf.write("%.4f,%.4f\n"%(act_return, market_return))


if __name__ == '__main__':
    with AlphaForest() as af:
        af.load_db("../../cffex_if")
        train(training_epochs=16)
        #back_trace()

