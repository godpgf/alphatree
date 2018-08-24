//
// Created by godpgf on 18-7-25.
//

#ifndef ALPHATREE_ALPHAFILTER_H
#define ALPHATREE_ALPHAFILTER_H


#include <list>
#include "dtree.h"
#include "../alphaforest.h"

#define DTREE_BLOCK_SIZE 32

class AlphaFilter {
protected:
    class Node{
    public:
        void clean() {
            featureIndex = -1;
        }

        float minValue, maxValue;
        float support, confidence;
        int featureIndex;
    };

public:
    static void initialize(AlphaForest *af, const char *alphatreeList, const int* alphatreeFlag, int alphatreeNum, const char *target, const char* open){
        alphaFilter_ = new AlphaFilter(af, alphatreeList, alphatreeFlag, alphatreeNum, target, open);
    }

    static void release(){
        if(alphaFilter_ != nullptr)
            delete alphaFilter_;
        alphaFilter_ = nullptr;
    }

    static AlphaFilter* getAlphaFilter(){ return alphaFilter_;}

    void train(const char* signName, int daybefore, int sampleSize, int sampleTime, float support, float confidence, float firstConfidence, float secondConfidence){
        Vector<float> avgList(alphatreeIds_->getSize());
        Vector<float> stdList(alphatreeIds_->getSize());
        calSplitValue(avgList, stdList, signName, daybefore, sampleSize, sampleTime);

        Vector<int*> skipList(sampleTime);
        Vector<int> skipLenList(sampleSize);
        Vector<IBaseIterator<float>*> targetList(sampleTime);
        Vector<Vector<IBaseIterator<float>*>*> featuresList(sampleTime);
        for(int i = 0; i < sampleTime; ++i){
            size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore + sampleSize * i, sampleSize, signName);
            skipList[i] = new int[signNum * alphatreeIds_->getSize()];

            IBaseIterator<float>* target = new AlphaSignIterator(af_, "t", signName, targetTreeId, daybefore + i * sampleSize, sampleSize, 0, signNum);
            targetList[i] = target;

            Vector<IBaseIterator<float>*>* features = new Vector<IBaseIterator<float>*>(alphatreeIds_->getSize());
            skipLenList[i] = createFeatureList((*features), skipList[i], signName, daybefore, sampleSize, i);
            featuresList[i] = features;
        }

        bool* hasChoose = new bool[alphatreeIds_->getSize()];
        memset(hasChoose, 0, alphatreeIds_->getSize() * sizeof(bool));

        Vector<int> firstHeroSkipLenList(sampleTime);
        Vector<int*> firstHeroSkipList(sampleTime);
        Vector<int> secondHeroSkipLenList(sampleTime);
        Vector<int*> secondHeroSkipList(sampleTime);
        for(int heroFeatureId1 = 0; heroFeatureId1 < alphatreeIds_->getSize(); ++heroFeatureId1) {
            if (((*alphaFlags_)[heroFeatureId1] & 2) != 0) {
                hasChoose[heroFeatureId1] = true;
                for(int cmpFlag1 = 0; cmpFlag1 < 2; ++cmpFlag1) {
                    float minValue = (cmpFlag1 == 0) ? -FLT_MAX : avgList[heroFeatureId1] + 2 * stdList[heroFeatureId1];
                    float maxValue = (cmpFlag1 == 0) ? avgList[heroFeatureId1] - 2 * stdList[heroFeatureId1] : FLT_MAX;

                    float firstHeroSupport = 1;
                    float firstHeroConfidence = 1;
                    calSupportAndConfidence(minValue, maxValue, featuresList, heroFeatureId1, targetList, support, firstConfidence, skipList, skipLenList, firstHeroSkipList, firstHeroSkipLenList, sampleTime, firstHeroSupport, firstHeroConfidence);
                    cout<<"first hero support and confidence:"<<firstHeroSupport<<" "<<firstHeroConfidence<<endl;
                    if(firstHeroSupport > support && firstHeroConfidence > firstConfidence){
                        cout<<"id:"<<heroFeatureId1<<" hero1 support:"<<firstHeroSupport<<" confidence:"<<firstHeroConfidence<<endl;
                        int curRootId = tree_->createNode();
                        (*tree_)[curRootId].featureIndex = heroFeatureId1;
                        (*tree_)[curRootId].minValue = minValue;
                        (*tree_)[curRootId].maxValue = maxValue;
                        (*tree_)[curRootId].confidence = firstHeroConfidence;
                        (*tree_)[curRootId].support = firstHeroSupport;
                        float maxRootConfidence = (*tree_)[curRootId].confidence;
                        for(int j = 0; j < alphatreeIds_->getSize(); ++j){
                            if(hasChoose[j] == false) {
                                maxRootConfidence = max(maxRootConfidence,
                                                        filter(featuresList, avgList, stdList, targetList, sampleTime,
                                                               support, confidence, firstHeroSkipList,
                                                               firstHeroSkipLenList, hasChoose, j, curRootId));
                            }
                        }
                        cout<<"test second hero\n";

                        for(int heroFeatureId2 = heroFeatureId1 + 1;  heroFeatureId2 < alphatreeIds_->getSize(); ++ heroFeatureId2){
                            if(((*alphaFlags_)[heroFeatureId2] & 2) != 0){
                                hasChoose[heroFeatureId2] = true;
                                for(int cmpFlag2 = 0; cmpFlag2 < 2; ++cmpFlag2) {
                                    float minValue = (cmpFlag2 == 0) ? -FLT_MAX : avgList[heroFeatureId2] +
                                                                                  2 * stdList[heroFeatureId2];
                                    float maxValue = (cmpFlag2 == 0) ? avgList[heroFeatureId2] -
                                                                       2 * stdList[heroFeatureId2] : FLT_MAX;
                                    float secondHeroSupport = 1;
                                    float secondHeroConfidence = 1;
                                    calSupportAndConfidence(minValue, maxValue, featuresList, heroFeatureId2, targetList, support, secondConfidence, firstHeroSkipList, firstHeroSkipLenList, secondHeroSkipList, secondHeroSkipLenList, sampleTime, secondHeroSupport, secondHeroConfidence);
                                    if(secondHeroSupport > support && secondHeroConfidence > secondConfidence){
                                        cout<<"hero2 support:"<<secondHeroSupport<<" confidence:"<<secondHeroConfidence<<endl;
                                        int subRootId = tree_->createNode();
                                        (*tree_)[subRootId].featureIndex = heroFeatureId2;
                                        (*tree_)[subRootId].minValue = minValue;
                                        (*tree_)[subRootId].maxValue = maxValue;
                                        (*tree_)[subRootId].confidence = secondHeroConfidence;
                                        (*tree_)[subRootId].support = secondHeroSupport;
                                        //cout<<"    confidence:"<<(*tree_)[nodeId].confidence<<endl;
                                        float maxSubRootConfidence = (*tree_)[subRootId].confidence;
                                        for(int j = 0; j < alphatreeIds_->getSize(); ++j){
                                            if(hasChoose[j] == false)
                                                maxSubRootConfidence = max(maxSubRootConfidence, filter(featuresList, avgList, stdList, targetList, sampleTime, support, confidence, secondHeroSkipList, secondHeroSkipLenList, hasChoose, j, subRootId));
                                        }

                                        if(maxSubRootConfidence > max((*tree_)[subRootId].support, support)){
                                            maxRootConfidence = max(maxRootConfidence, maxSubRootConfidence);
                                        } else {
                                            tree_->cleanNode(subRootId);
                                        }
                                    }
                                }

                                hasChoose[heroFeatureId2] = false;
                            }
                        }

                        //------------------
                        if(maxRootConfidence > confidence){
                            cout<<"success!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
                            rootIds_.push_back(curRootId);
                        } else {
                            tree_->cleanNode(curRootId);
                        }

                    }

                }

                hasChoose[heroFeatureId1] = false;
            }
        }

        for(int i = 0; i < sampleTime; ++i){
            delete []skipList[i];
            delete targetList[i];
            for(int j = 0; j < alphatreeIds_->getSize(); ++j){
                delete (*featuresList[i])[j];
            }
            delete featuresList[i];
        }
    }

    int pred(const char* signName, int daybefore, int sampleSize, float* predOut, float* openMinValue, float* openMaxValue){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize, signName);
        memset(predOut, 0, signNum * sizeof(float));
        if(isOpenBuy_){
            for(int i = 0; i < signNum; ++i){
                openMinValue[i] = -FLT_MAX;
                openMaxValue[i] = FLT_MAX;
            }
        }

        Vector<IBaseIterator<float>*> featureList(alphatreeIds_->getSize());
        int* skip = new int[signNum * 3];
        int skipLen = createFeatureList(featureList, skip, signName, daybefore, sampleSize);
        for(list<int>::iterator iter = rootIds_.begin(); iter != rootIds_.end();)
        {
            int rootId = *iter;
            pred(rootId, featureList, skip, skipLen, predOut, openMinValue, openMaxValue);
        }
        delete[] skip;
        return signNum;
    }


    void saveModel(const char* path){
        ofstream file;
        file.open(path, ios::binary);
        //1、写入树数
        int treeNum = rootIds_.size();
        file.write(reinterpret_cast<const char * >( &treeNum ), sizeof(int));
        for(list<int>::iterator iter = rootIds_.begin(); iter != rootIds_.end();){
            int rootId = *iter;
            int nodeNum = tree_->getNodeNum(rootId);
            file.write(reinterpret_cast<const char * >( &nodeNum ), sizeof(int));
            int nodeIndex = 0;
            saveTree(file, rootId, nodeIndex);
        }
        file.close();
    }

    void loadModel(const char* path){
        tree_->releaseAll();
        rootIds_.clear();
        ifstream file;
        file.open(path, ios::binary);
        int treeNum;
        file.read(reinterpret_cast< char* >( &treeNum ), sizeof( int ));
        int offset = 0;
        for(int i = 0; i < treeNum; ++i){
            int nodeNum;
            file.read(reinterpret_cast< char* >( &nodeNum ), sizeof( int ));

            for(int j = 0; j < nodeNum; ++j){
                int nodeId = tree_->createNode();

                int childNum;
                file.read(reinterpret_cast< char* >( &childNum ), sizeof( int ));
                for(int k = 0; k < childNum; ++k){
                    int childIndex;
                    file.read(reinterpret_cast< char* >( &childIndex ), sizeof( int ));
                    tree_->addChild(nodeId, offset + childIndex);
                }

                file.read(reinterpret_cast< char* >( &(*tree_)[nodeId].featureIndex ), sizeof( int ));
                file.read(reinterpret_cast< char* >( &(*tree_)[nodeId].minValue ), sizeof( float ));
                file.read(reinterpret_cast< char* >( &(*tree_)[nodeId].maxValue ), sizeof( float ));
                file.read(reinterpret_cast< char* >( &(*tree_)[nodeId].support ), sizeof( float ));
                file.read(reinterpret_cast< char* >( &(*tree_)[nodeId].confidence ), sizeof( float ));

                if(j == nodeNum - 1){
                    rootIds_.push_back(nodeId);
                }
            }
            offset += nodeNum;
        }
        file.close();
    }

    int tostring(char* out){
        strcpy(out, tostring().c_str());
        return strlen(out);
    }


protected:
    AlphaFilter(AlphaForest *af, const char *alphatreeList, const int* alphaFlagList, int alphatreeNum, const char *target, const char* open) : af_(af){
        tree_ = DTree<Node, DTREE_BLOCK_SIZE>::create();

        if(open != nullptr){
            isOpenBuy_ = true;
            ++alphatreeNum;
        }else{
            isOpenBuy_ = false;
        }


        alphatreeIds_ = new Vector<int>(alphatreeNum);
        alphaFlags_ = new Vector<int>(alphatreeNum);
        alphaNameList_ = new Vector<string>(alphatreeNum);
        int equalIndex;
        const char* tmp = alphatreeList;
        for (int i = 0; i < alphatreeNum; ++i) {
            if(i == 0 && open != nullptr){
                tmp = open;
            }
            equalIndex = 0;
            while (tmp[equalIndex] != '=')
                ++equalIndex;
            (*alphaNameList_)[i] = string(tmp, 0, equalIndex);
            //cout<<(*alphaNameList_)[i]<<"="<<tmp + (equalIndex + 1)<<endl;
            //cout<<(*alphaNameList_)[i].c_str()<<" "<<tmp + (equalIndex + 1)<<endl;
            (*alphatreeIds_)[i] = af->useAlphaTree();
            af->decode((*alphatreeIds_)[i], (*alphaNameList_)[i].c_str(), tmp + (equalIndex + 1));

            if(i == 0 && open != nullptr){
                tmp = alphatreeList;
                (*alphaFlags_)[i] = 3;
            } else{
                int len = strlen(tmp);
                tmp = tmp + (len + 1);
                (*alphaFlags_)[i] = alphaFlagList[0];
                alphaFlagList += 1;
            }
        }

        targetTreeId = af->useAlphaTree();
        af->decode(targetTreeId, "t", target);
    }

    ~AlphaFilter() {
        for (size_t i = 0; i < alphatreeIds_->getSize(); ++i)
            af_->releaseAlphaTree((*alphatreeIds_)[i]);
        delete alphatreeIds_;
        delete alphaFlags_;
        delete alphaNameList_;

        af_->releaseAlphaTree(targetTreeId);
    }

    inline void calSupportAndConfidence(float minValue, float maxValue, Vector<Vector<IBaseIterator<float>*>*>& featuresList, int featureId, Vector<IBaseIterator<float>*>& targetList, float support, float confidence, Vector<int*>& skipList, Vector<int>& skipLenList, Vector<int*>& outSkipList, Vector<int>& outSkipLenList, int sampleTime, float& outSupport, float& outConfidence){
        outSupport = 1;
        outConfidence = 1;
        for(int i = 0; i < sampleTime; ++i){
            Vector<IBaseIterator<float>*>* features = featuresList[i];
            int* skip = skipList[i];
            int skipLen = skipLenList[i];
            int newSkipLen = updateSkip((*features)[featureId], skip, skipLen, skip + skipLen, minValue, maxValue);
            outSkipLenList[i] = newSkipLen;
            outSkipList[i] = skip + skipLen;

            outSupport = min(outSupport, ((float)newSkipLen) / (*features)[featureId]->size());
            if(outSupport <= support){
                break;
            }
            outConfidence = min(outConfidence, calConfidence(outSkipList[i], outSkipLenList[i], targetList[i]));
            if(outConfidence <= confidence){
                break;
            }
        }
    }

    int createFeatureList(Vector<IBaseIterator<float>*>& featureList, int* skip, const char* signName, int daybefore, int sampleSize, int sampleIndex = 0){
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore + sampleIndex * sampleSize, sampleSize, signName);
        for(int i = 0; i < alphatreeIds_->getSize(); ++i){
            featureList[i] = new AlphaSignIterator(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i], daybefore + sampleIndex * sampleSize, sampleSize, 0, signNum);
        }

        for(int i = 0; i < signNum; ++i){
            skip[i] = 1;
        }
        skip[0] = 0;
        return signNum;
    }

    /*void destrouFeatureList(Vector<IBaseIterator<float>*>& featureList){
        for(int i = 0; i < featureList.getSize(); ++i){
            delete featureList[i];
        }
    }

    void destroyTarget(IBaseIterator<float>*& target){
        delete target;
    }*/

    size_t calSplitValue(Vector<float>& avgList, Vector<float>& stdList, const char* signName, int daybefore, int sampleSize, int sampleTime){
        //cout<<signName<<" "<<daybefore<<" "<<sampleSize * sampleTime<<endl;
        size_t signNum = af_->getAlphaDataBase()->getSignNum(daybefore, sampleSize * sampleTime, signName);
        //cout<<signNum<<endl;
        for(int i = 0; i < alphatreeIds_->getSize(); ++i){
            //cout<<(*alphaNameList_)[i].c_str()<<endl;
            AlphaSignIterator feature(af_, (*alphaNameList_)[i].c_str(), signName, (*alphatreeIds_)[i], daybefore, sampleSize * sampleTime, 0, signNum);
            float avg, std;
            calSplitValue(feature, avg, std);
            avgList[i] = avg;
            stdList[i] = std;
            //cout<<i <<" "<<avg<<" "<<std<<endl;
        }
        return signNum;
    }

    void calSplitValue(AlphaSignIterator& feature, float& avg, float& std){
        double sum = 0;
        double sumSqr = 0;
        double dataCount = feature.size();
        while (feature.isValid()){
            float value = (*feature);
            sum += value / dataCount;
            sumSqr += value * value / dataCount;
            feature.skip(1);
        }
        feature.skip(0, false);

        avg = sum;
        std = sqrtf(sumSqr  - avg * avg);
    }

    float filter(Vector<Vector<IBaseIterator<float>*>*>& featuresList, Vector<float>& avgList, Vector<float>& stdList, Vector<IBaseIterator<float>*>& targetList, int sampleTime, float support, float confidence, Vector<int*>& skipList, Vector<int>& skipLenList, bool* hasChoose, int startIndex, int rootId){
        if(hasChoose[startIndex])
            return 0;

        float maxConfidence = 0;
        //cout<<startIndex<<":\n";
        float minValue = -FLT_MAX;
        float maxValue = avgList[startIndex] - 1.6f * stdList[startIndex];
        //cout<<minValue<<" "<<maxValue<<endl;
        maxConfidence = max(maxConfidence, filter(minValue, maxValue, featuresList, avgList, stdList, targetList, sampleTime, support, confidence, skipList, skipLenList, hasChoose, startIndex, rootId));

        minValue = maxValue;
        maxValue = avgList[startIndex];
        //cout<<minValue<<" "<<maxValue<<endl;
        maxConfidence = max(maxConfidence, filter(minValue, maxValue, featuresList, avgList, stdList, targetList, sampleTime, support, confidence, skipList, skipLenList, hasChoose, startIndex, rootId));

        minValue = maxValue;
        maxValue = avgList[startIndex] + 1.6f * stdList[startIndex];
        //cout<<minValue<<" "<<maxValue<<endl;
        maxConfidence = max(maxConfidence, filter(minValue, maxValue, featuresList, avgList, stdList, targetList, sampleTime, support, confidence, skipList, skipLenList, hasChoose, startIndex, rootId));

        minValue = maxValue;
        maxValue = FLT_MAX;
        //cout<<minValue<<" "<<maxValue<<endl;
        maxConfidence = max(maxConfidence, filter(minValue, maxValue, featuresList, avgList, stdList, targetList, sampleTime, support, confidence, skipList, skipLenList, hasChoose, startIndex, rootId));

        return maxConfidence;

    }

    float filter(float minValue, float maxValue, Vector<Vector<IBaseIterator<float>*>*>& featuresList, Vector<float>& avgList, Vector<float>& stdList, Vector<IBaseIterator<float>*>& targetList, int sampleTime, float support, float confidence, Vector<int*>& skipList, Vector<int>& skipLenList, bool* hasChoose, int startIndex, int rootId){
        Vector<int*> outSkipList(sampleTime);
        Vector<int> outSkipLenList(sampleTime);
        float curSupport = 1;
        float curConfidence = 1;
        //cout<<"start index: "<<startIndex<<" ("<<minValue<<","<<maxValue<<")"<<endl;
        for(int i = 0; i < sampleTime; ++i){
            //cout<<"i:"<<i<<endl;
            //outSkipList[i] = skipList[i] + skipLenList[i];
            int* skip = skipList[i];
            int skipLen = skipLenList[i];
            //cout<<"skipLen:"<<skipLen<<endl;
            Vector<IBaseIterator<float>*>* features = featuresList[i];
            int* outSkip = skip + skipLen;
            int outSkipLen = updateSkip((*features)[startIndex], skip, skipLen, outSkip, minValue, maxValue);
            outSkipList[i] = outSkip;
            outSkipLenList[i] = outSkipLen;
            //cout<<outSkipLen<<" "<<(*features)[startIndex]->size()<<endl;
            curSupport = min(curSupport, ((float)outSkipLen) / (*features)[startIndex]->size());
            if(curSupport <= support){
                break;
            }

            //cout<<":"<<i<<" "<<targetList[i]->size()<<"\n";
            curConfidence = min(curConfidence, calConfidence(outSkip, outSkipLen, targetList[i]));
            //cout<<"cfd:"<<curConfidence<<endl;
        }

        if(curSupport > support){
            int nodeId = tree_->createNode();
            (*tree_)[nodeId].featureIndex = startIndex;
            (*tree_)[nodeId].minValue = minValue;
            (*tree_)[nodeId].maxValue = maxValue;
            (*tree_)[nodeId].confidence = curConfidence;
            (*tree_)[nodeId].support = curSupport;

            float maxChildConfidence = (*tree_)[nodeId].confidence;
            if(curSupport > support * 1.6f){
                for(int i = startIndex + 1; i < alphatreeIds_->getSize(); ++i){
                    if(hasChoose[i] == false){
                        maxChildConfidence = max(maxChildConfidence, filter(featuresList, avgList, stdList, targetList,  sampleTime, support, confidence, outSkipList, outSkipLenList, hasChoose, i, nodeId));
                    }
                }
            }

            //cout<<"support:"<<curSupport<<"    confidence:"<<(*tree_)[nodeId].confidence<<endl;
            if(maxChildConfidence > confidence){
                if(maxChildConfidence > (*tree_)[rootId].confidence){
                    cout<<startIndex<<" support:"<<curSupport<<"    confidence:"<<(*tree_)[nodeId].confidence<<endl;
                    tree_->addChild(rootId, nodeId);
                } else {
                    tree_->cleanNode(nodeId);
                }
                return maxChildConfidence;
            } else {
                tree_->cleanNode(nodeId);
                return 0;
            }
        } else {
            return 0;
        }

    }

    bool pred(int nodeId, Vector<IBaseIterator<float>*>& featureList, int* skip, int skipLen, float* predOut, float* openMinValue, float* openMaxValue){
        int* outSkip = skip + skipLen;
        int newSkipLen = updateSkip(featureList[(*tree_)[nodeId].featureIndex], skip, skipLen, outSkip, (*tree_)[nodeId].minValue, (*tree_)[nodeId].maxValue);
        if(newSkipLen == 0)
            return false;
        bool isPred = false;
        if(tree_->getChildNum(nodeId) > 0){
            for(int i = 0; i < tree_->getChildNum(nodeId); ++i){
                isPred = (isPred || pred(tree_->getChild(nodeId, i), featureList, outSkip, newSkipLen, predOut, openMinValue, openMaxValue));
            }
        }
        if(!isPred){
            int curSkip = 0;
            for(int i = 0; i < skipLen; ++i){
                curSkip += skip[i];
                if((*tree_)[nodeId].confidence > predOut[curSkip]){
                    predOut[curSkip] = (*tree_)[nodeId].confidence;
                    if(isOpenBuy_){
                        int rid = tree_->getRoot(nodeId);
                        if((*tree_)[rid].featureIndex == 0){
                            openMinValue[curSkip] = (*tree_)[rid].minValue;
                            openMaxValue[curSkip] = (*tree_)[rid].maxValue;
                        }
                    }
                }
            }
        }
        return isPred;
    }

    float calConfidence(int* skip, int skipLen, IBaseIterator<float>* target){
        int rightCnt = 0;
        for(int i = 0; i < skipLen; ++i){
            target->skip(skip[i]);
            if((**target) > 0.5f)
                ++rightCnt;
        }
        target->skip(0, false);
        return float(rightCnt) / skipLen;
    }

    static int updateSkip(IBaseIterator<float>* feature, int* skip, int skipLen, int* outSkip, float minValue, float maxValue){
        int outSkipLen = 0;
        int curIndex = 0;
        for(int i = 0; i < skipLen; ++i){
            feature->skip(skip[i]);
            curIndex += skip[i];
            if((**feature) > minValue && (**feature) <= maxValue){
                outSkip[outSkipLen++] = curIndex;
            }
        }

        for(int i = outSkipLen - 1; i > 0; --i){
            outSkip[i] -= outSkip[i-1];
        }

        feature->skip(0, false);
        return outSkipLen;
    }

    int saveTree(ofstream& file, int rootId, int& nodeIndex){
        int childNum = tree_->getChildNum(rootId);
        int* children = new int[childNum];
        for(int i = 0; i < childNum; ++i){
            children[i] = saveTree(file, tree_->getChild(rootId, i), nodeIndex);
        }


        //写入节点
        file.write(reinterpret_cast<const char * >( &childNum ), sizeof(int));
        for(int i = 0; i < childNum; ++i){
            file.write(reinterpret_cast<const char * >( &children[i] ), sizeof(int));
        }
        delete[] children;

        file.write(reinterpret_cast<const char * >( &(*tree_)[rootId].featureIndex), sizeof(int));
        file.write(reinterpret_cast<const char * >( &(*tree_)[rootId].minValue), sizeof(float));
        file.write(reinterpret_cast<const char * >( &(*tree_)[rootId].maxValue), sizeof(float));
        file.write(reinterpret_cast<const char * >( &(*tree_)[rootId].support), sizeof(float));
        file.write(reinterpret_cast<const char * >( &(*tree_)[rootId].confidence), sizeof(float));

        return nodeIndex++;
    }

    string tostring(){
        string str;
        for(list<int>::iterator iter = rootIds_.begin(); iter != rootIds_.end();){
            int rootId = *iter;
            tostring(str, rootId, 0);
        }
        return str;
    }

    void tostring(string& str, int rootId, int depth){
        for(int i = 0; i < depth; ++i)
            str += '\t';
        char tmp[128];
        str += (*tree_)[rootId].featureIndex < alphaNameList_->getSize() ? (*alphaNameList_)[(*tree_)[rootId].featureIndex] : "openReturns";
        if(tree_->getChildNum(rootId) == 0)
            sprintf(tmp," (%.4f, %.4f] support:%.4f confidence:%.4f", (*tree_)[rootId].minValue, (*tree_)[rootId].maxValue, (*tree_)[rootId].support, (*tree_)[rootId].confidence);
        else
            sprintf(tmp, " (%.4f, %.4f]", (*tree_)[rootId].minValue, (*tree_)[rootId].maxValue);
        str += tmp;
        str += '\n';
        for(int i = 0; i < tree_->getChildNum(rootId); ++i){
            tostring(str, tree_->getChild(rootId, i), depth + 1);
        }
    }

    AlphaForest *af_;
    Vector<int> *alphatreeIds_;
    Vector<int> *alphaFlags_;
    Vector<string> *alphaNameList_;

    int targetTreeId;
    DTree<Node, DTREE_BLOCK_SIZE>* tree_ = {nullptr};
    list<int> rootIds_;
    bool isOpenBuy_;

    static AlphaFilter* alphaFilter_;
};

AlphaFilter *AlphaFilter::alphaFilter_ = nullptr;

#endif //ALPHATREE_ALPHAFILTER_H
