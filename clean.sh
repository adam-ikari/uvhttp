#!/bin/bash
# UVHTTP 清理脚本
# 用于清理构建产物和无用文件

set -e

echo "=========================================="
echo "UVHTTP 清理脚本"
echo "=========================================="
echo ""

# 颜色输出
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# 获取项目根目录
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$PROJECT_ROOT"

# 清理选项
CLEAN_BUILD=false
CLEAN_DEPS=false
CLEAN_ALL=false

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --build)
            CLEAN_BUILD=true
            shift
            ;;
        --deps)
            CLEAN_DEPS=true
            shift
            ;;
        --all)
            CLEAN_ALL=true
            shift
            ;;
        -h|--help)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --build    清理构建目录（build, build-release等）"
            echo "  --deps     清理依赖库的构建产物（deps/*/build）"
            echo "  --all      清理所有构建产物"
            echo "  -h, --help 显示帮助信息"
            echo ""
            echo "示例:"
            echo "  $0 --build    # 只清理构建目录"
            echo "  $0 --deps     # 只清理依赖库构建产物"
            echo "  $0 --all      # 清理所有"
            exit 0
            ;;
        *)
            echo -e "${RED}未知选项: $1${NC}"
            echo "使用 -h 或 --help 查看帮助"
            exit 1
            ;;
    esac
done

# 如果没有指定参数，清理所有
if [ "$CLEAN_BUILD" = false ] && [ "$CLEAN_DEPS" = false ]; then
    CLEAN_ALL=true
fi

# 清理构建目录
clean_build_dirs() {
    echo -e "${YELLOW}清理构建目录...${NC}"

    local dirs_to_clean=(
        "build"
        "build-release"
        "build-memtest"
        "build-test"
        "build_test"
        "Testing"
    )

    for dir in "${dirs_to_clean[@]}"; do
        if [ -d "$dir" ]; then
            echo -e "  删除: $dir"
            rm -rf "$dir"
        fi
    done

    echo -e "${GREEN}✓ 构建目录清理完成${NC}"
}

# 清理依赖库构建产物
clean_deps_build() {
    echo -e "${YELLOW}清理依赖库构建产物...${NC}"

    if [ -d "deps" ]; then
        # 删除所有 deps/*/build 目录
        find deps -type d -name "build" -exec rm -rf {} + 2>/dev/null || true

        # 删除编译产物
        find deps -name "*.o" -delete 2>/dev/null || true
        find deps -name "*.a" -delete 2>/dev/null || true

        echo -e "${GREEN}✓ 依赖库构建产物清理完成${NC}"
    else
        echo -e "  跳过: deps 目录不存在"
    fi
}

# 清理临时文件
clean_temp_files() {
    echo -e "${YELLOW}清理临时文件...${NC}"

    # 删除临时文件
    find . -name "*.tmp" -delete 2>/dev/null || true
    find . -name "*.temp" -delete 2>/dev/null || true
    find . -name "*.log" -delete 2>/dev/null || true
    find . -name "*.orig" -delete 2>/dev/null || true
    find . -name "*.rej" -delete 2>/dev/null || true

    # 删除编辑器临时文件
    find . -name "*.swp" -delete 2>/dev/null || true
    find . -name "*.swo" -delete 2>/dev/null || true
    find . -name "*~" -delete 2>/dev/null || true
    find . -name ".DS_Store" -delete 2>/dev/null || true

    echo -e "${GREEN}✓ 临时文件清理完成${NC}"
}

# 清理覆盖率文件
clean_coverage() {
    echo -e "${YELLOW}清理覆盖率文件...${NC}"

    find . -name "*.gcov" -delete 2>/dev/null || true
    find . -name "*.gcda" -delete 2>/dev/null || true
    find . -name "*.gcno" -delete 2>/dev/null || true
    find . -name "coverage.info" -delete 2>/dev/null || true
    [ -d "coverage_html" ] && rm -rf coverage_html

    echo -e "${GREEN}✓ 覆盖率文件清理完成${NC}"
}

# 清理性能测试结果
clean_performance_results() {
    echo -e "${YELLOW}清理性能测试结果...${NC}"

    [ -d "test/performance/results" ] && rm -rf test/performance/results/*
    find . -name "stress_test_results_*" -type d -exec rm -rf {} + 2>/dev/null || true

    echo -e "${GREEN}✓ 性能测试结果清理完成${NC}"
}

# 显示清理前的大小
show_size_before() {
    echo ""
    echo "清理前的大小:"
    du -sh . 2>/dev/null || echo "  无法获取大小"
    echo ""
}

# 显示清理后的大小
show_size_after() {
    echo ""
    echo "清理后的大小:"
    du -sh . 2>/dev/null || echo "  无法获取大小"
    echo ""
}

# 主清理流程
main() {
    show_size_before

    if [ "$CLEAN_ALL" = true ]; then
        clean_build_dirs
        clean_deps_build
        clean_temp_files
        clean_coverage
        clean_performance_results
    else
        if [ "$CLEAN_BUILD" = true ]; then
            clean_build_dirs
        fi
        if [ "$CLEAN_DEPS" = true ]; then
            clean_deps_build
        fi
    fi

    show_size_after

    echo -e "${GREEN}=========================================="
    echo "清理完成！"
    echo "==========================================${NC}"
}

# 执行主流程
main