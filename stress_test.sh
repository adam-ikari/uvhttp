#!/bin/bash
# 压力测试脚本
echo "启动 UVHTTP 服务器..."

# 启动服务器
./build/dist/bin/helloworld &
SERVER_PID=$!
echo "服务器 PID: $SERVER_PID"

# 等待服务器启动
sleep 2

# 检查服务器是否在运行
if kill -0 $SERVER_PID 2>/dev/null; then
    echo "服务器正在运行，开始压力测试..."
    
    # 运行 ab 压力测试
    timeout 15s ab -n 100 -c 10 -k http://127.0.0.1:8080/
    AB_EXIT_CODE=$?
    
    echo "ab 测试退出码: $AB_EXIT_CODE"
    
    # 正常终止服务器
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo "发送 SIGTERM 信号终止服务器..."
        kill -TERM $SERVER_PID
        sleep 2
    fi
    
    # 检查服务器进程是否已终止
    if kill -0 $SERVER_PID 2>/dev/null; then
        echo "服务器未正常终止，强制终止..."
        kill -9 $SERVER_PID 2>/dev/null || true
    else
        echo "服务器已正常终止"
    fi
else
    echo "错误：服务器未启动"
fi

echo "压力测试完成"