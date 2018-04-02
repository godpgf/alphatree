//
// Created by godpgf on 18-3-28.
//

#ifndef ALPHATREE_GBDT_H
#define ALPHATREE_GBDT_H

#include "gbdtbase.h"
#include "act.h"

template <int DTREE_BLOCK_SIZE = 32>
class GBDT : public BaseGBDT<DTREE_BLOCK_SIZE>{
public:
    static GBDT<DTREE_BLOCK_SIZE>* create(int threadNum, float gamma = 0.01f, float lambda = 0.01f){ return new GBDT<DTREE_BLOCK_SIZE>(threadNum, gamma, lambda);}
    static void release(GBDT<DTREE_BLOCK_SIZE> * gbdt){delete gbdt;}

    void train(IVector<IBaseIterator<float>*>* featureList, IBaseIterator<float>* weight, IBaseIterator<float>* target,
               IOBaseIterator<float>* g, IOBaseIterator<float>* h, IOBaseIterator<int>* flag, IOBaseIterator<float>* pred, const char* lossFunName = "binary:logistic",
               int barSize = 64, float minWeight = 32, int maxDepth = 16, int boostNum = 2, float boostWeightScale = 1){
        BaseGBDT< DTREE_BLOCK_SIZE>::tree_->releaseAll();
        rootIds_.clear();
        int lastRootId = -1;
        auto opt = lossFun[lossFunName];
        for(int i = 0; i < boostNum; ++i){
            //完成上次boost的预测
            BaseGBDT<DTREE_BLOCK_SIZE>::boostPred(lastRootId, lambda_, flag, pred, boostWeightScale);
            //计算g和h
            opt(g, h, target, pred);
            //创建新树
            lastRootId = BaseGBDT<DTREE_BLOCK_SIZE>::boost(featureList, weight, target, g, h, flag, barSize, gamma_, lambda_, minWeight, maxDepth);
            //cout<<"add "<<lastRootId<<endl;
            rootIds_.add(lastRootId);
        }
        //cout<<"train child "<<BaseGBDT<DTREE_BLOCK_SIZE>::tree_->getChildNum(rootIds_[0])<<endl;
    }

    void pred(IVector<IBaseIterator<float>*>* featureList, IOBaseIterator<int>* flag, IOBaseIterator<float>* pred, const char* actFunName = "binary:logistic"){
        auto act = actFun[actFunName];
        pred->initialize(0);
        for(int i = 0; i < rootIds_.getSize(); ++i){
            flag->initialize(rootIds_[i]);
            BaseGBDT<DTREE_BLOCK_SIZE>::updateAllFlag(rootIds_[i], featureList, flag);
            BaseGBDT<DTREE_BLOCK_SIZE>::boostPred(rootIds_[i], lambda_, flag, pred, 1);
        }
        act(pred);
    }



    string tostring(IVector<string>* nameList){
        string str;
        for(int i = 0; i < rootIds_.getSize(); ++i){
            str += "booster["+to_string(i)+"]:\n";
            map<int, int> nodeId;
            map<int, int> childId;
            flagTree(rootIds_[i], nodeId, childId);
            tostring(str, rootIds_[i], nodeId, childId, nameList, lambda_, 0);
        }
        return str;
    }

    void saveModel(const char* path){
        ofstream file;
        file.open(path, ios::binary);
        //1、写入树数
        int treeNum = rootIds_.getSize();
        file.write(reinterpret_cast<const char * >( &treeNum ), sizeof(int));
        for(int i = 0; i < treeNum; ++i){
            int nodeNum = BaseGBDT< DTREE_BLOCK_SIZE>::tree_->getNodeNum(rootIds_[i]);
            file.write(reinterpret_cast<const char * >( &nodeNum ), sizeof(int));
            int nodeIndex = 0;
            saveTree(file, rootIds_[i], nodeIndex);
        }
        file.close();
    }

    void loadModel(const char* path){
        BaseGBDT< DTREE_BLOCK_SIZE>::tree_->releaseAll();
        rootIds_.clear();
        ifstream file;
        file.open(path, ios::binary);
        int treeNum;
        file.read(reinterpret_cast< char* >( &treeNum ), sizeof( int ));
        int offset = 0;
        for(int i = 0; i < treeNum; ++i){
            int nodeNum;
            file.read(reinterpret_cast< char* >( &nodeNum ), sizeof( int ));

            for(int j = 0; j < nodeNum; ++j){
                int nodeId = BaseGBDT< DTREE_BLOCK_SIZE>::tree_->createNode();

                int childNum;
                file.read(reinterpret_cast< char* >( &childNum ), sizeof( int ));
                for(int k = 0; k < childNum; ++k){
                    int childIndex;
                    file.read(reinterpret_cast< char* >( &childIndex ), sizeof( int ));
                    BaseGBDT< DTREE_BLOCK_SIZE>::tree_->addChild(nodeId, offset + childIndex);
                }

                file.read(reinterpret_cast< char* >( &(*BaseGBDT<DTREE_BLOCK_SIZE>::tree_)[nodeId].featureIndex ), sizeof( int ));
                file.read(reinterpret_cast< char* >( &(*BaseGBDT<DTREE_BLOCK_SIZE>::tree_)[nodeId].weight ), sizeof( float ));
                file.read(reinterpret_cast< char* >( &(*BaseGBDT<DTREE_BLOCK_SIZE>::tree_)[nodeId].split ), sizeof( float ));
                file.read(reinterpret_cast< char* >( &(*BaseGBDT<DTREE_BLOCK_SIZE>::tree_)[nodeId].g ), sizeof( float ));
                file.read(reinterpret_cast< char* >( &(*BaseGBDT<DTREE_BLOCK_SIZE>::tree_)[nodeId].h ), sizeof( float ));

                if(j == nodeNum - 1){
                    rootIds_[i] = nodeId;
                }
            }
            offset += nodeNum;
        }
        file.close();
    }
protected:
    GBDT(int threadNum, float gamma = 0.01f, float lambda = 0.01f):BaseGBDT<DTREE_BLOCK_SIZE>(threadNum), gamma_(gamma), lambda_(lambda){
        if(lossFun.getSize() == 0){
            //加入损失函数
            lossFun.add("binary:logistic", binaryLogisticLoss);
            lossFun.add("regression", regressionLoss);
            actFun.add("binary:logistic", binaryLogisticAct);
            actFun.add("regression", regressionAct);
        }
        ++objNum_;
    }

    virtual ~GBDT(){
        --objNum_;
        if(objNum_ == 0){
            lossFun.clear();
            actFun.clear();
        }
    }

    void flagTree(int rootId, map<int,int>& nodeFlag, map<int,int>& childFlag){
        int leafNum = 0;
        auto tree = BaseGBDT<DTREE_BLOCK_SIZE>::tree_;
        Vector<int> leafVector("leaf", tree->getSize());
        leafVector[leafNum++] = rootId;
        nodeFlag[rootId] = 0;
        int lineNum = 0;
        while (leafNum > 0){
            int newLeafNum = 0;
            for(int i = 0; i < leafNum; ++i){
                int lId = -1;
                int rId = -1;
                for(int j = 0; j < tree->getChildNum(leafVector[i]); ++j){
                    int nodeId = tree->getChild(leafVector[i], j);
                    if(j == 0)
                        lId = nodeId;
                    else
                        rId = nodeId;
                }
                if(lId > 0){
                    leafVector[leafNum + newLeafNum] = lId;
                    nodeFlag[lId] = lineNum + 1;
                    ++newLeafNum;
                }
                if(rId > 0){
                    leafVector[leafNum + newLeafNum] = rId;
                    nodeFlag[rId] = lineNum + 2;
                    ++newLeafNum;
                }
                childFlag[leafVector[i]] = lineNum + 1;
                lineNum += 2;
            }
            for(int i = 0; i < newLeafNum; ++i)
                leafVector[i] = leafVector[i+leafNum];
            leafNum = newLeafNum;
        }
    }

    inline static void toLeafString(string& str, int lineId, float weight, int depth){
        for(int i = 0; i < depth; ++i)
            str += '\t';
        str += to_string(lineId) + ":leaf=" + to_string(weight) + "\n";
    }

    void tostring(string& str, int rootId, map<int,int>& nodeId, map<int,int>& childId, IVector<string>* nameList, float lambda, int depth){
        auto tree = BaseGBDT<DTREE_BLOCK_SIZE>::tree_;
        for(int i = 0; i < depth; ++i)
            str += '\t';
        str += to_string(nodeId[rootId]) + ":[" + (*nameList)[(*tree)[rootId].featureIndex] + "<" + to_string((*tree)[rootId].split) + "] ";
        str += "yes=" + to_string(childId[rootId]) + ",no=" + to_string(childId[rootId] + 1) + "\n";
        int lId = -1;
        int rId = -1;
        for(int i = 0; i < tree->getChildNum(rootId); ++i){
            int nodeId = tree->getChild(rootId, i);
            if(i == 0)
                lId = nodeId;
            else
                rId = nodeId;
        }
        if(tree->getChildNum(lId) > 0)
            tostring(str, lId, nodeId, childId, nameList, lambda, depth+1);
        else
            toLeafString(str, childId[rootId], (*tree)[lId].getWeight(lambda_), depth + 1);
        if(tree->getChildNum(rId) > 0)
            tostring(str, rId, nodeId, childId, nameList, lambda, depth+1);
        else
            toLeafString(str, childId[rootId]+1,(*tree)[rId].getWeight(lambda_),depth+1);

    }

    int saveTree(ofstream& file, int rootId, int& nodeIndex){
        int childNum = BaseGBDT< DTREE_BLOCK_SIZE>::tree_->getChildNum(rootId);
        int* children = new int[BaseGBDT< DTREE_BLOCK_SIZE>::tree_->getMaxChildNum()];
        for(int i = 0; i < childNum; ++i){
            children[i] = saveTree(file, BaseGBDT< DTREE_BLOCK_SIZE>::tree_->getChild(rootId, i), nodeIndex);
        }


        //写入节点
        file.write(reinterpret_cast<const char * >( &childNum ), sizeof(int));
        for(int i = 0; i < childNum; ++i){
            file.write(reinterpret_cast<const char * >( &children[i] ), sizeof(int));
        }
        delete[] children;

        file.write(reinterpret_cast<const char * >( &(*BaseGBDT< DTREE_BLOCK_SIZE>::tree_)[rootId].featureIndex), sizeof(int));
        file.write(reinterpret_cast<const char * >( &(*BaseGBDT< DTREE_BLOCK_SIZE>::tree_)[rootId].weight), sizeof(float));
        file.write(reinterpret_cast<const char * >( &(*BaseGBDT< DTREE_BLOCK_SIZE>::tree_)[rootId].split), sizeof(float));
        file.write(reinterpret_cast<const char * >( &(*BaseGBDT< DTREE_BLOCK_SIZE>::tree_)[rootId].g), sizeof(float));
        file.write(reinterpret_cast<const char * >( &(*BaseGBDT< DTREE_BLOCK_SIZE>::tree_)[rootId].h), sizeof(float));

        return nodeIndex++;
    }

    DArray<int, 4> rootIds_;

    float gamma_, lambda_;

    static HashMap<void(*)(OBaseIterator<float>*, OBaseIterator<float>*, IBaseIterator<float>*, IBaseIterator<float>*), 8, 16> lossFun;
    static HashMap<void(*)(IOBaseIterator<float>*), 8, 16> actFun;
    static int objNum_;
};

template <int DTREE_BLOCK_SIZE>
HashMap<void(*)(OBaseIterator<float>*, OBaseIterator<float>*, IBaseIterator<float>*, IBaseIterator<float>*), 8, 16> GBDT<DTREE_BLOCK_SIZE>::lossFun;

template <int DTREE_BLOCK_SIZE>
HashMap<void(*)(IOBaseIterator<float>*), 8, 16> GBDT<DTREE_BLOCK_SIZE>::actFun;

template <int DTREE_BLOCK_SIZE>
int GBDT<DTREE_BLOCK_SIZE>::objNum_ = 0;

#endif //ALPHATREE_GBDT_H
