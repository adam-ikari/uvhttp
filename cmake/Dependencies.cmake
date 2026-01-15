# ============================================================================
# 依赖库配置
# ============================================================================

# 设置依赖包含目录
set(DEPS_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxhash
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uthash/src
)

# ============================================================================
# libuv
# ============================================================================
message(STATUS "Configuring libuv...")

# 检查 libuv 是否已经构建
set(LIBUV_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/build)
set(LIBUV_LIB ${LIBUV_BUILD_DIR}/libuv.a)

if(NOT EXISTS ${LIBUV_LIB})
    message(STATUS "Building libuv...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv -B ${LIBUV_BUILD_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_TESTING=OFF
            -DLIBUV_BUILD_SHARED=OFF
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv
        RESULT_VARIABLE LIBUV_CONFIG_RESULT
    )

    if(LIBUV_CONFIG_RESULT)
        message(FATAL_ERROR "Failed to configure libuv")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${LIBUV_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv
        RESULT_VARIABLE LIBUV_BUILD_RESULT
    )

    if(LIBUV_BUILD_RESULT)
        message(FATAL_ERROR "Failed to build libuv")
    endif()

    message(STATUS "libuv built successfully")
else()
    message(STATUS "libuv already built: ${LIBUV_LIB}")
endif()

# ============================================================================
# mbedtls
# ============================================================================
message(STATUS "Configuring mbedtls...")

# 检查 mbedtls 是否已经构建
set(MBEDTLS_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/build)
set(MBEDTLS_LIBS
    ${MBEDTLS_BUILD_DIR}/library/libmbedtls.a
    ${MBEDTLS_BUILD_DIR}/library/libmbedx509.a
    ${MBEDTLS_BUILD_DIR}/library/libmbedcrypto.a
)

if(NOT EXISTS ${MBEDTLS_BUILD_DIR}/library/libmbedtls.a)
    message(STATUS "Building mbedtls...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls -B ${MBEDTLS_BUILD_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DENABLE_TESTING=OFF
            -DENABLE_PROGRAMS=OFF
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls
        RESULT_VARIABLE MBEDTLS_CONFIG_RESULT
    )

    if(MBEDTLS_CONFIG_RESULT)
        message(FATAL_ERROR "Failed to configure mbedtls")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${MBEDTLS_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls
        RESULT_VARIABLE MBEDTLS_BUILD_RESULT
    )

    if(MBEDTLS_BUILD_RESULT)
        message(FATAL_ERROR "Failed to build mbedtls")
    endif()

    message(STATUS "mbedtls built successfully")
else()
    message(STATUS "mbedtls already built")
endif()

# ============================================================================
# xxhash
# ============================================================================
message(STATUS "Configuring xxhash...")

set(XXHASH_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxhash)
set(XXHASH_LIB ${XXHASH_BUILD_DIR}/libxxhash.a)

if(NOT EXISTS ${XXHASH_LIB})
    message(STATUS "Building xxhash...")
    execute_process(
        COMMAND make libxxhash.a
        WORKING_DIRECTORY ${XXHASH_BUILD_DIR}
        RESULT_VARIABLE XXHASH_BUILD_RESULT
    )

    if(XXHASH_BUILD_RESULT)
        message(WARNING "Failed to build xxhash library, will use source instead")
        set(XXHASH_LIB "")
    else()
        message(STATUS "xxhash built successfully")
    endif()
else()
    message(STATUS "xxhash already built: ${XXHASH_LIB}")
endif()

# ============================================================================
# llhttp
# ============================================================================
message(STATUS "Configuring llhttp...")

set(LLHTTP_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp)
set(LLHTTP_LIB ${LLHTTP_BUILD_DIR}/libllhttp.a)

if(NOT EXISTS ${LLHTTP_LIB})
    message(STATUS "Building llhttp...")
    execute_process(
        COMMAND make libllhttp.a
        WORKING_DIRECTORY ${LLHTTP_BUILD_DIR}
        RESULT_VARIABLE LLHTTP_BUILD_RESULT
    )

    if(LLHTTP_BUILD_RESULT)
        message(FATAL_ERROR "Failed to build llhttp")
    endif()

    message(STATUS "llhttp built successfully")
else()
    message(STATUS "llhttp already built: ${LLHTTP_LIB}")
endif()

# ============================================================================
# cjson
# ============================================================================
message(STATUS "Configuring cjson...")

set(CJSON_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson/build)
set(CJSON_LIB ${CJSON_BUILD_DIR}/libcjson.a)

if(NOT EXISTS ${CJSON_LIB})
    message(STATUS "Building cjson...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson -B ${CJSON_BUILD_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DENABLE_CJSON_TEST=OFF
            -DBUILD_SHARED_AND_STATIC_LIBS=OFF
            -DBUILD_SHARED_LIBS=OFF
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
        RESULT_VARIABLE CJSON_CONFIG_RESULT
    )

    if(CJSON_CONFIG_RESULT)
        message(FATAL_ERROR "Failed to configure cjson")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${CJSON_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
        RESULT_VARIABLE CJSON_BUILD_RESULT
    )

    if(CJSON_BUILD_RESULT)
        message(FATAL_ERROR "Failed to build cjson")
    endif()

    message(STATUS "cjson built successfully")
else()
    message(STATUS "cjson already built: ${CJSON_LIB}")
endif()

# ============================================================================
# mimalloc
# ============================================================================
if(BUILD_WITH_MIMALLOC)
    message(STATUS "Configuring mimalloc...")

    set(MIMALLOC_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc/build)
    # 根据构建类型选择正确的库文件名
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(MIMALLOC_LIB ${MIMALLOC_BUILD_DIR}/libmimalloc-debug.a)
    else()
        set(MIMALLOC_LIB ${MIMALLOC_BUILD_DIR}/libmimalloc.a)
    endif()

    if(NOT EXISTS ${MIMALLOC_LIB})

        message(STATUS "Building mimalloc...")

        execute_process(

            COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc -B ${MIMALLOC_BUILD_DIR}

                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

                -DMI_BUILD_STATIC=ON

                -DMI_BUILD_SHARED=OFF

                -DMI_BUILD_TESTS=OFF

                -DMI_BUILD_OBJECT=OFF

                -DMI_INSTALL=OFF

            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc

            RESULT_VARIABLE MIMALLOC_CONFIG_RESULT

        )

    

        if(MIMALLOC_CONFIG_RESULT)

            message(FATAL_ERROR "Failed to configure mimalloc")

        endif()

    

        execute_process(

            COMMAND ${CMAKE_COMMAND} --build ${MIMALLOC_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} --target mimalloc-static -j

            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc

            RESULT_VARIABLE MIMALLOC_BUILD_RESULT

        )

    

        if(MIMALLOC_BUILD_RESULT)

            message(FATAL_ERROR "Failed to build mimalloc")

        endif()

    

        message(STATUS "mimalloc built successfully")

    else()

        message(STATUS "mimalloc already built: ${MIMALLOC_LIB}")

    endif()
endif()

# ============================================================================
# 创建自定义目标
# ============================================================================
add_custom_target(libuv
    COMMAND ${CMAKE_COMMAND} --build ${LIBUV_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv
)

add_custom_target(mbedtls
    COMMAND ${CMAKE_COMMAND} --build ${MBEDTLS_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls
)

add_custom_target(xxhash
    COMMAND make libxxhash.a
    WORKING_DIRECTORY ${XXHASH_BUILD_DIR}
)

add_custom_target(llhttp
    COMMAND make libllhttp.a
    WORKING_DIRECTORY ${LLHTTP_BUILD_DIR}
)

add_custom_target(cjson
    COMMAND ${CMAKE_COMMAND} --build ${CJSON_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
)

if(BUILD_WITH_MIMALLOC)
    add_custom_target(mimalloc
        COMMAND ${CMAKE_COMMAND} --build ${MIMALLOC_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc
    )
endif()

# ============================================================================
# 打印依赖信息
# ============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "Dependencies Configuration")
message(STATUS "========================================")
message(STATUS "libuv: ${LIBUV_LIB}")
message(STATUS "mbedtls: ${MBEDTLS_LIBS}")
message(STATUS "xxhash: ${XXHASH_LIB}")
message(STATUS "llhttp: ${LLHTTP_LIB}")
message(STATUS "cjson: ${CJSON_LIB}")
if(BUILD_WITH_MIMALLOC)
    message(STATUS "mimalloc: ${MIMALLOC_LIB}")
endif()
message(STATUS "========================================")
message(STATUS "")