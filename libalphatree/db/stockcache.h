//
// Created by 严宇 on 2018/2/10.
//

#ifndef ALPHATREE_STOCKCACHE_H
#define ALPHATREE_STOCKCACHE_H

#include "../base/hashmap.h"
#include "stockfeature.h"


class StockCache{
public:
    StockCache(const char* path, StockDes* des):path_(path), des_(des){

    }

    virtual ~StockCache(){
        if(date_)
            delete date_;
        for(int i = 0 ;i < feature_.getSize(); ++i){
            delete feature_[i];
        }
    }

    void cacheFeature(const char* featureName){
        if(strcmp(featureName,"date") == 0){
            date_ = new StockFeature<long>(feature2path_(featureName).c_str(), des_->offset);
        } else {
            feature_[featureName] = new StockFeature<float>(feature2path_(featureName).c_str(), des_->offset);
        }
    }

    ofstream* createCacheFile(const char* featureName){
        ofstream* file = new ofstream();
        file->open(feature2path_(featureName).c_str(), ios::binary);
        return file;
    }

    void releaseCacheFile(ofstream * file){
        file->close();
    }

    void fill(float* dst, size_t dayBefore, size_t daySize, const char* code, const char* featureName, size_t offset, size_t stockNum){
        fill_(dayBefore, daySize, code, featureName, offset, stockNum,[dst](int index, float value){
            dst[index] = value;
        });
    }

    void fill(Iterator<float>& target,const float* sign, size_t dayBefore, size_t daySize, const char* code, const char* featureName, size_t offset, size_t stockNum){
        Iterator<float>* ptarget = &target;
        fill_(dayBefore, daySize, code, featureName, offset, stockNum, [ptarget, sign](int index, float value){
           if(sign[index] > 0){
               *(*ptarget) = value;
               ++(*ptarget);
           }
        });
    }

    void invFill2File(const float* cache, size_t dayBefore, size_t daySize, const char* code, const char* featureName, size_t offset, size_t stockNum, ofstream* file,
                      bool isWritePreData = false){
        StockFeature<long> *date = date_ ? date_ : new StockFeature<long>(feature2path_("date").c_str());

        auto mainStockMeta = des_->stockMetas[des_->mainStock];
        Iterator<long> mainDateIter(date->createIter(mainStockMeta.offset, mainStockMeta.days));
        auto curStockMeta = des_->stockMetas[code];
        Iterator<long> curDateIter(date->createIter(curStockMeta.offset, curStockMeta.days));


        //主元素先跳到子元素位置上
        int skip = mainDateIter.jumpTo(*curDateIter);
        ++skip;
        ++mainDateIter;
        //跳到的位置超过了当前要写入数据的范围就返回
        if(mainDateIter.size() - dayBefore - 1 < skip)
            return;

        //如果当前需要写入的第一个元素id还在现在的位置后面，继续向前跳
        if(mainDateIter.size() - (dayBefore + daySize) > skip){
            mainDateIter.skip(mainDateIter.size() - dayBefore - daySize - skip);
            skip = mainDateIter.size() - dayBefore - daySize;
        }

        int subSkip = curDateIter.jumpTo(*mainDateIter);
        //当前需要写入数据的位置超过了子元素的最后位置
        if(subSkip == curDateIter.size() - 1)
            return;
        if(subSkip < 0){
            if(*curDateIter != *mainDateIter)
                throw "不可能！";
            ++subSkip;
        } else {
            ++subSkip;
            ++curDateIter;
        }
        //对其数据curDateIter>=mainDateIter
        if(*curDateIter < *mainDateIter){
            cout<<code<<" "<<*curDateIter<<" "<<*mainDateIter<<endl;
            throw "不可能！";
        }


        //开始写入数据
        size_t startIndex = skip - (mainDateIter.size() - dayBefore - daySize);
        if(isWritePreData){
            (*file).seekp((des_->stockMetas[code].offset) * sizeof(float), ios::beg);
            for(int i = 0; i < subSkip; ++i)
                (*file).write( reinterpret_cast<const char* >( &cache[startIndex * stockNum + offset] ), sizeof( float ) );
                //(*file)<<cache[startIndex * stockNum + offset];
        }else{
            (*file).seekp((des_->stockMetas[code].offset + subSkip) * sizeof(float), ios::beg);
        }

        for(size_t i = startIndex; i < daySize; ++i){
            if(*curDateIter == *mainDateIter){
                //(*file) << cache[i * stockNum + offset];
                (*file).write( reinterpret_cast<const char* >( &cache[i * stockNum + offset] ), sizeof( float ) );
                if(strcmp(code,"0603156") == 0)
                    cout<<":"<<cache[i * stockNum + offset]<<endl;
                ++curDateIter;
            }
            ++mainDateIter;
        }
    }
protected:
    const char* path_;
    StockDes* des_;
    StockFeature<long>* date_ = nullptr;
    HashMap<StockFeature<float>*> feature_;

    template <class F>
    void fill_(size_t dayBefore, size_t daySize, const char* code, const char* featureName, size_t offset, size_t stockNum, F&& f){
        StockFeature<long> *date = date_ ? date_ : new StockFeature<long>(feature2path_("date").c_str());
        StockFeature<float> *ft = nullptr;
        auto** pHashNameNode = feature_.find(featureName);
        if(*pHashNameNode == nullptr){
            ft = new StockFeature<float>(feature2path_(featureName).c_str());
        } else {
            ft = (*pHashNameNode)->value;
        }


        auto mainStockMeta = des_->stockMetas[des_->mainStock];
        Iterator<long> mainDateIter(date->createIter(mainStockMeta.offset, mainStockMeta.days));
        auto curStockMeta = des_->stockMetas[code];
        Iterator<long> curDateIter(date->createIter(curStockMeta.offset, curStockMeta.days));
        Iterator<float> curFeatureIter(ft->createIter(curStockMeta.offset, curStockMeta.days));

        /*if(strcmp(code, "0603156") == 0){
            while (curFeatureIter.isValid() && curDateIter.isValid())
            {
                cout<<*curDateIter<<" "<<*curFeatureIter<<endl;
                ++curFeatureIter;
                ++curDateIter;
            }
        }*/

        //定位要读取的数据
        auto skipOffset = mainDateIter.size() - dayBefore - daySize;
        mainDateIter.skip(skipOffset);

        int skip = curDateIter.jumpTo(*mainDateIter);
        float lastFeature = *curFeatureIter;
        if(skip >= 0){
            curFeatureIter.skip(skip);
            lastFeature = *curFeatureIter;
            ++curDateIter;
            ++curFeatureIter;
        }



        //填写要读取的数据，如有缺失就补上
        for(size_t i = 0; i < daySize; ++i){

            if(curDateIter.isValid()){
                if(*curDateIter < *mainDateIter){
                    cout<<"主要的股票有的数据丢失，这是不允许的！";
                    throw "main stock loss date!";
                } else {
                    if(*curDateIter == *mainDateIter){
                        //日期正好对上
                        lastFeature = (*curFeatureIter);
                        ++curFeatureIter;
                        ++curDateIter;
                    }
                }
            }
            f(offset + i * stockNum, lastFeature);
            //if(strcmp(code,"0603156") == 0)
            //    cout<<*curDateIter<<" "<<*mainDateIter<<" "<<lastFeature<<endl;
            //dst[offset + i * stockNum] = lastFeature;
            ++mainDateIter;
        }

        if(!date->isCache())
            delete date;
        if(ft && !ft->isCache())
            delete ft;
    }

    string feature2path_(const char* featureName){
        string path = string(path_);
        path += "/";
        path += featureName;
        path += ".ft";
        return path;
    }
public:
    template <class T>
    static void csv2feature(const char* path, const char* featureName, StockDes* des){
        ofstream file;
        auto filepath = [path] (string file, string lastName) {
            string filePath = path;
            filePath += "/";
            filePath += file;
            filePath += lastName;
            return filePath;
        };
        file.open(filepath(featureName, ".ft"), ios::binary | ios::out);
        for(int i = 0; i < des->stockMetas.getSize(); ++i){
            auto meta = des->stockMetas[i];
            CSVIterator<T> iter(filepath(meta.code,".csv").c_str(), featureName);

            //file.seekp(meta.offset * sizeof(T), ios::beg);

            int dayNum = 0;
            while (iter.isValid()){
                //file<<*iter;
                T value = *iter;
                file.write( reinterpret_cast< char* >( &value ), sizeof( T ) );
                ++dayNum;
                ++iter;
            }
            if(dayNum != meta.days){
                throw "day num error!";
            }
        }
        file.close();
    }

    template <class T>
    static void testfeature(const char* path, const char* featureName, StockDes* des){

        auto filepath = [path] (string file, string lastName) {
            string filePath = path;
            filePath += "/";
            filePath += file;
            filePath += lastName;
            return filePath;
        };
        auto feature =  new StockFeature<T>(filepath(featureName,".ft").c_str(), des->offset);
        Iterator<T> itertest(feature->createIter(0, des->offset));
        for(int i = 0; i < des->stockMetas.getSize(); ++i){
            auto meta = des->stockMetas[i];
            CSVIterator<T> iter(filepath(meta.code,".csv").c_str(), featureName);

            //file.seekp(meta.offset * sizeof(T), ios::beg);

            int dayNum = 0;
            while (iter.isValid()){
                //file<<*iter;
                T value = *iter;
                if((*itertest) != value)
                    cout<<(*itertest)<<" "<<value<<"\n";
                //file.write( reinterpret_cast< char* >( &value ), sizeof( T ) );
                ++dayNum;
                ++iter;
                ++itertest;
            }
            if(dayNum != meta.days){
                throw "day num error!";
            }
        }
    }
};

#endif //ALPHATREE_STOCKCACHE_H
