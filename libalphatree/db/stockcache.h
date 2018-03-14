//
// Created by 严宇 on 2018/2/10.
//

#ifndef ALPHATREE_STOCKCACHE_H
#define ALPHATREE_STOCKCACHE_H

#include "../base/hashmap.h"
#include "stockfeature.h"
#include "stocksign.h"
#include <map>


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

    void cacheSign(const char* signName){
        sign_[signName] = new StockSign(feature2path_(signName).c_str(), des_->stockMetas[des_->mainStock].days, true);
    }

    ofstream* createCacheFile(const char* featureName){
        ofstream* file = new ofstream();
        file->open(feature2path_(featureName).c_str(), ios::binary);
        return file;
    }

    void releaseCacheFile(ofstream * file){
        file->close();
    }

    size_t getSignNum(size_t dayBefore, size_t daySize, const char* signName){
        size_t allDays = des_->stockMetas[des_->mainStock].days;
        StockSign* ss = nullptr;
        auto ** pSignHashNameNode = sign_.find(signName);
        if(*pSignHashNameNode == nullptr){
            ss = new StockSign(feature2path_(signName).c_str(), allDays, false);
        } else {
            ss = (*pSignHashNameNode)->value;
        }
        auto* iter = ss->createIter(dayBefore, daySize);
        size_t signNum = iter->size();
        delete iter;
        if(!ss->isCache())
            delete ss;
        return signNum;
    }

    //注意，信号填写不考虑补缺失数据，因为数据如果有缺损就不应该发出信号！
    void fill(float* dst, size_t dayBefore, size_t daySize, int historyDays, int futureDays, size_t startIndex, size_t signNum, const char* signName, const char* featureName){
        fill_(dayBefore, daySize, historyDays, futureDays, startIndex, signNum, signName, featureName,[dst](int index, float value){
            dst[index] = value;
        });
    }

    void fill(float* dst, size_t dayBefore, size_t daySize, const char* code, const char* featureName, size_t offset, size_t stockNum){
        fill_(dayBefore, daySize, code, featureName, offset, stockNum,[dst](int index, float value){
            dst[index] = value;
        });
    }

//    void fill(Iterator<float>& target,const float* sign, size_t dayBefore, size_t daySize, const char* code, const char* featureName, size_t offset, size_t stockNum){
//        Iterator<float>* ptarget = &target;
//        fill_(dayBefore, daySize, code, featureName, offset, stockNum, [ptarget, sign](int index, float value){
//           if(sign[index] > 0){
//               *(*ptarget) = value;
//               ++(*ptarget);
//           }
//        });
//    }


    template<class T>
    void invFill2File(const float* cache, size_t dayBefore, size_t daySize, const char* code, const char* featureName, size_t offset, size_t stockNum, ofstream* file,
                      bool isWritePreData = false, bool isWriteLastData = false){
        StockFeature<long> *date = date_ ? date_ : new StockFeature<long>(feature2path_("date").c_str());

        auto mainStockMeta = des_->stockMetas[des_->mainStock];
        Iterator<long> mainDateIter(date->createIter(mainStockMeta.offset, mainStockMeta.days));
        auto curStockMeta = des_->stockMetas[code];
        Iterator<long> curDateIter(date->createIter(curStockMeta.offset, curStockMeta.days));


        //主元素先跳到子元素位置上
        long skip = mainDateIter.jumpTo(*curDateIter);
        ++skip;
        if(skip > 0)
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
            for(int i = 0; i < subSkip; ++i) {
                T value = (T) cache[startIndex * stockNum + offset];
                (*file).write(reinterpret_cast<const char * >( &value ), sizeof(T));
            }
                //(*file)<<cache[startIndex * stockNum + offset];
        }else{
            (*file).seekp((des_->stockMetas[code].offset + subSkip) * sizeof(T), ios::beg);
        }

        size_t curIndex = 0;
        for(size_t i = startIndex; i < daySize; ++i){
            if(*curDateIter == *mainDateIter){
                curIndex = i;
                //(*file) << cache[i * stockNum + offset];
                T value = (T)cache[curIndex * stockNum + offset];
                (*file).write( reinterpret_cast<const char* >( &value ), sizeof( T ) );

               // if(strcmp(code,"0603156") == 0)
               //     cout<<":"<<cache[i * stockNum + offset]<<endl;
                ++curDateIter;
            }
            ++mainDateIter;
        }

        if(isWriteLastData){
            while (curDateIter.isValid()){
                (*file).write( reinterpret_cast<const char* >( &cache[curIndex * stockNum + offset] ), sizeof( T ) );
                ++curDateIter;
            }
        }
    }

    void invFill2Sign(const float* cache, size_t daySize, size_t stockNum, ofstream* file, size_t allDayNum, size_t& preDayNum, size_t& preSignCnt){
        //cout<<"ssss1\n";
        StockFeature<long> *date = date_ ? date_ : new StockFeature<long>(feature2path_("date").c_str());
        auto mainStockMeta = des_->stockMetas[des_->mainStock];
        Iterator<long> mainDateIter(date->createIter(mainStockMeta.offset, mainStockMeta.days));
        Iterator<long> allDateIter(date->createIter(0, des_->stockMetas[des_->stockMetas.getSize()-1].offset + des_->stockMetas[des_->stockMetas.getSize()-1].days));

        size_t lastId = 0;
        //包括今天在内，之前有多少信号
        size_t *pPreSignCnt = &preSignCnt;
        size_t *pPreDayNum = &preDayNum;
        size_t *pLastId = &lastId;
        Iterator<long>* pMainDateIter = &mainDateIter;
        Iterator<long>* pAllDateIter = &allDateIter;
        pMainDateIter->skip(preDayNum, false);

        file->seekp((allDayNum + (*pPreSignCnt)) * sizeof(size_t), ios::beg);
        StockDes* des = des_;

        //提高程序运行效率
        map<int,int> stock2offset;//股票上次发出信号的偏移+1
        map<int,long> stock2LastDate;//股票上次发出信号的日期
        for(int i=0; i < stockNum; ++i){
            stock2offset[i] = 0;
            stock2LastDate[i] = 0;
        }
        auto pStock2Offset = &stock2offset;
        auto pStock2LastDate = &stock2LastDate;

        //test---------------------------
//        StockFeature<float>* returns = new StockFeature<float>(feature2path_("returns").c_str());
//        IBaseIterator<float>* rt = returns->createIter(0, des->stockMetas[des->stockMetas.getSize()-1].offset + des->stockMetas[des->stockMetas.getSize()-1].days);

        invFillSign_(cache, daySize, stockNum, [pStock2Offset, pStock2LastDate, des, pMainDateIter, pAllDateIter, allDayNum, pPreSignCnt, pPreDayNum, pLastId, file](size_t dayIndex, size_t stockIndex){
            if((*pLastId) != dayIndex){
                file->seekp((*pPreDayNum) * sizeof(size_t), ios::beg);
                while ((*pLastId) != dayIndex){
                    //更新昨天以前的
                    file->write(reinterpret_cast<const char* >( pPreSignCnt ), sizeof(size_t));
                    ++(*pLastId);
                    ++(*pPreDayNum);
                }
                file->seekp((allDayNum + (*pPreSignCnt)) * sizeof(size_t), ios::beg);
                pMainDateIter->skip(*pPreDayNum, false);
            }
            //得到当前日期
            long date = *(*pMainDateIter);
            //得到偏移
            int evalStart = des->stockMetas[stockIndex].offset + (*pStock2Offset)[stockIndex];
            long lastDate = (*pStock2LastDate)[stockIndex];
            int evalLen = (int)min((long)(des->stockMetas[stockIndex].days - (*pStock2Offset)[stockIndex]), (long)(date - lastDate));
            int skip = pAllDateIter->jumpTo(date, evalStart, evalLen);
            ++skip;
            if(skip > 0)
                ++(*pAllDateIter);
            if(*(*pAllDateIter) == date){
                size_t subOffset = des->stockMetas[stockIndex].offset + skip + (*pStock2Offset)[stockIndex];

                //test
//                rt->skip(subOffset - 1, false);
//                if(**rt < 0)
//                    cout<<**rt<<endl;
//                rt->skip(1);
//                if(**rt < 0)
//                    cout<<**rt<<endl;

                //    cout<<stockIndex<<" "<<des->stockMetas[stockIndex].code<<" "<<date<<" "<<**rt<<endl;
                file->write(reinterpret_cast<const char* >( &subOffset ), sizeof(size_t));
                ++(*pPreSignCnt);
                (*pStock2Offset)[stockIndex] = (*pStock2Offset)[stockIndex] + skip + 1;
                //cout<<(*pStock2Offset)[stockIndex]<<endl;
                (*pStock2LastDate)[stockIndex] = date;
            } else {
                cout<<stockIndex<<" "<<date<<" "<<"我擦，居然没找到!\n";
            }

        });
        //cout<<"ssss3\n";
        file->seekp((*pPreDayNum) * sizeof(size_t), ios::beg);

        while ((*pLastId) != daySize-1){
            //更新昨天以前的
            file->write(reinterpret_cast<const char* >( pPreSignCnt ), sizeof(size_t));
            ++(*pLastId);
            ++(*pPreDayNum);
        }
        //最后一个元素再加一次(因为之前都是到了下一天才更新第一天的，最后一个元素没有下一天了)，以便下次调用是对的，这个bug查了两小时~
        file->write(reinterpret_cast<const char* >( pPreSignCnt ), sizeof(size_t));
        ++(*pPreDayNum);
        //cout<<"ssss4\n";
    }

    void testInvFill2Sign(const float* cache, const float* testCache, size_t daySize, size_t stockNum, ofstream* file, size_t allDayNum, size_t& preDayNum, size_t& preSignCnt){
        //cout<<"ssss1\n";
        StockFeature<long> *date = date_ ? date_ : new StockFeature<long>(feature2path_("date").c_str());
        auto mainStockMeta = des_->stockMetas[des_->mainStock];
        Iterator<long> mainDateIter(date->createIter(mainStockMeta.offset, mainStockMeta.days));
        Iterator<long> allDateIter(date->createIter(0, des_->stockMetas[des_->stockMetas.getSize()-1].offset + des_->stockMetas[des_->stockMetas.getSize()-1].days));

        size_t lastId = 0;
        //包括今天在内，之前有多少信号
        size_t *pPreSignCnt = &preSignCnt;
        size_t *pPreDayNum = &preDayNum;
        size_t *pLastId = &lastId;
        Iterator<long>* pMainDateIter = &mainDateIter;
        Iterator<long>* pAllDateIter = &allDateIter;
        pMainDateIter->skip(preDayNum, false);

        file->seekp((allDayNum + (*pPreSignCnt)) * sizeof(size_t), ios::beg);
        StockDes* des = des_;

        //提高程序运行效率
        map<int,int> stock2offset;//股票上次发出信号的偏移+1
        map<int,long> stock2LastDate;//股票上次发出信号的日期
        for(int i=0; i < stockNum; ++i){
            stock2offset[i] = 0;
            stock2LastDate[i] = 0;
        }
        auto pStock2Offset = &stock2offset;
        auto pStock2LastDate = &stock2LastDate;

        //test---------------------------
        StockFeature<float>* returns = new StockFeature<float>(feature2path_("returns").c_str());
        IBaseIterator<float>* rt = returns->createIter(0, des->stockMetas[des->stockMetas.getSize()-1].offset + des->stockMetas[des->stockMetas.getSize()-1].days);

        invFillSign_(cache, daySize, stockNum, [rt, stockNum, testCache, pStock2Offset, pStock2LastDate, des, pMainDateIter, pAllDateIter, allDayNum, pPreSignCnt, pPreDayNum, pLastId, file](size_t dayIndex, size_t stockIndex){
            if((*pLastId) != dayIndex){
                file->seekp((*pPreDayNum) * sizeof(size_t), ios::beg);
                while ((*pLastId) != dayIndex){
                    //更新昨天以前的
                    file->write(reinterpret_cast<const char* >( pPreSignCnt ), sizeof(size_t));
                    ++(*pLastId);
                    ++(*pPreDayNum);
                }
                file->seekp((allDayNum + (*pPreSignCnt)) * sizeof(size_t), ios::beg);
                pMainDateIter->skip(*pPreDayNum, false);
            }
            //得到当前日期
            long date = *(*pMainDateIter);
            //得到偏移
            int evalStart = des->stockMetas[stockIndex].offset + (*pStock2Offset)[stockIndex];
            long lastDate = (*pStock2LastDate)[stockIndex];
            int evalLen = (int)min((long)(des->stockMetas[stockIndex].days - (*pStock2Offset)[stockIndex]), (long)(date - lastDate));
            int skip = pAllDateIter->jumpTo(date, evalStart, evalLen);
            ++skip;
            if(skip > 0)
                ++(*pAllDateIter);
            if(*(*pAllDateIter) == date){
                size_t subOffset = des->stockMetas[stockIndex].offset + skip + (*pStock2Offset)[stockIndex];

                //test
                if(dayIndex > 0 && testCache[(dayIndex-1) * stockNum + stockIndex] < 0)
                    cout<<testCache[(dayIndex - 1) * stockNum + stockIndex]<<endl;
                if(testCache[dayIndex * stockNum + stockIndex] < 0)
                    cout<<testCache[dayIndex * stockNum + stockIndex]<<endl;
                rt->skip(subOffset - 1, false);
                if(dayIndex > 0 && **rt < 0)
                    cout<<**rt<<endl;
                rt->skip(1);
                if(**rt < 0)
                    cout<<**rt<<endl;

                //    cout<<stockIndex<<" "<<des->stockMetas[stockIndex].code<<" "<<date<<" "<<**rt<<endl;
                file->write(reinterpret_cast<const char* >( &subOffset ), sizeof(size_t));
                ++(*pPreSignCnt);
                (*pStock2Offset)[stockIndex] = (*pStock2Offset)[stockIndex] + skip + 1;
                //cout<<(*pStock2Offset)[stockIndex]<<endl;
                (*pStock2LastDate)[stockIndex] = date;
            } else {
                cout<<stockIndex<<" "<<date<<" "<<"我擦，居然没找到!\n";
            }

        });
        //cout<<"ssss3\n";
        file->seekp((*pPreDayNum) * sizeof(size_t), ios::beg);

        while ((*pLastId) != daySize-1){
            //更新昨天以前的
            file->write(reinterpret_cast<const char* >( pPreSignCnt ), sizeof(size_t));
            ++(*pLastId);
            ++(*pPreDayNum);
        }
        //最后一个元素再加一次(因为之前都是到了下一天才更新第一天的，最后一个元素没有下一天了)，以便下次调用是对的，这个bug查了两小时~
        file->write(reinterpret_cast<const char* >( pPreSignCnt ), sizeof(size_t));
        ++(*pPreDayNum);
        //cout<<"ssss4\n";
    }

    //todo delete later
//    IBaseIterator<float>* createSignFeatureIter(const char* signName, const char* featureName, size_t dayBefore, size_t sampleDays, int offset){
//        StockFeature<float> *ft = nullptr;
//        auto** pHashNameNode = feature_.find(featureName);
//        if(*pHashNameNode == nullptr){
//            ft = new StockFeature<float>(feature2path_(featureName).c_str());
//        } else {
//            ft = (*pHashNameNode)->value;
//        }
//        size_t allDays = des_->stockMetas[des_->mainStock].days;
//        IBaseIterator<size_t>* signIter = new BinarySignIterator(feature2path_(signName).c_str(), dayBefore, sampleDays, des_->stockMetas[des_->mainStock].days, offset);
//        //cout<<des_->stockMetas[des_->stockMetas.getSize()-1].offset + des_->stockMetas[des_->stockMetas.getSize()-1].days<<endl;
//        IBaseIterator<float>* featureIter = ft->createIter(0, des_->stockMetas[des_->stockMetas.getSize()-1].offset + des_->stockMetas[des_->stockMetas.getSize()-1].days);
//        return new Sign2FeatureIterator(signIter, featureIter);
//    }

protected:
    const char* path_;
    StockDes* des_;
    StockFeature<long>* date_ = nullptr;
    HashMap<StockFeature<float>*> feature_;
    HashMap<StockSign*> sign_;

    //在dayBefore前的daySize天取样信号，向前取信号发生时特征(featureName)的historyDays条数据,向后取-futureDays的数据（不考虑数据缺失！）
    template <class F>
    void fill_(size_t dayBefore, size_t daySize, int historyDays, int futureDays, size_t startIndex, size_t signNum, const char* signName, const char* featureName, F&& f){
        StockFeature<float> *ft = nullptr;
        auto** pHashNameNode = feature_.find(featureName);
        if(*pHashNameNode == nullptr){
            ft = new StockFeature<float>(feature2path_(featureName).c_str());
        } else {
            ft = (*pHashNameNode)->value;
        }

        size_t allDays = des_->stockMetas[des_->mainStock].days;
        StockSign* ss = nullptr;
        auto ** pSignHashNameNode = sign_.find(signName);
        if(*pSignHashNameNode == nullptr){
            ss = new StockSign(feature2path_(signName).c_str(), allDays, false);
        } else {
            ss = (*pSignHashNameNode)->value;
        }

        Iterator<float> curFeatureIter(ft->createIter(0, des_->stockMetas[des_->stockMetas.getSize()-1].offset + des_->stockMetas[des_->stockMetas.getSize()-1].days));
        Iterator<size_t> curSignIter(ss->createIter(dayBefore, daySize));
        size_t allSize = curSignIter.size();
        if(allSize < startIndex + signNum)
            throw "信号不够";
        int curIndex = 0;
        curSignIter.skip(startIndex);
        while (curSignIter.isValid() && curIndex < signNum){
            size_t offset = *curSignIter;
            curFeatureIter.skip(offset - (historyDays - 1), false);
            ++curSignIter;
            for(size_t i = 0; i < historyDays - futureDays; ++i){
//                if(*curFeatureIter <= 0)
//                    cout<<*curFeatureIter<<endl;
                //cout<<i * signNum + curIndex<<" "<<*curFeatureIter<<endl;
                f(i * signNum + curIndex, *curFeatureIter);
                ++curFeatureIter;
            }
            ++curIndex;
        }
        if(!ft->isCache())
            delete ft;
        if(!ss->isCache())
            delete ss;
    }

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

        //if(dayBefore == 4515 && strcmp(code, "0600867") == 0){
        //    cout<<":"<<*mainDateIter<<endl;
        //}

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
            //if(dayBefore == 4515 && strcmp(code, "0600867") == 0 && i == 1){
            //    cout<<":"<<*mainDateIter<<endl;
            //}
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

    template <class F>
    void invFillSign_(const float* cache, size_t daySize, size_t stockNum, F&& f){
        for(size_t i = 0; i < daySize; ++i){
            for(size_t j = 0; j < stockNum; ++j){
                if(cache[i * stockNum + j] > 0){
                    //cout<<cache[i * stockNum + j]<<endl;
                    f(i, j);
                }
            }
        }
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

        //T lastValue = 0;

        for(int i = 0; i < des->stockMetas.getSize(); ++i){
            auto meta = des->stockMetas[i];
            CSVIterator<T> iter(filepath(meta.code,".csv").c_str(), featureName);

            //file.seekp(meta.offset * sizeof(T), ios::beg);

            int dayNum = 0;
            while (iter.isValid()){
                //file<<*iter;
                T value = *iter;
                //if(value < lastValue)
                //    cout<<"error\n";
                //lastValue = value;
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
