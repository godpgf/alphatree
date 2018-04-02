//
// Created by godpgf on 18-1-23.
//

#ifndef ALPHATREE_DTREE_H
#define ALPHATREE_DTREE_H
#include <mutex>
#include <thread>
#include "darray.h"

template <class T, int DTREE_BLOCK_SIZE = 128>
class DTree{
public:
    static DTree<T, DTREE_BLOCK_SIZE>* create(int maxChildNum){ return new DTree<T, DTREE_BLOCK_SIZE>(maxChildNum);}
    static void release(DTree<T, DTREE_BLOCK_SIZE> * tree){delete tree;}
    int createNode(){
        return useNodeMemory();

    }

    void cleanNode(int nodeId){
        for(int i = 0; i < getChildNum(nodeId); ++i){
            cleanNode(getChild(nodeId, i));
        }
        nodeMemoryBuf_[nodeId].childNum = 0;
        releaseNodeMemory(nodeId);
    }

    void freeNode(int nodeId){
        for(int i = 0; i < getChildNum(nodeId); ++i){
            int childId = getChild(nodeId, i);
            nodeMemoryBuf_[childId].preId = -1;
        }
        nodeMemoryBuf_[nodeId].childNum = 0;
        releaseNodeMemory(nodeId);
    }

    void releaseAll(){
        std::unique_lock<std::mutex> lock{mutex_};
        nodeMemoryBuf_.resize(0);
        freeNodeMemoryIds_.resize(0);
        curFreeNodeMemoryId_ = 0;
    }



    bool addChild(int rootId, int childId){
        std::lock_guard<std::mutex> lock{mutex_};
        int childNum = nodeMemoryBuf_[rootId].childNum++;
        if(childNum == maxChildNum_ ){
            cout<<"超过最大孩子数量\n";
            throw "err";
        }
        nodeMemoryBuf_[rootId].children[childNum] = childId;
        nodeMemoryBuf_[childId].preId = rootId;
        return true;
    }

    int getNodeNum(int rootId){
        int nodeNum = 1;
        for(int i = 0; i < getChildNum(rootId); ++i){
            nodeNum += getNodeNum(getChild(rootId, i));
        }
        return nodeNum;
    }

//    int getLeafNum(int rootId){
//        int leftNum = 0;
//        for(int i = 0; i < getChildNum(rootId); ++i){
//            leftNum += getLeafNum(getChild(rootId, i));
//        }
//        if(leftNum % 2 == 1)
//            ++leftNum;
//        return leftNum == 0 ? 1 : leftNum;
//    }

    int getChildNum(int rootId){ return nodeMemoryBuf_[rootId].childNum;}
    int getChild(int rootId, int childIndex){ return nodeMemoryBuf_[rootId].children[childIndex];}
    int getParent(int rootId){ return nodeMemoryBuf_[rootId].preId;}
    void removeChild(int rootId, int childIndex){

        cleanNode(getChild(rootId, childIndex));

        while (childIndex <  getChildNum(rootId) - 1){
            nodeMemoryBuf_[rootId].children[childIndex] = nodeMemoryBuf_[rootId].children[childIndex+1];
            ++childIndex;
        }

        --nodeMemoryBuf_[rootId].childNum;
    }


    T& operator[](int nodeIndex){
        return nodeMemoryBuf_[nodeIndex].data;
    }

    int getSize(){
        return nodeMemoryBuf_.getSize();
    }

    int getMaxChildNum(){ return maxChildNum_;}
protected:
    class DTreeNode{
    public:
        ~DTreeNode(){
            clean();
        }

        void initialize(int maxChildNum){
            if(maxChildNum > this->maxChildNum){
                this->maxChildNum = maxChildNum;
                clean();
                children = new int[maxChildNum];
            }
            childNum = 0;
        }

        int childNum;
        int maxChildNum = {0};
        int* children = {nullptr};
        int preId = {-1};
        T data;
    protected:
        void clean(){
            if(children)
                delete []children;
            children = nullptr;
        }
    };

    int useNodeMemory(){
        //通过互斥遍历保证以下语句一个时间只有一个线程在执行,这时会mutex_.lock()
        std::unique_lock<std::mutex> lock{mutex_};
        if(curFreeNodeMemoryId_ == nodeMemoryBuf_.getSize()){
            nodeMemoryBuf_[curFreeNodeMemoryId_];
            freeNodeMemoryIds_.add(curFreeNodeMemoryId_);
        }
        int freeId = freeNodeMemoryIds_[curFreeNodeMemoryId_++];
        nodeMemoryBuf_[freeId].initialize(maxChildNum_);
        return freeId;
    }

    void releaseNodeMemory(int id){
        std::lock_guard<std::mutex> lock{mutex_};
        freeNodeMemoryIds_[--curFreeNodeMemoryId_] = id;
    }

    DTree(int maxChildNum):maxChildNum_(maxChildNum){ }

    ~DTree(){
        nodeMemoryBuf_.clear();
        freeNodeMemoryIds_.clear();
    }

    //中间结果的内存空间
    DArray<DTreeNode, DTREE_BLOCK_SIZE> nodeMemoryBuf_;
    DArray<int, DTREE_BLOCK_SIZE> freeNodeMemoryIds_;
    //curFreeNodeMemoryId_以后的数据都是闲置的,以前的数据是用过的(没意义)
    int curFreeNodeMemoryId_ = {0};

    int maxChildNum_;

    //线程安全
    std::mutex mutex_;
};


#endif //ALPHATREE_DTREE_H
