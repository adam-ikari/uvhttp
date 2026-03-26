#!/bin/bash
# Build script for GitHub Pages deployment
# This script ensures all necessary files are created for proper GitHub Pages hosting

set -e

echo "🚀 Building documentation for GitHub Pages..."
echo ""

# Check if we're in docs directory
if [ ! -f "package.json" ]; then
    echo "❌ Error: This script must be run from the docs/ directory"
    exit 1
fi

# Install dependencies if needed
if [ ! -d "node_modules" ]; then
    echo "📦 Installing dependencies..."
    npm install
fi

# Build with base path for GitHub Pages
echo "🔨 Building VitePress site with GitHub Pages configuration..."
DEPLOY=gh-pages npm run build

# Add .nojekyll file to disable Jekyll processing
# This is CRITICAL for VitePress to work on GitHub Pages
echo "📝 Creating .nojekyll file..."
touch .vitepress/dist/.nojekyll

# Verify critical files
echo ""
echo "✅ Verifying build output..."

if [ ! -f ".vitepress/dist/.nojekyll" ]; then
    echo "❌ ERROR: .nojekyll file not created!"
    exit 1
fi

if [ ! -f ".vitepress/dist/index.html" ]; then
    echo "❌ ERROR: index.html not found!"
    exit 1
fi

if [ ! -d ".vitepress/dist/assets" ]; then
    echo "❌ ERROR: assets directory not found!"
    exit 1
fi

# Check for base path in assets
if grep -q "/uvhttp/assets/" .vitepress/dist/index.html; then
    echo "✅ Assets have correct base path (/uvhttp/)"
else
    echo "⚠️  Warning: Assets may not have correct base path"
fi

echo ""
echo "✨ Build complete! Ready for GitHub Pages deployment."
echo ""
echo "📋 Summary:"
echo "  - Output directory: .vitepress/dist/"
echo "  - Base path: /uvhttp/"
echo "  - .nojekyll file: created"
echo ""
echo "🚀 To deploy, run:"
echo "  gh-pages -d .vitepress/dist -b gh-pages --repo adam-ikari/uvhttp --dotfiles"
