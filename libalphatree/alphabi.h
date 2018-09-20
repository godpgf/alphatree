//
// Created by godpgf on 18-8-29.
// 用来从各个维度评估alpha指标的
//

#ifndef ALPHATREE_ALPHABI_H
#define ALPHATREE_ALPHABI_H

#include "bi/basebi.h"
#include "bi/bigroup.h"
#include "alphaforest.h"

#define DEFAULT_CACHE_SIZE 2


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

    //创建一组对比数据，用来验证有效性的
    int useGroup(const char *signName, size_t daybefore, size_t sampleSize, size_t sampleTime, float support, float expectReturn) {
        int gId = groupCache_->useCacheMemory();
        groupCache_->getCacheMemory(gId).initialize(signName, daybefore, sampleSize, sampleTime, support, expectReturn);
        return gId;
    }

    void releaseGroup(int gId) {
        groupCache_->releaseCacheMemory(gId);
    }

    void pluginControlGroup(int gId, const char *feature, const char *returns) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        auto &group = groupCache_->getCacheMemory(gId);
        int featureId = af->useAlphaTree();
        af->decode(featureId, "t", feature);
        int returnsId = af->useAlphaTree();
        af->decode(returnsId, "t", returns);

        size_t sampleDays = group.getSampleSize() * group.getSampleTime();
        size_t signNum = af->getAlphaDataBase()->getSignNum(group.getDaybefore(), sampleDays, group.getSignName());

        int fId = useDataCache(signNum);
        int iId = useIndexCache(signNum);
        float *featureData = (float *) dataCache_->getCacheMemory(fId).cache;
        float *returnsData = group.initializeReturns(signNum);
        int *indexData = (int *) indexCache_->getCacheMemory(iId).cache;

        pluginFeature(group.getSignName(), featureId, group.getDaybefore(), group.getSampleSize(),
                      group.getSampleTime(), featureData, indexData);
        pluginFeature(group.getSignName(), returnsId, group.getDaybefore(), group.getSampleSize(),
                      group.getSampleTime(), returnsData, nullptr);
        sortFeature_(featureData, indexData, signNum, group.getSampleTime());
        calReturnsRatioAvgAndStd_(returnsData, indexData, signNum, group.getSampleTime(), group.getSupport(), group.getExpectReturn(),
                                  group.controlAvgList, group.controlStdList);

//        for(int i = 0; i < signNum; ++i)
//            cout<<returnsData[i]<<" ";
//        cout<<endl;

        releaseDataCache(fId);
        releaseIndexCache(iId);
        af->releaseAlphaTree(featureId);
        af->releaseAlphaTree(returnsId);
    }

    float getRandomPercent(int gId, const char* feature, float stdScale = 2){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        float disc = getRandomPercent(gId, alphatreeId, stdScale);
        af->releaseAlphaTree(alphatreeId);
        return disc;
    }

    //返回某个特征的区分度，它的区分能力可能是随机的，设置接受它是随机的概率minRandPercent,以及分类质量好坏指标minAUC
    float getDiscrimination(int gId, const char *feature, float stdScale = 2) {
//        cout<<AlphaBI::getAlphaBI()->groupCache_->getCacheMemory(gId).getSignName()<<endl;
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        float disc = getDiscrimination(gId, alphatreeId, stdScale);
        af->releaseAlphaTree(alphatreeId);
        return disc;
    }

    float getDiscriminationInc(int gId, const char *incFeature, const char* feature,  float stdScale = 2){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        int incAlphaTreeId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        af->decode(incAlphaTreeId, "t", incFeature);
//        cout<<feature<<" "<<incFeature<<endl;
        float discInc = getDiscriminationInc(gId, incAlphaTreeId, alphatreeId, stdScale);
        af->releaseAlphaTree(alphatreeId);
        af->releaseAlphaTree(incAlphaTreeId);
        return discInc;
    }

    float getCorrelation(int gId, const char *a, const char *b) {
        auto &group = groupCache_->getCacheMemory(gId);
        int sId = useDataCache(group.getSampleTime());
        float *seqList = (float *) dataCache_->getCacheMemory(sId).cache;
        getCorrelationList(gId, a, b, seqList);
        float maxCorr = 0;
        for (int i = 0; i < group.getSampleTime(); ++i) {
            if (fabsf(seqList[i]) > fabsf(maxCorr))
                maxCorr = seqList[i];
        }
        releaseDataCache(sId);
        return maxCorr;
    }

    int optimizeDiscrimination(int gId, const char *feature, char *outFeature,
                               float stdScale = 2, int maxHistoryDays = 75,
                               float exploteRatio = 0.1f, int errTryTime = 64) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        auto *alphatree = af->getAlphaTree(alphatreeId);
        af->decode(alphatreeId, "t", feature);

        int coffId = useDataCache(alphatree->getCoffSize());
        float *bestCoffList = (float *) dataCache_->getCacheMemory(coffId).cache;
        for (int i = 0; i < alphatree->getCoffSize(); ++i) {
            bestCoffList[i] = alphatree->getCoff(i);
        }

        float bestRes = getDiscrimination(gId, alphatreeId, stdScale);

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

                float res = getDiscrimination(gId, alphatreeId, stdScale);

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

        releaseDataCache(coffId);
        af->releaseAlphaTree(alphatreeId);
        return strlen(outFeature);
    }

    int optimizeDiscriminationInc(int gId, const char *incFeature, const char *feature, char *outFeature,
                               float stdScale = 2, int maxHistoryDays = 75,
                               float exploteRatio = 0.1f, int errTryTime = 64) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        //auto *alphatree = af->getAlphaTree(alphatreeId);
        af->decode(alphatreeId, "t", feature);

        int incAlphatreeId = af->useAlphaTree();
        auto* incalphatree = af->getAlphaTree(incAlphatreeId);
        af->decode(incAlphatreeId, "t", incFeature);

        int coffId = useDataCache(incalphatree->getCoffSize());
        float *bestCoffList = (float *) dataCache_->getCacheMemory(coffId).cache;
        for (int i = 0; i < incalphatree->getCoffSize(); ++i) {
            bestCoffList[i] = incalphatree->getCoff(i);
        }

        float bestRes = getDiscriminationInc(gId, incAlphatreeId, alphatreeId, stdScale);

        if (incalphatree->getCoffSize() > 0) {
            RandomChoose rc = RandomChoose(2 * incalphatree->getCoffSize());

            auto curErrTryTime = errTryTime;
            while (curErrTryTime > 0) {
                //修改参数
                float lastCoffValue = NAN;
                int curIndex = 0;
                bool isAdd = false;
                while (isnan(lastCoffValue)) {
                    curIndex = rc.choose();
                    isAdd = curIndex < incalphatree->getCoffSize();
                    curIndex = curIndex % incalphatree->getCoffSize();
                    if (isAdd && incalphatree->getCoff(curIndex) < incalphatree->getMaxCoff(curIndex)) {
                        lastCoffValue = incalphatree->getCoff(curIndex);
                        float curCoff = lastCoffValue;
                        if (incalphatree->getCoffUnit(curIndex) == CoffUnit::COFF_VAR) {
                            curCoff += 0.016f;
                        } else {
                            curCoff += 1.f;
                        }
                        incalphatree->setCoff(curIndex, std::min(curCoff, incalphatree->getMaxCoff(curIndex)));
                        if (incalphatree->getMaxHistoryDays() > maxHistoryDays) {
                            incalphatree->setCoff(curIndex, lastCoffValue);
                        }
                    }
                    if (!isAdd && incalphatree->getCoff(curIndex) > incalphatree->getMinCoff(curIndex)) {
                        lastCoffValue = incalphatree->getCoff(curIndex);
                        float curCoff = lastCoffValue;
                        if (incalphatree->getCoffUnit(curIndex) == CoffUnit::COFF_VAR) {
                            curCoff -= 0.016f;
                        } else {
                            curCoff -= 1;
                        }
                        incalphatree->setCoff(curIndex, std::max(curCoff, incalphatree->getMinCoff(curIndex)));
                    }
                    if (isnan(lastCoffValue)) {
                        curIndex = isAdd ? curIndex : incalphatree->getCoffSize() + curIndex;
                        rc.reduce(curIndex);
                    }

                }

                float res = getDiscriminationInc(gId, incAlphatreeId, alphatreeId, stdScale);

                if (res > bestRes) {
                    //cout<<"best res "<<res<<endl;
                    curErrTryTime = errTryTime;
                    bestRes = res;
                    for (int i = 0; i < incalphatree->getCoffSize(); ++i) {
                        bestCoffList[i] = incalphatree->getCoff(i);
                    }
                    //根据当前情况决定调整该参数的概率
                    curIndex = isAdd ? curIndex : incalphatree->getCoffSize() + curIndex;
                    rc.add(curIndex);
                } else {
                    --curErrTryTime;
                    if (!rc.isExplote(exploteRatio)) {
                        //恢复现场
                        incalphatree->setCoff(curIndex, lastCoffValue);
                    }
                    curIndex = isAdd ? curIndex : incalphatree->getCoffSize() + curIndex;
                    rc.reduce(curIndex);
                }

            }

            for (int i = 0; i < incalphatree->getCoffSize(); ++i) {
                incalphatree->setCoff(i, bestCoffList[i]);
            }
        }
        incalphatree->encode("t", outFeature);

        releaseDataCache(coffId);
        af->releaseAlphaTree(alphatreeId);
        af->releaseAlphaTree(incAlphatreeId);
        return strlen(outFeature);
    }

protected:
    AlphaBI() {
        dataCache_ = DCache<BaseCache, DEFAULT_CACHE_SIZE>::create();
        indexCache_ = DCache<BaseCache, DEFAULT_CACHE_SIZE>::create();
        groupCache_ = DCache<BIGroup, DEFAULT_CACHE_SIZE>::create();
    }

    ~AlphaBI() {
        DCache<BaseCache, DEFAULT_CACHE_SIZE>::release(dataCache_);
        DCache<BaseCache, DEFAULT_CACHE_SIZE>::release(indexCache_);
        DCache<BIGroup, DEFAULT_CACHE_SIZE>::release(groupCache_);
    }

    int useDataCache(size_t size) {
        int cId = dataCache_->useCacheMemory();
        dataCache_->getCacheMemory(cId).initialize<float>(size * sizeof(float));
        return cId;
    }

    void releaseDataCache(int cId) {
        dataCache_->releaseCacheMemory(cId);
    }

    int useIndexCache(size_t size) {
        int iId = indexCache_->useCacheMemory();
        indexCache_->getCacheMemory(iId).initialize<int>(size * sizeof(float));
        return iId;
    }

    void releaseIndexCache(int iId) {
        indexCache_->releaseCacheMemory(iId);
    }

    static int pluginFeature(const char *signName, const char *feature, size_t daybefore, size_t sampleSize, size_t sampleTime,
                  float *cache, int *index) {
//        cout<<signName<<" "<<feature<<" "<<daybefore<<" "<<sampleSize<<" "<<sampleTime<<endl;
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        int signNum = pluginFeature(signName, alphatreeId, daybefore, sampleSize, sampleTime, cache, index);
        af->releaseAlphaTree(alphatreeId);
        return signNum;
    }

    static int pluginFeature(const char *signName, int alphatreeId, size_t daybefore, size_t sampleSize, size_t sampleTime,
                  float *cache, int *index) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize * sampleTime;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName);

        AlphaSignIterator f(af, "t", signName, alphatreeId, daybefore, sampleDays, 0, signNum);

        for (int i = 0; i < signNum; ++i) {
            cache[i] = f.getValue();
            if (index != nullptr)
                index[i] = i;
            f.skip(1);
        }
        f.skip(0, false);
        return signNum;
    }

    //某个效果不错的分类特征可能是随机的，返回某个时段内它是随机的概率
    float getRandomPercent(int gId, int alphatreeId, float stdScale = 2){
        AlphaForest *af = AlphaForest::getAlphaforest();
        auto &group = groupCache_->getCacheMemory(gId);
//        cout<<group.getSignName()<<endl;
        size_t sampleDays = group.getSampleSize() * group.getSampleTime();
//        cout<<group.getSignName()<<" "<<sampleDays<<" "<<group.getDaybefore()<<" "<<group.getSampleTime()<<endl;
        size_t signNum = af->getAlphaDataBase()->getSignNum(group.getDaybefore(), sampleDays, group.getSignName());

        //先计算特征是越大越好还是越小越好
        int fId = useDataCache(signNum);
        int iId = useIndexCache(signNum);
        float *featureData = (float *) dataCache_->getCacheMemory(fId).cache;
        float *returnsData = group.returnsList;
        int *indexData = (int *) indexCache_->getCacheMemory(iId).cache;
        pluginFeature(group.getSignName(), alphatreeId, group.getDaybefore(), group.getSampleSize(),
                      group.getSampleTime(), featureData, indexData);

        bool isDirectlyPropor = getIsDirectlyPropor(featureData, returnsData, indexData, signNum, group.getSupport(), group.getExpectReturn());

        if (!isDirectlyPropor) {
            //如果特征和收益不成正比，强制转一下
            for (int i = 0; i < signNum; ++i)
                featureData[i] = -featureData[i];
        }
        //恢复排序过的index
        for (size_t i = 0; i < signNum; ++i)
            indexData[i] = i;
        //给每个时间段的特征排序
        sortFeature_(featureData, indexData, signNum, group.getSampleTime());

        //计算特征影响下的收益比（特征大的那部分股票收益/特征小的那部分股票收益）
        calReturnsRatioAvgAndStd_(returnsData, indexData, signNum, group.getSampleTime(), group.getSupport(), group.getExpectReturn(),
                                  group.observationAvgList, group.observationStdList);

//        if(strcmp(group.getSignName(),"valid_sign_1") == 0){
//            cout<<"control avg:"<<group.getSignName()<<" "<<signNum<<endl;
//            for(int i = 0; i < group.getSampleTime(); ++i){
//                cout<<group.controlAvgList[i]<<"/"<<group.observationAvgList[i]<<" ";
//            }
//            cout<<endl;
//        }


        int sId = useDataCache(group.getSampleTime());
//        int tId = useDataCache(group.getSampleTime());
        float *seqList = (float *) dataCache_->getCacheMemory(sId).cache;
//        float *timeList = (float *) dataCache_->getCacheMemory(tId).cache;
//        for (size_t i = 0; i < group.getSampleTime(); ++i)
//            timeList[i] = i;

        //计算特征收益其实是随机参数的概率
//        char tmp[512];
//        char* p = tmp;
//        memset(p, 0, 512 * sizeof(char));
        for (size_t i = 0; i < group.getSampleTime(); ++i) {
            if (group.observationAvgList[i] < group.controlAvgList[i]) {
                releaseIndexCache(iId);
                releaseDataCache(fId);
                releaseDataCache(sId);
//                releaseDataCache(tId);
//                if(i > (group.getSampleTime() >> 1))
//                    cout<<":"<<tmp<<endl;
                return 1;
            }

            float x = (group.observationAvgList[i] - group.controlAvgList[i]) / group.controlStdList[i];

            x = (1.f - normSDist(x));
            seqList[i] = x;
//            if(x >= minRandPercent){
//                releaseIndexCache(iId);
//                releaseDataCache(fId);
//                if(i > (group.getSampleTime() >> 1))
//                    cout<<":"<<tmp<<endl;
//                return 0;
//            }

//            sprintf(p,"%.4f ",x);
//            p += strlen(p);
        }
//        cout<<tmp<<endl;
        float minValue = FLT_MAX, maxValue = -FLT_MAX;
//        calAutoregressive_(timeList, seqList, group.getSampleTime(), stdScale, minValue, maxValue);
        calWaveRange_(seqList, group.getSampleTime(), stdScale, minValue, maxValue);
        releaseIndexCache(iId);
        releaseDataCache(fId);
        releaseDataCache(sId);
        return maxValue;
    }

    //返回比较比较倒霉的时候区分度（auc）是多少
    float getDiscrimination(int gId, int alphatreeId, float stdScale = 2) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        auto &group = groupCache_->getCacheMemory(gId);
//        cout<<group.getSignName()<<endl;
        size_t sampleDays = group.getSampleSize() * group.getSampleTime();
//        cout<<group.getSignName()<<" "<<sampleDays<<" "<<group.getDaybefore()<<" "<<group.getSampleTime()<<endl;
        size_t signNum = af->getAlphaDataBase()->getSignNum(group.getDaybefore(), sampleDays, group.getSignName());

        //先计算特征是越大越好还是越小越好
        int fId = useDataCache(signNum);
        int iId = useIndexCache(signNum);
        float *featureData = (float *) dataCache_->getCacheMemory(fId).cache;
        float *returnsData = group.returnsList;
        int *indexData = (int *) indexCache_->getCacheMemory(iId).cache;
        pluginFeature(group.getSignName(), alphatreeId, group.getDaybefore(), group.getSampleSize(),
                      group.getSampleTime(), featureData, indexData);

        bool isDirectlyPropor = getIsDirectlyPropor(featureData, returnsData, indexData, signNum, group.getSupport(), group.getExpectReturn());

        if (!isDirectlyPropor) {
            //如果特征和收益不成正比，强制转一下
            for (int i = 0; i < signNum; ++i)
                featureData[i] = -featureData[i];
        }
        //恢复排序过的index
        for (size_t i = 0; i < signNum; ++i)
            indexData[i] = i;
        //给每个时间段的特征排序
        sortFeature_(featureData, indexData, signNum, group.getSampleTime());

        //计算拟合优度
        calFeatureAvg_(featureData, indexData, signNum, group.getSampleTime(), group.getSupport(),
                       group.featureAvgList);
        calFeatureAvg_(returnsData, indexData, signNum, group.getSampleTime(), group.getSupport(),
                       group.returnsAvgList);

        int sId = useDataCache(group.getSampleTime());
//        int tId = useDataCache(group.getSampleTime());
        float *seqList = (float *) dataCache_->getCacheMemory(sId).cache;
//        calR2Seq_(featureData, group.featureAvgList, returnsData, group.returnsAvgList, indexData, signNum,
//                  group.getSampleTime(), group.getSupport(), seqList);
        calAUCSeq_(featureData, indexData, returnsData, group.getExpectReturn(), signNum, group.getSampleTime(), group.getSupport(), seqList);

        float minValue, maxValue;
        calWaveRange_(seqList, group.getSampleTime(), stdScale, minValue, maxValue);

        releaseIndexCache(iId);
        releaseDataCache(fId);
        releaseDataCache(sId);
//        releaseDataCache(tId);
        return minValue;
    }

    float getDiscriminationInc(int gId, int alphatreeId, int incAlphatreeId, float stdScale = 2.f){
        AlphaForest *af = AlphaForest::getAlphaforest();
        auto &group = groupCache_->getCacheMemory(gId);
        size_t sampleDays = group.getSampleSize() * group.getSampleTime();
        size_t signNum = af->getAlphaDataBase()->getSignNum(group.getDaybefore(), sampleDays, group.getSignName());

        int fId = useDataCache(signNum);
        int incId = useDataCache(signNum);
        int iId = useIndexCache(signNum);
        int sId = useDataCache(group.getSampleTime());

        float *seqList = (float *) dataCache_->getCacheMemory(sId).cache;
        float *featureData = (float *) dataCache_->getCacheMemory(fId).cache;
        float *incData = (float*) dataCache_->getCacheMemory(incId).cache;
        float *returnsData = group.returnsList;
        int *indexData = (int *) indexCache_->getCacheMemory(iId).cache;

        pluginFeature(group.getSignName(), incAlphatreeId, group.getDaybefore(), group.getSampleSize(),
                      group.getSampleTime(), incData, indexData);
        pluginFeature(group.getSignName(), alphatreeId, group.getDaybefore(), group.getSampleSize(),
                      group.getSampleTime(), featureData, nullptr);

        //先计算特征是越大越好还是越小越好
        bool isDirectlyPropor = getIsDirectlyPropor(featureData, returnsData, indexData, signNum, group.getSupport(), group.getExpectReturn());

        if (!isDirectlyPropor) {
            //如果特征和收益不成正比，强制转一下
            for (int i = 0; i < signNum; ++i)
                featureData[i] = -featureData[i];
        }

//        for(int i = 0; i < signNum; ++i)
//            cout<<featureData[i]<<"~"<<incData[i]<<" ";
//        cout<<endl;

        //恢复排序过的index
        for (size_t i = 0; i < signNum; ++i)
            indexData[i] = i;


        mulSortFeature_(incData, featureData, indexData, signNum, group.getSampleTime(), group.getSupport());
        calAUCIncSeq_(returnsData, indexData, signNum, group.getSampleTime(), group.getExpectReturn(), seqList);

        float avg = 0;
        for(int i = 0; i < group.getSampleTime(); ++i){
            avg += seqList[i];
        }
        avg /= group.getSampleTime();
        if(avg < 0){
            for(int i = 0; i < group.getSampleTime(); ++i)
                seqList[i] = -seqList[i];
        }

        float minValue = FLT_MAX, maxValue = -FLT_MAX;
        calWaveRange_(seqList, group.getSampleTime(), stdScale, minValue, maxValue);
//        calAutoregressive_(seqList, seqList, group.getSampleTime(), stdScale, minValue, maxValue);
//        cout << "dist inc=(" << minValue << "~" << maxValue << ")" << endl;

        releaseIndexCache(iId);
        releaseDataCache(fId);
        releaseDataCache(sId);
        releaseDataCache(incId);
        return minValue;
    }

    void getCorrelationList(int gId, const char *a, const char *b, float *outCorrelation) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        auto &group = groupCache_->getCacheMemory(gId);
        size_t sampleDays = group.getSampleSize() * group.getSampleTime();
        size_t signNum = af->getAlphaDataBase()->getSignNum(group.getDaybefore(), sampleDays, group.getSignName());

        int aId = af->useAlphaTree();
        int bId = af->useAlphaTree();
        af->decode(aId, "t", a);
        af->decode(bId, "t", b);

        AlphaSignIterator afeature(af, "t", group.getSignName(), aId, group.getDaybefore(), sampleDays, 0, signNum);
        AlphaSignIterator bfeature(af, "t", group.getSignName(), bId, group.getDaybefore(), sampleDays, 0, signNum);

        int cId = useDataCache(signNum * 2);
        float *cache = (float *) dataCache_->getCacheMemory(cId).cache;
        float *a_ = cache;
        float *b_ = cache + signNum;

        for (int i = 0; i < signNum; ++i) {
            a_[i] = afeature.getValue();
            b_[i] = bfeature.getValue();
            afeature.skip(1);
            bfeature.skip(1);
        }

        for (size_t i = 0; i < group.getSampleTime(); ++i) {
            size_t startIndex = (size_t) (i * signNum / (float) group.getSampleTime());
            size_t splitLen =
                    (size_t) (signNum * (i + 1) / (float) (group.getSampleTime())) -
                    (size_t) (signNum * i / (float) (group.getSampleTime()));
            outCorrelation[i] = correlation_(a_ + startIndex, b_ + startIndex, splitLen);
        }

        releaseDataCache(cId);
        af->releaseAlphaTree(aId);
        af->releaseAlphaTree(bId);
    }

public:
    DCache<BaseCache, DEFAULT_CACHE_SIZE> *dataCache_;
    DCache<BaseCache, DEFAULT_CACHE_SIZE> *indexCache_;
    DCache<BIGroup, DEFAULT_CACHE_SIZE> *groupCache_;

    static AlphaBI *alphaBI_;
};

AlphaBI *AlphaBI::alphaBI_ = nullptr;

#endif //ALPHATREE_ALPHABI_H
