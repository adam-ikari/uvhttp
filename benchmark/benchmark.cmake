# UVHTTP 基准性能测试 CMake 配置

# 性能测试可执行文件
add_executable(performance_allocator
    ${CMAKE_SOURCE_DIR}/benchmark/performance_allocator.c
)
target_link_libraries(performance_allocator
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

add_executable(performance_allocator_compare
    ${CMAKE_SOURCE_DIR}/benchmark/performance_allocator_compare.c
)
target_link_libraries(performance_allocator_compare
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

add_executable(test_bitfield
    ${CMAKE_SOURCE_DIR}/benchmark/test_bitfield.c
)
target_link_libraries(test_bitfield
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

# 新增基准测试程序
add_executable(benchmark_rps
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_rps.c
)
target_link_libraries(benchmark_rps
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

add_executable(benchmark_latency
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_latency.c
)
target_link_libraries(benchmark_latency
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

add_executable(benchmark_connection
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_connection.c
)
target_link_libraries(benchmark_connection
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

add_executable(benchmark_memory
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_memory.c
)
target_link_libraries(benchmark_memory
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

add_executable(benchmark_comprehensive
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_comprehensive.c
)
target_link_libraries(benchmark_comprehensive
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

# 安装性能测试可执行文件
install(TARGETS performance_allocator performance_allocator_compare test_bitfield
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/benchmark
)

install(TARGETS benchmark_rps benchmark_latency benchmark_connection benchmark_memory benchmark_comprehensive
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/benchmark
)