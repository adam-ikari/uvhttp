cmake_minimum_required(VERSION 3.10)
project(uvhttp VERSION 2.0.0 LANGUAGES C CXX)

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
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 平台检测
if(WIN32)
    set(PLATFORM_NAME "Windows")
    set(IS_WINDOWS TRUE)
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
    set(IS_MACOS TRUE)
else()
    set(PLATFORM_NAME "Linux")
    set(IS_LINUX TRUE)
endif()

message(STATUS "Detected platform: ${PLATFORM_NAME}")

# 编译选项 - 根据平台设置
if(ENABLE_DEBUG)
    if(MSVC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /Od /Zi")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    endif()
else()
    if(MSVC)
        # MSVC 优化选项
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /O2 /GL")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
    elseif(IS_MACOS)
        # macOS 优化选项
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -march=native")
    else()
        # Linux 优化选项
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -march=native -mtune=native")
    endif()
endif()

if(ENABLE_COVERAGE)
    if(MSVC)
        message(WARNING "Code coverage not supported on MSVC")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
    endif()
endif()

# 安全编译选项 - 根据平台设置
if(MSVC)
    # MSVC 安全选项
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
        /W4 \
        /WX \
        /GS \
        /sdl \
    ")
    # MSVC 链接器安全选项
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
        /DYNAMICBASE \
        /NXCOMPAT \
    ")
else()
    # GCC/Clang 安全选项
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
        -Wall \
        -Wextra \
        -Wformat=2 \
        -Wformat-security \
        -fstack-protector-strong \
        -fno-omit-frame-pointer \
        -fno-common \
        -Werror \
        -Werror=implicit-function-declaration \
        -Werror=format-security \
        -Werror=return-type \
    ")
    
    # Linux 特定的安全选项
    if(IS_LINUX)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FORTIFY_SOURCE=2")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
            -Wl,-z,relro \
            -Wl,-z,now \
        ")
    endif()
endif()

# 特性定义
if(NOT DEFINED UVHTTP_FEATURE_MIDDLEWARE)
    set(UVHTTP_FEATURE_MIDDLEWARE 1)
endif()
add_definitions(-DUVHTTP_FEATURE_MIDDLEWARE=${UVHTTP_FEATURE_MIDDLEWARE})
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

# ============================================================================
# 包含依赖构建配置
# ============================================================================
include(cmake/Dependencies.cmake)

# 添加依赖包含目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${DEPS_INCLUDE_DIRS})

# 源文件
set(SOURCES
    src/uvhttp_utils.c
    src/uvhttp_request.c
    src/uvhttp_response.c
    src/uvhttp_router.c
    src/uvhttp_connection.c
    src/uvhttp_server.c
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
    src/uvhttp_allocator.c
)

# 条件编译中间件源文件
if(UVHTTP_FEATURE_MIDDLEWARE)
    list(APPEND SOURCES 
        src/uvhttp_middleware.c
        src/uvhttp_cors_middleware.c
    )
endif()

if(BUILD_WITH_WEBSOCKET)
    list(APPEND SOURCES src/uvhttp_websocket_native.c src/uvhttp_websocket_auth.c)
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
    include/uvhttp_allocator.h
)

# 条件编译中间件相关头文件
if(UVHTTP_FEATURE_MIDDLEWARE)
    list(APPEND HEADERS
        include/uvhttp_log_middleware.h
        include/uvhttp_middleware.h
        include/uvhttp_websocket_middleware.h
        include/uvhttp_cors_middleware.h
        include/uvhttp_simple_middleware.h
    )
endif()

if(BUILD_WITH_WEBSOCKET)
    list(APPEND HEADERS include/uvhttp_websocket_native.h)
endif()

# ============================================================================
# 设置依赖库
# ============================================================================
set(LIBS
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
    ${XXHASH_LIB}
    ${LLHTTP_LIB}
    ${CJSON_LIB}
)

# 平台特定的库
if(IS_WINDOWS)
    # Windows 不需要额外的库
elseif(IS_MACOS)
    # macOS 需要 pthread
    list(APPEND LIBS pthread)
else()
    # Linux 需要 pthread, m, dl
    list(APPEND LIBS pthread m dl)
endif()

if(BUILD_WITH_MIMALLOC)
    list(APPEND LIBS ${MIMALLOC_LIB})
    add_definitions(-DUVHTTP_ENABLE_MIMALLOC)
    add_definitions(-DUVHTTP_FEATURE_ALLOCATOR=1)
endif()

# ============================================================================
# 创建静态库
# ============================================================================
add_library(uvhttp STATIC ${SOURCES})

# 添加依赖关系
add_dependencies(uvhttp libuv mbedtls xxhash)
# 不添加 mimalloc 依赖，因为它会构建测试程序导致失败

# 链接库
target_link_libraries(uvhttp ${LIBS})

# ============================================================================
# 启用测试
# ============================================================================
enable_testing()

# 添加 gtest 测试（仅支持 C++ 测试文件）
file(GLOB_RECURSE GTEST_TEST_FILES test/unit/test_*.cpp)
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*disabled.*")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_loop_helper.cpp")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_time_mock.cpp")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_.*_mock_coverage.cpp")

# 条件编译中间件相关测试
if(NOT UVHTTP_FEATURE_MIDDLEWARE)
    list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*middleware.*")
    list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*cors.*")
endif()

# 添加测试辅助文件
set(TEST_HELPER_FILES test/unit/test_time_mock.cpp test/unit/test_loop_helper.cpp)

foreach(test_file ${GTEST_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    
    # 所有测试文件都是 C++ 文件
    # 如果测试需要时间 mock，则添加辅助文件
    if(${test_name} STREQUAL "test_lru_cache_full_coverage" OR 
       ${test_name} STREQUAL "test_static_file_operations_coverage")
        add_executable(${test_name} ${test_file} ${TEST_HELPER_FILES} ${SOURCES})
    else()
        add_executable(${test_name} ${test_file} ${SOURCES})
    endif()
    
    set_target_properties(${test_name} PROPERTIES LINKER_LANGUAGE CXX)
    
    add_dependencies(${test_name} libuv mbedtls xxhash gtest)
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(${test_name} mimalloc)
    endif()
    # 对测试文件移除 -Werror，允许未使用变量警告
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /wd4100 /wd4101 /wd4505)
    else()
        target_compile_options(${test_name} PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    endif()
    # 为测试启用统计功能
    target_compile_definitions(${test_name} PRIVATE UVHTTP_FEATURE_STATISTICS=1)
    target_link_libraries(${test_name} ${LIBS} ${GTEST_LIBS})
    target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googlemock/include)
    add_test(NAME ${test_name} COMMAND ${test_name})
    
    # 设置测试超时时间和标签
    if(${test_name} STREQUAL "test_server_full_coverage" OR 
       ${test_name} STREQUAL "test_deps_full_coverage" OR
       ${test_name} STREQUAL "test_server_rate_limit_coverage" OR
       ${test_name} STREQUAL "test_server_simple_api_coverage" OR
       ${test_name} STREQUAL "test_static_prewarm_coverage")
        # 这些测试需要更长时间运行，设置为慢速测试，超时300秒
        # 添加 RUN_SERIAL TRUE 避免并发执行导致的竞争条件
        set_tests_properties(${test_name} PROPERTIES TIMEOUT 300 LABELS "slow" RUN_SERIAL TRUE)
    else()
        # 其他测试为快速测试，超时60秒
        set_tests_properties(${test_name} PROPERTIES TIMEOUT 60 LABELS "fast")
    endif()
endforeach()

# 添加 gtest 示例测试（使用 C++ 编译）
add_executable(uvhttp_unit_tests test/unit/simple_test.cpp ${SOURCES})
set_target_properties(uvhttp_unit_tests PROPERTIES LINKER_LANGUAGE CXX)
add_dependencies(uvhttp_unit_tests libuv mbedtls xxhash gtest)
if(BUILD_WITH_MIMALLOC)
    add_dependencies(uvhttp_unit_tests mimalloc)
endif()
if(MSVC)
    target_compile_options(uvhttp_unit_tests PRIVATE /wd4100 /wd4101 /wd4505)
else()
    target_compile_options(uvhttp_unit_tests PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
endif()
target_link_libraries(uvhttp_unit_tests ${LIBS} ${GTEST_LIBS})
target_include_directories(uvhttp_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googlemock/include)
add_test(NAME uvhttp_unit_tests COMMAND uvhttp_unit_tests)
set_tests_properties(uvhttp_unit_tests PROPERTIES TIMEOUT 60 LABELS "fast")

# ============================================================================
# CMocka + GTest 集成测试
# ============================================================================
# 注意：CMocka 已移除，因为链接器 wrap 机制与项目现有代码不兼容
# 如需 mock 功能，请考虑其他测试策略

# 添加压力测试
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test/test_stress.c)
    add_executable(test_stress test/test_stress.c ${SOURCES})
    add_dependencies(test_stress libuv mbedtls xxhash)
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(test_stress mimalloc)
    endif()
    if(MSVC)
        target_compile_options(test_stress PRIVATE /wd4100 /wd4101 /wd4505)
    else()
        target_compile_options(test_stress PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    endif()
    target_link_libraries(test_stress ${LIBS})
endif()

# ============================================================================
# 构建示例程序
# ============================================================================
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
# 安装规则
# ============================================================================
install(TARGETS uvhttp
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

install(DIRECTORY include/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# ============================================================================
# 打印配置信息
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "UVHTTP Configuration Summary")
message(STATUS "========================================")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C Standard: ${CMAKE_C_STANDARD}")
message(STATUS "WebSocket Support: ${BUILD_WITH_WEBSOCKET}")
message(STATUS "mimalloc Support: ${BUILD_WITH_MIMALLOC}")
message(STATUS "TLS Support: ${BUILD_WITH_TLS}")
message(STATUS "Middleware Support: ${UVHTTP_FEATURE_MIDDLEWARE}")
message(STATUS "Debug Mode: ${ENABLE_DEBUG}")
message(STATUS "Coverage: ${ENABLE_COVERAGE}")
message(STATUS "Examples: ${BUILD_EXAMPLES}")
message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# 集成测试
# ============================================================================
file(GLOB INTEGRATION_TEST_FILES test/integration/*.c)
foreach(test_file ${INTEGRATION_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file} ${SOURCES})
    add_dependencies(${test_name} libuv mbedtls xxhash)
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(${test_name} mimalloc)
    endif()
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /wd4100 /wd4101 /wd4505)
    else()
        target_compile_options(${test_name} PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    endif()
    target_link_libraries(${test_name} ${LIBS})
endforeach()

# ============================================================================
# 性能测试
# ============================================================================
file(GLOB PERFORMANCE_TEST_FILES test/performance/*.c)
foreach(test_file ${PERFORMANCE_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_executable(${test_name} ${test_file} ${SOURCES})
    add_dependencies(${test_name} libuv mbedtls xxhash)
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(${test_name} mimalloc)
    endif()
    if(MSVC)
        target_compile_options(${test_name} PRIVATE /wd4100 /wd4101 /wd4505)
    else()
        target_compile_options(${test_name} PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    endif()
    target_link_libraries(${test_name} ${LIBS})
endforeach()
