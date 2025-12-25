#!/bin/bash
# UVHTTP 冗余文件清理脚本
# 用于清理项目中的冗余文件和构建产物

echo "🧹 UVHTTP 冗余文件清理脚本"
echo "================================"

# 清理压力测试结果目录
echo "📊 清理压力测试结果..."
if [ -d "stress_test_results_20251222_191303" ]; then
    rm -rf stress_test_results_*/
    echo "✅ 已删除所有压力测试结果目录"
else
    echo "ℹ️  没有找到压力测试结果目录"
fi

# 清理构建产物
echo "🔨 清理构建产物..."
BUILD_FILES=("test_memory_simple" "test_simple")
for file in "${BUILD_FILES[@]}"; do
    if [ -f "$file" ]; then
        rm -f "$file"
        echo "✅ 已删除构建产物: $file"
    fi
done

# 清理测试可执行文件
echo "🧪 清理测试可执行文件..."
find test/ -name "test_*" -type f -executable 2>/dev/null | while read file; do
    rm -f "$file"
    echo "✅ 已删除测试可执行文件: $(basename $file)"
done

# 清理覆盖率文件
echo "📈 清理覆盖率文件..."
COVERAGE_FILES=("*.gcov" "*.gcda" "*.gcno" "coverage.info")
for pattern in "${COVERAGE_FILES[@]}"; do
    find . -name "$pattern" -type f -delete 2>/dev/null
done
if [ -d "coverage_html" ]; then
    rm -rf coverage_html
    echo "✅ 已删除覆盖率报告目录"
fi

# 清理性能分析文件
echo "⚡ 清理性能分析文件..."
PROF_FILES=("callgrind.out.*" "gmon.out")
for pattern in "${PROF_FILES[@]}"; do
    find . -name "$pattern" -type f -delete 2>/dev/null
done

# 清理临时文件
echo "🗂️  清理临时文件..."
TEMP_FILES=("*.tmp" "*.temp" "*.log")
for pattern in "${TEMP_FILES[@]}"; do
    find . -name "$pattern" -type f -delete 2>/dev/null
done

# 清理编辑器备份文件
echo "📝 清理编辑器备份文件..."
EDITOR_FILES=("*.swp" "*.swo" "*~" ".DS_Store")
for pattern in "${EDITOR_FILES[@]}"; do
    find . -name "$pattern" -type f -delete 2>/dev/null
done

# 清理 CMake 生成的文件
echo "🔧 清理 CMake 生成文件..."
if [ -f "CMakeCache.txt" ]; then
    rm -f CMakeCache.txt
    echo "✅ 已删除 CMakeCache.txt"
fi

if [ -d "CMakeFiles" ]; then
    rm -rf CMakeFiles
    echo "✅ 已删除 CMakeFiles 目录"
fi

# 统计清理结果
echo ""
echo "📊 清理完成！统计结果："
echo "================================"

# 显示项目大小变化
echo "📦 项目目录大小:"
du -sh . | cut -f1

echo ""
echo "🎯 建议："
echo "1. 定期运行此脚本保持项目整洁"
echo "2. 在提交代码前运行清理"
echo "3. 将构建产物添加到 .gitignore"
echo ""
echo "✨ 清理脚本执行完成！"