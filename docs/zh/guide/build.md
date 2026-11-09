---
title: 构建指南
description: "UVHTTP 构建指南——从源码编译、构建选项、交叉编译到故障排除，涵盖 CMake 配置、性能测试与安装路径。"
---

# 构建指南

UVHTTP 从源码构建，顶层 Makefile 转发给 CMake。下面按顺序讲系统要求、编译、在自己项目里引用、安装与交叉编译，最后是几类常见的构建失败。

## 系统要求

- **操作系统**: Linux
  - **未来计划**: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统
- **编译器**: GCC 4.8+ 或 Clang 3.4+
- **CMake**: 3.10 或更高版本
- **内存**: 至少 1GB RAM
- **磁盘空间**: 至少 500MB

> **注意**: 现在只有 Linux 能跑。macOS、Windows、FreeBSD 和 WASM 还在计划里。

## 从源码构建

### 1. 克隆仓库

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

> **注意**: `--recurse-submodules` 会把 deps 下的依赖一起拉下来。已经克隆过仓库却漏了这一步，运行 `git submodule update --init --recursive` 补齐即可。

### 2. 编译

```bash
make build
```

**自定义配置:** 直接修改 `CMakeLists.txt` 中的 `option()` 条目，然后编译：

```bash
make build
```

### 3. 在自己的项目中使用

编译完成后，在你的 CMake 项目中通过 `add_subdirectory` 引用：

```cmake
add_subdirectory(path/to/uvhttp)
target_link_libraries(myapp uvhttp)
```

## 构建选项

| 选项 | 默认值 | 说明 |
|--------|---------|-------------|
| `BUILD_WITH_WEBSOCKET` | ON | 启用 WebSocket 支持 |
| `BUILD_WITH_HTTPS` | ON | 启用 TLS（mbedtls） |
| `BUILD_WITH_MIMALLOC` | OFF | 使用 mimalloc 分配器 |
| `BUILD_EXAMPLES` | OFF | 编译示例程序 |
| `ENABLE_COVERAGE` | OFF | 启用代码覆盖率 |
| `ENABLE_DEBUG` | OFF | 启用 Debug 模式 |
| `CMAKE_BUILD_TYPE` | Release | 构建类型（Debug/Release） |
| `UVHTTP_ALLOCATOR_TYPE` | 0 | 0 = 系统分配器，1 = mimalloc，2 = 自定义 |

`UVHTTP_ALLOCATOR_TYPE=1` 要求同时打开 `BUILD_WITH_MIMALLOC`，否则配置阶段直接报错退出。

## 输出文件

编译后的文件位于 `build/dist/` 目录：
- `bin/` - 可执行文件（示例程序、测试程序）
- `lib/` - 库文件（libuvhttp.a）
- `include/` - 头文件

## 性能测试

`BUILD_BENCHMARKS` 默认开启，产出 `benchmark_unified`，默认在 18081 端口提供基准端点（第一个参数可以换端口）：

```bash
./build/dist/bin/benchmark_unified 18081 &

wrk -t2 -c10 -d10s http://localhost:18081/
wrk -t2 -c10 -d10s http://localhost:18081/json
wrk -t2 -c10 -d10s http://localhost:18081/large
```

单机数字换台机器就变了。CI 门禁怎么在同一条 runner 上比较两个版本，见[性能目标](../../PERFORMANCE_TARGETS.md)。

## 运行测试

```bash
make test
```

这条命令在 `build/` 里跑 `ctest`，遇到第一个失败就停下。要覆盖率报告：

```bash
make build-coverage
make coverage
```

## 安装路径

CMake 默认安装前缀为 `/usr/local`。通过 `CMAKE_INSTALL_PREFIX` 覆盖：

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=/custom/path
cmake --build build -j$(nproc)
cmake --install build
```

安装后库、头文件与 CMake 包配置分别位于 `lib/`、`include/` 与 `lib/cmake/uvhttp/`。

## 交叉编译

### 交叉编译 32 位版本

使用 32 位编译器构建：

```bash
make build CC="gcc -m32" CXX="g++ -m32"
```

### 交叉编译 ARM 版本

使用交叉编译工具链：

```bash
make build CMAKE_TOOLCHAIN_FILE=path/to/toolchain.cmake
```

## 故障排除

### 依赖缺失

如果出现关于 libuv 或其他依赖缺失的错误：

```bash
git submodule update --init --recursive
```

### 编译错误

确保你使用的是兼容 C11 的编译器：

```bash
gcc --version  # 应为 4.8 或更高版本
```

### 链接错误

如果出现链接错误，请确保链接了所需的库：

```bash
cc app.c -o app -luvhttp -lpthread -luv
```

通过 CMake 链接时，`uvhttp` target 会带上自己的依赖（`uv`、`pthread`，开启 HTTPS 时还有 mbedtls），不需要手工维护这份列表。

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
make build
```

### 内存分配器问题

如果遇到内存分配器相关问题，可修改 `CMakeLists.txt` 中的 `UVHTTP_ALLOCATOR_TYPE` 选项，然后重新编译：

```bash
make build
```

## 下一步

- [快速开始](/zh/guide/getting-started) - 创建你的第一个服务器
- [API 参考](../../api/API_REFERENCE.md) - 完整的 API 文档
- [示例](https://github.com/adam-ikari/uvhttp/tree/main/examples) - 示例程序
