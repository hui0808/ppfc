cmake_minimum_required(VERSION 3.12)
project(ppfc)

set(CMAKE_CXX_STANDARD 14)
#add_compile_options(-O1)
#add_compile_options(-g)

# 指定库的路径，必须先于target生成之前指定
link_directories(lib/x86_64-w64-mingw32/lib)

# 头文件
include_directories(lib/x86_64-w64-mingw32/include/SDL2)
include_directories(src)

file(GLOB SRC_FILE "src/*.cpp")
# 生成可执行文件
add_executable(${PROJECT_NAME} ${SRC_FILE})

# 库的目录
target_link_libraries(${PROJECT_NAME} SDL2)
