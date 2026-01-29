# 基础示例

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)
set(CJSON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson/libcjson.a)

add_executable(hello_world 01_hello_world.c)

# 使用声明库（IMPORTED targets）的方式链接
target_link_libraries(hello_world
    PRIVATE
        ${UVHTTP_LIB}
        libuv
        mbedtls
        xxhash
        llhttp
        pthread
        m
        dl
)

# 设置包含目录（UVHTTP头文件会自动包含依赖库的头文件）
target_include_directories(hello_world
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src
)

add_dependencies(hello_world uvhttp)

add_executable(quick_api_demo quick_api_demo.c)

# 使用声明库（IMPORTED targets）的方式链接
target_link_libraries(quick_api_demo
    PRIVATE
        ${UVHTTP_LIB}
        libuv
        mbedtls
        xxhash
        llhttp
        ${CJSON_LIB}
        pthread
        m
        dl
)

# 设置包含目录（UVHTTP头文件会自动包含依赖库的头文件）
target_include_directories(quick_api_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson
)

add_dependencies(quick_api_demo uvhttp)

add_executable(signal_handling_demo signal_handling_demo.c)

# 使用声明库（IMPORTED targets）的方式链接
target_link_libraries(signal_handling_demo
    PRIVATE
        ${UVHTTP_LIB}
        libuv
        mbedtls
        xxhash
        llhttp
        pthread
        m
        dl
)

# 设置包含目录（UVHTTP头文件会自动包含依赖库的头文件）
target_include_directories(signal_handling_demo
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src
)

add_dependencies(signal_handling_demo uvhttp)
