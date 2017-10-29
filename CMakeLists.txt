cmake_minimum_required(VERSION 3.3)
project(alphatree)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES test.cpp libalphatree/external/rapidjson/document.h libalphatree/external/rapidjson/writer.h libalphatree/external/rapidjson/stringbuffer.h libalphatree/alpha/alpha.h libalphatree/alpha/alphaatom.h libalphatree/alpha/alphatree.h libalphatree/alphaforest.h libalphatree/alpha/base.h libalphatree/alpha/converter.h libalphatree/alpha/unit.h libalphatree/base/cache.h libalphatree/base/threadpool.h libalphatree/alpha/alphadb.h libalphatree/alpha/normal.h libalphatree/alphaexaminer.h! libalphatree/base/hashname.h libalphatree/base/darray.h libalphatree/filter/alphagroup.h libalphatree/filter/alphawork.h libalphatree/filter/vgwork.h libalphatree/filter/flagwork.h libalphatree/filter/collectwork.h libalphatree/alphafilter.h! libalphatree/base/hashmap.h libalphatree/base/dcache.h libalphatree/alpha/basealphatree.h libalphatree/alpha/alphaprocess.h libalphatree/alpha/process.h)
add_executable(alphatree ${SOURCE_FILES})