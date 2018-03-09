#coding=utf-8
#author=godpgf

import tensorflow as tf
import numpy as np


class LSTM(object):

    def __init__(self, input_size, time_step, rnn_units, output_size, batch_size,
                 activity_function=None, loss_function=lambda pred, y: tf.reduce_mean(tf.square(tf.reshape(pred,[-1])-tf.reshape(y, [-1]))),
                 optimizer=tf.train.AdamOptimizer(0.0006)):
        self.input_size = input_size
        self.output_size = output_size
        self.time_step = time_step
        self.batch_size = batch_size

        self.x = tf.placeholder(tf.float32, shape=[batch_size, time_step, input_size])
        self.y = tf.placeholder(tf.float32, shape=[batch_size, time_step, output_size])
        self.w_in = tf.Variable(tf.random_normal([input_size, rnn_units[0]]))
        self.w_out = tf.Variable(tf.random_normal([rnn_units[-1], output_size]))
        self.b_in = tf.Variable(tf.constant(0.1, shape=[rnn_units[0],]))
        self.b_out = tf.Variable(tf.constant(0.1,shape=[output_size,]))
        self.keep_prob = tf.placeholder(tf.float32)

        input = tf.reshape(self.x, [-1, input_size])
        input_rnn = tf.matmul(input, self.w_in) + self.b_in
        input_rnn = tf.reshape(input_rnn, [batch_size, time_step, rnn_units[0]])

        def unit_lstm(hidden_size):
            # 定义一层 LSTM_cell，只需要说明 hidden_size, 它会自动匹配输入的 X 的维度
            lstm_cell = tf.rnn.BasicLSTMCell(num_units=hidden_size, forget_bias=1.0, state_is_tuple=True)
            # 添加 dropout layer, 一般只设置 output_keep_prob
            lstm_cell = tf.rnn.DropoutWrapper(cell=lstm_cell, input_keep_prob=1.0, output_keep_prob=self.keep_prob)
            return lstm_cell

        # 调用 MultiRNNCell 来实现多层 LSTM
        mlstm_cell = tf.rnn.MultiRNNCell([unit_lstm(rnn_units[i]) for i in range(len(rnn_units))], state_is_tuple=True)

        # 用全零来初始化state
        init_state = mlstm_cell.zero_state(batch_size, dtype=tf.float32)
        output_rnn, state = tf.nn.dynamic_rnn(mlstm_cell, inputs=input_rnn, initial_state=init_state, time_major=False)
        output = tf.reshape(output_rnn, [-1, rnn_units[-1]])  # 作为输出层的输入
        self.pred = tf.matmul(output, self.w_out) + self.b_out
        if activity_function:
            self.pred = activity_function(self.pred)
        self.pred = tf.reshape(self.pred, [-1, time_step, output_size])

        self.cost = loss_function(self.pred, self.y)
        self.optimizer = optimizer.minimize(self.loss)

    def partial_fit(self, sess, X, Y):
        cost, opt, pred = sess.run((self.cost, self.optimizer, self.pred), feed_dict={self.x: X, self.y: Y})
        return cost, opt, pred

    def predict(self, sess, X):
        return sess.run(self.pred, feed_dict = {self.x: X})

    def eval(self, sess, X, Y):
        return sess.run(self.cost, feed_dict = {sess.x: X, sess.y: Y})

    def train(self, sess, x_train, y_train, x_test, y_test, avg_x, std_x, low_x, high_x, training_epochs = 1000):
        for epoch in xrange(training_epochs):
            avg_cost = 0.
            start_index = 0
            data_num = len(y_train)
            data_len = self.batch_size * self.time_step
            while start_index < data_num - data_len:
                x = x_train[start_index: start_index + data_len]
                y = y_train[start_index: start_index + data_len]
                x = x.reshape([-1, self.time_step, self.input_size])
                y = y.reshape([-1, self.time_step, self.output_size])
                cost, opt, pred =self.partial_fit(sess, x, y)
                start_index += data_len

            start_index = 0
            data_num = len(y_test)
            while start_index < data_num -data_len:
                x = x_test[start_index: start_index + data_len]
                y = y_test[start_index: start_index + data_len]
                x = x.reshape([-1, self.time_step, self.input_size])
                y = y.reshape([-1, self.time_step, self.output_size])
                cost = self.eval(sess, x, y)
                avg_cost += cost / (data_num / data_len)
                start_index += data_len

            print ("Epoch:", '%04d' % (epoch + 1), "cost=", "{:.9f}".format(avg_cost))