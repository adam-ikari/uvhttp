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
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

### 2. Build Dependencies

UVHTTP includes all necessary dependencies as Git submodules. Initialize them:

```bash
git submodule update --init --recursive
```

### 3. Create Build Directory

```bash
mkdir build && cd build
```

### 4. Configure with CMake

**Basic configuration:**
```bash
cmake ..
```

**With all features:**
```bash
cmake -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON ..
```

**Custom configuration:**
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_WITH_WEBSOCKET=ON \
  -DBUILD_WITH_MIMALLOC=ON \
  -DBUILD_EXAMPLES=ON \
  ..
```

### 5. Build

```bash
make -j$(nproc)
```

### 6. Install (Optional)

```bash
sudo make install
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

```bash
# 使用自定义构建目录
BUILD_DIR=custom_build cmake ..

# 使用 Debug 模式
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 使用 -O2 优化（禁用 -O3）
cmake -DCMAKE_C_FLAGS_RELEASE="-O2 -DNDEBUG" ..

# 启用代码覆盖率
cmake -DENABLE_COVERAGE=ON ..

# 选择内存分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..  # 系统分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..  # mimalloc 分配器
```

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

## Installation Paths

By default, UVHTTP installs to:

- **Library**: `/usr/local/lib/libuvhttp.a`
- **Headers**: `/usr/local/include/uvhttp/`
- **Examples**: `/usr/local/bin/`

You can change these with CMake:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/custom/path ..
```

## Cross-Compilation

### Cross-compile for 32-bit

```bash
cmake \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_FLAGS="-m32" \
  -DCMAKE_CXX_FLAGS="-m32" \
  -DBUILD_WITH_MIMALLOC=OFF \
  ..
```

### Cross-compile for ARM

```bash
cmake \
  -DCMAKE_TOOLCHAIN_FILE=path/to/toolchain.cmake \
  ..
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
-luvhttp -lpthread -luv
```

### 依赖编译失败

如果依赖编译失败，可以手动编译：

```bash
# 编译 libuv
cd deps/libuv && mkdir -p build && cd build && cmake .. && make -j$(nproc)

# 编译 mbedtls
cd deps/mbedtls && python3 scripts/config.py set MBEDTLS_X509_USE_C && make -j$(nproc)

# 编译 llhttp
cd deps/cllhttp && gcc -c llhttp.c -o llhttp.o && ar rcs libllhttp.a llhttp.o
```

### 完全重新构建

如果遇到构建问题，可以完全重新构建：

```bash
cd build
make clean
cmake ..
make -j$(nproc)
```

### 内存分配器问题

如果遇到内存分配器相关问题：

```bash
# 切换到系统分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# 或切换到 mimalloc
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

## Next Steps

- [Quick Start](getting-started.md) - Create your first server
- [API Reference](../api/API_REFERENCE.md) - Complete API documentation
- [Examples](../../examples/) - Example programs