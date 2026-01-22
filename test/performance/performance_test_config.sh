#!/bin/bash
# UVHTTP 性能测试配置
# 此文件定义了所有性能测试的标准配置

# ==================== 测试环境配置 ====================

# 项目路径
PROJECT_ROOT="${PROJECT_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)}"
BUILD_DIR="${BUILD_DIR:-$PROJECT_ROOT/build}"
SERVER_BIN="${SERVER_BIN:-$BUILD_DIR/dist/bin/performance_static_server}"

# 服务器配置
UVHTTP_PORT="${UVHTTP_PORT:-8080}"
PUBLIC_DIR="${PUBLIC_DIR:-$PROJECT_ROOT/public}"
SERVER_LOG="${SERVER_LOG:-/tmp/uvhttp_server.log}"
SERVER_PID="${SERVER_PID:-/tmp/uvhttp_server.pid}"

# 结果目录
RESULTS_DIR="${RESULTS_DIR:-$PROJECT_ROOT/test/performance/results}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULTS_SUBDIR="${RESULTS_DIR}/run_${TIMESTAMP}"

# ==================== wrk 测试配置 ====================

# 标准测试配置（用于基准测试）
STANDARD_THREADS=4
STANDARD_CONNECTIONS=100
STANDARD_DURATION=30

# 测试场景配置
declare -A TEST_SCENARIOS

# 场景 1: 低并发（基准测试）
TEST_SCENARIOS[low_conc_threads]=2
TEST_SCENARIOS[low_conc_connections]=10
TEST_SCENARIOS[low_conc_duration]=30

# 场景 2: 中等并发（推荐配置）
TEST_SCENARIOS[medium_conc_threads]=4
TEST_SCENARIOS[medium_conc_connections]=50
TEST_SCENARIOS[medium_conc_duration]=30

# 场景 3: 高并发（压力测试）
TEST_SCENARIOS[high_conc_threads]=4
TEST_SCENARIOS[high_conc_connections]=100
TEST_SCENARIOS[high_conc_duration]=30

# 场景 4: 超高并发（极限测试）
TEST_SCENARIOS[ultra_conc_threads]=8
TEST_SCENARIOS[ultra_conc_connections]=200
TEST_SCENARIOS[ultra_conc_duration]=30

# ==================== 性能基准值 ====================

# 这些值是经过实际测试验证的基准值
# 用于性能回归检测

# 主页性能基准（中等并发场景）
declare -A PERFORMANCE_BASELINES

PERFORMANCE_BASELINES[homepage_rps_min]=8000
PERFORMANCE_BASELINES[homepage_rps_target]=9500
PERFORMANCE_BASELINES[homepage_avg_latency_max]=15
PERFORMANCE_BASELINES[homepage_p99_latency_max]=30

# 小文件性能基准（中等并发场景）
PERFORMANCE_BASELINES[small_file_rps_min]=6000
PERFORMANCE_BASELINES[small_file_rps_target]=7000
PERFORMANCE_BASELINES[small_file_avg_latency_max]=20
PERFORMANCE_BASELINES[small_file_p99_latency_max]=50

# 中等文件性能基准（中等并发场景）
PERFORMANCE_BASELINES[medium_file_rps_min]=4000
PERFORMANCE_BASELINES[medium_file_rps_target]=4500
PERFORMANCE_BASELINES[medium_file_avg_latency_max]=50
PERFORMANCE_BASELINES[medium_file_p99_latency_max]=100

# 大文件性能基准（中等并发场景）
PERFORMANCE_BASELINES[large_file_rps_min]=4000
PERFORMANCE_BASELINES[large_file_rps_target]=4600
PERFORMANCE_BASELINES[large_file_avg_latency_max]=30
PERFORMANCE_BASELINES[large_file_p99_latency_max]=60

# ==================== 性能回归阈值 ====================

# 性能下降超过这些阈值将被视为回归

# 吞吐量回归阈值（百分比）
RPS_REGRESSION_WARNING=5      # 下降 5% 发出警告
RPS_REGRESSION_FAILURE=10     # 下降 10% 视为失败

# 延迟回归阈值（百分比）
LATENCY_REGRESSION_WARNING=10 # 增加 10% 发出警告
LATENCY_REGRESSION_FAILURE=20 # 增加 20% 视为失败

# ==================== 测试文件配置 ====================

# 测试文件路径
declare -A TEST_FILES

TEST_FILES[homepage]="/"
TEST_FILES[small_file]="/static/index.html"
TEST_FILES[medium_file]="/static/medium.html"
TEST_FILES[large_file]="/static/large.html"

# 测试文件描述
declare -A TEST_FILE_DESCRIPTIONS

TEST_FILE_DESCRIPTIONS[homepage]="主页 (HTML)"
TEST_FILE_DESCRIPTIONS[small_file]="小文件 (12B)"
TEST_FILE_DESCRIPTIONS[medium_file]="中等文件 (10KB)"
TEST_FILE_DESCRIPTIONS[large_file]="大文件 (100KB)"

# ==================== 预热配置 ====================

# 预热测试配置
WARMUP_THREADS=2
WARMUP_CONNECTIONS=10
WARMUP_DURATION=5

# ==================== 工具配置 ====================

# 检测 wrk 是否可用
check_wrk() {
    if ! command -v wrk &> /dev/null; then
        echo "错误: wrk 未安装"
        echo "请安装 wrk: sudo apt-get install wrk"
        return 1
    fi
    return 0
}

# ==================== 辅助函数 ====================

# 打印配置
print_config() {
    echo "=========================================="
    echo "UVHTTP 性能测试配置"
    echo "=========================================="
    echo ""
    echo "项目路径: $PROJECT_ROOT"
    echo "构建目录: $BUILD_DIR"
    echo "服务器: $SERVER_BIN"
    echo "端口: $UVHTTP_PORT"
    echo "公共目录: $PUBLIC_DIR"
    echo "结果目录: $RESULTS_SUBDIR"
    echo ""
    echo "标准测试配置:"
    echo "  线程数: $STANDARD_THREADS"
    echo "  连接数: $STANDARD_CONNECTIONS"
    echo "  测试时长: $STANDARD_DURATION 秒"
    echo ""
    echo "性能基准值（中等并发场景）:"
    echo "  主页 RPS: ${PERFORMANCE_BASELINES[homepage_rps_target]} (最小: ${PERFORMANCE_BASELINES[homepage_rps_min]})"
    echo "  小文件 RPS: ${PERFORMANCE_BASELINES[small_file_rps_target]} (最小: ${PERFORMANCE_BASELINES[small_file_rps_min]})"
    echo "  中等文件 RPS: ${PERFORMANCE_BASELINES[medium_file_rps_target]} (最小: ${PERFORMANCE_BASELINES[medium_file_rps_min]})"
    echo "  大文件 RPS: ${PERFORMANCE_BASELINES[large_file_rps_target]} (最小: ${PERFORMANCE_BASELINES[large_file_rps_min]})"
    echo ""
    echo "性能回归阈值:"
    echo "  RPS 警告: ${RPS_REGRESSION_WARNING}%, 失败: ${RPS_REGRESSION_FAILURE}%"
    echo "  延迟警告: ${LATENCY_REGRESSION_WARNING}%, 失败: ${LATENCY_REGRESSION_FAILURE}%"
    echo ""
}

# 导出所有配置
export PROJECT_ROOT BUILD_DIR SERVER_BIN
export UVHTTP_PORT PUBLIC_DIR SERVER_LOG SERVER_PID
export RESULTS_DIR RESULTS_SUBDIR TIMESTAMP
export STANDARD_THREADS STANDARD_CONNECTIONS STANDARD_DURATION
export WARMUP_THREADS WARMUP_CONNECTIONS WARMUP_DURATION
export RPS_REGRESSION_WARNING RPS_REGRESSION_FAILURE
export LATENCY_REGRESSION_WARNING LATENCY_REGRESSION_FAILURE