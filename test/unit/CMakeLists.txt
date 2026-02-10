# 单元测试 CMake 配置

# 收集所有测试文件
file(GLOB_RECURSE UNIT_TEST_FILES test/unit/test_*.cpp)
list(FILTER UNIT_TEST_FILES EXCLUDE REGEX ".*disabled.*")
list(FILTER UNIT_TEST_FILES EXCLUDE REGEX ".*test_loop_helper.cpp")
list(FILTER UNIT_TEST_FILES EXCLUDE REGEX ".*test_time_mock.cpp")

# Mock 测试文件（使用 libuv mock）
set(MOCK_TEST_FILES
    test/unit/test_connection_lifecycle_mock.cpp
    test/unit/test_cmocka_simple.cpp
)

# 添加 libuv mock 库
add_library(libuv_mock STATIC
    test/mock/libuv_mock.c
)
target_include_directories(libuv_mock PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/test/mock
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
)

# 编译 mock 测试
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

# 添加到 CTest
foreach(test_file ${MOCK_TEST_FILES})
    get_filename_component(test_name ${test_file} NAME_WE)
    add_test(NAME ${test_name} COMMAND ${CMAKE_BINARY_DIR}/dist/bin/${test_name})
endforeach()