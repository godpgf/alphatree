//
// Created by yanyu on 2017/10/9.
//

#ifndef ALPHATREE_HASHMAP_H
#define ALPHATREE_HASHMAP_H

#include <string>
#include "hashname.h"

template<class T, int HASH_TABLE_LENGTH = 2048, int HASH_NAME_BLOCK_SIZE = 4096>
class HashMap : protected HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>{
    public:
        class HashMapNode : public HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>::HashNameNode{
            public:
                T value;
        };

        void add(const char* name, T& value){
            auto** pHashNameNode = find(name);
            if(*pHashNameNode == nullptr){
                (*pHashNameNode) = new HashMapNode();
                char* newName = new char[strlen(name)+1];
                strcpy(newName, name);
                (*pHashNameNode)->id = HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>::nameTable_.add(newName);
            }
            (*pHashNameNode)->value = value;
        }

        int getSize(){
            return HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>::getSize();
        }

        T& operator[](int id){
            return (*find(HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>::nameTable_[id]))->value;
        }

        T&operator[](const char* name){
            auto** pHashNameNode = find(name);
            if(*pHashNameNode == nullptr){
                (*pHashNameNode) = new HashMapNode();
                char* newName = new char[strlen(name)+1];
                strcpy(newName, name);
                (*pHashNameNode)->id = HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>::nameTable_.add(newName);
            }
            return (*pHashNameNode)->value;
            //return ((HashMapNode*)(*find(name)))->value;
        }

        const char* toName(int id){
            return HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>::toName(id);
        }

        void clear(){
            HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>::clear();
        }

        HashMapNode** find(const char* name){
            return (HashMapNode**)HashName<HASH_TABLE_LENGTH, HASH_NAME_BLOCK_SIZE>::find(name);
        }
};

#endif //ALPHATREE_HASHMAP_H
