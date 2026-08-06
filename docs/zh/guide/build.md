---
title: 构建指南
description: "UVHTTP 构建指南——从源码编译、构建选项、交叉编译到故障排除，涵盖 CMake 配置、性能测试与安装路径。"
---

# 构建指南

本指南介绍安装与构建 UVHTTP 的不同方式。

## 系统要求

- **操作系统**: Linux
  - **未来计划**: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统
- **编译器**: GCC 4.8+ 或 Clang 3.4+
- **CMake**: 3.10 或更高版本
- **内存**: 至少 1GB RAM
- **磁盘空间**: 至少 500MB

> **注意**: UVHTTP 目前仅支持 Linux 平台。我们计划在未来版本中扩展对其他操作系统的支持。

## 从源码构建

### 1. 克隆仓库

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

> **注意**: `--recurse-submodules` 参数会自动克隆所有依赖。如果忘记使用此参数，可以运行 `git submodule update --init --recursive` 来补全。
```

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
| `BUILD_WITH_MIMALLOC` | ON | 使用 mimalloc 分配器 |
| `BUILD_EXAMPLES` | OFF | 编译示例程序 |
| `ENABLE_COVERAGE` | OFF | 启用代码覆盖率 |
| `ENABLE_DEBUG` | OFF | 启用 Debug 模式 |
| `CMAKE_BUILD_TYPE` | Release | 构建类型（Debug/Release） |
| `UVHTTP_ALLOCATOR_TYPE` | 1 | 内存分配器（0=系统分配器，1=mimalloc） |

### 高级构建选项

高级选项可直接修改 `CMakeLists.txt` 中的 `option()` 配置，然后重新编译：

```bash
make build
```

## 输出文件

编译后的文件位于 `build/dist/` 目录：
- `bin/` - 可执行文件（示例程序、测试程序）
- `lib/` - 库文件（libuvhttp.a）
- `include/` - 头文件

## 性能测试

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

## 运行测试

编译完成后，运行测试套件：

```bash
./run_tests.sh
```

获取详细的覆盖率报告：

```bash
./run_tests.sh --detailed
```

## 安装路径

默认情况下，UVHTTP 安装到以下位置：

- **库文件**: `/usr/local/lib/libuvhttp.a`
- **头文件**: `/usr/local/include/uvhttp/`
- **示例程序**: `/usr/local/bin/`

你可以通过 `make build` 的 `PREFIX` 参数更改安装路径：

```bash
make build PREFIX=/custom/path
```

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

确保你使用的是兼容 C99 的编译器：

```bash
gcc --version  # 应为 4.8 或更高版本
```

### 链接错误

如果出现链接错误，请确保链接了所需的库：

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
