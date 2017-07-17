cmake_minimum_required(VERSION 3.5)
project(ffmpeg_demo)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES src/main.cpp)
add_executable(ffmpeg_demo ${SOURCE_FILES})