//
// Created by yanyu on 2017/8/28.
//

#ifndef ALPHATREE_ALPHAEXAMINER_H
#define ALPHATREE_ALPHAEXAMINER_H

#include "alpha/normal.h"
#include "alphaforest.h"
#include <float.h>
#include <random>
#include <iostream>
using namespace std;

class AlphaExaminer{
    public:
        static void initialize(AlphaForest* alphaForest);

        static void release();

        static AlphaExaminer* getAlphaexaminer();

        std::shared_future<float> eval(int alphatreeId,int cacheMemoryId, int flagCacheId, int codesId, size_t futureNum, size_t evalTime, size_t trainDateSize, size_t testDateSize, size_t minSieveNum, size_t maxSieveNum, int punishNum = 6){
            //先计算alphatree
            //cout<<"get codes"<<endl;
            size_t stockSize = alphaForest_->getCodes(0, futureNum, alphaForest_->getHistoryDays(alphatreeId), trainDateSize+testDateSize, alphaForest_->getCodesCache()->getCacheMemory(codesId));

            int dateSize = AlphaDataBase::getElementSize(alphaForest_->getHistoryDays(alphatreeId), testDateSize+trainDateSize, alphaForest_->getFutureDays(alphatreeId));
            int nodeNum = alphaForest_->getNodeNum(alphatreeId);

            //cout<<"use cache"<<endl;
            //分配缓存
            bool *flagCache = alphaForest_->getFlagCache()->getCacheMemory(flagCacheId);
            float *cacheMemory = alphaForest_->getAlphaTreeMemoryCache()->getCacheMemory(cacheMemoryId);
            const char* codes = alphaForest_->getCodesCache()->getCacheMemory(codesId);
            float *targetCache = AlphaTree::getNodeCacheMemory(nodeNum, dateSize, stockSize, cacheMemory);
            float *sieveValue = targetCache + stockSize * (testDateSize + trainDateSize);
            int* sieveCount = (int*)(sieveValue + maxSieveNum);

            //cout<<"request cal"<<endl;
            std::shared_future<const float*> alphatreeRes = alphaForest_->calAlpha(alphatreeId, flagCache, cacheMemory, codes, futureNum, testDateSize+trainDateSize,stockSize);

            //cout<<"request eval"<<endl;
            AlphaDataBase* alphaDatabase = alphaForest_->getAlphaDataBase();
            //开始取样数据并验证,注意如果发现测试数据小于0就不再测试,直接返回
            return alphaForest_->getThreadPool()->enqueue([alphaDatabase, alphatreeRes, futureNum, evalTime, trainDateSize, testDateSize, stockSize, codes, minSieveNum, maxSieveNum, targetCache, sieveValue, sieveCount, punishNum]{
                //cout<<"start eval"<<endl;
                //取样
                std::vector<int> shuffleIndex;
                const float* alphaCache = alphatreeRes.get();
                //cout<<"finish cal"<<endl;
                //float avg = AlphaExaminer::mean(alphaCache, (testDateSize + trainDateSize) * stockSize);
                //float std = AlphaExaminer::std(alphaCache, avg, (testDateSize + trainDateSize) * stockSize);
                float minAlpha = FLT_MAX;
                float maxAlpha = -FLT_MAX;
                getMinAndMax(alphaCache, (testDateSize + trainDateSize) * stockSize, minAlpha, maxAlpha);
                //cout<<"min "<<minAlpha<<" max "<<maxAlpha<<endl;

                int sampleSize = trainDateSize + testDateSize;

                float scoreSum = 0;
                for(int eid = 0; eid < evalTime; ++eid){
                    //得到洗牌后的数据
                    shuffleIndex.clear();
                    for(int i = 0; i < sampleSize; ++i)
                        shuffleIndex.push_back(i);
                    unsigned seed = std::chrono::system_clock::now ().time_since_epoch ().count ();
                    std::shuffle (shuffleIndex.begin (), shuffleIndex.end (), std::default_random_engine (seed));

                    float bestScore = -FLT_MAX;
                    int bestFutureNum = 0;
                    float bestAlphaStart = FLT_MAX;
                    float bestAlphaEnd = -FLT_MAX;

                    //尝试所有可能的未来天数-----------------------------------------------------------------------------------------------
                    for(int fIndex = 0; fIndex < futureNum; ++fIndex){
                        alphaDatabase->getTarget(fIndex, futureNum, testDateSize + trainDateSize, stockSize, targetCache, codes);

                        //开始尝试渔网尺寸,找到最好的尺寸和位置-------------------
                        for(int sieveNum = minSieveNum; sieveNum <= maxSieveNum; sieveNum *= 2){
                            //初始化
                            memset(sieveValue, 0, sieveNum * sizeof(float));
                            memset(sieveCount, 0, sieveNum * sizeof(int));

                            //排名靠前的(100/sieveNum)%的数据,他们的alpha是均值+criticalValue个标准差
                            //float criticalValue = normsinv(1 - 1.0f/sieveNum);
                            //float deltaCriticalValue = criticalValue * 2 / (sieveNum-2);

                            //cout<<"std scale:"<<criticalValue<<endl;

                            //AlphaExaminer::sieve(alphaCache, targetCache, 0, trainDateSize, stockSize, shuffleIndex, sieveValue, sieveCount, sieveNum, avg, std, criticalValue, deltaCriticalValue);
                            AlphaExaminer::sieve(alphaCache, targetCache, 0, trainDateSize, stockSize, shuffleIndex, sieveValue, sieveCount, sieveNum, minAlpha, maxAlpha);
                            for(int i = 0; i < sieveNum; ++i){
                                float score = (sieveValue[i]/(sieveCount[i] + punishNum));
                                if(score > bestScore){
                                    bestScore = score;
                                    bestFutureNum = fIndex+1;
                                    if(i == 0){
                                        bestAlphaStart = -FLT_MAX;
                                        bestAlphaEnd = (maxAlpha - minAlpha)/sieveNum + minAlpha;
                                        //bestAlphaEnd = avg - std * criticalValue;
                                    } else if(i == sieveNum-1){
                                        bestAlphaStart = maxAlpha - (maxAlpha - minAlpha)/sieveNum;
                                        bestAlphaEnd = FLT_MAX;
                                    } else{
                                        bestAlphaStart = i * (maxAlpha - minAlpha)/sieveNum + minAlpha;
                                        bestAlphaEnd = bestAlphaStart + (maxAlpha - minAlpha)/sieveNum;
                                    }
                                }
                            }
                        }

                    }

                    //验证-------------------------------------------------------------------------------------------------
                    //cout<<"reset target "<<bestFutureNum<<endl;
                    //重新放入未来数据
                    alphaDatabase->getTarget(bestFutureNum-1, futureNum, testDateSize + trainDateSize, stockSize, targetCache, codes);
                    //cout<<"test "<<endl;
                    //cout<<"error size: "<<abs(bestScore - AlphaExaminer::sieveEval(alphaCache, targetCache, 0, trainDateSize, stockSize, shuffleIndex, bestAlphaStart, bestAlphaEnd, punishNum))*10000<<endl;

                    float score = AlphaExaminer::sieveEval(alphaCache, targetCache, trainDateSize, testDateSize, stockSize, shuffleIndex, bestAlphaStart, bestAlphaEnd, punishNum);
                    //cout<<"finish test"<<endl;
                    if(score < 0)
                        return score;
                    scoreSum += score;
                }
                return scoreSum / evalTime;
            });
        }

    protected:
        AlphaExaminer(AlphaForest* alphaForest):
                alphaForest_(alphaForest){
        }

        ~AlphaExaminer(){
            cout<<"release AlphaExaminer"<<endl;
        }

        inline static void getMinAndMax(const float* src, int size, float& minValue, float& maxValue){
            for(int i = 0; i < size; ++i){
                if(src[i] < minValue)
                    minValue = src[i];
                if(src[i] > maxValue)
                    maxValue = src[i];
            }
        }

//        inline static float mean(const float* src, int size){
//            float sum = 0;
//            for(int i = 0; i < size; ++i){
//                sum += src[i];
//            }
//            return sum / size;
//        }
//
//        inline static float std(const float* src, float avg, int size){
//            float sum = 0;
//            for(int i = 0; i < size; ++i){
//                float t = src[i] - avg;
//                sum += t * t;
//            }
//            return sqrtf(sum / size);
//        }


        inline static void sieve(const float* alpha, const float* target, int startSampleIndex, int sampleSize, int stockSize, std::vector<int>& shuffleIndex, float* sieveValue, int* sieveCount, int sieveNum, float minAlpha, float maxAlpha){
            float deltaAlpha = (maxAlpha - minAlpha) / sieveNum;
            for(int i = 0; i < sampleSize; ++i){
            const float* curAlpha = alpha + shuffleIndex[startSampleIndex+i] * stockSize;
            const float* curTarget = target + shuffleIndex[startSampleIndex+i] * stockSize;
            for(int j = 0; j < stockSize; ++j){
                    int sieveIndex = (int)((curAlpha[j] - minAlpha) / deltaAlpha);
                    if(sieveIndex < 0)
                        sieveIndex = 0;
                    else if(sieveIndex >= sieveNum)
                        sieveIndex = sieveNum - 1;
                    sieveValue[sieveIndex] += curTarget[j];
                    ++sieveCount[sieveIndex];
                }
            }
        }

//        inline static void sieve(const float* alpha, const float* target, int startSampleIndex, int sampleSize, int stockSize, std::vector<int>& shuffleIndex, float* sieveValue, int* sieveCount, int sieveNum, float avg, float std,
//                                 float criticalValue, float deltaCriticalValue){
//
//            float startAlpha = avg - criticalValue * std;
//            float endAlpha = avg + criticalValue * std;
//            for(int i = 0; i < sampleSize; ++i){
//                const float* curAlpha = alpha + shuffleIndex[startSampleIndex+i] * stockSize;
//                const float* curTarget = target + shuffleIndex[startSampleIndex+i] * stockSize;
//                for(int j = 0; j < stockSize; ++j){
//                    int sieveIndex = NONE;
//                    if(curAlpha[j] < startAlpha){
//                        sieveIndex = 0;
//                    } else if (curAlpha[j] >= endAlpha){
//                        sieveIndex = sieveNum-1;
//                    } else{
//                        sieveIndex = (int)((curAlpha[j] - startAlpha) / deltaCriticalValue) + 1;
//                    }
//                    sieveValue[sieveIndex] += curTarget[j];
//                    ++sieveCount[sieveIndex];
//                }
//            }
//        }

        inline static float sieveEval(const float* alpha, const float* target, int startSampleIndex, int sampleSize, int stockSize, std::vector<int>& shuffleIndex, float alphaStart, float alphaEnd, int punishNum){
            float scoreSum = 0;
            int scoreCount = 0;
            for(int i = 0; i < sampleSize; ++i){
                const float* curAlpha = alpha + shuffleIndex[startSampleIndex+i] * stockSize;
                const float* curTarget = target + shuffleIndex[startSampleIndex+i] * stockSize;
                for(int j = 0; j < stockSize; ++j){
                    if(curAlpha[j] >= alphaStart && curAlpha[j] < alphaEnd){
                        scoreSum += curTarget[j];
                        ++scoreCount;
                    }

                }
            }
            //cout<<"test num: "<<scoreCount<<endl;
            return scoreSum / (scoreCount + punishNum);
        }

        AlphaForest* alphaForest_;

        static AlphaExaminer* alphaExaminer_;
};

void AlphaExaminer::initialize(AlphaForest* alphaForest){
    release();
    alphaExaminer_ = new AlphaExaminer(alphaForest);
}

void AlphaExaminer::release(){
    if(alphaExaminer_)
        delete alphaExaminer_;
}

AlphaExaminer* AlphaExaminer::getAlphaexaminer(){ return alphaExaminer_;}

AlphaExaminer* AlphaExaminer::alphaExaminer_ = nullptr;

#endif //ALPHATREE_ALPHAEXAMINER_H
