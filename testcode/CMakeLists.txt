# 获取当前目录下的所有源文件
file(GLOB EXAMPLE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# 创建可执行文件
add_executable(EchoServer ${EXAMPLE_SRCS})

# 链接必要的库，在 src 文件 CMakeLists 中 Lucky_lib 静态库，还有全局链接库
target_link_libraries(EchoServer Lucky ${LIBS})

# 设置编译选项
target_compile_options(EchoServer PRIVATE -std=c++11 -Wall)

# 设置 EchoServer 可执行文件输出目录
set_target_properties(EchoServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)