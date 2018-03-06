#coding=utf-8
#author=godpgf

import tensorflow as tf
import numpy as np
import random


class BP(object):

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
        self.pred = self.addLayer(self.l[-1], hidden_size[-1], n_output, activity_function)
        self.cost = tf.reduce_mean(tf.reduce_sum(tf.square(self.y - self.pred),
                    reduction_indices=[1]))
        self.optimizer = optimizer.minimize(self.cost)

    def partial_fit(self, sess, X, Y):
        cost, opt, pred = sess.run((self.cost, self.optimizer, self.pred), feed_dict={self.x: X, self.y: Y})
        return cost, opt, pred

    def predict(self, sess, X):
        return sess.run(self.pred, feed_dict = {self.x: X})

    def train(self, sess, x_iter, y_iter, training_epochs = 1000, batch_size = 512):
        random_skip = 9
        for epoch in xrange(training_epochs):
            avg_cost = 0.
            total_batch = int(x_iter.size() / batch_size)
            offset = random.randint(0, random_skip)
            x_iter.skip(offset)
            y_iter.skip(offset)
            for k in xrange(total_batch):
                # 得到当前层的输入数据
                x_train = x_iter.get_data(batch_size)
                y_train = y_iter.get_data(batch_size)
                offset = random.randint(0, random_skip)
                x_iter.skip(offset)
                y_iter.skip(offset)
                if not x_iter.is_valid():
                    x_iter.skip(offset, False)
                    y_iter.skip(offset, False)

                cost, opt, pred = self.partial_fit(sess, x_train, y_train)
                avg_cost += cost / x_train.shape[1] * batch_size

            print ("Epoch:", '%04d' % (epoch + 1), "cost=", "{:.9f}".format(avg_cost))

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