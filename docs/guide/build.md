# Installation

Build UVHTTP from source on Linux and install it.

## System Requirements

- **Operating System**: Linux
  - **Future Plans**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems
- **Compiler**: GCC 4.8+ or Clang 3.4+
- **CMake**: 3.10 or higher
- **Memory**: At least 1GB RAM
- **Disk Space**: At least 500MB

> **Note**: Linux is the only supported platform today. macOS, Windows, FreeBSD, and WASM are planned.

## Building from Source

### 1. Clone the Repository

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

### 2. Build

```bash
make build
```

To change the configuration, edit the `option()` entries in `CMakeLists.txt` and build again:

```bash
make build
```

### 3. Use It in Your Project

Link the built library from your own CMake project with `add_subdirectory`:

```cmake
add_subdirectory(path/to/uvhttp)
target_link_libraries(myapp uvhttp)
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_WITH_WEBSOCKET` | ON | Enable WebSocket support |
| `BUILD_WITH_HTTPS` | ON | Enable TLS support (mbedtls) |
| `BUILD_WITH_MIMALLOC` | OFF | Use the mimalloc allocator |
| `BUILD_EXAMPLES` | OFF | Build example programs |
| `ENABLE_COVERAGE` | OFF | Enable code coverage |
| `ENABLE_DEBUG` | OFF | Enable debug mode |
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug/Release) |
| `UVHTTP_ALLOCATOR_TYPE` | 0 | 0 = system, 1 = mimalloc, 2 = custom |

`UVHTTP_ALLOCATOR_TYPE=1` requires `BUILD_WITH_MIMALLOC=ON`; the configure step stops with an error otherwise.

## Output Files

Build output is located in `build/dist/`:
- `bin/` - Executables (examples, tests)
- `lib/` - Library (libuvhttp.a)
- `include/` - Headers

## Performance Testing

`BUILD_BENCHMARKS` defaults to ON and produces `benchmark_unified`, which serves the benchmark endpoints on port 18081 (or on the port passed as its first argument):

```bash
./build/dist/bin/benchmark_unified 18081 &

wrk -t2 -c10 -d10s http://localhost:18081/
wrk -t2 -c10 -d10s http://localhost:18081/json
wrk -t2 -c10 -d10s http://localhost:18081/large
```

Numbers from one machine say little about another; [Performance Targets](../PERFORMANCE_TARGETS.md) explains how the CI gate compares two revisions on the same runner instead.

## Running Tests

```bash
make test
```

This runs `ctest` inside `build/` and stops on the first failure. For a coverage report:

```bash
make build-coverage
make coverage
```

## Installation Path

The default install prefix is `/usr/local`. Override it with `CMAKE_INSTALL_PREFIX`:

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/custom/path
cmake --build build -j$(nproc)
cmake --install build
```

Installed files land in `lib/`, `include/`, and the CMake package config in `lib/cmake/uvhttp/`.

## Cross-Compilation

### Cross-compile for 32-bit

```bash
make build CC="gcc -m32" CXX="g++ -m32"
```

### Cross-compile for ARM

```bash
make build CMAKE_TOOLCHAIN_FILE=path/to/toolchain.cmake
```

## Troubleshooting

### Missing Dependencies

If you see errors about missing libuv or other dependencies:

```bash
git submodule update --init --recursive
```

### Compilation Errors

Make sure you have a C11-compliant compiler:

```bash
gcc --version  # Should be 4.8 or higher
```

### Linker Errors

If you see linker errors, make sure you're linking against the required libraries:

```bash
cc app.c -o app -luvhttp -lpthread -luv
```

The `uvhttp` CMake target carries its own link dependencies (`uv`, `pthread`, and mbedtls when HTTPS is on), so linking through CMake avoids hand-maintaining this list.

### Dependency Build Failures

If a bundled dependency fails to build, build it by hand:

```bash
# libuv
cd deps/libuv && make -f Makefile

# mbedtls
cd deps/mbedtls && python3 scripts/config.py set MBEDTLS_X509_USE_C && make -j$(nproc)

# llhttp
cd deps/cllhttp && gcc -c llhttp.c -o llhttp.o && ar rcs libllhttp.a llhttp.o
```

### Full Rebuild

For unexplained build failures, wipe and rebuild:

```bash
make rebuild
```

### Allocator Problems

If the allocator is implicated, change `UVHTTP_ALLOCATOR_TYPE` in `CMakeLists.txt` and rebuild:

```bash
make build
```

## Next Steps

Start with [Quick Start](getting-started.md). The [API Reference](../api/API_REFERENCE.md) covers the API, and [Examples](https://github.com/adam-ikari/uvhttp/tree/main/examples) holds runnable programs.