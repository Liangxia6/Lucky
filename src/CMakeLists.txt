#获取当前目录下的所有源文件
file(GLOB SRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

#创建静态库或共享库
add_library(Lucky SHARED ${SRC_FILES})

#设置头文件的路径
target_include_directories(Lucky PUBLIC ${CMAKE_SOURCE_DIR}/include)