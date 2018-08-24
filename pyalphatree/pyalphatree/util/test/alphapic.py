from pyalphatree.util.alphaarray import AlphaArray
import numpy as np

class AlphaPic(object):
    def __init__(self, codes, daybefore, sample_size, column = 16, max_element_days = 10):
        self.codes = codes
        self.max_elememt_days = max_element_days
        self.column = column
        self.refresh(daybefore, sample_size)
        self.day_price_avg, self.day_price_std, self.day_volume_avg, self.day_volume_std = self.get_nor_price_volume(
            self.day_line)
        self.week_price_avg, self.week_price_std, self.week_volume_avg, self.week_volume_std = self.get_nor_price_volume(
            self.week_line)
        self.month_price_avg, self.month_price_std, self.month_volume_avg, self.month_volume_std = self.get_nor_price_volume(
            self.month_line)

    def refresh(self, daybefore, sample_size):
        self.day_line, self.week_line, self.month_line = self.get_k_line(daybefore, sample_size, self.max_elememt_days, self.codes)


    def get_pic(self, day_index, element_days, stock_index, pic_list):
        datas = np.zeros((element_days, 5))
        last_index = day_index + self.max_elememt_days
        for i in range(element_days):
            for j in range(5):
                datas[i][j] = self.day_line[last_index - (element_days - i - 1)][j][stock_index]
        p, v = self._data2pic(datas, self.day_price_avg, self.day_price_std, self.day_volume_avg, self.day_volume_std)
        pic_list[0].append(p)
        pic_list[1].append(v)

        last_index = day_index + self.max_elememt_days * 5
        for i in range(element_days):
            for j in range(5):
                datas[i][j] = self.week_line[last_index - (element_days - i - 1) * 5][j][stock_index]
        p, v = self._data2pic(datas, self.week_price_avg, self.week_price_std, self.week_volume_avg, self.week_volume_std)
        pic_list[2].append(p)
        pic_list[3].append(v)

        last_index = day_index + self.max_elememt_days * 25
        for i in range(element_days):
            for j in range(5):
                datas[i][j] = self.month_line[last_index - (element_days - i - 1) * 25][j][stock_index]
        p, v = self._data2pic(datas, self.month_price_avg, self.month_price_std, self.month_volume_avg,
                         self.month_volume_std)
        pic_list[4].append(p)
        pic_list[5].append(v)

    @classmethod
    def get_k_line(cls, daybefore, day_num, element_days, codes):
        day_line = AlphaArray(codes, ["o = (open / delay(close, 1))",
                                      "h = (high / delay(close, 1))",
                                      "l = (low / delay(close, 1))",
                                      "c = (close / delay(close, 1))",
                                      "v = volume"], ["o", "h", "l", "c", "v"], daybefore, day_num + element_days * 1)[:]
        week_line = AlphaArray(codes, ["o = (delay(open, 4) / delay(close, 5))",
                                       "h = (ts_max(high, 4) / delay(close, 5))",
                                       "l = (ts_min(low, 4) / delay(close, 5))",
                                       "c = (close / delay(close, 5))",
                                       "v = sum(volume, 4)"], ["o", "h", "l", "c", "v"], daybefore,
                               day_num + element_days * 5)[:]
        month_line = AlphaArray(codes, ["o = (delay(open, 24) / delay(close, 25))",
                                        "h = (ts_max(high, 24) / delay(close, 25))",
                                        "l = (ts_min(low, 24) / delay(close, 25))",
                                        "c = (close / delay(close, 25))",
                                        "v = sum(volume, 24)"], ["o", "h", "l", "c", "v"], daybefore,
                                day_num + element_days * 25)[:]
        return day_line, week_line, month_line

    @classmethod
    def get_nor_price_volume(cls, datas):
        all_price = []
        all_volume = []
        for d in datas:
            all_price.append(d[0])
            all_price.append(d[1])
            all_price.append(d[2])
            all_price.append(d[3])
            all_volume.append(d[4])
        all_price = np.array(all_price)
        all_volume = np.array(all_volume)
        mean_price = np.mean(all_price)
        std_price = np.std(all_price)
        mean_volume = np.mean(all_volume)
        std_volume = np.std(all_volume)
        return mean_price, std_price, mean_volume, std_volume

    def _data2index(self, data, mean, std):
        return int((min(max((data - mean) / (std * 2), -1.0), 1.0) + 1.0) * 0.4999 * self.column)

    def _data2pic(self, datas, mean_price, std_price, mean_volume, std_volume):
        element_days = len(datas)
        price = np.zeros((element_days * 3, self.column))
        volume = np.zeros((element_days * 3, self.column))

        for i in range(element_days):
            price[i * 3][self._data2index(datas[i][0], mean_price, std_price)] = 1
            h = self._data2index(datas[i][1], mean_price, std_price)
            l = self._data2index(datas[i][2], mean_price, std_price)
            while l <= h:
                price[i * 3 + 1][l] = 1
                l += 1
            price[i * 3 + 2][self._data2index(datas[i][3], mean_price, std_price)] = 1

            v_index = self._data2index(datas[i][4], mean_volume, std_volume)
            if i == 0:
                volume[i * 3][v_index] = 1
                volume[i * 3 + 1][v_index] = 1
                volume[i * 3 + 2][v_index] = 1
            else:
                last_v_index = self._data2index(datas[i - 1][4], mean_volume, std_volume)
                volume[i * 3][int((v_index - last_v_index) * 1.0 / 3.0 + last_v_index)] = 1
                volume[i * 3 + 1][int((v_index - last_v_index) * 2.0 / 3.0 + last_v_index)] = 1
                volume[i * 3 + 2][v_index] = 1
        return price.T.reshape((self.column, element_days * 3, 1)), volume.T.reshape((self.column, element_days * 3, 1))