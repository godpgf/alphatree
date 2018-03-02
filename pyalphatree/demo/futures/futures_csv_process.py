# coding=utf-8
# author=godpgf
# 将期货csv处理成可以本系统可以使用的文件格式
import os
from pyalphatree import *


def _avg(value_list):
    sum = 0
    cnt = 0
    for v in value_list:
        if v > 0:
            sum += v
            cnt += 1
    return "%.4f"%(sum / max(float(cnt), 1.0))


def conbine_csv(rootdir, topath, time_offset = 0):
    list = os.listdir(rootdir)
    list.sort()

    column = ["ASKPRICE1",
              "ASKVOLUME1",
              "AVERAGEPRICE",
              "BIDPRICE1",
              "BIDVOLUME1",
              "HIGHESTPRICE",
              "LASTPRICE",
              "LOWERLIMITPRICE",
              "LOWESTPRICE",
              "MILLISEC",
              "OPENINTEREST",
              "OPENPRICE",
              "PRECLOSEPRICE",
              "PREOPENINTEREST",
              "PRESETTLEMENTPRICE",
              "TURNOVER", "UPPERLIMITPRICE", "VOLUME"]

    inst_map = {}
    str_list = ["date","open","high","low","close","volume","amount","askprice","askvolume","bidprice","bidvolume"]
    # c_map = {}
    date_num = 0
    with open(topath, "w") as wf:
        wf.write(','.join(str_list) + '\n')
        for i in range(0, len(list)):
            path = os.path.join(rootdir, list[i])
            print list[i]
            with open(path, 'r') as rf:
                line = rf.readline()[:-2]
                # if len(c_map) == 0:
                c_map = {}
                tmp = line.split(',')
                for j in xrange(len(tmp)):
                    c_map[tmp[j]] = j
                line = rf.readline()[:-2]

                data_map = {}
                vol_map = {}
                while line and len(line) > 1:
                    tmp = line.split(',')
                    date = tmp[c_map['TRADINGDAY']]
                    time = tmp[c_map['TIME']]
                    dt = date + time.replace(":", "")
                    if time_offset > 0:
                        dt = dt[:-time_offset]
                    data = [dt]
                    nonull = lambda a : a if a and len(a) else '0'
                    #open
                    data.append(nonull(tmp[c_map["LASTPRICE"]]))
                    #high
                    data.append(nonull(tmp[c_map["LASTPRICE"]]))
                    #low
                    data.append(nonull(tmp[c_map["LASTPRICE"]]))
                    #close
                    data.append(nonull(tmp[c_map["LASTPRICE"]]))
                    #volume
                    data.append(nonull(tmp[c_map["VOLUME"]]))
                    #amount
                    data.append(nonull(tmp[c_map["TURNOVER"]]))
                    #askprice
                    data.append([float(nonull(tmp[c_map["ASKPRICE1"]]))])
                    #askvolume
                    data.append(nonull(tmp[c_map["ASKVOLUME1"]]))
                    #bidprice
                    data.append([float(nonull(tmp[c_map['BIDPRICE1']]))])
                    #bidvolume
                    data.append(nonull(tmp[c_map['BIDVOLUME1']]))
                    #for j in xrange(len(column)):
                    #    data.append(tmp[c_map[column[j]]] if tmp[c_map[column[j]]] and len(tmp[c_map[column[j]]]) else '0')

                    inst = tmp[c_map['INSTRUMENTID']]
                    if inst not in data_map:
                        data_map[inst] = {}
                        vol_map[inst] = 0
                    if dt not in data_map[inst]:
                        data_map[inst][dt] = data
                        vol_map[inst] += float(data[6])
                    else:
                        #avg = lambda a, b: str((float(a) + float(b)) * 0.5)
                        data_map[inst][dt][2] = max(data_map[inst][dt][2], data[2])
                        data_map[inst][dt][3] = min(data_map[inst][dt][3], data[3])
                        data_map[inst][dt][4] = data[4]
                        data_map[inst][dt][5] = str(long(data_map[inst][dt][5]) + long(data[5]))
                        data_map[inst][dt][6] = str(float(data_map[inst][dt][6]) + float(data[6]))
                        data_map[inst][dt][7].extend(data[7])
                        data_map[inst][dt][8] = str(long(data_map[inst][dt][8]) + long(data[8]))
                        data_map[inst][dt][9].extend(data[9])
                        data_map[inst][dt][10] = str(long(data_map[inst][dt][10]) + long(data[10]))
                        vol_map[inst] += float(data[6])
                    line = rf.readline()[:-2]
                max_inst = None
                max_vol = 0
                for key, value in vol_map.items():
                    if value > max_vol:
                        max_vol = value
                        max_inst = key
                data_list = [value for key, value in data_map[max_inst].items()]
                data_list = sorted(data_list, key=lambda d: d[0])
                for data in data_list:
                    data[7] = _avg(data[7])
                    data[9] = _avg(data[9])
                #if tmp[c_map['TRADINGDAY']] == "20141201":
                #    for k in xrange(1, len(data_list)):
                #        if data_list[k][0] < data_list[k-1][0]:
                #            print data_list[k][0]
                #            print data_list[k-1][0]
                #第一个数据成交量设置为0
                data = [str(long(data_list[0][0])-1),data_list[0][1],data_list[0][1],data_list[0][1],data_list[0][1],'0','0',data_list[0][7],'0',data_list[0][9],'0']
                if list[i].startswith("IF%s"%data[0][:8]):
                    wf.write(','.join(data) + '\n')
                    date_num += 1
                #data_list.insert(0, data)
                last_date = str(long(data_list[0][0])-1)
                assert data_list[0][0][:8] in path
                assert data_list[0][-1][:8] in path
                for data in data_list:
                    if data[0] < last_date:
                        print data[0]
                        print last_date
                    assert data[0] > last_date
                    last_date = data[0]
                    if list[i].startswith("IF%s" % data[0][:8]):
                        wf.write(','.join(data) + '\n')
                        date_num += 1
        with open("../../cffex_if/codes.csv", 'w') as f:
            f.write("code,market,industry,price,cap,pe,days\nIF,,,,,,%d\n" % date_num)




if __name__ == "__main__":
    # with open("../../cffex_if/IF.csv", "r") as rf:
    #     line = rf.readline()
    #     line = rf.readline()[:-1]
    #     lnum = 0
    #     while line:
    #         if line.startswith("2014120107"):
    #             print "%d %s"%(lnum,line[:-1])
    #         #if (lnum > 11357355 and lnum < 11357457) or (lnum > 11373560 and lnum < 11373665):
    #         #    print line[:-1]
    #         line = rf.readline()
    #         lnum += 1

    conbine_csv("/home/godpgf/data/CFFEX_IF", "../../cffex_if/IF.csv", 0)

    af = AlphaForest()
    af.load_db("../../cffex_if")
    af.csv2binary("../../cffex_if", "date")
    af.csv2binary("../../cffex_if", "open")
    af.csv2binary("../../cffex_if", "high")
    af.csv2binary("../../cffex_if", "low")
    af.csv2binary("../../cffex_if", "close")
    af.csv2binary("../../cffex_if", "volume")
    af.csv2binary("../../cffex_if", "amount")
    af.csv2binary("../../cffex_if", "askprice")
    af.csv2binary("../../cffex_if", "askvolume")
    af.csv2binary("../../cffex_if", "bidprice")
    af.csv2binary("../../cffex_if", "bidvolume")
    af.cache_alpha("returns", "((close - delay(close, 1)) / delay(close, 1))")
    af.cache_alpha("target", "((returns > 0.0002) ? 1 : ((returns < -0.0002) ? 0 : 0.5))")
    af.cache_sign("filter","((product(volume, 6) > 0) & amplitude_sample(returns, 0.0002))")
    # af.cache_sign("test_filter","(returns < 0)")
    # af.cache_sign("test_filter","(delay(returns, 2) < 0)")
    #
    # it = AlphaIter("test_filter", "target",0,21002925)
    # num = 0
    # while it.is_valid():
    #     v = it.get_value()
    #     if v > 0.1:
    #         print "error %.4f"%it.get_value()
    #     else:
    #         num += 1
    #     it.skip()
    # print num


