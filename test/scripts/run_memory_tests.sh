#!/bin/bash
# 内存泄漏测试脚本
# 使用 Valgrind 和 AddressSanitizer 检测内存泄漏

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "========================================"
echo "  UVHTTP 内存泄漏测试"
echo "========================================"
echo ""

# 检查依赖
if ! command -v valgrind &> /dev/null; then
    echo -e "${RED}错误: valgrind 未安装${NC}"
    echo "请运行: sudo apt-get install valgrind"
    exit 1
fi

# 构建目录
BUILD_DIR="build-memtest"

echo "1. 清理旧的构建..."
rm -rf $BUILD_DIR

echo "2. 使用 AddressSanitizer 编译项目..."
cmake -B $BUILD_DIR \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g"

cmake --build $BUILD_DIR --config Debug -j$(nproc)

echo ""
echo "3. 运行测试（使用 AddressSanitizer）..."
cd $BUILD_DIR
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=0

# 运行所有测试
echo "运行所有测试..."
ctest --output-on-failure -j1 --timeout 120 || {
    echo -e "${RED}测试失败${NC}"
    exit 1
}

echo -e "${GREEN}✅ AddressSanitizer 测试通过${NC}"
echo ""

cd ..

echo "4. 使用 Valgrind 编译项目（用于更详细的泄漏检测）..."
BUILD_VG="build-valgrind"
rm -rf $BUILD_VG

cmake -B $BUILD_VG \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build $BUILD_VG --config Debug -j$(nproc)

echo ""
echo "5. 运行关键测试（使用 Valgrind）..."
cd $BUILD_VG

# 关键测试列表
TESTS=(
    "test_connection_full_coverage"
    "test_server_full_coverage"
    "test_request_full_coverage"
    "test_response_full_coverage"
    "test_config_full_coverage"
    "test_static_full_coverage"
    "test_router_full_coverage"
)

LEAKS_FOUND=0
TOTAL_LEAKS=0

for test in "${TESTS[@]}"; do
    if [ -f "./dist/bin/$test" ]; then
        echo -n "  检测 $test... "
        
        # 运行 Valgrind
        valgrind --leak-check=full \
                  --error-exitcode=0 \
                  --log-file=valgrind_${test}.log \
                  --show-leak-kinds=all \
                  --track-origins=yes \
                  ./dist/bin/$test > /dev/null 2>&1 || true
        
        # 检查内存泄漏
        leaks=$(grep "definitely lost:" valgrind_${test}.log | awk '{print $4}')
        
        if [ -n "$leaks" ] && [ "$leaks" != "0" ]; then
            echo -e "${RED}❌ 泄漏: ${leaks} 字节${NC}"
            cat valgrind_${test}.log
            LEAKS_FOUND=1
            TOTAL_LEAKS=$((TOTAL_LEAKS + leaks))
        else
            echo -e "${GREEN}✅ 无泄漏${NC}"
        fi
    else
        echo -e "${YELLOW}⚠️  $test 未找到，跳过${NC}"
    fi
done

echo ""
echo "========================================"
echo "  测试结果总结"
echo "========================================"

if [ $LEAKS_FOUND -eq 1 ]; then
    echo -e "${RED}❌ 发现内存泄漏: ${TOTAL_LEAKS} 字节${NC}"
    echo ""
    echo "Valgrind 日志保存在: $BUILD_VG/valgrind_*.log"
    exit 1
else
    echo -e "${GREEN}✅ 未发现内存泄漏${NC}"
    echo ""
    echo "所有内存泄漏测试通过！"
    exit 0
fi