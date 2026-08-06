#!/usr/bin/env bash
# Check for broken internal links in documentation.
# Validates that every relative markdown link points to an existing file.
#
# Usage: bash scripts/check-links.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DOCS_DIR="$PROJECT_ROOT/docs"

errors=0

while IFS= read -r -d '' file; do
  dir=$(dirname "$file")
  while IFS= read -r link; do
    # Extract relative markdown links: [text](path.md) or [text](path)
    target=$(echo "$link" | sed -n 's/.*](\([^)]*\.md[^)]*\)).*/\1/p' | sed 's/[#?].*//')
    [ -z "$target" ] && continue

    # Skip external URLs
    [[ "$target" =~ ^https?:// ]] && continue

    # Resolve relative path
    if [[ "$target" = /* ]]; then
      resolved="$DOCS_DIR$target"
    else
      resolved="$dir/$target"
    fi

    # Normalize
    resolved=$(realpath --relative-to="$PROJECT_ROOT" "$resolved" 2>/dev/null || echo "")

    if [ ! -f "$resolved" ]; then
      echo "BROKEN: $file -> $target" >&2
      errors=$((errors + 1))
    fi
  done < <(grep -oE '\[([^]]*)\]\(([^)]+\.md[^)]*)\)' "$file" || true)
done < <(find "$DOCS_DIR" -name "*.md" -type f -not -path "*/node_modules/*" -not -path "*/dev/*" -not -path "*/spec/*" -print0)

if [ "$errors" -gt 0 ]; then
  echo "Found $errors broken link(s)." >&2
  exit 1
fi

echo "All internal links valid."
