#!/bin/bash
###############################################################################
# UVHTTP Quick Start Script v2.0
#
# A professional, intelligent quick start tool for UVHTTP
# Features:
# - Automatic system detection and configuration
# - Parallel compilation with optimal settings
# - Progress tracking and visual feedback
# - Comprehensive error handling and recovery
# - Post-build validation and testing
###############################################################################

set -e

# Script metadata
VERSION="2.0.0"
SCRIPT_NAME="$(basename "$0")"

# Color scheme
readonly COLOR_RESET='\033[0m'
readonly COLOR_BOLD='\033[1m'
readonly COLOR_DIM='\033[2m'
readonly COLOR_RED='\033[31m'
readonly COLOR_GREEN='\033[32m'
readonly COLOR_YELLOW='\033[33m'
readonly COLOR_BLUE='\033[34m'
readonly COLOR_CYAN='\033[36m'

readonly COLOR_BRIGHT_RED='\033[91m'
readonly COLOR_BRIGHT_GREEN='\033[92m'
readonly COLOR_BRIGHT_YELLOW='\033[93m'
readonly COLOR_BRIGHT_BLUE='\033[94m'
readonly COLOR_BRIGHT_CYAN='\033[96m'

# Symbols
readonly SYMBOL_SUCCESS="✓"
readonly SYMBOL_ERROR="✗"
readonly SYMBOL_WARNING="⚠"
readonly SYMBOL_INFO="ℹ"
readonly SYMBOL_LOADING="⏳"
readonly SYMBOL_CHECK="✔"

# Configuration
BUILD_DIR="build"
BUILD_TYPE="Release"
RUN_TESTS=false
CLEAN_BUILD=true
VERBOSE=false
AUTO_DETECT=true

# System information
OS_NAME=""
ARCH_NAME=""
NUM_CORES=""
COMPILER_NAME=""
CMAKE_VERSION=""

# =============================================================================
# Utility Functions
# =============================================================================

# Print formatted messages
log_info() {
    echo -e "${COLOR_BRIGHT_CYAN}${SYMBOL_INFO}${COLOR_RESET} ${COLOR_CYAN}$1${COLOR_RESET}"
}

log_success() {
    echo -e "${COLOR_BRIGHT_GREEN}${SYMBOL_SUCCESS}${COLOR_RESET} ${COLOR_GREEN}$1${COLOR_RESET}"
}

log_warning() {
    echo -e "${COLOR_BRIGHT_YELLOW}${SYMBOL_WARNING}${COLOR_RESET} ${COLOR_YELLOW}$1${COLOR_RESET}"
}

log_error() {
    echo -e "${COLOR_BRIGHT_RED}${SYMBOL_ERROR}${COLOR_RESET} ${COLOR_RED}$1${COLOR_RESET}"
}

log_verbose() {
    if [ "$VERBOSE" = true ]; then
        echo -e "${COLOR_DIM}[VERBOSE]${COLOR_RESET} $1"
    fi
}

# Print section header
print_header() {
    local title="$1"
    echo ""
    echo -e "${COLOR_BOLD}${COLOR_CYAN}═══════════════════════════════════════════════════════════════${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_CYAN}  $title${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_CYAN}═══════════════════════════════════════════════════════════════${COLOR_RESET}"
    echo ""
}

# Print progress indicator
show_progress() {
    local message="$1"
    echo -ne "${COLOR_BRIGHT_CYAN}${SYMBOL_LOADING}${COLOR_RESET} ${COLOR_DIM}$message...${COLOR_RESET}"
}

# Complete progress indicator
complete_progress() {
    echo -e "\r${COLOR_BRIGHT_GREEN}${SYMBOL_CHECK}${COLOR_RESET} ${COLOR_GREEN}$1${COLOR_RESET}"
}

# Print separator
print_separator() {
    echo ""
    echo -e "${COLOR_DIM}─────────────────────────────────────────────────────────────────────${COLOR_RESET}"
    echo ""
}

# =============================================================================
# System Detection
# =============================================================================

detect_system() {
    log_info "Detecting system configuration..."

    # Detect OS
    OS_NAME="$(uname -s)"
    log_verbose "OS: $OS_NAME"

    # Detect architecture
    ARCH_NAME="$(uname -m)"
    case "$ARCH_NAME" in
        x86_64)
            ARCH_DISPLAY="64-bit"
            CMAKE_FLAGS=""
            ;;
        i686|i386)
            ARCH_DISPLAY="32-bit"
            CMAKE_FLAGS="-DCMAKE_C_FLAGS=-m32"
            log_warning "32-bit architecture detected"
            ;;
        aarch64|arm64)
            ARCH_DISPLAY="ARM64"
            CMAKE_FLAGS=""
            ;;
        *)
            ARCH_DISPLAY="Unknown ($ARCH_NAME)"
            CMAKE_FLAGS=""
            log_warning "Unknown architecture: $ARCH_NAME"
            ;;
    esac
    log_verbose "Architecture: $ARCH_NAME"

    # Detect CPU cores
    if command -v nproc &> /dev/null; then
        NUM_CORES=$(nproc)
    elif command -v sysctl &> /dev/null; then
        NUM_CORES=$(sysctl -n hw.ncpu)
    else
        NUM_CORES=4
    fi
    log_verbose "CPU cores: $NUM_CORES"

    # Detect compiler
    if command -v gcc &> /dev/null; then
        COMPILER_NAME="gcc"
        COMPILER_VERSION=$(gcc --version | head -1)
    elif command -v clang &> /dev/null; then
        COMPILER_NAME="clang"
        COMPILER_VERSION=$(clang --version | head -1)
    else
        COMPILER_NAME="none"
        COMPILER_VERSION="Not found"
    fi
    log_verbose "Compiler: $COMPILER_NAME"

    # Detect CMake
    if command -v cmake &> /dev/null; then
        CMAKE_VERSION=$(cmake --version | head -1)
    else
        CMAKE_VERSION="Not found"
    fi
    log_verbose "CMake: $CMAKE_VERSION"

    complete_progress "System detection complete"
}

# =============================================================================
# Dependency Checking
# =============================================================================

check_dependencies() {
    log_info "Checking dependencies..."

    local missing_deps=()
    local warnings=()

    # Required dependencies
    if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
        missing_deps+=("C compiler (gcc or clang)")
    fi

    if ! command -v cmake &> /dev/null; then
        missing_deps+=("cmake")
    fi

    if ! command -v make &> /dev/null; then
        missing_deps+=("make")
    fi

    if ! command -v git &> /dev/null; then
        missing_deps+=("git")
    fi

    # Optional dependencies
    if ! command -v python3 &> /dev/null; then
        warnings+=("Python 3 (optional, for some scripts)")
    fi

    # Report missing dependencies
    if [ ${#missing_deps[@]} -ne 0 ]; then
        log_error "Missing required dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo -e "  ${COLOR_RED}✗${COLOR_RESET} $dep"
        done
        echo ""

        log_error "Please install the missing dependencies:"
        echo ""
        echo -e "  ${COLOR_BOLD}Ubuntu/Debian:${COLOR_RESET}"
        echo -e "    sudo apt-get update && sudo apt-get install -y build-essential cmake git"
        echo ""
        echo -e "  ${COLOR_BOLD}Fedora/RHEL:${COLOR_RESET}"
        echo -e "    sudo dnf install -y gcc cmake make git"
        echo ""
        echo -e "  ${COLOR_BOLD}macOS:${COLOR_RESET}"
        echo -e "    brew install cmake git"
        echo ""
        exit 1
    fi

    # Report optional warnings
    if [ ${#warnings[@]} -ne 0 ]; then
        log_warning "Optional dependencies not found:"
        for warning in "${warnings[@]}"; do
            echo -e "  ${COLOR_YELLOW}⚠${COLOR_RESET} $warning"
        done
        echo ""
    fi

    complete_progress "All dependencies satisfied"
}

# =============================================================================
# Build Setup
# =============================================================================

setup_build_directory() {
    log_info "Setting up build directory..."

    if [ -d "$BUILD_DIR" ]; then
        if [ "$CLEAN_BUILD" = true ]; then
            log_verbose "Removing existing build directory"
            rm -rf "$BUILD_DIR"
        else
            log_verbose "Using existing build directory"
        fi
    fi

    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"

    complete_progress "Build directory ready"
}

# =============================================================================
# CMake Configuration
# =============================================================================

configure_cmake() {
    log_info "Configuring project with CMake..."

    local cmake_cmd="cmake .."

    # Add build type
    cmake_cmd="$cmake_cmd -DCMAKE_BUILD_TYPE=$BUILD_TYPE"

    # Add architecture-specific flags
    if [ -n "$CMAKE_FLAGS" ]; then
        cmake_cmd="$cmake_cmd $CMAKE_FLAGS"
    fi

    # Add mimalloc for 64-bit builds
    if [ "$ARCH_DISPLAY" = "64-bit" ]; then
        cmake_cmd="$cmake_cmd -DBUILD_WITH_MIMALLOC=ON"
        log_verbose "Enabling mimalloc for improved performance"
    fi

    # Add verbose flag if requested
    if [ "$VERBOSE" = true ]; then
        cmake_cmd="$cmake_cmd -DCMAKE_VERBOSE_MAKEFILE=ON"
    fi

    log_verbose "CMake command: $cmake_cmd"

    if eval "$cmake_cmd"; then
        complete_progress "CMake configuration successful"
    else
        log_error "CMake configuration failed"
        exit 1
    fi
}

# =============================================================================
# Build Compilation
# =============================================================================

compile_project() {
    log_info "Compiling UVHTTP..."

    local make_cmd="make -j$NUM_CORES"

    log_verbose "Using $NUM_CORES cores for compilation"

    if eval "$make_cmd"; then
        complete_progress "Compilation successful"
    else
        log_error "Compilation failed"
        exit 1
    fi
}

# =============================================================================
# Testing
# =============================================================================

run_tests() {
    if [ "$RUN_TESTS" = true ]; then
        log_info "Running tests..."

        if make test; then
            complete_progress "All tests passed"
        else
            log_warning "Some tests failed, but build was successful"
        fi
    else
        log_verbose "Skipping tests (use --test to run tests)"
    fi
}

# =============================================================================
# Validation
# =============================================================================

validate_build() {
    log_info "Validating build..."

    local required_files=(
        "dist/lib/libuvhttp.a"
        "dist/bin/hello_world"
    )

    local missing_files=()

    for file in "${required_files[@]}"; do
        if [ ! -f "$file" ]; then
            missing_files+=("$file")
        fi
    done

    if [ ${#missing_files[@]} -ne 0 ]; then
        log_error "Build validation failed. Missing files:"
        for file in "${missing_files[@]}"; do
            echo -e "  ${COLOR_RED}✗${COLOR_RESET} $file"
        done
        exit 1
    fi

    complete_progress "Build validation successful"
}

# =============================================================================
# Summary
# =============================================================================

print_summary() {
    print_header "Build Summary"

    echo -e "${COLOR_BOLD}System Information:${COLOR_RESET}"
    echo -e "  OS:           ${COLOR_CYAN}$OS_NAME${COLOR_RESET}"
    echo -e "  Architecture: ${COLOR_CYAN}$ARCH_DISPLAY${COLOR_RESET}"
    echo -e "  CPU Cores:    ${COLOR_CYAN}$NUM_CORES${COLOR_RESET}"
    echo -e "  Compiler:     ${COLOR_CYAN}$COMPILER_NAME${COLOR_RESET}"
    echo -e "  CMake:        ${COLOR_CYAN}$CMAKE_VERSION${COLOR_RESET}"
    echo ""

    echo -e "${COLOR_BOLD}Build Configuration:${COLOR_RESET}"
    echo -e "  Type:         ${COLOR_CYAN}$BUILD_TYPE${COLOR_RESET}"
    echo -e "  Directory:    ${COLOR_CYAN}$(pwd)${COLOR_RESET}"
    echo -e "  Tests Run:    ${COLOR_CYAN}$([ "$RUN_TESTS" = true ] && echo "Yes" || echo "No")${COLOR_RESET}"
    echo ""

    echo -e "${COLOR_BOLD}Quick Start:${COLOR_RESET}"
    echo -e "  ${COLOR_DIM}Run hello world:${COLOR_RESET}"
    echo -e "    ${COLOR_GREEN}./dist/bin/hello_world${COLOR_RESET}"
    echo ""
    echo -e "  ${COLOR_DIM}Run performance test:${COLOR_RESET}"
    echo -e "    ${COLOR_GREEN}./dist/bin/performance_static_server -d ../public -p 8080${COLOR_RESET}"
    echo ""
    echo -e "  ${COLOR_DIM}Build examples:${COLOR_RESET}"
    echo -e "    ${COLOR_GREEN}cd ../examples && make -f Makefile.examples${COLOR_RESET}"
    echo ""

    echo -e "${COLOR_BOLD}Documentation:${COLOR_RESET}"
    echo -e "  ${COLOR_DIM}API Reference:  ${COLOR_CYAN}../docs/api/${COLOR_RESET}"
    echo -e "  ${COLOR_DIM}User Guide:     ${COLOR_CYAN}../docs/guide/${COLOR_RESET}"
    echo -e "  ${COLOR_DIM}Performance:    ${COLOR_CYAN}../docs/performance.md${COLOR_RESET}"
    echo ""

    echo -e "${COLOR_BOLD}Support:${COLOR_RESET}"
    echo -e "  ${COLOR_DIM}GitHub Issues:   ${COLOR_CYAN}https://github.com/adam-ikari/uvhttp/issues${COLOR_RESET}"
    echo -e "  ${COLOR_DIM}Discussions:    ${COLOR_CYAN}https://github.com/adam-ikari/uvhttp/discussions${COLOR_RESET}"
    echo ""
}

# =============================================================================
# Help and Usage
# =============================================================================

show_help() {
    echo -e "${COLOR_BOLD}UVHTTP Quick Start Script v$VERSION${COLOR_RESET}"
    echo ""
    echo -e "${COLOR_BOLD}Usage:${COLOR_RESET}"
    echo -e "  $SCRIPT_NAME [OPTIONS]"
    echo ""
    echo -e "${COLOR_BOLD}Options:${COLOR_RESET}"
    echo -e "  ${COLOR_GREEN}--test, -t${COLOR_RESET}        Run tests after build"
    echo -e "  ${COLOR_GREEN}--debug, -d${COLOR_RESET}       Build with debug symbols"
    echo -e "  ${COLOR_GREEN}--no-clean, -n${COLOR_RESET}    Don't clean existing build directory"
    echo -e "  ${COLOR_GREEN}--verbose, -v${COLOR_RESET}     Enable verbose output"
    echo -e "  ${COLOR_GREEN}--help, -h${COLOR_RESET}        Show this help message"
    echo ""
    echo -e "${COLOR_BOLD}Examples:${COLOR_RESET}"
    echo -e "  $SCRIPT_NAME                 # Quick build with defaults"
    echo -e "  $SCRIPT_NAME --test          # Build and run tests"
    echo -e "  $SCRIPT_NAME --debug         # Debug build"
    echo -e "  $SCRIPT_NAME --test --debug  # Debug build with tests"
    echo ""
}

# =============================================================================
# Argument Parsing
# =============================================================================

parse_arguments() {
    while [ $# -gt 0 ]; do
        case $1 in
            --test|-t)
                RUN_TESTS=true
                shift
                ;;
            --debug|-d)
                BUILD_TYPE="Debug"
                shift
                ;;
            --no-clean|-n)
                CLEAN_BUILD=false
                shift
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                echo ""
                show_help
                exit 1
                ;;
        esac
    done
}

# =============================================================================
# Main Function
# =============================================================================

main() {
    # Parse command line arguments
    parse_arguments "$@"

    # Print welcome message
    print_header "UVHTTP Quick Start v$VERSION"

    # Execute build steps
    detect_system
    check_dependencies
    setup_build_directory
    configure_cmake
    compile_project
    run_tests
    validate_build

    # Print summary
    cd ..
    print_summary

    # Success message
    print_header "Build Complete!"

    echo -e "${COLOR_BRIGHT_GREEN}${SYMBOL_SUCCESS}${COLOR_RESET} ${COLOR_GREEN}UVHTTP has been built successfully!${COLOR_RESET}"
    echo ""
}

# Run main function
main "$@"