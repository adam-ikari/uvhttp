#!/bin/bash
# UVHTTP 标准性能测试脚本
# 此脚本执行规范化的性能测试，确保测试条件一致

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 加载配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/performance_test_config.sh"

# 创建结果目录
mkdir -p "$RESULTS_SUBDIR"

# ==================== 辅助函数 ====================

# 检查端口是否被占用
check_port() {
    if lsof -Pi :$1 -sTCP:LISTEN -t >/dev/null 2>&1 ; then
        return 0  # 端口被占用
    else
        return 1  # 端口可用
    fi
}

# 停止服务器
stop_server() {
    echo -e "${YELLOW}停止 UVHTTP 服务器...${NC}"
    if [ -f "$SERVER_PID" ]; then
        PID=$(cat "$SERVER_PID")
        if kill -0 $PID 2>/dev/null; then
            kill $PID 2>/dev/null || true
            sleep 1
            if kill -0 $PID 2>/dev/null; then
                kill -9 $PID 2>/dev/null || true
            fi
        fi
        rm -f "$SERVER_PID"
    fi
    pkill -9 -f "$SERVER_BIN" 2>/dev/null || true
}

# 启动服务器
start_server() {
    echo -e "${GREEN}启动 UVHTTP 服务器...${NC}"

    # 检查端口
    if check_port $UVHTTP_PORT; then
        echo -e "${RED}端口 $UVHTTP_PORT 已被占用，正在停止占用进程...${NC}"
        stop_server
        sleep 2
    fi

    # 检查可执行文件
    if [ ! -f "$SERVER_BIN" ]; then
        echo -e "${RED}错误: 服务器可执行文件不存在: $SERVER_BIN${NC}"
        echo "请先编译项目: cd $BUILD_DIR && make"
        exit 1
    fi

    # 检查公共目录
    if [ ! -d "$PUBLIC_DIR" ]; then
        echo -e "${RED}错误: 静态文件目录不存在: $PUBLIC_DIR${NC}"
        exit 1
    fi

    # 启动服务器
    nohup "$SERVER_BIN" -d "$PUBLIC_DIR" -p $UVHTTP_PORT > "$SERVER_LOG" 2>&1 &
    echo $! > "$SERVER_PID"

    # 等待服务器启动
    echo "等待服务器启动..."
    for i in {1..15}; do
        if check_port $UVHTTP_PORT; then
            echo -e "${GREEN}UVHTTP 服务器启动成功${NC}"
            return 0
        fi
        sleep 1
    done

    echo -e "${RED}UVHTTP 服务器启动失败${NC}"
    echo "查看日志: cat $SERVER_LOG"
    exit 1
}

# 预热服务器
warmup_server() {
    echo -e "${BLUE}预热服务器...${NC}"
    wrk -t$WARMUP_THREADS -c$WARMUP_CONNECTIONS -d${WARMUP_DURATION}s \
        "http://127.0.0.1:$UVHTTP_PORT/" > /dev/null 2>&1 || true
    sleep 2
    echo -e "${GREEN}预热完成${NC}"
}

# 运行单个测试
run_test() {
    local test_name=$1
    local test_file=$2
    local threads=$3
    local connections=$4
    local duration=$5

    local url="http://127.0.0.1:$UVHTTP_PORT$test_file"
    local output_file="$RESULTS_SUBDIR/${test_name}.txt"
    local json_file="$RESULTS_SUBDIR/${test_name}.json"

    echo -e "${YELLOW}运行测试: $test_name${NC}"
    echo "  URL: $url"
    echo "  配置: $threads 线程, $connections 连接, $duration 秒"

    # 运行 wrk 测试
    wrk -t$threads -c$connections -d${duration}s --latency "$url" | tee "$output_file"

    # 提取性能指标
    local qps=$(grep "Requests/sec:" "$output_file" | awk '{print $2}')
    local avg_latency=$(grep "Latency" "$output_file" | head -1 | awk '{print $2}')
    local p99_latency=$(grep "99%" "$output_file" | awk '{print $2}')

    # 保存 JSON 格式结果
    cat > "$json_file" << EOF
{
  "test_name": "$test_name",
  "test_file": "$test_file",
  "timestamp": "$TIMESTAMP",
  "config": {
    "threads": $threads,
    "connections": $connections,
    "duration": $duration
  },
  "results": {
    "qps": $qps,
    "avg_latency": "$avg_latency",
    "p99_latency": "$p99_latency"
  }
}
EOF

    echo -e "${GREEN}  RPS: $qps${NC}"
    echo -e "${GREEN}  平均延迟: $avg_latency${NC}"
    echo -e "${GREEN}  P99 延迟: $p99_latency${NC}"
    echo ""
}

# 生成测试报告
generate_report() {
    local report_file="$RESULTS_SUBDIR/summary_report.txt"

    echo "==========================================" > "$report_file"
    echo "UVHTTP 性能测试报告" >> "$report_file"
    echo "==========================================" >> "$report_file"
    echo "" >> "$report_file"
    echo "测试时间: $(date)" >> "$report_file"
    echo "测试配置: $STANDARD_THREADS 线程, $STANDARD_CONNECTIONS 连接, $STANDARD_DURATION 秒" >> "$report_file"
    echo "" >> "$report_file"
    echo "==========================================" >> "$report_file"
    echo "测试结果" >> "$report_file"
    echo "==========================================" >> "$report_file"
    echo "" >> "$report_file"

    # 汇总所有测试结果
    for test_name in homepage small_file medium_file large_file; do
        local json_file="$RESULTS_SUBDIR/${test_name}.json"
        if [ -f "$json_file" ]; then
            local file_desc="${TEST_FILE_DESCRIPTIONS[$test_name]}"
            echo "## $file_desc" >> "$report_file"
            echo "" >> "$report_file"
            cat "$json_file" >> "$report_file"
            echo "" >> "$report_file"
        fi
    done

    echo -e "${GREEN}测试报告已生成: $report_file${NC}"
}

# 清理函数
cleanup() {
    echo -e "${YELLOW}清理...${NC}"
    stop_server
    echo -e "${GREEN}清理完成${NC}"
}

# ==================== 主函数 ====================

main() {
    echo "=========================================="
    echo "UVHTTP 标准性能测试"
    echo "=========================================="
    echo ""

    # 打印配置
    print_config

    # 检查 wrk
    if ! check_wrk; then
        exit 1
    fi

    # 设置清理陷阱
    trap cleanup EXIT

    # 启动服务器
    start_server

    # 预热服务器
    warmup_server

    # 运行标准测试（中等并发场景）
    echo "=========================================="
    echo "标准性能测试（中等并发场景）"
    echo "=========================================="
    echo ""

    run_test "homepage" "${TEST_FILES[homepage]}" \
        $STANDARD_THREADS $STANDARD_CONNECTIONS $STANDARD_DURATION

    run_test "small_file" "${TEST_FILES[small_file]}" \
        $STANDARD_THREADS $STANDARD_CONNECTIONS $STANDARD_DURATION

    run_test "medium_file" "${TEST_FILES[medium_file]}" \
        $STANDARD_THREADS $STANDARD_CONNECTIONS $STANDARD_DURATION

    run_test "large_file" "${TEST_FILES[large_file]}" \
        $STANDARD_THREADS $STANDARD_CONNECTIONS $STANDARD_DURATION

    # 生成报告
    generate_report

    echo ""
    echo "=========================================="
    echo -e "${GREEN}所有测试完成！${NC}"
    echo "=========================================="
    echo "测试结果保存在: $RESULTS_SUBDIR"
    echo "测试报告: $RESULTS_SUBDIR/summary_report.txt"
}

# 运行主函数
main