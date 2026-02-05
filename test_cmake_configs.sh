#!/bin/bash
# CMake 配置宏组合验证脚本
# 测试不同的编译选项组合是否能成功配置

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 统计
TOTAL=0
PASSED=0
FAILED=0

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# 测试配置
test_config() {
    local name=$1
    shift
    local options=("$@")
    
    TOTAL=$((TOTAL + 1))
    
    echo ""
    echo "========================================"
    echo "  测试配置: $name"
    echo "========================================"
    echo "选项: ${options[@]}"
    echo ""
    
    local build_dir="build_test_${TOTAL}"
    
    # 返回项目根目录并清理旧目录
    cd /home/zhaodi-chen/project/uvhttp
    rm -rf "$build_dir" 2>/dev/null || true
    
    # 创建构建目录
    mkdir -p "$build_dir"
    cd "$build_dir"
    
    # 运行 CMake
    if cmake .. "${options[@]}" > cmake.log 2>&1; then
        log_success "✓ 配置成功: $name"
        PASSED=$((PASSED + 1))
        return 0
    else
        log_error "✗ 配置失败: $name"
        echo "错误日志:"
        cat cmake.log | tail -20
        FAILED=$((FAILED + 1))
        return 1
    fi
}

# 返回项目根目录
cd /home/zhaodi-chen/project/uvhttp

echo "========================================"
echo "  CMake 配置宏组合验证"
echo "========================================"
echo ""

# 测试 1: 最小配置
test_config "最小配置（无任何可选功能）" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=OFF \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=OFF \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 2: 完整功能
test_config "完整功能（所有可选功能）" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=ON \
    -DBUILD_WITH_TLS=ON \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 3: Debug 模式
test_config "Debug 模式" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=OFF \
    -DENABLE_DEBUG=ON \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 4: 覆盖率模式
test_config "覆盖率模式" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=OFF \
    -DENABLE_DEBUG=ON \
    -DENABLE_COVERAGE=ON \
    -DBUILD_EXAMPLES=OFF

# 测试 5: 系统分配器
test_config "系统分配器" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=ON \
    -DUVHTTP_ALLOCATOR_TYPE=0 \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 6: Mimalloc 分配器
test_config "Mimalloc 分配器" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=ON \
    -DBUILD_WITH_TLS=ON \
    -DUVHTTP_ALLOCATOR_TYPE=1 \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 7: 仅 WebSocket
test_config "仅 WebSocket" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=OFF \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 8: 仅 TLS
test_config "仅 TLS" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=OFF \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=ON \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 9: WebSocket + TLS
test_config "WebSocket + TLS" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=ON \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 10: 示例程序
test_config "示例程序" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=OFF \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=ON

# 测试 11: 性能优化（-O3）
test_config "性能优化（-O3）" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=ON \
    -DBUILD_WITH_TLS=OFF \
    -DCMAKE_C_FLAGS="-O3" \
    -DCMAKE_CXX_FLAGS="-O3" \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 测试 12: 32位兼容性检查
test_config "32位兼容性检查" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=OFF \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DCMAKE_C_FLAGS=-m32 \
    -DCMAKE_CXX_FLAGS=-m32

# 测试 13: 静态分析
test_config "静态分析准备" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DBUILD_WITH_WEBSOCKET=OFF \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=OFF \
    -DENABLE_DEBUG=ON \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DCMAKE_C_FLAGS="-Wall -Wextra -Wpedantic" \
    -DCMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic"

# 测试 14: 最小化构建
test_config "最小化构建" \
    -DCMAKE_BUILD_TYPE=MinSizeRel \
    -DBUILD_WITH_WEBSOCKET=OFF \
    -DBUILD_WITH_MIMALLOC=OFF \
    -DBUILD_WITH_TLS=OFF \
    -DENABLE_DEBUG=OFF \
    -DENABLE_COVERAGE=OFF \
    -DBUILD_EXAMPLES=OFF

# 返回项目根目录
cd /home/zhaodi-chen/project/uvhttp

# 清理测试构建目录
echo ""
echo "========================================"
echo "  清理测试构建目录"
echo "========================================"
rm -rf build_test_*

# 输出总结
echo ""
echo "========================================"
echo "  测试结果总结"
echo "========================================"
echo "总测试数: $TOTAL"
echo "通过: $PASSED"
echo "失败: $FAILED"
echo ""

if [ $FAILED -eq 0 ]; then
    log_success "✓ 所有配置测试通过！"
    exit 0
else
    log_error "✗ 有 $FAILED 个配置测试失败"
    exit 1
fi