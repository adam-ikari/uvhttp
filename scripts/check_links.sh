#!/bin/bash

# Check for broken links in documentation

cd "$(dirname "$0")/.."

echo "Checking documentation links..."

# Find all markdown files in docs directory (excluding node_modules)
find docs -name "*.md" -type f -not -path "*/node_modules/*" | while read -r file; do
    echo "Checking: $file"
    
    # Extract all HTTP/HTTPS links
    grep -oE 'https?://[^ ")]+' "$file" | while read -r url; do
        # Skip localhost, 127.0.0.1, and example.com
        if [[ "$url" =~ (localhost|127\.0\.0\.1|example\.com) ]]; then
            continue
        fi
        
        # Skip GitHub links (usually stable)
        if [[ "$url" =~ github\.com ]]; then
            continue
        fi
        
        # Check if URL is accessible
        if curl -sSf -L --max-time 5 "$url" > /dev/null 2>&1; then
            echo "  ✓ $url"
        else
            echo "  ✗ $url (BROKEN)"
        fi
    done
done

echo "Link check complete."