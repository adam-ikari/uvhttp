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

# Track all EN files that have a ZH counterpart, plus detect missing pairs.
# EN docs live in docs/ (index, MEMORY_SAFETY) and docs/guide/ (FAQ, build, ...).
# ZH docs live in docs/zh/ mirroring docs/, or docs/zh/guide/ mirroring docs/guide/.
declare -A en_files
for en_file in $(find "$DOCS_DIR" -name "*.md" -not -path "*/node_modules/*" -not -path "*/.vitepress/*" -not -path "*/zh/*" -not -path "*/api/*" -not -path "*/releases/*" -not -path "*/superpowers/*" -not -path "*/spec/*" | sort); do
  en_path="${en_file#$DOCS_DIR/}"

  # Find matching ZH file: strip optional guide/ prefix then re-add under zh/
  if [[ "$en_path" == guide/* ]]; then
    zh_file="$DOCS_DIR/zh/$en_path"
  else
    zh_file="$DOCS_DIR/zh/$en_path"
  fi
  # Also try docs/zh/guide/ for top-level EN docs that have guide-version ZH
  if [ ! -f "$zh_file" ] && [[ "$en_path" != guide/* ]]; then
    zh_file="$DOCS_DIR/zh/guide/$en_path"
  fi
  # Track basename for missing-pair detection
  en_files["${en_path##*/}"]=1

  if [ -f "$zh_file" ]; then
    en_hash=$(git log -1 --format=%H "$en_file" 2>/dev/null || echo "0000")
    zh_hash=$(git log -1 --format=%H "$zh_file" 2>/dev/null || echo "0000")

    if [ "$first" = true ]; then
      first=false
    else
      echo "," >> "$OUTPUT_FILE"
    fi

    cat >> "$OUTPUT_FILE" << EOF
  "$en_path": {
    "en": "$en_hash",
    "zh": "$zh_hash",
    "outdated": $([ "$en_hash" != "$zh_hash" ] && echo "1" || echo "0")
  }
EOF
  fi
done

# Detect ZH files that have no EN counterpart (missing English translation).
for zh_file in $(find "$DOCS_DIR/zh" -name "*.md" -not -path "*/node_modules/*" | sort); do
  base="${zh_file##*/}"
  # Skip helper/framework files that don't need translation
  case "$base" in
    DOCUMENTATION_STANDARDS.md|*) ;;
  esac
  if [ -z "${en_files[$base]+x}" ]; then
    if [ "$first" = true ]; then
      first=false
    else
      echo "," >> "$OUTPUT_FILE"
    fi

    rel_path="${zh_file#$DOCS_DIR/zh/}"
    cat >> "$OUTPUT_FILE" << EOF
  "$rel_path": {
    "en": "MISSING",
    "zh": "present",
    "outdated": 1
  }
EOF
  fi
done

echo "" >> "$OUTPUT_FILE"
echo "}" >> "$OUTPUT_FILE"