# ============================================================================
# 依赖库配置
# ============================================================================

# ============================================================================
# 依赖管理配置
# ============================================================================
# 设计原则：
# 1. 集中式管理：所有依赖都在此文件中配置
# 2. 使用 IMPORTED targets：所有依赖库都声明为 IMPORTED targets
# 3. 依赖分类：核心依赖、可选依赖、测试依赖
# 4. 传递依赖：通过 CMake 的 target_link_libraries 自动处理
# ============================================================================

# 设置依赖包含目录
set(DEPS_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxhash
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/uthash/src
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googletest/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/googlemock/include
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
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
            -DLIBUV_BUILD_BENCH=OFF
            -DLIBUV_BUILD_EXAMPLES=OFF
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

# 声明 libuv 为 IMPORTED 静态库
add_library(libuv STATIC IMPORTED)
set_target_properties(libuv PROPERTIES
    IMPORTED_LOCATION ${LIBUV_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
)

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
            -DENABLE_DOCS=OFF
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

# 声明 mbedtls 为 IMPORTED 静态库（接口库，包含多个子库）
add_library(mbedtls INTERFACE IMPORTED)
set_target_properties(mbedtls PROPERTIES
    INTERFACE_LINK_LIBRARIES "${MBEDTLS_LIBS}"
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/include
)

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

# 声明 xxhash 为 IMPORTED 静态库
add_library(xxhash STATIC IMPORTED)
set_target_properties(xxhash PROPERTIES
    IMPORTED_LOCATION ${XXHASH_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxhash
)

# ============================================================================
# llhttp
# ============================================================================
message(STATUS "Configuring llhttp...")

set(LLHTTP_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp/build)
set(LLHTTP_LIB ${LLHTTP_BUILD_DIR}/libllhttp.a)

if(NOT EXISTS ${LLHTTP_LIB})
    message(STATUS "Building llhttp...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp -B ${LLHTTP_BUILD_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp
        RESULT_VARIABLE LLHTTP_CONFIG_RESULT
    )

    if(LLHTTP_CONFIG_RESULT)
        message(FATAL_ERROR "Failed to configure llhttp")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${LLHTTP_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp
        RESULT_VARIABLE LLHTTP_BUILD_RESULT
    )

    if(LLHTTP_BUILD_RESULT)
        message(FATAL_ERROR "Failed to build llhttp")
    endif()

    message(STATUS "llhttp built successfully")
else()
    message(STATUS "llhttp already built: ${LLHTTP_LIB}")
endif()

# 声明 llhttp 为 IMPORTED 静态库
add_library(llhttp STATIC IMPORTED)
set_target_properties(llhttp PROPERTIES
    IMPORTED_LOCATION ${LLHTTP_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp
)

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

                -DMI_BUILD_OVERRIDE=OFF

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

    # 声明 mimalloc 为 IMPORTED 静态库
    add_library(mimalloc STATIC IMPORTED)
    set_target_properties(mimalloc PROPERTIES
        IMPORTED_LOCATION ${MIMALLOC_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc/include
    )
endif()

# ============================================================================
# 注意：已使用 IMPORTED targets 声明库，不再需要自定义目标
# ============================================================================

# ============================================================================
# googletest
# ============================================================================
message(STATUS "Configuring googletest...")

set(GTEST_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest/build)
set(GTEST_LIBS
    ${GTEST_BUILD_DIR}/lib/libgtest.a
    ${GTEST_BUILD_DIR}/lib/libgtest_main.a
    ${GTEST_BUILD_DIR}/lib/libgmock.a
    ${GTEST_BUILD_DIR}/lib/libgmock_main.a
)

if(NOT EXISTS ${GTEST_BUILD_DIR}/lib/libgtest.a)
    message(STATUS "Building googletest...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest -B ${GTEST_BUILD_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_GTEST=ON
            -DBUILD_GMOCK=ON
            -DINSTALL_GTEST=OFF
            -DBUILD_SHARED_LIBS=OFF
            -Dgtest_force_shared_crt=OFF
            -Dgtest_build_samples=OFF
            -Dgtest_build_tests=OFF
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest
        RESULT_VARIABLE GTEST_CONFIG_RESULT
    )

    if(GTEST_CONFIG_RESULT)
        message(FATAL_ERROR "Failed to configure googletest")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${GTEST_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest
        RESULT_VARIABLE GTEST_BUILD_RESULT
    )

    if(GTEST_BUILD_RESULT)
        message(FATAL_ERROR "Failed to build googletest")
    endif()

    message(STATUS "googletest built successfully")
else()
    message(STATUS "googletest already built")
endif()

add_custom_target(gtest
    COMMAND ${CMAKE_COMMAND} --build ${GTEST_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/googletest
)

# ============================================================================
# cJSON
# ============================================================================
message(STATUS "Configuring cJSON...")

set(CJSON_BUILD_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson/build)
set(CJSON_LIB ${CJSON_BUILD_DIR}/libcjson.a)

if(NOT EXISTS ${CJSON_LIB})
    message(STATUS "Building cJSON...")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson -B ${CJSON_BUILD_DIR}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -DBUILD_SHARED_LIBS=OFF
            -DENABLE_CJSON_TEST=OFF
            -DENABLE_CJSON_UTILS=OFF
            -DENABLE_FUZZING=OFF
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
        RESULT_VARIABLE CJSON_CONFIG_RESULT
    )

    if(CJSON_CONFIG_RESULT)
        message(FATAL_ERROR "Failed to configure cJSON")
    endif()

    execute_process(
        COMMAND ${CMAKE_COMMAND} --build ${CJSON_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
        RESULT_VARIABLE CJSON_BUILD_RESULT
    )

    if(CJSON_BUILD_RESULT)
        message(FATAL_ERROR "Failed to build cJSON")
    endif()

    message(STATUS "cJSON built successfully")
else()
    message(STATUS "cJSON already built: ${CJSON_LIB}")
endif()

# 声明 cJSON 为 IMPORTED 静态库
add_library(cjson STATIC IMPORTED)
set_target_properties(cjson PROPERTIES
    IMPORTED_LOCATION ${CJSON_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
)

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
message(STATUS "googletest: ${GTEST_LIBS}")
message(STATUS "========================================")
message(STATUS "")

# ============================================================================
# 依赖导出配置
# ============================================================================
# 为示例程序和测试提供便捷的依赖变量
# ============================================================================

# 核心依赖（uvhttp 库必需）
set(UVHTTP_CORE_DEPS libuv mbedtls xxhash llhttp)

# 可选依赖（示例程序可选使用）
set(UVHTTP_OPTIONAL_DEPS cjson)

# 测试依赖
set(UVHTTP_TEST_DEPS gtest gmock)

# 平台特定库
if(IS_WINDOWS)
    set(UVHTTP_PLATFORM_LIBS "")
elseif(IS_MACOS)
    set(UVHTTP_PLATFORM_LIBS pthread)
else()
    # Linux
    set(UVHTTP_PLATFORM_LIBS pthread m dl)
endif()

# mimalloc（如果启用）
if(BUILD_WITH_MIMALLOC)
    list(APPEND UVHTTP_OPTIONAL_DEPS mimalloc)
endif()

# 导出给子项目使用
set(UVHTTP_CORE_DEPS ${UVHTTP_CORE_DEPS} PARENT_SCOPE)
set(UVHTTP_OPTIONAL_DEPS ${UVHTTP_OPTIONAL_DEPS} PARENT_SCOPE)
set(UVHTTP_TEST_DEPS ${UVHTTP_TEST_DEPS} PARENT_SCOPE)
set(UVHTTP_PLATFORM_LIBS ${UVHTTP_PLATFORM_LIBS} PARENT_SCOPE)

message(STATUS "核心依赖: ${UVHTTP_CORE_DEPS}")
message(STATUS "可选依赖: ${UVHTTP_OPTIONAL_DEPS}")
message(STATUS "平台库: ${UVHTTP_PLATFORM_LIBS}")
message(STATUS "")