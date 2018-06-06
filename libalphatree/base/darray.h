//
// Created by yanyu on 2017/9/24.
//

#ifndef ALPHATREE_DARRAY_H
#define ALPHATREE_DARRAY_H
#include <stdlib.h>
#include <string.h>
#include <mutex>
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
            std::lock_guard<std::mutex> lock{mutex_};
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

        int add(T value){
            int id = getSize();
            //setValue(id, value);
            (*this)[id] = value;
            return id;
        }


        int getSize(){
            return size_;
        }

    protected:
        size_t size_{0};
        std::mutex mutex_;

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
