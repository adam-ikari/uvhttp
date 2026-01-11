# UVHTTP 构建指南

## 快速开始

### 一键构建

```bash
make build
```

这将自动：
1. 检查并编译所有依赖（libuv、mbedtls、llhttp）
2. 编译 uvhttp 核心库
3. 编译性能测试程序

### 其他构建命令

```bash
make                    # 仅编译项目（假设依赖已编译）
make rebuild            # 完全重新构建
make clean              # 清理构建文件
make test               # 运行测试
make examples           # 编译示例程序
```

## 构建选项

可以通过环境变量或直接修改 Makefile 来自定义构建选项：

```bash
BUILD_DIR=custom_build make build    # 使用自定义构建目录
BUILD_TYPE=Debug make build          # 使用 Debug 模式
```

## 依赖要求

- GCC 编译器
- Python 3（用于 mbedtls 配置）
- CMake 3.10+

## 输出文件

编译后的文件位于 `build/dist/` 目录：
- `bin/` - 可执行文件
- `lib/` - 库文件
- `include/` - 头文件

## 性能测试

性能测试程序位于 `build/dist/bin/`：
- `performance_test` - 基础性能测试
- `performance_test_static` - 静态文件服务性能测试

## 故障排除

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

```bash
make rebuild
```

这将清理所有构建文件并重新编译所有内容。