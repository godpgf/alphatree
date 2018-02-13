cmake_minimum_required(VERSION 3.3)
project(alphatree)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} libalphatree/base libalphatree/carf libalphatree/alpha libalphatree/db)
find_package(Threads REQUIRED)

set(SOURCE_FILES test.cpp libalphatree/alphaforest.h)
add_executable(alphatree ${SOURCE_FILES})
target_link_libraries(alphatree Threads::Threads)