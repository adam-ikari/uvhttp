cmake_minimum_required(VERSION 3.10)

# 读取版本配置
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_FILE)
    string(REGEX MATCH "VERSION=([0-9]+\\.[0-9]+\\.[0-9]+)" _ ${VERSION_FILE})
    set(PROJECT_VERSION ${CMAKE_MATCH_1})
else()
    set(PROJECT_VERSION "2.3.0")
endif()

project(uvhttp VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

# 构建选项
option(BUILD_WITH_WEBSOCKET "Build with WebSocket support" ON)
option(BUILD_WITH_MIMALLOC "Build with mimalloc allocator" ON)
option(BUILD_WITH_TLS "Build with TLS support" ON)
option(ENABLE_DEBUG "Enable debug build with -O0" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(BUILD_EXAMPLES "Build example programs" ON)

# 检测32位系统
include(CheckTypeSize)
check_type_size("void*" SIZEOF_VOID_PTR)
if(SIZEOF_VOID_PTR EQUAL 4)
    message(STATUS "Building for 32-bit system")
    add_definitions(-DUVHTTP_32BIT)
elseif(SIZEOF_VOID_PTR EQUAL 8)
    message(STATUS "Building for 64-bit system")
    add_definitions(-DUVHTTP_64BIT)
else()
    message(FATAL_ERROR "Unsupported pointer size: ${SIZEOF_VOID_PTR}")
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 编译选项
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -s")
endif()

# 明确设置 Release 模式编译选项（覆盖 CMake 默认的 -O3）
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)

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
    -fno-common \
    -Werror \
    -Werror=implicit-function-declaration \
    -Werror=format-security \
    -Werror=return-type \
    -D_FORTIFY_SOURCE=2 \
")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -Wl,-z,relro \
    -Wl,-z,now \
")

# 特性定义
add_definitions(-DUVHTTP_FEATURE_MIDDLEWARE=1)
add_definitions(-DUVHTTP_FEATURE_STATIC_FILES=1)
add_definitions(-DUVHTTP_FEATURE_TLS=1)
add_definitions(-DUVHTTP_FEATURE_LRU_CACHE=1)

# Protocol upgrade support
set(UVHTTP_FEATURE_PROTOCOL_UPGRADE ON CACHE BOOL "Enable protocol upgrade support")
add_definitions(-DUVHTTP_FEATURE_PROTOCOL_UPGRADE=1)

# ========== 可配置常量选项 ==========
set(UVHTTP_MAX_HEADER_NAME_SIZE 256 CACHE STRING "Max HTTP header name size")
set(UVHTTP_MAX_HEADER_VALUE_SIZE 4096 CACHE STRING "Max HTTP header value size")
set(UVHTTP_MAX_HEADERS 64 CACHE STRING "Max number of HTTP headers")
set(UVHTTP_INLINE_HEADERS_CAPACITY 32 CACHE STRING "Inline headers capacity")
set(UVHTTP_MAX_URL_SIZE 2048 CACHE STRING "Max URL size")
set(UVHTTP_MAX_PATH_SIZE 1024 CACHE STRING "Max path size")
set(UVHTTP_MAX_METHOD_SIZE 16 CACHE STRING "Max method size")
set(UVHTTP_MAX_CONNECTIONS_DEFAULT 2048 CACHE STRING "Default max connections")
set(UVHTTP_MAX_CONNECTIONS_MAX 10000 CACHE STRING "Max recommended connections")
set(UVHTTP_BACKLOG 8192 CACHE STRING "TCP backlog size")
set(UVHTTP_CONNECTION_TIMEOUT_DEFAULT 60 CACHE STRING "Connection timeout (seconds)")
set(UVHTTP_INITIAL_BUFFER_SIZE 8192 CACHE STRING "Initial buffer size")
set(UVHTTP_MAX_BODY_SIZE 1048576 CACHE STRING "Max request body size")
set(UVHTTP_READ_BUFFER_SIZE 16384 CACHE STRING "Read buffer size")
set(UVHTTP_ASYNC_FILE_BUFFER_SIZE 65536 CACHE STRING "Async file buffer size")
set(UVHTTP_ASYNC_FILE_MAX_CONCURRENT 64 CACHE STRING "Max concurrent file reads")
set(UVHTTP_ASYNC_FILE_MAX_SIZE 10485760 CACHE STRING "Max file size")
set(UVHTTP_STATIC_MAX_CACHE_SIZE 1048576 CACHE STRING "Static file cache size")
set(UVHTTP_STATIC_MAX_PATH_SIZE 1024 CACHE STRING "Static file max path size")
set(UVHTTP_STATIC_MAX_CONTENT_LENGTH 32 CACHE STRING "Static file max content length")
set(UVHTTP_STATIC_MAX_FILE_SIZE 1073741824 CACHE STRING "Static file max file size")
set(UVHTTP_STATIC_SMALL_FILE_THRESHOLD 4096 CACHE STRING "Static file small file threshold")
set(UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE 16777216 CACHE STRING "WebSocket default max frame size")
set(UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE 67108864 CACHE STRING "WebSocket default max message size")
set(UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE 65536 CACHE STRING "WebSocket default receive buffer size")
set(UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL 30 CACHE STRING "WebSocket default ping interval")
set(UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT 10 CACHE STRING "WebSocket default ping timeout")
set(UVHTTP_TCP_KEEPALIVE_TIMEOUT 60 CACHE STRING "TCP keepalive timeout")
set(UVHTTP_CLIENT_IP_BUFFER_SIZE 64 CACHE STRING "Client IP buffer size")
set(UVHTTP_SENDFILE_TIMEOUT_MS 30000 CACHE STRING "Sendfile timeout (ms)")
set(UVHTTP_SENDFILE_MAX_RETRY 2 CACHE STRING "Sendfile max retry count")
set(UVHTTP_SENDFILE_CHUNK_SIZE 262144 CACHE STRING "Sendfile chunk size")
set(UVHTTP_SENDFILE_MIN_FILE_SIZE 65536 CACHE STRING "Sendfile min file size")
set(UVHTTP_FILE_SIZE_SMALL 1048576 CACHE STRING "Small file size threshold (1MB)")
set(UVHTTP_FILE_SIZE_MEDIUM 10485760 CACHE STRING "Medium file size threshold (10MB)")
set(UVHTTP_FILE_SIZE_LARGE 104857600 CACHE STRING "Large file size threshold (100MB)")
set(UVHTTP_CHUNK_SIZE_SMALL 65536 CACHE STRING "Small chunk size (64KB)")
set(UVHTTP_CHUNK_SIZE_MEDIUM 262144 CACHE STRING "Medium chunk size (256KB)")
set(UVHTTP_CHUNK_SIZE_LARGE 1048576 CACHE STRING "Large chunk size (1MB)")
set(UVHTTP_CACHE_DEFAULT_MAX_ENTRIES 1000 CACHE STRING "Cache default max entries")
set(UVHTTP_CACHE_DEFAULT_TTL 3600 CACHE STRING "Cache default TTL (seconds)")
set(UVHTTP_SOCKET_SEND_BUF_SIZE 262144 CACHE STRING "Socket send buffer size (256KB)")
set(UVHTTP_SOCKET_RECV_BUF_SIZE 262144 CACHE STRING "Socket receive buffer size (256KB)")
set(UVHTTP_PAGE_SIZE 4096 CACHE STRING "Memory page size")
set(UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE 10 CACHE STRING "LRU cache batch eviction size")
set(UVHTTP_IP_OCTET_MAX_VALUE 255 CACHE STRING "IP octet max value")
set(UVHTTP_RATE_LIMIT_MAX_REQUESTS 1000000 CACHE STRING "Rate limit max requests")
set(UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS 86400 CACHE STRING "Rate limit max window seconds")
set(UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS 10 CACHE STRING "Rate limit min timeout seconds")

# 将选项定义为编译宏
add_definitions(-DUVHTTP_MAX_HEADER_NAME_SIZE=${UVHTTP_MAX_HEADER_NAME_SIZE})
add_definitions(-DUVHTTP_MAX_HEADER_VALUE_SIZE=${UVHTTP_MAX_HEADER_VALUE_SIZE})
add_definitions(-DUVHTTP_MAX_HEADERS=${UVHTTP_MAX_HEADERS})
add_definitions(-DUVHTTP_INLINE_HEADERS_CAPACITY=${UVHTTP_INLINE_HEADERS_CAPACITY})
add_definitions(-DUVHTTP_MAX_URL_SIZE=${UVHTTP_MAX_URL_SIZE})
add_definitions(-DUVHTTP_MAX_PATH_SIZE=${UVHTTP_MAX_PATH_SIZE})
add_definitions(-DUVHTTP_MAX_METHOD_SIZE=${UVHTTP_MAX_METHOD_SIZE})
add_definitions(-DUVHTTP_MAX_CONNECTIONS_DEFAULT=${UVHTTP_MAX_CONNECTIONS_DEFAULT})
add_definitions(-DUVHTTP_MAX_CONNECTIONS_MAX=${UVHTTP_MAX_CONNECTIONS_MAX})
add_definitions(-DUVHTTP_BACKLOG=${UVHTTP_BACKLOG})
add_definitions(-DUVHTTP_CONNECTION_TIMEOUT_DEFAULT=${UVHTTP_CONNECTION_TIMEOUT_DEFAULT})
add_definitions(-DUVHTTP_INITIAL_BUFFER_SIZE=${UVHTTP_INITIAL_BUFFER_SIZE})
add_definitions(-DUVHTTP_MAX_BODY_SIZE=${UVHTTP_MAX_BODY_SIZE})
add_definitions(-DUVHTTP_READ_BUFFER_SIZE=${UVHTTP_READ_BUFFER_SIZE})
add_definitions(-DUVHTTP_ASYNC_FILE_BUFFER_SIZE=${UVHTTP_ASYNC_FILE_BUFFER_SIZE})
add_definitions(-DUVHTTP_ASYNC_FILE_MAX_CONCURRENT=${UVHTTP_ASYNC_FILE_MAX_CONCURRENT})
add_definitions(-DUVHTTP_ASYNC_FILE_MAX_SIZE=${UVHTTP_ASYNC_FILE_MAX_SIZE})
add_definitions(-DUVHTTP_STATIC_MAX_CACHE_SIZE=${UVHTTP_STATIC_MAX_CACHE_SIZE})
add_definitions(-DUVHTTP_STATIC_MAX_PATH_SIZE=${UVHTTP_STATIC_MAX_PATH_SIZE})
add_definitions(-DUVHTTP_STATIC_MAX_CONTENT_LENGTH=${UVHTTP_STATIC_MAX_CONTENT_LENGTH})
add_definitions(-DUVHTTP_STATIC_MAX_FILE_SIZE=${UVHTTP_STATIC_MAX_FILE_SIZE})
add_definitions(-DUVHTTP_STATIC_SMALL_FILE_THRESHOLD=${UVHTTP_STATIC_SMALL_FILE_THRESHOLD})
add_definitions(-DUVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE=${UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE})
add_definitions(-DUVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE=${UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE})
add_definitions(-DUVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE=${UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE})
add_definitions(-DUVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL=${UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL})
add_definitions(-DUVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT=${UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT})
add_definitions(-DUVHTTP_TCP_KEEPALIVE_TIMEOUT=${UVHTTP_TCP_KEEPALIVE_TIMEOUT})
add_definitions(-DUVHTTP_CLIENT_IP_BUFFER_SIZE=${UVHTTP_CLIENT_IP_BUFFER_SIZE})
add_definitions(-DUVHTTP_SENDFILE_TIMEOUT_MS=${UVHTTP_SENDFILE_TIMEOUT_MS})
add_definitions(-DUVHTTP_SENDFILE_MAX_RETRY=${UVHTTP_SENDFILE_MAX_RETRY})
add_definitions(-DUVHTTP_SENDFILE_CHUNK_SIZE=${UVHTTP_SENDFILE_CHUNK_SIZE})
add_definitions(-DUVHTTP_SENDFILE_MIN_FILE_SIZE=${UVHTTP_SENDFILE_MIN_FILE_SIZE})
add_definitions(-DUVHTTP_FILE_SIZE_SMALL=${UVHTTP_FILE_SIZE_SMALL})
add_definitions(-DUVHTTP_FILE_SIZE_MEDIUM=${UVHTTP_FILE_SIZE_MEDIUM})
add_definitions(-DUVHTTP_FILE_SIZE_LARGE=${UVHTTP_FILE_SIZE_LARGE})
add_definitions(-DUVHTTP_CHUNK_SIZE_SMALL=${UVHTTP_CHUNK_SIZE_SMALL})
add_definitions(-DUVHTTP_CHUNK_SIZE_MEDIUM=${UVHTTP_CHUNK_SIZE_MEDIUM})
add_definitions(-DUVHTTP_CHUNK_SIZE_LARGE=${UVHTTP_CHUNK_SIZE_LARGE})
add_definitions(-DUVHTTP_CACHE_DEFAULT_MAX_ENTRIES=${UVHTTP_CACHE_DEFAULT_MAX_ENTRIES})
add_definitions(-DUVHTTP_CACHE_DEFAULT_TTL=${UVHTTP_CACHE_DEFAULT_TTL})
add_definitions(-DUVHTTP_SOCKET_SEND_BUF_SIZE=${UVHTTP_SOCKET_SEND_BUF_SIZE})
add_definitions(-DUVHTTP_SOCKET_RECV_BUF_SIZE=${UVHTTP_SOCKET_RECV_BUF_SIZE})
add_definitions(-DUVHTTP_PAGE_SIZE=${UVHTTP_PAGE_SIZE})
add_definitions(-DUVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE=${UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE})
add_definitions(-DUVHTTP_IP_OCTET_MAX_VALUE=${UVHTTP_IP_OCTET_MAX_VALUE})
add_definitions(-DUVHTTP_RATE_LIMIT_MAX_REQUESTS=${UVHTTP_RATE_LIMIT_MAX_REQUESTS})
add_definitions(-DUVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS=${UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS})
add_definitions(-DUVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS=${UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS})

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
    add_definitions(-DUVHTTP_FEATURE_LOGGING=1)
    message(STATUS "Development mode: ENABLED (logging enabled)")
else()
    # 生产模式 - 禁用日志系统以提高性能
    # 但在构建测试时自动启用日志功能
    file(GLOB TEST_FILES test/unit/test_*.cpp)
    if(TEST_FILES)
        add_definitions(-DUVHTTP_FEATURE_LOGGING=1)
        message(STATUS "Development mode: DISABLED (logging enabled for testing)")
    else()
        add_definitions(-DUVHTTP_FEATURE_LOGGING=0)
        message(STATUS "Development mode: DISABLED (no logging, zero overhead)")
    endif()
endif()

add_definitions(-DUVHTTP_ALLOCATOR_TYPE=${UVHTTP_ALLOCATOR_TYPE})

if(BUILD_WITH_WEBSOCKET)
    add_definitions(-DUVHTTP_FEATURE_WEBSOCKET=1)
    # WebSocket protocol upgrade requires protocol upgrade framework
    if(NOT UVHTTP_FEATURE_PROTOCOL_UPGRADE)
        message(FATAL_ERROR "WebSocket protocol upgrade requires UVHTTP_FEATURE_PROTOCOL_UPGRADE=ON. "
                           "Please enable protocol upgrade support or disable WebSocket.")
    endif()
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
    src/uvhttp_router_cache.c
    src/uvhttp_connection.c
    src/uvhttp_server.c
    src/uvhttp_tls.c
    src/uvhttp_config.c
    src/uvhttp_context.c
    src/uvhttp_error.c
    src/uvhttp_error_helpers.c
    src/uvhttp_lru_cache.c
    src/uvhttp_static.c
    src/uvhttp_protocol_upgrade.c
    src/uvhttp_version.c
)

# 条件编译 WebSocket 源文件
if(BUILD_WITH_WEBSOCKET)
    list(APPEND SOURCES src/uvhttp_websocket.c)
endif()

# 头文件
set(HEADERS
    include/uvhttp.h
    include/uvhttp_common.h
    include/uvhttp_config.h
    include/uvhttp_constants.h
    include/uvhttp_connection.h
    include/uvhttp_context.h
    include/uvhttp_error.h
    include/uvhttp_error_handler.h
    include/uvhttp_error_helpers.h
    include/uvhttp_features.h
    include/uvhttp_hash.h
    include/uvhttp_lru_cache.h
    include/uvhttp_middleware.h
    include/uvhttp_protocol_upgrade.h
    include/uvhttp_request.h
    include/uvhttp_response.h
    include/uvhttp_router.h
    include/uvhttp_server.h
    include/uvhttp_static.h
    include/uvhttp_tls.h
    include/uvhttp_utils.h
    include/uvhttp_validation.h
    include/uvhttp_allocator.h
)

if(BUILD_WITH_WEBSOCKET)
    list(APPEND HEADERS include/uvhttp_websocket.h)
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
    pthread
    m
    dl
)

if(BUILD_WITH_MIMALLOC)
    list(APPEND LIBS ${MIMALLOC_LIB})
    add_definitions(-DUVHTTP_ENABLE_MIMALLOC)
    add_definitions(-DUVHTTP_FEATURE_ALLOCATOR=1)
endif()

# ============================================================================
# 创建静态库
# ============================================================================
add_library(uvhttp STATIC ${SOURCES})

# 设置静态库属性，减小大小
set_target_properties(uvhttp PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib
    POSITION_INDEPENDENT_CODE ON
)

# 使用现代CMake的target_link_libraries规范方式
# 使用声明库（IMPORTED targets）的方式链接
# PRIVATE: 仅在uvhttp库内部使用，不传递给依赖uvhttp的目标
target_link_libraries(uvhttp
    PRIVATE
        libuv
        mbedtls
        xxhash
        llhttp
)

# mimalloc（PRIVATE）
if(BUILD_WITH_MIMALLOC)
    target_link_libraries(uvhttp PRIVATE mimalloc)
    target_compile_definitions(uvhttp PRIVATE UVHTTP_ENABLE_MIMALLOC UVHTTP_FEATURE_ALLOCATOR=1)
endif()

# Linux 平台特定的库（PRIVATE）
target_link_libraries(uvhttp PRIVATE pthread m dl)

# 设置包含目录（PUBLIC，传递给依赖uvhttp的目标）
target_include_directories(uvhttp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ============================================================================
# 添加 gtest 测试（仅支持 C++ 测试文件）
# ============================================================================
file(GLOB_RECURSE GTEST_TEST_FILES test/unit/test_*.cpp)
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*disabled.*")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_loop_helper.cpp")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_time_mock.cpp")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_.*_mock_coverage.cpp")

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
    
    add_dependencies(${test_name} libuv xxhash gtest)
    if(BUILD_WITH_TLS)
        add_dependencies(${test_name} mbedtls)
    endif()
    
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(${test_name} mimalloc)
    endif()
    # 对测试文件移除 -Werror，允许未使用变量警告
    target_compile_options(${test_name} PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    # 为测试启用统计功能
    target_compile_definitions(${test_name} PRIVATE UVHTTP_FEATURE_STATISTICS=1)
    target_link_libraries(${test_name} PRIVATE ${LIBS} ${GTEST_LIBS})
    target_include_directories(${test_name} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include 
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googlemock/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
    )
endforeach()

# 添加 gtest 示例测试（使用 C++ 编译）
add_executable(uvhttp_unit_tests test/unit/simple_test.cpp ${SOURCES})
set_target_properties(uvhttp_unit_tests PROPERTIES LINKER_LANGUAGE CXX)
add_dependencies(uvhttp_unit_tests libuv xxhash gtest)
if(BUILD_WITH_TLS)
    add_dependencies(uvhttp_unit_tests mbedtls)
endif()
if(BUILD_WITH_MIMALLOC)
    add_dependencies(uvhttp_unit_tests mimalloc)
endif()
target_compile_options(uvhttp_unit_tests PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
target_link_libraries(uvhttp_unit_tests ${LIBS} ${GTEST_LIBS})
target_include_directories(uvhttp_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googlemock/include)

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
    target_compile_options(test_stress PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    target_link_libraries(test_stress ${LIBS})
endif()

# ============================================================================
# 构建示例程序
# ============================================================================
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
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
    add_dependencies(${test_name} libuv xxhash)
    if(BUILD_WITH_TLS)
        add_dependencies(${test_name} mbedtls)
    endif()
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(${test_name} mimalloc)
    endif()
    target_compile_options(${test_name} PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    target_link_libraries(${test_name} ${LIBS})
endforeach()

# ============================================================================
# 基准性能测试
# ============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/benchmark/benchmark.cmake)
    include(${CMAKE_SOURCE_DIR}/benchmark/benchmark.cmake)
endif()

# ============================================================================
# 测试和覆盖率目标
# ============================================================================
if(ENABLE_COVERAGE)
    # 收集所有测试可执行文件
    file(GLOB_RECURSE GTEST_TEST_FILES test/unit/test_*.cpp)
    list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*disabled.*")
    list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_loop_helper.cpp")
    list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_time_mock.cpp")
    list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_.*_mock_coverage.cpp")
    
    set(ALL_TEST_EXECUTABLES)
    foreach(test_file ${GTEST_TEST_FILES})
        get_filename_component(test_name ${test_file} NAME_WE)
        list(APPEND ALL_TEST_EXECUTABLES ${test_name})
    endforeach()
    
    # 创建运行所有测试的脚本
    file(WRITE ${CMAKE_BINARY_DIR}/run_all_tests.sh "#!/bin/bash\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "export LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/dist/lib:$LD_LIBRARY_PATH\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Running all unit tests...\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"\"\n")
    
    # 初始化计数器
    list(LENGTH ALL_TEST_EXECUTABLES TOTAL_TESTS_COUNT)
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TEST_COUNT=0\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TEST_PASSED=0\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TEST_FAILED=0\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TOTAL_TESTS=${TOTAL_TESTS_COUNT}\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"\"\n")
    
    foreach(test_name ${ALL_TEST_EXECUTABLES})
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TEST_COUNT=$((TEST_COUNT + 1))\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"[$TEST_COUNT/$TOTAL_TESTS] Running ${test_name}...\"\n")
        
        # 根据测试类型设置不同的超时时间
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "case \"${test_name}\" in\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    *stress*|*performance*)\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "        TIMEOUT=60\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "        ;;\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    *)\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "        TIMEOUT=30\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "        ;;\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "esac\n")
        
        # 运行测试并记录结果
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "if ${CMAKE_BINARY_DIR}/dist/bin/${test_name}; then\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    TEST_PASSED=$((TEST_PASSED + 1))\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    echo \"✅ PASSED\"\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "else\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    TEST_FAILED=$((TEST_FAILED + 1))\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    echo \"❌ FAILED\"\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "fi\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"\"\n")
    endforeach()
    
    # 添加测试结果汇总
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"=========================================\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Test Summary\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"=========================================\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Total tests: \$TEST_COUNT\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Passed: \$TEST_PASSED\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Failed: \$TEST_FAILED\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"=========================================\"\n")
    
    # 如果有测试失败，返回错误码
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "if [ \$TEST_FAILED -gt 0 ]; then\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    echo \"❌ Some tests failed\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    exit 1\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "else\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    echo \"✅ All tests passed\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    exit 0\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "fi\n")
    
    # 添加测试目标
    add_custom_target(run_tests
        COMMAND chmod +x ${CMAKE_BINARY_DIR}/run_all_tests.sh
        COMMAND ${CMAKE_BINARY_DIR}/run_all_tests.sh
        DEPENDS ${ALL_TEST_EXECUTABLES}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all unit tests"
    )

    # 添加覆盖率目标
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E rm -rf coverage.info coverage_html
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' '*/deps/*' '*/test/*' '*/build/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report"
    )

    # 添加运行测试并生成覆盖率的目标
    add_custom_target(test_and_coverage
        COMMAND chmod +x ${CMAKE_BINARY_DIR}/run_all_tests.sh
        COMMAND ${CMAKE_BINARY_DIR}/run_all_tests.sh
        COMMAND ${CMAKE_COMMAND} -E rm -rf coverage.info coverage_html
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' '*/deps/*' '*/test/*' '*/build/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_html
        DEPENDS ${ALL_TEST_EXECUTABLES}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running tests and generating coverage report"
    )
endif()
