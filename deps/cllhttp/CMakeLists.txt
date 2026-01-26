cmake_minimum_required(VERSION 3.10)
project(llhttp C)

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-error=unused-parameter")

# 优化选项
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
endif()

# 源文件
set(LLHTTP_SOURCES
    llhttp.c
    api.c
)

# 头文件
set(LLHTTP_HEADERS
    llhttp.h
)

# 创建静态库
add_library(llhttp STATIC ${LLHTTP_SOURCES})

# 设置库属性
set_target_properties(llhttp PROPERTIES
    VERSION 1.0.0
    SOVERSION 1
    PUBLIC_HEADER "${LLHTTP_HEADERS}"
    POSITION_INDEPENDENT_CODE ON
)

# 安装规则
install(TARGETS llhttp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)

# 打印配置信息
message(STATUS "llhttp Configuration:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")