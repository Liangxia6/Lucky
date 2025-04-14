cmake_minimum_required(VERSION 2.5)
project(Lucky)

# Lucky最终编译成so动态库 设置动态库的路径 放置项目根目录的lib文件夹下面
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#设置全局的c++标准
set (CMAKE_CXX_STANDARD 11) 
set (CMAKE_CXX_STANDARD_REQUIRED True)

# 定义参与编译的源代码文件
# aux_source_directory(. SRC_LIST)

# 编译动态库
# add_library(Lucky SHARED ${SRC_LIST})
#设置全局链接库
set(LIBS
    pthread
)

#添加子目录
add_subdirectory(src)
add_subdirectory(testcode)