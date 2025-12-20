#!/bin/bash

# UVHTTP示例程序编译脚本

echo "🔨 编译UVHTTP示例程序..."

# 检查依赖
if [ ! -d "deps/libuv" ]; then
    echo "❌ 错误: 找不到libuv依赖目录"
    echo "请确保已正确安装所有依赖"
    exit 1
fi

# 创建输出目录
mkdir -p build/examples

# 编译参数
CC=gcc
CFLAGS="-std=c11 -Wall -Wextra -O2 -g -D_GNU_SOURCE"
INCLUDES="-I include -I deps/libuv/include -I deps/mbedtls/include -I third_party/llhttp/include -I deps/libwebsockets/include"
LIBS="-L build -L deps/libuv/.libs -luvhttp -lllhttp -luv -lpthread -lm"

# 编译简单示例程序
echo "📦 编译简单示例程序..."
$CC $CFLAGS $INCLUDES -o build/examples/simple_server examples/simple_server.c $LIBS

if [ $? -eq 0 ]; then
    echo "✅ 编译成功!"
    echo ""
    echo "🚀 运行示例程序:"
    echo "   cd build/examples"
    echo "   export LD_LIBRARY_PATH=../../deps/libuv/.libs:\$LD_LIBRARY_PATH"
    echo "   ./simple_server"
    echo ""
    echo "🧪 测试命令:"
    echo "   curl http://localhost:8080/"
    echo "   curl http://localhost:8080/api"
else
    echo "❌ 编译失败!"
    exit 1
fi