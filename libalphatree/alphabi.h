//
// Created by godpgf on 18-8-29.
// 用来从各个维度评估alpha指标的
//

#ifndef ALPHATREE_ALPHABI_H
#define ALPHATREE_ALPHABI_H

#include "alphaforest.h"

class AlphaBI {
public:
    static void initialize() {
        alphaBI_ = new AlphaBI();
    }

    static void release() {
        if (alphaBI_)
            delete alphaBI_;
        alphaBI_ = nullptr;
    }

    static AlphaBI *getAlphaBI() { return alphaBI_; }

    float getConfidence(const char *signName, const char *feature, const char *target, int daybefore, int sampleSize,
                        int sampleTime, float support) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        int targetId = af->useAlphaTree();

        af->decode(alphatreeId, "t", feature);
        af->decode(targetId, "t", target);

        float *outConfidence = new float[sampleTime];
        float confidence = predCurrentConfidence_(signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime,
                                                  support, outConfidence);
        delete[] outConfidence;

        af->releaseAlphaTree(alphatreeId);
        af->releaseAlphaTree(targetId);
        return confidence;
    }

    float getDiscrimination(const char *signName, const char *feature, const char *target, int daybefore, int sampleSize,
                            int sampleTime, float support){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        int targetId = af->useAlphaTree();

        af->decode(alphatreeId, "t", feature);
        af->decode(targetId, "t", target);

        float *outDiscrimination = new float[sampleTime];
        float disc = predCurrentDiscrimination_(signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime,
                                                  support, outDiscrimination);
        delete[] outDiscrimination;

        af->releaseAlphaTree(alphatreeId);
        af->releaseAlphaTree(targetId);
        return disc;
    }

    float getCorrelation(const char *signName, const char *a, const char *b, int daybefore, int sampleSize, int sampleTime) {
        float*  outCorrelation = new float[sampleTime];
        getCorrelationList(signName, a, b, daybefore, sampleSize, sampleTime, outCorrelation);
        float maxCorr = 0;
        for(int i = 0; i < sampleTime; ++i){
//            cout<<outCorrelation[i]<<" ";
            if(fabsf(outCorrelation[i]) > fabsf(maxCorr))
                maxCorr = outCorrelation[i];
        }
//        cout<<endl;
        return maxCorr;
    }

    bool getConfidenceList(const char *signName, const char *feature, const char *target, int daybefore, int sampleSize,
                           int sampleTime, float support, float *outConfidence) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        int targetId = af->useAlphaTree();

        af->decode(alphatreeId, "t", feature);
        af->decode(targetId, "t", target);

        bool isLeft = getConfidenceList_(signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime, support,
                                         outConfidence);

        af->releaseAlphaTree(alphatreeId);
        af->releaseAlphaTree(targetId);
        return isLeft;
    }

    void getDiscriminationList(const char *signName, const char *feature, const char *target, int daybefore, int sampleSize,
                               int sampleTime, float support, float *outDiscrimination){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        int targetId = af->useAlphaTree();

        af->decode(alphatreeId, "t", feature);
        af->decode(targetId, "t", target);

        getDiscriminationList_(signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime, support, outDiscrimination);
        af->releaseAlphaTree(alphatreeId);
        af->releaseAlphaTree(targetId);
    }

    int optimizeConfidence(const char *signName, const char *feature, const char *target, int daybefore, int sampleSize,
                           int sampleTime, float support, char *outFeature, int maxHistoryDays = 75,
                           float exploteRatio = 0.1f, int errTryTime = 64) {
        auto* alphabi = this;
        return optimizeConfidenceOrDiscrimination(feature, target, daybefore, sampleSize, sampleTime, outFeature, [alphabi, signName, support](int alphatreeId, int targetId, int curDaybefore, int curSampleSize,
                                                                                                                                               int curSampleTime, float *outConfidence){
            return alphabi->predCurrentConfidence_(signName, alphatreeId, targetId, curDaybefore, curSampleSize, curSampleTime, support, outConfidence);
        }, maxHistoryDays, exploteRatio, errTryTime);
    }

    int optimizeDiscrimination(const char *signName, const char *feature, const char *target, int daybefore, int sampleSize,
                               int sampleTime, float support, char *outFeature, int maxHistoryDays = 75,
                               float exploteRatio = 0.1f, int errTryTime = 64){
        auto* alphabi = this;
        return optimizeConfidenceOrDiscrimination(feature, target, daybefore, sampleSize, sampleTime, outFeature, [alphabi, signName, support](int alphatreeId, int targetId, int curDaybefore, int curSampleSize,
                                                                                                                                               int curSampleTime, float *outDiscrimination){
            return alphabi->predCurrentDiscrimination_(signName, alphatreeId, targetId, curDaybefore, curSampleSize, curSampleTime, support, outDiscrimination);
        }, maxHistoryDays, exploteRatio, errTryTime);
    }

    void getCorrelationList(const char *signName, const char *a, const char *b, int daybefore, int sampleSize,
                            int sampleTime, float *outCorrelation) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        int aId = af->useAlphaTree();
        int bId = af->useAlphaTree();
        af->decode(aId, "t", a);
        af->decode(bId, "t", b);

        int sampleDays = sampleSize * sampleTime;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName);
        AlphaSignIterator afeature(af, "t", signName, aId, daybefore, sampleDays, 0, signNum);
        AlphaSignIterator bfeature(af, "t", signName, bId, daybefore, sampleDays, 0, signNum);
        checkCache(signNum * 2);

        float *a_ = cache_;
        float *b_ = cache_ + signNum;

        for (int i = 0; i < signNum; ++i) {
            a_[i] = afeature.getValue();
            b_[i] = bfeature.getValue();
            afeature.skip(1);
            bfeature.skip(1);
        }

        for (int i = 0; i < sampleTime; ++i) {
            int startIndex = (int) (i * signNum / (float) sampleTime);
            int splitLen =
                    (int) (signNum * (i + 1) / (float) (sampleTime)) - (int) (signNum * i / (float) (sampleTime));
            outCorrelation[i] = getCorrelation_(a_ + startIndex, b_ + startIndex, splitLen);
        }

        af->releaseAlphaTree(aId);
        af->releaseAlphaTree(bId);
    }

    void calAutoregressive(const float *data, int len, float &minValue, float &maxValue) {
        checkCache(len);
        for (int i = 0; i < len; ++i) {
            cache_[i] = i;
        }
        float alpha, beta;
        lstsq_(cache_, data, len, beta, alpha);

//        cout<<beta<<" "<<alpha<<endl;

        float std = 0;
        for (int i = 0; i < len; ++i) {
            float err = data[i] - (i * beta + alpha);
            std += err * err;
//            cout<<data[i]<<"/"<<i*beta+alpha<<endl;
        }
        std = sqrtf(std / len);
        float value = len * beta + alpha;
//        cout<<value<<"-2*"<<std<<endl;
        minValue = value - 2 * std;
        maxValue = value + 2 * std;
    }


protected:
    AlphaBI() : cache_(nullptr), index_(nullptr), targetCache_(nullptr), cacheSize_(0) {}

    ~AlphaBI() {
        if (cache_)
            delete[] cache_;
        if (index_)
            delete[] index_;
        if (targetCache_)
            delete[] targetCache_;
    }

    void checkCache(int size) {
        if (size > cacheSize_) {
            if (cache_)
                delete[]cache_;
            if (index_)
                delete[]index_;
            if (targetCache_)
                delete[]targetCache_;
            cacheSize_ = size;
            cache_ = new float[cacheSize_];
            index_ = new int[cacheSize_ * 2];
            targetCache_ = new bool[cacheSize_];
        }
    }

    float predCurrentConfidence_(const char *signName, int alphatreeId, int targetId, int daybefore, int sampleSize,
                                 int sampleTime, float support, float *outConfidence) {
        getConfidenceList_(signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime, support, outConfidence);
        float minValue, maxValue;
        calAutoregressive(outConfidence, sampleTime, minValue, maxValue);
        return minValue;
    }

    float predCurrentDiscrimination_(const char *signName, int alphatreeId, int targetId, int daybefore, int sampleSize,
                                     int sampleTime, float support, float *outDiscrimination){
        getDiscriminationList_(signName, alphatreeId, targetId, daybefore, sampleSize, sampleTime, support, outDiscrimination);
//        for(int i = 0; i < sampleTime; ++i){
//            cout<<outDiscrimination[i]<<" ";
//        }
//        cout<<endl;
        float minValue, maxValue;
        calAutoregressive(outDiscrimination, sampleTime, minValue, maxValue);
        return minValue;
    }

    bool getConfidenceList_(const char *signName, int alphatreeId, int targetId, int daybefore, int sampleSize,
                            int sampleTime, float support, float *outConfidence) {
        AlphaForest *af = AlphaForest::getAlphaforest();

        int sampleDays = sampleSize * sampleTime;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName);
        AlphaSignIterator f(af, "t", signName, alphatreeId, daybefore, sampleDays, 0, signNum);
        AlphaSignIterator t(af, "t", signName, targetId, daybefore, sampleDays, 0, signNum);
        checkCache(f.size());

        if (sampleTime * 3 >= f.size()) {
            cout << "每个划分所得到的数据太少！";
            throw "err";
        }

        for (int i = 0; i < f.size(); ++i) {
            cache_[i] = f.getValue();
            targetCache_[i] = t.getValue() > 0.5f;
            index_[i] = i;
            f.skip(1);
            t.skip(1);
        }
        f.skip(0, false);
        t.skip(0, false);
        quickSort_(cache_, index_, 0, f.size() - 1);

        int leftCnt = 0;
        int rightCnt = 0;
        int midIndex = f.size() / 2;
        for (int i = 0; i < (midIndex << 1); ++i) {
            int id = index_[i];
            if (targetCache_[id]) {
                if (i < midIndex) {
                    ++leftCnt;
                } else {
                    ++rightCnt;
                }
            }
        }
        bool isLeft = (rightCnt < leftCnt);

        //得到每个时间分段的排序
        memset(index_ + f.size(), 0, sampleTime * 2 * sizeof(float));
        //每个数据段已经遍历过的有序数据的数量
        int *orderDataCnt = index_ + f.size();
        //每个数据段标签为1的数据数量
        int *orderLabelCnt = orderDataCnt + sampleTime;

        for (int i = 0; i < f.size(); ++i) {
            int orderId = isLeft ? i : (f.size() - i - 1);
            int id = index_[orderId];
            int splitId = (int) (id * sampleTime / (float) f.size());
            int splitLen = (int) (f.size() * (splitId + 1) / (float) (sampleTime)) -
                           (int) (f.size() * splitId / (float) (sampleTime));
            if (orderDataCnt[splitId] < splitLen * support) {
                ++orderDataCnt[splitId];
                if (targetCache_[id])
                    ++orderLabelCnt[splitId];
            }
        }

        for (int i = 0; i < sampleTime; ++i) {
            outConfidence[i] = orderLabelCnt[i] / (float) orderDataCnt[i];
        }

        return isLeft;
    }

    void getDiscriminationList_(const char *signName, int alphatreeId, int targetId, int daybefore, int sampleSize,
                                int sampleTime, float support, float *outDiscrimination){
        AlphaForest *af = AlphaForest::getAlphaforest();

        int sampleDays = sampleSize * sampleTime;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName);
        AlphaSignIterator f(af, "t", signName, alphatreeId, daybefore, sampleDays, 0, signNum);
        AlphaSignIterator t(af, "t", signName, targetId, daybefore, sampleDays, 0, signNum);
        checkCache(f.size());

        if (sampleTime * 3 >= f.size()) {
            cout << "每个划分所得到的数据太少！";
            throw "err";
        }

        for (int i = 0; i < f.size(); ++i) {
            cache_[i] = f.getValue();
            targetCache_[i] = t.getValue() > 0.5f;
            index_[i] = i;
            f.skip(1);
            t.skip(1);
        }
        f.skip(0, false);
        t.skip(0, false);
        quickSort_(cache_, index_, 0, f.size() - 1);

        int leftCnt = 0;
        int rightCnt = 0;
        int midIndex = f.size() / 2;
        for (int i = 0; i < (midIndex << 1); ++i) {
            int id = index_[i];
            if (targetCache_[id]) {
                if (i < midIndex) {
                    ++leftCnt;
                } else {
                    ++rightCnt;
                }
            }
        }
        bool isLeft = (rightCnt < leftCnt);

        //得到每个时间分段的排序
        memset(index_ + f.size(), 0, sampleTime * 2 * sizeof(float));
        //每个数据段已经遍历过的有序数据的数量
        int *orderDataCnt = index_ + f.size();
        //每个数据段标签为1的数据数量
        int *orderLabelCnt = orderDataCnt + sampleTime;

        for (int i = 0; i < f.size(); ++i) {
            int orderId = isLeft ? i : (f.size() - i - 1);
            int id = index_[orderId];
//            cout<<cache_[id]<<" ";
            int splitId = (int) (id * sampleTime / (float) f.size());
            int splitLen = (int) (f.size() * (splitId + 1) / (float) (sampleTime)) -
                           (int) (f.size() * splitId / (float) (sampleTime));
            if (orderDataCnt[splitId] < splitLen * support) {
                ++orderDataCnt[splitId];
                if (targetCache_[id])
                    ++orderLabelCnt[splitId];
            }
        }
//        cout<<endl;

        for (int i = 0; i < sampleTime; ++i) {
            outDiscrimination[i] = orderLabelCnt[i] / (float) orderDataCnt[i];
        }

        //得到每个时间分段的排序
        memset(index_ + f.size(), 0, sampleTime * 2 * sizeof(float));

        for (int i = 0; i < f.size(); ++i) {
            int orderId = isLeft ? (f.size() - i - 1) : i;
            int id = index_[orderId];
            int splitId = (int) (id * sampleTime / (float) f.size());
            int splitLen = (int) (f.size() * (splitId + 1) / (float) (sampleTime)) -
                           (int) (f.size() * splitId / (float) (sampleTime));
            if (orderDataCnt[splitId] < splitLen * support) {
                ++orderDataCnt[splitId];
                if (targetCache_[id])
                    ++orderLabelCnt[splitId];
            }
        }

        for (int i = 0; i < sampleTime; ++i) {
            outDiscrimination[i] /= (orderLabelCnt[i] / (float) orderDataCnt[i]);
        }
    }

    template <class F>
    int optimizeConfidenceOrDiscrimination(const char *feature, const char *target, int daybefore, int sampleSize,
                           int sampleTime, char *outFeature, F&& f,  int maxHistoryDays = 75,
                           float exploteRatio = 0.1f, int errTryTime = 64) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        auto *alphatree = af->getAlphaTree(alphatreeId);
        int targetId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        af->decode(targetId, "t", target);

        float *bestCoffList = new float[alphatree->getCoffSize()];
        for (int i = 0; i < alphatree->getCoffSize(); ++i) {
            bestCoffList[i] = alphatree->getCoff(i);
        }

        float *outConfidence = new float[sampleTime];

        float bestResL = f(alphatreeId, targetId, daybefore, sampleSize, sampleTime, outConfidence);
        float bestResR = f(alphatreeId, targetId, daybefore + sampleSize / 2, sampleSize, sampleTime, outConfidence);
        float bestRes = min(bestResL, bestResR);

        if (alphatree->getCoffSize() > 0) {
            RandomChoose rc = RandomChoose(2 * alphatree->getCoffSize());

            auto curErrTryTime = errTryTime;
            while (curErrTryTime > 0) {
                //修改参数
                float lastCoffValue = NAN;
                int curIndex = 0;
                bool isAdd = false;
                while (isnan(lastCoffValue)) {
                    curIndex = rc.choose();
                    isAdd = curIndex < alphatree->getCoffSize();
                    curIndex = curIndex % alphatree->getCoffSize();
                    if (isAdd && alphatree->getCoff(curIndex) < alphatree->getMaxCoff(curIndex)) {
                        lastCoffValue = alphatree->getCoff(curIndex);
                        float curCoff = lastCoffValue;
                        if (alphatree->getCoffUnit(curIndex) == CoffUnit::COFF_VAR) {
                            curCoff += 0.016f;
                        } else {
                            curCoff += 1.f;
                        }
                        alphatree->setCoff(curIndex, std::min(curCoff, alphatree->getMaxCoff(curIndex)));
                        if (alphatree->getMaxHistoryDays() > maxHistoryDays) {
                            alphatree->setCoff(curIndex, lastCoffValue);
                        }
                    }
                    if (!isAdd && alphatree->getCoff(curIndex) > alphatree->getMinCoff(curIndex)) {
                        lastCoffValue = alphatree->getCoff(curIndex);
                        float curCoff = lastCoffValue;
                        if (alphatree->getCoffUnit(curIndex) == CoffUnit::COFF_VAR) {
                            curCoff -= 0.016f;
                        } else {
                            curCoff -= 1;
                        }
                        alphatree->setCoff(curIndex, std::max(curCoff, alphatree->getMinCoff(curIndex)));
                    }
                    if (isnan(lastCoffValue)) {
                        curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                        rc.reduce(curIndex);
                    }

                }

                float resL = f(alphatreeId, targetId, daybefore, sampleSize, sampleTime, outConfidence);
                float resR = f(alphatreeId, targetId, daybefore + sampleSize / 2,
                                                    sampleSize, sampleTime, outConfidence);
                float res = min(resL, resR);

                if (res > bestRes) {
                    //cout<<"best res "<<res<<endl;
                    curErrTryTime = errTryTime;
                    bestRes = res;
                    for (int i = 0; i < alphatree->getCoffSize(); ++i) {
                        bestCoffList[i] = alphatree->getCoff(i);
                    }
                    //根据当前情况决定调整该参数的概率
                    curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                    rc.add(curIndex);
                } else {
                    --curErrTryTime;
                    if (!rc.isExplote(exploteRatio)) {
                        //恢复现场
                        alphatree->setCoff(curIndex, lastCoffValue);
                    }
                    curIndex = isAdd ? curIndex : alphatree->getCoffSize() + curIndex;
                    rc.reduce(curIndex);
                }

            }

            for (int i = 0; i < alphatree->getCoffSize(); ++i) {
                alphatree->setCoff(i, bestCoffList[i]);
            }
        }
        alphatree->encode("t", outFeature);

        delete[] bestCoffList;
        delete[] outConfidence;
        af->releaseAlphaTree(alphatreeId);
        af->releaseAlphaTree(targetId);
        return strlen(outFeature);
    }


    static void quickSort_(const float *src, int *index, int left, int right) {
        if (left >= right)
            return;
        int key = index[left];

        int low = left;
        int high = right;
        while (low < high) {
            //while (low < high && (isnan(src[(int)index[high]]) || src[(int)index[high]] > src[key])){
            while (low < high && src[(int) index[high]] > src[key]) {
                --high;
            }
            if (low < high)
                index[low++] = index[high];
            else
                break;

            //while (low < high && (isnan(src[(int)index[low]]) || src[(int)index[low]] <= src[key])){
            while (low < high && src[(int) index[low]] <= src[key]) {
                ++low;
            }
            if (low < high)
                index[high--] = index[low];
        }
        index[low] = (float) key;

        quickSort_(src, index, left, low - 1);
        quickSort_(src, index, low + 1, right);
    }

    static float getCorrelation_(float *a, float *b, int len) {
        //计算当前股票的均值和方差
        double meanLeft = 0;
        double meanRight = 0;
        double sumSqrLeft = 0;
        double sumSqrRight = 0;
        for (int j = 0; j < len; ++j) {
            meanLeft += a[j];
            sumSqrLeft += a[j] * a[j];
            meanRight += b[j];
            sumSqrRight += b[j] * b[j];
        }
        meanLeft /= len;
        meanRight /= len;

        float cov = 0;
        for (int k = 0; k < len; ++k) {
            cov += (a[k] - meanLeft) * (b[k] - meanRight);
        }

        float xDiff2 = (sumSqrLeft - meanLeft * meanLeft * len);
        float yDiff2 = (sumSqrRight - meanRight * meanRight * len);

        if (isnormal(cov) && isnormal(xDiff2) && isnormal(yDiff2)) {
            float corr = cov / sqrtf(xDiff2) / sqrtf(yDiff2);
            if (isnormal(corr)) {
                return fmaxf(fminf(corr, 1.0f), -1.0f);
            }

            return 1;
        }
        return 1;
    }

    static void lstsq_(const float *x, const float *y, int len, float &beta, float &alpha) {
        float sumx = 0.f;
        float sumy = 0.f;
        float sumxy = 0.f;
        float sumxx = 0.f;
        for (int j = 0; j < len; ++j) {
            sumx += x[j];
            sumy += y[j];
            sumxy += x[j] * y[j];
            sumxx += x[j] * x[j];
        }
        float tmp = (len * sumxx - sumx * sumx);
        beta = abs(tmp) < 0.0001f ? 0 : (len * sumxy - sumx * sumy) / tmp;
        alpha = abs(tmp) < 0.0001f ? 0 : sumy / len - beta * sumx / len;
    }

    float *cache_;
    int *index_;
    bool *targetCache_;
    int cacheSize_;
    static AlphaBI *alphaBI_;
};

AlphaBI *AlphaBI::alphaBI_ = nullptr;

#endif //ALPHATREE_ALPHABI_H
