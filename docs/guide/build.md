# Installation

This guide covers different ways to install and build UVHTTP.

## System Requirements

- **Operating System**: Linux
  - **Future Plans**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems
- **Compiler**: GCC 4.8+ or Clang 3.4+
- **CMake**: 3.10 or higher
- **Memory**: At least 1GB RAM
- **Disk Space**: At least 500MB

> **Note**: UVHTTP currently supports Linux platforms only. We plan to expand support to other operating systems in future releases.

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

**Custom configuration:** edit `option()` entries in `CMakeLists.txt`, then build:

```bash
make build
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_WITH_WEBSOCKET` | ON | Enable WebSocket support |
| `BUILD_WITH_MIMALLOC` | ON | Use mimalloc allocator |
| `BUILD_EXAMPLES` | OFF | Build example programs |
| `ENABLE_COVERAGE` | OFF | Enable code coverage |
| `ENABLE_DEBUG` | OFF | Enable debug mode |
| `CMAKE_BUILD_TYPE` | Release | Build type (Debug/Release) |
| `UVHTTP_ALLOCATOR_TYPE` | 1 | Memory allocator (0=system, 1=mimalloc) |

### Advanced Build Options

Advanced options can be configured by editing the `option()` entries in `CMakeLists.txt` directly, then rebuild:

```bash
make build
```

Common options available in `CMakeLists.txt`:

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_WITH_WEBSOCKET` | ON | Enable WebSocket support |
| `BUILD_WITH_MIMALLOC` | ON | Use mimalloc allocator |
| `BUILD_EXAMPLES` | OFF | Build example programs |
| `ENABLE_COVERAGE` | OFF | Enable code coverage |
| `ENABLE_DEBUG` | OFF | Enable debug mode |

## Output Files

编译后的文件位于 `build/dist/` 目录：
- `bin/` - 可执行文件（示例程序、测试程序）
- `lib/` - 库文件（libuvhttp.a）
- `include/` - 头文件

## Performance Testing

性能测试程序位于 `build/dist/bin/`：
- `performance_test` - 基础性能测试
- `performance_test_static` - 静态文件服务性能测试

运行性能测试：
```bash
# 启动测试服务器
./build/dist/bin/performance_static_server -d ./public -p 8080

# 使用 wrk 进行性能测试
wrk -t4 -c100 -d30s http://localhost:8080/

# 使用 ab 进行性能测试
ab -n 10000 -c 100 http://localhost:8080/
```

## Running Tests

After building, run the test suite:

```bash
./run_tests.sh
```

For detailed coverage report:

```bash
./run_tests.sh --detailed
```

## Output Files

Build output is located in `build/dist/`:
- `bin/` - Executables (examples, tests)
- `lib/` - Library (libuvhttp.a)
- `include/` - Headers

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

Make sure you have a C99-compliant compiler:

```bash
gcc --version  # Should be 4.8 or higher
```

### Linker Errors

If you see linker errors, make sure you're linking against the required libraries:

```bash
-luvhttp -lpthread -luv
```

### 依赖编译失败

如果依赖编译失败，可以手动编译：

```bash
# 编译 libuv
cd deps/libuv && make -f Makefile

# 编译 mbedtls
cd deps/mbedtls && python3 scripts/config.py set MBEDTLS_X509_USE_C && make -j$(nproc)

# 编译 llhttp
cd deps/cllhttp && gcc -c llhttp.c -o llhttp.o && ar rcs libllhttp.a llhttp.o
```

### 完全重新构建

如果遇到构建问题，可以完全重新构建：

```bash
make rebuild
```

### 内存分配器问题

如果遇到内存分配器相关问题，可修改 `CMakeLists.txt` 中的 `UVHTTP_ALLOCATOR_TYPE` 选项，然后重新编译：

```bash
make build
```

## Next Steps

- [Quick Start](getting-started.md) - Create your first server
- [API Reference](../api/API_REFERENCE.md) - Complete API documentation
- [Examples](https://github.com/adam-ikari/uvhttp/tree/main/examples) - Example programs