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
    int useGroup(const char *signName, size_t daybefore, size_t sampleSize, size_t sampleTime, float support){
        int gId = groupCache_->useCacheMemory();
        groupCache_->getCacheMemory(gId).initialize(signName, daybefore, sampleSize, sampleTime, support);
        return gId;
    }

    void releaseGroup(int gId){
        groupCache_->releaseCacheMemory(gId);
    }

    void pluginControlGroup(int gId, const char* feature, const char* returns){
        AlphaForest *af = AlphaForest::getAlphaforest();
        auto& group = groupCache_->getCacheMemory(gId);
        int featureId = af->useAlphaTree();
        af->decode(featureId, "t", feature);
        int returnsId = af->useAlphaTree();
        af->decode(returnsId,"t",returns);

        size_t sampleDays = group.getSampleSize() * group.getSampleTime();
        size_t signNum = af->getAlphaDataBase()->getSignNum(group.getDaybefore(), sampleDays, group.getSignName());

        int fId = useDataCache(signNum);
        int iId = useIndexCache(signNum);
        float* featureData = (float*)dataCache_->getCacheMemory(fId).cache;
        float* returnsData = group.initializeReturns(signNum);
        int* indexData = (int*)indexCache_->getCacheMemory(iId).cache;

        pluginFeature(group.getSignName(), featureId, group.getDaybefore(), group.getSampleSize(), group.getSampleTime(), featureData, indexData);
        pluginFeature(group.getSignName(), returnsId, group.getDaybefore(), group.getSampleSize(), group.getSampleTime(), returnsData, nullptr);
        sortFeature_(featureData, indexData, signNum, group.getSampleTime());
        calReturnsRatioAvgAndStd_(returnsData, indexData, signNum, group.getSampleTime(), group.getSupport(), group.controlAvgList, group.controlStdList);

        releaseDataCache(fId);
        releaseIndexCache(iId);
        af->releaseAlphaTree(featureId);
        af->releaseAlphaTree(returnsId);
    }

    //返回某个特征的区分度，它的区分能力可能是随机的，设置接受它是随机的概率minRandPercent,以及回归质量好坏指标minR2(《计量经济学（3版）》古扎拉蒂--上册p59)
    float getDiscrimination(int gId, const char *feature, float expectReturn = 0.006f, float minRandPercent = 0.6f, float minR2 = 0.36, float stdScale = 2){
//        cout<<AlphaBI::getAlphaBI()->groupCache_->getCacheMemory(gId).getSignName()<<endl;
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        float disc = getDiscrimination(gId, alphatreeId, expectReturn, minRandPercent, minR2, stdScale);
        af->releaseAlphaTree(alphatreeId);
        return disc;
    }

    float getCorrelation(int gId, const char *a, const char *b) {
        auto& group = groupCache_->getCacheMemory(gId);
        int sId = useDataCache(group.getSampleTime());
        float* seqList = (float*)dataCache_->getCacheMemory(sId).cache;
        getCorrelationList(gId, a, b, seqList);
        float maxCorr = 0;
        for(int i = 0; i < group.getSampleTime(); ++i){
            if(fabsf(seqList[i]) > fabsf(maxCorr))
                maxCorr = seqList[i];
        }
        releaseDataCache(sId);
        return maxCorr;
    }

    int optimizeDiscrimination(int gId, const char *feature, char *outFeature, float expectReturn = 0.006f, float minRandPercent = 0.6f, float minR2 = 0.36f, float stdScale = 2, int maxHistoryDays = 75,
                               float exploteRatio = 0.1f, int errTryTime = 64){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        auto *alphatree = af->getAlphaTree(alphatreeId);
        af->decode(alphatreeId, "t", feature);

        int coffId = useDataCache(alphatree->getCoffSize());
        float *bestCoffList = (float*)dataCache_->getCacheMemory(coffId).cache;
        for (int i = 0; i < alphatree->getCoffSize(); ++i) {
            bestCoffList[i] = alphatree->getCoff(i);
        }

        float bestRes = getDiscrimination(gId, alphatreeId, expectReturn, minRandPercent, minR2, stdScale);

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

                float res = getDiscrimination(gId, alphatreeId, expectReturn, minRandPercent, minR2, stdScale);

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

protected:
    AlphaBI(){
        dataCache_ = DCache<BaseCache, DEFAULT_CACHE_SIZE>::create();
        indexCache_ = DCache<BaseCache, DEFAULT_CACHE_SIZE>::create();
        groupCache_ = DCache<BIGroup, DEFAULT_CACHE_SIZE>::create();
    }

    ~AlphaBI() {
        DCache<BaseCache, DEFAULT_CACHE_SIZE>::release(dataCache_);
        DCache<BaseCache, DEFAULT_CACHE_SIZE>::release(indexCache_);
        DCache<BIGroup, DEFAULT_CACHE_SIZE>::release(groupCache_);
    }

    int useDataCache(size_t size){
        int cId = dataCache_->useCacheMemory();
        dataCache_->getCacheMemory(cId).initialize<float>(size * sizeof(float));
        return cId;
    }

    void releaseDataCache(int cId){
        dataCache_->releaseCacheMemory(cId);
    }

    int useIndexCache(size_t size){
        int iId = indexCache_->useCacheMemory();
        indexCache_->getCacheMemory(iId).initialize<int>(size * sizeof(float));
        return iId;
    }

    void releaseIndexCache(int iId){
        indexCache_->releaseCacheMemory(iId);
    }

    static int pluginFeature(const char* signName, const char* feature, size_t daybefore, size_t sampleSize, size_t sampleTime, float* cache, int* index){
//        cout<<signName<<" "<<feature<<" "<<daybefore<<" "<<sampleSize<<" "<<sampleTime<<endl;
        AlphaForest *af = AlphaForest::getAlphaforest();
        int alphatreeId = af->useAlphaTree();
        af->decode(alphatreeId, "t", feature);
        int signNum = pluginFeature(signName, alphatreeId, daybefore, sampleSize, sampleTime, cache, index);
        af->releaseAlphaTree(alphatreeId);
        return signNum;
    }

    static int pluginFeature(const char* signName, int alphatreeId, size_t daybefore, size_t sampleSize, size_t sampleTime, float* cache, int* index){
        AlphaForest *af = AlphaForest::getAlphaforest();
        int sampleDays = sampleSize * sampleTime;
        int signNum = af->getAlphaDataBase()->getSignNum(daybefore, sampleDays, signName);

        AlphaSignIterator f(af, "t", signName, alphatreeId, daybefore, sampleDays, 0, signNum);

        for(int i = 0; i < signNum; ++i){
            cache[i] = f.getValue();
            if(index != nullptr)
                index[i] = i;
            f.skip(1);
        }
        f.skip(0, false);
        return signNum;
    }

    float getDiscrimination(int gId, int alphatreeId, float expectReturn = 0.006f, float minRandPercent = 0.06f, float minR2 = 0.32, float stdScale = 2){
        AlphaForest *af = AlphaForest::getAlphaforest();
        auto& group = groupCache_->getCacheMemory(gId);
//        cout<<group.getSignName()<<endl;
        size_t sampleDays = group.getSampleSize() * group.getSampleTime();
//        cout<<group.getSignName()<<" "<<sampleDays<<" "<<group.getDaybefore()<<" "<<group.getSampleTime()<<endl;
        size_t signNum = af->getAlphaDataBase()->getSignNum(group.getDaybefore(), sampleDays, group.getSignName());

        //先计算特征是越大越好还是越小越好
        int fId = useDataCache(signNum);
        int iId = useIndexCache(signNum);
        float* featureData = (float*)dataCache_->getCacheMemory(fId).cache;
        float* returnsData = group.returnsList;
        int* indexData = (int*)indexCache_->getCacheMemory(iId).cache;
        pluginFeature(group.getSignName(), alphatreeId, group.getDaybefore(), group.getSampleSize(), group.getSampleTime(), featureData, indexData);

        bool isDirectlyPropor = getIsDirectlyPropor(featureData, returnsData, indexData, signNum);

        if(!isDirectlyPropor){
            //如果特征和收益不成正比，强制转一下
            for(int i = 0; i < signNum; ++i)
                featureData[i] = -featureData[i];
        }
        //恢复排序过的index
        for(size_t i = 0; i < signNum; ++i)
            indexData[i] = i;
        //给每个时间段的特征排序
        sortFeature_(featureData, indexData, signNum, group.getSampleTime());

        //计算特征影响下的收益比（特征大的那部分股票收益/特征小的那部分股票收益）
        calReturnsRatioAvgAndStd_(returnsData, indexData, signNum, group.getSampleTime(), group.getSupport(), group.observationAvgList, group.observationStdList);

//        cout<<"control avg:\n";
//        for(int i = 0; i < group.getSampleTime(); ++i){
//            cout<<group.controlAvgList[i]<<"/"<<group.observationAvgList[i]<<" ";
//        }
//        cout<<endl;

        int sId = useDataCache(group.getSampleTime());
        int tId = useDataCache(group.getSampleTime());
        float* seqList = (float*)dataCache_->getCacheMemory(sId).cache;
        float* timeList = (float*)dataCache_->getCacheMemory(tId).cache;
        for(size_t i = 0; i < group.getSampleTime(); ++i)
            timeList[i] = i;

        //计算特征收益其实是随机参数的概率
//        char tmp[512];
//        char* p = tmp;
//        memset(p, 0, 512 * sizeof(char));
        for(size_t i = 0; i < group.getSampleTime(); ++i){
            if(group.observationAvgList[i] < group.controlAvgList[i]){
                releaseIndexCache(iId);
                releaseDataCache(fId);
//                if(i > (group.getSampleTime() >> 1))
//                    cout<<":"<<tmp<<endl;
                return 0;
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
        calAutoregressive_(timeList, seqList, group.getSampleTime(), stdScale, minValue, maxValue);
        if(maxValue >= minRandPercent){
            releaseIndexCache(iId);
            releaseDataCache(fId);
            return 0;
        }

        //计算拟合优度
        calFeatureAvg_(featureData, indexData, signNum, group.getSampleTime(), group.getSupport(), group.featureAvgList);
        calFeatureAvg_(returnsData, indexData, signNum, group.getSampleTime(), group.getSupport(), group.returnsAvgList);


        calR2Seq_(featureData, group.featureAvgList, returnsData, group.returnsAvgList, indexData, signNum, group.getSampleTime(), group.getSupport(), seqList);

        calAutoregressive_(timeList, seqList, group.getSampleTime(), stdScale, minValue, maxValue);
//        for(int i = 0; i < group.getSampleTime(); ++i)
//            if(seqList[i] < minValue)
//                minValue = seqList[i];
        cout<<"r2="<<minValue<<endl;
        if(minValue < minR2){
            releaseIndexCache(iId);
            releaseDataCache(fId);
            releaseDataCache(sId);
            releaseDataCache(tId);
            return 0;
        }

        //最后计算区分度，前面两个指标大概率排除了特征是随机的可能（特征有没有可能是假的），现在就有返回特征有没有效果（特征区分度）
        calDiscriminationSeq_(returnsData, indexData, signNum, group.getSampleTime(), group.getSupport(), expectReturn, seqList);
        calAutoregressive_(timeList, seqList, group.getSampleTime(), stdScale, minValue, maxValue);
        cout<<"dist=("<<minValue<<"~"<<maxValue<<")"<<endl;

        releaseIndexCache(iId);
        releaseDataCache(fId);
        releaseDataCache(sId);
        releaseDataCache(tId);
        return minValue;
    }

    void getCorrelationList(int gId, const char *a, const char *b, float *outCorrelation) {
        AlphaForest *af = AlphaForest::getAlphaforest();
        auto& group = groupCache_->getCacheMemory(gId);
        size_t sampleDays = group.getSampleSize() * group.getSampleTime();
        size_t signNum = af->getAlphaDataBase()->getSignNum(group.getDaybefore(), sampleDays, group.getSignName());

        int aId = af->useAlphaTree();
        int bId = af->useAlphaTree();
        af->decode(aId, "t", a);
        af->decode(bId, "t", b);

        AlphaSignIterator afeature(af, "t", group.getSignName(), aId, group.getDaybefore(), sampleDays, 0, signNum);
        AlphaSignIterator bfeature(af, "t", group.getSignName(), bId, group.getDaybefore(), sampleDays, 0, signNum);

        int cId = useDataCache(signNum * 2);
        float* cache = (float*)dataCache_->getCacheMemory(cId).cache;
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
                    (size_t) (signNum * (i + 1) / (float) (group.getSampleTime())) - (size_t)(signNum * i / (float) (group.getSampleTime()));
            outCorrelation[i] = correlation_(a_ + startIndex, b_ + startIndex, splitLen);
        }

        releaseDataCache(cId);
        af->releaseAlphaTree(aId);
        af->releaseAlphaTree(bId);
    }

public:
    DCache<BaseCache, DEFAULT_CACHE_SIZE>* dataCache_;
    DCache<BaseCache, DEFAULT_CACHE_SIZE>* indexCache_;
    DCache<BIGroup, DEFAULT_CACHE_SIZE>* groupCache_;

    static AlphaBI *alphaBI_;
};

AlphaBI *AlphaBI::alphaBI_ = nullptr;

#endif //ALPHATREE_ALPHABI_H
