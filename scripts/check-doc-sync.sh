#!/usr/bin/env bash
# Check EN/ZH doc sync status
# Compares git hashes of EN and ZH doc pairs.
# If the EN file has a different hash than the ZH file (meaning EN was
# updated without updating ZH), marks it as outdated.
#
# Usage: bash scripts/check-doc-sync.sh
# Output: docs/.vitepress/sync-status.json

set -euo pipefail

DOCS_DIR="/home/gem/project/uvhttp/docs"
OUTPUT_FILE="$DOCS_DIR/.vitepress/sync-status.json"

echo "{" > "$OUTPUT_FILE"
first=true

for en_file in $(find "$DOCS_DIR" -name "*.md" -not -path "*/node_modules/*" -not -path "*/.vitepress/*" -not -path "*/zh/*" -not -path "*/api/*" -not -path "*/releases/*" -not -path "*/superpowers/*" -not -path "*/spec/*" | sort); do
  rel_path="${en_file#$DOCS_DIR/}"
  zh_file="$DOCS_DIR/zh/$rel_path"

  if [ -f "$zh_file" ]; then
    en_hash=$(git log -1 --format=%H "$en_file" 2>/dev/null || echo "0000")
    zh_hash=$(git log -1 --format=%H "$zh_file" 2>/dev/null || echo "0000")

    if [ "$first" = true ]; then
      first=false
    else
      echo "," >> "$OUTPUT_FILE"
    fi

    cat >> "$OUTPUT_FILE" << EOF
  "$rel_path": {
    "en": "$en_hash",
    "zh": "$zh_hash",
    "outdated": $([ "$en_hash" != "$zh_hash" ] && echo "1" || echo "0")
  }
EOF
  fi
done

echo "" >> "$OUTPUT_FILE"
echo "}" >> "$OUTPUT_FILE"