#!/bin/bash
# UVHTTP Release Creation Script
# This script automates the creation of release notes and migration guides

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
RELEASES_DIR="$PROJECT_ROOT/docs/releases"
TEMPLATE_FILE="$RELEASES_DIR/release-template.md"

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Create a new UVHTTP release with release notes and migration guide.

OPTIONS:
    -v, --version VERSION       Version number (e.g., 2.4.0)
    -t, --type TYPE            Release type (major, minor, patch)
    -d, --date DATE             Release date (default: today)
    -p, --previous VERSION     Previous version (default: auto-detect)
    -o, --output FILE          Output file (default: docs/releases/VERSION.md)
    -h, --help                 Show this help message

EXAMPLES:
    $0 --version 2.4.0 --type minor
    $0 --version 2.4.0 --type minor --date 2026-03-01
    $0 --version 2.4.0 --type minor --output docs/releases/custom.md

EOF
    exit 1
}

# Parse command line arguments
VERSION=""
TYPE=""
DATE=$(date +%Y-%m-%d)
PREVIOUS=""
OUTPUT=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--version)
            VERSION="$2"
            shift 2
            ;;
        -t|--type)
            TYPE="$2"
            shift 2
            ;;
        -d|--date)
            DATE="$2"
            shift 2
            ;;
        -p|--previous)
            PREVIOUS="$2"
            shift 2
            ;;
        -o|--output)
            OUTPUT="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            print_error "Unknown option: $1"
            usage
            ;;
    esac
done

# Validate required arguments
if [[ -z "$VERSION" ]]; then
    print_error "Version number is required"
    usage
fi

if [[ -z "$TYPE" ]]; then
    print_error "Release type is required"
    usage
fi

# Set default output file
if [[ -z "$OUTPUT" ]]; then
    OUTPUT="$RELEASES_DIR/$VERSION.md"
fi

# Auto-detect previous version
if [[ -z "$PREVIOUS" ]]; then
    # Find the latest release file
    LATEST_RELEASE=$(find "$RELEASES_DIR" -name "*.md" -type f ! -name "release-template.md" | sort -V | tail -1)
    if [[ -n "$LATEST_RELEASE" ]]; then
        PREVIOUS=$(basename "$LATEST_RELEASE" .md)
        print_info "Auto-detected previous version: $PREVIOUS"
    else
        print_warning "Could not auto-detect previous version"
        PREVIOUS="2.2.2"
    fi
fi

# Calculate next version
NEXT_VERSION=""
if [[ "$TYPE" == "major" ]]; then
    MAJOR=$(echo $VERSION | cut -d. -f1)
    NEXT_VERSION="$((MAJOR + 1)).0.0"
elif [[ "$TYPE" == "minor" ]]; then
    MAJOR=$(echo $VERSION | cut -d. -f1)
    MINOR=$(echo $VERSION | cut -d. -f2)
    NEXT_VERSION="$MAJOR.$((MINOR + 1)).0"
elif [[ "$TYPE" == "patch" ]]; then
    MAJOR=$(echo $VERSION | cut -d. -f1)
    MINOR=$(echo $VERSION | cut -d. -f2)
    PATCH=$(echo $VERSION | cut -d. -f3)
    NEXT_VERSION="$MAJOR.$MINOR.$((PATCH + 1))"
fi

print_info "Creating release notes for UVHTTP v$VERSION"
print_info "Release type: $TYPE"
print_info "Release date: $DATE"
print_info "Previous version: $PREVIOUS"
print_info "Next version: $NEXT_VERSION"
print_info "Output file: $OUTPUT"

# Check if template exists
if [[ ! -f "$TEMPLATE_FILE" ]]; then
    print_error "Template file not found: $TEMPLATE_FILE"
    exit 1
fi

# Create release notes from template
print_info "Creating release notes from template..."
sed -e "s/{VERSION}/$VERSION/g" \
    -e "s/{DATE}/$DATE/g" \
    -e "s/{RELEASE_TYPE}/$TYPE/g" \
    -e "s/{PREVIOUS_VERSION}/$PREVIOUS/g" \
    -e "s/{NEXT_VERSION}/$NEXT_VERSION/g" \
    "$TEMPLATE_FILE" > "$OUTPUT"

print_success "Release notes created: $OUTPUT"
print_warning "Please edit the release notes to add specific information for this release"
print_warning "Fill in the following sections:"
print_warning "  - Overview"
print_warning "  - New Features"
print_warning "  - Bug Fixes"
print_warning "  - Breaking Changes"
print_warning "  - Performance Improvements"
print_warning "  - Migration Guide"

# Ask if user wants to open the file for editing
read -p "Do you want to open the release notes for editing now? (y/n) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    ${EDITOR:-nano} "$OUTPUT"
fi

print_success "Release creation complete!"
print_info "Next steps:"
print_info "  1. Edit the release notes to add specific information"
print_info "  2. Review and test the migration guide"
print_info "  3. Commit the release notes"
print_info "  4. Create a GitHub release"
print_info "  5. Update the website documentation"