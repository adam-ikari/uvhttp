# 安装指南

本指南介绍如何安装和构建 UVHTTP。

## 📌 平台支持

**当前支持**: Linux

**未来计划**: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统

UVHTTP 目前针对 Linux 平台进行了优化。我们计划在未来版本中扩展对其他操作系统和平台的支持。

## 系统要求

### 最低要求
- **CMake**: 3.10 或更高版本
- **C 编译器**: 
  - GCC 4.9+ (Linux)
  - Clang 3.5+ (Linux)
- **操作系统**: Linux

### 推荐要求
- **CMake**: 3.15 或更高版本
- **C 编译器**: 
  - GCC 7+ (Linux)
  - Clang 10+ (Linux)

## 从源码编译

### 1. 克隆仓库

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

> **注意**: `--recurse-submodules` 参数会自动克隆所有依赖。如果忘记使用此参数，可以运行 `git submodule update --init --recursive` 来补全。

### 2. 配置并编译项目

```bash
# 基本配置并编译（Release 模式）
make build

## 平台特定说明

### Ubuntu/Debian

#### 安装依赖

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential
```

> 注意：libuv 已作为子模块包含在项目中，无需单独安装。编译时会自动构建。

#### 编译

```bash
# 初始化子模块（首次克隆时需要）
git submodule update --init --recursive

make build
```

### CentOS/RHEL

#### 安装依赖

```bash
sudo yum groupinstall "Development Tools"
sudo yum install -y \
    cmake3 \
    openssl-devel
```

> 注意：libuv 已作为子模块包含在项目中，无需单独安装。编译时会自动构建。

#### 编译

```bash
make build
```

### macOS

#### 使用 Homebrew

```bash
# 安装 Homebrew（如果尚未安装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake openssl

# 编译
make build
```

#### 使用 MacPorts

```bash
# 安装 MacPorts（如果尚未安装）
# 然后安装依赖
sudo port install cmake libuv openssl

# 编译
make build
```

### Windows

#### 使用 vcpkg

```bash
# 安装 vcpkg（如果尚未安装）
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg integrate install

# 安装依赖
vcpkg install libuv openssl:x64-windows

# 编译
make build
```

#### 使用预编译依赖

1. 下载并安装 libuv: https://github.com/libuv/libuv/releases
2. 下载并安装 OpenSSL: https://slproweb.com/products/Win32OpenSSL.html
3. 在 `CMakeLists.txt` 中指定库路径，然后运行 `make build`：
   ```cmake
   set(LIBUV_INCLUDE_DIR "[libuv include路径]" CACHE PATH "")
   set(LIBUV_LIBRARY "[libuv lib路径]" CACHE FILEPATH "")
   set(OPENSSL_INCLUDE_DIR "[OpenSSL include路径]" CACHE PATH "")
   set(OPENSSL_LIBRARY "[OpenSSL lib路径]" CACHE FILEPATH "")
   ```

## 构建选项

### 常用 CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_WITH_WEBSOCKET` | `ON` | 启用 WebSocket 支持 |
| `BUILD_WITH_MIMALLOC` | `ON` | 启用 mimalloc 内存分配器 |
| `BUILD_WITH_HTTPS` | `ON` | 启用 TLS 支持 |
| `BUILD_EXAMPLES` | `ON` | 编译示例程序 |
| `ENABLE_DEBUG` | `OFF` | 启用 Debug 模式（-O0） |
| `ENABLE_COVERAGE` | `OFF` | 启用代码覆盖率 |

### 示例配置

编辑 `CMakeLists.txt` 中的 `option()` 默认值，然后运行 `make build`：

```bash
# 最小化配置（仅核心功能）— 在 CMakeLists.txt 中将相应选项设为 OFF
make build

# 完整配置（所有功能）
make build

# 调试配置 — 在 CMakeLists.txt 中将 ENABLE_DEBUG 和 ENABLE_COVERAGE 设为 ON
make build
```

## 验证安装

### 运行测试

```bash
cd build
ctest --output-on-failure
```

### 运行示例

```bash
# 编译示例
make

# 运行 Hello World 示例
./dist/bin/hello_world

# 运行 WebSocket 示例
./dist/bin/websocket_echo_server
```

### 检查版本

```bash
./dist/bin/hello_world --version
```

## 故障排除

### 编译错误

**问题**: 找不到依赖

**解决方案**:
```bash
# 确保子模块已初始化
git submodule update --init --recursive
```

### 链接错误

**问题**: undefined reference to `uv_*`

**解决方案**:
```bash
# 确保链接了正确的库
# 在 CMakeLists.txt 中添加：
target_link_libraries(your_target ${LIBUV_LIB} ${MBEDTLS_LIBS} ...)
```

### CMake 版本过低

**问题**: CMake 3.10+ required

**解决方案**:
```bash
# Linux
sudo apt-get install cmake3

# macOS
brew install cmake

# 从源码安装
wget https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0.tar.gz
tar -xzf cmake-3.28.0.tar.gz
cd cmake-3. 相关
./bootstrap
make build
```

## 下一步

安装完成后，请继续阅读：
- [快速开始](./getting-started.md) - 5 分钟快速上手
- [第一个服务器](./first-server.md) - 创建你的第一个 HTTP 服务器
- [完整教程](./TUTORIAL.md) - 从基础到高级的完整教程

## 获取帮助

如果遇到安装问题：
- 查看 [常见问题](./FAQ.md)
- 提交 [Issue](https://github.com/adam-ikari/uvhttp/issues)
- 查看 错误码参考