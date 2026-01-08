# 基础示例

add_executable(hello_world 01_hello_world.c)
target_link_libraries(hello_world ${UVHTTP_LIBRARY} uv pthread m)
