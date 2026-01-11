#!/bin/bash
# UVHTTP 性能测试脚本（本地版本）

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 配置
PROJECT_DIR="/home/zhaodi-chen/project/uvhttp"
BUILD_DIR="$PROJECT_DIR/build"
SERVER_BIN="$BUILD_DIR/dist/bin/performance_static_server"
UVHTTP_PORT=8080
PUBLIC_DIR="$PROJECT_DIR/public"
RESULTS_DIR="$PROJECT_DIR/test/uvhttp_performance_results"
WRK_THREADS=4
WRK_CONNECTIONS=100
WRK_DURATION=30

# 创建结果目录
mkdir -p "$RESULTS_DIR"

# 函数：检查端口是否被占用
check_port() {
    if lsof -Pi :$1 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
        return 0  # 端口被占用
    else
        return 1  # 端口可用
    fi
}

# 函数：停止uvhttp服务器
stop_server() {
    echo -e "${YELLOW}停止 UVHTTP 服务器...${NC}"
    # 查找并杀掉进程
    pkill -9 -f "$SERVER_BIN" 2>/dev/null || true
    sleep 1
}

# 函数：启动uvhttp服务器
start_server() {
    echo -e "${GREEN}启动 UVHTTP 服务器...${NC}"
    
    # 检查端口
    if check_port $UVHTTP_PORT; then
        echo -e "${RED}端口 $UVHTTP_PORT 已被占用，正在停止占用进程...${NC}"
        stop_server
    fi
    
    # 检查可执行文件是否存在
    if [ ! -f "$SERVER_BIN" ]; then
        echo -e "${RED}错误: 服务器可执行文件不存在: $SERVER_BIN${NC}"
        echo "请先编译项目: cd build && make"
        exit 1
    fi
    
    # 启动服务器（后台运行）
    nohup "$SERVER_BIN" -d "$PUBLIC_DIR" -p $UVHTTP_PORT > /tmp/uvhttp_server.log 2>&1 &
    
    # 保存PID
    echo $! > /tmp/uvhttp_server.pid
    
    # 等待服务器启动
    sleep 3
    
    # 检查是否启动成功
    if check_port $UVHTTP_PORT; then
        echo -e "${GREEN}UVHTTP 服务器启动成功，监听端口 $UVHTTP_PORT${NC}"
    else
        echo -e "${RED}UVHTTP 服务器启动失败${NC}"
        echo "查看日志: cat /tmp/uvhttp_server.log"
        exit 1
    fi
}

# 函数：运行wrk测试
run_wrk_test() {
    local test_name=$1
    local url=$2
    local output_file="$RESULTS_DIR/${test_name}.txt"
    
    echo -e "${YELLOW}运行测试: $test_name${NC}"
    echo "URL: $url"
    echo "配置: $WRK_THREADS 线程, $WRK_CONNECTIONS 连接, $WRK_DURATION 秒"
    echo ""
    
    wrk -t$WRK_THREADS -c$WRK_CONNECTIONS -d${WRK_DURATION}s "$url" | tee "$output_file"
    
    echo ""
    echo -e "${GREEN}测试结果已保存到: $output_file${NC}"
    echo "----------------------------------------"
    echo ""
}

# 函数：清理
cleanup() {
    echo -e "${YELLOW}清理...${NC}"
    stop_server
    echo -e "${GREEN}清理完成${NC}"
}

# 主函数
main() {
    echo "=========================================="
    echo "UVHTTP 性能测试（本地版本）"
    echo "=========================================="
    echo ""
    
    # 检查wrk是否安装
    if ! command -v wrk &> /dev/null; then
        echo -e "${RED}错误: wrk 未安装${NC}"
        echo "请安装 wrk: sudo apt-get install wrk"
        exit 1
    fi
    
    # 检查public目录
    if [ ! -d "$PUBLIC_DIR" ]; then
        echo -e "${RED}错误: 静态文件目录不存在: $PUBLIC_DIR${NC}"
        exit 1
    fi
    
    # 设置清理陷阱
    trap cleanup EXIT
    
    # 启动服务器
    start_server
    
    # 等待服务器完全启动
    sleep 2
    
    # 运行测试
    echo "开始性能测试..."
    echo ""
    
    # 测试1: 主页
    run_wrk_test "uvhttp_homepage" "http://127.0.0.1:$UVHTTP_PORT/"
    
    # 测试2: 静态文件
    run_wrk_test "uvhttp_static" "http://127.0.0.1:$UVHTTP_PORT/static/test.html"
    
    # 测试3: 不同并发级别
    run_wrk_test "uvhttp_low_concurrency" "http://127.0.0.1:$UVHTTP_PORT/"
    WRK_CONNECTIONS=50
    run_wrk_test "uvhttp_medium_concurrency" "http://127.0.0.1:$UVHTTP_PORT/"
    WRK_CONNECTIONS=200
    run_wrk_test "uvhttp_high_concurrency" "http://127.0.0.1:$UVHTTP_PORT/"
    
    echo ""
    echo "=========================================="
    echo -e "${GREEN}所有测试完成！${NC}"
    echo "=========================================="
    echo "测试结果保存在: $RESULTS_DIR"
}

# 运行主函数
main