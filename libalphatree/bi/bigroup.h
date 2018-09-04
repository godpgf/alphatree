//
// Created by godpgf on 18-9-4.
// 缓存一组数据的统计指标，以便下次再用到时不用再次计算
//

#ifndef ALPHATREE_BIGROUP_H
#define ALPHATREE_BIGROUP_H

#include <string.h>
#include <glob.h>
#include <iostream>
using namespace std;

#define CLEAN_POINT(p) if(p != nullptr) delete[] p

class BIGroup{
public:
    BIGroup(){

    }

    virtual ~BIGroup(){
        cleanList();
        CLEAN_POINT(signName_);
        CLEAN_POINT(returnsList);
    }

    void initialize(const char* signName, size_t daybefore, size_t sampleSize, size_t  sampleTime, float support){
        if(signName_)
            CLEAN_POINT(signName_);
        signName_ = new char[strlen(signName) + 1];
        strcpy(signName_, signName);

        if(sampleTime > listLen_){
            cleanList();
            listLen_ = sampleTime;
        }
        initializeList(sampleTime);

        daybefore_ = daybefore;
        sampleSize_ = sampleSize;
        sampleTime_ = sampleTime;
        support_ = support;
    }

    float* initializeReturns(size_t len){
        if(len > returnsLen_){
            returnsLen_ = len;
            CLEAN_POINT(returnsList);
            returnsList = new float[len];
        }
        return returnsList;
    }

    size_t getDaybefore(){ return daybefore_;}
    size_t getSampleSize(){ return sampleSize_;}
    size_t getSampleTime(){ return sampleTime_;}
    float getSupport(){ return support_;}
    const char* getSignName(){return signName_;}

    float* controlAvgList = {nullptr};
    float* controlStdList = {nullptr};
    float* observationAvgList = {nullptr};
    float* observationStdList = {nullptr};
    float* featureAvgList = {nullptr};
    float* returnsAvgList = {nullptr};

    float* returnsList = {nullptr};
protected:
    virtual void cleanList(){
        CLEAN_POINT(controlAvgList);
        CLEAN_POINT(controlStdList);
        CLEAN_POINT(observationAvgList);
        CLEAN_POINT(observationStdList);
        CLEAN_POINT(featureAvgList);
        CLEAN_POINT(returnsAvgList);

    }

    virtual void initializeList(size_t sampleTime){
        controlAvgList = new float[sampleTime];
        controlStdList = new float[sampleTime];
        observationAvgList = new float[sampleTime];
        observationStdList = new float[sampleTime];
        featureAvgList = new float[sampleTime];
        returnsAvgList = new float[sampleTime];
    }

protected:

    size_t listLen_ = {0};
    size_t returnsLen_ = {0};
    char* signName_ = {nullptr};
    size_t daybefore_ = {0};
    size_t sampleSize_ = {0};
    size_t sampleTime_ = {0};
    float support_ = {0};

};

#endif //ALPHATREE_BIGROUP_H
