//
// Created by godpgf on 18-8-29.
// 用来从各个维度评估alpha指标的
//

#ifndef ALPHATREE_ALPHABI_H
#define ALPHATREE_ALPHABI_H

#include "alphaforest.h"
#include "base/normal.h"

class AlphaBI {
public:
    static void initialize(const char *signName, const char *randFeature, const char *returns, int daybefore, int sampleSize,
                           int sampleTime, float support) {
        alphaBI_ = new AlphaBI(signName, randFeature, returns, daybefore, sampleSize, sampleTime, support);
    }

    static void release() {
        if (alphaBI_)
            delete alphaBI_;
        alphaBI_ = nullptr;
    }

    static AlphaBI *getAlphaBI() { return alphaBI_; }

    //返回某个特征的区分度，它的区分能力可能是随机的，设置接受它是随机的概率minRandPercent,以及回归质量好坏指标minR2(《计量经济学（3版）》古扎拉蒂--上册p59)
    float getDiscrimination(const char *feature, const char *target, float minRandPercent = 0.000006f, float minR2 = 0.16){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        float disc = getDiscrimination(alphatreeId, target, minRandPercent, minR2);
        af->releaseAlphaTree(alphatreeId);
        return disc;
    }

    float getCorrelation(const char *a, const char *b) {
        float* outCorrelation = seqCache_;
        getCorrelationList(signName_, a, b, outCorrelation);
        float maxCorr = 0;
        for(int i = 0; i < sampleTime_; ++i){
            if(fabsf(outCorrelation[i]) > fabsf(maxCorr))
                maxCorr = outCorrelation[i];
        }
        return maxCorr;
    }

    int optimizeDiscrimination(const char *feature, const char *target, char *outFeature,
                               float minRandPercent = 0.000006f, float minR2 = 0.16, int maxHistoryDays = 75,
                               float exploteRatio = 0.1f, int errTryTime = 64){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        auto *alphatree = af->getAlphaTree(alphatreeId);
        af->decode(alphatreeId, "t", feature);

        float *bestCoffList = new float[alphatree->getCoffSize()];
        for (int i = 0; i < alphatree->getCoffSize(); ++i) {
            bestCoffList[i] = alphatree->getCoff(i);
        }



        float bestRes = getDiscrimination(alphatreeId, target, minRandPercent, minR2);

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

                float res = getDiscrimination(alphatreeId, target, minRandPercent, minR2);

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
        af->releaseAlphaTree(alphatreeId);
        return strlen(outFeature);
    }



    void calAutoregressive(const float *data, int len, float &minValue, float &maxValue) {
        for (int i = 0; i < len; ++i) {
            cache_[i] = i;
        }
        float alpha, beta;
        lstsq_(cache_, data, len, beta, alpha);

        float std = 0;
        for (int i = 0; i < len; ++i) {
            float err = data[i] - (i * beta + alpha);
            std += err * err;
        }
        std = sqrtf(std / len);
        float value = len * beta + alpha;
        minValue = value - 2 * std;
        maxValue = value + 2 * std;
    }


protected:
    AlphaBI(const char *signName, const char *randFeature, const char *returns, int daybefore, int sampleSize,
            int sampleTime, float support) : daybefore_(daybefore), sampleSize_(sampleSize), sampleTime_(sampleTime), support_(support),
                                             controlReturnsRatioAvg_(sampleTime), controlReturnsRatioStd_(sampleTime),
                                             observationReturnsRatioAvg_(sampleTime),observationReturnsRatioStd_(sampleTime),
                                             observationFeatureAvg_(sampleTime), observationReturnsAvg_(sampleTime){
        signName_ = new char[strlen(signName) + 1];
        strcpy(signName_, signName);
        returns_ = new char[strlen(returns) + 1];
        strcpy(returns_, returns);

        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize * sampleTime;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName);
        cache_ = new float[signNum * 2];
        index_ = new int[signNum];
        seqCache_ = new float[sampleTime];
        pluginFeature(randFeature, cache_, index_);
        sortFeature(cache_, index_);
        pluginFeature(returns, cache_, nullptr);
        calReturnsRatioAvgAndStd(controlReturnsRatioAvg_, controlReturnsRatioStd_, cache_);
    }

    ~AlphaBI() {
        delete[] cache_;
        delete[] index_;
        delete[] seqCache_;
        delete []signName_;
        delete []returns_;
    }

    int pluginFeature(const char* feature, float* cache, int* index){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        int signNum = pluginFeature(alphatreeId, cache, index);
        af->releaseAlphaTree(alphatreeId);
        return signNum;
    }

    int pluginFeature(int alphatreeId, float* cache, int* index){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize_ * sampleTime_;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore_, sampleDays, signName_);

        AlphaSignIterator f(af, "t", signName_, alphatreeId, daybefore_, sampleDays, 0, signNum);

        for(int i = 0; i < signNum; ++i){
            cache[i] = f.getValue();
            if(index != nullptr)
                index[i] = i;
            f.skip(1);
        }
        f.skip(0, false);
        return signNum;
    }

    //给每个时间段的特征排序,排序后数据保存在缓存中
    void sortFeature(float* cache, int* index){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize_ * sampleTime_;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore_, sampleDays, signName_);

        for(int splitId = 0; splitId < sampleTime_; ++splitId){
            int preId = (int)(splitId * signNum / (float)sampleTime_);
            int nextId = (int)((splitId + 1) * signNum / (float)sampleTime_);
            quickSort_(cache_, index_, preId, nextId-1);
        }
    }

    void calFeatureAvg(float* cache, Vector<float>& featureAvg){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize_ * sampleTime_;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore_, sampleDays, signName_);

        featureAvg.initialize(0);
        for(int splitId = 0; splitId < sampleTime_; ++splitId){
            int nextId = (int)((splitId + 1) * signNum / (float)sampleTime_);
            int preId = (int)(splitId * signNum / (float)sampleTime_);
            int supportNextId = preId + (nextId - preId) * support_ * 0.5;
            for(int j = preId; ++j; j < supportNextId){
                int lid = index_[j];
                int rid = index_[nextId - 1 - (j - preId)];
                featureAvg[splitId] += cache[lid];
                featureAvg[splitId] += cache[rid];
            }
            featureAvg[splitId] /= 2 * (supportNextId - preId);
        }

    }

    void calReturnsRatioAvgAndStd(Vector<float>& returnsRatioAvg, Vector<float>& returnsRatioStd, float* cache){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize_ * sampleTime_;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore_, sampleDays, signName_);

        returnsRatioAvg.initialize(0);
        returnsRatioStd.initialize(0);

        for(int splitId = 0; splitId < sampleTime_; ++splitId){
            int nextId = (int)((splitId + 1) * signNum / (float)sampleTime_);
            int preId = (int)(splitId * signNum / (float)sampleTime_);
            int supportNextId = preId + (nextId - preId) * support_ * 0.5;
            for(int j = preId; ++j; j < supportNextId){
                int lid = index_[j];
                int rid = index_[nextId - 1 - (j - preId)];
                float v = (cache[lid] + 1.f) / (cache[rid] + 1.f);
                returnsRatioAvg[splitId] += v;
                returnsRatioStd[splitId] += v * v;
            }
            returnsRatioAvg[splitId] /= (supportNextId - preId);
            returnsRatioStd[splitId] = sqrtf(returnsRatioStd[splitId] / (supportNextId - preId) - returnsRatioAvg[splitId] * returnsRatioAvg[splitId]);
            //样本均值的标准差=样本的标准差 / sqrt(样本数量)
            returnsRatioStd[splitId] /= sqrtf(supportNextId - preId);
        }
    }

    void calDiscriminationSeq(float* cache){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize_ * sampleTime_;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore_, sampleDays, signName_);

        for(int splitId = 0; splitId < sampleTime_; ++splitId){
            int preId = (int)(splitId * signNum / (float)sampleTime_);
            int nextId = (int)((splitId + 1) * signNum / (float)sampleTime_);
            int supportNextId = preId + (nextId - preId) * support_ * 0.5;
            int leftCnt = 0, rightCnt = 0;
            for(int j = preId; ++j; j < supportNextId){
                int lid = index_[j];
                int rid = index_[nextId - 1 - (j - preId)];
                if(cache[lid] > 0.5f)
                    ++leftCnt;
                if(cache[rid] > 0.5f)
                    ++rightCnt;

            }
            seqCache_[splitId] = (leftCnt > 0) ? 1 : rightCnt / (float)leftCnt;
        }
    }

    void calR2Seq(float* xCache, Vector<float>& xAvg, float* yCache, Vector<float>& yAvg){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize_ * sampleTime_;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore_, sampleDays, signName_);

        for(int splitId = 0; splitId < sampleTime_; ++splitId){
            int preId = (int)(splitId * signNum / (float)sampleTime_);
            int nextId = (int)((splitId + 1) * signNum / (float)sampleTime_);
            int supportNextId = preId + (nextId - preId) * support_ * 0.5;
            float sumxy = 0;
            float sumx2 = 0;
            float sumy2 = 0;
            for(int j = preId; ++j; j < supportNextId){
                int lid = index_[j];
                int rid = index_[nextId - 1 - (j - preId)];
                float x = xCache[lid] - xAvg[splitId];
                float y = yCache[lid] - yAvg[splitId];
                sumxy += x * y;
                sumx2 += x * x;
                sumy2 += y * y;
                x = xCache[rid] - xAvg[splitId];
                y = yCache[rid] - yAvg[splitId];
                sumxy += x * y;
                sumx2 += x * x;
                sumy2 += y * y;
            }
            seqCache_[splitId] = sumxy / sumx2 * sumxy / sumy2;
        }
    }

    float getDiscrimination(int alphatreeId, const char *target, float minRandPercent = 0.000006f, float minR2 = 0.16){
        //先计算特征是越大越好还是越小越好
        int signNum = pluginFeature(alphatreeId, cache_, index_);
        float* returns = cache_ + signNum;
        pluginFeature(returns_, returns, nullptr);
        quickSort_(cache_, index_, 0, signNum-1);
        float leftReturns = 0, rightReturns = 0;
        int midSize = (signNum >> 1);
        for(int i = 0; i < midSize; ++i){
            leftReturns += returns[index_[i]];
            rightReturns += returns[index_[i+midSize]];
        }
        bool isDirectlyPropor = (rightReturns > leftReturns);

        pluginFeature(alphatreeId, cache_, index_);
        if(!isDirectlyPropor){
            //如果特征和收益不成正比，强制转一下
            for(int i = 0; i < signNum; ++i)
                cache_[i] = -cache_[i];
        }

        //计算特征影响下的收益比（特征大的那部分股票收益/特征小的那部分股票收益）
        sortFeature(cache_, index_);
        calReturnsRatioAvgAndStd(observationReturnsRatioAvg_, observationReturnsRatioStd_, returns);

        //计算特征收益其实是随机参数的概率
        float p = 1;
        for(int i = 0; i < sampleTime_; ++i){
            float x = (observationReturnsRatioAvg_[i] - controlReturnsRatioAvg_[i]) / controlReturnsRatioStd_[i];
            p *= (1.f - normSDist(x));
        }

        if(p < minRandPercent)
            return 0;

        //计算拟合优度
        calFeatureAvg(cache_, observationFeatureAvg_);
        calFeatureAvg(returns, observationReturnsAvg_);
        calR2Seq(cache_, observationFeatureAvg_, returns, observationReturnsAvg_);
        float minValue, maxValue;
        calAutoregressive(seqCache_, sampleTime_, minValue, maxValue);
        if(minValue < minR2)
            return 0;

        //最后计算区分度，前面两个指标大概率排除了特征是随机的可能（特征有没有可能是假的），现在就有返回特征有没有效果（特征区分度）
        pluginFeature(target, cache_, nullptr);
        calDiscriminationSeq(cache_);
        calAutoregressive(seqCache_, sampleTime_, minValue, maxValue);
        return minValue;
    }

    void getCorrelationList(const char *signName, const char *a, const char *b, float *outCorrelation) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        int aId = af->useAlphaTree();
        int bId = af->useAlphaTree();
        af->decode(aId, "t", a);
        af->decode(bId, "t", b);

        int sampleDays = sampleSize_ * sampleTime_;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore_, sampleDays, signName);
        AlphaSignIterator afeature(af, "t", signName, aId, daybefore_, sampleDays, 0, signNum);
        AlphaSignIterator bfeature(af, "t", signName, bId, daybefore_, sampleDays, 0, signNum);

        float *a_ = cache_;
        float *b_ = cache_ + signNum;

        for (int i = 0; i < signNum; ++i) {
            a_[i] = afeature.getValue();
            b_[i] = bfeature.getValue();
            afeature.skip(1);
            bfeature.skip(1);
        }

        for (int i = 0; i < sampleTime_; ++i) {
            int startIndex = (int) (i * signNum / (float) sampleTime_);
            int splitLen =
                    (int) (signNum * (i + 1) / (float) (sampleTime_)) - (int) (signNum * i / (float) (sampleTime_));
            outCorrelation[i] = getCorrelation_(a_ + startIndex, b_ + startIndex, splitLen);
        }

        af->releaseAlphaTree(aId);
        af->releaseAlphaTree(bId);
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
    float *seqCache_;

    char* signName_;
    char* returns_;
    int daybefore_;
    int sampleSize_;
    int sampleTime_;
    float support_;

    //对照组收益率比的均值和标准差
    Vector<float> controlReturnsRatioAvg_;
    Vector<float> controlReturnsRatioStd_;
    //观察组的收益率比的均值和标准差
    Vector<float> observationReturnsRatioAvg_;
    Vector<float> observationReturnsRatioStd_;
    //观察组特征均值
    Vector<float> observationFeatureAvg_;
    //观察组收益均值
    Vector<float> observationReturnsAvg_;

    static AlphaBI *alphaBI_;
};

AlphaBI *AlphaBI::alphaBI_ = nullptr;

#endif //ALPHATREE_ALPHABI_H
