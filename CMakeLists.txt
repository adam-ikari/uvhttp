cmake_minimum_required(VERSION 3.10)
project(uvhttp VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 内存分配器选择 - CMake配置选项
set(UVHTTP_ALLOCATOR "default" CACHE STRING "Memory allocator type")
set_property(CACHE UVHTTP_ALLOCATOR PROPERTY STRINGS "default" "mimalloc" "custom")

# 根据CMake选项设置编译宏
if(${UVHTTP_ALLOCATOR} STREQUAL "mimalloc")
    # 设置编译宏为枚举值
    add_compile_definitions(UVHTTP_ALLOCATOR_TYPE=1)  # UVHTTP_ALLOCATOR_MIMALLOC
    add_compile_definitions(UVHTTP_ENABLE_MIMALLOC=1)
    
    # 包含mimalloc头文件
    include_directories(deps/mimalloc/include)
    
    # 添加mimalloc作为子目录
    add_subdirectory(mimalloc)
    
    # 链接mimalloc静态库
    set(MIMALLOC_LIB mimalloc-static)
    message(STATUS "Using mimalloc allocator (compile-time optimized)")
    
elseif(${UVHTTP_ALLOCATOR} STREQUAL "custom")
    # 设置编译宏为枚举值
    add_compile_definitions(UVHTTP_ALLOCATOR_TYPE=2)  # UVHTTP_ALLOCATOR_CUSTOM
    message(STATUS "Using custom allocator (runtime selection)")
    
else()
    # 设置编译宏为枚举值
    add_compile_definitions(UVHTTP_ALLOCATOR_TYPE=0)  # UVHTTP_ALLOCATOR_DEFAULT
    message(STATUS "Using default allocator (zero-overhead abstraction)")
endif()

# 编译优化选项
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # 发布版本优化：内联所有分配器调用
    add_definitions(-DUVHTTP_INLINE_OPTIMIZED=1)
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -flto")
    endif()
endif()

# 调试选项
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    option(UVHTTP_DEBUG_ALLOCATOR "Enable allocator debugging" OFF)
    if(UVHTTP_DEBUG_ALLOCATOR)
        add_definitions(-DUVHTTP_DEBUG_ALLOCATOR=1)
        message(STATUS "Allocator debugging enabled")
    endif()
    # 启用调试日志
    add_definitions(-DUVHTTP_DEBUG=1)
    message(STATUS "Debug logging enabled")
else()
    # Release版本禁用调试日志
    add_definitions(-DUVHTTP_DEBUG=1)
endif()

# 功能配置选项 - 使用编译宏控制
option(UVHTTP_ENABLE_CORS "Enable CORS support" ON)
if(UVHTTP_ENABLE_CORS)
    add_definitions(-DUVHTTP_ENABLE_CORS=1)
    message(STATUS "CORS support enabled")
else()
    add_definitions(-DUVHTTP_ENABLE_CORS=0)
endif()

option(UVHTTP_ENABLE_RATE_LIMIT "Enable rate limiting support" ON)
if(UVHTTP_ENABLE_RATE_LIMIT)
    add_definitions(-DUVHTTP_ENABLE_RATE_LIMIT=1)
    message(STATUS "Rate limiting support enabled")
else()
    add_definitions(-DUVHTTP_ENABLE_RATE_LIMIT=0)
endif()

option(UVHTTP_ENABLE_AUTH "Enable authentication support" ON)
if(UVHTTP_ENABLE_AUTH)
    add_definitions(-DUVHTTP_ENABLE_AUTH=1)
    message(STATUS "Authentication support enabled")
else()
    add_definitions(-DUVHTTP_ENABLE_AUTH=0)
endif()

option(UVHTTP_ENABLE_COMPRESSION "Enable compression support" OFF)
if(UVHTTP_ENABLE_COMPRESSION)
    add_definitions(-DUVHTTP_ENABLE_COMPRESSION=1)
    message(STATUS "Compression support enabled")
else()
    add_definitions(-DUVHTTP_ENABLE_COMPRESSION=0)
endif()

# 中间件功能选项
# 编译选项已在上面的统一配置中设置

# 查找libuv - 作为子目录
set(UV_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include)
# 添加 libuv 作为子目录
set(BUILD_TESTING OFF CACHE BOOL "Disable libuv tests" FORCE)
set(LIBUV_BUILD_SHARED OFF CACHE BOOL "Build static libuv" FORCE)
add_subdirectory(deps/libuv)
set(UV_LIBRARIES uv_a)

# 添加mbedtls作为子目录
set(ENABLE_TESTING OFF CACHE BOOL "Disable mbedtls tests" FORCE)
set(ENABLE_PROGRAMS OFF CACHE BOOL "Disable mbedtls programs" FORCE)
add_subdirectory(deps/mbedtls)
set(MBEDTLS_LIBRARIES mbedtls mbedx509 mbedcrypto)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/library)

# 代码覆盖率配置
option(ENABLE_COVERAGE "Enable code coverage" ON)
if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage -lgcov")
    endif()
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/deps/libwebsockets/include)
include_directories(${CMAKE_BINARY_DIR}/deps/libwebsockets/include)  # 添加构建目录
include_directories(${UV_INCLUDE_DIRS})

# Source files (只包含我们的代码，不依赖libuv)
set(SOURCES
    src/uvhttp_utils.c
    src/uvhttp_request.c
    src/uvhttp_response.c
        src/uvhttp_router.c
        src/uvhttp_connection.c
        src/uvhttp_server.c
        src/uvhttp_tls_real.c  # 启用TLS支持（真实OpenSSL实现）
        src/uvhttp_json.c
        
        # src/uvhttp_mimalloc.c  # Temporarily disabled - requires mimalloc
        src/uvhttp_error.c
        src/uvhttp_error_handler.c
    src/uvhttp_websocket_wrapper.c  # 使用包装层实现
)

# Header files
set(HEADERS
    include/uvhttp.h
    include/uvhttp_server.h
    include/uvhttp_request.h
    include/uvhttp_response.h
    include/uvhttp_router.h
    include/uvhttp_utils.h
    include/uvhttp_tls.h  # 启用TLS头文件
    include/uvhttp_connection.h
    
    include/uvhttp_json.h
    include/uvhttp_websocket.h
)

# Create library
add_library(uvhttp STATIC ${SOURCES} ${HEADERS})

# Compiler flags - 启用安全编译选项
target_compile_options(uvhttp PRIVATE 
    -Wall 
    -Wextra 
    -Werror
    -Wformat=2
    -Wformat-security
    -Wnull-dereference
    -Wstack-protector
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
    -fno-strict-overflow
    -Wno-missing-field-initializers
)

# 链接mimalloc（如果启用）
if(MIMALLOC_LIB)
    target_link_libraries(uvhttp PRIVATE ${MIMALLOC_LIB})
endif()

# 添加cjson支持 - 使用预编译方式
find_path(CJSON_INCLUDE_DIR cJSON.h 
          PATHS ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson)
find_library(CJSON_LIBRARY cjson 
             PATHS ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson)

if(CJSON_INCLUDE_DIR AND CJSON_LIBRARY)
    target_include_directories(uvhttp PRIVATE ${CJSON_INCLUDE_DIR})
    target_link_libraries(uvhttp PRIVATE ${CJSON_LIBRARY})
    message(STATUS "Found cjson: ${CJSON_LIBRARY}")
else()
    # 如果没有找到预编译库，尝试添加子目录
    add_subdirectory(deps/cjson)
    target_link_libraries(uvhttp PRIVATE cjson)
    message(STATUS "Using cjson from subdirectory")
endif()

# 添加libwebsockets支持 - 使用预编译方式
find_path(LIBWEBSOCKETS_INCLUDE_DIR libwebsockets.h 
          PATHS ${CMAKE_CURRENT_SOURCE_DIR}/deps/libwebsockets/include)
find_library(LIBWEBSOCKETS_LIBRARY websockets 
             PATHS ${CMAKE_CURRENT_SOURCE_DIR}/deps/libwebsockets/lib)

if(LIBWEBSOCKETS_INCLUDE_DIR AND LIBWEBSOCKETS_LIBRARY)
    target_include_directories(uvhttp PRIVATE ${LIBWEBSOCKETS_INCLUDE_DIR})
    target_link_libraries(uvhttp PRIVATE ${LIBWEBSOCKETS_LIBRARY})
    message(STATUS "Found libwebsockets: ${LIBWEBSOCKETS_LIBRARY}")
else()
    # 如果没有找到预编译库，尝试添加子目录
    # 设置 libwebsockets 选项以避免交互式证书生成
    # 保存当前的 SOURCES 变量
    set(_SAVED_SOURCES ${SOURCES})
    unset(SOURCES)
    
    # 设置 libwebsockets 选项
    set(LWS_WITHOUT_TESTAPPS ON CACHE BOOL "Don't build the libwebsocket-test-apps")
    set(LWS_WITHOUT_TEST_SERVER ON CACHE BOOL "Don't build the test server")
    set(LWS_WITHOUT_TEST_CLIENT ON CACHE BOOL "Don't build the client test application")
    
    # 添加 libwebsockets 子目录
    add_subdirectory(deps/libwebsockets)
    
    # 恢复 SOURCES 变量
    set(SOURCES ${_SAVED_SOURCES})
    unset(_SAVED_SOURCES)
    
    target_link_libraries(uvhttp PRIVATE websockets)
endif()

# 使用我们自己的llhttp实现
set(LLHTTP_SOURCES
    deps/cllhttp/llhttp.c
    deps/cllhttp/api.c
)

# 创建 llhttp 静态库
add_library(llhttp STATIC ${LLHTTP_SOURCES})

# 将llhttp链接到主库
target_link_libraries(uvhttp PRIVATE llhttp)

# 使用 googletest 作为子目录
add_subdirectory(deps/googletest)

# 单元测试可执行文件
add_executable(uvhttp_unit_tests 
    test/uvhttp_test_framework.c
    test/test_utils.c
    test/test_response.c
    test/test_request.c
    test/test_router.c
    test/test_main.c
    ${SOURCES}
)

# 性能测试
add_executable(uvhttp_test test/simple_test.c)

# 设置链接库
set(TEST_LIBS uvhttp llhttp cjson websockets gtest gtest_main ${UV_LIBRARIES} dl pthread m ssl crypto)
if(APPLE)
    list(APPEND TEST_LIBS /opt/homebrew/opt/openssl@3/lib/libssl.dylib /opt/homebrew/opt/openssl@3/lib/libcrypto.dylib)
else()
    list(APPEND TEST_LIBS ssl crypto)
endif()

# 添加 libwebsockets 库路径
link_directories(${CMAKE_BINARY_DIR}/deps/libwebsockets/lib)

# 链接mimalloc（如果启用）
if(MIMALLOC_LIB)
    target_link_libraries(uvhttp PRIVATE ${MIMALLOC_LIB})
    list(APPEND TEST_LIBS ${MIMALLOC_LIB})
endif()

# 链接OpenSSL库
target_link_libraries(uvhttp PRIVATE ssl crypto)

# Hello World 示例
add_executable(helloworld examples/helloworld.c)
target_link_libraries(helloworld uvhttp llhttp cjson websockets ${UV_LIBRARIES} gtest_main dl pthread m ssl crypto)

add_executable(simple_test examples/simple_test.c)
target_link_libraries(simple_test uvhttp llhttp cjson websockets ${UV_LIBRARIES} gtest_main dl pthread m ssl crypto)

# 链接测试库
target_link_libraries(uvhttp_unit_tests ${TEST_LIBS})
target_link_libraries(uvhttp_test ${TEST_LIBS})
# target_link_libraries(test_websocket_basic ${TEST_LIBS})
# target_link_libraries(test_websocket_integration ${TEST_LIBS})
# target_link_libraries(test_websocket_stress ${TEST_LIBS})
# target_link_libraries(test_websocket_performance ${TEST_LIBS})

# libuv集成测试 - 暂时注释
# add_executable(uvhttp_libuv_tests 
#     test/test_libuv_integration.c
#     deps/googletest/gtest_fixed.c
# )
# target_link_libraries(uvhttp_libuv_tests uvhttp ${UV_LIBRARIES} gcov m)

# 为libuv测试添加覆盖率标志
# if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
#     if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
#         target_compile_options(uvhttp_libuv_tests PRIVATE --coverage -fprofile-arcs -ftest-coverage)
#     endif()
# endif()

# 添加自定义目标用于生成覆盖率报告
if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_custom_target(coverage
            COMMAND lcov --capture --directory . --output-file coverage.info
            COMMAND lcov --remove coverage.info '/usr/*' --output-file coverage.info
            COMMAND lcov --list coverage.info
            COMMAND genhtml coverage.info --output-directory coverage_html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating code coverage report"
        )
        
        # 添加清理覆盖率的目标
        add_custom_target(coverage-clean
            COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcda" -delete
            COMMAND find ${CMAKE_BINARY_DIR} -name "*.gcno" -delete
            COMMAND find ${CMAKE_BINARY_DIR} -name "coverage.info" -delete
            COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage_html
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Cleaning coverage files"
        )
    endif()
endif()

# 安装规则
install(TARGETS uvhttp DESTINATION lib)
install(FILES ${HEADERS} DESTINATION include/uvhttp)