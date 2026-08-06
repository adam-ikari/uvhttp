#!/usr/bin/env bash
# AI-assisted EN→ZH documentation translation.
#
# Detects outdated or missing ZH translations, then invokes an AI agent
# (Claude Code) to translate each EN file to Chinese.
#
# Usage:
#   bash scripts/translate-docs.sh              # translate ALL outdated files
#   bash scripts/translate-docs.sh <file.md>    # translate one specific file
#   bash scripts/translate-docs.sh --dry-run    # list what would be translated

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DOCS_DIR="$PROJECT_ROOT/docs"

DRY_RUN=false
TARGET=""

while [ $# -gt 0 ]; do
  case "$1" in
    --dry-run) DRY_RUN=true; shift ;;
    *) TARGET="$1"; shift ;;
  esac
done

FILES=()

if [ -n "$TARGET" ]; then
  # Single file mode
  EN_FILE="$TARGET"
  REL="${EN_FILE#docs/}"
  ZH_FILE="$DOCS_DIR/zh/$REL"

  if [ ! -f "$DOCS_DIR/$REL" ]; then
    echo "ERROR: EN source not found: docs/$REL" >&2
    exit 1
  fi
  FILES=("$REL")
else
  # Batch mode: find all outdated via check-doc-sync.sh --list
  # Output is plain file paths, one per line; exit code 1 if any outdated
  mapfile -t FILES < <(bash "$SCRIPT_DIR/check-doc-sync.sh" --list 2>/dev/null || true)

  # Also find EN files with no corresponding ZH file
  while IFS= read -r -d '' en_file; do
    rel="${en_file#$DOCS_DIR/}"
    # Skip excluded dirs
    case "$rel" in
      zh/*|*/zh/*|api/*|*/api/*|releases/*|*/releases/*|spec/*|*/spec/*|dev/*|*/dev/*|node_modules/*|*/node_modules/*|.vitepress/*|*/.vitepress/*|superpowers/*|*/superpowers/*) continue ;;
    esac
    # Skip process/management docs
    case "$rel" in
      AGILE.md|development-rhythm.md|release-strategy.md|sprint-backlog.md|PERFORMANCE_TARGETS.md|SECURITY.md) continue ;;
    esac

    zh_path="$DOCS_DIR/zh/$rel"
    if [ ! -f "$zh_path" ]; then
      # Add missing ZH files that aren't already in the list
      FILES+=("$rel")
    fi
  done < <(find "$DOCS_DIR" -name "*.md" -type f -not -path "*/zh/*" -not -path "*/node_modules/*" -not -path "*/.vitepress/*" -print0 | sort -z)

  # Deduplicate
  FILES=($(printf '%s\n' "${FILES[@]}" | sort -u))
fi

if [ ${#FILES[@]} -eq 0 ]; then
  echo "All ZH translations are in sync. Nothing to translate."
  exit 0
fi

echo "Files to translate: ${#FILES[@]}"
for rel in "${FILES[@]}"; do
  echo "  docs/$rel → docs/zh/$rel"
done

if [ "$DRY_RUN" = true ]; then
  exit 0
fi

echo ""
echo "=== Running AI-assisted translation ==="
echo "This script invokes a Claude Code subagent for each file."
echo "Estimated time: ~30-60s per file."
echo ""

# Translate each file
FAILED=()
for rel in "${FILES[@]}"; do
  en_path="$DOCS_DIR/$rel"
  zh_path="$DOCS_DIR/zh/$rel"

  echo ""
  echo "=== Translating: $rel ==="

  # Ensure ZH directory exists
  mkdir -p "$(dirname "$zh_path")"

  # Check if ZH already exists and is already in sync
  if [ -f "$zh_path" ]; then
    en_hash=$(git -C "$PROJECT_ROOT" log -1 --format=%H -- "$en_path" 2>/dev/null || echo "0")
    zh_hash=$(git -C "$PROJECT_ROOT" log -1 --format=%H -- "$zh_path" 2>/dev/null || echo "0")
    if [ "$en_hash" = "$zh_hash" ] && [ "$en_hash" != "0" ]; then
      echo "  (already in sync — skipping)"
      continue
    fi
  fi

  lines=$(wc -l < "$en_path")
  echo "  EN source: $rel ($lines lines)"
  echo "  Target:    docs/zh/$rel"

  # Run Claude Code subagent to perform the translation
  # Uses a structured prompt with translation rules matching project standards
  if claude --print --output-format text --mcp-config none --allowedTools Read,Write,Bash \
    "Translate the following English markdown file to Simplified Chinese (zh-CN) and write the result.

**Source:** docs/$rel
**Target:** docs/zh/$rel

**Translation rules:**
1. Keep all markdown structure intact (headings, code blocks, tables, links, frontmatter)
2. Translate frontmatter \`title:\` and \`description:\` fields
3. DO NOT translate code identifiers, function names, file paths, or URLs
4. DO NOT add emoji, hype adjectives, filler transitions, or rhetorical questions
5. Keep the same voice as the English: terse, factual, technical
6. Match the structure exactly — same sections, same order, same list items
7. For technical terms, use Chinese where natural but keep English terms parenthetically where clarity matters
8. Output ONLY the translated markdown, no preamble or postamble

**Steps:**
1. Read the source file: docs/$rel
2. Translate the content following the rules above
3. Write the translated content to: docs/zh/$rel
" 2>&1; then
    echo "  ✓ Translated: docs/zh/$rel"
  else
    echo "  ✗ FAILED translating: $rel" >&2
    FAILED+=("$rel")
  fi
done

echo ""
echo "=== Summary ==="
translated=$(( ${#FILES[@]} - ${#FAILED[@]} ))
echo "Translated: $translated / ${#FILES[@]}"

if [ ${#FAILED[@]} -gt 0 ]; then
  echo "Failed:"
  for f in "${FAILED[@]}"; do
    echo "  - $f"
  done
fi

echo ""
echo "Run: bash scripts/check-doc-sync.sh --check"
echo "to verify all files are in sync."
