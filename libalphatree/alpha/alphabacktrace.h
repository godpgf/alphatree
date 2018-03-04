//
// Created by godpgf on 18-3-3.
//

#ifndef ALPHATREE_ALPHABACKTRACE_H
#define ALPHATREE_ALPHABACKTRACE_H

#include "../base/darray.h"
#include <cstddef>
#include <algorithm>
#include <float.h>

#define MAX_HOLD_STOCK_NUM 1024
#define MAX_TRANSACTION_NUM 1024

struct TransactionSign{
    //是否是平仓
    bool isSell;
    //交易数量
    int stockNum;
    //交易价格
    float price;
    int stockIndex;
    long dayIndex;
    //这只股票上次交易信息索引
    long preTransactionIndex;
    //下次交易相信索引
    long nextTransactionIndex;
};

class Transaction{
public:
    void init(float remain, float fee){
        remain_ = remain;
        reserve_ = 0;
        startMoney_ = remain;
        transactionsBuy_.resize(0);
        transactionsShort_.resize(0);
        holdBuySize_ = 0;
        holdShortSize_ = 0;
        fee_ = fee;
    }

    //买入股票,weight表示以余额的多少买入，fee表示手续费
    void buyStock(int stockIndex, long dayIndex, float price, float weight){
        int tid = transactionsBuy_.getSize();
        int holdId = getHoldId(stockIndex, transactionsBuy_, holdBuyStock_, holdBuySize_);
        long preId = -1;
        if(holdId >= 0){
            preId = holdBuyStock_[holdId];
            holdBuyStock_[holdId] = tid;
            transactionsBuy_[preId].nextTransactionIndex = tid;
        }
        transactionsBuy_[tid].preTransactionIndex = preId;
        transactionsBuy_[tid].nextTransactionIndex = -1;
        transactionsBuy_[tid].dayIndex = dayIndex;
        transactionsBuy_[tid].stockIndex = stockIndex;
        transactionsBuy_[tid].price = price;
        transactionsBuy_[tid].isSell = false;
        float money = getCanUseMoney() * weight * (1 - fee_);
        transactionsBuy_[tid].stockNum = (int)(money / price);
        remain_ -= (transactionsBuy_[tid].stockNum * price) * (1 + fee_);
    }

    //做空
    void shortStock(int stockIndex, long dayIndex, float price, float weight){
        int tid = transactionsShort_.getSize();
        int holdId = getHoldId(stockIndex, transactionsShort_, holdShortStock_, holdShortSize_);
        long preId = -1;
        if(holdId >= 0){
            preId = holdShortStock_[holdId];
            holdShortStock_[holdId] = tid;
            transactionsBuy_[preId].nextTransactionIndex = tid;
        }
        transactionsShort_[tid].preTransactionIndex = preId;
        transactionsShort_[tid].nextTransactionIndex = -1;
        transactionsShort_[tid].dayIndex = dayIndex;
        transactionsShort_[tid].stockIndex = stockIndex;
        transactionsShort_[tid].price = price;
        transactionsShort_[tid].isSell = false;
        float money = getCanUseMoney() * weight * (1 - fee_);
        transactionsShort_[tid].stockNum = (int)(money / price);
        //把股票借来卖掉所产生的交易费用
        remain_ -= (transactionsShort_[tid].stockNum * price) * (fee_);
        //卖出借来的股票需要支付的押金，付出押金得到的抵押物就是股票
        reserve_ += (transactionsShort_[tid].stockNum * price);
    }

    //平仓,weight表示卖出的比例，isSellBuy表示卖出的是买入的股票（不是做空的股票）
    void sellStock(int stockIndex, long dayIndex, float price, float weight, bool isSellBuy){
        DArray<TransactionSign, MAX_TRANSACTION_NUM>& transactions = isSellBuy ? transactionsBuy_ : transactionsShort_;
        int holdId = -1;
        long lastIndex = -1;
        long* holdStock = nullptr;
        int& holdSize = isSellBuy ? holdBuySize_ : holdShortSize_;
        if(isSellBuy){
            holdStock = holdBuyStock_;
        } else{
            holdStock = holdBuyStock_;
        }
        holdId = getHoldId(stockIndex, transactions, holdStock , holdSize);
        if(holdId == -1)
            return;
        lastIndex = holdBuyStock_[holdId];

        int stockNum = getStockNum(transactions, lastIndex);
        int tId = transactions.getSize();
        transactions[tId].isSell = true;
        transactions[tId].price = price;
        transactions[tId].stockNum = (int)(weight * stockNum);
        transactions[tId].preTransactionIndex = lastIndex;
        transactions[lastIndex].nextTransactionIndex = tId;
        transactions[tId].dayIndex = dayIndex;
        transactions[tId].stockIndex = stockIndex;
        if(transactions[tId].stockNum == stockNum){
            for(int i = holdId; i < holdSize - 1; ++i)
                holdStock[i] = holdStock[i+1];
            --holdSize;
        } else {
            holdStock[holdId] = tId;
        }

        //计算收益
        if(isSellBuy){
            remain_ += transactions[tId].stockNum * price * (1 - fee_);
        } else {
            //用预留的钱把股票买回来
            remain_ -= transactions[tId].stockNum * price * (1 + fee_);

            //用买回来的股票退押金
            int remainStockNum = stockNum - transactions[tId].stockNum;
            long sellIndex = lastIndex;
            long shortIndex = lastIndex;
            long preIndex = lastIndex;
            while (transactions[preIndex].preTransactionIndex >= 0){
                if(transactions[preIndex].isSell)
                    sellIndex = preIndex;
                else
                    shortIndex = preIndex;
                preIndex = transactions[preIndex].preTransactionIndex;
            }
            //最早做空的股票最早赎回,找到最早可以赎回的交易和股票数
            //当前平仓数量
            int curSellStockNum = transactions[sellIndex].stockNum;
            //当前平掉的那个交易剩余股票量
            int curShortStockNum = transactions[shortIndex].stockNum;
            while (sellIndex != lastIndex){
                if(curSellStockNum > curShortStockNum){
                    curSellStockNum -= curShortStockNum;
                    do{
                        shortIndex = transactions[shortIndex].nextTransactionIndex;
                    }while (transactions[shortIndex].isSell);
                    curShortStockNum = transactions[shortIndex].stockNum;
                } else {
                    curShortStockNum -= curSellStockNum;
                    do{
                        sellIndex = transactions[sellIndex].nextTransactionIndex;
                    }while (!transactions[sellIndex].isSell);
                    curSellStockNum = transactions[sellIndex].stockNum;
                }
            }
            while (curSellStockNum > curShortStockNum){
                curSellStockNum -= curShortStockNum;
                //返还押金
                remain_ += curShortStockNum * transactions[shortIndex].price;
                do{
                    shortIndex = transactions[shortIndex].nextTransactionIndex;
                }while (transactions[shortIndex].isSell);
                curShortStockNum = transactions[shortIndex].stockNum;
            }
            curShortStockNum -= curSellStockNum;
            //返还押金
            remain_ += curSellStockNum * transactions[shortIndex].price;
            //计算还没有平掉的剩余仓需要的预留资金，以后再用它来继续赎回股票
            reserve_ = 0;
            do{
                reserve_ = curShortStockNum * transactions[shortIndex].price;
                do{
                    shortIndex = transactions[shortIndex].nextTransactionIndex;
                }while (shortIndex != -1 && transactions[shortIndex].isSell);
                if(shortIndex != -1)
                    curShortStockNum = transactions[shortIndex].stockNum;
                else
                    curShortStockNum = 0;
            }while (curShortStockNum > 0);

        }
    }

protected:

    static int getHoldId(int stockIndex, DArray<TransactionSign, MAX_TRANSACTION_NUM>& transactions,const long* holdStock, int holdSize){
        int holdId = -1;
        for (int i = 0; i < holdSize; ++i){
            if(transactions[holdStock[i]].stockIndex == stockIndex){
                holdId = i;
                break;
            }
        }
        return holdId;
    }

    int getStockNum(DArray<TransactionSign, MAX_TRANSACTION_NUM>& transactions, long lastIndex){
        int stockNum = 0;
        while (lastIndex >= 0){
            if(transactions[lastIndex].isSell)
                stockNum -= transactions[lastIndex].stockNum;
            else
                stockNum += transactions[lastIndex].stockNum;
            lastIndex = transactions[lastIndex].preTransactionIndex;
        }
        return stockNum;
    }

    float getCanUseMoney(){
        return remain_ - reserve_;
    }

    //买入卖出单
    DArray<TransactionSign, MAX_TRANSACTION_NUM> transactionsBuy_;
    //卖出买回单
    DArray<TransactionSign, MAX_TRANSACTION_NUM> transactionsShort_;
    //记录当前所有没有卖光的股票的最后交易id
    long holdBuyStock_[MAX_HOLD_STOCK_NUM];
    int holdBuySize_ = {0};
    long holdShortStock_[MAX_HOLD_STOCK_NUM];
    int holdShortSize_ = {0};
    //开始资金
    float startMoney_;
    //账户余额
    float remain_;
    //预留资金，因为押了一部分钱去借股票又把借来的股票卖了，必须预留部分钱将来把股票再买回来
    float reserve_;
    //手续费
    float fee_;
};

#endif //ALPHATREE_ALPHABACKTRACE_H
