//
// Created by yanyu on 2017/7/12.
//
#ifndef ALPHATREE_ALPHATREE_H
#define ALPHATREE_ALPHATREE_H

#include <map>
#include "unit.h"
#include "alphaatom.h"
#include "converter.h"
#include "alphadb.h"
#include "../base/threadpool.h"

const size_t MAX_LEAF_DATA_CLASS_LEN = 64;
const size_t MAX_SUB_TREE_NAME_LEN = 64;

//inline float logalpha2returns(float logalpha){ return (expf(logalpha) - 1) * 100; }

class AlphaTree{
    public:

        class AlphaNode{
            public:
                AlphaNode(): leftId(-1), rightId(-1), preId(-1), element_(nullptr){
                    //memset(des_, 0, MAX_LEAF_DATA_CLASS_LEN * sizeof(char));
                    des_[0] = 0;
                }

                //是否是子树
                bool isRoot(){ return preId == -1;}
                //得到名字
                const char* getName(){ return element_->getName();}
                //得到系数
                double getCoff(){ return coff_;}
                //得到分类
                const char* getWatchLeafDataClass(){return des_[0] == 0 ? nullptr : des_;}
                //得到系数
                void getCoffStr(char* coffStr){
                    //写系数
                    switch(getCoffUnit()){
                        case CoffUnit::COFF_NONE:
                            coffStr[0] = 0;
                            break;
                        case CoffUnit::COFF_DAY:
                        case CoffUnit::COFF_FUTURE_DAY:
                        case CoffUnit::COFF_CONST:
                            {
                                sprintf(coffStr,"%.8f", getCoff());
                                int curIndex = strlen(coffStr);
                                while(coffStr[curIndex-1] == '0' || coffStr[curIndex-1] == '.') {
                                    --curIndex;
                                    if(coffStr[curIndex] == '.'){
                                        coffStr[curIndex] = 0;
                                        break;
                                    }
                                }
                                coffStr[curIndex] = 0;
                            }
                            break;
                        case CoffUnit::COFF_INDCLASS:
                            strcpy(coffStr, getWatchLeafDataClass());
                            break;
                    }
                }


                void setup(IAlphaElement* element, double coff = 0, const char* watchLeafDataClass = nullptr){
                    element_ = element;
                    coff_ = coff;
                    preId = -1;

                    if(watchLeafDataClass)
                        strcpy(des_, watchLeafDataClass);
                    else
                        des_[0] = 0;
                }

                int getChildNum(){ return element_->getChildNum();}

                CoffUnit getCoffUnit(){return element_->getCoffUnit();}

                IAlphaElement* getElement(){ return element_;}

                int leftId, rightId, preId;

                std::shared_future<const float*> res;

            protected:
                double coff_;
                char des_[MAX_LEAF_DATA_CLASS_LEN];
                IAlphaElement* element_;

        };

        //注意,规定子树中不能包含工业数据
        class SubTreeDes{
            public:
                char name[MAX_SUB_TREE_NAME_LEN];
                int rootId;
        };

        AlphaTree(size_t maxNodeSize, size_t maxSubTreeNum){
            nodeList_ = new AlphaNode[maxNodeSize];
            subtreeList_ = new SubTreeDes[maxSubTreeNum];
            clean();
        }
        ~AlphaTree(){
            delete []nodeList_;
            delete []subtreeList_;
        }

        void clean(){
            nodeLen_ = 0;
            subtreeLen_ = 0;
            historyDay_ = NONE;
            futureDay_ = NONE;
        }

        //必须先解码子树再解码主树
        void decode(const char* name, const char* line, map<const char*,IAlphaElement*, ptrCmp>& alphaElementMap){
            int outCache[MAX_DECODE_RANGE_LEN];
            char optCache[MAX_OPT_STR_LEN];
            char normalizeLine[MAX_ALPHATREE_STR_LEN];
            converter_.operator2function(line, normalizeLine);
            strcpy(subtreeList_[subtreeLen_].name, name);
            subtreeList_[subtreeLen_].rootId = decode(normalizeLine, alphaElementMap, 0, strlen(normalizeLine)-1, outCache, optCache);
            subtreeLen_++;
        }

        void decode(const char* line, map<const char*,IAlphaElement*, ptrCmp>& alphaElementMap){
            int outCache[MAX_DECODE_RANGE_LEN];
            char optCache[MAX_OPT_STR_LEN];
            char normalizeLine[MAX_ALPHATREE_STR_LEN];
            converter_.operator2function(line, normalizeLine);
            decode(normalizeLine, alphaElementMap, 0, strlen(normalizeLine)-1, outCache, optCache);

            //在编码主树时,子树的父节点会被写坏,重新再恢复一次
            for(int i = 0; i < subtreeLen_; ++i){
                getNode(subtreeList_[i].rootId)->preId = -1;
            }
        }

        //编码成字符串
        const char* encode(char* pout){
            int curIndex = 0;
            char normalizeLine[MAX_ALPHATREE_STR_LEN];
            encode(normalizeLine, getRootId(), curIndex);
            normalizeLine[curIndex] = 0;
            converter_.function2operator(normalizeLine, pout);
            return pout;
        }

        int searchPublicSubstring(const char* line, char* pout, int minTime = 1, int minDepth = 3){
            int curIndex = 0;
            //缓存编码出来的字符串,*2是因为字符串包括编码成符号的
            char normalizeLine[MAX_ALPHATREE_STR_LEN * 2];
            return searchPublicSubstring(getRootId(), line, normalizeLine, curIndex, pout, minTime, minDepth);
        }

        void addLeftChild(int nodeId, int childId){
            nodeList_[nodeId].leftId = childId;
            nodeList_[childId].preId = nodeId;
        }

        void addRightChild(int nodeId, int childId){
            nodeList_[nodeId].rightId = childId;
            nodeList_[childId].preId = nodeId;
        }

        int getLeftChild(int nodeId){ return nodeList_[nodeId].leftId;}
        int getRightChild(int nodeId){ return nodeList_[nodeId].rightId;}

        int createNode(IAlphaElement* element, double coff = 0, const char* watchLeafDataClass = nullptr){
            nodeList_[nodeLen_].setup(element, coff, watchLeafDataClass);
            return nodeLen_++;
        }

        int getRootId(){ return nodeLen_ - 1;}

        AlphaNode* getNode(int nodeId){
            AlphaNode* node = nodeList_ + nodeId;
            return node;
        }

        const char* getName(int nodeId){
            auto node = getNode(nodeId);
            //如果是子树
            if(node->isRoot()){
                for(int i = 0; i < subtreeLen_; ++i){
                    if(subtreeList_[i].rootId == nodeId){
                        return subtreeList_[i].name;
                    }
                }
            }
            return node->getName();
        }

        size_t getHistoryDays(){
            if(historyDay_ == NONE)
                historyDay_ = getHistoryDays(getRootId()) + 1;
            return historyDay_;
        }

        size_t getFutureDays(){
            if(futureDay_ == NONE)
                futureDay_ = getFutureDays(getRootId());
            return futureDay_;
        }

//        float* getLastCacheMemory(size_t sampleSize, size_t stockSize, int futureId, float* cacheMemory){
//            int dateSize = AlphaDataBase::getElementSize(getHistoryDays(), sampleSize, getFutureDays());
//            return getNodeCacheMemory(getRootId()+futureId, dateSize, stockSize, cacheMemory);
//        }

        //计算alpha并返回
        std::shared_future<const float*> calAlpha(AlphaDataBase *alphaDataBase, size_t dayBefore, size_t sampleSize, bool *flagCache, float *cacheMemory,
                       const char *codes, size_t stockSize, ThreadPool *threadPool = nullptr){
            //cout<<"start cal"<<endl;
            //获取历史和未来日期,避免在多线程中再去获取
            getHistoryDays();
            getFutureDays();

            //标记要运算的日期
            flagNeedCalDate(sampleSize, flagCache);
            //cout<<"finish flag"<<endl;

            for(int i = 0; i < nodeLen_; ++i) {
                int nodeId = i;
                //cout<<"request node "<<i<<endl;
                AlphaTree* alphaTree = this;
                getNode(nodeId)->res = threadPool->enqueue([alphaTree, alphaDataBase, nodeId, dayBefore, sampleSize, stockSize, codes, flagCache, cacheMemory]{
                    //cout<<"start node "<<nodeId<<" "<<alphaTree->getNode(nodeId)->getName()<<endl;
                    const float* res = alphaTree->cast(alphaDataBase, nodeId, dayBefore, sampleSize, stockSize, codes, flagCache, cacheMemory);
                    //cout<<"finish node "<<nodeId<<endl;
                    //如果不是根节点,返回所有缓存数据,否则仅返回取样数据
                    return nodeId == alphaTree->getRootId() ? (res + (int)((alphaTree->getHistoryDays() - 1) * stockSize)) : res;
                }).share();
            }
            //cout<<"finish cal req"<<endl;
            //return getNode(getRootId())->res.get() + (int)((getHistoryDays() - 1) * stockSize);
            return getNode(getRootId())->res;
        }


        const float* cast(AlphaDataBase* alphaDataBase, int nodeId, size_t dayBefore, size_t sampleSize, size_t stockSize,
                          const char* codes, bool* flagCache, float* cacheMemory){
            //cout<<"start cast "<<nodeId<<endl;
            int dateSize = AlphaDataBase::getElementSize(getHistoryDays(), sampleSize, getFutureDays());
            float* curCacheMemory = getNodeCacheMemory(nodeId, dateSize, stockSize, cacheMemory);
            //计算叶子节点
            bool* pflag = getNodeFlag(nodeId, dateSize, flagCache);
            //cout<<"finish flag "<<nodeId<<endl;
            if(getNode(nodeId)->getChildNum() == 0) {
                alphaDataBase->getStock(dayBefore, getFutureDays(), getHistoryDays(), sampleSize, stockSize, ((AlphaPar *) getNode(nodeId)->getElement())->leafDataType, getNode(nodeId)->getWatchLeafDataClass(), pflag, curCacheMemory, codes);
            }
            else{
                const float* leftCacheMemory = getNode(getNode(nodeId)->leftId)->res.get();
                const float* rightCacheMemory = (getNode(nodeId)->getChildNum() == 1) ? nullptr : getNode(getNode(nodeId)->rightId)->res.get();
                //cout<<"finish read child "<<getNode(nodeId)->leftId<<endl;
                //float* leftCacheMemory = getNodeCacheMemory(getNode(nodeId)->leftId, dateSize, stockSize, cacheMemory);
                //float* rightCacheMemory = (getNode(nodeId)->getChildNum() == 1) ? nullptr : getNodeCacheMemory(getNode(nodeId)->rightId, dateSize, stockSize, cacheMemory);
                getNode(nodeId)->getElement()->cast(leftCacheMemory, rightCacheMemory, getNode(nodeId)->getCoff(), dateSize, stockSize, pflag, curCacheMemory);
                //cout<<"finish cast "<<nodeId<<endl;
            }
            //cout<<"finish "<<nodeId<<endl;
            return curCacheMemory;
        }

        //读取已经计算好的alpha
        const float* getAlpha(int nodeId, size_t sampleSize, float *cacheMemory, size_t stockSize){
            float* curCacheMemory = cacheMemory + nodeId * AlphaDataBase::getElementSize(getHistoryDays(), sampleSize, getFutureDays()) * stockSize;
            return curCacheMemory + (int)((getHistoryDays() - 1) * stockSize);
        }

        int getNodeNum(){
            return nodeLen_;
        }

        //是否是子树的根节点
        bool isSubtreeRoot(int nodeId){
            if(getNode(nodeId)->isRoot()){
                for(int i = 0; i < subtreeLen_; ++i){
                    if(subtreeList_[i].rootId == nodeId){
                        return true;
                    }
                }
            }
            return false;
        }

        static const float* getAlpha(const float* res, size_t sampleIndex, size_t stockSize){ return res + (sampleIndex * stockSize);}
        static inline float* getNodeCacheMemory(int nodeId, int dateSize, int stockSize, float* cacheMemory){
            return cacheMemory + nodeId * dateSize * stockSize;
        }

        static inline bool* getNodeFlag(int nodeId, int dateSize, bool* flagCache){
            return flagCache + nodeId * dateSize;
        }
    protected:
        int getDepth(int nodeId){
            if(nodeList_[nodeId].getChildNum() == 0)
                return 1;
            int leftDepth = getNode(nodeList_[nodeId].leftId)->isRoot() ? 1 : getDepth(nodeList_[nodeId].leftId);
            if(nodeList_[nodeId].getChildNum() == 1)
                return leftDepth + 1;
            int rightDepth = getNode(nodeList_[nodeId].rightId)->isRoot() ? 1 : getDepth(nodeList_[nodeId].rightId);
            return leftDepth > rightDepth ? leftDepth : rightDepth;
        }

        float getHistoryDays(int nodeId){
            if(nodeList_[nodeId].getChildNum() == 0)
                return 0;
            float leftDays = getHistoryDays(nodeList_[nodeId].leftId);
            float rightDays = 0;
            if(nodeList_[nodeId].getChildNum() == 2)
                rightDays = getHistoryDays(nodeList_[nodeId].rightId);
            int maxDays = leftDays > rightDays ? leftDays : rightDays;
            if(nodeList_[nodeId].getCoffUnit() == CoffUnit::COFF_DAY)
                return roundf(nodeList_[nodeId].getCoff()) + maxDays;
            return maxDays;
        }

        float getFutureDays(int nodeId){
            if(nodeList_[nodeId].getChildNum() == 0)
                return 0;
            float leftDays = getFutureDays(nodeList_[nodeId].leftId);
            float rightDays = 0;
            if(nodeList_[nodeId].getChildNum() == 2)
                rightDays = getFutureDays(nodeList_[nodeId].rightId);
            int maxDays = leftDays > rightDays ? leftDays : rightDays;
            if(nodeList_[nodeId].getCoffUnit() == CoffUnit::COFF_FUTURE_DAY)
                return roundf(nodeList_[nodeId].getCoff()) + maxDays;
            return maxDays;
        }

        int decode(const char* line, map<const char*,IAlphaElement*, ptrCmp>& alphaElementMap, int l, int r, int* outCache, char* optCache, const char* parDataClass = nullptr){
            converter_.decode(line, l, r, outCache);
            const char* opt = converter_.readOpt(line, outCache, optCache);
            //读出系数
            double coff = 0;
            //读出操作符
            IAlphaElement* alphaElement  = nullptr;
            //创建节点
            int nodeId = NONE;
            //读出左右孩子
            int ll = outCache[2], lr = outCache[3], rl = outCache[4], rr = outCache[5];

            int leftId = -1, rightId = -1;

            char curDataClass[MAX_LEAF_DATA_CLASS_LEN];
            if(parDataClass != nullptr)
                strcpy(curDataClass, parDataClass);
            char* dataClass = (parDataClass == nullptr) ? nullptr : curDataClass;

            //特殊处理一些操作符的系数
            if(converter_.isSymbolFun(opt)){
                //如果第一个孩子是数字,变成opt_from
                if(converter_.getOptSize(outCache, 1) < MAX_OPT_STR_LEN && converter_.isNum(converter_.readOpt(line, outCache, optCache,1))){
                    coff = atof(optCache);
                    //重新得到刚才的操作数
                    opt = converter_.readOpt(line, outCache, optCache);
                    //在操作符后面拼接上_from
                    char* p = optCache + strlen(opt);
                    strcpy(p, "_from");
                    alphaElement = alphaElementMap[optCache];
                    leftId = decode(line, alphaElementMap, rl, rr, outCache, optCache, dataClass);
                    nodeId = createNode(alphaElement, coff, dataClass);
                    //字符串的第一部分是系数,第二部分才是子孩子,添加第二部分
                    addLeftChild(nodeId, leftId);
                    //将这种系数在左边的特殊情况直接返回
                    return nodeId;
                } else if(converter_.getOptSize(outCache, 2) < MAX_OPT_STR_LEN && converter_.isNum(converter_.readOpt(line, outCache, optCache, 2))){
                    //重新得到刚才的操作数
                    opt = converter_.readOpt(line, outCache, optCache);
                    //在操作符后面拼接上_from
                    char* p = optCache + strlen(opt);
                    strcpy(p, "_to");
                } else {
                    //重新得到刚才的操作数
                    opt = converter_.readOpt(line, outCache, optCache);
                }
            }

            //特殊处理子树
            for(int i = 0; i < subtreeLen_; i++){
                if(strcmp(opt, subtreeList_[i].name) == 0){
                    return subtreeList_[i].rootId;
                }
            }

            //特殊处理rank,拆分成两个节点---------------------------------------------------------------
            if(strcmp(opt,"rank") == 0){
                leftId = decode(line, alphaElementMap, ll, lr, outCache, optCache, dataClass);
                int nextNodeId = createNode(alphaElementMap["rank_sort"], 0, dataClass);
                addLeftChild(nextNodeId, leftId);
                nodeId = createNode(alphaElementMap["rank_scale"], 0, dataClass);
                addLeftChild(nodeId, nextNodeId);
                return nodeId;
            }
            //-----------------------------------------------------------------------------------------

            alphaElement = alphaElementMap[opt];
            if(alphaElement->getCoffUnit() != CoffUnit::COFF_NONE){
                converter_.readOpt(line, outCache, optCache, alphaElement->getChildNum() + 1);
                switch (alphaElement->getCoffUnit()){
                    case CoffUnit::COFF_DAY:
                    case CoffUnit::COFF_CONST:
                    case CoffUnit::COFF_FUTURE_DAY:
                        coff = atof(optCache);
                        break;
                    case CoffUnit::COFF_INDCLASS:
                        strcpy(curDataClass, optCache);
                        dataClass = curDataClass;
                        break;
                    default:
                        throw "coff error!";
                }
            }

            //保证自底向上的方式创建节点,根节点一定排在数组最后面
            if(alphaElement->getChildNum() > 0){
                leftId = decode(line, alphaElementMap, ll, lr, outCache, optCache, dataClass);
            }
            if(alphaElement->getChildNum() > 1){
                rightId = decode(line, alphaElementMap, rl, rr, outCache, optCache, dataClass);
            }

            nodeId = createNode(alphaElement, coff, dataClass);
            if(leftId != -1)
                addLeftChild(nodeId, leftId);
            if(rightId != -1)
                addRightChild(nodeId, rightId);

            return nodeId;
        }

        void encode(char* pout, int nodeId, int& curIndex){
            AlphaNode* node = getNode(nodeId);
            const char* name = getName(nodeId);
            int nameLen = strlen(name);

            //编码子树
            if(isSubtreeRoot(nodeId)){
                strcpy(pout + curIndex, name);
                curIndex += nameLen;
                return;
            }

            //特殊处理系数在左边的情况
            if(nameLen > 5 and strcmp(name + (nameLen-5),"_from") == 0){
                //先写名字
                memcpy(pout + curIndex, node->getName(), (nameLen - 5) * sizeof(char));
                curIndex += (nameLen - 5);
                pout[curIndex++] = '(';

                //写系数
                sprintf(pout + curIndex,"%.8f", node->getCoff());
                curIndex = strlen(pout + curIndex) + curIndex;
                while(pout[curIndex-1] == '0' || pout[curIndex-1] == '.') {
                    --curIndex;
                    if (pout[curIndex] == '.')
                        break;
                }

                //写左孩子
                pout[curIndex++] = ',';
                pout[curIndex++] = ' ';
                encode(pout, node->leftId, curIndex);

                pout[curIndex++] = ')';
            } else {
                //特殊处理rank(因为rank被拆分成两个节点)------------------------------------
                if(strcmp(name,"rank_scale") == 0){
                    encode(pout, node->leftId, curIndex);
                    return;
                }
                if(strcmp(name,"rank_sort") == 0){
                    nameLen -= 5;
                }
                //-----------------------------------------------------------------------

                //特殊处理一些符号
                if(nameLen > 3 and strcmp(name + (nameLen-3),"_to") == 0)
                    nameLen -= 3;

                //先写名字
                memcpy(pout + curIndex, node->getName(), nameLen * sizeof(char));
                curIndex += nameLen;

                if(node->getChildNum() > 0) {
                    //写左孩子
                    pout[curIndex++] = '(';
                    encode(pout, node->leftId, curIndex);

                    //写右孩子
                    if(node->getChildNum() > 1){
                        pout[curIndex++] = ',';
                        pout[curIndex++] = ' ';
                        encode(pout, node->rightId, curIndex);
                    }

                    //写系数
                    if(node->getCoffUnit() != CoffUnit::COFF_NONE){
                        pout[curIndex++] = ',';
                        pout[curIndex++] = ' ';
                        node->getCoffStr(pout + curIndex);
                        curIndex += strlen(pout + curIndex);
                    }
//                    switch(node->getCoffUnit()){
//                        case CoffUnit::COFF_NONE:
//                            break;
//                        case CoffUnit::COFF_DAY:
//                        case CoffUnit::COFF_FUTURE_DAY:
//                        case CoffUnit::COFF_CONST:
//                            pout[curIndex++] = ',';
//                            pout[curIndex++] = ' ';
//                            sprintf(pout + curIndex,"%.8f", node->getCoff());
//                            curIndex = strlen(pout + curIndex) + curIndex;
//                            while(pout[curIndex-1] == '0' || pout[curIndex-1] == '.') {
//                                --curIndex;
//                                if(pout[curIndex] == '.')
//                                    break;
//                            }
//                            break;
//                        case CoffUnit::COFF_INDCLASS:
//                            pout[curIndex++] = ',';
//                            pout[curIndex++] = ' ';
//                            const char* coff = node->getWatchLeafDataClass();
//                            int len = strlen(coff);
//                            memcpy(pout + curIndex, coff, len * sizeof(char));
//                            curIndex += len;
//                            break;
//                    }

                    pout[curIndex++] = ')';
                }
            }
        }

        //搜索某个节点下所有子串是否在line中出现minTime以上(包括minTime),限制节点的最小深度minDepth
        int searchPublicSubstring(int nodeId, const char* line, char* encodeCache, int& curIndex, char* pout, int minTime = 1, int minDepth = 3){
            if(getDepth(nodeId) < minDepth)
                return 0;
            int encodeIndex = 0;
            encode(encodeCache, nodeId, encodeIndex);
            encodeCache[encodeIndex] = 0;
            char* realSubstring = encodeCache + (encodeIndex+1);
            converter_.function2operator(encodeCache, realSubstring);

            int count = 0;
            char* subline = strstr(line, realSubstring);
            while (subline){
                ++count;
                subline += 1;
                subline = strstr(subline, realSubstring);
            }

            if(count >= minTime){
                strcpy(pout + curIndex, realSubstring);
                curIndex += (strlen(realSubstring) + 1);
                return 1;
            }

            count = 0;
            if(getNode(nodeId)->getChildNum() > 0)
                count += searchPublicSubstring(getNode(nodeId)->leftId, line, encodeCache, curIndex, pout, minTime, minDepth);
            if(getNode(nodeId)->getChildNum() > 1)
                count += searchPublicSubstring(getNode(nodeId)->rightId, line, encodeCache, curIndex, pout, minTime, minDepth);
            return count;
        }

        //标记需要计算的数据
        void flagNeedCalDate(size_t sampleSize, bool *cacheFlag){
            int dateSize = AlphaDataBase::getElementSize(getHistoryDays(), sampleSize, getFutureDays());
            memset(cacheFlag, 0, dateSize * nodeLen_ * sizeof(bool));

            //标记根节点需要的输出
            bool* curFlagCache = getNodeFlag(getRootId(), dateSize, cacheFlag);
            for(int i = getHistoryDays()-1; i < getHistoryDays()-1+sampleSize; ++i){
                curFlagCache[i] = true;
            }
            flagChildNeedCalDate(getRootId(), sampleSize, cacheFlag);

            //标记子树
            for(int i = 0; i < subtreeLen_; ++i){
                flagChildNeedCalDate(subtreeList_[i].rootId, sampleSize, cacheFlag);
            }

            //测试----------
//            for(int i = 0; i < subtreeLen_; ++i){
//                bool* curFlagCache = cacheFlag + subtreeList_[i].rootId * dateSize;
//                cout<<subtreeList_[i].name<<endl;
//                for(int i = 0; i < dateSize; ++i){
//                    cout<<int(curFlagCache[i])<<" ";
//                }
//                cout<<endl;
//            }
        }

        //给某个节点下所有孩子标记需要输出的日期
        void flagChildNeedCalDate(int nodeId, size_t sampleSize, bool *cacheFlag){
            int dateSize = AlphaDataBase::getElementSize(getHistoryDays(), sampleSize, getFutureDays());
            bool* curFlagCache = getNodeFlag(nodeId, dateSize, cacheFlag);

            if(nodeList_[nodeId].getChildNum() > 0){
                //计算当前节点的需要的数据标记(即当前孩子需要输出的数据标记)
                bool* leftFlagCache = getNodeFlag(getNode(nodeId)->leftId, dateSize, cacheFlag);
                calFlag(curFlagCache, leftFlagCache, (int)roundf(getNode(nodeId)->getCoff()), dateSize,  getNode(nodeId)->getElement()->getDateRange());
                if(!isSubtreeRoot(getNode(nodeId)->leftId))
                    flagChildNeedCalDate(getNode(nodeId)->leftId, sampleSize, cacheFlag);
                if(nodeList_[nodeId].getChildNum() > 1) {
                    bool* rightFlagCache = getNodeFlag(getNode(nodeId)->rightId, dateSize, cacheFlag);
                    calFlag(curFlagCache, rightFlagCache, (int)roundf(getNode(nodeId)->getCoff()), dateSize,  getNode(nodeId)->getElement()->getDateRange());
                    if(!isSubtreeRoot(getNode(nodeId)->rightId))
                        flagChildNeedCalDate(getNode(nodeId)->rightId, sampleSize, cacheFlag);
                }
            }

            //测试---------------------
//            cout<<nodeList_[nodeId].getName()<<endl;
//            for(int i = 0; i < dateSize; ++i)
//                cout << int(curFlagCache[i]) << " ";
//            cout<<endl;
        }

        inline static void calFlag(const bool* parFlagCache, bool *cacheFlag, int dayNum, int dateSize, DateRange dateRange){
            switch (dateRange){
                case DateRange::CUR_DAY:
                    for(int i = 0; i < dateSize; ++i)
                        if(parFlagCache[i])
                            cacheFlag[i] = true;
                    break;
                case DateRange::BEFORE_DAY:
                    for(int i = 0; i < dateSize; ++i)
                        if(i+dayNum < dateSize && parFlagCache[i+dayNum])
                            cacheFlag[i] = true;
                    break;
                case DateRange::CUR_AND_BEFORE_DAY:
                    for(int i = 0; i < dateSize; ++i){
                        if(i < dateSize-dayNum && parFlagCache[i+dayNum])
                            cacheFlag[i] = true;
                        else if(parFlagCache[i])
                            cacheFlag[i] = true;
                    }
                    break;
                case DateRange::ALL_DAY:
                    for(int i = 0; i < dateSize-dayNum; ++i)
                        if(parFlagCache[i+dayNum])
                            for(int j = 0; j <= dayNum; ++j)
                                cacheFlag[(i + j)] = true;
                    break;
                case DateRange::FUTURE_DAY:
                    for(int i = dateSize-1; i >= 0; --i) {
                        if (i - dayNum >= 0 && parFlagCache[(i - dayNum)])
                            cacheFlag[i] = true;
                    }
                    break;
            }
        }

        AlphaNode*nodeList_;
        size_t nodeLen_;

        size_t historyDay_;
        size_t futureDay_;

        SubTreeDes* subtreeList_;
        size_t subtreeLen_;

        static AlphaTreeConverter converter_;
};

AlphaTreeConverter AlphaTree::converter_ = AlphaTreeConverter();

#endif //ALPHATREE_ALPHATREE_H
