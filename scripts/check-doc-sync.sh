#!/usr/bin/env bash
# Check EN/ZH doc sync status.
#
# Without flags: generates docs/.vitepress/sync-status.json
# With --check:  exits 1 if any file is outdated or missing (CI blocking gate)
# With --list:   prints outdated or missing file paths, one per line
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

# Get the latest commit hash that touched a file.
# NOTE: git log on an untracked path exits 0 with empty output, so fall
# back on empty result too, not just non-zero exit.
file_hash() {
  local h
  h=$(git -C "$PROJECT_ROOT" log -1 --format=%H -- "$1" 2>/dev/null)
  echo "${h:-0000000000000000000000000000000000000000}"
}

# Get the commit timestamp (Unix epoch) for a file's latest commit
file_timestamp() {
  local t
  t=$(git -C "$PROJECT_ROOT" log -1 --format=%ct -- "$1" 2>/dev/null)
  echo "${t:-0}"
}

# Check if ZH is outdated relative to EN.
# Returns 0 (in-sync) if ZH commit timestamp >= EN commit timestamp,
# meaning ZH was last modified at or after EN's latest change.
is_outdated() {
  local en_ts="$1"
  local zh_ts="$2"
  [ "$zh_ts" -lt "$en_ts" ] && return 0 || return 1
}

MODE="${1:-generate}"

declare -A en_hashes
declare -A zh_hashes
declare -A en_timestamps
declare -A zh_timestamps
declare -A missing_hashes
missing_count=0
outdated_count=0
total_count=0

# --- Scan EN files in scope ---
while IFS= read -r -d '' en_file; do
  rel="${en_file#$DOCS_DIR/}"
  is_in_scope "$rel" || continue

  en_hash=$(file_hash "$en_file")
  total_count=$((total_count + 1))

  zh_file="$DOCS_DIR/zh/$rel"
  if [ ! -f "$zh_file" ]; then
    # ZH translation missing entirely — counts as a sync failure
    echo "MISSING: $rel (no ZH translation at zh/$rel)" >&2
    missing_count=$((missing_count + 1))
    missing_hashes["$rel"]="$en_hash"
    continue
  fi

  zh_hash=$(file_hash "$zh_file")
  en_ts=$(file_timestamp "$en_file")
  zh_ts=$(file_timestamp "$zh_file")

  en_hashes["$rel"]="$en_hash"
  zh_hashes["$rel"]="$zh_hash"
  en_timestamps["$rel"]="$en_ts"
  zh_timestamps["$rel"]="$zh_ts"
done < <(find "$DOCS_DIR" -name "*.md" -type f -not -path "*/zh/*" -not -path "*/node_modules/*" -not -path "*/.vitepress/*" -print0 | sort -z)

# --- Handle modes that don't write JSON ---
if [ "$MODE" = "--list" ]; then
  for rel in $(printf '%s\n' "${!en_hashes[@]}" | sort); do
    en_ts="${en_timestamps[$rel]}"
    zh_ts="${zh_timestamps[$rel]}"
    if [ "$zh_ts" -lt "$en_ts" ]; then
      echo "$rel"
      outdated_count=$((outdated_count + 1))
    fi
  done
  # Missing ZH translations are also out of sync
  for rel in $(printf '%s\n' "${!missing_hashes[@]}" | sort); do
    echo "$rel"
  done
  exit $(( (outdated_count + missing_count) > 0 ? 1 : 0 ))
fi

# --- Generate sync-status.json ---
{
  echo "{"
  first=true
  for rel in $(printf '%s\n' "${!en_hashes[@]}" "${!missing_hashes[@]}" | sort -u); do
    if [ "${missing_hashes[$rel]+set}" = "set" ]; then
      en_h="${missing_hashes[$rel]}"
      zh_h="null"
      outdated=1
    else
      en_h="${en_hashes[$rel]}"
      zh_h="\"${zh_hashes[$rel]}\""
      en_ts="${en_timestamps[$rel]}"
      zh_ts="${zh_timestamps[$rel]}"
      outdated=$([ "$zh_ts" -lt "$en_ts" ] && echo "1" || echo "0")
      [ "$outdated" = "1" ] && outdated_count=$((outdated_count + 1))
    fi

    if [ "$first" = true ]; then first=false; else echo ","; fi
    printf '  "%s": {\n    "en": "%s",\n    "zh": %s,\n    "outdated": %s\n  }' \
      "$rel" "$en_h" "$zh_h" "$outdated"
  done
  echo ""
  echo "}"
} > "$OUTPUT_FILE"

echo "sync-status.json: $total_count EN files ($outdated_count outdated, $missing_count missing ZH)"

if [ "$MODE" = "--check" ]; then
  if [ $((outdated_count + missing_count)) -gt 0 ]; then
    echo "FAIL: $outdated_count outdated, $missing_count missing ZH translation(s)." >&2
    echo "Run: bash scripts/check-doc-sync.sh --list to see out-of-sync files." >&2
    exit 1
  fi
  echo "OK: all $total_count EN/ZH pairs in sync."
fi
