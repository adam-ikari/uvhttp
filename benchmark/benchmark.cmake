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

# 安装性能测试可执行文件
install(TARGETS performance_allocator performance_allocator_compare test_bitfield
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}/benchmark
)