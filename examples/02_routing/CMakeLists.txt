# 路由示例

add_executable(simple_routing 01_simple_routing.c)
target_link_libraries(simple_routing ${UVHTTP_LIBRARY} uv pthread m)

add_executable(method_routing 02_method_routing.c)
target_link_libraries(method_routing ${UVHTTP_LIBRARY} uv pthread m)
