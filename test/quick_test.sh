#!/bin/bash

echo "测试连接数限制功能..."
echo "预期最大连接数: 500"

# 快速测试几个连接
for i in {1..3}; do
    response=$(curl -s -w "HTTP_CODE:%{http_code}" -m 2 http://localhost:8080/ 2>/dev/null)
    http_code=$(echo "$response" | grep -o "HTTP_CODE:[0-9]*" | cut -d: -f2)
    echo "连接 $i: HTTP $http_code"
done

echo "基本功能测试完成"