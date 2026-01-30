#!/bin/bash
# Convert Doxygen HTML documentation to Markdown format
# This script converts the generated HTML docs to Markdown using pandoc

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
HTML_DIR="docs/api/html"
MARKDOWN_DIR="docs/api/markdown"

echo -e "${GREEN}📝 Converting Doxygen HTML to Markdown...${NC}"

# Check if HTML directory exists
if [ ! -d "$HTML_DIR" ]; then
    echo -e "${RED}Error: HTML documentation not found at $HTML_DIR${NC}"
    echo "Please run 'make docs' first to generate HTML documentation."
    exit 1
fi

# Check if pandoc is installed
if ! command -v pandoc >/dev/null 2>&1; then
    echo -e "${RED}Error: pandoc is not installed${NC}"
    echo "Please install pandoc: sudo apt-get install pandoc"
    exit 1
fi

# Create markdown directory
mkdir -p "$MARKDOWN_DIR"

# Convert main index.html
echo -e "${YELLOW}Converting index.html...${NC}"
pandoc "$HTML_DIR/index.html" -o "$MARKDOWN_DIR/index.md" \
    --from html \
    --to markdown \
    --wrap=none \
    --extract-media=./images 2>/dev/null || true

# Convert all HTML files
echo -e "${YELLOW}Converting HTML files...${NC}"
for html_file in "$HTML_DIR"/*.html; do
    if [ -f "$html_file" ]; then
        filename=$(basename "$html_file" .html)
        echo "  Converting $filename.html..."
        pandoc "$html_file" -o "$MARKDOWN_DIR/$filename.md" \
            --from html \
            --to markdown \
            --wrap=none \
            --extract-media=./images 2>/dev/null || true
    fi
done

# Convert subdirectories
echo -e "${YELLOW}Converting subdirectories...${NC}"
find "$HTML_DIR" -type d -mindepth 1 | while read -r subdir; do
    relative_path="${subdir#$HTML_DIR/}"
    mkdir -p "$MARKDOWN_DIR/$relative_path"

    for html_file in "$subdir"/*.html; do
        if [ -f "$html_file" ]; then
            filename=$(basename "$html_file" .html)
            echo "  Converting $relative_path/$filename.html..."
            pandoc "$html_file" -o "$MARKDOWN_DIR/$relative_path/$filename.md" \
                --from html \
                --to markdown \
                --wrap=none \
                --extract-media=../../images 2>/dev/null || true
        fi
    done
done

# Create a README for the markdown docs
cat > "$MARKDOWN_DIR/README.md" << 'EOF'
# UVHTTP API Documentation (Markdown)

This directory contains the UVHTTP API documentation converted to Markdown format.

## Files

- `index.md` - Main index page
- `files.md` - File documentation
- `annotated.md` - Annotated data structures
- `classes.md` - Class documentation
- `globals.md` - Global functions and macros

## Subdirectories

- `struct` - Structure documentation
- `search` - Search results (can be ignored)

## Notes

These Markdown files are automatically generated from the Doxygen HTML documentation using pandoc. For the most up-to-date documentation, please refer to the HTML version at `../html/index.html`.

To regenerate these files, run:
```bash
make docs-markdown
```
EOF

echo -e "${GREEN}✅ Markdown documentation generated successfully!${NC}"
echo -e "  Location: $MARKDOWN_DIR/"
echo -e "  Main file: $MARKDOWN_DIR/index.md"