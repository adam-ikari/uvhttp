#!/bin/bash
# UVHTTP Documentation Build Script
# This script builds all documentation including API docs and VitePress site

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Project root directory
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}  UVHTTP Documentation Build Script${NC}"
echo -e "${GREEN}========================================${NC}"
echo ""

# Step 1: Check prerequisites
echo -e "${YELLOW}[1/4] Checking prerequisites...${NC}"

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo -e "${RED}Error: Doxygen is not installed${NC}"
    echo "Please install Doxygen:"
    echo "  Ubuntu/Debian: sudo apt-get install doxygen graphviz"
    echo "  macOS: brew install doxygen graphviz"
    exit 1
fi

# Check if Node.js is installed
if ! command -v node &> /dev/null; then
    echo -e "${RED}Error: Node.js is not installed${NC}"
    echo "Please install Node.js 18 or higher"
    exit 1
fi

# Check Node.js version
NODE_VERSION=$(node -v | cut -d'v' -f2 | cut -d'.' -f1)
if [ "$NODE_VERSION" -lt 18 ]; then
    echo -e "${RED}Error: Node.js version must be 18 or higher (current: $(node -v))${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Doxygen found: $(doxygen --version)${NC}"
echo -e "${GREEN}✓ Node.js found: $(node -v)${NC}"
echo ""

# Step 2: Generate API documentation with Doxygen
echo -e "${YELLOW}[2/4] Generating API documentation with Doxygen...${NC}"

# Create necessary directories (only XML is generated per Doxyfile configuration)
mkdir -p docs/api/xml docs/api/markdown_from_xml

# Run Doxygen
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ API documentation generated successfully${NC}"
else
    echo -e "${RED}✗ Doxygen failed${NC}"
    exit 1
fi
echo ""

# Step 3: Process API documentation (convert XML to Markdown if needed)
echo -e "${YELLOW}[3/4] Processing API documentation...${NC}"

# Check if XML to Markdown conversion script exists
if [ -f "scripts/docs/convert_xml_to_markdown.js" ]; then
    node scripts/docs/convert_xml_to_markdown.js
    echo -e "${GREEN}✓ API documentation processed${NC}"
else
    echo -e "${YELLOW}⚠ XML to Markdown script not found, skipping conversion${NC}"
fi
echo ""

# Step 4: Build VitePress site
echo -e "${YELLOW}[4/4] Building VitePress documentation site...${NC}"

cd docs

# Install dependencies
if [ ! -d "node_modules" ]; then
    echo "Installing npm dependencies..."
    npm install
else
    echo "npm dependencies already installed"
fi

# Build VitePress site
DEPLOY=${DEPLOY:-local} npm run build

if [ $? -eq 0 ]; then
    cd "$PROJECT_ROOT"
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}  Documentation Build Complete!${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo ""
    echo "Generated files:"
    echo -e "  • API XML: ${GREEN}docs/api/xml/${NC}"
    echo -e "  • API Markdown: ${GREEN}docs/api/markdown_from_xml/${NC}"
    echo -e "  • VitePress Site: ${GREEN}docs/.vitepress/dist/${NC}"
    echo ""
    echo "To preview the VitePress site:"
    echo -e "  ${YELLOW}cd docs && npm run dev${NC}"
    echo ""
    echo "Then open: ${GREEN}http://localhost:5173${NC}"
    echo ""
else
    echo -e "${RED}✗ VitePress build failed${NC}"
    exit 1
fi