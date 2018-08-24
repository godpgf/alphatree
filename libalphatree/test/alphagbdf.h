//
// Created by godpgf on 18-4-7.
//

#ifndef ALPHATREE_ALPHAGBDF_H
#define ALPHATREE_ALPHAGBDF_H

#include "iterator.h"
#include "vector.h"
#include "../alphaforest.h"
#include "../cart/gbdt.h"

class AlphaGBDF{
public:
    static void initialize(AlphaForest* af, const char* alphatreeList, int alphatreeNum, int forestSize, float gamma, float lambda, int threadNum, const char* lossFunName = "binary:logistic"){
        alphaGBDF_ = new AlphaGBDF(af, alphatreeList, alphatreeNum, forestSize, gamma, lambda, threadNum, lossFunName);
    }

    static void release(){
        if(alphaGBDF_ != nullptr)
            delete alphaGBDF_;
        alphaGBDF_ = nullptr;
    }

    static AlphaGBDF* getAlphaGBDF(){ return alphaGBDF_;}

    //把数据读出去，主要为了测试
    int testReadData(size_t daybefore, size_t sampleSize, const char* target, const char* signName, float* pFeature, float* pTarget, int cacheSize = 4096){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        //填写特征
        Vector<IBaseIterator<float>*> features(featureList_->getSize());
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

    void train(size_t daybefore, size_t sampleSize, const char* weight, const char* target, const char* signName, float samplePercent = 1, float featurePercent = 1,
               int barSize = 64, float minWeight = 32, int maxDepth = 16, int boostNum = 2, float boostWeightScale = 1, int cacheSize = 4096){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        //填写特征
        Vector<IBaseIterator<float>*> features(featureList_->getSize());
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

        //申请中间内存
        float* memory = new float[3 * signNum * threadNum_];
        //分配3份内存，第一份是真实用的，第二份是中间结果，第三份是初始取样
        int* skip = new int[3 * signNum * threadNum_];

        Vector<IBaseIterator<float>*> weights(threadNum_);
        Vector<MemoryIterator<float>*> preds( threadNum_);
        Vector<MemoryIterator<float>*> gs(threadNum_);
        Vector<MemoryIterator<float>*> hs( threadNum_);
        Vector<IBaseIterator<float>*> targets(  threadNum_);
        for(int i = 0; i < threadNum_; ++i){
            weights[i] = weightIter->clone();
            preds[i] = new MemoryIterator<float>(memory + (i * 3 * signNum), signNum);
            gs[i] = new MemoryIterator<float>(memory + (i * 3 * signNum + 1 * signNum), signNum);
            hs[i] = new MemoryIterator<float>(memory + (i * 3 * signNum + 2 * signNum), signNum);
            targets[i] = targetIter.clone();
        }

        ThreadPool threadPool(threadNum_);

        std::shared_future<void>* res = new std::shared_future<void>[threadNum_];
        for(int i = 0; i < gbdf->getSize(); i+= threadNum_){
            for(int j = 0; j < threadNum_; ++j){
                if(i + j < gbdf->getSize()){
                    auto* weight = weights[j];
                    auto* pred = preds[j];
                    auto* g = gs[j];
                    auto* h = hs[j];
                    int* curSkip = skip + j * signNum * 3;
                    auto* gbdt = (*gbdf)[i + j];
                    auto* curFeatures = &features;
                    auto* curTarget = targets[j];
                    auto* lossFunName = this->lossFunName;
                    res[j] = threadPool.enqueue([gbdt, curFeatures, weight, curTarget, g, h, curSkip, pred, lossFunName, barSize, minWeight, maxDepth, samplePercent, featurePercent, boostNum, boostWeightScale]{
                        return gbdt->train(curFeatures, weight, curTarget, g, h, curSkip, pred, lossFunName, barSize, minWeight, maxDepth, samplePercent, featurePercent, boostNum, boostWeightScale);
                    }).share();
                } else {
                    break;
                }
            }
            for(int j = 0; j < threadNum_; ++j){
                if(i + j < gbdf->getSize())
                    res[j].get();
            }
        }
        //gbdt->train(&features, &weight, &targetIter, &g, &h, skip, &pred, lossFunName, barSize, minWeight, maxDepth, 1, 1, boostNum, boostWeightScale);

        delete []res;
        for(int i = 0; i < threadNum_; ++i){
            delete weights[i];
            delete preds[i];
            delete gs[i];
            delete hs[i];
            delete targets[i];
        }

        //释放空间
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

    void pred(size_t daybefore, size_t sampleSize, const char* signName, float* predOut, int cacheSize = 4096){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        //填写特征
        Vector<Vector<IBaseIterator<float>*>*> features( threadNum_);
        Vector<MemoryIterator<float>*> preds( threadNum_);
        float* tmpPred = new float[signNum * threadNum_];
        for(int i = 0; i < threadNum_; ++i){
            features[i] = new Vector<IBaseIterator<float>*>(featureList_->getSize());
            preds[i] = new MemoryIterator<float>(tmpPred + i * signNum, signNum);
            for(int j = 0; j < featureList_->getSize(); ++j){
                (*features[i])[j] = new AlphaSignIterator(af_, (*alphaNameList_)[j].c_str(), signName, (*alphatreeIds_)[j],daybefore,sampleSize,0,signNum, cacheSize);
            }
        }

        //申请中间内存
        int* skip = new int[signNum * 2 * threadNum_];
        MemoryIterator<float> pred(predOut, signNum);
        pred.initialize(0);

        ThreadPool threadPool(threadNum_);
        std::shared_future<void>* res = new std::shared_future<void>[threadNum_];

        for(int i = 0; i < gbdf->getSize(); i += threadNum_){
            for(int j = 0; j < threadNum_; ++j){
                if(i + j < gbdf->getSize()){
                    auto* curFeatures = features[j];
                    auto* curSkip = skip + j * signNum * 2;
                    auto* curPred = preds[j];
                    auto* gbdt = (*gbdf)[i + j];
                    auto* lossFunName = this->lossFunName;
                    res[j] = threadPool.enqueue([gbdt, curFeatures, curSkip, curPred, lossFunName]{
                        return gbdt->pred(curFeatures, curSkip, curPred, lossFunName);
                    }).share();
                } else {
                    break;
                }
            }
            for(int j = 0; j < threadNum_; ++j){
                if(i + j < gbdf->getSize()){
                    res[j].get();
                    int k = 0;
                    auto* curPred = preds[j];
                    while (curPred->isValid()){
                        //cout<<curPred->getValue()<<endl;
                        predOut[k++] += curPred->getValue() / gbdf->getSize();
                        curPred->skip(1);
                    }
                    curPred->skip(0, false);
                }
            }
        }

        //删除特征
        for(size_t i = 0; i < threadNum_; ++i){
            for(int j = 0; j < featureList_->getSize(); ++j)
                delete (*features[i])[j];
            delete features[i];
            delete preds[i];
        }

        delete []skip;
        delete []tmpPred;
    }

    int tostring(char* out){
        char* p = out;
        for(int i = 0; i < gbdf->getSize(); ++i){
            strcpy(p, (*gbdf)[i]->tostring(alphaNameList_).c_str());
            if(i < gbdf->getSize() - 1){
                p += strlen(p);
                p[0] = '\n';
                p += 1;
            }
        }

        return strlen(out);
    }

    void saveModel(const char* path){
        for(int i = 0; i < gbdf->getSize(); ++i){
            string newPath(path);
            newPath += ".tree";
            newPath += to_string(i);
            (*gbdf)[i]->saveModel(newPath.c_str());
        }
    }

    void loadModel(const char* path){
        for(int i = 0; i < gbdf->getSize(); ++i){
            string newPath(path);
            newPath += ".tree";
            newPath += to_string(i);
            (*gbdf)[i]->loadModel(newPath.c_str());
        }
    }

protected:
    AlphaGBDF(AlphaForest* af, const char* alphatreeList, int alphatreeNum, int forestSize, float gamma, float lambda, int threadNum, const char* lossFunName = "binary:logistic"):af_(af), threadNum_(threadNum){
            strcpy(this->lossFunName, lossFunName);
            gbdf = new Vector<GBDT<>*>( forestSize);
            for(int i = 0; i < gbdf->getSize(); ++i){
                (*gbdf)[i] = GBDT<>::create(threadNum, gamma, lambda);
            }
            featureList_ = new Vector<AlphaSignIterator*>(alphatreeNum);
            alphatreeIds_ = new Vector<int>(alphatreeNum);
            alphaNameList_ = new Vector<string>(alphatreeNum);
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

    ~AlphaGBDF(){
        for(int i = 0; i < gbdf->getSize(); ++i)
            GBDT<>::release((*gbdf)[i]);
        delete gbdf;
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
    Vector<GBDT<>*>* gbdf;
    char lossFunName[128];
    int threadNum_;

    static AlphaGBDF* alphaGBDF_;
};

AlphaGBDF *AlphaGBDF::alphaGBDF_ = nullptr;

#endif //ALPHATREE_ALPHAGBDF_H
