###############################################################################
# UVHTTP User Build Options
#
# This file allows users to easily customize UVHTTP build configuration
# without modifying the main CMakeLists.txt
#
# Usage:
#   1. Copy this file to cmake/UserOptions.local.cmake
#   2. Modify the options below as needed
#   3. Run cmake with: cmake -DCMAKE_USER_CONFIG=ON ..
###############################################################################

# =============================================================================
# Build Type
# =============================================================================

# Build type: Debug, Release, RelWithDebInfo, MinSizeRel
set(USER_BUILD_TYPE "Release" CACHE STRING "Build type")

# =============================================================================
# Memory Allocator
# =============================================================================

# Memory allocator type:
#   0 = System allocator (malloc/free)
#   1 = mimalloc (high performance)
#   2 = Custom allocator (requires implementation)
set(USER_ALLOCATOR_TYPE "1" CACHE STRING "Memory allocator type")

# Enable/disable mimalloc (legacy option)
set(USER_BUILD_WITH_MIMALLOC "ON" CACHE BOOL "Enable mimalloc allocator")

# =============================================================================
# Feature Modules
# =============================================================================

# Enable WebSocket support
set(USER_BUILD_WITH_WEBSOCKET "ON" CACHE BOOL "Enable WebSocket support")

# Enable static file serving
set(USER_BUILD_WITH_STATIC_FILES "ON" CACHE BOOL "Enable static file serving")

# Enable rate limiting
set(USER_BUILD_WITH_RATE_LIMIT "ON" CACHE BOOL "Enable rate limiting")

# Enable TLS/SSL support
set(USER_BUILD_WITH_TLS "ON" CACHE BOOL "Enable TLS/SSL support")

# =============================================================================
# Compiler Options
# =============================================================================

# Enable compiler warnings as errors
set(USER_WARNINGS_AS_ERRORS "ON" CACHE BOOL "Treat warnings as errors")

# Enable optimization level
set(USER_OPTIMIZATION_LEVEL "2" CACHE STRING "Optimization level (0-3)")

# Enable debugging symbols
set(USER_ENABLE_DEBUG_SYMBOLS "OFF" CACHE BOOL "Include debugging symbols")

# =============================================================================
# Testing and Coverage
# =============================================================================

# Enable building tests
set(USER_BUILD_TESTS "ON" CACHE BOOL "Build unit tests")

# Enable code coverage (Debug builds only)
set(USER_ENABLE_COVERAGE "OFF" CACHE BOOL "Enable code coverage")

# Enable benchmarking
set(USER_BUILD_BENCHMARKS "ON" CACHE BOOL "Build benchmark programs")

# =============================================================================
# Examples
# =============================================================================

# Enable building examples
set(USER_BUILD_EXAMPLES "ON" CACHE BOOL "Build example programs")

# =============================================================================
# Advanced Options
# =============================================================================

# Enable logging system
set(USER_ENABLE_LOGGING "ON" CACHE BOOL "Enable logging")

# Log level: 0=TRACE, 1=DEBUG, 2=INFO, 3=WARN, 4=ERROR, 5=FATAL
set(USER_LOG_LEVEL "2" CACHE STRING "Log level")

# Enable API validation
set(USER_ENABLE_VALIDATION "ON" CACHE BOOL "Enable API parameter validation")

# =============================================================================
# Installation
# =============================================================================

# Installation prefix
set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix")

# Enable shared library
set(USER_BUILD_SHARED_LIBS "OFF" CACHE BOOL "Build shared library (default is static)")

# =============================================================================
# Platform-Specific Options
# =============================================================================

# 32-bit build (for embedded systems)
set(USER_32BIT_BUILD "OFF" CACHE BOOL "Enable 32-bit build")

# Enable specific optimizations for current CPU
set(USER_ENABLE_CPU_OPTIMIZATIONS "ON" CACHE BOOL "Enable CPU-specific optimizations")

# =============================================================================
# Dependency Versions (if you want to use specific versions)
# =============================================================================

# These are the versions used in the official build
# You can override them if needed
set(USER_LIBUV_VERSION "v1.x")
set(USER_LLHTTP_VERSION "v9.x")
set(USER_MBEDTLS_VERSION "v3.x")
set(USER_MIMALLOC_VERSION "v2.x")

# =============================================================================
# Apply Options
# =============================================================================

# Apply build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "${USER_BUILD_TYPE}")
endif()

# Apply allocator settings
if(USER_ALLOCATOR_TYPE EQUAL "1")
    set(BUILD_WITH_MIMALLOC ON)
elseif(USER_ALLOCATOR_TYPE EQUAL "0")
    set(BUILD_WITH_MIMALLOC OFF)
endif()

# Override with legacy option if set
if(DEFINED USER_BUILD_WITH_MIMALLOC)
    set(BUILD_WITH_MIMALLOC ${USER_BUILD_WITH_MIMALLOC})
endif()

# Apply feature modules
if(NOT DEFINED BUILD_WITH_WEBSOCKET)
    set(BUILD_WITH_WEBSOCKET ${USER_BUILD_WITH_WEBSOCKET})
endif()

if(NOT DEFINED BUILD_WITH_STATIC_FILES)
    set(BUILD_WITH_STATIC_FILES ${USER_BUILD_WITH_STATIC_FILES})
endif()

if(NOT DEFINED BUILD_WITH_RATE_LIMIT)
    set(BUILD_WITH_RATE_LIMIT ${USER_BUILD_WITH_RATE_LIMIT})
endif()

if(NOT DEFINED BUILD_WITH_TLS)
    set(BUILD_WITH_TLS ${USER_BUILD_WITH_TLS})
endif()

# Apply compiler options
if(USER_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
endif()

if(USER_OPTIMIZATION_LEVEL)
    string(APPEND CMAKE_C_FLAGS " -O${USER_OPTIMIZATION_LEVEL}")
endif()

if(USER_ENABLE_DEBUG_SYMBOLS)
    string(APPEND CMAKE_C_FLAGS " -g")
endif()

# Apply 32-bit build
if(USER_32BIT_BUILD)
    string(APPEND CMAKE_C_FLAGS " -m32")
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -m32")
    string(APPEND CMAKE_SHARED_LINKER_FLAGS " -m32")
endif()

# Apply CPU optimizations
if(USER_ENABLE_CPU_OPTIMIZATIONS)
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|i686|i386")
        string(APPEND CMAKE_C_FLAGS " -march=native")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        string(APPEND CMAKE_C_FLAGS " -mcpu=native")
    endif()
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "========================================================================")
message(STATUS " UVHTTP Build Configuration")
message(STATUS "========================================================================")
message(STATUS "Build Type:            ${CMAKE_BUILD_TYPE}")
message(STATUS "Allocator:             ${USER_ALLOCATOR_TYPE} ($BUILD_WITH_MIMALLOC)")
message(STATUS "WebSocket:             ${USER_BUILD_WITH_WEBSOCKET}")
message(STATUS "Static Files:          ${USER_BUILD_WITH_STATIC_FILES}")
message(STATUS "Rate Limit:            ${USER_BUILD_WITH_RATE_LIMIT}")
message(STATUS "TLS Support:           ${USER_BUILD_WITH_TLS}")
message(STATUS "Build Tests:           ${USER_BUILD_TESTS}")
message(STATUS "Build Examples:        ${USER_BUILD_EXAMPLES}")
message(STATUS "Build Benchmarks:      ${USER_BUILD_BENCHMARKS}")
message(STATUS "Enable Coverage:       ${USER_ENABLE_COVERAGE}")
message(STATUS "Enable Logging:        ${USER_ENABLE_LOGGING}")
message(STATUS "32-bit Build:          ${USER_32BIT_BUILD}")
message(STATUS "CPU Optimizations:     ${USER_ENABLE_CPU_OPTIMIZATIONS}")
message(STATUS "Install Prefix:        ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================================================")
message(STATUS "")