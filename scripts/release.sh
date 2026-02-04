#!/bin/bash
# UVHTTP 简化发布脚本
# 用法: ./scripts/release.sh 2.4.0 minor

set -e

VERSION=$1
TYPE=$2

if [[ -z "$VERSION" ]]; then
    echo "用法: $0 <版本号> <类型>"
    echo "示例: $0 2.4.0 minor"
    echo "类型: major, minor, patch"
    exit 1
fi

if [[ -z "$TYPE" ]]; then
    TYPE="patch"
fi

# 颜色输出
GREEN='\033[0;32m'
NC='\033[0m'

echo -e "${GREEN}=== UVHTTP v$VERSION 发布 ===${NC}"

# 检测上一个版本
PREVIOUS=$(find docs/releases -name "*.md" -type f ! -name "template.md" ! -name "README.md" 2>/dev/null | sort -V | tail -1)
if [[ -n "$PREVIOUS" ]]; then
    PREVIOUS=$(basename "$PREVIOUS" .md)
    echo "上一个版本: v$PREVIOUS"
else
    PREVIOUS="2.2.2"
    echo "警告: 无法检测到上一个版本，使用默认值: v$PREVIOUS"
fi

# 创建发布文件
OUTPUT="docs/releases/$VERSION.md"

# 复制模板
if [[ -f "docs/releases/template.md" ]]; then
    cp docs/releases/template.md "$OUTPUT"
else
    echo "错误: 找不到模板文件 docs/releases/template.md"
    exit 1
fi

# 替换变量
sed -i "s/{VERSION}/$VERSION/g" "$OUTPUT"
sed -i "s/{DATE}/$(date +%Y-%m-%d)/g" "$OUTPUT"
sed -i "s/{RELEASE_TYPE}/$TYPE/g" "$OUTPUT"
sed -i "s/{PREVIOUS_VERSION}/$PREVIOUS/g" "$OUTPUT"

echo -e "${GREEN}✓ 发布文件已创建: $OUTPUT${NC}"
echo ""
echo "下一步:"
echo "1. 编辑 $OUTPUT 填写发布信息"
echo "2. 提交文件: git add docs/releases/$VERSION.md && git commit -m 'docs: add release notes for v$VERSION'"
echo "3. 推送: git push"
echo "4. 在 GitHub 创建 release tag"