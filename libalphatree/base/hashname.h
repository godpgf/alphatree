//
// Created by yanyu on 2017/9/24.
// 字符串到整数的互相映射,只读时线程安全
//

#ifndef ALPHATREE_HASHNAME_H
#define ALPHATREE_HASHNAME_H
#include <string>
#include "darray.h"

template<int HASH_TABLE_LENGTH = 2048, int HASH_NAME_BLOCK_SIZE = 4096>
class HashName{
    public:
        HashName(){
            for(auto i = 0; i < HASH_TABLE_LENGTH; ++i)
                hashNameNodes_[i] = nullptr;
        }

        ~HashName(){
            clear();
        }

        class HashNameNode{
            public:
                int id{-1};
                HashNameNode* next{nullptr};
        };

        void clear(){
            //cout<<"clear\n";
            for(auto i = 0; i < HASH_TABLE_LENGTH; ++i)
                if(hashNameNodes_[i] != nullptr){
                    destroyHashNameNode(hashNameNodes_[i]);
                    hashNameNodes_[i] = nullptr;
                }

            for(auto i = 0; i < nameTable_.getSize(); ++i)
            {
                delete []nameTable_[i];
            }
            nameTable_.clear();
        }

        const char* toName(int id){
            return nameTable_[id];
        }

        int toId(const char* name){
            return generateId(name);
        }

        int getSize(){
            return nameTable_.getSize();
        }

        HashNameNode** find(const char* name){
            int hashCode = BKDRHash(name) % HASH_TABLE_LENGTH;
            if(hashCode < 0)
                hashCode = HASH_TABLE_LENGTH + hashCode;
            if(hashNameNodes_[hashCode] == nullptr || strcmp(nameTable_[hashNameNodes_[hashCode]->id], name) == 0){
                return &hashNameNodes_[hashCode];
            }
            HashNameNode* curNode = hashNameNodes_[hashCode];

            do{
                if(curNode->next != nullptr){
                    if(strcmp(nameTable_[curNode->next->id], name) == 0){
                        return &curNode->next;
                    }
                    curNode = curNode->next;
                }else{
                    return &curNode->next;
                }
            }while (curNode != nullptr);
            return nullptr;
        }

    protected:

        int generateId(const char* name){
            HashNameNode** pHashNameNode = find(name);
            if(*pHashNameNode == nullptr){
                (*pHashNameNode) = new HashNameNode();
                (*pHashNameNode)->id = createNewId(name);
            }
            return (*pHashNameNode)->id;
        }

        int createNewId(const char* name){
            char* newName = new char[strlen(name)+1];
            strcpy(newName, name);
            return nameTable_.add(newName);
        }

        static int BKDRHash(const char* name){
            int len = strlen(name);
            int hash = 0;
            int seed = 131;
            for(auto i = 0; i < len; ++i){
                hash = hash * seed + name[i];
            }
            return (hash & 0x7FFFFFFF);
        }

        static void destroyHashNameNode(HashNameNode* node){
            if(node->next != nullptr)
                destroyHashNameNode(node->next);
            delete node;
        }

        //字符串到id映射
        HashNameNode* hashNameNodes_[HASH_TABLE_LENGTH];
        //id到字符串映射
        DArray<char*,HASH_NAME_BLOCK_SIZE> nameTable_;
};

#endif //ALPHATREE_HASHNAME_H
