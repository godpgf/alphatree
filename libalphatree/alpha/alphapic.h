//
// Created by godpgf on 18-7-12.
//

#ifndef ALPHATREE_ALPHAPIC_H
#define ALPHATREE_ALPHAPIC_H

#include "alphatree.h"
#include <math.h>
#include <stdlib.h>
#include <float.h>
using namespace std;

class AlphaPic{
public:
    void initialize(IVector<IBaseIterator<float>*>* featureIterList){
        double sum = 0;
        double sumSqr = 0;
        double dataCount = (*featureIterList)[0]->size() * featureIterList->getSize();
        for(int i = 0; i < featureIterList->getSize(); ++i){
            IBaseIterator<float>* feature = (*featureIterList)[0];
            while (feature->isValid()){
                float value = (**feature);
                sum += value / dataCount;
                sumSqr += value * value / dataCount;
                feature->skip(1);
            }
            feature->skip(0, false);
        }
        meanValue = sum;
        stdValue = sqrtf(sumSqr  - meanValue * meanValue);
        //cout<<meanValue<<" "<<stdValue<<endl;
    }

    void getKLinePic(IVector<IBaseIterator<float>*>* openElement, IVector<IBaseIterator<float>*>* highElement, IVector<IBaseIterator<float>*>* lowElement, IVector<IBaseIterator<float>*>* closeElement, float* outPic, int column, float maxStdScale){
        int elementSize = openElement->getSize();
        int lineLen = elementSize * 3;
        memset(outPic, 0, lineLen * column * (*openElement)[0]->size() * sizeof(float));
        float* curPic = outPic;
        while ((*openElement)[0]->isValid()){
            for(int dayindex = 0; dayindex < elementSize; ++dayindex){
                //cout<<"o:"<<(*openElement)[dayindex]->getValue()<<" h:"<<(*highElement)[dayindex]->getValue()<<" l:"<<(*lowElement)[dayindex]->getValue()<<" c:"<<(*closeElement)[dayindex]->getValue()<<endl;
                int openIndex = data2index((*openElement)[dayindex]->getValue(), column, maxStdScale);
                int highIndex = data2index((*highElement)[dayindex]->getValue(), column, maxStdScale);
                int lowIndex = data2index((*lowElement)[dayindex]->getValue(), column, maxStdScale);
                int closeIndex = data2index((*closeElement)[dayindex]->getValue(), column, maxStdScale);

                curPic[openIndex * lineLen + dayindex * 3] = 1.f;
                for(int j = lowIndex; j <= highIndex; ++j)
                    curPic[j * lineLen + dayindex * 3 + 1] = 1.f;
                curPic[closeIndex * lineLen + dayindex * 3 + 2] = 1.f;

                (*openElement)[dayindex]->skip(1);
                (*highElement)[dayindex]->skip(1);
                (*lowElement)[dayindex]->skip(1);
                (*closeElement)[dayindex]->skip(1);
            }
            curPic += lineLen * column;
        }

        for(int dayindex = 0; dayindex < elementSize; ++dayindex){
            (*openElement)[dayindex]->skip(0, false);
            (*highElement)[dayindex]->skip(0, false);
            (*lowElement)[dayindex]->skip(0, false);
            (*closeElement)[dayindex]->skip(0, false);
        }
    }

    void getTrendPic(IVector<IBaseIterator<float>*>* valueElement, float* outPic, int column, float maxStdScale){
        int elementSize = valueElement->getSize();
        int lineLen = elementSize * 3;
        memset(outPic, 0, lineLen * column * (*valueElement)[0]->size() * sizeof(float));
        float* curPic = outPic;
        int lastIndex = -1;
        while ((*valueElement)[0]->isValid()){
            for(int dayindex = 0; dayindex < elementSize; ++dayindex){
                int valueIndex = data2index((*valueElement)[dayindex]->getValue(), column, maxStdScale);
                curPic[valueIndex * lineLen + dayindex * 3 + 1] = 1.f;
                if(dayindex == 0){
                    curPic[valueIndex * lineLen + dayindex * 3] = 1.f;
                } else {
                    int left = ((valueIndex + lastIndex) >> 1);
                    int minId = (left < valueIndex ? left : valueIndex);
                    int maxId = (left < valueIndex ? valueIndex : left);
                    for(left = minId; left <= maxId; ++left)
                        curPic[left * lineLen + dayindex * 3] = 1.f;

                    minId = (left < lastIndex ? left : lastIndex);
                    maxId = (left < lastIndex ? lastIndex : left);
                    for(left = minId; left <= maxId; ++left)
                        curPic[left * lineLen + (dayindex - 1) * 3 + 2] = 1.f;
                }

                if(dayindex == elementSize - 1) {
                    curPic[valueIndex * lineLen + dayindex * 3 + 2] = 1.f;
                }

                lastIndex = valueIndex;
                (*valueElement)[dayindex]->skip(1);
            }
            curPic += lineLen * column;
        }

        for(int dayindex = 0; dayindex < elementSize; ++dayindex){
            (*valueElement)[dayindex]->skip(0, false);
        }
    }
protected:
    int data2index(float data, int column, float maxStdScale){
        //cout<<data<<" "<<meanValue<<" "<<stdValue * maxStdScale<<endl;
        return (int)((std::min(std::max((data - meanValue) / (stdValue * maxStdScale), -1.0f), 1.0f) + 1.0f) * 0.4999f * column);
    }
    float meanValue;
    float stdValue;
};

#endif //ALPHATREE_ALPHAPIC_H
