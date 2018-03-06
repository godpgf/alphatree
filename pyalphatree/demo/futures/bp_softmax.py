#coding=utf-8
#author=godpgf

import tensorflow as tf
import numpy as np
import random
import  math


class BPSoftmax(object):

    def __init__(self, n_input, hidden_size, n_output=1, activity_function=tf.nn.sigmoid, optimizer = tf.train.GradientDescentOptimizer(0.1)):
        self.hidden_size = hidden_size
        self.n_input = n_input
        self.n_output = n_output
        self.x = tf.placeholder(tf.float32, [None, self.n_input])
        self.y = tf.placeholder(tf.float32, [None, self.n_output])
        self.l = []
        for i in xrange(len(self.hidden_size)):
            if i == 0:
                self.l.append(self.addLayer(self.x, self.n_input, hidden_size[i], activity_function))
            else:
                self.l.append(self.addLayer(self.l[i-1], hidden_size[i-1], hidden_size[i], activity_function))
        self.pred = self.addLayer(self.l[-1], hidden_size[-1], n_output, tf.nn.sigmoid)

        # 交叉熵：-tf.reduce_sum(y_ * tf.log(y)是一个样本的，外面的tf.reduce_mean是batch的
        self.cost = tf.reduce_mean(-tf.reduce_sum(self.y * tf.log(self.pred) + (1 - self.y) * tf.log(1 - self.pred), reduction_indices=[1]))
        #self.cost = tf.reduce_mean(tf.nn.softmax_cross_entropy_with_logits(self.pred, self.y))

        # 规定训练的方法：注意：使用GradientDescentOptimizer适合上述的误差项
        self.optimizer = tf.train.GradientDescentOptimizer(0.5).minimize(self.cost)

    def partial_fit(self, sess, X, Y):
        cost, opt, pred = sess.run((self.cost, self.optimizer, self.pred), feed_dict={self.x: X, self.y: Y})
        return cost, opt, pred

    def predict(self, sess, X):
        return sess.run(self.pred, feed_dict = {self.x: X})

    def train(self, sess, x_train, y_train, x_test, y_test, avg_x, std_x, low_x, high_x, training_epochs = 1000, batch_size = 512):
        for epoch in xrange(training_epochs):
            avg_cost = 0.
            avg_win = 0.0
            avg_loss = 0.0
            start_index = 0
            data_num = len(y_train)
            while start_index < data_num:
                data_size = min(batch_size, data_num - start_index)
                # 得到当前层的输入数据
                x = (np.minimum(np.maximum(x_train[start_index:start_index + data_size], low_x), high_x) - avg_x) / std_x
                #x = (x_train[start_index:start_index + data_size] - avg_x) / std_x
                y = y_train[start_index:start_index + data_size]

                cost, opt, pred = self.partial_fit(sess, x, y)
                avg_cost += cost / x.shape[1] * batch_size
                start_index += data_size

            start_index = 0
            data_num = len(y_test)
            while start_index < data_num:
                data_size = min(batch_size, data_num - start_index)
                # 得到当前层的输入数据
                x = (np.minimum(np.maximum(x_test[start_index:start_index + data_size], low_x), high_x) - avg_x) / std_x
                #x = (x_train[start_index:start_index + data_size] - avg_x) / std_x
                y = y_test[start_index:start_index + data_size]
                pred = self.predict(sess, x)
                loss_cnt = 0
                real_loss_cnt = 0
                win_cnt = 0
                real_win_cnt = 0
                for i in xrange(data_size):
                    p = pred[i]
                    t = y[i]

                    if p[0] > 0.5 and p[0] > max(p[1], p[2]):
                        loss_cnt += 1
                        if t[0] > 0.5:
                            real_loss_cnt += 1
                    if p[2] > 0.5 and p[2] > max(p[0], p[1]):
                        win_cnt += 1
                        if t[2] > 0.5:
                            real_win_cnt += 1

                    # if t[0] > 0.5:
                    #     loss_cnt += 1
                    #     if p[0] > 0.5 and p[0] > max(p[1], p[2]):
                    #         real_loss_cnt += 1
                    # if t[2] > 0.5:
                    #     win_cnt += 1
                    #     if p[2] > 0.5 and p[2] > max(p[0], p[1]):
                    #         real_win_cnt += 1

                avg_win += float(real_win_cnt)/max(win_cnt,1) * (float(data_size) / data_num)
                avg_loss += float(real_loss_cnt)/max(loss_cnt,1) * (float(data_size) / data_num)
                #print "%.4f %.4f"%(float(real_loss_cnt)/max(loss_cnt,1), float(real_win_cnt)/max(win_cnt,1))
                start_index += data_size

            print ("Epoch:", '%04d' % (epoch + 1), "cost=", "{:.9f}".format(avg_cost), "avg_win=", "{:.9f}".format(avg_win), "avg_loss=", "{:.9f}".format(avg_loss))


    @classmethod
    def addLayer(cls, inputData, inSize, outSize, activity_function=None):
        Weights = tf.Variable(tf.random_normal([inSize, outSize]))
        basis = tf.Variable(tf.zeros([1, outSize]) + 0.1)
        weights_plus_b = tf.matmul(inputData, Weights) + basis
        if activity_function is None:
            ans = weights_plus_b
        else:
            ans = activity_function(weights_plus_b)
        return ans