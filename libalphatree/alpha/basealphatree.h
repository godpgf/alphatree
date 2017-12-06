//
// Created by yanyu on 2017/10/12.
//

#ifndef ALPHATREE_BASEALPHATREE_H
#define ALPHATREE_BASEALPHATREE_H

#include "alphaatom.h"
#include "alphaprocess.h"
#include "converter.h"
#include "alphadb.h"

const size_t MAX_NODE_STR_LEN = 512;
const size_t MAX_SUB_ALPHATREE_STR_NUM = 512;
const size_t MAX_DECODE_RANGE_LEN = 10;
const size_t MAX_OPT_STR_LEN = 64;

const size_t MAX_LEAF_DATA_STR_LEN = 64;
const size_t MAX_NODE_NAME_LEN = 64;

const size_t MAX_SUB_TREE_BLOCK = 16;
const size_t MAX_NODE_BLOCK = 64;

#define STOCK_SIZE 3600
#define HISTORY_DAYS 512
#define SAMPLE_DAYS 2500

class AlphaNode{
public:
    //是否是子树
    bool isRoot(){ return preId == -1;}
    //得到名字
    const char* getName(){ return element_ ?  element_->getName() : dataName_;}
    //得到系数
    double getCoff(DArray<double, MAX_NODE_BLOCK>& coffList){
        if(isLocalCoff())
            return coff_;
        else
            return coffList[externalCoffIndex_];
    }

    double getCoff(){
        return coff_;
    }

    //系数是否是内部的
    bool isLocalCoff(){ return externalCoffIndex_ < 0;}
    //得到分类
    const char* getWatchLeafDataClass(){return dataClass_[0] == 0 ? nullptr : dataClass_;}
    //得到系数
    int getCoffStr(char* coffStr, DArray<double, MAX_NODE_BLOCK>& coffList){
        //写系数
        switch(getCoffUnit()){
            case CoffUnit::COFF_NONE:
                coffStr[0] = 0;
                return 0;
            case CoffUnit::COFF_DAY:
            case CoffUnit::COFF_CONST:
            {
                sprintf(coffStr,"%.8f", getCoff(coffList));
                int curIndex = strlen(coffStr);
                while(coffStr[curIndex-1] == '0' || coffStr[curIndex-1] == '.') {
                    --curIndex;
                    if(coffStr[curIndex] == '.'){
                        coffStr[curIndex] = 0;
                        break;
                    }
                }
                coffStr[curIndex] = 0;
                return curIndex;
            }
            case CoffUnit::COFF_INDCLASS:
                strcpy(coffStr, getWatchLeafDataClass());
                return strlen(coffStr);
        }
        return 0;
    }


    void setup(IAlphaElement* element, double coff = 0, const char* watchLeafDataClass = nullptr, int externalCoffIndex = -1){
        CHECK(element != nullptr, "err");
        setup(nullptr, watchLeafDataClass);
        element_ = element;
        externalCoffIndex_ = externalCoffIndex;
        coff_ = coff;
        preId = -1;
    }

    void setup(const char* name = nullptr, const char* watchLeafDataClass = nullptr){
        if(watchLeafDataClass)
            strcpy(dataClass_, watchLeafDataClass);
        else
            dataClass_[0] = 0;
        if(name)
            strcpy(dataName_, name);
        else
            dataName_[0] = 0;
        element_ = nullptr;
    }

    int getChildNum(){return element_ == nullptr ? 0 : element_->getChildNum();}

    CoffUnit getCoffUnit(){return element_->getCoffUnit();}

    float getNeedBeforeDays(){
        if(getCoffUnit() == CoffUnit::COFF_DAY)
            return fmaxf(roundf(coff_), element_->getMinHistoryDays());
        return element_->getMinHistoryDays();
    }

    double getNeedBeforeDays(DArray<double, MAX_NODE_BLOCK>& coffList){
        double day = 0;
        if(getCoffUnit() == CoffUnit::COFF_DAY){
            if(isLocalCoff())
                day = coff_;
            else
                day = coffList[externalCoffIndex_];
        }
        return fmaxf(day, element_->getMinHistoryDays());
    }

    IAlphaElement* getElement(){ return element_;}

    int getExternalCoffId(){ return externalCoffIndex_;};
    int leftId = {-1};
    int rightId = {-1};
    int preId = {-1};
protected:
    int externalCoffIndex_ = {-1};
    double coff_ = {0};
    char dataClass_[MAX_LEAF_DATA_STR_LEN] = {0};
    char dataName_[MAX_LEAF_DATA_STR_LEN] = {0};
    IAlphaElement* element_ = {nullptr};
};


//注意,规定子树中不能包含工业数据
class SubTreeDes{
public:
    char name[MAX_NODE_NAME_LEN];
    int rootId;
    //是否是局部的(不需要输出只作为中间结果)
    bool isLocal;
};

//后处理,在树计算完成后的结果上再做处理
class AlphaProcessNode{
    public:
        void setup(const char* name, IAlphaProcess* process){
            strcpy(this->name, name);
            process_ = process;
            chilIndex.resize(0);
            childRes.resize(0);
            coff.resize(0);
        }

        ~AlphaProcessNode(){
            chilIndex.clear();
            childRes.clear();
            coff.clear();
        }

        IAlphaProcess* getProcess(){
            return process_;
        }

        char name[MAX_NODE_NAME_LEN];
        //引用的子树
        DArray<int, MAX_PROCESS_BLOCK> chilIndex;
        DArray<const float*, MAX_PROCESS_BLOCK> childRes;
        DArray<char[MAX_PROCESS_COFF_STR_LEN],MAX_PROCESS_BLOCK> coff;
    protected:
        IAlphaProcess* process_ = {nullptr};
};


//仅仅负责树的构造,不负责运算
class BaseAlphaTree{
public:

    BaseAlphaTree(){
        clean();
    }

    virtual ~BaseAlphaTree(){
        coffList_.clear();
        nodeList_.clear();
        subtreeList_.clear();
        processList_.clear();
    }

    virtual void clean(){
        coffList_.resize(0);
        nodeList_.resize(0);
        subtreeList_.resize(0);
        processList_.resize(0);
    }

    int getSubtreeSize(){ return subtreeList_.getSize();}
    const char* getSubtreeName(int index){ return subtreeList_[index].name;}

    void setCoff(int index, double coff){
        coffList_[index] = coff;
    }

    double getCoff(int index){
        return coffList_[index];
    }


    //必须先解码子树再解码主树
    void decode(const char* rootName, const char* line, HashMap<IAlphaElement*>& alphaElementMap, bool isLocal){
        int outCache[MAX_DECODE_RANGE_LEN];
        char optCache[MAX_OPT_STR_LEN];
        char normalizeLine[MAX_NODE_STR_LEN];
        converter_.operator2function(line, normalizeLine);
        int subtreeLen = subtreeList_.getSize();
        strcpy(subtreeList_[subtreeLen].name, rootName);
        int rootId = decode(normalizeLine, alphaElementMap, 0, strlen(normalizeLine)-1, outCache, optCache);
        subtreeList_[subtreeLen].rootId = rootId;
        subtreeList_[subtreeLen].isLocal = isLocal;
        //恢复被写坏的preId
        for(auto i = 0; i < subtreeLen; ++i)
            nodeList_[subtreeList_[i].rootId].preId = -1;
    }


    //编码成字符串
    const char* encode(const char* rootName, char* pout){
        int curIndex = 0;
        char normalizeLine[MAX_NODE_STR_LEN];
        encode(normalizeLine, getSubtreeRootId(rootName), curIndex, getSubtreeIndex(rootName));
        normalizeLine[curIndex] = 0;
        converter_.function2operator(normalizeLine, pout);
        return pout;
    }

    //编码后处理
    void decodeProcess(const char* processName, const char* line, HashMap<IAlphaProcess*>& alphaProcessMap, HashMap<IAlphaElement*>& alphaElementMap){
        int processLen = processList_.getSize();
        char normalizeLine[MAX_NODE_STR_LEN];
        strcpy(normalizeLine, line);

        //把最后一个')'变成0
        CHECK(normalizeLine[strlen(normalizeLine)-1] == ')', "格式错误");
        normalizeLine[strlen(normalizeLine)-1] = 0;

        int nameLen = 0;
        while(normalizeLine[nameLen] != '(')
            ++nameLen;
        normalizeLine[nameLen] = 0;
        const char* optName = normalizeLine;
        IAlphaProcess* opt = alphaProcessMap[optName];
        processList_[processLen].setup(processName, opt);

        int startIndex = nameLen+1;
        int endIndex = startIndex;
        int curChildNum = 0;
        int curCoffNum = 0;
        int depth = 0;
        char processSubtreeName[MAX_NODE_NAME_LEN];
        //解码孩子和系数
        while(true){
            if((normalizeLine[endIndex] == ',' || normalizeLine[endIndex] == 0) && depth == 0){
                bool isFinish = (normalizeLine[endIndex] == 0);
                normalizeLine[endIndex] = 0;
                if(curChildNum < opt->getChildNum()){
                    int subtreeIndex = getSubtreeIndex(normalizeLine + startIndex);
                    if(subtreeIndex == -1){
                        //如果找不到子树就解码出一个来
                        subtreeIndex = subtreeList_.getSize();
                        sprintf(processSubtreeName, "process:%d",curChildNum);
                        decode(processSubtreeName, normalizeLine + startIndex, alphaElementMap, false);
                    }
                    processList_[processLen].chilIndex[curChildNum++] = subtreeIndex;
                }
                else
                    strcpy(processList_[processLen].coff[curCoffNum++],normalizeLine + startIndex);
                startIndex = endIndex+1;
                if(isFinish){
                    break;
                }
            } else if(normalizeLine[endIndex] == '('){
                ++depth;
            } else if(normalizeLine[endIndex] == ')'){
                --depth;
            }
            if(normalizeLine[startIndex] == ' ')
                ++startIndex;
            ++endIndex;
        }
    }
    //解码后处理
    const char* encodeProcess(const char* processName, char* pout){
        for(auto i = 0; i < processList_.getSize(); ++i){
            if(strcmp(processList_[i].name, processName) == 0){
                strcpy(pout, processList_[i].getProcess()->getName());
                char* curStr = pout + strlen(pout);
                curStr[0] = '(';
                curStr += 1;
                for(int j = 0; j < processList_[i].chilIndex.getSize() + processList_[i].coff.getSize(); ++j){
                    if(j < processList_[i].chilIndex.getSize())
                        strcpy(curStr, subtreeList_[processList_[i].chilIndex[j]].name);
                    else
                        strcpy(curStr, processList_[i].coff[j - processList_[i].chilIndex.getSize()]);
                    curStr += strlen(curStr);
                    if(j != processList_[i].chilIndex.getSize() + processList_[i].coff.getSize() - 1){
                        curStr[0] = ',';
                        curStr[1] = ' ';
                        curStr += 2;
                    }
                }

                curStr[0] = ')';
                curStr[1] = 0;
                return pout;
            }
        }
        return nullptr;
    }


    int searchPublicSubstring(const char* rootName, const char* line, char* pout, int minTime = 1, int minDepth = 3){
        int curIndex = 0;
        //缓存编码出来的字符串,*2是因为字符串包括编码成符号的
        char normalizeLine[MAX_NODE_STR_LEN * 2];
        return searchPublicSubstring(getSubtreeRootId(rootName), getSubtreeIndex(rootName), line, normalizeLine, curIndex, pout, minTime, minDepth);
    }
protected:
    int decode(const char* line, HashMap<IAlphaElement*>& alphaElementMap, int l, int r, int* outCache, char* optCache, const char* parDataClass = nullptr){
        converter_.decode(line, l, r, outCache);
        const char* opt = converter_.readOpt(line, outCache, optCache);
        //读出系数
        double coff = 0;
        int externalCoffIndex = -1;
        //读出操作符
        IAlphaElement* alphaElement  = nullptr;
        //创建节点
        int nodeId = NONE;
        //读出左右孩子
        int ll = outCache[2], lr = outCache[3], rl = outCache[4], rr = outCache[5];

        int leftId = -1, rightId = -1;

        char curDataClass[MAX_LEAF_DATA_STR_LEN];
        if(parDataClass != nullptr)
            strcpy(curDataClass, parDataClass);
        char* dataClass = (parDataClass == nullptr) ? nullptr : curDataClass;

        //特殊处理一些操作符的系数
        if(converter_.isSymbolFun(opt)){
            //如果第一个孩子是数字,变成opt_from
            if(converter_.getOptSize(outCache, 1) < MAX_OPT_STR_LEN && converter_.isNum(converter_.readOpt(line, outCache, optCache,1))){
                if(optCache[0] == 'c'){
                    externalCoffIndex = (int)atof(optCache + 1);
                    coff = coffList_[externalCoffIndex];
                } else{
                    coff = atof(optCache);
                }

                //重新得到刚才的操作数
                opt = converter_.readOpt(line, outCache, optCache);
                //在操作符后面拼接上_from
                char* p = optCache + strlen(opt);
                strcpy(p, "_from");
                alphaElement = alphaElementMap[optCache];
                leftId = decode(line, alphaElementMap, rl, rr, outCache, optCache, dataClass);
                nodeId = createNode(alphaElement, coff, dataClass, externalCoffIndex);
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
        for(auto i = 0; i < subtreeList_.getSize(); i++){
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

        //特殊处理叶节点
        auto iter = *alphaElementMap.find(opt);
        if(iter == nullptr){
            nodeId = createNode(opt, dataClass);
            return nodeId;
        }

        alphaElement = alphaElementMap[opt];
        if(alphaElement->getCoffUnit() != CoffUnit::COFF_NONE){
            converter_.readOpt(line, outCache, optCache, alphaElement->getChildNum() + 1);
            switch (alphaElement->getCoffUnit()){
                case CoffUnit::COFF_DAY:
                case CoffUnit::COFF_CONST:
                    if(optCache[0] == 'c'){
                        externalCoffIndex = (int)atof(optCache + 1);
                        coff = coffList_[externalCoffIndex];
                    }else{
                        coff = atof(optCache);
                    }
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

        nodeId = createNode(alphaElement, coff, dataClass, externalCoffIndex);
        if(leftId != -1)
            addLeftChild(nodeId, leftId);
        if(rightId != -1)
            addRightChild(nodeId, rightId);
        return nodeId;
    }

    void encode(char* pout, int nodeId, int& curIndex, int subtreeSize){
        const char* name = getSubtreeRootName(nodeId, subtreeSize);
        //编码子树
        if(name != nullptr){
            strcpy(pout + curIndex, name);
            curIndex += strlen(name);
            return;
        }
        AlphaNode* node = &nodeList_[nodeId];
        name = node->getName();
        int nameLen = strlen(name);

        //特殊处理系数在左边的情况
        if(nameLen > 5 and strcmp(name + (nameLen-5),"_from") == 0){
            //先写名字
            memcpy(pout + curIndex, node->getName(), (nameLen - 5) * sizeof(char));
            curIndex += (nameLen - 5);
            pout[curIndex++] = '(';

            //写系数
            int coffLen = node->getCoffStr(pout + curIndex, coffList_);
            curIndex += coffLen;

            //写左孩子
            pout[curIndex++] = ',';
            pout[curIndex++] = ' ';
            encode(pout, node->leftId, curIndex, subtreeSize);

            pout[curIndex++] = ')';
        } else {
            //特殊处理rank(因为rank被拆分成两个节点)------------------------------------
            if(strcmp(name,"rank_scale") == 0){
                encode(pout, node->leftId, curIndex, subtreeSize);
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
                encode(pout, node->leftId, curIndex, subtreeSize);

                //写右孩子
                if(node->getChildNum() > 1){
                    pout[curIndex++] = ',';
                    pout[curIndex++] = ' ';
                    encode(pout, node->rightId, curIndex, subtreeSize);
                }

                //写系数
                if(node->getCoffUnit() != CoffUnit::COFF_NONE){
                    pout[curIndex++] = ',';
                    pout[curIndex++] = ' ';
                    int coffLen = node->getCoffStr(pout + curIndex, coffList_);
                    curIndex += coffLen;
                }

                pout[curIndex++] = ')';
            }
        }
    }


    const char* getSubtreeRootName(int nodeId, int subtreeSize = -1){
        if(subtreeSize == -1)
            subtreeSize = subtreeList_.getSize();
        if(nodeList_[nodeId].isRoot()){
            for(auto i = 0; i < subtreeSize; ++i){
                if(subtreeList_[i].rootId == nodeId){
                    return subtreeList_[i].name;
                }
            }
        }
        return nullptr;
    }

    int getSubtreeRootId(const char* rootName){
        for(auto i = 0; i < subtreeList_.getSize(); ++i)
            if(strcmp(subtreeList_[i].name, rootName) == 0)
                return subtreeList_[i].rootId;
        return -1;
    }

    int getSubtreeIndex(const char* rootName){
        for(auto i = 0; i < subtreeList_.getSize(); ++i)
            if(strcmp(subtreeList_[i].name, rootName) == 0)
                return i;
        return -1;
    }

    inline void addLeftChild(int nodeId, int childId){
        nodeList_[nodeId].leftId = childId;
        nodeList_[childId].preId = nodeId;
    }

    inline void addRightChild(int nodeId, int childId){
        nodeList_[nodeId].rightId = childId;
        nodeList_[childId].preId = nodeId;
    }

    inline int createNode(IAlphaElement* element, double coff = 0, const char* watchLeafDataClass = nullptr,  int externalCoffIndex = -1){
        int nodeLen = nodeList_.getSize();
        nodeList_[nodeLen].setup(element, coff, watchLeafDataClass, externalCoffIndex);
        return nodeLen;
    }

    inline int createNode(const char* name, const char* watchLeafDataClass = nullptr){
        int nodeLen = nodeList_.getSize();
        nodeList_[nodeLen].setup(name, watchLeafDataClass);
        return nodeLen;
    }


    //搜索某个节点下所有子串是否在line中出现minTime以上(包括minTime),限制节点的最小深度minDepth
    int searchPublicSubstring(int nodeId, int subtreeSize, const char* line, char* encodeCache, int& curIndex, char* pout, int minTime = 1, int minDepth = 3){
        if(getDepth(nodeId, subtreeSize) < minDepth)
            return 0;
        int encodeIndex = 0;
        encode(encodeCache, nodeId, encodeIndex, subtreeSize);
        encodeCache[encodeIndex] = 0;
        char* realSubstring = encodeCache + (encodeIndex+1);
        converter_.function2operator(encodeCache, realSubstring);

        int count = 0;
        const char* subline = strstr(line, realSubstring);
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
        if(nodeList_[nodeId].getChildNum() > 0)
            count += searchPublicSubstring(nodeList_[nodeId].leftId, subtreeSize, line, encodeCache, curIndex, pout, minTime, minDepth);
        if(nodeList_[nodeId].getChildNum() > 1)
            count += searchPublicSubstring(nodeList_[nodeId].rightId, subtreeSize, line, encodeCache, curIndex, pout, minTime, minDepth);
        return count;
    }

    int getDepth(int nodeId, int subtreeSize = -1){
        if(nodeList_[nodeId].getChildNum() == 0 || getSubtreeRootName(nodeId, subtreeSize) != nullptr)
            return 1;

        int myDepth = 1;
        if(strcmp(nodeList_[nodeId].getName(),"rank_scale") == 0){
            myDepth = 0;
        }

        int leftDepth = getDepth(nodeList_[nodeId].leftId, false);
        if(nodeList_[nodeId].getChildNum() == 1)
            return leftDepth + myDepth;
        int rightDepth = getDepth(nodeList_[nodeId].rightId, false);
        return (leftDepth > rightDepth ? leftDepth : rightDepth) + myDepth;
    }
protected:
    //某个节点的参数可以从coffList_中读取,方便最优化
    DArray<double, MAX_NODE_BLOCK> coffList_;
    DArray<AlphaNode, MAX_NODE_BLOCK> nodeList_;
    DArray<SubTreeDes, MAX_SUB_TREE_BLOCK> subtreeList_;
    DArray<AlphaProcessNode, MAX_PROCESS_BLOCK> processList_;
    static AlphaTreeConverter converter_;
};

AlphaTreeConverter BaseAlphaTree::converter_ = AlphaTreeConverter();

#endif //ALPHATREE_BASEALPHATREE_H
