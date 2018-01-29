//
// Created by godpgf on 18-1-23.
//

#ifndef ALPHATREE_DTREE_H
#define ALPHATREE_DTREE_H
#include <mutex>
#include <thread>
#include "darray.h"

template <class T, int MAX_CHILD_NUM = 32, int DTREE_BLOCK_SIZE = 32>
class DTree{
public:
    static DTree<T, MAX_CHILD_NUM, DTREE_BLOCK_SIZE>* create(){ return new DTree<T, MAX_CHILD_NUM, DTREE_BLOCK_SIZE>();}
    static void release(DTree<T, MAX_CHILD_NUM, DTREE_BLOCK_SIZE> * tree){delete tree;}
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



    void releaseAll(){
        std::unique_lock<std::mutex> lock{mutex_};
        nodeMemoryBuf_.resize(0);
        freeNodeMemoryIds_.resize(0);
        curFreeNodeMemoryId_ = 0;
    }



    bool addChild(int rootId, int childId){
        std::lock_guard<std::mutex> lock{mutex_};
        int childNum = nodeMemoryBuf_[rootId].childNum++;
        if(childNum == MAX_CHILD_NUM)
            return false;
        nodeMemoryBuf_[rootId].children[childNum] = childId;
        nodeMemoryBuf_[childId].preId = rootId;
        return true;
    }

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
protected:
    struct DTreeNode{
        int childNum;
        int children[MAX_CHILD_NUM];
        int preId = {-1};
        T data;
    };

    int useNodeMemory(){
        //通过互斥遍历保证以下语句一个时间只有一个线程在执行,这时会mutex_.lock()
        std::unique_lock<std::mutex> lock{mutex_};
        if(curFreeNodeMemoryId_ == nodeMemoryBuf_.getSize()){
            nodeMemoryBuf_[curFreeNodeMemoryId_];
            freeNodeMemoryIds_.add(curFreeNodeMemoryId_);
        }
        return freeNodeMemoryIds_[curFreeNodeMemoryId_++];
    }

    void releaseNodeMemory(int id){
        std::lock_guard<std::mutex> lock{mutex_};
        freeNodeMemoryIds_[--curFreeNodeMemoryId_] = id;
    }

    DTree(){ }

    ~DTree(){
        nodeMemoryBuf_.clear();
        freeNodeMemoryIds_.clear();
    }

    //中间结果的内存空间
    DArray<DTreeNode, DTREE_BLOCK_SIZE> nodeMemoryBuf_;
    DArray<int, DTREE_BLOCK_SIZE> freeNodeMemoryIds_;
    //curFreeNodeMemoryId_以后的数据都是闲置的,以前的数据是用过的(没意义)
    int curFreeNodeMemoryId_ = {0};


    //线程安全
    std::mutex mutex_;
};


#endif //ALPHATREE_DTREE_H
