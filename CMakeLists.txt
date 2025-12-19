cmake_minimum_required(VERSION 3.10)
project(uvhttp VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 查找libuv - 手动设置路径
set(UV_INCLUDE_DIRS deps/libuv/include)
set(UV_LIBRARIES deps/libuv/.libs/libuv.a)

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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party/llhttp/include)

# Source files (只包含我们的代码，不依赖libuv)
set(SOURCES
    src/uvhttp_utils.c
    src/uvhttp_request.c
    src/uvhttp_response.c
    src/uvhttp_router.c
    src/uvhttp_connection.c
    src/uvhttp_server_no_tls.c
)

# Header files
set(HEADERS
    include/uvhttp.h
    include/uvhttp_server.h
    include/uvhttp_request.h
    include/uvhttp_response.h
    include/uvhttp_router.h
    include/uvhttp_utils.h
    include/uvhttp_tls.h
    include/uvhttp_connection.h
)

# Create library
add_library(uvhttp STATIC ${SOURCES} ${HEADERS})

# Compiler flags
target_compile_options(uvhttp PRIVATE -Wall -Wextra)

# 使用我们自己的llhttp实现
add_library(llhttp STATIC third_party/llhttp/src/llhttp.c)
target_include_directories(llhttp PUBLIC third_party/llhttp/include)

# 使用修复的gtest框架
add_library(gtest STATIC deps/googletest/gtest_fixed.c)
target_include_directories(gtest PUBLIC deps/googletest)

# 单元测试可执行文件（只测试核心功能）
add_executable(uvhttp_unit_tests 
    tests/test_comprehensive.c
    deps/googletest/gtest_fixed.c
)
target_link_libraries(uvhttp_unit_tests uvhttp gcov m)

# 为测试添加覆盖率标志
if(ENABLE_COVERAGE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(uvhttp_unit_tests PRIVATE --coverage -fprofile-arcs -ftest-coverage)
    endif()
endif()

# 性能测试
add_executable(uvhttp_test tests/test_basic.c deps/googletest/gtest_fixed.c)
target_link_libraries(uvhttp_test uvhttp gcov m)

# libuv集成测试 - 暂时注释
# add_executable(uvhttp_libuv_tests 
#     tests/test_libuv_integration.c
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