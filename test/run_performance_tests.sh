#!/bin/bash
# UVHTTP 性能测试脚本
# 用于执行所有性能测试场景

set -e

# 配置
TEST_DIR="test/performance_results"
SERVER_PID=""
MONITOR_PID=""
HOST="127.0.0.1"
PORT="8080"
URL="http://${HOST}:${PORT}"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 打印信息
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# 创建结果目录
mkdir -p $TEST_DIR

# 检查依赖
check_dependencies() {
    print_info "检查依赖..."
    
    if ! command -v wrk &> /dev/null; then
        print_error "wrk 未安装，请先安装: sudo apt-get install wrk"
        exit 1
    fi
    
    if ! command -v pidstat &> /dev/null; then
        print_warning "pidstat 未安装，将跳过资源监控"
        MONITOR_PID=""
    fi
    
    print_info "依赖检查完成"
}

# 启动服务器
start_server() {
    print_info "启动性能测试服务器..."
    
    # 检查端口是否被占用
    if lsof -i :$PORT &> /dev/null; then
        print_error "端口 $PORT 已被占用"
        exit 1
    fi
    
    # 启动服务器
    ./build/dist/bin/performance_test &
    SERVER_PID=$!
    
    # 等待服务器启动
    sleep 3
    
    # 验证服务器运行
    if ! curl -s -o /dev/null -w "%{http_code}" $URL/ | grep -q "200"; then
        print_error "服务器启动失败"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    
    print_info "服务器已启动 (PID: $SERVER_PID)"
}

# 启动监控
start_monitor() {
    if command -v pidstat &> /dev/null; then
        print_info "启动资源监控..."
        pidstat -p $SERVER_PID -d -h -r -s -u -w 1 > $TEST_DIR/resource_stats.log 2>&1 &
        MONITOR_PID=$!
        print_info "资源监控已启动 (PID: $MONITOR_PID)"
    fi
}

# 停止监控
stop_monitor() {
    if [ -n "$MONITOR_PID" ]; then
        print_info "停止资源监控..."
        kill $MONITOR_PID 2>/dev/null || true
        MONITOR_PID=""
    fi
}

# 停止服务器
stop_server() {
    if [ -n "$SERVER_PID" ]; then
        print_info "停止性能测试服务器..."
        kill $SERVER_PID 2>/dev/null || true
        wait $SERVER_PID 2>/dev/null || true
        SERVER_PID=""
    fi
}

# 清理函数
cleanup() {
    print_info "清理资源..."
    stop_monitor
    stop_server
    exit 0
}

# 捕获退出信号
trap cleanup EXIT INT TERM

# 运行测试场景
run_test() {
    local name=$1
    local threads=$2
    local connections=$3
    local duration=$4
    local iterations=$5
    local path=${6:-"/"}
    
    print_info "运行测试场景: $name"
    print_info "参数: 线程=$threads, 连接=$connections, 时长=${duration}s, 路径=$path"
    
    for i in $(seq 1 $iterations); do
        print_info "  迭代 $i/$iterations..."
        wrk -t$threads -c$connections -d${duration}s \
            ${URL}${path} \
            | tee $TEST_DIR/${name}_${i}.txt
    done
    
    print_info "测试场景 $name 完成"
}

# 运行 POST 测试
run_post_test() {
    local name=$1
    local threads=$2
    local connections=$3
    local duration=$4
    local iterations=$5
    
    print_info "运行 POST 测试场景: $name"
    
    # 创建 Lua 脚本
    cat > /tmp/post.lua << 'EOF'
request = function()
    local body = '{"test":"data"}'
    return wrk.format("POST", "/api", nil, body)
end
EOF
    
    for i in $(seq 1 $iterations); do
        print_info "  迭代 $i/$iterations..."
        wrk -t$threads -c$connections -d${duration}s \
            -s /tmp/post.lua \
            ${URL}/api \
            | tee $TEST_DIR/${name}_${i}.txt
    done
    
    rm -f /tmp/post.lua
    print_info "POST 测试场景 $name 完成"
}

# 主流程
main() {
    echo "========================================"
    echo "  UVHTTP 性能测试"
    echo "========================================"
    echo "开始时间: $(date)"
    echo ""
    
    # 检查依赖
    check_dependencies
    
    # 启动服务器和监控
    start_server
    start_monitor
    
    # 清理系统缓存
    print_info "清理系统缓存..."
    sync
    echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null 2>&1 || true
    sleep 1
    
    echo ""
    echo "========================================"
    echo "  基础性能测试"
    echo "========================================"
    
    # 低并发测试
    run_test "low_concurrent" 2 10 10 5
    
    # 中等并发测试
    run_test "medium_concurrent" 4 50 10 5
    
    # 高并发测试
    run_test "high_concurrent" 8 200 10 5
    
    echo ""
    echo "========================================"
    echo "  扩展性能测试"
    echo "========================================"
    
    # 不同线程数测试
    print_info "测试不同线程数..."
    for threads in 1 2 4 8 16; do
        run_test "thread_${threads}" $threads 50 10 3
    done
    
    # 不同响应体大小测试
    print_info "测试不同响应体大小..."
    run_test "small_response" 4 50 10 3 "/small"
    run_test "medium_response" 4 50 10 3 "/medium"
    run_test "large_response" 4 50 10 3 "/large"
    
    # POST 请求测试
    print_info "测试 POST 请求..."
    run_post_test "post_request" 4 50 10 3
    
    echo ""
    echo "========================================"
    echo "  完整 HTTP 方法测试"
    echo "========================================"
    
    # PUT 请求测试
    print_info "测试 PUT 请求..."
    cat > /tmp/put.lua << 'EOF'
request = function()
    local body = '{"test":"data"}'
    return wrk.format("PUT", "/api/put", nil, body)
end
EOF
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s -s /tmp/put.lua ${URL}/api/put | tee $TEST_DIR/put_request_${i}.txt
    done
    rm -f /tmp/put.lua
    
    # DELETE 请求测试
    print_info "测试 DELETE 请求..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s --method DELETE ${URL}/api/delete | tee $TEST_DIR/delete_request_${i}.txt
    done
    
    # PATCH 请求测试
    print_info "测试 PATCH 请求..."
    cat > /tmp/patch.lua << 'EOF'
request = function()
    local body = '{"test":"data"}'
    return wrk.format("PATCH", "/api/patch", nil, body)
end
EOF
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s -s /tmp/patch.lua ${URL}/api/patch | tee $TEST_DIR/patch_request_${i}.txt
    done
    rm -f /tmp/patch.lua
    
    # HEAD 请求测试
    print_info "测试 HEAD 请求..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s --method HEAD ${URL}/api/head | tee $TEST_DIR/head_request_${i}.txt
    done
    
    # OPTIONS 请求测试
    print_info "测试 OPTIONS 请求..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s --method OPTIONS ${URL}/api/options | tee $TEST_DIR/options_request_${i}.txt
    done
    
    echo ""
    echo "========================================"
    echo "  错误处理性能测试"
    echo "========================================"
    
    # 404 错误测试
    print_info "测试 404 错误处理..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s ${URL}/error/notfound | tee $TEST_DIR/error_404_${i}.txt
    done
    
    # 400 错误测试
    print_info "测试 400 错误处理..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s ${URL}/error/badrequest | tee $TEST_DIR/error_400_${i}.txt
    done
    
    # 500 错误测试
    print_info "测试 500 错误处理..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s ${URL}/error/server | tee $TEST_DIR/error_500_${i}.txt
    done
    
    # 429 限流错误测试
    print_info "测试 429 限流错误处理..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s ${URL}/error/ratelimit | tee $TEST_DIR/error_429_${i}.txt
    done
    
    echo ""
    echo "========================================"
    echo "  静态文件服务性能测试"
    echo "========================================"
    
    # 停止当前服务器
    stop_server
    stop_monitor
    
    # 启动静态文件服务测试服务器
    print_info "启动静态文件服务测试服务器..."
    ./build/dist/bin/performance_test_static &
    SERVER_PID=$!
    sleep 3
    
    # 验证服务器运行
    if ! curl -s -o /dev/null -w "%{http_code}" ${URL}/static/index.html | grep -q "200"; then
        print_error "静态文件服务器启动失败"
        kill $SERVER_PID 2>/dev/null || true
        exit 1
    fi
    
    print_info "静态文件服务器已启动 (PID: $SERVER_PID)"
    
    # 启动监控
    start_monitor
    
    # 小文件测试（1KB）
    print_info "测试小文件（1KB）..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s ${URL}/static/small/test.txt | tee $TEST_DIR/static_small_${i}.txt
    done
    
    # 中等文件测试（10KB）
    print_info "测试中等文件（10KB）..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s ${URL}/static/medium/test.txt | tee $TEST_DIR/static_medium_${i}.txt
    done
    
    # 大文件测试（100KB）
    print_info "测试大文件（100KB）..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s ${URL}/static/large/test.txt | tee $TEST_DIR/static_large_${i}.txt
    done
    
    # HTML 文件测试
    print_info "测试 HTML 文件..."
    for i in {1..3}; do
        print_info "  迭代 $i/3..."
        wrk -t4 -c50 -d10s ${URL}/static/index.html | tee $TEST_DIR/static_html_${i}.txt
    done
    
    # 停止静态文件服务器
    stop_server
    stop_monitor
    
    # 重新启动标准测试服务器
    start_server
    start_monitor
    
    echo ""
    echo "========================================"
    echo "  TLS/HTTPS 性能测试"
    echo "========================================"
    
    # 停止当前服务器
    stop_server
    stop_monitor
    
    # 检查证书文件是否存在
    if [ -f "./test/certs/server.crt" ] && [ -f "./test/certs/server.key" ]; then
        # 启动 TLS 测试服务器
        print_info "启动 TLS/HTTPS 测试服务器..."
        ./build/dist/bin/performance_test_tls &
        SERVER_PID=$!
        sleep 3
        
        # 验证服务器运行
        if curl -k -s -o /dev/null -w "%{http_code}" https://127.0.0.1:8443/ | grep -q "200"; then
            print_info "TLS/HTTPS 服务器已启动 (PID: $SERVER_PID)"
            
            # 启动监控
            start_monitor
            
            # TLS 低并发测试
            print_info "测试 TLS 低并发..."
            for i in {1..3}; do
                print_info "  迭代 $i/3..."
                wrk -t2 -c10 -d10s -k https://127.0.0.1:8443/ | tee $TEST_DIR/tls_low_${i}.txt
            done
            
            # TLS 中等并发测试
            print_info "测试 TLS 中等并发..."
            for i in {1..3}; do
                print_info "  迭代 $i/3..."
                wrk -t4 -c50 -d10s -k https://127.0.0.1:8443/ | tee $TEST_DIR/tls_medium_${i}.txt
            done
            
            # TLS 高并发测试
            print_info "测试 TLS 高并发..."
            for i in {1..3}; do
                print_info "  迭代 $i/3..."
                wrk -t8 -c200 -d10s -k https://127.0.0.1:8443/ | tee $TEST_DIR/tls_high_${i}.txt
            done
            
            # 停止 TLS 服务器
            stop_server
            stop_monitor
        else
            print_warning "TLS 服务器启动失败，跳过 TLS 测试"
            kill $SERVER_PID 2>/dev/null || true
        fi
    else
        print_warning "TLS 证书文件不存在，跳过 TLS 测试"
        print_info "要运行 TLS 测试，请运行以下命令生成证书："
        print_info "  ./test/generate_test_certs.sh"
    fi
    
    echo ""
    echo "========================================"
    echo "  WebSocket 性能测试"
    echo "========================================"
    
    # 检查是否启用了 WebSocket 支持
    if [ -f "./build/dist/bin/performance_test_websocket" ]; then
        # 启动 WebSocket 测试服务器
        print_info "启动 WebSocket 测试服务器..."
        ./build/dist/bin/performance_test_websocket &
        SERVER_PID=$!
        sleep 3
        
        # 验证服务器运行
        if curl -s -o /dev/null -w "%{http_code}" ${URL}/ | grep -q "200"; then
            print_info "WebSocket 服务器已启动 (PID: $SERVER_PID)"
            
            # 启动监控
            start_monitor
            
            # WebSocket 低并发测试（使用简单的 HTTP 请求测试 WebSocket 端点）
            print_info "测试 WebSocket 端点性能..."
            for i in {1..3}; do
                print_info "  迭代 $i/3..."
                wrk -t4 -c50 -d10s ${URL}/ws | tee $TEST_DIR/websocket_${i}.txt
            done
            
            # 停止 WebSocket 服务器
            stop_server
            stop_monitor
        else
            print_warning "WebSocket 服务器启动失败，跳过 WebSocket 测试"
            kill $SERVER_PID 2>/dev/null || true
        fi
    else
        print_warning "WebSocket 支持未启用，跳过 WebSocket 测试"
        print_info "要运行 WebSocket 测试，请使用 -DBUILD_WITH_WEBSOCKET=ON 编译"
    fi
    
    # 重新启动标准测试服务器
    start_server
    start_monitor
    
    echo ""
    echo "========================================"
    echo "  长时间稳定性测试"
    echo "========================================"
    
    # 长时间运行测试（30秒）
    print_info "运行长时间稳定性测试（30秒）..."
    wrk -t4 -c50 -d30s ${URL}/ | tee $TEST_DIR/stability_30s.txt
    
    # 长时间运行测试（60秒）
    print_info "运行长时间稳定性测试（60秒）..."
    wrk -t4 -c50 -d60s ${URL}/ | tee $TEST_DIR/stability_60s.txt
    
    echo ""
    echo "========================================"
    echo "  测试完成"
    echo "========================================"
    echo "结束时间: $(date)"
    echo "结果保存在: $TEST_DIR"
    echo ""
    
    # 生成摘要
    print_info "生成测试摘要..."
    echo "=== 性能测试摘要 ===" > $TEST_DIR/summary.txt
    echo "测试时间: $(date)" >> $TEST_DIR/summary.txt
    echo "" >> $TEST_DIR/summary.txt
    
    for test_name in low_concurrent medium_concurrent high_concurrent; do
        echo "--- $test_name ---" >> $TEST_DIR/summary.txt
        grep "Requests/sec" $TEST_DIR/${test_name}_*.txt | \
            awk '{print $2}' | \
            awk '{sum+=$1; count++} END {print "平均吞吐量: " sum/count " RPS"}' >> $TEST_DIR/summary.txt
        echo "" >> $TEST_DIR/summary.txt
    done
    
    cat $TEST_DIR/summary.txt
}

# 执行主流程
main