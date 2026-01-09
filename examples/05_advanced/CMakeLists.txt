# 高级特性示例

add_executable(libuv_data_pointer 01_libuv_data_pointer.c)
target_link_libraries(libuv_data_pointer ${UVHTTP_LIBRARY} uv pthread m)

add_executable(middleware_demo ../middleware_demo.c)
target_link_libraries(middleware_demo ${UVHTTP_LIBRARY} uv pthread m)

add_executable(cors_rate_limit_demo ../cors_rate_limit_demo.c)
target_link_libraries(cors_rate_limit_demo ${UVHTTP_LIBRARY} uv pthread m)
