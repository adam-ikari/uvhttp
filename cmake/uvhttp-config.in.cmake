@PACKAGE_INIT@

include(CMakeFindDependencyMacro)

# ============================================================================
# Dependencies
# ============================================================================
# The exported uvhttp::uvhttp target links its dependencies by their plain
# target names (libuv, xxhash, llhttp, mbedtls, mimalloc) plus the exported
# uvhttp::miniz. None of the vendored dependencies are in the export set, so
# define them here as IMPORTED targets pointing at the static archives and
# headers shipped inside this install tree.

set(UVHTTP_DEP_LIBDIR "${PACKAGE_PREFIX_DIR}/@CMAKE_INSTALL_LIBDIR@")
set(UVHTTP_DEP_INCLUDEDIR "${PACKAGE_PREFIX_DIR}/@CMAKE_INSTALL_INCLUDEDIR@")

# libuv — vendored and installed with the package.
if(NOT TARGET libuv)
    add_library(libuv STATIC IMPORTED)
    set_target_properties(libuv PROPERTIES
        IMPORTED_LOCATION "${UVHTTP_DEP_LIBDIR}/libuv.a"
        INTERFACE_INCLUDE_DIRECTORIES "${UVHTTP_DEP_INCLUDEDIR}")
endif()

# xxhash — vendored.
if(NOT TARGET xxhash)
    add_library(xxhash STATIC IMPORTED)
    set_target_properties(xxhash PROPERTIES
        IMPORTED_LOCATION "${UVHTTP_DEP_LIBDIR}/libxxhash.a"
        INTERFACE_INCLUDE_DIRECTORIES "${UVHTTP_DEP_INCLUDEDIR}")
endif()

# llhttp — vendored.
if(NOT TARGET llhttp)
    add_library(llhttp STATIC IMPORTED)
    set_target_properties(llhttp PROPERTIES
        IMPORTED_LOCATION "${UVHTTP_DEP_LIBDIR}/libllhttp.a"
        INTERFACE_INCLUDE_DIRECTORIES "${UVHTTP_DEP_INCLUDEDIR}")
endif()

# mbedtls — vendored; only built when HTTPS or WebSocket was enabled.
if(@BUILD_WITH_HTTPS@ OR @BUILD_WITH_WEBSOCKET@)
    if(NOT TARGET mbedtls)
        add_library(mbedtls INTERFACE IMPORTED)
        set_target_properties(mbedtls PROPERTIES
            INTERFACE_LINK_LIBRARIES
                "${UVHTTP_DEP_LIBDIR}/libmbedtls.a;${UVHTTP_DEP_LIBDIR}/libmbedx509.a;${UVHTTP_DEP_LIBDIR}/libmbedcrypto.a"
            INTERFACE_INCLUDE_DIRECTORIES "${UVHTTP_DEP_INCLUDEDIR}")
    endif()
endif()

# mimalloc — vendored; only built when UVHTTP_ALLOCATOR_TYPE=1.
if(@BUILD_WITH_MIMALLOC@)
    if(NOT TARGET mimalloc)
        add_library(mimalloc STATIC IMPORTED)
        set_target_properties(mimalloc PROPERTIES
            IMPORTED_LOCATION "${UVHTTP_DEP_LIBDIR}/@UVHTTP_MIMALLOC_LIB_NAME@"
            INTERFACE_INCLUDE_DIRECTORIES "${UVHTTP_DEP_INCLUDEDIR}")
    endif()
endif()

# Include exported targets (uvhttp::uvhttp and uvhttp::miniz)
include("${CMAKE_CURRENT_LIST_DIR}/uvhttp-targets.cmake")

# Set package variables
set(UVHTTP_VERSION "@UVHTTP_VERSION@")
set(UVHTTP_INCLUDE_DIRS "${UVHTTP_DEP_INCLUDEDIR}")
set(UVHTTP_LIBRARIES uvhttp)
set(UVHTTP_LIBRARY_DIRS "${UVHTTP_DEP_LIBDIR}")

check_required_components(uvhttp)

# ============================================================================
# Feature flags the library was compiled with
# ============================================================================
# Consumers can use these to gate their own code on the same feature set.
set(UVHTTP_FEATURE_WEBSOCKET @BUILD_WITH_WEBSOCKET@)
set(UVHTTP_FEATURE_HTTPS @BUILD_WITH_HTTPS@)
set(UVHTTP_FEATURE_STATIC_FILES @UVHTTP_FEATURE_STATIC_FILES_VALUE@)
set(UVHTTP_FEATURE_RATE_LIMIT @UVHTTP_FEATURE_RATE_LIMIT_VALUE@)
set(UVHTTP_FEATURE_LRU_CACHE @UVHTTP_FEATURE_LRU_CACHE_VALUE@)
set(UVHTTP_FEATURE_ROUTER_CACHE @UVHTTP_FEATURE_ROUTER_CACHE_VALUE@)
set(UVHTTP_FEATURE_COMPRESSION @UVHTTP_FEATURE_COMPRESSION_VALUE@)
set(UVHTTP_FEATURE_LOGGING @UVHTTP_FEATURE_LOGGING_VALUE@)
set(UVHTTP_ALLOCATOR_TYPE @UVHTTP_ALLOCATOR_TYPE@)
