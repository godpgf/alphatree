//
// Created by godpgf on 18-3-7.
//

#ifndef ALPHATREE_ALPHATRANSACTION_H
#define ALPHATREE_ALPHATRANSACTION_H
#include "dcache.h"
#include "alphabacktrace.h"
#include <iostream>

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

    //todo test
    float cur_return = 1;
    float last_price = 1;
    //-1空，1多，0平
    int state = 0;

    void buyStock(int tid, int stockIndex, float price, float weight){
        //allTransaction_->getCacheMemory(tid).buyStock(stockIndex, price, weight);
        last_price = price;
        state = 1;
    }

    void shortStock(int tid, int stockIndex, float price, float weight){
        //allTransaction_->getCacheMemory(tid).shortStock(stockIndex, price, weight);
        last_price = price;
        state = -1;
    }

    void sellStock(int tid, int stockIndex, float price, float weight, bool isSellBuy){
        //allTransaction_->getCacheMemory(tid).sellStock(stockIndex, price, weight, isSellBuy);
        if(isSellBuy){
            cur_return *= ((price - last_price) / last_price - allTransaction_->getCacheMemory(tid).fee + 1.f);
        } else {
            cur_return *= ((last_price - price) / price - allTransaction_->getCacheMemory(tid).fee + 1.f);
        }
        state = 0;
    }

    float getBalance(int tid, float* price){
        //cout<<cur_return<<endl;
        //return allTransaction_->getCacheMemory(tid).getBalance(price);
        if(state == 1){
            return 100000 * cur_return * ((price[0] - last_price) / last_price - allTransaction_->getCacheMemory(tid).fee + 1.f);
        } else if(state = -1){
            return 100000 * cur_return * ((last_price - price[0]) / price[0] - allTransaction_->getCacheMemory(tid).fee + 1.f);
        } else {
            return cur_return * 100000;
        }
    }
protected:
    //用来记录交易，方便回测
    DCache<Transaction> *allTransaction_ = {nullptr};
    static AlphaTransaction* alphaTransaction_;
};

AlphaTransaction *AlphaTransaction::alphaTransaction_ = nullptr;
#endif //ALPHATREE_ALPHATRANSACTION_H
