cmake_minimum_required(VERSION 3.20)
project(NeoRPC LANGUAGES CXX)

# 开启 C++20 支持
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 基础编译选项：开启警告
add_compile_options(-Wall -Wextra -O3)

# 包含头文件路径
include_directories(include)

# 生成可执行文件
add_executable(NeoRPC src/main.cpp src/TcpServer.cpp src/Conn.cpp
                      src/LogicHandler.cpp)

# 如果之后用到 io_uring，需要链接 liburing
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(URING REQUIRED liburing)
# target_link_libraries(NeoRPC PRIVATE ${URING_LIBRARIES})
