//
// Created by godpgf on 18-7-13.
//

#ifndef ALPHATREE_ALPHAGRAPH_H
#define ALPHATREE_ALPHAGRAPH_H

#include "alphaforest.h"
#include "alpha/alphapic.h"

class AlphaGraph{
public:
    static void initialize(AlphaForest* af){
        alphaGraph_ = new AlphaGraph(af);
    }

    static void release(){
        if(alphaGraph_ != nullptr)
            delete alphaGraph_;
        alphaGraph_ = nullptr;
    }

    static AlphaGraph* getAlphaGraph(){ return alphaGraph_;}

    int useAlphaPic(int alphaTreeId, const char* signName, const char* features, int featureSize, int dayBefore, int sampleSize){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
        Vector<IBaseIterator<float>*> featureIterList(featureSize);
        const char* rootName = features;
        for(int i = 0; i < featureSize; ++i){
            featureIterList[i] = new AlphaSignIterator(af_, rootName, signName, alphaTreeId, dayBefore, sampleSize, 0, signNum);
        }

        int id = alphaPicCache_->useCacheMemory();
        AlphaPic* alphaPic = getAlphaPic(id);
        alphaPic->initialize(&featureIterList);

        for(int i = 0; i < featureSize; ++i){
            delete featureIterList[i];
        }
		return id;
    }

    void getKLinePic(int picId, int alphaTreeId, const char* signName, const char* openElements, const char* highElements, const char* lowElements, const char* closeElements, int elementNum, int dayBefore, int sampleSize, float* outPic, int column, float maxStdScale){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
        Vector<IBaseIterator<float>*> openIterList(elementNum);
        Vector<IBaseIterator<float>*> highIterList(elementNum);
        Vector<IBaseIterator<float>*> lowIterList(elementNum);
        Vector<IBaseIterator<float>*> closeIterList(elementNum);
        const char* openRootName = openElements;
        const char* highRootName = highElements;
        const char* lowRootName = lowElements;
        const char* closeRootName = closeElements;
        for(int i = 0; i < elementNum; ++i){
            openIterList[i] = new AlphaSignIterator(af_, openRootName, signName, alphaTreeId, dayBefore, sampleSize, 0, signNum);
            highIterList[i] = new AlphaSignIterator(af_, highRootName, signName, alphaTreeId, dayBefore, sampleSize, 0, signNum);
            lowIterList[i] = new AlphaSignIterator(af_, lowRootName, signName, alphaTreeId, dayBefore, sampleSize, 0, signNum);
            closeIterList[i] = new AlphaSignIterator(af_, closeRootName, signName, alphaTreeId, dayBefore, sampleSize, 0, signNum);
            openRootName += (strlen(openRootName) + 1);
            highRootName += (strlen(highRootName) + 1);
            lowRootName += (strlen(lowRootName) + 1);
            closeRootName += (strlen(closeRootName) + 1);
        }

        AlphaPic* alphaPic = getAlphaPic(picId);
        alphaPic->getKLinePic(&openIterList, &highIterList, &lowIterList, &closeIterList, outPic, column, maxStdScale);

        for(int i = 0; i < elementNum; ++i){
            delete openIterList[i];
            delete highIterList[i];
            delete lowIterList[i];
            delete closeIterList[i];
        }
    }

    void getTrendPic(int picId, int alphaTreeId, const char* signName, const char* elements, int elementNum, int dayBefore, int sampleSize, float* outPic, int column, float maxStdScale){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
        Vector<IBaseIterator<float>*> iterList(elementNum);
        const char* rootName = elements;
        for(int i = 0; i < elementNum; ++i){
            iterList[i] = new AlphaSignIterator(af_, rootName, signName, alphaTreeId, dayBefore, sampleSize, 0, signNum);
            rootName += (strlen(rootName) + 1);
        }

        AlphaPic* alphaPic = getAlphaPic(picId);
        alphaPic->getTrendPic(&iterList, outPic, column, maxStdScale);

        for(int i = 0; i < elementNum; ++i){
            delete iterList[i];
        }
    }

    void releaseAlphaPic(int id){
        alphaPicCache_->releaseCacheMemory(id);
    }
protected:
    AlphaGraph(AlphaForest* af):af_(af){alphaPicCache_ = DCache<AlphaPic>::create();}
    ~AlphaGraph(){DCache<AlphaPic>::release(alphaPicCache_);}
    AlphaPic *getAlphaPic(int id) {
        return &alphaPicCache_->getCacheMemory(id);
    }
    //alpha pic memory
    DCache<AlphaPic> *alphaPicCache_ = {nullptr};
    AlphaForest* af_;


    static AlphaGraph* alphaGraph_;
};

 AlphaGraph* AlphaGraph::alphaGraph_ = nullptr;

#endif //ALPHATREE_ALPHAGRAPH_H
