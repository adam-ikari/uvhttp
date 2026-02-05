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

# 综合性能测试服务器（统一所有单一项目 benchmark）
add_executable(benchmark_unified
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_unified.c
)
target_link_libraries(benchmark_unified
    uvhttp
    ${UVHTTP_CORE_DEPS}
)
set_target_properties(benchmark_unified PROPERTIES
    COMPILE_FLAGS "-O2 -DNDEBUG"
)

# 安装性能测试可执行文件
install(TARGETS performance_allocator performance_allocator_compare test_bitfield benchmark_unified
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/benchmark
)