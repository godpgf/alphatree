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
    static DTree<T, DTREE_BLOCK_SIZE>* create(){ return new DTree<T, DTREE_BLOCK_SIZE>();}
    static void release(DTree<T, DTREE_BLOCK_SIZE> * tree){delete tree;}
    int createNode(){
        return useNodeMemory();
    }

    void cleanNode(int nodeId){
        //删除所有孩子
        std::unique_lock<std::mutex> lock{nodeMemoryBuf_[nodeId].nodeMutex};
        int childId = nodeMemoryBuf_[nodeId].firstChildId;
        while (childId >= 0){
            cleanNode(childId);
            childId = nodeMemoryBuf_[childId].nextBrotherId;
        }

        //从新排列兄弟
        if(nodeMemoryBuf_[nodeId].preBrotherId == -1){
            nodeMemoryBuf_[nodeMemoryBuf_[nodeId].preId].firstChildId = nodeMemoryBuf_[nodeId].nextBrotherId;
            if(nodeMemoryBuf_[nodeId].nextBrotherId != -1)
                nodeMemoryBuf_[nodeMemoryBuf_[nodeId].nextBrotherId].preBrotherId = -1;
        } else {
            nodeMemoryBuf_[nodeMemoryBuf_[nodeId].preBrotherId].nextBrotherId = nodeMemoryBuf_[nodeId].nextBrotherId;
            if(nodeMemoryBuf_[nodeId].nextBrotherId != -1)
                nodeMemoryBuf_[nodeMemoryBuf_[nodeId].nextBrotherId].preBrotherId = nodeMemoryBuf_[nodeId].preBrotherId;
        }
        releaseNodeMemory(nodeId);
    }

    /*void freeNode(int nodeId){
        for(int i = 0; i < getChildNum(nodeId); ++i){
            int childId = getChild(nodeId, i);
            nodeMemoryBuf_[childId].preId = -1;
        }
        nodeMemoryBuf_[nodeId].childNum = 0;
        releaseNodeMemory(nodeId);
    }*/

    void releaseAll(){
        std::unique_lock<std::mutex> lock{mutex_};
        nodeMemoryBuf_.resize(0);
        freeNodeMemoryIds_.resize(0);
        curFreeNodeMemoryId_ = 0;
    }

    bool lock(int nodeId){
        return nodeMemoryBuf_[nodeId].lock();
    }

    void unlock(int nodeId){
        nodeMemoryBuf_[nodeId].unlock();
    }

    void addChild(int rootId, int childId){
        if(rootId == childId)
            cout<<"ffffff\n";
        std::unique_lock<std::mutex> lock{nodeMemoryBuf_[rootId].nodeMutex};
        nodeMemoryBuf_[childId].preId = rootId;
        if(nodeMemoryBuf_[rootId].firstChildId == -1){
            nodeMemoryBuf_[rootId].firstChildId = childId;
        }else{
            int preChildId = nodeMemoryBuf_[rootId].firstChildId;
            while (nodeMemoryBuf_[preChildId].nextBrotherId != -1){
                preChildId = nodeMemoryBuf_[preChildId].nextBrotherId;
            }
            nodeMemoryBuf_[preChildId].nextBrotherId = childId;
            nodeMemoryBuf_[childId].preBrotherId = preChildId;
        }
    }

    int getNodeNum(int rootId){
        int nodeNum = 1;

        int childId = nodeMemoryBuf_[rootId].firstChildId;
        while (childId >= 0){
            nodeNum += getNodeNum(childId);
            childId = nodeMemoryBuf_[childId].nextBrotherId;
        }

        return nodeNum;
    }

    int getChildNum(int rootId){
        int childId = nodeMemoryBuf_[rootId].firstChildId;
        int childNum = 0;
        while (childId >= 0){
            ++childNum;
            childId = nodeMemoryBuf_[childId].nextBrotherId;
        }
        return childNum;
    }
    int getChild(int rootId, int childIndex){
        //注意，读的时候不加锁，这就必须保证加入孩子之前，孩子身体上所有结构数据必须是完好的，不能加入孩子后再去改孩子的结构
        int childId = nodeMemoryBuf_[rootId].firstChildId;
        while (childIndex-- > 0){
            childId = nodeMemoryBuf_[childId].nextBrotherId;
        }
        return childId;
    }
    int getParent(int rootId){ return nodeMemoryBuf_[rootId].preId;}
    int getRoot(int nodeId){
        while (getParent(nodeId) >= 0)
            nodeId = getParent(nodeId);
        return nodeId;
    }
    void removeChild(int rootId, int childIndex){
        std::unique_lock<std::mutex> lock{nodeMemoryBuf_[rootId].nodeMutex};
        cleanNode(getChild(rootId, childIndex));
    }


    T& operator[](int nodeIndex){
        return nodeMemoryBuf_[nodeIndex].data;
    }

    int getSize(){
        return nodeMemoryBuf_.getSize();
    }
protected:
    class DTreeNode{
    public:
        int preId = {-1};
        int firstChildId = {-1};
        int preBrotherId = {-1};
        int nextBrotherId = {-1};
        void clean(){
            preId = -1;
            firstChildId = -1;
            preBrotherId = -1;
            nextBrotherId = -1;
        }
        T data;

        bool lock(){
            std::unique_lock<std::mutex> lock{mutex_};
            if(isLock_ == false){
                isLock_ = true;
                return true;
            }
            return false;
        }
        void unlock(){
            std::unique_lock<std::mutex> lock{mutex_};
            isLock_ = false;
        }
        std::mutex nodeMutex;
    protected:
        //线程安全
        std::mutex mutex_;
        bool isLock_ = {false};
    };

    int useNodeMemory(){
        //通过互斥遍历保证以下语句一个时间只有一个线程在执行,这时会mutex_.lock()
        std::unique_lock<std::mutex> lock{mutex_};
        if(curFreeNodeMemoryId_ == nodeMemoryBuf_.getSize()){
            nodeMemoryBuf_[curFreeNodeMemoryId_];
            freeNodeMemoryIds_.add(curFreeNodeMemoryId_);
        }
        int freeId = freeNodeMemoryIds_[curFreeNodeMemoryId_++];
        nodeMemoryBuf_[freeId].clean();
        return freeId;
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
