//
// Created by godpgf on 18-3-29.
//

#ifndef ALPHATREE_ALPHAGBDT_H
#define ALPHATREE_ALPHAGBDT_H

#include "base/iterator.h"
#include "base/vector.h"
#include "alphaforest.h"
#include "cart/gbdt.h"

class AlphaGBDT{
public:
    static void initialize(AlphaForest* af, const char* alphatreeList, int alphatreeNum, float gamma, float lambda, int threadNum, const char* lossFunName = "binary:logistic"){
        alphaGBDT_ = new AlphaGBDT(af, alphatreeList, alphatreeNum, gamma, lambda, threadNum, lossFunName);
    }

    static void release(){
        if(alphaGBDT_ != nullptr)
            delete alphaGBDT_;
        alphaGBDT_ = nullptr;
    }

    static AlphaGBDT* getAlphaGBDT(){ return alphaGBDT_;}

    void train(size_t daybefore, size_t sampleSize, const char* target, const char* signName,
               int barSize = 64, float minWeight = 32, int maxDepth = 16, int boostNum = 2, float boostWeightScale = 1, int cacheSize = 4096){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        //填写特征
        Vector<IBaseIterator<float>*> features("features", featureList_->getSize());
        for(size_t i = 0; i < features.getSize(); ++i){
            //cout<<(*alphaNameList_)[i].c_str()<<" "<<daybefore<<" "<<sampleSize<<endl;
            features[i] = new AlphaSignIterator(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i],daybefore,sampleSize,0,signNum, cacheSize);
        }

        int equalIndex = 0;
        while (target[equalIndex] != '=')
            ++equalIndex;
        string targetName = string(target, 0, equalIndex);
        int targetAlphatreeId = af_->useAlphaTree();
        af_->decode(targetAlphatreeId, targetName.c_str(), target + (equalIndex + 1));

        //填写目标
        AlphaSignIterator targetIter(af_, targetName.c_str(), signName, targetAlphatreeId, daybefore, sampleSize, 0, signNum, cacheSize);

        //申请中间内存
        float* memory = new float[4 * signNum];
        int* fmemory = new int[signNum];
        MemoryIterator<float> weight(memory, signNum);
        weight.initialize(1);
        MemoryIterator<float> pred(memory + signNum, signNum);
        MemoryIterator<float> g(memory + 2 * signNum, signNum);
        MemoryIterator<float> h(memory + 3 * signNum, signNum);
        MemoryIterator<int> flag(fmemory, signNum);

        gbdt->train(&features, &weight, &targetIter, &g, &h, &flag, &pred, lossFunName, barSize, minWeight, maxDepth, boostNum, boostWeightScale);

        //释放空间
        delete []memory;
        delete []fmemory;

        //删除特征
        for(size_t i = 0; i < features.getSize(); ++i)
            delete features[i];
        af_->releaseAlphaTree(targetAlphatreeId);
    }

    void pred(size_t daybefore, size_t sampleSize, const char* signName, float* predOut, int cacheSize = 4096){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        //填写特征
        Vector<IBaseIterator<float>*> features("features", featureList_->getSize());
        for(size_t i = 0; i < features.getSize(); ++i){
            //cout<<(*alphaNameList_)[i].c_str()<<" "<<daybefore<<" "<<sampleSize<<endl;
            features[i] = new AlphaSignIterator(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i],daybefore,sampleSize,0,signNum, cacheSize);
        }

        //申请中间内存
        int* fmemory = new int[signNum];
        MemoryIterator<int> flag(fmemory, signNum);

        MemoryIterator<float> pred(predOut, signNum);

        gbdt->pred(&features, &flag, &pred, lossFunName);

        //删除特征
        for(size_t i = 0; i < features.getSize(); ++i)
            delete features[i];
        delete []fmemory;
    }

    int tostring(char* out){
        strcpy(out, gbdt->tostring(alphaNameList_).c_str());
        return strlen(out);
    }

    void saveModel(const char* path){
        gbdt->saveModel(path);
    }

    void loadModel(const char* path){
        gbdt->loadModel(path);
    }
protected:
    AlphaGBDT(AlphaForest* af, const char* alphatreeList, int alphatreeNum, float gamma, float lambda, int threadNum, const char* lossFunName = "binary:logistic"):af_(af){
            strcpy(this->lossFunName, lossFunName);
            gbdt = GBDT<>::create(threadNum, gamma, lambda);
            featureList_ = new Vector<AlphaSignIterator*>("featureList", alphatreeNum);
            alphatreeIds_ = new Vector<int>("alphatreeIds", alphatreeNum);
            alphaNameList_ = new Vector<string>("alphaNameList", alphatreeNum);
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

    ~AlphaGBDT(){
        GBDT<>::release(gbdt);
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
    GBDT<>* gbdt;
    char lossFunName[128];

    static AlphaGBDT* alphaGBDT_;
};

AlphaGBDT *AlphaGBDT::alphaGBDT_ = nullptr;

#endif //ALPHATREE_ALPHAGBDT_H
