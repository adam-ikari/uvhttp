#!/bin/bash
# UVHTTP 性能测试 Docker 运行脚本
#
# 使用方式:
#   ./benchmark/docker-run.sh                    # 运行所有测试
#   ./benchmark/docker-run.sh --quick           # 快速测试（仅基本测试）
#   ./benchmark/docker-run.sh --full            # 完整测试（包括数据库和文件上传）
#   ./benchmark/docker-run.sh --regression      # 仅回归检测

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 配置
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_NAME="uvhttp-benchmark"
CONTAINER_NAME="uvhttp-benchmark-$$"

# 解析参数
QUICK=false
FULL=false
REGRESSION_ONLY=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --quick)
            QUICK=true
            shift
            ;;
        --full)
            FULL=true
            shift
            ;;
        --regression)
            REGRESSION_ONLY=true
            shift
            ;;
        --help)
            echo "使用方式: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --quick        快速测试（仅基本性能测试）"
            echo "  --full         完整测试（包括数据库和文件上传）"
            echo "  --regression   仅运行回归检测"
            echo "  --help         显示此帮助信息"
            exit 0
            ;;
        *)
            echo "未知选项: $1"
            echo "使用 --help 查看帮助"
            exit 1
            ;;
    esac
done

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

# 检查 Docker
if ! command -v docker &> /dev/null; then
    log_error "Docker 未安装"
    log_info "请安装 Docker: https://docs.docker.com/get-docker/"
    exit 1
fi

# 检查 Docker 是否运行
if ! docker info &> /dev/null; then
    log_error "Docker 未运行"
    log_info "请启动 Docker 服务"
    exit 1
fi

log_info "========================================"
log_info "  UVHTTP 性能测试 (Docker)"
log_info "========================================"
echo ""

# 构建 Docker 镜像（如果不存在）
if ! docker images -q "$IMAGE_NAME" | grep -q .; then
    log_info "构建 Docker 镜像..."
    docker build -t "$IMAGE_NAME" -f "$PROJECT_ROOT/benchmark/Dockerfile" "$PROJECT_ROOT"
    log_success "Docker 镜像构建完成"
    echo ""
fi

# 准备测试命令
TEST_CMD=""

if [ "$REGRESSION_ONLY" = true ]; then
    log_info "运行回归检测..."
    TEST_CMD="
        cd /workspace/benchmark
        if [ -f /tmp/results.csv ]; then
            python3 /scripts/detect_performance_regression.py \\
                /tmp/results.csv \\
                --baseline /workspace/config/performance-baseline.yml \\
                --output /tmp/regression_report.json \\
                --fail-on-regression || EXIT_CODE=\$?
            
            if [ \"\${EXIT_CODE:-0}\" = \"1\" ]; then
                echo ' 检测到性能回归！'
                exit 1
            elif [ \"\${EXIT_CODE:-0}\" = \"2\" ]; then
                echo '  检测到性能警告'
                exit 0
            else
                echo ' 性能检测通过'
            fi
        else
            echo '错误: 没有找到测试结果文件 /tmp/results.csv'
            echo '请先运行性能测试: $0'
            exit 1
        fi
    "
else
    log_info "运行性能测试..."
    
    if [ "$QUICK" = true ]; then
        log_info "模式: 快速测试"
        TEST_CMD="
            cd /workspace/benchmark
            ./run_benchmarks.sh --fast
        "
    elif [ "$FULL" = true ]; then
        log_info "模式: 完整测试"
        TEST_CMD="
            cd /workspace/benchmark
            ./run_benchmarks.sh
            
            # 运行数据库模拟测试
            echo ''
            echo '=== 数据库查询模拟测试 ==='
            ../build/dist/bin/benchmark_database_simulation > /tmp/db_server.log 2>&1 &
            DB_PID=\$!
            sleep 3
            wrk -t4 -c50 -d10s http://127.0.0.1:18085/api/fast
            wrk -t4 -c50 -d10s http://127.0.0.1:18085/api/mixed
            kill \$DB_PID 2>/dev/null || true
            
            # 运行文件上传测试
            echo ''
            echo '=== 文件上传测试 ==='
            ../build/dist/bin/benchmark_file_upload > /tmp/upload_server.log 2>&1 &
            UPLOAD_PID=\$!
            sleep 3
            curl -s http://127.0.0.1:18086/health
            echo 'test data' | curl -X POST -F 'file=@-' http://127.0.0.1:18086/upload
            kill \$UPLOAD_PID 2>/dev/null || true
        "
    else
        log_info "模式: 标准测试"
        TEST_CMD="
            cd /workspace/benchmark
            ./run_benchmarks.sh
        "
    fi
fi

# 运行 Docker 容器
log_info "启动测试容器..."
echo ""

docker run --rm --name "$CONTAINER_NAME" \
    --privileged \
    --cpuset-cpus=0-3 \
    --memory=8g \
    --memory-swap=8g \
    --network=host \
    -v "$PROJECT_ROOT":/workspace \
    -v /tmp:/tmp \
    "$IMAGE_NAME" \
    bash -c "
        set -e
        
        echo '========================================'
        echo '  测试环境配置'
        echo '========================================'
        echo ''
        
        # 禁用 Swap
        echo '禁用 Swap...'
        swapoff -a 2>/dev/null || echo '  Swap 已禁用或无法禁用'
        
        # 锁定 CPU 频率
        echo '锁定 CPU 频率...'
        cpupower frequency-set -g performance 2>/dev/null || echo '  无法设置 CPU 频率'
        
        echo ''
        echo '=== 环境验证 ==='
        echo 'CPU Governor:'
        cpupower frequency-info 2>/dev/null | grep 'governor' || echo '  (无法检测)'
        echo 'Swap Status:'
        free -h | grep Swap || echo '  (无法检测)'
        echo 'File Descriptors:'
        ulimit -n
        echo 'Available Memory:'
        free -h | grep Mem
        echo ''
        
        # 编译项目
        echo '=== 编译项目 ==='
        mkdir -p /workspace/build
        cd /workspace/build
        cmake -DCMAKE_BUILD_TYPE=Release .. > /dev/null 2>&1
        make -j\$(nproc) > /dev/null 2>&1
        echo '编译完成'
        echo ''
        
        # 运行测试
        echo '========================================'
        echo '  运行测试'
        echo '========================================'
        echo ''
        
        $TEST_CMD
        
        echo ''
        echo '========================================'
        echo '  测试完成'
        echo '========================================'
    "

EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ]; then
    log_success "所有测试通过！"
    echo ""
    log_info "结果文件:"
    log_info "  /tmp/wrk_*.txt - wrk 测试结果"
    log_info "  /tmp/results.csv - CSV 格式结果"
    log_info "  /tmp/regression_report.json - 回归检测报告"
else
    log_error "测试失败，退出码: $EXIT_CODE"
fi

exit $EXIT_CODE