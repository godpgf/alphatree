//
// Created by yanyu on 2017/7/19.
//

#ifndef ALPHATREE_CONVERTER_H
#define ALPHATREE_CONVERTER_H

#include <map>
using namespace std;

class AlphaTreeConverter{
public:
    AlphaTreeConverter(){
        optMap['+'] = "add";
        optMap['-'] = "reduce";
        optMap['*'] = "mul";
        optMap['/'] = "div";
        optMap['&'] = "and";
        optMap['|'] = "or";
        optMap['~'] = "cross";
        optMap['^'] = "signed_power";
        optMap['<'] = "less";
        optMap['>'] = "more";
        optMap['?'] = "if";
        optMap[':'] = "else(";

        invOptMap["add"] = '+';
        invOptMap["reduce"] = '-';
        invOptMap["mul"] = '*';
        invOptMap["div"] = '/';
        invOptMap["and"] = '&';
        invOptMap["or"] = '|';
        invOptMap["cross"] = '~';
        invOptMap["signed_power"] = '^';
        invOptMap["less"] = '<';
        invOptMap["more"] = '>';
        invOptMap["if"] = '?';
        invOptMap["else"] = ':';
    }

    //把一些函数符号化
    char* function2operator(const char* line, char* pout){
        int r = strlen(line) - 1;
        int curIndex = 0;
        int rangeCache[10];
        function2operator(line, pout, 0, r, curIndex, rangeCache);
        pout[curIndex] = 0;
        return pout;
    }

    //把一些基础符号函数化
    char* operator2function(const char* line, char* pout){
        int i = 0;
        int curIndex = 0;
        size_t size = strlen(line);
        map<char, const char*>::iterator it;
        while(i < size){
            it=optMap.find(line[i]);
            if(it!=optMap.end() && line[i+1] == ' '){
                //找到需要替换的符号
                //找到最左边的'(',leftId是它的前一个元素,有可能是-1
                int depth = 1;
                int leftId = curIndex-1;
                while(depth != 0){
                    if(leftId < 0)
                        throw "can't find left";
                    if(pout[leftId] == '(')
                        --depth;
                    else if(pout[leftId] == ')')
                        ++depth;
                    --leftId;
                }

                //特殊处理':',目的是将三个孩子变成两个
                if(line[i] == ':'){
                    leftId -= strlen(optMap['?']);
                }

                //将'('和以后的内容向后移动funLen
                int funLen = strlen(it->second);
                int moveIndex = curIndex - 1 + funLen;
                while(moveIndex - funLen != leftId) {
                    pout[moveIndex] = pout[moveIndex - funLen];
                    --moveIndex;
                }

                //将函数名填写到之前空出来的位置
                for(int j = 0; j < funLen; j++)
                    pout[leftId + j + 1] = it->second[j];

                //去掉最后的空格并加上','
                curIndex += funLen;
                while(pout[curIndex-1] == ' ')
                    --curIndex;
                if(line[i] == ':')
                    pout[curIndex++] = ')';
                pout[curIndex++] = ',';
            }
            else
            {
                pout[curIndex++] = line[i];
            }
            ++i;
        }
        pout[curIndex] = 0;
        return pout;
    }

    int getCacheSize(const int* outCache){
        int id = 0;
        while (outCache[id] != NONE)
            ++id;
        return (id >> 1);
    }

    //输出当前行操作符的范围,以及左右孩子范围,没有就返回(-1,-1)
    int* decode(const char* line, int l, int r, int* pout){
        getOpt(line, l, r, pout);
        getChildRange(line, l, r, pout);
        return pout;
    }

    const char* readOpt(const char* line, const int* pout, char* optCache, int elementNum = 0){
        int optStrLen = getOptSize(pout, elementNum);
        memcpy(optCache,line + pout[0 + elementNum * 2], optStrLen * sizeof(char));
        optCache[optStrLen] = 0;
        //std::cout<<optCache<<std::endl;
        return optCache;
    }

    size_t getOptSize(const int* pout, int elementNum = 0){
        return pout[1 + elementNum * 2] - pout[0 + elementNum * 2] + 1;
    }

    static bool isSymbolFun(const char* str){
        if (strcmp(str, "add") == 0 ||
                strcmp(str, "reduce") == 0 ||
                strcmp(str, "mul") == 0 ||
                strcmp(str, "div") == 0 ||
                strcmp(str, "and") == 0 ||
                strcmp(str, "or") == 0 ||
                strcmp(str, "cross") == 0 ||
                strcmp(str, "signed_power") == 0 ||
                strcmp(str, "less") == 0 ||
                strcmp(str, "more") == 0 ||
                strcmp(str, "if") == 0 ||
                strcmp(str,"else") == 0)
            return true;
        return false;
    }

    static bool isNum(const char* str){
        int curIndex = 0;
        int len = strlen(str);
        //判断符号
        if(str[curIndex] != '+' && str[curIndex] != '-' && str[curIndex] != 'c' && (str[curIndex] < '0' || str[curIndex] > '9')){
            return false;
        }
        if(str[curIndex] < '0' || str[curIndex] > '9')
            ++curIndex;
        //判断整数部分
        int startIndex = curIndex;
        while(curIndex < len && str[curIndex] != '.') {
            if (str[curIndex] < '0' || str[curIndex] > '9'){
                return false;
            }
            ++curIndex;
        }
        if(str[startIndex] == '0' && curIndex - startIndex > 1 && str[startIndex+1] != '.'){
            return false;
        }
        if(curIndex == len)
            return true;
        ++curIndex;
        //判断小数部分
        while(curIndex < len){
            if (str[curIndex] < '0' || str[curIndex] > '9')
                return false;
            ++curIndex;
        }
        return true;
    }

protected:
    void getOpt(const char* line, int& l, int& r, int* pout){
        int depth = 0;
        //去掉左边所有括号
        depth = 0;
        while (line[l] == '(' || line[l] == ' '){
            if(line[l] == '(')
                ++depth;
            ++l;
        }

        //读出操作数
        pout[0] = l;
        while(l <= r && line[l] != '(' && line[l] != ' '){
            ++l;
        }
        pout[1] = l-1;

        //全部读取完毕
        if(l > r) {
            if (depth != 0)
                throw "depth need equal 0!";
            return;
        }

        //操作数后必须紧跟'('
        if(line[l] != '(')
            throw "'(' must after opt!";
        ++l;
        ++depth;

        //去掉右边括号
        while(l <= r && (line[r] == ')' || line[r] == ' ')){
            if(line[r] == ')')
                --depth;
            --r;
            //深度等于0时才可以结束
            if(r < l)
                throw "deth must equal 0!";
            if(depth == 0)
                break;
        }
    }

    void getChildRange(const char* line, int& l, int& r, int* pout){
        int id = l;
        int curIndex = 2;
        int depth = 0;
        while(id <= r){
            if(line[id] == '(')
                ++depth;
            else if(line[id] == ')')
                --depth;
            //找到深度是0的第一个','
            if(depth == 0 && line[id] == ','){
                while(line[l] == ' ') ++l;
                pout[curIndex++] = l;
                pout[curIndex++] = id-1;
                l = id+1;
            }

            //将最后一段做成孩子（不以','结尾）
            if(id == r){
                //深度等于0时才可以结束
                if(depth != 0)
                    throw "deth must equal 0!";
                while(line[l] == ' ') ++l;
                pout[curIndex++] = l;
                pout[curIndex++] = r;
            }
            ++id;
        }
        pout[curIndex++] = NONE;
    }

    char* function2operator(const char* line, char* pout, int l, int r, int& curIndex, int* rangeCache){
        decode(line, l, r, rangeCache);

        //读出操作数
        int optLen = rangeCache[1]-rangeCache[0]+1;
        memcpy(pout + curIndex, line + rangeCache[0], optLen * sizeof(char));
        pout[curIndex+optLen] = 0;

        //所有操作符号都是有两个孩子
        int ll = rangeCache[2], lr = rangeCache[3], rl = rangeCache[4], rr = rangeCache[5], cl = rangeCache[6], cr = rangeCache[7];

        map<const char *, char, ptrCmp>::iterator it;
        it=invOptMap.find(pout+curIndex);
        if(it != invOptMap.end()){

            if(it->second != '?')
                pout[curIndex++] = '(';

            function2operator(line, pout, ll, lr, curIndex, rangeCache);
            pout[curIndex++] = ' ';
            pout[curIndex++] = it->second;
            pout[curIndex++] = ' ';
            function2operator(line, pout, rl, rr, curIndex, rangeCache);

            if(it->second != '?')
                pout[curIndex++] = ')';
        }else{
            curIndex += optLen;
            if(ll != -1){
                pout[curIndex++] = '(';

                function2operator(line, pout, ll, lr, curIndex, rangeCache);

                if(rl != -1){
                    pout[curIndex++] = ',';
                    pout[curIndex++] = ' ';
                    function2operator(line, pout, rl, rr, curIndex, rangeCache);

                    if(cl != -1){
                        pout[curIndex++] = ',';
                        pout[curIndex++] = ' ';
                        function2operator(line, pout, cl, cr, curIndex, rangeCache);
                    }
                }


                pout[curIndex++] = ')';
            }
        }
        return pout;
    }

    map<char, const char*> optMap;
    map<const char *, char, ptrCmp> invOptMap;
};

#endif //ALPHATREE_CONVERTER_H
