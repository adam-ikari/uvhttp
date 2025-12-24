#!/bin/bash

# 测试连接数限制的脚本
echo "测试连接数限制功能..."
echo "配置的最大连接数: 100"

# 创建多个并发连接
echo "创建105个并发连接测试..."

for i in {1..105}; do
    (
        curl -s -m 5 http://localhost:8080/ > /dev/null 2>&1
        echo "连接 $i 完成"
    ) &
    
    # 每10个连接显示一次进度
    if [ $((i % 10)) -eq 0 ]; then
        echo "已启动 $i 个连接..."
    fi
done

# 等待所有连接完成
wait

echo "所有连接测试完成"