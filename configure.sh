#!/bin/bash
###############################################################################
# UVHTTP Configuration Assistant v2.0
#
# A professional, user-friendly configuration tool for UVHTTP
# Features:
# - Progressive disclosure of options
# - Intelligent defaults based on system
# - Clear visual hierarchy
# - Context-sensitive help
# - Validation and error recovery
###############################################################################

set -e

# Version information
VERSION="2.0.0"

# Color scheme
COLOR_RESET='\033[0m'
COLOR_BOLD='\033[1m'
COLOR_DIM='\033[2m'
COLOR_UNDERLINE='\033[4m'
COLOR_BLINK='\033[5m'
COLOR_REVERSE='\033[7m'

# Foreground colors
COLOR_BLACK='\033[30m'
COLOR_RED='\033[31m'
COLOR_GREEN='\033[32m'
COLOR_YELLOW='\033[33m'
COLOR_BLUE='\033[34m'
COLOR_MAGENTA='\033[35m'
COLOR_CYAN='\033[36m'
COLOR_WHITE='\033[37m'

# Background colors
COLOR_BG_BLACK='\033[40m'
COLOR_BG_RED='\033[41m'
COLOR_BG_GREEN='\033[42m'
COLOR_BG_YELLOW='\033[43m'
COLOR_BG_BLUE='\033[44m'
COLOR_BG_MAGENTA='\033[45m'
COLOR_BG_CYAN='\033[46m'
COLOR_BG_WHITE='\033[47m'

# Bright foreground colors
COLOR_BRIGHT_BLACK='\033[90m'
COLOR_BRIGHT_RED='\033[91m'
COLOR_BRIGHT_GREEN='\033[92m'
COLOR_BRIGHT_YELLOW='\033[93m'
COLOR_BRIGHT_BLUE='\033[94m'
COLOR_BRIGHT_MAGENTA='\033[95m'
COLOR_BRIGHT_CYAN='\033[96m'
COLOR_BRIGHT_WHITE='\033[97m'

# Symbols
SYMBOL_SUCCESS="✓"
SYMBOL_ERROR="✗"
SYMBOL_WARNING="⚠"
SYMBOL_INFO="ℹ"
SYMBOL_QUESTION="?"
SYMBOL_ARROW="→"
SYMBOL_BULLET="•"

# Terminal capabilities
if ! command -v tput &> /dev/null; then
    # Fallback if tput is not available
    COLS=80
else
    COLS=$(tput cols 2>/dev/null || echo 80)
fi

# Progress tracking
STEP_CURRENT=0
STEP_TOTAL=8

# Configuration storage
declare -A CONFIG
CONFIG[build_type]="Release"
CONFIG[allocator]="mimalloc"
CONFIG[arch]="64-bit"
CONFIG[websocket]="on"
CONFIG[static_files]="on"
CONFIG[rate_limit]="on"
CONFIG[tls]="on"
CONFIG[build_tests]="on"
CONFIG[build_examples]="on"
CONFIG[build_benchmarks]="on"
CONFIG[logging]="on"
CONFIG[optimization]="2"
CONFIG[install_prefix]="/usr/local"

# =============================================================================
# UI Functions
# =============================================================================

# Print a header
print_header() {
    local title="$1"
    local width=${#title}

    echo ""
    echo -e "${COLOR_BOLD}${COLOR_CYAN}╔$(printf '═%.0s' $(seq 1 $((width + 4))))╗${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_CYAN}║${COLOR_RESET}  ${COLOR_BOLD}${COLOR_WHITE}$title${COLOR_RESET}  ${COLOR_BOLD}${COLOR_CYAN}║${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_CYAN}╚$(printf '═%.0s' $(seq 1 $((width + 4))))╝${COLOR_RESET}"
    echo ""
}

# Print a section header
print_section() {
    local title="$1"
    echo ""
    echo -e "${COLOR_BOLD}${COLOR_BLUE}$title${COLOR_RESET}"
    echo -e "${COLOR_BOLD}${COLOR_BLUE}$(printf '─%.0s' $(seq 1 ${#title}))${COLOR_RESET}"
    echo ""
}

# Print progress
print_progress() {
    STEP_CURRENT=$((STEP_CURRENT + 1))
    local percentage=$((STEP_CURRENT * 100 / STEP_TOTAL))
    local filled=$((percentage / 5))
    local empty=$((20 - filled))

    printf "\r${COLOR_BOLD}${COLOR_CYAN}Progress:${COLOR_RESET} "
    printf "[${COLOR_BRIGHT_GREEN}"
    printf '█%.0s' $(seq 1 $filled)
    printf "${COLOR_BRIGHT_BLACK}"
    printf '░%.0s' $(seq 1 $empty)
    printf "${COLOR_RESET}] ${COLOR_BOLD}%d%%${COLOR_RESET} (%d/%d)" "$percentage" "$STEP_CURRENT" "$STEP_TOTAL"
}

# Print success message
print_success() {
    echo -e "${COLOR_BRIGHT_GREEN}${SYMBOL_SUCCESS}${COLOR_RESET} ${COLOR_GREEN}$1${COLOR_RESET}"
}

# Print error message
print_error() {
    echo -e "${COLOR_BRIGHT_RED}${SYMBOL_ERROR}${COLOR_RESET} ${COLOR_RED}$1${COLOR_RESET}" >&2
}

# Print warning message
print_warning() {
    echo -e "${COLOR_BRIGHT_YELLOW}${SYMBOL_WARNING}${COLOR_RESET} ${COLOR_YELLOW}$1${COLOR_RESET}"
}

# Print info message
print_info() {
    echo -e "${COLOR_BRIGHT_CYAN}${SYMBOL_INFO}${COLOR_RESET} ${COLOR_CYAN}$1${COLOR_RESET}"
}

# Print question
print_question() {
    echo -e "${COLOR_BRIGHT_MAGENTA}${SYMBOL_QUESTION}${COLOR_RESET} ${COLOR_MAGENTA}$1${COLOR_RESET}"
}

# Print option
print_option() {
    local key="$1"
    local value="$2"
    local description="$3"

    printf "  ${COLOR_BOLD}${COLOR_CYAN}%s${COLOR_RESET} " "$key"
    printf "[${COLOR_BRIGHT_GREEN}%s${COLOR_RESET}] " "$value"
    printf "- ${COLOR_DIM}%s${COLOR_RESET}\n" "$description"
}

# Print help text
print_help() {
    echo -e "${COLOR_DIM}${COLOR_UNDERLINE}Help:${COLOR_RESET}"
    echo -e "${COLOR_DIM}$1${COLOR_RESET}"
    echo ""
}

# Print separator
print_separator() {
    echo ""
    echo -e "${COLOR_DIM}$(printf '─%.0s' $(seq 1 $((COLS - 2))))${COLOR_RESET}"
    echo ""
}

# =============================================================================
# Input Functions
# =============================================================================

# Read user input with default value
read_input() {
    local prompt="$1"
    local default="$2"
    local response

    read -p "$(echo -e "${COLOR_BOLD}${COLOR_MAGENTA}$prompt${COLOR_RESET} [${COLOR_BRIGHT_GREEN}$default${COLOR_RESET}]: ")" response
    echo "${response:-$default}"
}

# Ask yes/no question
ask_yes_no() {
    local prompt="$1"
    local default="${2:-yes}"
    local response

    if [ "$default" = "yes" ]; then
        local default_display="[Y/n]"
    else
        local default_display="[y/N]"
    fi

    while true; do
        read -p "$(echo -e "${COLOR_BOLD}${COLOR_MAGENTA}$prompt${COLOR_RESET} ${COLOR_DIM}$default_display${COLOR_RESET}: ")" response
        response=${response:-$default}

        case "${response,,}" in
            y|yes)
                return 0
                ;;
            n|no)
                return 1
                ;;
            *)
                echo -e "${COLOR_BRIGHT_RED}${SYMBOL_ERROR}${COLOR_RESET} ${COLOR_RED}Please answer yes or no${COLOR_RESET}"
                ;;
        esac
    done
}

# Select from menu
select_menu() {
    local prompt="$1"
    shift
    local options=("$@")
    local default="$1"

    echo ""
    print_question "$prompt"

    local i
    for i in "${!options[@]}"; do
        local num=$((i + 1))
        local option="${options[$i]}"

        if [ "$option" = "$default" ]; then
            echo -e "  ${COLOR_BOLD}${COLOR_BRIGHT_GREEN}$num)${COLOR_RESET} $option ${COLOR_DIM}(default)${COLOR_RESET}"
        else
            echo -e "  ${COLOR_DIM}$num)${COLOR_RESET} $option"
        fi
    done
    echo ""

    while true; do
        read -p "$(echo -e "${COLOR_BOLD}${COLOR_MAGENTA}Select option [1-${#options[@]}]:${COLOR_RESET} ")" choice
        choice=${choice:-1}

        if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#options[@]}" ]; then
            echo "${options[$((choice-1))]}"
            return 0
        else
            echo -e "${COLOR_BRIGHT_RED}${SYMBOL_ERROR}${COLOR_RESET} ${COLOR_RED}Please enter a number between 1 and ${#options[@]}${COLOR_RESET}"
        fi
    done
}

# =============================================================================
# System Detection
# =============================================================================

detect_system() {
    print_progress
    print_section "System Detection"

    # Detect OS
    OS="$(uname -s)"
    print_option "OS" "$OS" "Operating system"

    # Detect architecture
    ARCH="$(uname -m)"
    case "$ARCH" in
        x86_64)
            CONFIG[arch]="64-bit"
            ;;
        i686|i386)
            CONFIG[arch]="32-bit"
            ;;
        aarch64|arm64)
            CONFIG[arch]="ARM64"
            ;;
        *)
            CONFIG[arch]="unknown"
            print_warning "Architecture '$ARCH' may not be fully supported"
            ;;
    esac
    print_option "Architecture" "${CONFIG[arch]}" "System architecture"

    # Detect CPU cores
    CORES=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    print_option "CPU Cores" "$CORES" "Available cores for compilation"

    # Detect compiler
    if command -v gcc &> /dev/null; then
        COMPILER="gcc $(gcc --version | head -1)"
    elif command -v clang &> /dev/null; then
        COMPILER="clang $(clang --version | head -1)"
    else
        COMPILER="Not found"
        print_error "No C compiler found!"
    fi
    print_option "Compiler" "$COMPILER" "C compiler"

    # Detect CMake
    if command -v cmake &> /dev/null; then
        CMAKE_VERSION=$(cmake --version | head -1)
    else
        CMAKE_VERSION="Not found"
        print_error "CMake not found!"
    fi
    print_option "CMake" "$CMAKE_VERSION" "Build system"

    # Set intelligent defaults
    if [ "${CONFIG[arch]}" = "32-bit" ]; then
        CONFIG[allocator]="system"
        CONFIG[websocket]="off"
        CONFIG[tls]="off"
        print_info "Adjusted defaults for 32-bit architecture"
    fi
}

# =============================================================================
# Configuration Steps
# =============================================================================

configure_build_type() {
    print_progress
    print_section "Build Configuration"

    local build_types=("Debug" "Release" "RelWithDebInfo" "MinSizeRel")

    print_help "Build types:"
    print_help "  Debug        - Full debugging symbols, no optimization"
    print_help "  Release      - Optimized for performance (recommended)"
    print_help "  RelWithDebInfo - Optimized with debug symbols"
    print_help "  MinSizeRel   - Optimized for size"

    CONFIG[build_type]=$(select_menu "Select build type" "${build_types[@]}")
    print_success "Build type: ${CONFIG[build_type]}"
}

configure_allocator() {
    print_progress
    print_section "Memory Allocator"

    local allocators=("mimalloc" "system" "custom")

    print_help "Memory allocators:"
    print_help "  mimalloc - High-performance allocator (recommended)"
    print_help "  system   - Standard malloc/free (most compatible)"
    print_help "  custom   - Your own allocator implementation"

    if [ "${CONFIG[arch]}" = "32-bit" ]; then
        CONFIG[allocator]="system"
        print_info "Using system allocator for 32-bit builds"
    else
        CONFIG[allocator]=$(select_menu "Select memory allocator" "${allocators[@]}")
    fi
    print_success "Allocator: ${CONFIG[allocator]}"
}

configure_features() {
    print_progress
    print_section "Feature Modules"

    print_help "Enable or disable optional features:"

    if ask_yes_no "Enable WebSocket support?" "yes"; then
        CONFIG[websocket]="on"
        print_success "WebSocket: enabled"
    else
        CONFIG[websocket]="off"
        print_warning "WebSocket: disabled"
    fi

    if ask_yes_no "Enable static file serving?" "yes"; then
        CONFIG[static_files]="on"
        print_success "Static files: enabled"
    else
        CONFIG[static_files]="off"
        print_warning "Static files: disabled"
    fi

    if ask_yes_no "Enable rate limiting?" "yes"; then
        CONFIG[rate_limit]="on"
        print_success "Rate limiting: enabled"
    else
        CONFIG[rate_limit]="off"
        print_warning "Rate limiting: disabled"
    fi

    if ask_yes_no "Enable TLS/SSL support?" "yes"; then
        CONFIG[tls]="on"
        print_success "TLS/SSL: enabled"
    else
        CONFIG[tls]="off"
        print_warning "TLS/SSL: disabled"
    fi
}

configure_components() {
    print_progress
    print_section "Build Components"

    print_help "Select which components to build:"

    if ask_yes_no "Build unit tests?" "yes"; then
        CONFIG[build_tests]="on"
        print_success "Tests: will be built"
    else
        CONFIG[build_tests]="off"
        print_warning "Tests: will not be built"
    fi

    if ask_yes_no "Build example programs?" "yes"; then
        CONFIG[build_examples]="on"
        print_success "Examples: will be built"
    else
        CONFIG[build_examples]="off"
        print_warning "Examples: will not be built"
    fi

    if ask_yes_no "Build benchmark programs?" "yes"; then
        CONFIG[build_benchmarks]="on"
        print_success "Benchmarks: will be built"
    else
        CONFIG[build_benchmarks]="off"
        print_warning "Benchmarks: will not be built"
    fi
}

configure_advanced() {
    print_progress
    print_section "Advanced Options"

    print_help "Configure advanced build options:"

    if ask_yes_no "Enable logging system?" "yes"; then
        CONFIG[logging]="on"
        print_success "Logging: enabled"
    else
        CONFIG[logging]="off"
        print_warning "Logging: disabled"
    fi

    local prefix
    prefix=$(read_input "Installation prefix" "${CONFIG[install_prefix]}")
    CONFIG[install_prefix]="$prefix"
    print_success "Install prefix: $prefix"
}

# =============================================================================
# Configuration Summary
# =============================================================================

print_summary() {
    print_progress
    print_section "Configuration Summary"

    echo ""
    echo -e "${COLOR_BOLD}${COLOR_CYAN}Build Configuration:${COLOR_RESET}"
    echo ""
    print_option "Build Type" "${CONFIG[build_type]}" "Optimization level"
    print_option "Allocator" "${CONFIG[allocator]}" "Memory allocation strategy"
    print_option "Architecture" "${CONFIG[arch]}" "Target architecture"
    print_option "Install Prefix" "${CONFIG[install_prefix]}" "Installation directory"

    echo ""
    echo -e "${COLOR_BOLD}${COLOR_CYAN}Feature Modules:${COLOR_RESET}"
    echo ""
    print_option "WebSocket" "${CONFIG[websocket]}" "WebSocket protocol support"
    print_option "Static Files" "${CONFIG[static_files]}" "Static file serving"
    print_option "Rate Limiting" "${CONFIG[rate_limit]}" "Request rate limiting"
    print_option "TLS/SSL" "${CONFIG[tls]}" "Encrypted connections"

    echo ""
    echo -e "${COLOR_BOLD}${COLOR_CYAN}Build Components:${COLOR_RESET}"
    echo ""
    print_option "Tests" "${CONFIG[build_tests]}" "Unit test suite"
    print_option "Examples" "${CONFIG[build_examples]}" "Example programs"
    print_option "Benchmarks" "${CONFIG[build_benchmarks]}" "Performance benchmarks"
    print_option "Logging" "${CONFIG[logging]}" "Debug logging system"

    echo ""
}

# =============================================================================
# Build Command Generation
# =============================================================================

generate_build_command() {
    local cmd="mkdir -p build && cd build && cmake"

    # Build type
    cmd="$cmd -DCMAKE_BUILD_TYPE=${CONFIG[build_type]}"

    # Allocator
    if [ "${CONFIG[allocator]}" = "mimalloc" ]; then
        cmd="$cmd -DUVHTTP_ALLOCATOR_TYPE=1 -DBUILD_WITH_MIMALLOC=ON"
    elif [ "${CONFIG[allocator]}" = "system" ]; then
        cmd="$cmd -DUVHTTP_ALLOCATOR_TYPE=0 -DBUILD_WITH_MIMALLOC=OFF"
    else
        cmd="$cmd -DUVHTTP_ALLOCATOR_TYPE=2"
    fi

    # Features
    cmd="$cmd -DBUILD_WITH_WEBSOCKET=${CONFIG[websocket]}"
    cmd="$cmd -DBUILD_WITH_STATIC_FILES=${CONFIG[static_files]}"
    cmd="$cmd -DBUILD_WITH_RATE_LIMIT=${CONFIG[rate_limit]}"
    cmd="$cmd -DBUILD_WITH_TLS=${CONFIG[tls]}"

    # Components
    cmd="$cmd -DBUILD_TESTS=${CONFIG[build_tests]}"
    cmd="$cmd -DBUILD_EXAMPLES=${CONFIG[build_examples]}"
    cmd="$cmd -DBUILD_BENCHMARKS=${CONFIG[build_benchmarks]}"

    # Advanced
    if [ "${CONFIG[logging]}" = "off" ]; then
        cmd="$cmd -DUVHTTP_FEATURE_LOGGING=OFF"
    fi

    # Install prefix
    cmd="$cmd -DCMAKE_INSTALL_PREFIX=${CONFIG[install_prefix]}"

    # 32-bit flag
    if [ "${CONFIG[arch]}" = "32-bit" ]; then
        cmd="$cmd -DCMAKE_C_FLAGS=-m32"
    fi

    # Build command
    cmd="$cmd .. && make -j$CORES"

    echo "$cmd"
}

# =============================================================================
# Main Function
# =============================================================================

main() {
    # Clear screen for clean start
    clear

    # Welcome banner
    print_header "UVHTTP Configuration Assistant $VERSION"

    echo -e "${COLOR_DIM}This tool will guide you through configuring UVHTTP for your needs.${COLOR_RESET}"
    echo -e "${COLOR_DIM}You can press Enter at any time to accept the default value.${COLOR_RESET}"
    echo ""

    # Run configuration steps
    detect_system
    configure_build_type
    configure_allocator
    configure_features
    configure_components
    configure_advanced
    print_summary

    # Confirm configuration
    print_separator
    if ! ask_yes_no "Use this configuration?" "yes"; then
        echo ""
        print_info "Configuration cancelled. Please run this script again."
        exit 0
    fi

    # Generate build command
    print_section "Build Command"

    BUILD_CMD=$(generate_build_command)

    echo ""
    echo -e "${COLOR_BOLD}${COLOR_CYAN}To build UVHTTP, run:${COLOR_RESET}"
    echo ""
    echo -e "${COLOR_BRIGHT_GREEN}$BUILD_CMD${COLOR_RESET}"
    echo ""

    # Option to run build now
    print_separator
    if ask_yes_no "Run build now?" "yes"; then
        print_info "Starting build..."
        echo ""

        if eval "$BUILD_CMD"; then
            print_separator
            print_header "Build Successful!"

            echo ""
            echo -e "${COLOR_SUCCESS}UVHTTP has been built successfully!${COLOR_RESET}"
            echo ""
            echo -e "${COLOR_BOLD}${COLOR_CYAN}Quick Start:${COLOR_RESET}"
            echo ""
            echo -e "  ${COLOR_DIM}cd build${COLOR_RESET}"
            echo -e "  ${COLOR_DIM}./dist/bin/hello_world${COLOR_RESET}"
            echo ""
            echo -e "${COLOR_BOLD}${COLOR_CYAN}Documentation:${COLOR_RESET}"
            echo ""
            echo -e "  ${COLOR_DIM}API Reference:  ../docs/api/${COLOR_RESET}"
            echo -e "  ${COLOR_DIM}User Guide:     ../docs/guide/${COLOR_RESET}"
            echo ""
            echo -e "${COLOR_BOLD}${COLOR_CYAN}Examples:${COLOR_RESET}"
            echo ""
            echo -e "  ${COLOR_DIM}cd ../examples${COLOR_RESET}"
            echo -e "  ${COLOR_DIM}make -f Makefile.examples${COLOR_RESET}"
            echo ""
            print_success "Configuration and build completed!"
        else
            print_error "Build failed. Please check the error messages above."
            exit 1
        fi
    else
        print_info "Build command generated. You can run it manually later."
        echo ""
        print_info "Save the command for later use:"
        echo ""
        echo -e "${COLOR_DIM}$BUILD_CMD${COLOR_RESET}"
        echo ""
    fi
}

# Run main function
main "$@"
