//
// Created by godpgf on 18-3-3.
//

#ifndef ALPHATREE_ALPHABACKTRACE_H
#define ALPHATREE_ALPHABACKTRACE_H

#include "../base/darray.h"
#include <cstddef>
#include <algorithm>
#include <utility>
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
    //long dayIndex;
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
        fee = fee;
    }

    //买入股票,weight表示以余额的多少买入，fee表示手续费
    void buyStock(int stockIndex, float price, float weight){
        int tid = transactionsBuy_.getSize();
        int holdId = getHoldId(stockIndex, transactionsBuy_, holdBuyStock_, holdBuySize_);
        long preId = -1;
        if(holdId >= 0){
            preId = holdBuyStock_[holdId];
            holdBuyStock_[holdId] = tid;
            transactionsBuy_[preId].nextTransactionIndex = tid;
        } else {
            holdBuyStock_[holdBuySize_++] = tid;
        }
        transactionsBuy_[tid].preTransactionIndex = preId;
        transactionsBuy_[tid].nextTransactionIndex = -1;
        //transactionsBuy_[tid].dayIndex = dayIndex;
        transactionsBuy_[tid].stockIndex = stockIndex;
        transactionsBuy_[tid].price = price;
        transactionsBuy_[tid].isSell = false;
        float money = getCanUseMoney() * weight * (1 - fee);
        transactionsBuy_[tid].stockNum = (int)(money / price);
        remain_ -= (transactionsBuy_[tid].stockNum * price) * (1 + fee);
        //cout<<"买多"<<transactionsBuy_[tid].stockNum<<"价格"<<price<<endl;
    }

    //做空
    void shortStock(int stockIndex, float price, float weight){
        int tid = transactionsShort_.getSize();
        int holdId = getHoldId(stockIndex, transactionsShort_, holdShortStock_, holdShortSize_);
        long preId = -1;
        if(holdId >= 0){
            preId = holdShortStock_[holdId];
            holdShortStock_[holdId] = tid;
            transactionsBuy_[preId].nextTransactionIndex = tid;
        } else {
            holdShortStock_[holdShortSize_++] = tid;
        }
        transactionsShort_[tid].preTransactionIndex = preId;
        transactionsShort_[tid].nextTransactionIndex = -1;
        //transactionsShort_[tid].dayIndex = dayIndex;
        transactionsShort_[tid].stockIndex = stockIndex;
        transactionsShort_[tid].price = price;
        transactionsShort_[tid].isSell = false;
        float money = getCanUseMoney() * weight * (1 - fee);
        transactionsShort_[tid].stockNum = (int)(money / price);
        //把股票借来卖掉所产生的交易费用
        remain_ -= (transactionsShort_[tid].stockNum * price) * (fee);
        //卖出借来的股票需要支付的押金，付出押金得到的抵押物就是股票
        reserve_ += (transactionsShort_[tid].stockNum * price);
        //cout<<"买空"<<transactionsShort_[tid].stockNum<<" 价格"<<price<<" 保证金"<<reserve_<<" 交易号"<<tid<<endl;
    }

    //平仓,weight表示卖出的比例，isSellBuy表示卖出的是买入的股票（不是做空的股票）
    void sellStock(int stockIndex, float price, float weight, bool isSellBuy){
        auto& transactions = getTransaction(isSellBuy);
        long* holdStock = getHoldStock(isSellBuy);
        int& holdSize = getHoldSize(isSellBuy);
        int holdId = getHoldId(stockIndex, transactions, holdStock , holdSize);
        //当前要卖出的股票的最后订单id
        long lastIndex = holdStock[holdId];
        int stockNum = getRemainStockNum(transactions, lastIndex);

        auto deltaMoney = getSellReward((int)(stockNum * weight), price, transactions, lastIndex, isSellBuy);
        remain_ += deltaMoney.first;
        reserve_ += deltaMoney.second;


        int tId = transactions.getSize();
        transactions[tId].isSell = true;
        transactions[tId].price = price;
        transactions[tId].stockNum = (int)(weight * stockNum);
        transactions[tId].preTransactionIndex = lastIndex;
        transactions[tId].nextTransactionIndex = -1;
        transactions[lastIndex].nextTransactionIndex = tId;
        //transactions[tId].dayIndex = dayIndex;
        transactions[tId].stockIndex = stockIndex;
        if(transactions[tId].stockNum == stockNum){
            for(int i = holdId; i < holdSize - 1; ++i)
                holdStock[i] = holdStock[i+1];
            --holdSize;
        } else {
            holdStock[holdId] = tId;
        }
        //cout<<"平仓"<<transactions[tId].stockNum<<" 价格"<<price<<" 平仓后保证金"<<reserve_<<endl;
    }

    //得到账户余额
    float getBalance(float* price){
        float remain = remain_;
        float reserve = reserve_;

        for(int i = 0; i < holdBuySize_; ++i){
            int lastIndex = holdBuyStock_[i];
            int stockIndex = transactionsBuy_[lastIndex].stockIndex;
            float p = price[stockIndex];
            int stockNum = getRemainStockNum(transactionsBuy_, lastIndex);
            auto deltaMoney = getSellReward(stockNum, p, transactionsBuy_, lastIndex, true);
            remain += deltaMoney.first;
            reserve += deltaMoney.second;
        }

        for(int i = 0; i < holdShortSize_; ++i){
            int lastIndex = holdShortStock_[i];
            int stockIndex = transactionsShort_[lastIndex].stockIndex;
            float p = price[stockIndex];
            int stockNum = getRemainStockNum(transactionsShort_, lastIndex);
            //cout<<"remain stock "<<stockNum<<endl;
            auto deltaMoney = getSellReward(stockNum, p, transactionsShort_, lastIndex, false);
            remain += deltaMoney.first;
            reserve += deltaMoney.second;
        }

        if(reserve > 0.0001){
            cout<<"卖光股票怎么押金还有"<<reserve<<"\n";
            throw "error";
        }
        return remain;
    }


protected:
    //得到之前做多或者做空的交易
    inline DArray<TransactionSign, MAX_TRANSACTION_NUM>& getTransaction(bool isBuy){
        return isBuy ? transactionsBuy_ : transactionsShort_;
    }
    //得到当前还继续持有的多仓或者空仓
    inline long* getHoldStock(bool isBuy){
        return isBuy ? holdBuyStock_ : holdShortStock_;
    }

    //得到当前多仓或者空仓的不同股票数
    inline int& getHoldSize(bool isBuy){
        return isBuy ? holdBuySize_ : holdShortSize_;
    }

    //通过股票id，得到它在持有数组中的index
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

    //得到订单中还没卖掉的股票笔数
    inline static int getRemainStockNum(DArray<TransactionSign, MAX_TRANSACTION_NUM>& transactions, long lastIndex){
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

    //计算把某只股票以某个价格卖掉部分，得到的钱和减少的保证金
    //lastTranId是这个股票最后的订单
    inline pair<float,float> getSellReward(int sellNum, float price, DArray<TransactionSign, MAX_TRANSACTION_NUM>& transactions, long lastIndex, bool isSellBuy){
        //cout<<"尝试平仓 "<<isSellBuy<<" "<<sellNum<<endl;
        if(isSellBuy){
            return pair<float, float>(sellNum * price * (1 - fee),0);
        } else {
            float reward = 0;
            float reserve = 0;
            //先要把之前做空的股票以当前价格买回来
            reward -= sellNum * price * (1 + fee);

            //找到最早的做空单据和平仓单据
            long sellIndex = transactions[lastIndex].isSell ? lastIndex : -1;
            long shortIndex = !transactions[lastIndex].isSell ? lastIndex : -1;
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
            int curSellStockNum = sellIndex == -1 ? 0 : transactions[sellIndex].stockNum;
            //当前平掉的那个交易剩余股票量
            int curShortStockNum = shortIndex == -1 ? 0 : transactions[shortIndex].stockNum;
            //cout<<curSellStockNum<<" "<<curShortStockNum<<" "<<sellIndex<<" "<<shortIndex<<endl;
            while (sellIndex != -1 ){

                if(curSellStockNum > curShortStockNum){
                    //当前平仓数大于之前的做空数，平掉之前的做空
                    curSellStockNum -= curShortStockNum;
                    //找到下一个做空单，以便剩下的平仓单继续
                    do{
                        shortIndex = transactions[shortIndex].nextTransactionIndex;
                        if(shortIndex == -1) {
                            cout<<"当前平仓的数量不可能超过之前做空的数量\n";
                            throw "数量错误";
                        }
                    }while (transactions[shortIndex].isSell);
                    curShortStockNum = transactions[shortIndex].stockNum;
                } else {
                    //把之前的做空平掉，可能还剩下一些做空单
                    curShortStockNum -= curSellStockNum;
                    //找到下一个平仓单，以便剩下的做空单继续
                    do{
                        sellIndex = transactions[sellIndex].nextTransactionIndex;
                    }while (sellIndex != -1 && !transactions[sellIndex].isSell);

                    //cout<<sellIndex<<" "<<curSellStockNum<<" "<<curShortStockNum<<endl;

                    if(sellIndex != -1){
                        curSellStockNum = transactions[sellIndex].stockNum;
                    } else {
                        //往后已经没有平单了
                        curSellStockNum = 0;
                    }
                }
            }
            if(curSellStockNum > 0){
                cout<<"不可能还有平单剩余，平单都会和之前的空单配对\n";
                throw "不可能";
            }
            //cout<<"sell num "<<sellNum<<endl;
            //把剩余的空单用新买回的股票补上，释放保证金
            while (sellNum > 0){
                float deposit = 0;
                if(sellNum > curShortStockNum){
                    sellNum -= curShortStockNum;
                    deposit = curShortStockNum * transactions[shortIndex].price;
                    //找到下一个做空单，以便剩下的平仓单继续
                    do{
                        shortIndex = transactions[shortIndex].nextTransactionIndex;
                        if(shortIndex == -1) {
                            cout<<"当前平仓的数量不可能超过之前做空的数量\n";
                            throw "数量错误";
                        }
                    }while (transactions[shortIndex].isSell);
                    curShortStockNum = transactions[shortIndex].stockNum;
                } else {
                    deposit = sellNum * transactions[shortIndex].price;
                    sellNum = 0;
                }
                reward += deposit;
                reserve -= deposit;
            }
            return pair<float, float>(reward, reserve);
        }
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
public:
    //手续费
    float fee;
};

#endif //ALPHATREE_ALPHABACKTRACE_H
