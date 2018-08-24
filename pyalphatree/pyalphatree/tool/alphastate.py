from hmmlearn.hmm import GaussianHMM
import numpy as np


def pain_state(path, hmm_train_days = 0, n_components = 3):

    with open(path, 'r') as f:
        titles = f.readline()[:-1].split(',')
        datas = []
        need_to_append = (not titles[-1] == 'state')
        if need_to_append:
            titles.append('state')
        line = f.readline()
        while line:
            line = line[:-1].split(',')
            if need_to_append:
                line.append('0')
            datas.append(line)
            line = f.readline()

    with open(path, 'w') as f:
        f.write('%s\n'%(','.join(titles)))
        index = 0
        open_price = []
        high_price = []
        low_price = []
        close_price = []
        volume = []
        min_train_days = hmm_train_days + 5
        while index < len(datas) and (len(close_price) < min_train_days-1 or hmm_train_days == 0):
            f.write('%s\n'%(','.join(datas[index])))
            v = float(datas[index][5])
            if v > 0:
                open_price.append(float(datas[index][1]))
                high_price.append(float(datas[index][2]))
                low_price.append(float(datas[index][3]))
                close_price.append(float(datas[index][4]))
                volume.append(v)
            index += 1

        while index < len(datas):
            v = float(datas[index][5])
            if v > 0:
                open_price.append(float(datas[index][1]))
                high_price.append(float(datas[index][2]))
                low_price.append(float(datas[index][3]))
                close_price.append(float(datas[index][4]))
                volume.append(v)

                logDel = np.log(np.array(high_price[-hmm_train_days:])) - np.log(np.array(low_price[-hmm_train_days:]))
                logRet_2 = np.log(np.array(close_price[-hmm_train_days:])) - np.log(np.array(open_price[-hmm_train_days-2:-2]))
                logRet_5 = np.log(np.array(close_price[-hmm_train_days:])) - np.log(np.array(close_price[-hmm_train_days-5:-5]))
                logVol_5 = np.log(np.array(volume[-hmm_train_days:])) - np.log(np.array(volume[-hmm_train_days-5:-5]))
                A = np.column_stack([logVol_5, logRet_5, logRet_2])
                model = GaussianHMM(n_components=n_components, covariance_type='full', n_iter=16).fit(A)
                means = np.array([ele[1] for ele in model.means_])
                ids = np.argsort(means)
                state = model.predict(A)[-1]
                for i in range(n_components):
                    if state == ids[i]:
                        state = i + 1
                        break
                datas[index][-1] = "%d"%state
            f.write('%s\n' % (','.join(datas[index])))
            index += 1
