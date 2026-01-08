# 高级特性示例

add_executable(libuv_data_pointer 01_libuv_data_pointer.c)
target_link_libraries(libuv_data_pointer ${UVHTTP_LIBRARY} uv pthread m)
