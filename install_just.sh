#!/usr/bin/env bash
# Just Installation Script for UVHTTP
# This script installs the Just command runner

set -euo pipefail

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
JUST_VERSION="1.47.0"
INSTALL_DIR="${HOME}/.local/bin"
JUST_URL="https://github.com/casey/just/releases/download/v${JUST_VERSION}/just-${JUST_VERSION}-x86_64-unknown-linux-musl.tar.gz"

# Functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $*"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $*"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*"
}

# Check if Just is already installed
check_existing() {
    if command -v just &> /dev/null; then
        local current_version=$(just --version 2>/dev/null || echo "unknown")
        log_info "Just is already installed: ${current_version}"
        return 0
    fi
    return 1
}

# Install Just using curl
install_with_curl() {
    log_info "Installing Just using curl..."
    
    if ! command -v curl &> /dev/null; then
        log_error "curl not found. Please install curl first."
        return 1
    fi
    
    # Create install directory
    mkdir -p "${INSTALL_DIR}"
    
    # Download Just
    log_info "Downloading Just v${JUST_VERSION}..."
    curl -fsSL "${JUST_URL}" -o /tmp/just.tar.gz
    
    # Extract and install
    log_info "Extracting Just..."
    tar -xzf /tmp/just.tar.gz -C /tmp/
    
    log_info "Installing to ${INSTALL_DIR}..."
    mv /tmp/just "${INSTALL_DIR}/just"
    chmod +x "${INSTALL_DIR}/just"
    
    # Cleanup
    rm -f /tmp/just.tar.gz
    
    log_info "Just installed successfully!"
}

# Install Just using wget
install_with_wget() {
    log_info "Installing Just using wget..."
    
    if ! command -v wget &> /dev/null; then
        log_error "wget not found. Please install wget first."
        return 1
    fi
    
    # Create install directory
    mkdir -p "${INSTALL_DIR}"
    
    # Download Just
    log_info "Downloading Just v${JUST_VERSION}..."
    wget -q "${JUST_URL}" -O /tmp/just.tar.gz
    
    # Extract and install
    log_info "Extracting Just..."
    tar -xzf /tmp/just.tar.gz -C /tmp/
    
    log_info "Installing to ${INSTALL_DIR}..."
    mv /tmp/just "${INSTALL_DIR}/just"
    chmod +x "${INSTALL_DIR}/just"
    
    # Cleanup
    rm -f /tmp/just.tar.gz
    
    log_info "Just installed successfully!"
}

# Check PATH configuration
check_path() {
    if [[ ":$PATH:" != *":${INSTALL_DIR}:"* ]]; then
        log_warn "${INSTALL_DIR} is not in PATH"
        log_info "Add the following to your ~/.bashrc or ~/.zshrc:"
        echo "  export PATH=\"${INSTALL_DIR}:\$PATH\""
        log_info "Then run: source ~/.bashrc (or ~/.zshrc)"
    fi
}

# Main installation
main() {
    echo "========================================="
    echo "  UVHTTP Just Installation Script"
    echo "========================================="
    echo ""
    
    # Check existing installation
    if check_existing; then
        log_info "You can update Just by running this script again."
        read -p "Do you want to reinstall Just? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            log_info "Installation cancelled."
            exit 0
        fi
    fi
    
    # Detect download method
    if command -v curl &> /dev/null; then
        install_with_curl
    elif command -v wget &> /dev/null; then
        install_with_wget
    else
        log_error "Neither curl nor wget found. Please install one of them."
        exit 1
    fi
    
    # Verify installation
    if [[ -f "${INSTALL_DIR}/just" ]]; then
        log_info "Just v${JUST_VERSION} installed successfully!"
        log_info "Location: ${INSTALL_DIR}/just"
        check_path
    else
        log_error "Installation failed. Just not found at ${INSTALL_DIR}/just"
        exit 1
    fi
    
    echo ""
    echo "========================================="
    echo "  Installation Complete!"
    echo "========================================="
    echo ""
    echo "Usage:"
    echo "  just              # Show available tasks"
    echo "  just build        # Build UVHTTP"
    echo "  just test         # Run tests"
    echo "  just help         # Show all tasks"
    echo ""
    echo "For more information: https://just.systems/"
}

# Run main function
main "$@"