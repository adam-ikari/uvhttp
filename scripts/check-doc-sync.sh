#!/usr/bin/env bash
# Check EN/ZH doc sync status
# Generates a JSON file with last-modified timestamps for all EN/ZH doc pairs
# Used by VitePress to show "outdated" warnings on ZH pages

set -euo pipefail

DOCS_DIR="/home/gem/project/uvhttp/docs"
OUTPUT_FILE="$DOCS_DIR/.vitepress/sync-status.json"

echo "{" > "$OUTPUT_FILE"
first=true

# Map EN files to ZH files (same relative path under docs/ vs docs/zh/)
for en_file in $(find "$DOCS_DIR" -name "*.md" -not -path "*/node_modules/*" -not -path "*/.vitepress/*" -not -path "*/zh/*" -not -path "*/api/*" -not -path "*/releases/*" -not -path "*/superpowers/*" -not -path "*/spec/*" | sort); do
  rel_path="${en_file#$DOCS_DIR/}"
  zh_file="$DOCS_DIR/zh/$rel_path"

  if [ -f "$zh_file" ]; then
    en_time=$(git log -1 --format=%ct "$en_file" 2>/dev/null || echo "0")
    zh_time=$(git log -1 --format=%ct "$zh_file" 2>/dev/null || echo "0")

    if [ "$first" = true ]; then
      first=false
    else
      echo "," >> "$OUTPUT_FILE"
    fi

    cat >> "$OUTPUT_FILE" << EOF
  "$rel_path": {
    "en": $en_time,
    "zh": $zh_time,
    "outdated": $(($en_time > $zh_time))
  }
EOF
  fi
done

echo "" >> "$OUTPUT_FILE"
echo "}" >> "$OUTPUT_FILE"

echo "Sync status written to $OUTPUT_FILE"
cat "$OUTPUT_FILE"