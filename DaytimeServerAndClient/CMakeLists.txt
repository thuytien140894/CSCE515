cmake_minimum_required(VERSION 3.6)
project(DaytimeServerAndClient)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Werror")

set(SOURCE_FILES main.c daytimetcpcli.c daytimetcpsrv.c myFile.h)
add_executable(DaytimeServerAndClient ${SOURCE_FILES})