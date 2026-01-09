#!/bin/bash

# UVHTTP 限流功能测试脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印函数
print_header() {
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════════════════════${NC}"
    echo -e "${BLUE} $1${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════════════════════════════════${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

# 检查依赖
check_dependencies() {
    print_header "检查依赖"
    
    if [ ! -f "build/dist/lib/libuvhttp.a" ]; then
        print_error "libuvhttp 未找到，请先编译项目"
        echo "运行: cd build && cmake .. && make"
        exit 1
    fi
    
    print_success "依赖检查通过"
}

# 编译测试程序
compile_tests() {
    print_header "编译测试程序"
    
    # 编译单元测试
    echo "编译单元测试..."
    gcc -o test_rate_limit_unit test_rate_limit_unit.c \
        -I../include \
        -I../deps/libuv/include \
        -I../deps/cllhttp \
        -I../deps/mbedtls/include \
        -I../deps/xxhash \
        -I../deps/mimalloc/include \
        -I../deps/uthash/src \
        -L../build/dist/lib \
        -L../deps/libuv/.libs \
        -luvhttp -luv -lpthread -lm
    
    if [ $? -eq 0 ]; then
        print_success "单元测试编译成功"
    else
        print_error "单元测试编译失败"
        exit 1
    fi
    
    # 编译性能测试
    echo "编译性能测试..."
    gcc -o rate_limit_benchmark rate_limit_benchmark.c \
        -I../include \
        -I../deps/libuv/include \
        -I../deps/cllhttp \
        -I../deps/mbedtls/include \
        -I../deps/xxhash \
        -I../deps/mimalloc/include \
        -I../deps/uthash/src \
        -L../build/dist/lib \
        -L../deps/libuv/.libs \
        -luvhttp -luv -lpthread -lm
    
    if [ $? -eq 0 ]; then
        print_success "性能测试编译成功"
    else
        print_error "性能测试编译失败"
        exit 1
    fi
    
    # 编译压力测试
    echo "编译压力测试..."
    gcc -o rate_limit_stress rate_limit_stress.c \
        -I../include \
        -I../deps/libuv/include \
        -I../deps/cllhttp \
        -I../deps/mbedtls/include \
        -I../deps/xxhash \
        -I../deps/mimalloc/include \
        -I../deps/uthash/src \
        -L../build/dist/lib \
        -L../deps/libuv/.libs \
        -luvhttp -luv -lpthread -lm
    
    if [ $? -eq 0 ]; then
        print_success "压力测试编译成功"
    else
        print_error "压力测试编译失败"
        exit 1
    fi
}

# 运行单元测试
run_unit_tests() {
    print_header "运行单元测试"
    
    ./test_rate_limit_unit
    
    if [ $? -eq 0 ]; then
        print_success "单元测试通过"
    else
        print_error "单元测试失败"
        exit 1
    fi
}

# 运行性能测试
run_performance_tests() {
    print_header "运行性能测试"
    
    ./rate_limit_benchmark
    
    if [ $? -eq 0 ]; then
        print_success "性能测试完成"
    else
        print_error "性能测试失败"
        exit 1
    fi
}

# 运行压力测试
run_stress_tests() {
    local duration=${1:-60}
    local threads=${2:-8}
    
    print_header "运行压力测试（${duration}秒，${threads}线程）"
    
    ./rate_limit_stress ${threads} ${duration}
    
    if [ $? -eq 0 ]; then
        print_success "压力测试完成"
    else
        print_error "压力测试失败"
        exit 1
    fi
}

# 运行所有测试
run_all_tests() {
    print_header "运行所有测试"
    
    run_unit_tests
    echo ""
    run_performance_tests
    echo ""
    run_stress_tests 30 4
    
    print_success "所有测试完成"
}

# 清理
clean() {
    print_header "清理测试文件"
    
    rm -f test_rate_limit_unit rate_limit_benchmark rate_limit_stress
    
    print_success "清理完成"
}

# 显示帮助
show_help() {
    echo "UVHTTP 限流功能测试脚本"
    echo ""
    echo "用法: $0 [选项] [参数]"
    echo ""
    echo "选项:"
    echo "  -h, --help          显示帮助信息"
    echo "  -c, --compile       仅编译测试程序"
    echo "  -u, --unit          仅运行单元测试"
    echo "  -p, --performance   仅运行性能测试"
    echo "  -s, --stress [时长] [线程数]  运行压力测试（默认：60秒，8线程）"
    echo "  -a, --all           运行所有测试"
    echo "  --clean             清理测试文件"
    echo ""
    echo "示例:"
    echo "  $0                  运行所有测试"
    echo "  $0 -c               仅编译"
    echo "  $0 -u               仅运行单元测试"
    echo "  $0 -s 120 16        运行压力测试（120秒，16线程）"
    echo "  $0 --clean          清理测试文件"
}

# 主函数
main() {
    cd "$(dirname "$0")"
    
    case "$1" in
        -h|--help)
            show_help
            ;;
        -c|--compile)
            check_dependencies
            compile_tests
            ;;
        -u|--unit)
            check_dependencies
            compile_tests
            run_unit_tests
            ;;
        -p|--performance)
            check_dependencies
            compile_tests
            run_performance_tests
            ;;
        -s|--stress)
            check_dependencies
            compile_tests
            run_stress_tests "$2" "$3"
            ;;
        -a|--all|"")
            check_dependencies
            compile_tests
            run_all_tests
            ;;
        --clean)
            clean
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
}

# 运行主函数
main "$@"