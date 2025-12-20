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
CFLAGS="-std=c11 -Wall -Wextra -O2 -g"
INCLUDES="-I include -I deps/libuv/include -I deps/mbedtls/include"
LIBS="-L deps/libuv/.libs -luv -lpthread -lm"

# 编译完整示例程序
echo "📦 编译完整示例程序..."
$CC $CFLAGS $INCLUDES -o build/examples/complete_example examples/complete_example.c $LIBS

if [ $? -eq 0 ]; then
    echo "✅ 编译成功!"
    echo ""
    echo "🚀 运行示例程序:"
    echo "   cd build/examples"
    echo "   export LD_LIBRARY_PATH=../../deps/libuv/.libs:\$LD_LIBRARY_PATH"
    echo "   ./complete_example"
    echo ""
    echo "🧪 测试命令:"
    echo "   curl http://localhost:8080/"
    echo "   curl http://localhost:8080/api/hello"
    echo "   curl -X POST -d 'Hello UVHTTP' http://localhost:8080/api/echo"
else
    echo "❌ 编译失败!"
    exit 1
fi