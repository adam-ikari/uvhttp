#!/bin/bash
# 更新 Doxygen API 文档

set -e

echo "🔄 开始更新 API 文档..."

# 生成 Doxygen 文档（直接输出到 docs/public/api-reference）
echo "📝 生成 Doxygen 文档..."
doxygen Doxyfile

echo "✅ API 文档更新完成！"
echo ""
echo "文档位置："
echo "  - 本地文件: docs/public/api-reference/index.html"
echo "  - 网站访问: /api-reference/"
echo ""
echo "如需预览网站，运行："
echo "  cd docs && npm run dev"