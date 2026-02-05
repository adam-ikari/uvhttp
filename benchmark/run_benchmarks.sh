#!/bin/bash
# UVHTTP HTTP 性能基准测试自动化脚本
# 
# 这个脚本使用标准的 HTTP 性能测试工具（wrk、ab）对 UVHTTP 服务器进行全面测试
# 支持多种测试场景、并发级别和自动化报告生成

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
BENCHMARK_DIR="$BUILD_DIR/dist/bin"
RESULTS_DIR="$SCRIPT_DIR/results"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
RUN_DIR="$RESULTS_DIR/run_$TIMESTAMP"

# 测试配置
DEFAULT_PORT=18081
TEST_DURATION=30  # 测试时长（秒）
WARMUP_DURATION=5  # 预热时长（秒）

# 并发级别配置
CONCURRENCY_LEVELS=(
    "2:10:低并发"      # 线程:连接:描述
    "4:50:中等并发"
    "8:200:高并发"
    "16:500:极高并发"
)

# 测试端点配置
TEST_ENDPOINTS=(
    "/:简单文本:13"
    "/json:JSON响应:50"
    "/small:小响应:1023"
    "/medium:中等响应:10239"
    "/large:大响应:102399"
    "/health:健康检查:22"
)

# 创建结果目录
mkdir -p "$RUN_DIR"

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    log_info "检查依赖..."
    
    local missing_deps=()
    
    # 检查 wrk
    if ! command -v wrk &> /dev/null; then
        missing_deps+=("wrk")
    fi
    
    # 检查 ab（可选）
    if ! command -v ab &> /dev/null; then
        log_warning "ab 未安装，将跳过 ab 测试"
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_error "缺少必要的依赖: ${missing_deps[*]}"
        log_info "请安装 wrk: sudo apt-get install wrk"
        exit 1
    fi
    
    log_success "所有依赖已安装"
}

# 检查测试环境
check_test_environment() {
    log_info "检查测试环境..."
    
    local issues_found=0
    
    # 1. 检查 CPU 频率锁定
    log_info "  检查 CPU 频率..."
    if command -v cpupower &> /dev/null; then
        local governor=$(cpupower frequency-info -p | grep "governor" | awk '{print $3}')
        if [ "$governor" != "performance" ]; then
            log_warning "  CPU 频率调节器为 '$governor'，建议设置为 'performance' 以获得稳定性能"
            log_info "  设置命令: sudo cpupower frequency-set -g performance"
            issues_found=$((issues_found + 1))
        else
            log_success "  CPU 频率调节器: performance "
        fi
    else
        log_warning "  cpupower 未安装，无法检查 CPU 频率调节器"
        log_info "  安装命令: sudo apt-get install linux-tools-common linux-tools-generic"
    fi
    
    # 2. 检查 Swap 状态
    log_info "  检查 Swap 状态..."
    local swap_total=$(free -m | awk '/^Swap:/ {print $2}')
    local swap_used=$(free -m | awk '/^Swap:/ {print $3}')
    if [ "$swap_total" -gt 0 ] && [ "$swap_used" -gt 0 ]; then
        log_warning "  Swap 正在使用 (${swap_used}MB / ${swap_total}MB)，可能影响性能测试结果"
        log_info "  禁用 Swap: sudo swapoff -a"
        issues_found=$((issues_found + 1))
    elif [ "$swap_total" -gt 0 ]; then
        log_success "  Swap 已配置但未使用 "
    else
        log_success "  Swap 未配置 "
    fi
    
    # 3. 检查文件描述符限制
    log_info "  检查文件描述符限制..."
    local soft_limit=$(ulimit -n)
    local required_limit=65536
    if [ "$soft_limit" -lt "$required_limit" ]; then
        log_warning "  文件描述符限制过低 ($soft_limit)，建议至少 $required_limit"
        log_info "  临时设置: ulimit -n $required_limit"
        log_info "  永久设置: 在 /etc/security/limits.conf 添加 '* soft nofile $required_limit'"
        issues_found=$((issues_found + 1))
    else
        log_success "  文件描述符限制: $soft_limit "
    fi
    
    # 4. 检查系统负载
    log_info "  检查系统负载..."
    local load_1min=$(uptime | awk -F'load average:' '{print $2}' | awk '{print $1}' | sed 's/,//')
    local cpu_count=$(nproc)
    local load_per_cpu=$(echo "$load_1min / $cpu_count" | bc -l 2>/dev/null || echo "0")
    local load_threshold=0.5
    
    if (( $(echo "$load_per_cpu > $load_threshold" | bc -l 2>/dev/null || echo 0) )); then
        log_warning "  系统负载较高 ($load_1min, $load_per_cpu per CPU)，可能影响性能测试结果"
        log_info "  建议在系统负载较低时运行测试"
        issues_found=$((issues_found + 1))
    else
        log_success "  系统负载: $load_1min "
    fi
    
    # 5. 检查可用内存
    log_info "  检查可用内存..."
    local available_mem=$(free -m | awk '/^Mem:/ {print $7}')
    local required_mem=1024  # 至少 1GB 可用内存
    if [ "$available_mem" -lt "$required_mem" ]; then
        log_warning "  可用内存不足 (${available_mem}MB)，建议至少 ${required_mem}MB"
        issues_found=$((issues_found + 1))
    else
        log_success "  可用内存: ${available_mem}MB "
    fi
    
    # 6. 检查网络配置
    log_info "  检查网络配置..."
    local tcp_backlog=$(sysctl net.core.somaxconn 2>/dev/null | awk '{print $3}')
    if [ "$tcp_backlog" -lt 1024 ]; then
        log_warning "  TCP backlog 较低 ($tcp_backlog)，建议设置为 1024 或更高"
        log_info "  设置命令: sudo sysctl -w net.core.somaxconn=1024"
        issues_found=$((issues_found + 1))
    else
        log_success "  TCP backlog: $tcp_backlog "
    fi
    
    # 7. 检查端口占用
    log_info "  检查端口 $DEFAULT_PORT..."
    if command -v lsof &> /dev/null; then
        if lsof -i :$DEFAULT_PORT &> /dev/null; then
            log_warning "  端口 $DEFAULT_PORT 已被占用"
            log_info "  查看占用: lsof -i :$DEFAULT_PORT"
            issues_found=$((issues_found + 1))
        else
            log_success "  端口 $DEFAULT_PORT 可用 "
        fi
    elif command -v ss &> /dev/null; then
        if ss -tuln | grep -q ":$DEFAULT_PORT "; then
            log_warning "  端口 $DEFAULT_PORT 已被占用"
            log_info "  查看占用: ss -tuln | grep $DEFAULT_PORT"
            issues_found=$((issues_found + 1))
        else
            log_success "  端口 $DEFAULT_PORT 可用 "
        fi
    else
        log_warning "  无法检查端口占用（lsof 和 ss 都未安装）"
    fi
    
    # 总结
    if [ $issues_found -gt 0 ]; then
        log_warning "发现 $issues_found 个环境问题，建议修复后重新运行测试以获得准确结果"
        log_info "是否继续测试？(Ctrl+C 取消，回车继续)"
        read -r
    else
        log_success "测试环境检查通过 "
    fi
}

# 编译基准测试程序
compile_benchmark_server() {
    log_info "编译基准测试服务器..."
    
    cd "$PROJECT_ROOT"
    
    if [ ! -d "$BUILD_DIR" ]; then
        log_info "创建构建目录..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release ..
    fi
    
    cd "$BUILD_DIR"
    make -j$(nproc) benchmark_rps
    
    if [ ! -f "$BENCHMARK_DIR/benchmark_rps" ]; then
        log_error "benchmark_rps 编译失败"
        exit 1
    fi
    
    log_success "基准测试服务器编译完成"
}

# 启动测试服务器
start_server() {
    local port=$1
    
    log_info "启动测试服务器 (端口: $port)..."
    
    "$BENCHMARK_DIR/benchmark_rps" $port > "$RUN_DIR/server.log" 2>&1 &
    local server_pid=$!
    
    # 等待服务器启动
    sleep 3
    
    # 检查服务器是否启动成功
    if ! kill -0 $server_pid 2>/dev/null; then
        log_error "服务器启动失败"
        cat "$RUN_DIR/server.log"
        exit 1
    fi
    
    # 测试服务器是否响应
    if ! curl -s "http://127.0.0.1:$port/health" > /dev/null 2>&1; then
        log_error "服务器无响应"
        kill $server_pid 2>/dev/null || true
        exit 1
    fi
    
    log_success "服务器已启动 (PID: $server_pid)"
    echo $server_pid
}

# 停止测试服务器
stop_server() {
    local server_pid=$1
    
    log_info "停止测试服务器..."
    kill $server_pid 2>/dev/null || true
    wait $server_pid 2>/dev/null || true
    log_success "服务器已停止"
}

# 运行 wrk 测试
run_wrk_test() {
    local endpoint=$1
    local threads=$2
    local connections=$3
    local duration=$4
    local port=$5
    local output_file=$6
    
    local url="http://127.0.0.1:$port$endpoint"
    
    log_info "  wrk 测试: $threads 线程 / $connections 连接 / $duration 秒"
    
    wrk -t$threads -c$connections -d${duration}s "$url" > "$output_file" 2>&1
    
    if [ $? -ne 0 ]; then
        log_warning "  wrk 测试失败"
        return 1
    fi
}

# 运行 ab 测试（可选）
run_ab_test() {
    local endpoint=$1
    local connections=$2
    local requests=$3
    local port=$4
    local output_file=$5
    
    if ! command -v ab &> /dev/null; then
        return 0
    fi
    
    local url="http://127.0.0.1:$port$endpoint"
    
    log_info "  ab 测试: $connections 并发 / $requests 请求"
    
    ab -n $requests -c $connections -k "$url" > "$output_file" 2>&1
    
    if [ $? -ne 0 ]; then
        log_warning "  ab 测试失败"
        return 1
    fi
}

# 提取 wrk 结果
extract_wrk_results() {
    local output_file=$1
    
    # 提取 RPS
    local rps=$(grep "Requests/sec" "$output_file" | awk '{print $2}')
    echo "$rps"
}

# 提取 ab 结果
extract_ab_results() {
    local output_file=$1
    
    # 提取 RPS
    local rps=$(grep "Requests per second" "$output_file" | awk '{print $4}')
    echo "$rps"
}

# 运行完整的性能测试
run_performance_tests() {
    local server_pid=$1
    local port=$2
    
    log_info "开始性能测试..."
    
    # 创建 CSV 文件
    local csv_file="$RUN_DIR/results.csv"
    echo "Endpoint,Threads,Connections,RPS" > "$csv_file"
    
    # 预热
    log_info "预热服务器 (${WARMUP_DURATION} 秒)..."
    wrk -t4 -c50 -d${WARMUP_DURATION}s "http://127.0.0.1:$port/" > /dev/null 2>&1 || true
    sleep 1
    
    # 对每个端点进行测试
    for endpoint_info in "${TEST_ENDPOINTS[@]}"; do
        IFS=':' read -r endpoint name size <<< "$endpoint_info"
        
        log_info "测试端点: $endpoint ($name, ${size} bytes)"
        
        # 对每个并发级别进行测试
        for level_info in "${CONCURRENCY_LEVELS[@]}"; do
            IFS=':' read -r threads connections desc <<< "$level_info"
            
            local test_dir="$RUN_DIR/${endpoint//\//_}/${desc}"
            mkdir -p "$test_dir"
            
            # 运行 wrk 测试
            local wrk_output="$test_dir/wrk.txt"
            run_wrk_test "$endpoint" "$threads" "$connections" "$TEST_DURATION" "$port" "$wrk_output"
            
            # 提取结果
            local rps=$(extract_wrk_results "$wrk_output")
            if [ -n "$rps" ]; then
                echo "$endpoint,$threads,$connections,$rps" >> "$csv_file"
                log_success "  RPS: $rps"
            fi
            
            # 运行 ab 测试（可选）
            local requests=$((connections * 100))
            local ab_output="$test_dir/ab.txt"
            run_ab_test "$endpoint" "$connections" "$requests" "$port" "$ab_output"
        done
        
        echo ""
    done
    
    log_success "性能测试完成"
}

# 运行突发流量测试
run_burst_traffic_test() {
    local server_pid=$1
    local port=$2
    
    log_info "开始突发流量测试..."
    
    local burst_dir="$RUN_DIR/burst_traffic"
    mkdir -p "$burst_dir"
    
    # 测试场景 1: 从低并发突然增加到高并发
    log_info "场景 1: 低并发 -> 高并发突发"
    log_info "  启动低并发测试 (2 线程 / 10 连接, 5 秒)..."
    wrk -t2 -c10 -d5s "http://127.0.0.1:$port/" > "$burst_dir/phase1_low.txt" 2>&1
    
    log_info "  突然增加到高并发 (16 线程 / 500 连接, 20 秒)..."
    wrk -t16 -c500 -d20s "http://127.0.0.1:$port/" > "$burst_dir/phase2_high.txt" 2>&1
    
    sleep 2
    
    # 测试场景 2: 多次突发
    log_info "场景 2: 多次突发测试"
    for i in {1..3}; do
        log_info "  突发 $i/3: 低负载 (2 线程 / 10 连接, 3 秒)..."
        wrk -t2 -c10 -d3s "http://127.0.0.1:$port/" > "$burst_dir/multi_burst_${i}_low.txt" 2>&1
        
        log_info "  突发 $i/3: 高负载 (16 线程 / 500 连接, 5 秒)..."
        wrk -t16 -c500 -d5s "http://127.0.0.1:$port/" > "$burst_dir/multi_burst_${i}_high.txt" 2>&1
        
        sleep 1
    done
    
    # 测试场景 3: 持续高并发突发
    log_info "场景 3: 持续高并发突发测试"
    log_info "  启动持续突发测试 (16 线程 / 500 连接, 30 秒)..."
    wrk -t16 -c500 -d30s "http://127.0.0.1:$port/" > "$burst_dir/sustained_burst.txt" 2>&1
    
    # 生成突发流量测试报告
    local burst_report="$burst_dir/report.md"
    cat > "$burst_report" << EOF
# 突发流量测试报告

## 测试时间
$(date)

## 测试场景

### 场景 1: 低并发 -> 高并发突发
- **阶段 1**: 2 线程 / 10 连接, 5 秒
- **阶段 2**: 16 线程 / 500 连接, 20 秒

**结果**:
EOF
    
    # 提取结果
    local rps1=$(grep "Requests/sec" "$burst_dir/phase1_low.txt" | awk '{print $2}')
    local rps2=$(grep "Requests/sec" "$burst_dir/phase2_high.txt" | awk '{print $2}')
    
    echo "- 低并发 RPS: $rps1" >> "$burst_report"
    echo "- 高并发 RPS: $rps2" >> "$burst_report"
    
    cat >> "$burst_report" << EOF

### 场景 2: 多次突发测试
- **循环次数**: 3 次
- **每次突发**: 低负载 3 秒 -> 高负载 5 秒

**结果**:
EOF
    
    local total_rps=0
    local count=0
    for i in {1..3}; do
        local rps_low=$(grep "Requests/sec" "$burst_dir/multi_burst_${i}_low.txt" | awk '{print $2}')
        local rps_high=$(grep "Requests/sec" "$burst_dir/multi_burst_${i}_high.txt" | awk '{print $2}')
        echo "- 突发 $i: 低 $rps_low -> 高 $rps_high" >> "$burst_report"
        if [ -n "$rps_high" ]; then
            total_rps=$((total_rps + $(echo "$rps_high" | cut -d. -f1)))
            count=$((count + 1))
        fi
    done
    
    if [ $count -gt 0 ]; then
        local avg_rps=$((total_rps / count))
        echo "- 平均高并发 RPS: $avg_rps" >> "$burst_report"
    fi
    
    cat >> "$burst_report" << EOF

### 场景 3: 持续高并发突发
- **配置**: 16 线程 / 500 连接, 30 秒

**结果**:
EOF
    
    local rps_sustained=$(grep "Requests/sec" "$burst_dir/sustained_burst.txt" | awk '{print $2}')
    echo "- 持续高并发 RPS: $rps_sustained" >> "$burst_report"
    
    cat >> "$burst_report" << EOF

## 性能评估

### 突发响应能力
- **低并发稳定性**: $rps1 RPS
- **突发处理能力**: $rps2 RPS
- **突发响应比**: $(echo "scale=2; $rps2 / $rps1" | bc 2>/dev/null || echo "N/A")x

### 多次突发稳定性
- **平均高并发 RPS**: $avg_rps
- **波动范围**: 需要分析各次突发结果

### 持续突发稳定性
- **持续高并发 RPS**: $rps_sustained

## 建议

EOF
    
    # 评估突发性能
    if [ -n "$rps2" ] && [ -n "$rps1" ]; then
        local burst_ratio=$(echo "scale=2; $rps2 / $rps1" | bc 2>/dev/null || echo "0")
        if (( $(echo "$burst_ratio >= 0.8" | bc -l 2>/dev/null || echo 0) )); then
            echo " 突发响应能力优秀（突发响应比: ${burst_ratio}x）" >> "$burst_report"
        elif (( $(echo "$burst_ratio >= 0.5" | bc -l 2>/dev/null || echo 0) )); then
            echo "  突发响应能力良好（突发响应比: ${burst_ratio}x）" >> "$burst_report"
        else
            echo " 突发响应能力需要改进（突发响应比: ${burst_ratio}x）" >> "$burst_report"
        fi
    fi
    
    log_success "突发流量测试完成"
    log_info "报告: $burst_report"
}

# 生成测试报告
generate_report() {
    log_info "生成测试报告..."
    
    local report_file="$RUN_DIR/report.md"
    
    cat > "$report_file" << EOF
# UVHTTP HTTP 性能基准测试报告

## 测试信息

- **测试时间**: $(date)
- **测试时长**: ${TEST_DURATION} 秒
- **预热时长**: ${WARMUP_DURATION} 秒
- **服务器端口**: $DEFAULT_PORT
- **结果目录**: $RUN_DIR

## 测试端点

| 端点 | 描述 | 响应大小 |
|------|------|----------|
EOF
    
    # 添加端点信息
    for endpoint_info in "${TEST_ENDPOINTS[@]}"; do
        IFS=':' read -r endpoint name size <<< "$endpoint_info"
        echo "| $endpoint | $name | ${size} bytes |" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF

## 并发级别

| 级别 | 线程数 | 连接数 |
|------|--------|--------|
EOF
    
    # 添加并发级别信息
    for level_info in "${CONCURRENCY_LEVELS[@]}"; do
        IFS=':' read -r threads connections desc <<< "$level_info"
        echo "| $desc | $threads | $connections |" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF

## 测试结果

### RPS 性能

| 端点 | 线程数 | 连接数 | RPS |
|------|--------|--------|-----|
EOF
    
    # 添加测试结果
    if [ -f "$RUN_DIR/results.csv" ]; then
        tail -n +2 "$RUN_DIR/results.csv" | while IFS=',' read -r endpoint threads connections rps; do
            echo "| $endpoint | $threads | $connections | $rps |" >> "$report_file"
        done
    fi
    
    cat >> "$report_file" << EOF

## 性能评估

根据 UVHTTP 性能基准目标：

- **低并发（2 线程 / 10 连接）**: ≥ 17,000 RPS
- **中等并发（4 线程 / 50 连接）**: ≥ 17,000 RPS
- **高并发（8 线程 / 200 连接）**: ≥ 16,000 RPS
- **极高并发（16 线程 / 500 连接）**: ≥ 15,000 RPS

## 详细日志

详细的测试日志保存在以下目录：

EOF
    
    # 列出所有测试目录
    find "$RUN_DIR" -type d -name "*" ! -path "$RUN_DIR" | while read -r dir; do
        echo "- $(basename "$dir")/" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF

## 服务器日志

服务器运行日志: \`server.log\`

## 使用说明

### 运行单个测试

\`\`\`bash
# 启动服务器
./build/dist/bin/benchmark_rps 18081

# 使用 wrk 测试
wrk -t4 -c100 -d30s http://127.0.0.1:18081/
wrk -t4 -c100 -d30s http://127.0.0.1:18081/json
wrk -t4 -c100 -d30s http://127.0.0.1:18081/small
wrk -t4 -c100 -d30s http://127.0.0.1:18081/medium
wrk -t4 -c100 -d30s http://127.0.0.1:18081/large

# 使用 ab 测试
ab -n 100000 -c 100 -k http://127.0.0.1:18081/
\`\`\`

### 运行完整测试

\`\`\`bash
cd benchmark
./run_benchmarks.sh
\`\`\`

---

**报告生成时间**: $(date)
**UVHTTP 版本**: $(cd "$PROJECT_ROOT" && git describe --tags --always 2>/dev/null || echo "unknown")
EOF
    
    log_success "测试报告已生成: $report_file"
}

# 清理函数
cleanup() {
    if [ -n "$server_pid" ] && kill -0 $server_pid 2>/dev/null; then
        log_info "清理中..."
        stop_server $server_pid
    fi
}

# 设置清理陷阱
trap cleanup EXIT INT TERM

# 主函数
main() {
    echo "========================================"
    echo "  UVHTTP HTTP 性能基准测试"
    echo "========================================"
    echo ""
    
    # 检查依赖
    check_dependencies
    echo ""
    
    # 检查测试环境
    check_test_environment
    echo ""
    
    # 编译基准测试服务器
    compile_benchmark_server
    echo ""
    
    # 启动服务器
    server_pid=$(start_server $DEFAULT_PORT)
    echo ""
    
    # 运行性能测试
    run_performance_tests $server_pid $DEFAULT_PORT
    echo ""
    
    # 运行突发流量测试
    run_burst_traffic_test $server_pid $DEFAULT_PORT
    echo ""
    
    # 生成报告
    generate_report
    echo ""
    
    echo "========================================"
    echo "  测试完成"
    echo "========================================"
    echo ""
    echo "结果目录: $RUN_DIR"
    echo "报告文件: $RUN_DIR/report.md"
    echo ""
}

# 运行主函数
main "$@"