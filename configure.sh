#!/bin/bash
###############################################################################
# UVHTTP Configuration Assistant
#
# Interactive script to help users configure UVHTTP build options
###############################################################################

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

# Print functions
print_header() {
    echo ""
    echo "========================================================================"
    echo "  $1"
    echo "========================================================================"
    echo ""
}

print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

print_error() {
    echo -e "${RED}[✗]${NC} $1"
}

# Configuration variables
BUILD_TYPE="Release"
ALLOCATOR_TYPE="1"
BUILD_WITH_WEBSOCKET="ON"
BUILD_WITH_STATIC_FILES="ON"
BUILD_WITH_RATE_LIMIT="ON"
BUILD_WITH_TLS="ON"
BUILD_TESTS="ON"
BUILD_EXAMPLES="ON"
BUILD_BENCHMARKS="ON"
ENABLE_LOGGING="ON"
USE_MIMALLOC="ON"
BUILD_32BIT="OFF"

# Ask user a yes/no question
ask_yes_no() {
    local prompt="$1"
    local default="$2"
    local response

    if [ "$default" = "yes" ]; then
        prompt="$prompt [Y/n]: "
    else
        prompt="$prompt [y/N]: "
    fi

    while true; do
        read -p "$prompt" response
        response=${response:-$default}

        case $response in
            [Yy]|[Yy][Ee][Ss])
                return 0
                ;;
            [Nn]|[Nn][Oo])
                return 1
                ;;
            *)
                print_warning "Please answer yes or no"
                ;;
        esac
    done
}

# Ask user to select an option
ask_option() {
    local prompt="$1"
    shift
    local options=("$@")
    local default="$1"

    echo ""
    print_info "$prompt"
    for i in "${!options[@]}"; do
        if [ "${options[$i]}" = "$default" ]; then
            echo "  $((i+1)). ${options[$i]} (default)"
        else
            echo "  $((i+1)). ${options[$i]}"
        fi
    done

    while true; do
        read -p "Select option [1-${#options[@]}]: " choice
        choice=${choice:-1}

        if [[ "$choice" =~ ^[0-9]+$ ]] && [ "$choice" -ge 1 ] && [ "$choice" -le "${#options[@]}" ]; then
            echo "${options[$((choice-1))]}"
            return 0
        else
            print_warning "Please enter a number between 1 and ${#options[@]}"
        fi
    done
}

# Main configuration
main() {
    clear
    print_header "UVHTTP Configuration Assistant"
    echo "This script will help you configure UVHTTP build options."
    echo "You can press Enter to accept the default value."
    echo ""

    # Build type
    BUILD_TYPE=$(ask_option "Select build type:" "Release" "Debug" "RelWithDebInfo" "MinSizeRel")
    print_success "Build type: $BUILD_TYPE"

    # Memory allocator
    print_info ""
    print_info "Memory allocator selection:"
    print_info "  1. System allocator (malloc/free) - Standard, works everywhere"
    print_info "  2. mimalloc - High performance, recommended for production"
    print_info "  3. Custom - Requires your own allocator implementation"

    if ask_yes_no "Use mimalloc for better performance?" "yes"; then
        ALLOCATOR_TYPE="1"
        USE_MIMALLOC="ON"
        print_success "Using mimalloc allocator"
    else
        ALLOCATOR_TYPE="0"
        USE_MIMALLOC="OFF"
        print_success "Using system allocator"
    fi

    # 32-bit build
    print_info ""
    print_info "32-bit build is for embedded systems with limited resources."
    print_info "Most users should use 64-bit for better performance."

    if ask_yes_no "Build for 32-bit architecture (embedded systems)?" "no"; then
        BUILD_32BIT="ON"
        print_success "32-bit build enabled"
    else
        BUILD_32BIT="OFF"
        print_success "64-bit build (default)"
    fi

    # Feature modules
    print_header "Feature Modules"

    if ask_yes_no "Enable WebSocket support?" "yes"; then
        BUILD_WITH_WEBSOCKET="ON"
        print_success "WebSocket enabled"
    else
        BUILD_WITH_WEBSOCKET="OFF"
        print_warning "WebSocket disabled"
    fi

    if ask_yes_no "Enable static file serving?" "yes"; then
        BUILD_WITH_STATIC_FILES="ON"
        print_success "Static file serving enabled"
    else
        BUILD_WITH_STATIC_FILES="OFF"
        print_warning "Static file serving disabled"
    fi

    if ask_yes_no "Enable rate limiting?" "yes"; then
        BUILD_WITH_RATE_LIMIT="ON"
        print_success "Rate limiting enabled"
    else
        BUILD_WITH_RATE_LIMIT="OFF"
        print_warning "Rate limiting disabled"
    fi

    if ask_yes_no "Enable TLS/SSL support?" "yes"; then
        BUILD_WITH_TLS="ON"
        print_success "TLS/SSL enabled"
    else
        BUILD_WITH_TLS="OFF"
        print_warning "TLS/SSL disabled"
    fi

    # Additional components
    print_header "Additional Components"

    if ask_yes_no "Build tests?" "yes"; then
        BUILD_TESTS="ON"
        print_success "Tests will be built"
    else
        BUILD_TESTS="OFF"
        print_warning "Tests will not be built"
    fi

    if ask_yes_no "Build examples?" "yes"; then
        BUILD_EXAMPLES="ON"
        print_success "Examples will be built"
    else
        BUILD_EXAMPLES="OFF"
        print_warning "Examples will not be built"
    fi

    if ask_yes_no "Build benchmarks?" "yes"; then
        BUILD_BENCHMARKS="ON"
        print_success "Benchmarks will be built"
    else
        BUILD_BENCHMARKS="OFF"
        print_warning "Benchmarks will not be built"
    fi

    if ask_yes_no "Enable logging system?" "yes"; then
        ENABLE_LOGGING="ON"
        print_success "Logging enabled"
    else
        ENABLE_LOGGING="OFF"
        print_warning "Logging disabled"
    fi

    # Show configuration summary
    print_header "Configuration Summary"

    echo "Build Type:          $BUILD_TYPE"
    echo "Allocator:           $ALLOCATOR_TYPE ($USE_MIMALLOC)"
    echo "Architecture:        $([ "$BUILD_32BIT" = "ON" ] && echo "32-bit" || echo "64-bit")"
    echo ""
    echo "Features:"
    echo "  WebSocket:          $BUILD_WITH_WEBSOCKET"
    echo "  Static Files:       $BUILD_WITH_STATIC_FILES"
    echo "  Rate Limit:         $BUILD_WITH_RATE_LIMIT"
    echo "  TLS/SSL:            $BUILD_WITH_TLS"
    echo ""
    echo "Components:"
    echo "  Tests:              $BUILD_TESTS"
    echo "  Examples:           $BUILD_EXAMPLES"
    echo "  Benchmarks:         $BUILD_BENCHMARKS"
    echo "  Logging:            $ENABLE_LOGGING"
    echo ""

    # Confirm configuration
    if ! ask_yes_no "Use this configuration?" "yes"; then
        print_info "Configuration cancelled. Please run this script again."
        exit 0
    fi

    # Generate build command
    print_header "Build Command"

    BUILD_CMD="mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DUVHTTP_ALLOCATOR_TYPE=$ALLOCATOR_TYPE -DBUILD_WITH_MIMALLOC=$USE_MIMALLOC -DBUILD_WITH_WEBSOCKET=$BUILD_WITH_WEBSOCKET -DBUILD_WITH_STATIC_FILES=$BUILD_WITH_STATIC_FILES -DBUILD_WITH_RATE_LIMIT=$BUILD_WITH_RATE_LIMIT -DBUILD_WITH_TLS=$BUILD_WITH_TLS .."

    if [ "$BUILD_32BIT" = "ON" ]; then
        BUILD_CMD="$BUILD_CMD -DCMAKE_C_FLAGS=-m32"
    fi

    BUILD_CMD="$BUILD_CMD && make -j\$(nproc)"

    echo ""
    echo "Run the following command to build UVHTTP:"
    echo ""
    echo -e "${CYAN}$BUILD_CMD${NC}"
    echo ""

    # Option to run build now
    if ask_yes_no "Run build now?" "yes"; then
        print_info "Starting build..."
        eval $BUILD_CMD

        if [ $? -eq 0 ]; then
            print_success "Build completed successfully!"
            echo ""
            echo "UVHTTP is now ready to use!"
            echo ""
            echo "Quick start:"
            echo "  cd build"
            echo "  ./dist/bin/hello_world"
            echo ""
        else
            print_error "Build failed. Please check the error messages above."
            exit 1
        fi
    else
        print_info "Build command generated. You can run it manually later."
        echo ""
        echo "To build UVHTTP later, run:"
        echo ""
        echo -e "${CYAN}$BUILD_CMD${NC}"
        echo ""
    fi

    print_success "Configuration completed!"
}

# Run main function
main