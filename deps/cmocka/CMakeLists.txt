cmake_minimum_required(VERSION 3.10)

project(cmocka C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

# cmocka 源文件
set(CMOCKA_SOURCES
    src/cmocka.c
    src/cmocka_panic.c
    src/cmocka_print.c
)

# 创建静态库
add_library(cmocka STATIC ${CMOCKA_SOURCES})

# 设置包含目录
target_include_directories(cmocka PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 禁用警告
if(MSVC)
    target_compile_options(cmocka PRIVATE /W0)
else()
    target_compile_options(cmocka PRIVATE -w)
endif()