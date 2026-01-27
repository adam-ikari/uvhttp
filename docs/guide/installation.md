# 安装指南

本指南介绍如何在不同平台上安装 UVHTTP。

## 系统要求

### 最低要求
- **CMake**: 3.10 或更高版本
- **C 编译器**: 
  - GCC 4.9+ (Linux)
  - Clang 3.5+ (macOS, Linux)
  - MSVC 2015+ (Windows)
- **操作系统**: Linux, macOS, Windows

### 推荐要求
- **CMake**: 3.15 或更高版本
- **C 编译器**: 
  - GCC 7+ (Linux)
  - Clang 10+ (macOS, Linux)
  - MSVC 2019+ (Windows)

## 从源码编译

### 1. 克隆仓库

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

### 2. 创建构建目录

```bash
mkdir build && cd build
```

### 3. 配置项目

```bash
# 基本配置（Release 模式）
cmake ..

# Debug 模式
cmake -DCMAKE_BUILD_TYPE=Debug ..

# 启用所有功能
cmake -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_TLS=ON \
      ..

# 启用代码覆盖率
cmake -DENABLE_COVERAGE=ON ..
```

### 4. 编译

```bash
# 使用所有 CPU 核心编译
make -j$(nproc)

# 或使用特定数量的核心
make -j4
```

### 5. 安装（可选）

```bash
make install
```

## 平台特定说明

### Ubuntu/Debian

#### 安装依赖

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential \
    libuv1-dev \
    libssl-dev
```

#### 编译

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### CentOS/RHEL

#### 安装依赖

```bash
sudo yum groupinstall "Development Tools"
sudo yum install -y \
    cmake3 \
    libuv-devel \
    openssl-devel
```

#### 编译

```bash
mkdir build && cd build
cmake3 ..
make -j$(nproc)
```

### macOS

#### 使用 Homebrew

```bash
# 安装 Homebrew（如果尚未安装）
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake libuv openssl

# 编译
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.nc)
```

#### 使用 MacPorts

```bash
# 安装 MacPorts（如果尚未安装）
# 然后安装依赖
sudo port install cmake libuv openssl

# 编译
mkdir build && cd build
cmake ..
make -j$(sysctl -n hw.nc)
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
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg路径]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

#### 使用预编译依赖

1. 下载并安装 libuv: https://github.com/libuv/libuv/releases
2. 下载并安装 OpenSSL: https://slproweb.com/products/Win32OpenSSL.html
3. 在 CMake 配置时指定库路径：
   ```bash
   cmake .. -DLIBUV_INCLUDE_DIR=[libuv include路径] \
            -DLIBUV_LIBRARY=[libuv lib路径] \
            -DOPENSSL_INCLUDE_DIR=[OpenSSL include路径] \
            -DOPENSSL_LIBRARY=[OpenSSL lib路径]
   ```

## 构建选项

### 常用 CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `BUILD_WITH_WEBSOCKET` | `ON` | 启用 WebSocket 支持 |
| `BUILD_WITH_MIMALLOC` | `ON` | 启用 mimalloc 内存分配器 |
| `BUILD_WITH_TLS` | `ON` | 启用 TLS 支持 |
| `BUILD_EXAMPLES` | `ON` | 编译示例程序 |
| `ENABLE_DEBUG` | `OFF` | 启用 Debug 模式（-O0） |
| `ENABLE_COVERAGE` | `OFF` | 启用代码覆盖率 |

### 示例配置

```bash
# 最小化配置（仅核心功能）
cmake -DBUILD_WITH_WEBSOCKET=OFF \
      -DBUILD_WITH_MIMALLOC=OFF \
      -DBUILD_WITH_TLS=OFF \
      -DBUILD_EXAMPLES=OFF \
      ..

# 完整配置（所有功能）
cmake -DBUILD_WITH_WEBSOCKET=ON \
      -DBUILD_WITH_MIMALLOC=ON \
      -DBUILD_WITH_TLS=ON \
      -DBUILD_EXAMPLES=ON \
      ..

# 调试配置
cmake -DENABLE_DEBUG=ON \
      -DENABLE_COVERAGE=ON \
      ..
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

**问题**: 找不到 libuv 或其他依赖

**解决方案**:
```bash
# 检查依赖是否已安装
# Linux
ldconfig -p | grep libuv

# macOS
otool -L /usr/local/lib | grep libuv

# Windows
where libuv.dll
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
make -j$(nproc)
sudo make install
```

## 下一步

安装完成后，请继续阅读：
- [快速开始](./getting-started.md) - 5 分钟快速上手
- [第一个服务器](./first-server.md) - 创建你的第一个 HTTP 服务器
- [完整教程](../TUTORIAL.md) - 从基础到高级的完整教程

## 获取帮助

如果遇到安装问题：
- 查看 [常见问题](./faq.md)
- 提交 [Issue](https://github.com/adam-ikari/uvhttp/issues)
- 查看 [错误码参考](../ERROR_CODES.md)