cmake_minimum_required(VERSION 3.3)
project(alphatree)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} libalphatree/base libalphatree/alpha libalphatree/alpha/atom libalphatree/db libalphatree/test/hmm libalphatree/cart)
find_package(Threads REQUIRED)

#set(SOURCE_FILES libalphatree/libalphatree_api.cpp libalphatree/alphaforest.h libalphatree/alphagbdt.h)
#ADD_LIBRARY(alphatree ${SOURCE_FILES})
set(SOURCE_FILES test.cpp libalphatree/alphaforest.h libalphatree/alphagbdt.h libalphatree/alphabi.h)
ADD_EXECUTABLE(alphatree ${SOURCE_FILES})
target_link_libraries(alphatree Threads::Threads)