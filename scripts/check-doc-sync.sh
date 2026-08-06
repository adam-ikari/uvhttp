#!/usr/bin/env bash
# Check EN/ZH doc sync status.
#
# Without flags: generates docs/.vitepress/sync-status.json
# With --check:  exits 1 if any file is outdated (for CI blocking gate)
# With --list:   prints outdated file paths, one per line
#
# Scope: user-facing docs (excludes dev/, spec/, releases/, api/)
# EN is source of truth. ZH files must have a commit hash >= EN file's commit hash.
#
# Usage:
#   bash scripts/check-doc-sync.sh           # generate sync-status.json
#   bash scripts/check-doc-sync.sh --check   # CI gate: exit 1 if outdated
#   bash scripts/check-doc-sync.sh --list    # list outdated files

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DOCS_DIR="$PROJECT_ROOT/docs"
OUTPUT_FILE="$DOCS_DIR/.vitepress/sync-status.json"

# Files/dirs excluded from sync scope
is_in_scope() {
  local path="$1"
  # Exclude dirs
  case "$path" in
    zh/*|*/zh/*|api/*|*/api/*|releases/*|*/releases/*|spec/*|*/spec/*|dev/*|*/dev/*|node_modules/*|*/node_modules/*|.vitepress/*|*/.vitepress/*|superpowers/*|*/superpowers/*) return 1 ;;
  esac
  # Exclude process/management docs (no ZH translation needed)
  case "$path" in
    AGILE.md|development-rhythm.md|release-strategy.md|sprint-backlog.md|PERFORMANCE_TARGETS.md|SECURITY.md) return 1 ;;
  esac
  return 0
}

# Get the latest commit hash that touched a file
file_hash() {
  git -C "$PROJECT_ROOT" log -1 --format=%H -- "$1" 2>/dev/null || echo "0000000000000000000000000000000000000000"
}

MODE="${1:-generate}"

declare -A en_hashes
declare -A zh_hashes
outdated_count=0
total_count=0

# --- Scan EN files in scope ---
while IFS= read -r -d '' en_file; do
  rel="${en_file#$DOCS_DIR/}"
  is_in_scope "$rel" || continue

  zh_file="$DOCS_DIR/zh/$rel"
  if [ ! -f "$zh_file" ]; then
    # ZH translation missing entirely
    echo "MISSING: $rel (no ZH translation at zh/$rel)" >&2
    continue
  fi

  en_hash=$(file_hash "$en_file")
  zh_hash=$(file_hash "$zh_file")

  en_hashes["$rel"]="$en_hash"
  zh_hashes["$rel"]="$zh_hash"
  total_count=$((total_count + 1))
done < <(find "$DOCS_DIR" -name "*.md" -type f -not -path "*/zh/*" -not -path "*/node_modules/*" -not -path "*/.vitepress/*" -print0 | sort -z)

# --- Handle modes that don't write JSON ---
if [ "$MODE" = "--list" ]; then
  for rel in $(printf '%s\n' "${!en_hashes[@]}" | sort); do
    en_h="${en_hashes[$rel]}"
    zh_h="${zh_hashes[$rel]}"
    if [ "$en_h" != "$zh_h" ]; then
      echo "$rel"
      outdated_count=$((outdated_count + 1))
    fi
  done
  exit $(( outdated_count > 0 ? 1 : 0 ))
fi

# --- Generate sync-status.json ---
{
  echo "{"
  first=true
  for rel in $(printf '%s\n' "${!en_hashes[@]}" | sort); do
    en_h="${en_hashes[$rel]}"
    zh_h="${zh_hashes[$rel]}"
    outdated=$([ "$en_h" != "$zh_h" ] && echo "1" || echo "0")
    [ "$outdated" = "1" ] && outdated_count=$((outdated_count + 1))

    if [ "$first" = true ]; then first=false; else echo ","; fi
    printf '  "%s": {\n    "en": "%s",\n    "zh": "%s",\n    "outdated": %s\n  }' \
      "$rel" "$en_h" "$zh_h" "$outdated"
  done
  echo ""
  echo "}"
} > "$OUTPUT_FILE"

echo "sync-status.json: $total_count files, $outdated_count outdated"

if [ "$MODE" = "--check" ]; then
  if [ "$outdated_count" -gt 0 ]; then
    echo "FAIL: $outdated_count file(s) have outdated ZH translations." >&2
    echo "Run: bash scripts/check-doc-sync.sh --list to see outdated files." >&2
    exit 1
  fi
  echo "OK: all $total_count EN/ZH pairs in sync."
fi
