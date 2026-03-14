#!/bin/bash
# UVHTTP 大文件压缩传输性能测试脚本
# 
# 测试不同大小文件的压缩传输性能，包括：
# - 无压缩传输性能
# - gzip 压缩传输性能
# - 压缩率分析
# - 吞吐量对比

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# 配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/build"
BENCHMARK_DIR="$BUILD_DIR/dist/bin"
RESULTS_DIR="$SCRIPT_DIR/results/compression_large_files"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
RUN_DIR="$RESULTS_DIR/run_$TIMESTAMP"

# 测试配置
DEFAULT_PORT=18082
TEST_DURATION=30
WARMUP_DURATION=5

# 文件大小配置（字节）
FILE_SIZES=(
    "102400:100KB"
    "524288:512KB"
    "1048576:1MB"
    "5242880:5MB"
    "10485760:10MB"
)

# 并发级别
CONCURRENCY_LEVELS=(
    "4:50:中等并发"
    "8:200:高并发"
    "16:500:极高并发"
)

# 创建结果目录
mkdir -p "$RUN_DIR"

# 日志函数
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }
log_header() { echo -e "${CYAN}[TEST]${NC} $1"; }

# 检查依赖
check_dependencies() {
    log_info "检查依赖..."
    
    local missing_deps=()
    
    if ! command -v wrk &> /dev/null; then
        missing_deps+=("wrk")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_error "缺少必要的依赖: ${missing_deps[*]}"
        log_info "请安装 wrk: sudo apt-get install wrk"
        exit 1
    fi
    
    log_success "所有依赖已安装"
}

# 创建测试文件
create_test_files() {
    log_info "创建测试文件..."
    
    local test_dir="$PROJECT_ROOT/public/compression_test"
    mkdir -p "$test_dir"
    
    for size_info in "${FILE_SIZES[@]}"; do
        IFS=':' read -r size name <<< "$size_info"
        local file="$test_dir/test_${name}.txt"
        
        if [ ! -f "$file" ]; then
            log_info "  创建 $name 文件 ($size 字节)..."
            
            # 创建高度可压缩的文本文件（重复模式）
            local pattern="HTTP compression testing. This is a highly compressible text pattern. "
            local pattern_len=${#pattern}
            local remaining=$size
            local count=0
            
            {
                while [ $remaining -gt 0 ]; do
                    if [ $remaining -lt $pattern_len ]; then
                        echo -n "${pattern:0:$remaining}"
                    else
                        echo -n "$pattern"
                    fi
                    remaining=$((remaining - pattern_len))
                    count=$((count + 1))
                done
                echo ""
            } > "$file"
            
            log_success "  已创建: $file (${count} 个模式)"
        else
            log_info "  文件已存在: $file"
        fi
    done
    
    log_success "测试文件创建完成"
}

# 编译 benchmark 服务器
compile_benchmark_server() {
    log_info "编译 benchmark 服务器..."
    
    cd "$PROJECT_ROOT"
    
    if [ ! -d "$BUILD_DIR" ]; then
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_COMPRESSION=ON ..
    fi
    
    cd "$BUILD_DIR"
    make -j$(nproc) benchmark_unified
    
    if [ ! -f "$BENCHMARK_DIR/benchmark_unified" ]; then
        log_error "benchmark_unified 编译失败"
        exit 1
    fi
    
    log_success "benchmark 服务器编译完成"
}

# 启动服务器
start_server() {
    local port=$1
    
    log_info "启动测试服务器 (端口: $port)..."
    
    "$BENCHMARK_DIR/benchmark_unified" $port > "$RUN_DIR/server.log" 2>&1 &
    local server_pid=$!
    
    sleep 3
    
    if ! kill -0 $server_pid 2>/dev/null; then
        log_error "服务器启动失败"
        cat "$RUN_DIR/server.log"
        exit 1
    fi
    
    if ! curl -s "http://127.0.0.1:$port/health" > /dev/null 2>&1; then
        log_error "服务器无响应"
        kill $server_pid 2>/dev/null || true
        exit 1
    fi
    
    log_success "服务器已启动 (PID: $server_pid)"
    echo $server_pid
}

# 停止服务器
stop_server() {
    local server_pid=$1
    log_info "停止测试服务器..."
    kill $server_pid 2>/dev/null || true
    wait $server_pid 2>/dev/null || true
    log_success "服务器已停止"
}

# 获取文件信息（原始大小和压缩后大小）
get_file_info() {
    local url=$1
    local port=$2
    
    log_info "  获取文件信息..."
    
    # 获取未压缩的响应
    local original_size=$(curl -s -o /dev/null -w "%{size_download}" "http://127.0.0.1:$port$url")
    local original_time=$(curl -s -o /dev/null -w "%{time_total}" "http://127.0.0.1:$port$url")
    
    # 获取压缩的响应
    local compressed_size=$(curl -s -H "Accept-Encoding: gzip" -o /dev/null -w "%{size_download}" "http://127.0.0.1:$port$url")
    local compressed_time=$(curl -s -H "Accept-Encoding: gzip" -o /dev/null -w "%{time_total}" "http://127.0.0.1:$port$url")
    
    # 计算压缩率
    local compression_ratio=$(echo "scale=2; (1 - $compressed_size / $original_size) * 100" | bc 2>/dev/null || echo "0")
    
    echo "$original_size,$compressed_size,$compression_ratio,$original_time,$compressed_time"
}

# 运行性能测试
run_performance_test() {
    local endpoint=$1
    local description=$2
    original_size=$3
    compressed_size=$4
    local threads=$5
    local connections=$6
    local duration=$7
    local port=$8
    local use_compression=$9
    local output_file="${10}"
    
    local url="http://127.0.0.1:$port$endpoint"
    
    log_info "  测试: $description ($threads 线程 / $connections 连接 / ${duration}s)"
    
    if [ "$use_compression" = "1" ]; then
        wrk -t$threads -c$connections -d${duration}s -H "Accept-Encoding: gzip" "$url" > "$output_file" 2>&1
    else
        wrk -t$threads -c$connections -d${duration}s "$url" > "$output_file" 2>&1
    fi
    
    if [ $? -ne 0 ]; then
        log_warning "  测试失败"
        return 1
    fi
}

# 提取测试结果
extract_results() {
    local output_file=$1
    
    local rps=$(grep "Requests/sec" "$output_file" | awk '{print $2}')
    local avg_latency=$(grep "Latency" "$output_file" | awk '{print $2}')
    local stdev=$(grep "Stdev" "$output_file" | awk '{print $2}')
    local transfer=$(grep "Transfer/sec" "$output_file" | awk '{print $2}')
    
    echo "$rps,$avg_latency,$stdev,$transfer"
}

# 运行完整的压缩性能测试
run_compression_tests() {
    local server_pid=$1
    local port=$2
    
    log_header "开始压缩性能测试..."
    
    # 预热
    log_info "预热服务器 (${WARMUP_DURATION} 秒)..."
    wrk -t4 -c50 -d${WARMUP_DURATION}s -H "Accept-Encoding: gzip" "http://127.0.0.1:$port/compression/text" > /dev/null 2>&1 || true
    sleep 1
    
    # 创建 CSV 结果文件
    local csv_file="$RUN_DIR/results.csv"
    echo "FileSize,Compression,Threads,Connections,RPS,AvgLatency,Stdev,Transfer" > "$csv_file"
    
    # 创建汇总文件
    local summary_file="$RUN_DIR/summary.csv"
    echo "FileSize,OriginalSize,CompressedSize,CompressionRatio,OriginalTime,CompressedTime" > "$summary_file"
    
    # 对每个文件大小进行测试
    for size_info in "${FILE_SIZES[@]}"; do
        IFS=':' read -r size name <<< "$size_info"
        local endpoint="/file/compression_test/test_${name}.txt"
        
        log_header "测试文件: $name ($size 字节)"
        
        # 获取文件信息
        local file_info=$(get_file_info "$endpoint" "$port")
        IFS=',' read -r original_size compressed_size compression_ratio original_time compressed_time <<< "$file_info"
        
        log_info "  原始大小: $original_size 字节"
        log_info "  压缩大小: $compressed_size 字节"
        log_info "  压缩率: ${compression_ratio}%"
        log_info "  原始传输时间: ${original_time}s"
        log_info "  压缩传输时间: ${compressed_time}s"
        
        # 保存到汇总文件
        echo "$name,$original_size,$compressed_size,$compression_ratio,$original_time,$compressed_time" >> "$summary_file"
        
        # 对每个并发级别进行测试
        for level_info in "${CONCURRENCY_LEVELS[@]}"; do
            IFS=':' read -r threads connections desc <<< "$level_info"
            
            local test_dir="$RUN_DIR/${name}/${desc}"
            mkdir -p "$test_dir"
            
            # 测试无压缩
            log_info "  测试无压缩传输..."
            local no_compress_output="$test_dir/no_compress.txt"
            run_performance_test "$endpoint" "无压缩" "$original_size" "$compressed_size" \
                "$threads" "$connections" "$TEST_DURATION" "$port" "0" "$no_compress_output"
            
            local no_compress_results=$(extract_results "$no_compress_output")
            IFS=',' read -r rps_no_compress avg_no_compress stdev_no_compress transfer_no_compress <<< "$no_compress_results"
            
            if [ -n "$rps_no_compress" ]; then
                echo "$name,无压缩,$threads,$connections,$rps_no_compress,$avg_no_compress,$stdev_no_compress,$transfer_no_compress" >> "$csv_file"
                log_success "    RPS: $rps_no_compress, 延迟: $avg_no_compress"
            fi
            
            # 测试压缩
            log_info "  测试压缩传输..."
            local compress_output="$test_dir/compress.txt"
            run_performance_test "$endpoint" "压缩" "$original_size" "$compressed_size" \
                "$threads" "$connections" "$TEST_DURATION" "$port" "1" "$compress_output"
            
            local compress_results=$(extract_results "$compress_output")
            IFS=',' read -r rps_compress avg_compress stdev_compress transfer_compress <<< "$compress_results"
            
            if [ -n "$rps_compress" ]; then
                echo "$name,压缩,$threads,$connections,$rps_compress,$avg_compress,$stdev_compress,$transfer_compress" >> "$csv_file"
                log_success "    RPS: $rps_compress, 延迟: $avg_compress"
            fi
            
            # 计算性能提升
            if [ -n "$rps_no_compress" ] && [ -n "$rps_compress" ]; then
                local rps_improvement=$(echo "scale=2; ($rps_compress - $rps_no_compress) / $rps_no_compress * 100" | bc 2>/dev/null || echo "0")
                log_info "    RPS 提升: ${rps_improvement}%"
            fi
        done
        
        echo ""
    done
    
    log_success "压缩性能测试完成"
}

# 生成测试报告
generate_report() {
    log_info "生成测试报告..."
    
    local report_file="$RUN_DIR/report.md"
    
    cat > "$report_file" << EOF
# UVHTTP 大文件压缩传输性能测试报告

## 测试信息

- **测试时间**: $(date)
- **测试时长**: ${TEST_DURATION} 秒
- **预热时长**: ${WARMUP_DURATION} 秒
- **服务器端口**: $DEFAULT_PORT
- **结果目录**: $RUN_DIR

## 测试文件

| 文件大小 | 原始大小 | 压缩大小 | 压缩率 |
|---------|---------|---------|--------|
EOF
    
    # 添加文件信息
    tail -n +2 "$RUN_DIR/summary.csv" | while IFS=',' read -r name original_size compressed_size compression_ratio original_time compressed_time; do
        echo "| $name | $original_size | $compressed_size | ${compression_ratio}% |" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF

## 并发级别

| 级别 | 线程数 | 连接数 |
|------|--------|--------|
EOF
    
    for level_info in "${CONCURRENCY_LEVELS[@]}"; do
        IFS=':' read -r threads connections desc <<< "$level_info"
        echo "| $desc | $threads | $connections |" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF

## 性能对比

### 无压缩 vs 压缩传输性能

| 文件大小 | 并发级别 | 无压缩 RPS | 压缩 RPS | RPS 提升 | 无压缩延迟 | 压缩延迟 |
|---------|---------|-----------|---------|---------|-----------|---------|
EOF
    
    # 分析并添加性能对比
    for size_info in "${FILE_SIZES[@]}"; do
        IFS=':' read -r size name <<< "$size_info"
        
        for level_info in "${CONCURRENCY_LEVELS[@]}"; do
            IFS=':' read -r threads connections desc <<< "$level_info"
            
            local rps_no_compress=$(grep "^$name,无压缩,$threads,$connections," "$RUN_DIR/results.csv" | cut -d',' -f5)
            local rps_compress=$(grep "^$name,压缩,$threads,$connections," "$RUN_DIR/results.csv" | cut -d',' -f5)
            local avg_no_compress=$(grep "^$name,无压缩,$threads,$connections," "$RUN_DIR/results.csv" | cut -d',' -f6)
            local avg_compress=$(grep "^$name,压缩,$threads,$connections," "$RUN_DIR/results.csv" | cut -d',' -f6)
            
            if [ -n "$rps_no_compress" ] && [ -n "$rps_compress" ]; then
                local improvement=$(echo "scale=2; ($rps_compress - $rps_no_compress) / $rps_no_compress * 100" | bc 2>/dev/null || echo "0")
                echo "| $name | $desc | $rps_no_compress | $rps_compress | ${improvement}% | $avg_no_compress | $avg_compress |" >> "$report_file"
            fi
        done
    done
    
    cat >> "$report_file" << EOF

## 结论

### 压缩效果分析

EOF
    
    # 分析压缩效果
    tail -n +2 "$RUN_DIR/summary.csv" | while IFS=',' read -r name original_size compressed_size compression_ratio original_time compressed_time; do
        echo "#### $name" >> "$report_file"
        echo "- 原始大小: $original_size 字节" >> "$report_file"
        echo "- 压缩大小: $compressed_size 字节" >> "$report_file"
        echo "- 压缩率: ${compression_ratio}%" >> "$report_file"
        echo "- 传输时间减少: $(echo "scale=2; (1 - $compressed_time / $original_time) * 100" | bc 2>/dev/null || echo "0")%" >> "$report_file"
        echo "" >> "$report_file"
    done
    
    cat >> "$report_file" << EOF

### 性能建议

EOF
    
    # 生成性能建议
    echo "1. **小文件 (< 100KB)**: 压缩开销可能大于收益，建议根据实际情况选择" >> "$report_file"
    echo "2. **中等文件 (100KB - 1MB)**: 压缩能显著减少传输量，推荐使用" >> "$report_file"
    echo "3. **大文件 (> 1MB)**: 压缩能大幅减少带宽使用，但会增加 CPU 开销，需要权衡" >> "$report_file"
    echo "4. **高并发场景**: 压缩能提高整体吞吐量，但需要考虑 CPU 资源" >> "$report_file"
    echo "" >> "$report_file"
    
    cat >> "$report_file" << EOF

## 使用方法

### 运行测试

\`\`\`bash
cd benchmark
./test_compression_large_files.sh
\`\`\`

### 查看结果

\`\`\`bash
# 查看报告
cat results/compression_large_files/run_$TIMESTAMP/report.md

# 查看详细数据
cat results/compression_large_files/run_$TIMESTAMP/results.csv
cat results/compression_large_files/run_$TIMESTAMP/summary.csv
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
    echo "  UVHTTP 大文件压缩传输性能测试"
    echo "========================================"
    echo ""
    
    # 检查依赖
    check_dependencies
    echo ""
    
    # 创建测试文件
    create_test_files
    echo ""
    
    # 编译 benchmark 服务器
    compile_benchmark_server
    echo ""
    
    # 启动服务器
    server_pid=$(start_server $DEFAULT_PORT)
    echo ""
    
    # 运行压缩性能测试
    run_compression_tests $server_pid $DEFAULT_PORT
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
    echo "快速查看结果:"
    echo "  cat $RUN_DIR/summary.csv"
    echo "  cat $RUN_DIR/report.md"
    echo ""
}

# 运行主函数
main "$@"