//
// Created by godpgf on 18-3-7.
//

#ifndef ALPHATREE_ALPHATRANSACTION_H
#define ALPHATREE_ALPHATRANSACTION_H
#include "base/dcache.h"
#include "alpha/alphabacktrace.h"

class AlphaTransaction {
public:
    static void initialize() {
        alphaTransaction_ = new AlphaTransaction();
    }

    static void release() {
        if (alphaTransaction_)
            delete alphaTransaction_;
        alphaTransaction_ = nullptr;
    }

    static AlphaTransaction *getAlphatransaction() { return alphaTransaction_; }

    AlphaTransaction(){
        allTransaction_ = DCache<Transaction>::create();
    }

    virtual ~AlphaTransaction(){
        DCache<Transaction>::release(allTransaction_);
    }

    int useTransaction(float remain, float fee){
        int tid = allTransaction_->useCacheMemory();
        allTransaction_->getCacheMemory(tid).init(remain, fee);
        return tid;
    }

    void releaseTransaction(int tid){
        allTransaction_->releaseCacheMemory(tid);
    }

    void buyStock(int tid, int stockIndex, float price, float weight){
        allTransaction_->getCacheMemory(tid).buyStock(stockIndex, price, weight);
    }

    void shortStock(int tid, int stockIndex, float price, float weight){
        allTransaction_->getCacheMemory(tid).shortStock(stockIndex, price, weight);
    }

    void sellStock(int tid, int stockIndex, float price, float weight, bool isSellBuy){
        allTransaction_->getCacheMemory(tid).sellStock(stockIndex, price, weight, isSellBuy);
    }

    float getBalance(int tid, float* price){
        return allTransaction_->getCacheMemory(tid).getBalance(price);
    }
protected:
    //用来记录交易，方便回测
    DCache<Transaction> *allTransaction_ = {nullptr};
    static AlphaTransaction* alphaTransaction_;
};

AlphaTransaction *AlphaTransaction::alphaTransaction_ = nullptr;
#endif //ALPHATREE_ALPHATRANSACTION_H
