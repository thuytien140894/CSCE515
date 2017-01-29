cmake_minimum_required(VERSION 3.6)
project(PreforkedServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp echoserver.c echoclient.c)
add_executable(PreforkedServer ${SOURCE_FILES})