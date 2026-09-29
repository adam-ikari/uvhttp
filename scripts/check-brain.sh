#!/usr/bin/env bash
# Check that brain pages carry real content, not scaffolding placeholders.
#
# The brain stores durable knowledge in markdown. A page created but never
# filled in leaves a placeholder in its compiled_truth block, which the index
# then renders verbatim — so an empty page silently reads as "understood" when
# nothing was actually recorded. This check fails the build on that state.
#
# Structure differs by kind, and the check follows it:
#   pages/*.md  — frontmatter has `id:`, body has a <!-- compiled_truth -->
#                 block that must be present and non-empty
#   root + index — free-form bodies, no compiled_truth block; only the
#                 placeholder scan applies
#
# It also verifies that [[wiki-links]] resolve, matching what `brain lint-links`
# does locally, so the guarantee holds in CI where the CLI is not installed.
#
# Usage: bash scripts/check-brain.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BRAIN_DIR="$PROJECT_ROOT/brain"

errors=0

if [ ! -d "$BRAIN_DIR" ]; then
    echo "check-brain: no brain/ directory, nothing to check"
    exit 0
fi

# Placeholders the scaffolding leaves behind. Kept in sync with the brain
# skill's create-page template.
PLACEHOLDER_PATTERN='current best understanding|replace this with the real content'

echo "Checking brain pages for placeholder content..."

while IFS= read -r file; do
    rel="${file#"$PROJECT_ROOT/"}"
    is_page=false
    grep -q '^id:' "$file" && is_page=true

    if grep -qE "$PLACEHOLDER_PATTERN" "$file"; then
        echo "  FAIL $rel — contains an unfilled placeholder"
        grep -nE "$PLACEHOLDER_PATTERN" "$file" | sed 's/^/       /'
        errors=$((errors + 1))
        continue
    fi

    if [ "$is_page" = true ]; then
        # A page with frontmatter but no compiled_truth block records nothing.
        if ! grep -q '<!-- compiled_truth -->' "$file"; then
            echo "  FAIL $rel — missing compiled_truth block"
            errors=$((errors + 1))
            continue
        fi

        # compiled_truth must be non-empty: the marker followed by nothing is
        # the same "page exists but says nothing" state as a placeholder.
        body=$(sed -n '/<!-- compiled_truth -->/,/<!-- \/compiled_truth -->/p' \
               "$file" | sed '1d;$d' | tr -d '[:space:]')
        if [ -z "$body" ]; then
            echo "  FAIL $rel — compiled_truth is empty"
            errors=$((errors + 1))
            continue
        fi
    fi

    echo "  ok   $rel"
done < <(find "$BRAIN_DIR" -name '*.md' -not -path '*/node_modules/*' | sort)

# Wiki-links must resolve, so a page renamed without updating its references
# fails here rather than leaving a dangling pointer in the knowledge base.
echo "Checking brain wiki-links..."
page_ids=$(find "$BRAIN_DIR/pages" -name '*.md' -exec basename {} .md \; 2>/dev/null | sort)
root_slugs=$(find "$BRAIN_DIR" -maxdepth 1 -name '*.md' -exec basename {} .md \; 2>/dev/null | sort)

while IFS= read -r link_file; do
    rel="${link_file#"$PROJECT_ROOT/"}"
    while IFS= read -r target; do
        [ -z "$target" ] && continue
        if ! echo "$page_ids" | grep -qx "$target" \
           && ! echo "$root_slugs" | grep -qx "$target"; then
            echo "  FAIL $rel — broken wiki-link [[$target]]"
            errors=$((errors + 1))
        fi
    done < <(grep -oE '\[\[[^]]+\]\]' "$link_file" 2>/dev/null \
             | sed 's/^\[\[//; s/\]\]$//' | sed 's/|.*$//' | sort -u)
done < <(find "$BRAIN_DIR" -name '*.md' -not -path '*/node_modules/*' | sort)

if [ "$errors" -gt 0 ]; then
    echo ""
    echo "check-brain: FAILED — $errors problem(s) found"
    exit 1
fi

echo ""
echo "check-brain: OK (no placeholders, all wiki-links resolve)"
