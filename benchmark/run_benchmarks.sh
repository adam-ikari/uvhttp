#!/bin/bash
# UVHTTP 基准性能测试自动化脚本
# 
# 这个脚本自动化运行所有基准性能测试，收集结果并生成报告

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
TEST_DURATION=10  # 测试时长（秒）
WRK_THREADS=(2 4 8 16)
WRK_CONNECTIONS=(10 50 200 500)

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
    
    # 检查 ab
    if ! command -v ab &> /dev/null; then
        missing_deps+=("ab")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_warning "缺少以下依赖: ${missing_deps[*]}"
        log_info "某些测试将被跳过"
    else
        log_success "所有依赖已安装"
    fi
}

# 编译基准测试程序
compile_benchmarks() {
    log_info "编译基准测试程序..."
    
    cd "$PROJECT_ROOT"
    
    if [ ! -d "$BUILD_DIR" ]; then
        log_info "创建构建目录..."
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake ..
    fi
    
    cd "$BUILD_DIR"
    make -j$(nproc) benchmark_rps benchmark_latency benchmark_connection benchmark_memory benchmark_comprehensive
    
    log_success "基准测试程序编译完成"
}

# 运行单个基准测试
run_benchmark() {
    local name=$1
    local port=$2
    local output_file=$3
    
    log_info "运行 $name..."
    
    # 启动服务器（后台运行）
    "$BENCHMARK_DIR/$name" > "$output_file.server.log" 2>&1 &
    local server_pid=$!
    
    # 等待服务器启动
    sleep 2
    
    # 检查服务器是否启动成功
    if ! kill -0 $server_pid 2>/dev/null; then
        log_error "$name 启动失败"
        cat "$output_file.server.log"
        return 1
    fi
    
    log_success "$name 已启动 (PID: $server_pid, 端口: $port)"
    
    # 运行性能测试（如果安装了 wrk）
    if command -v wrk &> /dev/null; then
        log_info "使用 wrk 进行性能测试..."
        
        for threads in "${WRK_THREADS[@]}"; do
            for connections in "${WRK_CONNECTIONS[@]}"; do
                local test_name="${name}_${threads}t_${connections}c"
                local wrk_output="$output_file.${test_name}.txt"
                
                log_info "  测试: $threads 线程 / $connections 连接"
                wrk -t$threads -c$connections -d${TEST_DURATION}s "http://127.0.0.1:$port/" > "$wrk_output" 2>&1 || true
                
                # 提取 RPS
                local rps=$(grep "Requests/sec" "$wrk_output" | awk '{print $2}')
                if [ -n "$rps" ]; then
                    echo "$test_name,$rps" >> "$output_file.rps.csv"
                fi
            done
        done
    fi
    
    # 等待测试完成
    sleep 2
    
    # 停止服务器
    log_info "停止 $name..."
    kill $server_pid 2>/dev/null || true
    wait $server_pid 2>/dev/null || true
    
    log_success "$name 测试完成"
}

# 运行内存分配器测试
run_allocator_tests() {
    log_info "运行内存分配器测试..."
    
    "$BENCHMARK_DIR/performance_allocator" > "$RUN_DIR/allocator.txt" 2>&1
    "$BENCHMARK_DIR/performance_allocator_compare" > "$RUN_DIR/allocator_compare.txt" 2>&1
    
    log_success "内存分配器测试完成"
}

# 运行位字段测试
run_bitfield_test() {
    log_info "运行位字段测试..."
    
    "$BENCHMARK_DIR/test_bitfield" > "$RUN_DIR/bitfield.txt" 2>&1
    
    log_success "位字段测试完成"
}

# 生成测试报告
generate_report() {
    log_info "生成测试报告..."
    
    local report_file="$RUN_DIR/summary_report.md"
    
    cat > "$report_file" << EOF
# UVHTTP 基准性能测试报告

## 测试信息

- **测试时间**: $(date)
- **测试时长**: ${TEST_DURATION} 秒
- **测试目录**: $RUN_DIR

## 测试结果

### 1. RPS 测试

EOF
    
    # 提取 RPS 结果
    if [ -f "$RUN_DIR/benchmark_rps.rps.csv" ]; then
        cat >> "$report_file" << EOF
| 测试场景 | RPS |
|---------|-----|
EOF
        tail -n +2 "$RUN_DIR/benchmark_rps.rps.csv" | while IFS=, read -r name rps; do
            echo "| $name | $rps |" >> "$report_file"
        done
        echo "" >> "$report_file"
    fi
    
    # 添加性能评估
    cat >> "$report_file" << EOF
### 2. 性能评估

根据性能基准文档的目标：

- **低并发（2 线程 / 10 连接）**: ≥ 17,000 RPS
- **中等并发（4 线程 / 50 连接）**: ≥ 17,000 RPS
- **高并发（8 线程 / 200 连接）**: ≥ 16,000 RPS
- **平均延迟**: < 15ms
- **错误率**: < 0.1%

### 3. 详细日志

详细的测试日志保存在以下文件：

EOF
    
    # 列出所有日志文件
    for log_file in "$RUN_DIR"/*.log "$RUN_DIR"/*.txt; do
        if [ -f "$log_file" ]; then
            echo "- $(basename "$log_file")" >> "$report_file"
        fi
    done
    
    log_success "测试报告已生成: $report_file"
}

# 主函数
main() {
    echo "========================================"
    echo "  UVHTTP 基准性能测试"
    echo "========================================"
    echo ""
    
    # 检查依赖
    check_dependencies
    echo ""
    
    # 编译基准测试
    compile_benchmarks
    echo ""
    
    # 运行基准测试
    log_info "开始基准性能测试..."
    
    # RPS 测试
    run_benchmark "benchmark_rps" 18081 "$RUN_DIR/benchmark_rps"
    echo ""
    
    # 延迟测试
    run_benchmark "benchmark_latency" 18081 "$RUN_DIR/benchmark_latency"
    echo ""
    
    # 连接测试
    run_benchmark "benchmark_connection" 18082 "$RUN_DIR/benchmark_connection"
    echo ""
    
    # 内存测试
    run_benchmark "benchmark_memory" 18083 "$RUN_DIR/benchmark_memory"
    echo ""
    
    # 综合测试
    run_benchmark "benchmark_comprehensive" 18084 "$RUN_DIR/benchmark_comprehensive"
    echo ""
    
    # 内存分配器测试
    run_allocator_tests
    echo ""
    
    # 位字段测试
    run_bitfield_test
    echo ""
    
    # 生成报告
    generate_report
    echo ""
    
    echo "========================================"
    echo "  测试完成"
    echo "========================================"
    echo ""
    echo "结果目录: $RUN_DIR"
    echo "报告文件: $RUN_DIR/summary_report.md"
}

# 运行主函数
main "$@"