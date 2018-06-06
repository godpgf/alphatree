cmake_minimum_required(VERSION 3.3)
project(alphatree)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} libalphatree/base libalphatree/alpha libalphatree/alpha/atom libalphatree/db libalphatree/hmm)
find_package(Threads REQUIRED)

set(SOURCE_FILES libalphatree/libalphatree_api.cpp libalphatree/alphaforest.h)
ADD_LIBRARY(alphatree ${SOURCE_FILES})
target_link_libraries(alphatree Threads::Threads)