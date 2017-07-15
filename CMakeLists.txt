cmake_minimum_required(VERSION 3.5)
project(ffmpeg_demo)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# -I 
# include_directories(../../../thirdparty/comm/include)
include_directories("./ffmpeg-3.3.2/include")
# -L 
# link_directories("/home/server/third/lib")
link_directories("./ffmpeg-3.3.2/lib")

set(SOURCE_FILES src/main.cpp)
add_executable(ffmpeg_demo ${SOURCE_FILES})

target_link_libraries(ffmpeg_demo  -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale)
