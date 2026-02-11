cmake_minimum_required(VERSION 3.10)

# Read version configuration
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" VERSION_FILE)
    string(REGEX MATCH "VERSION=([0-9]+\\.[0-9]+\\.[0-9]+)" _ ${VERSION_FILE})
    set(PROJECT_VERSION ${CMAKE_MATCH_1})
else()
    set(PROJECT_VERSION "2.3.0")
endif()

project(uvhttp VERSION ${PROJECT_VERSION} LANGUAGES C CXX)

# ========== Feature Options ==========
# These options control which features are compiled into the library
option(BUILD_WITH_WEBSOCKET "Build with WebSocket support" ON)
option(BUILD_WITH_HTTPS "Build with HTTPS support" ON)
option(BUILD_WITH_STATIC_FILES "Build with static file service support" OFF)
option(BUILD_WITH_LRU_CACHE "Build with LRU cache support" ON)
option(BUILD_WITH_ROUTER_CACHE "Build with router cache support" OFF)

# Memory allocator type (must be set before BUILD_WITH_MIMALLOC option)
if(NOT DEFINED UVHTTP_ALLOCATOR_TYPE)
    set(UVHTTP_ALLOCATOR_TYPE 0)
endif()

# Validate UVHTTP_ALLOCATOR_TYPE value
if(NOT UVHTTP_ALLOCATOR_TYPE EQUAL 0 AND NOT UVHTTP_ALLOCATOR_TYPE EQUAL 1 AND NOT UVHTTP_ALLOCATOR_TYPE EQUAL 2)
    message(FATAL_ERROR "UVHTTP_ALLOCATOR_TYPE must be 0 (system), 1 (mimalloc), or 2 (mimalloc auto), got: ${UVHTTP_ALLOCATOR_TYPE}")
endif()

# Set default BUILD_WITH_MIMALLOC based on UVHTTP_ALLOCATOR_TYPE
if(NOT DEFINED BUILD_WITH_MIMALLOC)
    if(UVHTTP_ALLOCATOR_TYPE EQUAL 1)
        set(BUILD_WITH_MIMALLOC ON)
    else()
        # UVHTTP_ALLOCATOR_TYPE=0 (system) or 2 (custom) - don't enable mimalloc by default
        set(BUILD_WITH_MIMALLOC OFF)
    endif()
endif()

option(BUILD_WITH_MIMALLOC "Build with mimalloc allocator" ${BUILD_WITH_MIMALLOC})

# Validate configuration consistency
if(UVHTTP_ALLOCATOR_TYPE EQUAL 1 AND NOT BUILD_WITH_MIMALLOC)
    message(FATAL_ERROR "UVHTTP_ALLOCATOR_TYPE=1 (mimalloc) requires BUILD_WITH_MIMALLOC=ON. "
                       "Set BUILD_WITH_MIMALLOC=ON or use UVHTTP_ALLOCATOR_TYPE=0 (system) or 2 (custom).")
endif()

if(UVHTTP_ALLOCATOR_TYPE EQUAL 2 AND BUILD_WITH_MIMALLOC)
    message(WARNING "UVHTTP_ALLOCATOR_TYPE=2 (custom allocator) but BUILD_WITH_MIMALLOC=ON. "
                   "Custom allocator will be used instead of mimalloc.")
endif()

option(BUILD_EXAMPLES "Build example programs" OFF)
option(BUILD_BENCHMARKS "Build performance benchmark programs" ON)

# ========== Build Mode Options ==========
# These options control the build mode (debug, release, coverage)
option(ENABLE_DEBUG "Enable debug build with -O0" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_DEV_MODE "Enable development mode with logging" OFF)

# ========== Optimization Options ==========
# These options control performance optimizations
option(ENABLE_LTO "Enable Link Time Optimization (LTO)" OFF)
option(ENABLE_PGO "Enable Profile Guided Optimization (PGO)" OFF)
option(ENABLE_FASTER_MATH "Enable faster math optimizations" OFF)

# ========== Debugging Tools ==========
# These options enable runtime error detection tools (sanitizers)
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_VALGRIND "Enable Valgrind support" OFF)

# ========== Logging Options ==========
# These options control logging verbosity
option(ENABLE_LOG_DEBUG "Enable debug log level" OFF)
option(ENABLE_LOG_TRACE "Enable trace log level" OFF)
option(ENABLE_LOG_PERFORMANCE "Enable performance logging" OFF)

# ========== Security Options ==========
# These options enable security hardening features
option(ENABLE_HARDENING "Enable security hardening" ON)
option(ENABLE_STACK_PROTECTION "Enable stack protection" ON)
option(ENABLE_FORTIFY_SOURCE "Enable _FORTIFY_SOURCE" ON)

# Detect 32-bit system
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

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler options
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG -ffunction-sections -fdata-sections")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -s")
endif()

# Explicitly set Release mode compiler options (override CMake default -O3)
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)

if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()

# Security compiler options
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
")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -Wl,-z,relro \
    -Wl,-z,now \
")

# ========== Feature Definitions Based on Options ==========
# Static files support
if(BUILD_WITH_STATIC_FILES)
    add_definitions(-DUVHTTP_FEATURE_STATIC_FILES=1)
    message(STATUS "Static files support: ENABLED")
else()
    add_definitions(-DUVHTTP_FEATURE_STATIC_FILES=0)
    message(STATUS "Static files support: DISABLED")
endif()

# LRU cache support
if(BUILD_WITH_LRU_CACHE)
    add_definitions(-DUVHTTP_FEATURE_LRU_CACHE=1)
    message(STATUS "LRU cache support: ENABLED")
else()
    add_definitions(-DUVHTTP_FEATURE_LRU_CACHE=0)
    message(STATUS "LRU cache support: DISABLED (reduced memory usage)")
endif()

# Router cache support
if(BUILD_WITH_ROUTER_CACHE)
    add_definitions(-DUVHTTP_FEATURE_ROUTER_CACHE=1)
    message(STATUS "Router cache support: ENABLED")
else()
    add_definitions(-DUVHTTP_FEATURE_ROUTER_CACHE=0)
    message(STATUS "Router cache support: DISABLED")
endif()

# HTTPS support (using mbedtls)
if(BUILD_WITH_HTTPS)
    add_definitions(-DUVHTTP_FEATURE_TLS=1)
    message(STATUS "HTTPS support: ENABLED")
else()
    message(STATUS "HTTPS support: DISABLED")
endif()

# WebSocket support
# Note: WebSocket automatically enables TLS support (UVHTTP_FEATURE_TLS=1)
# because WebSocket needs mbedtls for SHA1/Base64 encoding
if(BUILD_WITH_WEBSOCKET)
    add_definitions(-DUVHTTP_FEATURE_WEBSOCKET=1)
    add_definitions(-DUVHTTP_FEATURE_TLS=1)
    message(STATUS "WebSocket support: ENABLED (TLS automatically enabled)")
else()
    add_definitions(-DUVHTTP_FEATURE_WEBSOCKET=0)
    message(STATUS "WebSocket support: DISABLED")
endif()

# Compile mbedtls if either HTTPS or WebSocket is enabled
if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
    message(STATUS "TLS library (mbedtls): ENABLED")
else()
    add_definitions(-DUVHTTP_FEATURE_TLS=0)
    message(STATUS "TLS library (mbedtls): DISABLED")
endif()

add_definitions(-DUVHTTP_ALLOCATOR_TYPE=${UVHTTP_ALLOCATOR_TYPE})

# ========== Apply Performance Optimization Options ==========
if(ENABLE_LTO)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    message(STATUS "Link Time Optimization (LTO) enabled")
endif()

if(ENABLE_FASTER_MATH)
    add_compile_options(-ffast-math)
    message(STATUS "Fast math optimizations enabled")
endif()

# ========== Apply Debug Options ==========
# Sanitizer conflict validation
set(ENABLED_SANITIZERS 0)
if(ENABLE_ASAN)
    math(EXPR ENABLED_SANITIZERS "${ENABLED_SANITIZERS}+1")
endif()
if(ENABLE_UBSAN)
    math(EXPR ENABLED_SANITIZERS "${ENABLED_SANITIZERS}+1")
endif()
if(ENABLE_TSAN)
    math(EXPR ENABLED_SANITIZERS "${ENABLED_SANITIZERS}+1")
endif()

if(ENABLED_SANITIZERS GREATER 1)
    message(FATAL_ERROR "Cannot enable multiple sanitizers simultaneously. "
                       "Please choose only one of: ENABLE_ASAN, ENABLE_UBSAN, or ENABLE_TSAN")
endif()

# LTO conflicts with sanitizers
if(ENABLE_LTO AND (ENABLE_ASAN OR ENABLE_UBSAN OR ENABLE_TSAN))
    message(WARNING "LTO with sanitizers may cause compilation or runtime issues. "
                   "Consider disabling ENABLE_LTO when using sanitizers.")
endif()

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "AddressSanitizer enabled")
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
endif()

if(ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
    message(STATUS "ThreadSanitizer enabled")
endif()

# ========== Apply Logging Options ==========
if(ENABLE_LOG_DEBUG)
    add_definitions(-DUVHTTP_LOG_LEVEL_DEBUG=1)
    message(STATUS "Debug log level enabled")
endif()

if(ENABLE_LOG_TRACE)
    add_definitions(-DUVHTTP_LOG_LEVEL_TRACE=1)
    message(STATUS "Trace log level enabled")
endif()

if(ENABLE_LOG_PERFORMANCE)
    add_definitions(-DUVHTTP_LOG_PERFORMANCE=1)
    message(STATUS "Performance logging enabled")
endif()

# ========== Apply Security Options ==========
if(ENABLE_HARDENING)
    add_compile_options(-fstack-protector-strong -D_FORTIFY_SOURCE=2)
    add_link_options(-Wl,-z,relro,-z,now)
    message(STATUS "Security hardening enabled")
endif()

if(ENABLE_STACK_PROTECTION)
    add_compile_options(-fstack-protector-all)
    message(STATUS "Stack protection enabled")
endif()

if(ENABLE_FORTIFY_SOURCE)
    add_compile_options(-D_FORTIFY_SOURCE=2)
    message(STATUS "_FORTIFY_SOURCE enabled")
endif()

# ========== Configurable Constant Options ==========
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

# Define options as compile macros
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

# ========== Development Mode - Enable Logging ==========
option(ENABLE_DEV_MODE "Enable development mode with logging" OFF)
if(ENABLE_DEV_MODE)
    add_definitions(-DUVHTTP_DEV_MODE)
    add_definitions(-DUVHTTP_FEATURE_LOGGING=1)
    message(STATUS "Development mode: ENABLED (logging enabled)")
else()
    # Production mode - disable logging for performance (unless DEBUG build)
    if(ENABLE_DEBUG)
        add_definitions(-DUVHTTP_FEATURE_LOGGING=1)
        message(STATUS "Development mode: DISABLED (logging enabled for DEBUG build)")
    else()
        add_definitions(-DUVHTTP_FEATURE_LOGGING=0)
        message(STATUS "Development mode: DISABLED (no logging, zero overhead)")
    endif()
endif()

# ============================================================================
# Include dependency build configuration
# ============================================================================
include(cmake/Dependencies.cmake)

# Add dependency include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${DEPS_INCLUDE_DIRS})

# Source files
set(SOURCES
    src/uvhttp_utils.c
    src/uvhttp_request.c
    src/uvhttp_response.c
    src/uvhttp_router.c
    src/uvhttp_router_cache.c
    src/uvhttp_connection.c
    src/uvhttp_server.c
    src/uvhttp_config.c
    src/uvhttp_context.c
    src/uvhttp_error.c
    src/uvhttp_error_helpers.c
    src/uvhttp_lru_cache.c
    src/uvhttp_static.c
    src/uvhttp_protocol_upgrade.c
    src/uvhttp_version.c
)

# Conditionally compile TLS source files
if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
    list(APPEND SOURCES src/uvhttp_tls.c)
endif()

# Conditionally compile WebSocket source files
if(BUILD_WITH_WEBSOCKET)
    list(APPEND SOURCES src/uvhttp_websocket.c)
endif()

# Header files
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
# Setup dependency libraries
# ============================================================================
set(LIBS
    ${LIBUV_LIB}
    ${XXHASH_LIB}
    ${LLHTTP_LIB}
    ${CJSON_LIB}
    pthread
    m
    dl
)

if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
    list(APPEND LIBS ${MBEDTLS_LIBS})
endif()

if(BUILD_WITH_MIMALLOC)
    list(APPEND LIBS ${MIMALLOC_LIB})
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

# Use modern CMake target_link_libraries specification
# Use IMPORTED targets approach for linking
# PRIVATE: Only used internally in uvhttp library, not passed to targets that depend on uvhttp
target_link_libraries(uvhttp
    PRIVATE
        libuv
        xxhash
        llhttp
)

# TLS library support (mbedtls)
if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
    target_link_libraries(uvhttp PRIVATE mbedtls)
    target_compile_definitions(uvhttp PRIVATE UVHTTP_ENABLE_TLS)
endif()

# mimalloc (PRIVATE)
if(BUILD_WITH_MIMALLOC)
    target_link_libraries(uvhttp PRIVATE mimalloc)
    target_compile_definitions(uvhttp PRIVATE UVHTTP_ENABLE_MIMALLOC)
endif()

# Linux platform-specific libraries (PRIVATE)
target_link_libraries(uvhttp PRIVATE pthread m dl)

# Set include directories (PUBLIC, passed to targets that depend on uvhttp)
target_include_directories(uvhttp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ============================================================================
# Add libuv mock library for testing
# ============================================================================
add_library(libuv_mock STATIC
    test/mock/libuv_mock.c
)
target_include_directories(libuv_mock PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/test/mock
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
)

# ============================================================================
# Add gtest tests (C++ test files only)
# ============================================================================
file(GLOB_RECURSE GTEST_TEST_FILES test/unit/test_*.cpp)
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*disabled.*")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_loop_helper.cpp")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_time_mock.cpp")
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_.*_mock_coverage.cpp")
# 排除 mock 测试文件（它们在单独的循环中处理）
list(FILTER GTEST_TEST_FILES EXCLUDE REGEX ".*test_.*_mock\\.cpp")

# Add test helper files
set(TEST_HELPER_FILES test/unit/test_time_mock.cpp test/unit/test_loop_helper.cpp)

foreach(test_file ${GTEST_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)

    # WebSocket-related tests require WebSocket support
    if(${test_name} STREQUAL "test_connection_api_coverage" OR ${test_name} STREQUAL "test_connection_enhanced_coverage" OR ${test_name} STREQUAL "test_connection_full_api_coverage")
        if(NOT BUILD_WITH_WEBSOCKET)
            message(WARNING "Test ${test_name} requires WebSocket support, skipping...")
            continue()
        endif()
    endif()
    if(${test_name} MATCHES ".*websocket.*")
        if(NOT BUILD_WITH_WEBSOCKET)
            message(WARNING "Test ${test_name} requires WebSocket support, skipping...")
            continue()
        endif()
    endif()

    # TLS-related tests require HTTPS support
    # Server TLS API tests require HTTPS support
    if(${test_name} STREQUAL "test_server_api_coverage" OR ${test_name} STREQUAL "test_server_error_coverage" OR ${test_name} STREQUAL "test_server_simple_api_coverage")
        if(NOT BUILD_WITH_HTTPS)
            message(WARNING "Test ${test_name} requires HTTPS support, skipping...")
            continue()
        endif()
    endif()
    if(${test_name} MATCHES ".*tls.*")
        if(NOT BUILD_WITH_HTTPS)
            message(WARNING "Test ${test_name} requires HTTPS support, skipping...")
            continue()
        endif()
    endif()
    
    # All test files are C++ files
    # Add helper files if tests need time mock
    if(${test_name} STREQUAL "test_lru_cache_full_coverage" OR 
       ${test_name} STREQUAL "test_static_file_operations_coverage")
        add_executable(${test_name} ${test_file} ${TEST_HELPER_FILES} ${SOURCES})
    else()
        add_executable(${test_name} ${test_file} ${SOURCES})
    endif()
    
    set_target_properties(${test_name} PROPERTIES LINKER_LANGUAGE CXX)
    
    add_dependencies(${test_name} libuv xxhash gtest)
    if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
        add_dependencies(${test_name} mbedtls)
    endif()
    
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(${test_name} mimalloc)
    endif()
    # Remove -Werror for test files, allow unused variable warnings
    target_compile_options(${test_name} PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    # Enable statistics for tests
    target_compile_definitions(${test_name} PRIVATE UVHTTP_FEATURE_STATISTICS=1)
    # Link googletest first, then LIBS (gmock_main depends on gtest)
    target_link_libraries(${test_name} PRIVATE ${GTEST_LIBS} ${LIBS})
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googlemock/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
    )
endforeach()

# ============================================================================
# Compile mock tests with libuv mock library
# ============================================================================
foreach(test_file ${MOCK_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    
    add_executable(${test_name} ${test_file} ${SOURCES})
    set_target_properties(${test_name} PROPERTIES LINKER_LANGUAGE CXX)
    
    # 链接 mock 库
    target_link_libraries(${test_name} PRIVATE
        ${GTEST_LIBS}
        ${LIBS}
        libuv_mock
    )
    
    # 添加链接器 wrap 选项
    target_link_options(${test_name} PRIVATE
        -Wl,--wrap=uv_loop_init
        -Wl,--wrap=uv_loop_close
        -Wl,--wrap=uv_run
        -Wl,--wrap=uv_tcp_init
        -Wl,--wrap=uv_tcp_bind
        -Wl,--wrap=uv_listen
        -Wl,--wrap=uv_tcp_accept
        -Wl,--wrap=uv_read_start
        -Wl,--wrap=uv_read_stop
        -Wl,--wrap=uv_write
        -Wl,--wrap=uv_close
        -Wl,--wrap=uv_is_active
        -Wl,--wrap=uv_is_closing
        -Wl,--wrap=uv_idle_init
        -Wl,--wrap=uv_timer_init
    )
    
    add_dependencies(${test_name} libuv xxhash gtest libuv_mock)
    if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
        add_dependencies(${test_name} mbedtls)
    endif()
    
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(${test_name} mimalloc)
    endif()
    
    target_compile_options(${test_name} PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
    target_compile_definitions(${test_name} PRIVATE UVHTTP_FEATURE_STATISTICS=1)
    
    target_include_directories(${test_name} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googlemock/include
        ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
        ${CMAKE_CURRENT_SOURCE_DIR}/test/mock
    )
endforeach()

# Add gtest sample test (compiled with C++)
add_executable(uvhttp_unit_tests test/unit/simple_test.cpp ${SOURCES})
set_target_properties(uvhttp_unit_tests PROPERTIES LINKER_LANGUAGE CXX)
add_dependencies(uvhttp_unit_tests libuv xxhash gtest)
if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
    add_dependencies(uvhttp_unit_tests mbedtls)
endif()
if(BUILD_WITH_MIMALLOC)
    add_dependencies(uvhttp_unit_tests mimalloc)
endif()
target_compile_options(uvhttp_unit_tests PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=unused-function)
# Link googletest first, then LIBS (gmock_main depends on gtest)
target_link_libraries(uvhttp_unit_tests ${GTEST_LIBS} ${LIBS})
target_include_directories(uvhttp_unit_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googlemock/include)

# ============================================================================
# CTest Configuration
# ============================================================================
enable_testing()

# Add unit tests to CTest
foreach(test_file ${GTEST_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(NAME ${test_name} COMMAND ${CMAKE_BINARY_DIR}/dist/bin/${test_name})
endforeach()

# Add gtest sample test to CTest
add_test(NAME uvhttp_unit_tests COMMAND ${CMAKE_BINARY_DIR}/dist/bin/uvhttp_unit_tests)

# ============================================================================
# CMocka + GTest Integration Tests
# ============================================================================
# Note: CMocka has been removed because linker wrap mechanism is incompatible with existing code
# For mock functionality, consider other testing strategies

# Add stress tests
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
# Build example programs
# ============================================================================
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# ============================================================================
# Print configuration summary
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
message(STATUS "HTTPS Support: ${BUILD_WITH_HTTPS}")
if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
    message(STATUS "TLS Library (mbedtls): ENABLED")
else()
    message(STATUS "TLS Library (mbedtls): DISABLED")
endif()
message(STATUS "Middleware Support: ${UVHTTP_FEATURE_MIDDLEWARE}")
message(STATUS "Debug Mode: ${ENABLE_DEBUG}")
message(STATUS "Coverage: ${ENABLE_COVERAGE}")
message(STATUS "Examples: ${BUILD_EXAMPLES}")
message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# Integration tests
# ============================================================================
file(GLOB INTEGRATION_TEST_FILES test/integration/*.c)
foreach(test_file ${INTEGRATION_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)

    # TLS-related tests require HTTPS support
    # Server TLS API tests require HTTPS support
    if(${test_name} STREQUAL "test_server_api_coverage" OR ${test_name} STREQUAL "test_server_error_coverage" OR ${test_name} STREQUAL "test_server_simple_api_coverage")
        if(NOT BUILD_WITH_HTTPS)
            message(WARNING "Test ${test_name} requires HTTPS support, skipping...")
            continue()
        endif()
    endif()
    if(${test_name} MATCHES ".*tls.*")
        if(NOT BUILD_WITH_HTTPS)
            message(WARNING "Test ${test_name} requires HTTPS support, skipping...")
            continue()
        endif()
    endif()

    # WebSocket-related tests require WebSocket support
    if(${test_name} MATCHES ".*websocket.*")
        if(NOT BUILD_WITH_WEBSOCKET)
            message(WARNING "Test ${test_name} requires WebSocket support, skipping...")
            continue()
        endif()
    endif()

    add_executable(${test_name} ${test_file} ${SOURCES})
    add_dependencies(${test_name} libuv xxhash)
    if(BUILD_WITH_HTTPS OR BUILD_WITH_WEBSOCKET)
        add_dependencies(${test_name} mbedtls)
    endif()
    if(BUILD_WITH_MIMALLOC)
        add_dependencies(${test_name} mimalloc)
    endif()

    target_compile_options(${test_name} PRIVATE -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error-unused-function)
    target_link_libraries(${test_name} ${LIBS})
endforeach()

# ============================================================================
# Performance benchmarks
# ============================================================================
if(EXISTS ${CMAKE_SOURCE_DIR}/benchmark/benchmark.cmake)
    include(${CMAKE_SOURCE_DIR}/benchmark/benchmark.cmake)
endif()

# ============================================================================
# Test and coverage targets
# ============================================================================
if(ENABLE_COVERAGE)
    # Collect all test executables
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
    
    # Create script to run all tests
    file(WRITE ${CMAKE_BINARY_DIR}/run_all_tests.sh "#!/bin/bash\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "export LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/dist/lib:$LD_LIBRARY_PATH\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Running all unit tests...\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"\"\n")
    
    # Initialize counters
    list(LENGTH ALL_TEST_EXECUTABLES TOTAL_TESTS_COUNT)
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TEST_COUNT=0\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TEST_PASSED=0\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TEST_FAILED=0\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TOTAL_TESTS=${TOTAL_TESTS_COUNT}\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"\"\n")
    
    foreach(test_name ${ALL_TEST_EXECUTABLES})
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "TEST_COUNT=$((TEST_COUNT + 1))\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"[$TEST_COUNT/$TOTAL_TESTS] Running ${test_name}...\"\n")
        
        # Set different timeout based on test type
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "case \"${test_name}\" in\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    *stress*|*performance*)\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "        TIMEOUT=60\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "        ;;\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    *)\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "        TIMEOUT=30\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "        ;;\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "esac\n")
        
        # Run test and record result
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "if ${CMAKE_BINARY_DIR}/dist/bin/${test_name}; then\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    TEST_PASSED=$((TEST_PASSED + 1))\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    echo \"PASSED\"\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "else\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    TEST_FAILED=$((TEST_FAILED + 1))\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    echo \"FAILED\"\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "fi\n")
        file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"\"\n")
    endforeach()
    
    # Add test result summary
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"=========================================\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Test Summary\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"=========================================\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Total tests: \$TEST_COUNT\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Passed: \$TEST_PASSED\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"Failed: \$TEST_FAILED\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "echo \"=========================================\"\n")
    
    # Return error code if any test failed
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "if [ \$TEST_FAILED -gt 0 ]; then\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    echo \"Some tests failed\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    exit 1\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "else\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    echo \"All tests passed\"\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "    exit 0\n")
    file(APPEND ${CMAKE_BINARY_DIR}/run_all_tests.sh "fi\n")
    
    # Add test target
    add_custom_target(run_tests
        COMMAND chmod +x ${CMAKE_BINARY_DIR}/run_all_tests.sh
        COMMAND ${CMAKE_BINARY_DIR}/run_all_tests.sh
        DEPENDS ${ALL_TEST_EXECUTABLES}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Running all unit tests"
    )

    # Add coverage target
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E rm -rf coverage.info coverage_html
        COMMAND lcov --capture --directory . --output-file coverage.info
        COMMAND lcov --remove coverage.info '/usr/*' '*/deps/*' '*/test/*' '*/build/*' --output-file coverage.info
        COMMAND lcov --list coverage.info
        COMMAND genhtml coverage.info --output-directory coverage_html
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating coverage report"
    )

    # Add target to run tests and generate coverage
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
