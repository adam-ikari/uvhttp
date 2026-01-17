# 测试目录 CMake 配置

# 添加单元测试子目录
add_subdirectory(unit)

# 可选：添加集成测试
if(BUILD_INTEGRATION_TESTS)
    add_subdirectory(integration)
endif()

# 可选：添加性能测试
if(BUILD_PERFORMANCE_TESTS)
    add_subdirectory(performance)
endif()