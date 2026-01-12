cmake_minimum_required(VERSION 3.10)
project(uvhttp VERSION 1.4.0 LANGUAGES C)

# 构建选项
option(BUILD_WITH_WEBSOCKET "Build with WebSocket support" ON)
option(BUILD_WITH_MIMALLOC "Build with mimalloc allocator" ON)
option(BUILD_WITH_TLS "Build with TLS support" ON)
option(ENABLE_DEBUG "Enable debug build with -O0" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(BUILD_EXAMPLES "Build example programs" ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
else()
    # 性能优化选项（不使用 LTO，避免性能下降）
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native -mtune=native")
    # 函数内联优化
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -finline-functions -finline-limit=1000")
    # 循环优化
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -funroll-loops")
endif()

if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# 安全编译选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -Wall \
    -Wextra \
    -Wformat=2 \
    -Wformat-security \
    -fstack-protector-strong \
    -D_FORTIFY_SOURCE=2 \
    -fno-omit-frame-pointer \
    -fno-common \
    -Werror=implicit-function-declaration \
    -Werror=format-security \
    -Werror=return-type \
")

# 链接器安全标志
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -Wl,-z,relro \
    -Wl,-z,now \
")

# 特性定义
add_definitions(-DUVHTTP_FEATURE_STATIC_FILES=1)
add_definitions(-DUVHTTP_FEATURE_TLS=1)
add_definitions(-DUVHTTP_FEATURE_LRU_CACHE=1)

# 内存分配器类型
if(NOT DEFINED UVHTTP_ALLOCATOR_TYPE)
    set(UVHTTP_ALLOCATOR_TYPE 0)
endif()

# 验证 UVHTTP_ALLOCATOR_TYPE 的值
if(NOT UVHTTP_ALLOCATOR_TYPE EQUAL 0 AND NOT UVHTTP_ALLOCATOR_TYPE EQUAL 1)
    message(FATAL_ERROR "UVHTTP_ALLOCATOR_TYPE must be 0 (system) or 1 (mimalloc), got: ${UVHTTP_ALLOCATOR_TYPE}")
endif()

# 开发模式 - 启用日志输出
option(ENABLE_DEV_MODE "Enable development mode with logging" OFF)
if(ENABLE_DEV_MODE)
    add_definitions(-DUVHTTP_DEV_MODE)
    message(STATUS "Development mode: ENABLED (logging enabled)")
else()
    message(STATUS "Development mode: DISABLED (no logging)")
endif()

add_definitions(-DUVHTTP_ALLOCATOR_TYPE=${UVHTTP_ALLOCATOR_TYPE})

if(BUILD_WITH_WEBSOCKET)
    add_definitions(-DUVHTTP_FEATURE_WEBSOCKET=1)
endif()

# 包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/uthash/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/xxhash)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/include)

# 源文件
set(SOURCES
    src/uvhttp_utils.c
    src/uvhttp_request.c
    src/uvhttp_response.c
    src/uvhttp_router.c
    src/uvhttp_connection.c
    src/uvhttp_server.c
    src/uvhttp_middleware.c
    src/uvhttp_tls_mbedtls.c
    src/uvhttp_config.c
    src/uvhttp_network.c
    src/uvhttp_context.c
    src/uvhttp_deps.c
    src/uvhttp_error.c
    src/uvhttp_error_handler.c
    src/uvhttp_error_helpers.c
    src/uvhttp_hash.c
    src/uvhttp_mempool.c
    src/uvhttp_lru_cache.c
    src/uvhttp_router_cache.c
    src/uvhttp_static.c
    src/uvhttp_async_file.c
    src/uvhttp_validation.c
    src/uvhttp_cors_middleware.c
    src/uvhttp_allocator.c
)

if(BUILD_WITH_WEBSOCKET)
    list(APPEND SOURCES src/uvhttp_websocket_native.c)
endif()

# 头文件
set(HEADERS
    include/uvhttp.h
    include/uvhttp_common.h
    include/uvhttp_config.h
    include/uvhttp_constants.h
    include/uvhttp_connection.h
    include/uvhttp_context.h
    include/uvhttp_deps.h
    include/uvhttp_error.h
    include/uvhttp_error_handler.h
    include/uvhttp_error_helpers.h
    include/uvhttp_features.h
    include/uvhttp_hash.h
    include/uvhttp_lru_cache.h
    include/uvhttp_log_middleware.h
    include/uvhttp_middleware.h
    include/uvhttp_mempool.h
    include/uvhttp_network.h
    include/uvhttp_request.h
    include/uvhttp_response.h
    include/uvhttp_router.h
    include/uvhttp_router_cache.h
    include/uvhttp_server.h
    include/uvhttp_static.h
    include/uvhttp_string_pool.h
    include/uvhttp_tls.h
    include/uvhttp_utils.h
    include/uvhttp_validation.h
    include/uvhttp_websocket_middleware.h
    include/uvhttp_cors_middleware.h
    include/uvhttp_simple_middleware.h
    include/uvhttp_allocator.h
)

if(BUILD_WITH_WEBSOCKET)
    list(APPEND HEADERS include/uvhttp_websocket_native.h)
endif()

# xxhash 支持
set(XXHASH_LIB ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxhash/libxxhash.a)
if(EXISTS ${XXHASH_LIB})
    message(STATUS "Found xxhash library: ${XXHASH_LIB}")
    list(APPEND LIBS ${XXHASH_LIB})
else()
    find_path(XXHASH_SRC xxhash.c
        PATHS ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxhash
        NO_DEFAULT_PATH
    )
    if(XXHASH_SRC)
        message(STATUS "Found xxhash source: ${XXHASH_SRC}")
        list(APPEND SOURCES ${XXHASH_SRC})
    else()
        message(FATAL_ERROR "xxhash library not found")
    endif()
endif()

# mimalloc 支持
if(BUILD_WITH_MIMALLOC)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc/include)
    add_definitions(-DUVHTTP_ENABLE_MIMALLOC)
    # 优先使用静态库
    set(MIMALLOC_STATIC_LIB ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc/build/libmimalloc.a)
    if(EXISTS ${MIMALLOC_STATIC_LIB})
        message(STATUS "Found mimalloc static library: ${MIMALLOC_STATIC_LIB}")
        set(MIMALLOC_LIB ${MIMALLOC_STATIC_LIB})
    else()
        # 回退到动态库
        find_library(MIMALLOC_LIB mimalloc
            PATHS ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc/build
            NO_DEFAULT_PATH
        )
    endif()
    if(MIMALLOC_LIB)
        message(STATUS "Using mimalloc library: ${MIMALLOC_LIB}")
        add_definitions(-DUVHTTP_FEATURE_ALLOCATOR=1)
    else()
        message(WARNING "mimalloc library not found, BUILD_WITH_MIMALLOC will be ignored")
    endif()
endif()

# 库文件
set(LIBS
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson/libcjson.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/library/libmbedcrypto.a
    ${XXHASH_LIB}
    pthread
    m
)

# 添加mimalloc库（如果找到）
if(BUILD_WITH_MIMALLOC AND MIMALLOC_LIB)
    list(APPEND LIBS ${MIMALLOC_LIB})
endif()

# 尝试使用pkg-config查找libuv
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBUV libuv)
    if(LIBUV_FOUND)
        list(APPEND LIBS ${LIBUV_LIBRARIES})
        include_directories(${LIBUV_INCLUDE_DIRS})
        link_directories(${LIBUV_LIBRARY_DIRS})
    endif()
endif()

# 如果pkg-config没有找到，使用deps/libuv
if(NOT LIBUV_FOUND)
    list(APPEND LIBS ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/.libs/libuv.a)
    list(APPEND LIBS dl)  # dl库必须在libuv之后
endif()

# 创建静态库
add_library(uvhttp STATIC ${SOURCES})
target_link_libraries(uvhttp ${LIBS})

# 共享库需要 -fPIC，暂时禁用
# add_library(uvhttp_shared SHARED ${SOURCES})
# set_target_properties(uvhttp_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)
# target_link_libraries(uvhttp_shared ${LIBS})

# 启用测试
enable_testing()

# 添加测试
file(GLOB TEST_FILES test/unit/test_*.c)
list(FILTER TEST_FILES EXCLUDE REGEX ".*disabled.*")
list(FILTER TEST_FILES EXCLUDE REGEX ".*test_memory_helpers\\.c")  # 排除辅助文件
foreach(test_file ${TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file} test/unit/test_memory_helpers.c ${SOURCES})
    target_link_libraries(${test_name} ${LIBS})
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# 添加压力测试
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_stress.c)
    add_executable(test_stress test/test_stress.c ${SOURCES})
    target_link_libraries(test_stress ${LIBS})
    message(STATUS "压力测试程序已配置")
endif()

# 添加测试服务器
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_server.c)
    add_executable(uvhttp_test_server test/test_server.c ${SOURCES})
    target_link_libraries(uvhttp_test_server ${LIBS})
    message(STATUS "测试服务器已配置")
endif()

# 添加简单性能测试服务器
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple_bench_server.c)
    add_executable(simple_bench_server examples/simple_bench_server.c ${SOURCES})
    target_link_libraries(simple_bench_server ${LIBS})
    message(STATUS "简单性能测试服务器已配置")
endif()

# 添加性能测试服务器
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/examples/bench_server.c)
    add_executable(bench_server examples/bench_server.c ${SOURCES})
    target_link_libraries(bench_server ${LIBS})
    message(STATUS "性能测试服务器已配置")
endif()

# 添加uvhttp压力测试服务器
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/uvhttp_bench_server.c)
    add_executable(uvhttp_bench_server test/uvhttp_bench_server.c ${SOURCES})
    target_link_libraries(uvhttp_bench_server ${LIBS})
    message(STATUS "uvhttp压力测试服务器已配置")
endif()

# 添加uvhttp直接测试服务器
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/uvhttp_direct_server.c)
    add_executable(uvhttp_direct_server test/uvhttp_direct_server.c ${SOURCES})
    target_link_libraries(uvhttp_direct_server ${LIBS})
    message(STATUS "uvhttp直接测试服务器已配置")
endif()

# 添加自定义目标来运行所有测试
add_custom_target(run_tests
    COMMAND ${CMAKE_COMMAND} -E env "LC_ALL=C" bash -c "for f in test_*; do echo \"Running \$f...\" && ./\$f || true; done"
    DEPENDS ${TEST_FILES}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 安装规则
install(TARGETS uvhttp DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/uvhttp)

# 打印配置信息
message(STATUS "Build configuration:")
message(STATUS "  WebSocket: ${BUILD_WITH_WEBSOCKET}")
message(STATUS "  mimalloc: ${BUILD_WITH_MIMALLOC}")
message(STATUS "  Debug: ${ENABLE_DEBUG}")
message(STATUS "  Coverage: ${ENABLE_COVERAGE}")

# 添加示例编译支持
option(BUILD_EXAMPLES "Build example programs" ON)

if(BUILD_EXAMPLES)
    # 暂时只编译性能测试，跳过其他有问题的示例
    add_subdirectory(examples/performance_only)
    add_subdirectory(examples)
    message(STATUS "示例编译: 已启用")
endif()