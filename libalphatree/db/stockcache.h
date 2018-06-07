//
// Created by 严宇 on 2018/2/10.
//

#ifndef ALPHATREE_STOCKCACHE_H
#define ALPHATREE_STOCKCACHE_H

#include "../base/hashmap.h"
#include "stockfeature.h"
#include "stocksign.h"
#include "../hmm/eventhmm.h"
#include <map>
#include <algorithm>

class StockCache{
public:
    StockCache(const char* path, StockDes* des): des_(des){
        path_ = new char[strlen(path) + 1];
        strcpy(path_, path);
    }

    virtual ~StockCache(){
        releaseFeatures();
        releaseSigns();
        delete []path_;
    }

    virtual void releaseFeatures(){
        if(date_)
            delete date_;
        date_ = nullptr;
        if(miss_)
            delete miss_;
        miss_ = nullptr;
        for(int i = 0 ;i < feature_.getSize(); ++i){
            delete feature_[i];
        }
        feature_.clear();
    }

    virtual void releaseSigns(){
        for(int i = 0; i < feature_.getSize(); ++i){
            delete sign_[i];
        }
        sign_.clear();
    }

    void loadFeature(const char* featureName){
        if(strcmp(featureName,"date") == 0){
            date_ = new StockFeature<long>(feature2path_(featureName).c_str(), des_->offset);
        } else if(strcmp(featureName, "miss") == 0){
            miss_ = new StockFeature<int>(feature2path_(featureName).c_str(), des_->offset);
        }else{
            feature_[featureName] = new StockFeature<float>(feature2path_(featureName).c_str(), des_->offset);
        }
    }

    void updateFeature(const char* featureName){
        if(strcmp(featureName,"date") == 0){
            delete date_;
            date_ = new StockFeature<long>(feature2path_(featureName).c_str(), des_->offset);
        } else if(strcmp(featureName, "miss") == 0){
            delete miss_;
            miss_ = new StockFeature<int>(feature2path_(featureName).c_str(), des_->offset);
        }else{
            delete feature_[featureName];
            feature_[featureName] = new StockFeature<float>(feature2path_(featureName).c_str(), des_->offset);
        }
    }

    void loadSign(const char* signName){
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

    size_t getStockIds(size_t dayBefore, size_t sampleSize, const char* signName, int* dst){
        size_t allDays = des_->stockMetas[des_->mainStock].days;
        StockSign* ss = nullptr;
        auto ** pSignHashNameNode = sign_.find(signName);
        if(*pSignHashNameNode == nullptr){
            ss = new StockSign(feature2path_(signName).c_str(), allDays, false);
        } else {
            ss = (*pSignHashNameNode)->value;
        }
        auto* iter = ss->createIter(dayBefore, sampleSize);
        size_t cnt = 0;
        while (iter->isValid()){
            dst[cnt++] = des_->offset2index(**iter);
            iter->skip(1);
        }
        delete iter;
        return cnt;
    }

    size_t getSignNum(size_t dayBefore, size_t sampleSize, const char* signName){
        size_t allDays = des_->stockMetas[des_->mainStock].days;
        StockSign* ss = nullptr;
        auto ** pSignHashNameNode = sign_.find(signName);
        if(*pSignHashNameNode == nullptr){
            ss = new StockSign(feature2path_(signName).c_str(), allDays, false);
        } else {
            ss = (*pSignHashNameNode)->value;
        }
        auto* iter = ss->createIter(dayBefore, sampleSize);
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
        if(mainDateIter.size() - (long)dayBefore - 1 < skip)
            return;

        //如果当前需要写入的第一个元素id还在现在的位置后面，继续向前跳
        if(mainDateIter.size() - (long)(dayBefore + daySize) > skip){
            mainDateIter.skip(mainDateIter.size() - dayBefore - daySize - skip);
            skip = mainDateIter.size() - dayBefore - daySize;
        }

        int subSkip = curDateIter.jumpTo(*mainDateIter);
        //当前需要写入数据的位置超过了子元素的最后位置
        if(subSkip == curDateIter.size() - 1)
            return;
        if(subSkip < 0){
            if(*curDateIter != *mainDateIter){
                cout<<"缓存错误\n";
                throw "不可能！";
            }

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


    void invFill2Sign(const float* cache, size_t daySize, size_t stockNum, ofstream* file, size_t allDayNum, size_t& preDayNum, size_t& preSignCnt, const bool* stockFlag = nullptr){
        //cout<<"ssss1\n";
        StockFeature<long> *date = date_ ? date_ : new StockFeature<long>(feature2path_("date").c_str());
        auto mainStockMeta = des_->stockMetas[des_->mainStock];
        Iterator<long> mainDateIter(date->createIter(mainStockMeta.offset, mainStockMeta.days));
        Iterator<long> allDateIter(date->createIter(0, des_->offset));

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
        for(size_t i=0; i < stockNum; ++i){
            stock2offset[i] = 0;
            stock2LastDate[i] = 0;
        }
        auto pStock2Offset = &stock2offset;
        auto pStock2LastDate = &stock2LastDate;

        //test---------------------------
//        StockFeature<float>* returns = new StockFeature<float>(feature2path_("returns").c_str());
//        IBaseIterator<float>* rt = returns->createIter(0, des->stockMetas[des->stockMetas.getSize()-1].offset + des->stockMetas[des->stockMetas.getSize()-1].days);

        invFillSign_(cache, daySize, stockNum, stockFlag, [pStock2Offset, pStock2LastDate, des, pMainDateIter, pAllDateIter, allDayNum, pPreSignCnt, pPreDayNum, pLastId, file](size_t dayIndex, size_t stockIndex){
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
            int evalLen = (int)std::min((long)(des->stockMetas[stockIndex].days - (*pStock2Offset)[stockIndex]), (long)(date - lastDate));
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

                cout<<stockIndex<<" "<<des->stockMetas[stockIndex].code<<" "<<date<<" "<<"我擦，居然没找到!\n";
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
        Iterator<long> allDateIter(date->createIter(0, des_->offset));

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
        for(size_t i=0; i < stockNum; ++i){
            stock2offset[i] = 0;
            stock2LastDate[i] = 0;
        }
        auto pStock2Offset = &stock2offset;
        auto pStock2LastDate = &stock2LastDate;

        //test---------------------------
        StockFeature<float>* returns = new StockFeature<float>(feature2path_("returns").c_str());
        IBaseIterator<float>* rt = returns->createIter(0, des->stockMetas[des->stockMetas.getSize()-1].offset + des->stockMetas[des->stockMetas.getSize()-1].days);

        invFillSign_(cache, daySize, stockNum, nullptr, [rt, stockNum, testCache, pStock2Offset, pStock2LastDate, des, pMainDateIter, pAllDateIter, allDayNum, pPreSignCnt, pPreDayNum, pLastId, file](size_t dayIndex, size_t stockIndex){
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
            int evalLen = (int)std::min((long)(des->stockMetas[stockIndex].days - (*pStock2Offset)[stockIndex]), (long)(date - lastDate));
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
    char* path_;
    StockDes* des_;
    StockFeature<long>* date_ = nullptr;
    StockFeature<int>* miss_ = nullptr;
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

        //必须有缺失数据描述才可以读取信号特征
        StockFeature<int> *ms = miss_ == nullptr ? new StockFeature<int>(feature2path_("miss").c_str()) : miss_;

        size_t allDays = des_->stockMetas[des_->mainStock].days;
        StockSign* ss = nullptr;
        auto ** pSignHashNameNode = sign_.find(signName);
        if(*pSignHashNameNode == nullptr){
            ss = new StockSign(feature2path_(signName).c_str(), allDays, false);
        } else {
            ss = (*pSignHashNameNode)->value;
        }

        Iterator<float> curFeatureIter(ft->createIter(0, des_->offset));
        Iterator<int> curMissIter(ms->createIter(0, des_->offset));
        Iterator<size_t> curSignIter(ss->createIter(dayBefore, daySize));
        size_t allSize = curSignIter.size();
        if(allSize < startIndex + signNum){
            cout<<"信号不够\n";
            throw "信号不够";
        }

        size_t curIndex = 0;
        curSignIter.skip(startIndex);
        while (curSignIter.isValid() && curIndex < signNum){
            size_t offset = *curSignIter;
            curMissIter.skip(offset, false);
            curFeatureIter.skip(offset, false);

            int missFlag = *curMissIter;
            //在信号发生当天之前，还需要signBeforeday这么多天的数据
            int signBeforeday = historyDays - 1;
            while (signBeforeday > 0 && missFlag != 0){
                if(missFlag > 0){
                    if(missFlag >= signBeforeday){
                        offset -= signBeforeday;
                        signBeforeday = 0;
                    } else {
                        offset -= missFlag;
                        signBeforeday -= missFlag;
                    }

                } else{
                    if(1 - missFlag > signBeforeday){
                        break;
                    } else {
                        offset -= 1;
                        signBeforeday -= (1 - missFlag);
                    }
                }
                curMissIter.skip(offset, false);
                curFeatureIter.skip(offset, false);
                missFlag = *curMissIter;
            }

            //先填写历史数据,迭代器最后
            if(missFlag == 0){
                //没有历史数据，用第一条数据补上
                for(int i = 0; i <= signBeforeday; ++i){
                    f(i * signNum + curIndex, *curFeatureIter);
                }
            } else if(missFlag < 0){
                if(1 - missFlag <= signBeforeday){
                    cout<<"miss文件有问题\n";
                    throw "err";
                }
                curFeatureIter.skip(offset-1, false);
                for(int i = 0; i < signBeforeday; ++i){
                    f(i * signNum + curIndex, *curFeatureIter);
                }
                ++curFeatureIter;
                f(signBeforeday * signNum + curIndex, *curFeatureIter);
            } else {
                f(signBeforeday * signNum + curIndex, *curFeatureIter);
            }

            //往后填写剩下的数据
            int dayindex = signBeforeday + 1;
            while (dayindex < historyDays - futureDays){
                //定位当前的miss迭代器，方便得到当前缺失天数
                ++curMissIter;
                if(*curMissIter > 0){
                    ++curFeatureIter;
                } else {
                    int missDays = -*curMissIter;
                    for(int i = 0; i < missDays; ++i){
                        f((dayindex + i)*signNum + curIndex, *curFeatureIter);
                    }
                    dayindex += missDays;
                    ++curFeatureIter;
                }
                f(dayindex * signNum + curIndex, *curFeatureIter);
                ++dayindex;
            }

            ++curIndex;
            ++curSignIter;
        }
        if(!ft->isCache())
            delete ft;
        if(!ss->isCache())
            delete ss;
        if(!ms->isCache())
            delete ms;
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
                    cout<<code<<endl;
                    cout<<curStockMeta.offset<<" "<<curStockMeta.days<<endl;
                    cout<<*curDateIter<<" "<<*mainDateIter<<endl;
                    cout<<"main stock loss date!\n";
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
    void invFillSign_(const float* cache, size_t daySize, size_t stockNum, const bool* stockFlag, F&& f){
        for(size_t i = 0; i < daySize; ++i){
            for(size_t j = 0; j < stockNum; ++j){
                if(cache[i * stockNum + j] > 0 && (stockFlag == nullptr || stockFlag[j])){
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
    //将缺失数据描述写入文件，如果当前数据前n天的数据都是完整的，文件中特征就是n，如果缺失了m天，特征就是-m。如果是首个元素就是0
    void miss2file(){
        StockFeature<long> *date = date_ ? date_ : new StockFeature<long>(feature2path_("date").c_str());
        auto mainStockMeta = des_->stockMetas[des_->mainStock];
        Iterator<long> mainDateIter(date->createIter(mainStockMeta.offset, mainStockMeta.days));
        Iterator<long> allDateIter(date->createIter(0, des_->offset));
        ofstream file;
        file.open(feature2path_("miss"), ios::binary | ios::out);
        for(int i = 0; i < des_->stockMetas.getSize(); ++i){
            auto meta = des_->stockMetas[i];
            long start = meta.offset;
            long end = start + meta.days;
            int fullDataNum = 0;
            file.write(reinterpret_cast< char* >( &fullDataNum ), sizeof( int ));
            allDateIter.skip(start, false);
            long lastDate = *allDateIter;
            int mainOffset = mainDateIter.jumpTo(lastDate);
            ++mainOffset;
            if(mainOffset > 0)
                ++mainDateIter;
            if(*mainDateIter != lastDate){
                cout<<"保存miss文件错误\n";
                throw "err";
            }

            for(int j = 1; j < meta.days; ++j){
                ++allDateIter;
                ++mainDateIter;
                ++mainOffset;
                if(*mainDateIter == *allDateIter){
                    ++fullDataNum;
                    file.write(reinterpret_cast< char* >( &fullDataNum ), sizeof( int ));
                } else {
                    fullDataNum = 0;
                    int skip = mainDateIter.jumpTo(*allDateIter, mainOffset, mainDateIter.size() - mainOffset);
                    if(skip < 0){
                        cout<<"保存miss文件错误\n";
                        throw "err";
                    }
                    ++skip;
                    ++mainDateIter;
                    if(*mainDateIter != *allDateIter){
                        cout<<"不可能\n";
                        throw "err";
                    }
                    mainOffset += skip;
                    skip = -skip;
                    file.write(reinterpret_cast< char* >( &skip ), sizeof( int ));
                }
            }
        }
        file.close();
    }

    void boolhmm2binary(const char* featureName, int hideStateNum, size_t seqLength, bool* stockFlag, int epochNum = 8){
        StockFeature<float> *ft = nullptr;
        auto** pHashNameNode = feature_.find(featureName);
        if(*pHashNameNode == nullptr){
            ft = new StockFeature<float>(feature2path_(featureName).c_str());
        } else {
            ft = (*pHashNameNode)->value;
        }

        ofstream convertPercentFile;
        string hmmConvName = string(featureName) + "_hmm_conv";
        convertPercentFile.open(feature2path_(hmmConvName.c_str()), ios::binary | ios::out);

        ofstream* hidePercentFiles = new ofstream[hideStateNum];
        for(int i = 0; i < hideStateNum; ++i){
            string hmmHideName = string(featureName) + "_hmm_" + to_string(i);
            hidePercentFiles[i].open(feature2path_(hmmHideName.c_str()), ios::binary | ios::out);
        }

        EventHMM hmm(hideStateNum, 2, seqLength);

        float* o = new float[seqLength];
        float* curEvent = new float[hideStateNum];
        float* curEventArgSortIndex = new float[hideStateNum];

        int useBitSize = 64 / (hideStateNum * hideStateNum);//确定用多少bit去存转移概率
        int useBitValue = (int)powf(2.f, (float)useBitSize) - 1;

        for(int i = 0; i < des_->stockMetas.getSize(); ++i){
            if(stockFlag && stockFlag[i]){
                Iterator<float> curIter(ft->createIter(des_->stockMetas[i].offset, des_->stockMetas[i].days));
                long percentBatch = 0;
                float hidePercent = 0;
                int curDateIndex = 0;

                //写入先前空缺的数据
                while (curDateIndex < seqLength - 1 && curDateIndex < curIter.size()){
                    o[curDateIndex++] = (*curIter) > 0 ? 1.f : 0.f;
                    ++curIter;
                    convertPercentFile.write(reinterpret_cast< char* >( &percentBatch ), sizeof( long ));
                    for(int j = 0; j < hideStateNum; ++j){
                        hidePercentFiles[j].write(reinterpret_cast< char* >( &hidePercent ), sizeof( float ));
                    }
                }

                //计算hmm
                while (curDateIndex < curIter.size()){
                    o[seqLength-1] = (*curIter) > 0 ? 1.f : 0.f;
                    ++curIter;
                    ++curDateIndex;
                    DMatrix<float> out(seqLength, 1, o);
                    hmm.train(&out, epochNum);
                    //写入结果
                    for(int j = 0; j < hideStateNum; ++j){
                        //上涨概率
                        curEvent[j] = hmm.getB(j, 1);
                    }
                    //写入隐藏状态的概率
                    _ranksort(curEventArgSortIndex, curEvent, hideStateNum);

                    /*{
                        for(int j = 0; j < hideStateNum; ++j){
                            cout<<curEvent[j]<<" ";
                        }
                        cout<<endl;

                        for(int j = 0; j < hideStateNum; ++j){
                            cout<<curEventArgSortIndex[j]<<" ";
                        }
                        cout<<endl;
                    }*/

                    for(int j = 0; j < hideStateNum; ++j){
                        hidePercent = hmm.getGamma((int)curEventArgSortIndex[j], seqLength-1);
                        hidePercentFiles[j].write(reinterpret_cast< char* >( &hidePercent ), sizeof( float ));
                    }
                    //写入隐藏状态的转移概率
                    percentBatch = 0;
                    for(int j = 0; j < hideStateNum; ++j){
                        for(int k = 0; k < hideStateNum; ++k){
                            int v = hmm.getA((int)curEventArgSortIndex[j], (int)curEventArgSortIndex[k]) * useBitValue;
                            percentBatch += v;
                            percentBatch = (percentBatch << useBitSize);
                        }
                    }
                    convertPercentFile.write(reinterpret_cast< char* >( &percentBatch ), sizeof( long ));

                    for(int j = 0; j < seqLength - 1; ++j){
                        o[j] = o[j + 1];
                    }
                }
            } else {
                for(int j = 0; j < hideStateNum; ++j){
                    hidePercentFiles[j].seekp(des_->stockMetas[i].days * sizeof( float ), ios::cur);
                }
                convertPercentFile.seekp(des_->stockMetas[i].days * sizeof( long ), ios::cur);
            }

        }

        delete []o;
        delete []curEvent;
        delete []curEventArgSortIndex;
        if(ft && !ft->isCache())
            delete ft;

        convertPercentFile.close();
        for(int i = 0; i < hideStateNum; ++i)
            hidePercentFiles[i].close();
        delete []hidePercentFiles;
    }

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
                cout<<"day num error\n";
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
