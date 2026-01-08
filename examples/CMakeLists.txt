cmake_minimum_required(VERSION 3.10)
project(uvhttp_examples C)

set(CMAKE_C_STANDARD 11)

# 查找 UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ../include NO_DEFAULT_PATH)
find_library(UVHTTP_LIBRARY uvhttp PATHS ../build NO_DEFAULT_PATH)

if(NOT UVHTTP_INCLUDE_DIR)
    message(FATAL_ERROR "找不到 uvhttp.h，请先编译 UVHTTP")
endif()

if(NOT UVHTTP_LIBRARY)
    message(FATAL_ERROR "找不到 libuvhttp，请先编译 UVHTTP")
endif()

message(STATUS "UVHTTP 头文件: ${UVHTTP_INCLUDE_DIR}")
message(STATUS "UVHTTP 库文件: ${UVHTTP_LIBRARY}")

include_directories(${UVHTTP_INCLUDE_DIR})

# 基础示例
add_subdirectory(01_basics)

# 路由示例
add_subdirectory(02_routing)

# 请求处理示例（待添加）
# add_subdirectory(03_requests)

# 响应处理示例（待添加）
# add_subdirectory(04_responses)

# 响应处理示例
add_subdirectory(04_responses)

# 高级特性示例
add_subdirectory(05_advanced)

# 生产环境示例（待添加）
# add_subdirectory(06_production)
