# UVHTTP 基准性能测试 CMake 配置
#
# 注意：性能测试程序必须使用 Release 模式编译，以确保准确的性能数据
# 参考：docs/zh/dev/BUILD_MODES.md
#
# 编译性能测试的正确方式：
#   cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_COVERAGE=OFF .
#   make benchmark_unified
#
# 不要使用 Debug 模式编译性能测试，因为：
# - Debug 模式会禁用编译器优化（-O0）
# - Debug 模式会添加调试符号（-g），增加二进制大小
# - 性能测试结果会严重偏离实际部署性能（通常低 50-70%）

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

# 综合性能测试服务器（统一所有单一项目 benchmark）
add_executable(benchmark_unified
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_unified.c
)
target_link_libraries(benchmark_unified
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

# 安装性能测试可执行文件
install(TARGETS performance_allocator performance_allocator_compare test_bitfield benchmark_unified
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/benchmark
)