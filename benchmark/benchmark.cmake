# UVHTTP 基准性能测试 CMake 配置
#
# 注意：所有性能测试程序必须使用 Release 模式编译，以确保准确的性能数据
# 参考：docs/zh/dev/BUILD_MODES.md

# 性能测试可执行文件
add_executable(performance_allocator
    ${CMAKE_SOURCE_DIR}/benchmark/performance_allocator.c
)
target_link_libraries(performance_allocator
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
# 确保使用 Release 优化
set_target_properties(performance_allocator PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

add_executable(performance_allocator_compare
    ${CMAKE_SOURCE_DIR}/benchmark/performance_allocator_compare.c
)
target_link_libraries(performance_allocator_compare
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(performance_allocator_compare PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

add_executable(test_bitfield
    ${CMAKE_SOURCE_DIR}/benchmark/test_bitfield.c
)
target_link_libraries(test_bitfield
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(test_bitfield PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# RPS 性能测试服务器
add_executable(benchmark_rps
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_rps.c
)
target_link_libraries(benchmark_rps
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_rps PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 延迟测试
add_executable(benchmark_latency
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_latency.c
)
target_link_libraries(benchmark_latency
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_latency PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 连接性能测试
add_executable(benchmark_connection
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_connection.c
)
target_link_libraries(benchmark_connection
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_connection PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 内存性能测试
add_executable(benchmark_memory
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_memory.c
)
target_link_libraries(benchmark_memory
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_memory PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 综合性能测试
add_executable(benchmark_comprehensive
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_comprehensive.c
)
target_link_libraries(benchmark_comprehensive
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_comprehensive PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 文件传输性能测试
add_executable(benchmark_file_transfer
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_file_transfer.c
)
target_link_libraries(benchmark_file_transfer
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_file_transfer PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 路由性能测试
add_executable(benchmark_router
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_router.c
)
target_link_libraries(benchmark_router
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_router PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 简化版路由性能测试
add_executable(benchmark_router_simple
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_router_simple.c
)
target_link_libraries(benchmark_router_simple
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_router_simple PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 路由优化对比测试
add_executable(benchmark_router_comparison
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_router_comparison.c
)
target_link_libraries(benchmark_router_comparison
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_router_comparison PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 简化版路由优化对比测试
add_executable(benchmark_router_simple_comparison
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_router_simple_comparison.c
)
target_link_libraries(benchmark_router_simple_comparison
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_router_simple_comparison PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 最小化路由性能测试
add_executable(benchmark_router_minimal
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_router_minimal.c
)
target_link_libraries(benchmark_router_minimal
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_router_minimal PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 安装性能测试可执行文件
install(TARGETS performance_allocator performance_allocator_compare test_bitfield
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/benchmark
)

install(TARGETS benchmark_rps benchmark_latency benchmark_connection benchmark_memory benchmark_comprehensive benchmark_file_transfer benchmark_router benchmark_router_simple benchmark_router_comparison benchmark_router_simple_comparison benchmark_router_minimal
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/benchmark
)