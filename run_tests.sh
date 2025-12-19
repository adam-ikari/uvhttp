#!/bin/bash

# UVHTTP 测试运行脚本

set -e

echo "=== UVHTTP 测试套件 ==="

# 检查必要工具
check_tools() {
    echo "检查必要的工具..."
    
    if ! command -v gcc &> /dev/null; then
        echo "错误: 需要安装 gcc"
        exit 1
    fi
    
    if ! command -v gcov &> /dev/null; then
        echo "警告: gcov 未找到，覆盖率功能可能不可用"
    fi
    
    if ! command -v lcov &> /dev/null; then
        echo "警告: lcov 未找到，将生成基本的覆盖率报告"
    fi
    
    if ! command -v genhtml &> /dev/null; then
        echo "警告: genhtml 未找到，将无法生成HTML覆盖率报告"
    fi
}

# 清理之前的构建
clean_build() {
    echo "清理之前的构建..."
    rm -rf build
    rm -f *.gcda *.gcno *.info
    rm -rf coverage_html
}

# 构建项目
build_project() {
    echo "构建项目（Debug模式，启用覆盖率）..."
    mkdir -p build
    cd build
    
    cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
    make -j$(nproc)
    
    cd ..
}

# 运行单元测试
run_unit_tests() {
    echo "运行单元测试..."
    cd build
    
    if [ -f "./uvhttp_unit_tests" ]; then
        echo "执行单元测试..."
        ./uvhttp_unit_tests
        
        if [ $? -eq 0 ]; then
            echo "✓ 单元测试通过"
        else
            echo "✗ 单元测试失败"
            exit 1
        fi
    else
        echo "错误: 单元测试可执行文件未找到"
        exit 1
    fi
    
    cd ..
}

# 生成覆盖率报告
generate_coverage() {
    echo "生成代码覆盖率报告..."
    cd build
    
    # 检查是否有覆盖率数据
    if ls *.gcda >/dev/null 2>&1; then
        echo "发现覆盖率数据文件"
        
        # 生成基础覆盖率信息
        if command -v lcov &> /dev/null; then
            echo "使用 lcov 生成详细报告..."
            
            # 捕获覆盖率数据
            lcov --capture --directory . --output-file coverage.info
            
            # 过滤系统文件
            lcov --remove coverage.info '/usr/*' --output-file coverage.info
            
            # 显示覆盖率摘要
            echo "=== 覆盖率摘要 ==="
            lcov --list coverage.info
            
            # 生成HTML报告
            if command -v genhtml &> /dev/null; then
                echo "生成HTML覆盖率报告..."
                genhtml coverage.info --output-directory coverage_html
                echo "HTML报告已生成: coverage_html/index.html"
            fi
            
            # 检查覆盖率是否达到80%
            COVERAGE_PERCENT=$(lcov --summary coverage.info 2>&1 | grep "lines......:" | tail -1 | awk '{print $2}' | sed 's/%//')
            
            if [ -n "$COVERAGE_PERCENT" ]; then
                echo "当前代码覆盖率: ${COVERAGE_PERCENT}%"
                
                # 使用bc进行浮点数比较（如果可用）
                if command -v bc &> /dev/null; then
                    if (( $(echo "$COVERAGE_PERCENT >= 80" | bc -l) )); then
                        echo "✓ 覆盖率达标 (>=80%)"
                    else
                        echo "✗ 覆盖率不达标 (<80%)"
                        echo "需要增加更多测试用例以提高覆盖率"
                    fi
                else
                    # 如果没有bc，使用字符串比较
                    if [ "${COVERAGE_PERCENT%.*}" -ge 80 ]; then
                        echo "✓ 覆盖率达标 (>=80%)"
                    else
                        echo "✗ 覆盖率不达标 (<80%)"
                    fi
                fi
            fi
        else
            echo "lcov 不可用，使用 gcov 生成基础报告..."
            
            # 使用 gcov 生成基础报告
            gcov *.gcno 2>/dev/null || true
            
            echo "基础覆盖率报告已生成 (*.gcov 文件)"
        fi
    else
        echo "未找到覆盖率数据文件"
        echo "可能原因："
        echo "1. 测试未正确执行"
        echo "2. 编译时未启用覆盖率标志"
        echo "3. 使用了不支持的编译器"
    fi
    
    cd ..
}

# 运行性能测试
run_performance_tests() {
    echo "运行性能测试..."
    cd build
    
    if [ -f "./uvhttp_test" ]; then
        echo "执行性能测试..."
        ./uvhttp_test
    else
        echo "性能测试可执行文件未找到，跳过"
    fi
    
    cd ..
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo ""
    echo "选项:"
    echo "  -h, --help     显示此帮助信息"
    echo "  -c, --clean    仅清理构建文件"
    echo "  -b, --build    仅构建项目"
    echo "  -t, --test     仅运行测试"
    echo "  -f, --fast     快速模式（跳过性能测试）"
    echo ""
    echo "默认行为：完整构建、测试并生成覆盖率报告"
}

# 主函数
main() {
    local clean_only=false
    local build_only=false
    local test_only=false
    local fast_mode=false
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_help
                exit 0
                ;;
            -c|--clean)
                clean_only=true
                shift
                ;;
            -b|--build)
                build_only=true
                shift
                ;;
            -t|--test)
                test_only=true
                shift
                ;;
            -f|--fast)
                fast_mode=true
                shift
                ;;
            *)
                echo "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # 执行相应操作
    check_tools
    
    if [ "$clean_only" = true ]; then
        clean_build
        echo "清理完成"
        exit 0
    fi
    
    if [ "$test_only" = true ]; then
        run_unit_tests
        generate_coverage
        exit 0
    fi
    
    clean_build
    
    if [ "$build_only" = true ]; then
        build_project
        echo "构建完成"
        exit 0
    fi
    
    build_project
    run_unit_tests
    generate_coverage
    
    if [ "$fast_mode" = false ]; then
        run_performance_tests
    fi
    
    echo ""
    echo "=== 测试完成 ==="
    echo "构建目录: build/"
    if [ -d "build/coverage_html" ]; then
        echo "覆盖率报告: build/coverage_html/index.html"
    fi
}

# 运行主函数
main "$@"