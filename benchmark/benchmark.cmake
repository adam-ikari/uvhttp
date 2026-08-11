# benchmark/benchmark.cmake — included from CMakeLists.txt (L941-944)
# Builds the benchmark_unified executable used by nightly test-stress /
# performance-full jobs.
#
# uvhttp PUBLIC-propagates its include dirs (include/, deps/), so linking
# it gives benchmark_unified.c access to uv.h / uvhttp.h. The UVHTTP_FEATURE_*
# macros come from the global add_definitions() calls earlier in
# CMakeLists.txt, which apply to every target defined afterwards.
# uvhttp links its own deps PRIVATE (libuv, xxhash, llhttp), so the static
# library does not propagate them — they must be linked explicitly here.
add_executable(benchmark_unified
    benchmark/benchmark_unified.c
)

target_link_libraries(benchmark_unified PRIVATE
    uvhttp
    libuv
    xxhash
    llhttp
    ${CMAKE_DL_LIBS}
)
add_dependencies(benchmark_unified libuv xxhash llhttp)
