#!/bin/bash
###############################################################################
# UVHTTP Quick Start Script
# This script helps you get started with UVHTTP quickly and easily.
###############################################################################

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
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

# Check dependencies
check_dependencies() {
    print_info "Checking dependencies..."

    # Check for required tools
    local missing_deps=()

    if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
        missing_deps+=("gcc or clang")
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

    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "Missing required dependencies:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo ""
        echo "Please install the missing dependencies and try again."
        echo ""
        echo "Ubuntu/Debian:"
        echo "  sudo apt-get install build-essential cmake git"
        echo ""
        echo "Fedora/RHEL:"
        echo "  sudo dnf install gcc cmake make git"
        echo ""
        echo "macOS:"
        echo "  brew install cmake git"
        exit 1
    fi

    print_success "All dependencies found"
}

# Detect system architecture
detect_architecture() {
    ARCH=$(uname -m)
    print_info "Detected architecture: $ARCH"

    if [ "$ARCH" = "x86_64" ]; then
        BUILD_ARCH="64-bit"
    elif [ "$ARCH" = "i686" ] || [ "$ARCH" = "i386" ]; then
        BUILD_ARCH="32-bit"
        CMAKE_FLAGS="-DCMAKE_C_FLAGS=-m32"
    else
        print_warning "Architecture $ARCH may not be fully supported"
        BUILD_ARCH="unknown"
    fi
}

# Setup build directory
setup_build() {
    print_info "Setting up build directory..."

    if [ -d "build" ]; then
        print_warning "Build directory already exists. Cleaning..."
        rm -rf build
    fi

    mkdir -p build
    cd build

    print_success "Build directory created"
}

# Configure project
configure_project() {
    print_info "Configuring project with CMake..."

    local cmake_cmd="cmake .."

    # Add optional flags
    if [ -n "$CMAKE_FLAGS" ]; then
        cmake_cmd="$cmake_cmd $CMAKE_FLAGS"
    fi

    # Check for mimalloc availability
    if [ "$BUILD_ARCH" = "64-bit" ]; then
        cmake_cmd="$cmake_cmd -DBUILD_WITH_MIMALLOC=ON"
        print_info "Enabling mimalloc for improved performance"
    fi

    # Add user-specified flags
    if [ -n "$CMAKE_USER_FLAGS" ]; then
        cmake_cmd="$cmake_cmd $CMAKE_USER_FLAGS"
    fi

    print_info "Running: $cmake_cmd"
    eval $cmake_cmd

    if [ $? -ne 0 ]; then
        print_error "CMake configuration failed"
        exit 1
    fi

    print_success "Project configured successfully"
}

# Build project
build_project() {
    print_info "Building UVHTTP..."

    local num_cores=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    print_info "Using $num_cores cores for compilation"

    make -j$num_cores

    if [ $? -ne 0 ]; then
        print_error "Build failed"
        exit 1
    fi

    print_success "Build completed successfully"
}

# Run tests
run_tests() {
    if [ "$RUN_TESTS" = "true" ]; then
        print_info "Running tests..."

        make test

        if [ $? -ne 0 ]; then
            print_warning "Some tests failed, but build was successful"
        else
            print_success "All tests passed"
        fi
    else
        print_info "Skipping tests (use --test to run tests)"
    fi
}

# Print summary
print_summary() {
    echo ""
    echo "======================================================================"
    print_success "UVHTTP setup completed successfully!"
    echo "======================================================================"
    echo ""
    echo "Build information:"
    echo "  Architecture: $BUILD_ARCH"
    echo "  Build type:   Release"
    echo "  Location:     $(pwd)"
    echo ""
    echo "Quick start:"
    echo "  # Run hello world example"
    echo "  ./dist/bin/hello_world"
    echo ""
    echo "  # Run performance test server"
    echo "  ./dist/bin/performance_static_server -d ../public -p 8080"
    echo ""
    echo "  # View all examples"
    echo "  ls ../examples/"
    echo ""
    echo "Documentation:"
    echo "  API Reference:  ../docs/api/"
    echo "  User Guide:     ../docs/guide/"
    echo "  Performance:    ../docs/performance.md"
    echo ""
    echo "Next steps:"
    echo "  1. Explore examples in ../examples/"
    echo "  2. Read the getting started guide: ../docs/guide/getting-started.md"
    echo "  3. Check out the API documentation: ../docs/api/"
    echo ""
    echo "Need help?"
    echo "  GitHub Issues: https://github.com/adam-ikari/uvhttp/issues"
    echo "  Discussions:   https://github.com/adam-ikari/uvhttp/discussions"
    echo ""
}

# Main function
main() {
    echo ""
    echo "======================================================================"
    echo "                     UVHTTP Quick Start Script"
    echo "======================================================================"
    echo ""

    # Parse command line arguments
    while [ $# -gt 0 ]; do
        case $1 in
            --help|-h)
                echo "Usage: $0 [OPTIONS]"
                echo ""
                echo "Options:"
                echo "  --test, -t    Run tests after build"
                echo "  --debug, -d   Build with debug symbols"
                echo "  --help, -h    Show this help message"
                echo ""
                echo "Examples:"
                echo "  $0                    # Quick build"
                echo "  $0 --test            # Build and run tests"
                echo "  $0 --debug           # Debug build"
                echo ""
                exit 0
                ;;
            --test|-t)
                RUN_TESTS="true"
                shift
                ;;
            --debug|-d)
                CMAKE_USER_FLAGS="-DCMAKE_BUILD_TYPE=Debug"
                shift
                ;;
            *)
                print_error "Unknown option: $1"
                echo "Use --help for usage information"
                exit 1
                ;;
        esac
    done

    # Execute setup steps
    check_dependencies
    detect_architecture
    setup_build
    configure_project
    build_project
    run_tests

    # Print summary
    cd ..
    print_summary
}

# Run main function
main "$@"