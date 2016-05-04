
cmake_minimum_required(VERSION 3.1)
project(rabit)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp)
add_executable(rabit ${SOURCE_FILES} Socket.h utils.h rabit_tracker.h)
#add_executable(rabit test/test_exec.cc)