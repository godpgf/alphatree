//
// Created by godpgf on 18-3-16.
//

#ifndef ALPHATREE_ALPHARFT_H
#define ALPHATREE_ALPHARFT_H

#include "iterator.h"
#include "vector.h"
#include "../alphaforest.h"
#include "../cart/test/rft.h"

class AlphaRFT{
public:
    static void initialize(AlphaForest* af, const char* alphatreeList, int alphatreeNum, int threadNum){
        alphaRFT_ = new AlphaRFT(af, alphatreeList, alphatreeNum, threadNum);
    }

    static void release(){
        if(alphaRFT_ != nullptr)
            delete alphaRFT_;
        alphaRFT_ = nullptr;
    }

    static AlphaRFT* getAlphaRFT(){ return alphaRFT_;}

    //把数据读出去，主要为了测试
    int testReadData(size_t daybefore, size_t sampleSize, const char* target, const char* signName, float* pFeature, float* pTarget, int cacheSize = 4096){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        //填写特征
        Vector<IBaseIterator<float>*> features( featureList_->getSize());
        for(size_t i = 0; i < features.getSize(); ++i){
            //cout<<(*alphaNameList_)[i].c_str()<<" "<<daybefore<<" "<<sampleSize<<endl;
            features[i] = new AlphaSignIterator(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i],daybefore,sampleSize,0,signNum, cacheSize);
        }

        int targetAlphatreeId = af_->useAlphaTree();
        af_->decode(targetAlphatreeId, "_target", target);

        //填写目标
        AlphaSignIterator targetIter(af_, "_target", signName, targetAlphatreeId, daybefore, sampleSize, 0, signNum, cacheSize);

        int fid = 0;
        for(int i = 0; i < signNum; ++i){
            pTarget[i] = targetIter.getValue();
            ++targetIter;
            for(int j = 0; j < features.getSize(); ++j){
                pFeature[fid++] = features[j]->getValue();
                features[j]->skip(1);
            }
        }

        //删除特征
        for(size_t i = 0; i < features.getSize(); ++i)
            delete features[i];
        return signNum;
    }

    void train(size_t daybefore, size_t sampleSize, const char* weight, const char* target, const char* signName,
               float gamma, float lambda, float minWeight = 1024, int epochNum = 30000,
               const char* rewardFunName = "base", int stepNum = 1, int splitNum = 64, int cacheSize = 4096,
               const char* lossFunName = "binary:logistic", float lr = 0.1f, float step = 0.8f, float tiredCoff = 0.016f
               ){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        //填写特征 todo thread unsafe,need to modify
        Vector<IBaseIterator<float>*> features( featureList_->getSize());
        for(size_t i = 0; i < features.getSize(); ++i){
            //cout<<(*alphaNameList_)[i].c_str()<<" "<<daybefore<<" "<<sampleSize<<endl;
            features[i] = new AlphaSignIterator(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i],daybefore,sampleSize,0,signNum, cacheSize);
        }


        int targetAlphatreeId = af_->useAlphaTree();
        af_->decode(targetAlphatreeId, "_target", target);

        int weightAlphatreeId = -1;
        float* weightMemory = nullptr;
        IBaseIterator<float>* weightIter;
        if(weight != nullptr){
            weightAlphatreeId = af_->useAlphaTree();
            af_->decode(weightAlphatreeId, "_weight", weight);
            weightIter = new AlphaSignIterator(af_, "_weight", signName, weightAlphatreeId, daybefore, sampleSize, 0, signNum, cacheSize);
        } else {
            weightMemory = new float[signNum];
            weightIter = new MemoryIterator<float>(weightMemory, signNum);
            ((MemoryIterator<float>*)weightIter)->initialize(1);
        }

        //填写目标
        AlphaSignIterator targetIter(af_, "_target", signName, targetAlphatreeId, daybefore, sampleSize, 0, signNum, cacheSize);

        //申请空间
        //注意，必须*2,一半是左分裂，一半是右分裂
        Vector<IVector<float>*> splitWeights( threadNum_);
        for(int i = 0; i < threadNum_; ++i){
            splitWeights[i] = new Vector<float>(featureList_->getSize() * 2);
        }
        float* memory = new float[3 * signNum];
        int* skip = new int[signNum * threadNum_];

        MemoryIterator<float> lastPred(memory, signNum);
        lastPred.initialize(0);
        MemoryIterator<float> g(memory + 1 * signNum, signNum);
        MemoryIterator<float> h(memory + 2 * signNum, signNum);
        rft->initialize(weightIter, &targetIter, &lastPred, &g, &h, lossFunName, stepNum);

        //开始训练
        rft->train(&features, &splitWeights, weightIter, &targetIter, &g, &h, skip, splitNum, gamma, lambda, minWeight, epochNum, rewardFunName, stepNum, lr, step, tiredCoff);

        //释放空间
        for(int i = 0; i < threadNum_; ++i){
            delete splitWeights[i];
        }
        delete []memory;
        delete []skip;

        //删除特征
        for(size_t i = 0; i < features.getSize(); ++i)
            delete features[i];
        af_->releaseAlphaTree(targetAlphatreeId);
        if(weightAlphatreeId != -1)
            af_->releaseAlphaTree(weightAlphatreeId);
        delete weightIter;
        if(weightMemory)
            delete []weightMemory;
    }

    void eval(size_t daybefore, size_t sampleSize,const char* target, const char* signName, int stepIndex = 1, int cacheSize = 4096){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);

        //填写特征
        Vector<IBaseIterator<float>*> features(featureList_->getSize());
        for(size_t i = 0; i < features.getSize(); ++i){
            features[i] = new AlphaSignIterator(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i],daybefore,sampleSize,0,signNum, cacheSize);
        }

        float* memory = new float[signNum];
        MemoryIterator<float> weight(memory, signNum);
        weight.initialize(1);

//        int equalIndex = 0;
//        while (target[equalIndex] != '=')
//            ++equalIndex;
//        string targetName = string(target, 0, equalIndex);
        int targetAlphatreeId = af_->useAlphaTree();
        af_->decode(targetAlphatreeId, "_target", target);

        //填写目标
        AlphaSignIterator targetIter(af_, "_target", signName, targetAlphatreeId, daybefore, sampleSize, 0, signNum, cacheSize);
        rft->eval(&features, &weight, &targetIter, stepIndex);

        delete []memory;
        //删除特征
        for(size_t i = 0; i < features.getSize(); ++i)
            delete features[i];
        af_->releaseAlphaTree(targetAlphatreeId);
    }

    size_t pred(size_t daybefore, float* predOut, const char* signName, const char* rewardFunName, int startStepIndex, int stepNum = 1, int cacheSize = 4096){
        int sampleSize = 1;
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        //填写特征
        Vector<IBaseIterator<float>*> features( featureList_->getSize());
        for(size_t i = 0; i < features.getSize(); ++i){
            features[i] = new AlphaSignIterator(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i],daybefore,sampleSize,0,signNum, cacheSize);
        }

        rft->pred(&features, predOut, rewardFunName, startStepIndex, stepNum);
        return signNum;
        /*
        //删除特征
        for(size_t i = 0; i < features.getSize(); ++i)
            delete features[i];
        return signNum;

        float* cache = new float[signNum * alphatreeIds_->getSize()];

        int cacheId = af_->useCache();
        for(int i = 0; i < alphatreeIds_->getSize(); ++i){
            af_->calAlpha((*alphatreeIds_)[i], cacheId, daybefore, 1, codes, stockSize);
            const float* alpha = af_->getAlpha((*alphatreeIds_)[i], (*alphaNameList_)[i].c_str(), cacheId);
            for(int j = 0; j < stockSize; ++j){
                cache[j * alphatreeIds_->getSize() + i] = alpha[j];
            }
        }

        for(int i = 0; i < stockSize; ++i){
            Vector<float> f("feature", cache + i * alphatreeIds_->getSize(), alphatreeIds_->getSize());
            predOut[i] = rft->pred(0, &f, rewardFunName, startStepIndex, stepNum);
        }

        af_->releaseCache(cacheId);
        delete []cache;*/
    }

    void cleanTree(float threshold = 0.16f, const char* rewardFunName = "base", int startStepIndex = 0, int stepNum = 1){
        rft->cleanTree(0, threshold, rewardFunName, startStepIndex,  stepNum);
    }

    void  cleanCorrelationLeaf(size_t daybefore, size_t sampleSize, const char* signName, float corrOtherPercent = 0.16f, float corrAllPercent = 0.32f, int cacheSize = 2048){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);

        //填写特征
        Vector<IBaseIterator<float>*> features(featureList_->getSize());
        for(size_t i = 0; i < features.getSize(); ++i){
            features[i] = new AlphaSignIterator(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i],daybefore,sampleSize,0,signNum, cacheSize);
        }

        float* memory = new float[signNum];
        MemoryIterator<float> weight(memory, signNum);
        weight.initialize(1);

        bool isCleaning = true;
        while (isCleaning) {
            isCleaning = false;
            if(rft->cleanCorrelation(0, &features, &weight, corrOtherPercent, false))
                isCleaning = true;
            if(rft->cleanCorrelation(0, &features, &weight, corrAllPercent, true))
                isCleaning = true;
        }

        delete []memory;
        //删除特征
        for(size_t i = 0; i < features.getSize(); ++i)
            delete features[i];
    }

    int tostring(char* out, const char* rewardFunName = "base", int startStepIndex = 0, int stepNum = 1){
        strcpy(out, rft->tree2str(alphaNameList_, rewardFunName, startStepIndex, stepNum).c_str());
        return strlen(out);
    }

    void saveModel(const char* path){
        rft->saveModel(path);
    }

    void loadModel(const char* path){
        rft->loadModel(path);
    }
protected:
    AlphaRFT(AlphaForest* af, const char* alphatreeList, int alphatreeNum, int threadNum):af_(af), threadNum_(threadNum){
        rft = RFT<>::create();
        featureList_ = new Vector<AlphaSignIterator*>( alphatreeNum);
        alphatreeIds_ = new Vector<int>( alphatreeNum);
        alphaNameList_ = new Vector<string>( alphatreeNum);
        int equalIndex;
        for(int i = 0; i < alphatreeNum; ++i){
            int len = strlen(alphatreeList);
            equalIndex = 0;
            while (alphatreeList[equalIndex] != '=')
                ++equalIndex;
            (*alphaNameList_)[i] = string(alphatreeList, 0, equalIndex);
            //cout<<(*alphaNameList_)[i].c_str()<<" "<<alphatreeList + (equalIndex + 1)<<endl;
            (*alphatreeIds_)[i] = af->useAlphaTree();
            af->decode((*alphatreeIds_)[i], (*alphaNameList_)[i].c_str(), alphatreeList + (equalIndex + 1));
            alphatreeList = alphatreeList + (len + 1);
        }

    }

    ~AlphaRFT(){
        RFT<>::release(rft);
        for(size_t i = 0; i < alphatreeIds_->getSize(); ++i)
            af_->releaseAlphaTree((*alphatreeIds_)[i]);
        delete featureList_;
        delete alphatreeIds_;
        delete alphaNameList_;
    }

    AlphaForest* af_;
    Vector<int>* alphatreeIds_;
    Vector<string>* alphaNameList_;
    Vector<AlphaSignIterator*>* featureList_;
    RFT<>* rft;
    int threadNum_;
    static AlphaRFT* alphaRFT_;
};

AlphaRFT *AlphaRFT::alphaRFT_ = nullptr;

#endif //ALPHATREE_ALPHARFT_H
