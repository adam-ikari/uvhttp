#!/bin/bash

# 测试连接数限制的脚本
echo "测试连接数限制功能..."
echo "配置的最大连接数: 10"

# 创建15个并发连接（超过限制的10个）
echo "创建15个并发连接测试..."

for i in {1..15}; do
    (
        response=$(curl -s -w "HTTP_CODE:%{http_code}" -m 5 http://localhost:8080/ 2>/dev/null)
        http_code=$(echo "$response" | grep -o "HTTP_CODE:[0-9]*" | cut -d: -f2)
        
        if [ "$http_code" = "200" ]; then
            echo "连接 $i: 成功 (HTTP 200)"
        elif [ "$http_code" = "503" ]; then
            echo "连接 $i: 被拒绝 (HTTP 503) - 连接数限制生效"
        else
            echo "连接 $i: 失败 (HTTP $http_code)"
        fi
    ) &
    
    # 控制并发速度
    sleep 0.1
done

# 等待所有连接完成
wait

echo "所有连接测试完成"