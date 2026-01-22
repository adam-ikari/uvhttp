#!/bin/bash
# UVHTTP 性能回归检查脚本
# 此脚本检查性能测试结果是否低于基准值

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 加载配置
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/performance_test_config.sh"

# 检查结果目录
if [ ! -d "$RESULTS_DIR" ]; then
    echo -e "${RED}错误: 结果目录不存在: $RESULTS_DIR${NC}"
    echo "请先运行性能测试: $SCRIPT_DIR/run_standard_performance_test.sh"
    exit 1
fi

# 获取最新的测试结果
LATEST_RESULT=$(ls -t "$RESULTS_DIR" | head -1)

if [ -z "$LATEST_RESULT" ]; then
    echo -e "${RED}错误: 未找到测试结果${NC}"
    exit 1
fi

RESULTS_PATH="$RESULTS_DIR/$LATEST_RESULT"

echo "=========================================="
echo "UVHTTP 性能回归检查"
echo "=========================================="
echo ""
echo "检查测试结果: $RESULTS_PATH"
echo ""

# 初始化统计
TOTAL_TESTS=0
PASSED_TESTS=0
WARNING_TESTS=0
FAILED_TESTS=0

# 检查单个测试结果
check_test_result() {
    local test_name=$1
    local rps_min_key="${test_name}_rps_min"
    local rps_target_key="${test_name}_rps_target"
    local avg_latency_max_key="${test_name}_avg_latency_max"
    local p99_latency_max_key="${test_name}_p99_latency_max"

    local json_file="$RESULTS_PATH/${test_name}.json"

    if [ ! -f "$json_file" ]; then
        echo -e "${YELLOW}跳过: $test_name (结果文件不存在)${NC}"
        return
    fi

    TOTAL_TESTS=$((TOTAL_TESTS + 1))

    # 提取结果
    local rps=$(grep -o '"rps": [0-9.]*' "$json_file" | awk '{print $2}')
    local avg_latency=$(grep -o '"avg_latency": "[0-9.]*[a-z]*"' "$json_file" | sed 's/"avg_latency": "//; s/"//')
    local p99_latency=$(grep -o '"p99_latency": "[0-9.]*[a-z]*"' "$json_file" | sed 's/"p99_latency": "//; s/"//')

    # 转换延迟为毫秒
    local avg_latency_ms=$(echo "$avg_latency" | sed 's/ms//; s/us/0.001/')
    local p99_latency_ms=$(echo "$p99_latency" | sed 's/ms//; s/us/0.001/')

    # 获取基准值
    local rps_min=${PERFORMANCE_BASELINES[$rps_min_key]}
    local rps_target=${PERFORMANCE_BASELINES[$rps_target_key]}
    local avg_latency_max=${PERFORMANCE_BASELINES[$avg_latency_max_key]}
    local p99_latency_max=${PERFORMANCE_BASELINES[$p99_latency_max_key]}

    # 检查 RPS
    local rps_status="✅ 通过"
    local rps_color="$GREEN"
    if (( $(echo "$rps < $rps_min" | bc -l) )); then
        rps_status="❌ 失败"
        rps_color="$RED"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    elif (( $(echo "$rps < $rps_target" | bc -l) )); then
        rps_status="⚠️  警告"
        rps_color="$YELLOW"
        WARNING_TESTS=$((WARNING_TESTS + 1))
    else
        PASSED_TESTS=$((PASSED_TESTS + 1))
    fi

    # 检查延迟
    local latency_status="✅ 通过"
    local latency_color="$GREEN"
    if (( $(echo "$avg_latency_ms > $avg_latency_max" | bc -l) )); then
        latency_status="❌ 失败"
        latency_color="$RED"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    elif (( $(echo "$p99_latency_ms > $p99_latency_max" | bc -l) )); then
        latency_status="⚠️  警告"
        latency_color="$YELLOW"
        WARNING_TESTS=$((WARNING_TESTS + 1))
    fi

    # 打印结果
    local file_desc="${TEST_FILE_DESCRIPTIONS[$test_name]}"
    echo "## $file_desc"
    echo ""
    echo "  RPS: $rps (目标: $rps_target, 最小: $rps_min) - ${rps_color}${rps_status}${NC}"
    echo "  平均延迟: $avg_latency_ms ms (最大: ${avg_latency_max}ms)"
    echo "  P99 延迟: $p99_latency_ms ms (最大: ${p99_latency_max}ms) - ${latency_color}${latency_status}${NC}"
    echo ""
}

# 检查所有测试
check_test_result "homepage"
check_test_result "small_file"
check_test_result "medium_file"
check_test_result "large_file"

# 生成总结
echo "=========================================="
echo "性能回归检查总结"
echo "=========================================="
echo ""
echo -e "总测试数: $TOTAL_TESTS"
echo -e "${GREEN}通过: $PASSED_TESTS${NC}"
echo -e "${YELLOW}警告: $WARNING_TESTS${NC}"
echo -e "${RED}失败: $FAILED_TESTS${NC}"
echo ""

# 判断整体结果
if [ $FAILED_TESTS -gt 0 ]; then
    echo -e "${RED}❌ 性能回归检测失败！${NC}"
    echo "发现 $FAILED_TESTS 个性能问题，请检查代码变更。"
    exit 1
elif [ $WARNING_TESTS -gt 0 ]; then
    echo -e "${YELLOW}⚠️  性能警告：发现 $WARNING_TESTS 个潜在问题${NC}"
    exit 0
else
    echo -e "${GREEN}✅ 所有性能检查通过！${NC}"
    exit 0
fi