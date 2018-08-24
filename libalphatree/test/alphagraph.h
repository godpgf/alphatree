//
// Created by godpgf on 18-7-13.
//

#ifndef ALPHATREE_ALPHAGRAPH_H
#define ALPHATREE_ALPHAGRAPH_H

#include "../alphaforest.h"
#include "test/alphapic.h"

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

    int useAlphaPic(const char* signName, const char* features, int featureSize, int dayBefore, int sampleSize){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
        Vector<IBaseIterator<float>*> featureIterList(featureSize);
        const char* feature = features;
        Vector<int> alphatreeIds(featureSize);
        for(int i = 0; i < featureSize; ++i){
            alphatreeIds[i] = af_->useAlphaTree();
            af_->decode(alphatreeIds[i], "_p", feature);
            featureIterList[i] = new AlphaSignIterator(af_, "_p", signName, alphatreeIds[i], dayBefore, sampleSize, 0, signNum);
            feature += (strlen(feature) + 1);
        }

        int id = alphaPicCache_->useCacheMemory();
        AlphaPic* alphaPic = getAlphaPic(id);
        alphaPic->initialize(&featureIterList);

        for(int i = 0; i < featureSize; ++i){
            delete featureIterList[i];
            af_->releaseAlphaTree(alphatreeIds[i]);
        }
		return id;
    }

    void getKLinePic(int picId, const char* signName, const char* openElements, const char* highElements, const char* lowElements, const char* closeElements, int elementNum, int dayBefore, int sampleSize, float* outPic, int column, float maxStdScale){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
        Vector<IBaseIterator<float>*> openIterList(elementNum);
        Vector<int> openTree(elementNum);
        Vector<IBaseIterator<float>*> highIterList(elementNum);
        Vector<int> highTree(elementNum);
        Vector<IBaseIterator<float>*> lowIterList(elementNum);
        Vector<int> lowTree(elementNum);
        Vector<IBaseIterator<float>*> closeIterList(elementNum);
        Vector<int> closeTree(elementNum);

        const char* openRootName = openElements;
        const char* highRootName = highElements;
        const char* lowRootName = lowElements;
        const char* closeRootName = closeElements;
        for(int i = 0; i < elementNum; ++i){
            openTree[i] = af_->useAlphaTree();
            af_->decode(openTree[i], "_p", openRootName);
            openIterList[i] = new AlphaSignIterator(af_, "_p", signName, openTree[i], dayBefore, sampleSize, 0, signNum);
            //cout<<openRootName<<" "<<dayBefore<<" "<<sampleSize<<" "<<openIterList[i]->getValue()<<endl;

            highTree[i] = af_->useAlphaTree();
            af_->decode(highTree[i], "_p", highRootName);
            highIterList[i] = new AlphaSignIterator(af_, "_p", signName, highTree[i], dayBefore, sampleSize, 0, signNum);
            //cout<<highRootName<<" "<<dayBefore<<" "<<sampleSize<<" "<<highIterList[i]->getValue()<<endl;

            lowTree[i] = af_->useAlphaTree();
            af_->decode(lowTree[i], "_p", lowRootName);
            lowIterList[i] = new AlphaSignIterator(af_, "_p", signName, lowTree[i], dayBefore, sampleSize, 0, signNum);
            //cout<<lowRootName<<" "<<dayBefore<<" "<<sampleSize<<" "<<lowIterList[i]->getValue()<<endl;

            closeTree[i] = af_->useAlphaTree();
            af_->decode(closeTree[i], "_p", closeRootName);
            closeIterList[i] = new AlphaSignIterator(af_, "_p", signName, closeTree[i], dayBefore, sampleSize, 0, signNum);
            //cout<<closeRootName<<" "<<dayBefore<<" "<<sampleSize<<" "<<closeIterList[i]->getValue()<<endl;

            openRootName += (strlen(openRootName) + 1);
            highRootName += (strlen(highRootName) + 1);
            lowRootName += (strlen(lowRootName) + 1);
            closeRootName += (strlen(closeRootName) + 1);
        }

        AlphaPic* alphaPic = getAlphaPic(picId);
        alphaPic->getKLinePic(&openIterList, &highIterList, &lowIterList, &closeIterList, outPic, column, maxStdScale);

        for(int i = 0; i < elementNum; ++i){
            delete openIterList[i];
            af_->releaseAlphaTree(openTree[i]);
            delete highIterList[i];
            af_->releaseAlphaTree(highTree[i]);
            delete lowIterList[i];
            af_->releaseAlphaTree(lowTree[i]);
            delete closeIterList[i];
            af_->releaseAlphaTree(closeTree[i]);
        }
    }

    void getTrendPic(int picId, const char* signName, const char* elements, int elementNum, int dayBefore, int sampleSize, float* outPic, int column, float maxStdScale){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(dayBefore, sampleSize, signName);
        Vector<IBaseIterator<float>*> iterList(elementNum);
        const char* rootName = elements;
        Vector<int> elementTree(elementNum);
        for(int i = 0; i < elementNum; ++i){
            elementTree[i] = af_->useAlphaTree();
            af_->decode(elementTree[i], "_p", rootName);
            iterList[i] = new AlphaSignIterator(af_, "_p", signName, elementTree[i], dayBefore, sampleSize, 0, signNum);
            rootName += (strlen(rootName) + 1);
        }

        AlphaPic* alphaPic = getAlphaPic(picId);
        alphaPic->getTrendPic(&iterList, outPic, column, maxStdScale);

        for(int i = 0; i < elementNum; ++i){
            delete iterList[i];
            af_->releaseAlphaTree(elementTree[i]);
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
