//
// Created by yanyu on 2017/9/24.
//

#ifndef ALPHATREE_DARRAY_H
#define ALPHATREE_DARRAY_H
#include <stdlib.h>
#include <string.h>
using namespace std;

#define DARRAY_HASH_TABLE_LEN 512

template<class T,int BLOCK_SIZE>
class DArray{
    protected:
        struct DArrayBlock{
            DArrayBlock(int blockId){this->blockId = blockId;}
            T dataBlock[BLOCK_SIZE];
            int blockId;
            DArrayBlock* next{nullptr};
        };
    public:
        DArray(){
            for(auto i = 0; i < DARRAY_HASH_TABLE_LEN; ++i){
                blockHashTable[i] = nullptr;
            }
        }

        ~DArray(){
            clear();
        }

        void clear(){
            for(auto i = 0; i < DARRAY_HASH_TABLE_LEN; ++i){
                if(blockHashTable[i] != nullptr){
                    destroyBlock(blockHashTable[i]);
                    blockHashTable[i] = nullptr;
                }
            }
            size_ = 0;
        }

        void resize(int size){size_ = size;}

        T& operator[](size_t index){
            int blockId = getBlockId(index);
            int hashBlockId = getBlockHashId(blockId);
            size_ = size_ > index + 1 ? size_ : index + 1;
            if(blockHashTable[hashBlockId] == nullptr){
                blockHashTable[hashBlockId] = new DArrayBlock(blockId);
                return blockHashTable[hashBlockId]->dataBlock[getOffsetId(index)];
            } else{
                DArrayBlock* curBlock = blockHashTable[hashBlockId];
                if(curBlock->blockId != blockId){
                    while(curBlock->next != nullptr){
                        if(curBlock->next->blockId == blockId){
                            return curBlock->next->dataBlock[getOffsetId(index)];
                        } else {
                            curBlock = curBlock->next;
                        }
                    }
                    curBlock->next = new DArrayBlock(blockId);
                    return curBlock->next->dataBlock[getOffsetId(index)];
                }

                return curBlock->dataBlock[getOffsetId(index)];
            }
        }

        /*bool getValue(int index, T& value){
            int blockId = getBlockId(index);
            int hashBlockId = getBlockHashId(blockId);
            DArrayBlock* curBlock = blockHashTable[hashBlockId];
            while(curBlock != nullptr){
                if(curBlock->blockId == blockId){
                    value = curBlock->dataBlock[getOffsetId(index)];
                    return true;
                }
            }
            return false;
        }*/

        int add(T value){
            int id = getSize();
            //setValue(id, value);
            (*this)[id] = value;
            return id;
        }

        /*void setValue(int index, T value){
            int blockId = getBlockId(index);
            int hashBlockId = getBlockHashId(blockId);
            if(blockHashTable[hashBlockId] == nullptr){
                blockHashTable[hashBlockId] = new DArrayBlock(blockId);
                blockHashTable[hashBlockId]->dataBlock[getOffsetId(index)] = value;
            } else{
                DArrayBlock* curBlock = blockHashTable[hashBlockId];
                while(curBlock->next != nullptr){
                    if(curBlock->next->blockId == blockId){
                        curBlock->next->dataBlock[getOffsetId(index)] = value;
                        return;
                    } else {
                        curBlock = curBlock->next;
                    }
                }
                curBlock->next = new DArrayBlock(blockId);
                curBlock->next->dataBlock[getOffsetId(index)] = value;
            }
            size_ = max(size_, index + 1);
        }*/

        int getSize(){
            return size_;
        }

    protected:
        size_t size_{0};

        inline void destroyBlock(DArrayBlock* block){
            if(block->next != nullptr)
                destroyBlock(block->next);
            delete block;
        }

        inline static int getOffsetId(int id){
            return id % BLOCK_SIZE;
        }

        inline static int getBlockId(int id){
            return id / BLOCK_SIZE;
        }

        inline static int getBlockHashId(int blockId){
            return blockId % DARRAY_HASH_TABLE_LEN;
        }

        DArrayBlock* blockHashTable[DARRAY_HASH_TABLE_LEN];
};

#endif //ALPHATREE_DARRAY_H
