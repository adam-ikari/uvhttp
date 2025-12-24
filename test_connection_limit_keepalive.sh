#!/bin/bash

# 测试连接数限制的脚本 - 使用长连接
echo "测试连接数限制功能..."
echo "配置的最大连接数: 10"

# 创建多个长连接
echo "创建15个长连接测试..."

# 创建临时文件来存储进程ID
temp_pids="/tmp/test_pids.txt"
rm -f $temp_pids

for i in {1..15}; do
    (
        # 使用nc创建长连接
        echo -e "GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n" | nc localhost 8080 &
        echo $! >> $temp_pids
        echo "启动连接 $i (PID: $!)"
    ) &
    
    # 控制并发速度
    sleep 0.2
done

# 等待所有连接启动
sleep 2

echo "检查当前活动连接数..."
netstat -an | grep :8080 | grep ESTABLISHED | wc -l

# 尝试建立另一个连接
echo "尝试建立第16个连接..."
response=$(curl -s -w "HTTP_CODE:%{http_code}" -m 5 http://localhost:8080/ 2>/dev/null)
http_code=$(echo "$response" | grep -o "HTTP_CODE:[0-9]*" | cut -d: -f2)

if [ "$http_code" = "200" ]; then
    echo "连接 16: 成功 (HTTP 200)"
elif [ "$http_code" = "503" ]; then
    echo "连接 16: 被拒绝 (HTTP 503) - 连接数限制生效"
else
    echo "连接 16: 失败 (HTTP $http_code)"
fi

# 清理所有连接
if [ -f $temp_pids ]; then
    echo "清理所有连接..."
    while read pid; do
        kill $pid 2>/dev/null
    done < $temp_pids
    rm -f $temp_pids
fi

echo "测试完成"