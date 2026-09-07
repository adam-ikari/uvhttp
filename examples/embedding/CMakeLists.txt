cmake_minimum_required(VERSION 3.10)
project(uvhttp_embedding_example C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# 嵌入 uvhttp（静态库 target：uvhttp）
#
# 本示例位于仓库内，因此用相对路径 ../.. 指向 uvhttp 源码根目录。
# 在你的真实项目中，通常把 uvhttp 放到第三方目录，例如：
#   add_subdirectory(third_party/uvhttp)
#
# 前置条件：uvhttp 的 deps/ 均为 git submodule，克隆时必须带子模块：
#   git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
# ---------------------------------------------------------------------------
set(UVHTTP_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

# 嵌入者通常不需要 uvhttp 自带的 benchmark / example 目标
set(BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(${UVHTTP_SOURCE_DIR} uvhttp)

# ---------------------------------------------------------------------------
# 可选：用 FetchContent 方式代替 add_subdirectory
#   include(FetchContent)
#   FetchContent_Declare(uvhttp
#     GIT_REPOSITORY https://github.com/adam-ikari/uvhttp.git
#     GIT_TAG        v2.7.1)
#   FetchContent_MakeAvailable(uvhttp)
# 两种方式产出的链接 target 都是 uvhttp。
# ---------------------------------------------------------------------------

add_executable(embedding_server main.c)
target_link_libraries(embedding_server PRIVATE uvhttp)
