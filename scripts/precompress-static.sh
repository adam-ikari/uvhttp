#!/bin/bash
# Pre-compress static files for uvhttp static file server.
#
# Creates .gz files alongside originals so the static file server can
# serve them with Content-Encoding: gzip without compressing on the fly.
#
# Usage:
#   ./scripts/precompress-static.sh <static-root> [options]
#
# Options:
#   --level N     Compression level (1-9, default: 6)
#   --types EXT   File extensions to compress (default: html css js json svg xml)
#   --quiet       Suppress output
#
# Examples:
#   # Compress all HTML, CSS, JS in ./public
#   ./scripts/precompress-static.sh ./public
#
#   # Compress with maximum compression
#   ./scripts/precompress-static.sh ./public --level 9
#
#   # Compress only HTML and JS
#   ./scripts/precompress-static.sh ./public --types "html js"

set -euo pipefail

STATIC_ROOT="${1:?Usage: $0 <static-root> [options]}"
COMPRESS_LEVEL=6
QUIET=0
TYPES="html css js json svg xml"

shift
while [[ $# -gt 0 ]]; do
    case "$1" in
        --level) COMPRESS_LEVEL="$2"; shift 2 ;;
        --types) TYPES="$2"; shift 2 ;;
        --quiet) QUIET=1; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

if ! [[ "$COMPRESS_LEVEL" =~ ^[1-9]$ ]]; then
    echo "Error: compression level must be 1-9"
    exit 1
fi

if [ ! -d "$STATIC_ROOT" ]; then
    echo "Error: directory not found: $STATIC_ROOT"
    exit 1
fi

# Build find pattern: -o for each extension
PATTERN=""
for ext in $TYPES; do
    if [ -n "$PATTERN" ]; then
        PATTERN="$PATTERN -o"
    fi
    PATTERN="$PATTERN -name \"*.$ext\""
done

COUNT=0
SKIPPED=0
TOTAL_ORIG=0
TOTAL_GZ=0

# Use a temp file to collect results from the subshell
TMPFILE=$(mktemp)
trap "rm -f $TMPFILE" EXIT

eval "find \"$STATIC_ROOT\" -type f $PATTERN" > "$TMPFILE"

while IFS= read -r file; do
    # Skip if .gz already exists and is newer than the original
    if [ -f "${file}.gz" ] && [ "${file}.gz" -nt "$file" ]; then
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    ORIG_SIZE=$(stat -c%s "$file" 2>/dev/null || stat -f%z "$file" 2>/dev/null)

    if gzip -"$COMPRESS_LEVEL" -c "$file" > "${file}.gz"; then
        GZ_SIZE=$(stat -c%s "${file}.gz" 2>/dev/null || stat -f%z "${file}.gz" 2>/dev/null)
        SAVED=$((ORIG_SIZE - GZ_SIZE))
        PCT=0
        if [ "$ORIG_SIZE" -gt 0 ]; then
            PCT=$((SAVED * 100 / ORIG_SIZE))
        fi
        COUNT=$((COUNT + 1))
        TOTAL_ORIG=$((TOTAL_ORIG + ORIG_SIZE))
        TOTAL_GZ=$((TOTAL_GZ + GZ_SIZE))
        if [ "$QUIET" -eq 0 ]; then
            printf "  %-60s %s\n" "${file#${STATIC_ROOT}/}" "${SAVED} bytes saved (${PCT}%)"
        fi
    else
        echo "  FAILED: $file" >&2
        rm -f "${file}.gz"
    fi
done < "$TMPFILE"

if [ "$QUIET" -eq 0 ]; then
    echo ""
    echo "Done: $COUNT files compressed, $SKIPPED skipped"
    if [ "$COUNT" -gt 0 ]; then
        echo "Total: $TOTAL_ORIG -> $TOTAL_GZ bytes (saved $((TOTAL_ORIG - TOTAL_GZ)) bytes)"
    fi
fi