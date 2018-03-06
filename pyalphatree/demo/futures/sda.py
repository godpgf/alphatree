#coding=utf-8
#author=godpgf
import tensorflow as tf
import numpy as np
import random

class MLIter(object):

    def get_data(self, batch_size):
        pass

    def skip(self, offset=1, is_relative=True):
        pass

    def get_dimension(self):
        pass

    def size(self):
        pass

    def is_valid(self):
        pass


def xavier_init(fan_in, fan_out, constant = 1):
    low = -constant * np.sqrt(6.0 / (fan_in + fan_out))
    high = constant * np.sqrt(6.0 / (fan_in + fan_out))
    return tf.random_uniform((fan_in, fan_out),
                             minval=low, maxval=high, dtype=tf.float32)


class AdditiveGaussianNoiseAutoencoder(object):

    def __init__(self, n_input, n_hidden, transfer_function = tf.nn.softplus, optimizer = tf.train.AdamOptimizer(), scale = 0.01):
        #输入数据的维度
        self.n_input = n_input
        self.n_hidden = n_hidden
        self.transfer = transfer_function
        self.scale = tf.placeholder(tf.float32)
        self.training_scale = scale
        network_weights = self._initialize_weights()
        self.weights = network_weights
        self.x = tf.placeholder(tf.float32, [None, self.n_input])
        # 编码
        self.hidden = self.transfer(tf.add(tf.matmul(self.x + scale * tf.random_normal((n_input,)),
                                                     self.weights['w1']),
                                           self.weights['b1']))
        # 解码
        self.reconstruction = tf.add(tf.matmul(self.hidden, self.weights['w2']), self.weights['b2'])

        # cost
        self.cost = 0.5 * tf.reduce_sum(tf.pow(tf.subtract(self.reconstruction, self.x), 2.0))
        self.optimizer = optimizer.minimize(self.cost)

    def _initialize_weights(self):
        all_weights = dict()
        all_weights['w1'] = tf.Variable(xavier_init(self.n_input, self.n_hidden))
        all_weights['b1'] = tf.Variable(tf.zeros([self.n_hidden], dtype = tf.float32))
        all_weights['w2'] = tf.Variable(tf.zeros([self.n_hidden, self.n_input], dtype = tf.float32))
        all_weights['b2'] = tf.Variable(tf.zeros([self.n_input], dtype = tf.float32))
        return all_weights

        # 优化参数

    def partial_fit(self, sess, X):
        cost, opt = sess.run((self.cost, self.optimizer), feed_dict={self.x: X,
                                                                          self.scale: self.training_scale
                                                                          })
        return cost

    def transform(self, sess, X):
        return sess.run(self.hidden, feed_dict = {self.x: X,
                                                       self.scale: self.training_scale
                                                       })

    # def get_weights(self):
    #     return self.sess.run(self.weights['w1'])
    #
    # def get_biases(self):
    #     return self.sess.run(self.weights['b1'])


class AdditiveGaussianNoiseAutoencoderAdjust(object):

    def __init__(self, sdae, n_output = None, transfer_function = tf.nn.softplus, optimizer = tf.train.AdamOptimizer()):
        self.sdae = sdae
        self.x = tf.placeholder(tf.float32, [None, sdae[0].n_input])
        self.y = tf.placeholder(tf.float32, [None, n_output]) if n_output else None
        self.transfer = transfer_function
        self.w = tf.Variable(xavier_init(sdae[-1].n_hidden, n_output)) if n_output else None
        self.b = tf.Variable(tf.zeros([n_output], dtype = tf.float32)) if n_output else None
        hidden = self.x
        for i in xrange(len(self.sdae)):
            hidden = self.transfer(tf.add(tf.matmul(hidden,
                                      self.sdae[i].weights['w1']),
                            self.sdae[i].weights['b1']))
        if n_output:
            self.pred = tf.add(tf.matmul(hidden, self.w), self.b)
            self.cost = 0.5 * tf.reduce_sum(tf.pow(tf.subtract(self.pred, self.y), 2.0))
        else:
            for i in xrange(len(self.sdae)):
                hidden = tf.add(tf.matmul(hidden,
                                          self.sdae[-1 - i].weights['w2']),
                                self.sdae[-1 - i].weights['b2'])
            self.pred = hidden
            self.cost = 0.5 * tf.reduce_sum(tf.pow(tf.subtract(self.pred, self.x), 2.0))
        self.optimizer = optimizer.minimize(self.cost)

    def partial_fit(self, sess, X, Y = None):
        if Y:
            cost, opt = sess.run((self.cost, self.optimizer), feed_dict={self.x: X, self.y: Y})
        else:
            cost, opt = sess.run((self.cost, self.optimizer), feed_dict={self.x: X})
        return cost

    def predict(self, sess, X):
        return sess.run(self.pred, feed_dict = {self.x: X})


class SDA(object):

    def __init__(self, n_input, hidden_size, n_output = None):
        self.hidden_size = hidden_size
        self.sdae = []

        for i in xrange(len(self.hidden_size)):
            if i == 0:
                ae = AdditiveGaussianNoiseAutoencoder(
                    n_input=n_input,
                    n_hidden=self.hidden_size[i],
                    transfer_function=tf.nn.softplus,
                    optimizer=tf.train.AdamOptimizer(learning_rate=0.001),
                    scale=0.01
                )
            else:
                ae = AdditiveGaussianNoiseAutoencoder(
                    n_input=self.hidden_size[i-1],
                    n_hidden=self.hidden_size[i],
                    transfer_function=tf.nn.softplus,
                    optimizer=tf.train.AdamOptimizer(learning_rate=0.001),
                    scale=0.01
                )
            ae._initialize_weights()
            self.sdae.append(ae)

        self.adj = AdditiveGaussianNoiseAutoencoderAdjust(self.sdae, n_output, transfer_function=tf.nn.softplus,
                    optimizer=tf.train.AdamOptimizer(learning_rate=0.001))


    def train(self, sess, x_iter, training_epochs = 1000, batch_size = 512):
        #一层层训练
        for i in xrange(len(self.hidden_size)):
            random_skip = 9
            for epoch in xrange(training_epochs):
                avg_cost = 0.
                total_batch = int(x_iter.size() / batch_size)
                offset = random.randint(0, random_skip)
                x_iter.skip(offset)
                for k in xrange(total_batch):
                    #得到当前层的输入数据
                    x_train = x_iter.get_data(batch_size)
                    offset = random.randint(0, random_skip)
                    x_iter.skip(offset)
                    if not x_iter.is_valid():
                        x_iter.skip(offset, False)
                    for j in xrange(i):
                        x_train = self.sdae[j].transform(sess, x_train)
                    
                    cost = self.sdae[i].partial_fit(sess, x_train)
                    avg_cost += cost / x_train.shape[1] * batch_size

                print ("Epoch:", '%04d' % (epoch + 1), "cost=", "{:.9f}".format(avg_cost))

    def adjust(self, sess, x_iter, y_iter = None, training_epochs = 1000, batch_size = 512):
        random_skip = 9

        for epoch in xrange(training_epochs):
            avg_cost = 0.
            total_batch = int(x_iter.size() / batch_size)
            offset = random.randint(0, random_skip)
            x_iter.skip(offset)
            if y_iter:
                y_iter.skip(offset)
            for k in xrange(total_batch):
                # 得到当前层的输入数据
                x_train = x_iter.get_data(batch_size)
                y_train = y_iter.get_data(batch_size) if y_iter else None
                offset = random.randint(0, random_skip)
                x_iter.skip(offset)
                if y_iter:
                    y_iter.skip(offset)
                if not x_iter.is_valid():
                    x_iter.skip(offset, False)
                    if y_iter:
                        y_iter.skip(offset, False)

                cost =self.adj.partial_fit(sess, x_train, y_train)
                avg_cost += cost / (y_train.shape[1] if y_iter else x_train.shape[1]) * batch_size

            print ("Epoch:", '%04d' % (epoch + 1), "cost=", "{:.9f}".format(avg_cost))

    def predict(self, sess, X):
        return self.adj.predict(sess, X)


