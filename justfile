# UVHTTP Justfile - Modern Build System
# This file provides a simple, fast, and dependency-free way to build UVHTTP
# Usage: just <task>

# Default task
default: build

# ============================================================================
# Configuration Variables
# ============================================================================

# Project version
version := `grep "^VERSION=" VERSION | head -1 | cut -d'=' -f2`

# Build configuration
BUILD_DIR := "build"
BUILD_TYPE := "release"
JOBS := `nproc`
INSTALL_PREFIX := "/usr/local"

# Feature flags
FEATURE_WEBSOCKET := "ON"
FEATURE_STATIC_FILES := "ON"
FEATURE_RATE_LIMIT := "ON"
FEATURE_TLS := "ON"

# ============================================================================
# Build Tasks
# ============================================================================

# Build UVHTTP library and examples
build type="release":
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔨 Building UVHTTP in {{type}} mode..."
    mkdir -p {{BUILD_DIR}}
    cd {{BUILD_DIR}}
    cmake .. \
        -DCMAKE_BUILD_TYPE={{type}} \
        -DBUILD_WITH_WEBSOCKET={{FEATURE_WEBSOCKET}} \
        -DBUILD_WITH_STATIC_FILES={{FEATURE_STATIC_FILES}} \
        -DBUILD_WITH_RATE_LIMIT={{FEATURE_RATE_LIMIT}} \
        -DBUILD_WITH_TLS={{FEATURE_TLS}} \
        -DCMAKE_INSTALL_PREFIX={{INSTALL_PREFIX}}
    make -j{{JOBS}}
    echo "✅ Build complete!"

# Debug build with symbols
debug:
    just build type="debug"

# Release build with optimizations
release:
    just build type="release"

# ============================================================================
# Test Tasks
# ============================================================================

# Run all tests
test: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🧪 Running tests..."
    cd {{BUILD_DIR}}
    make test
    echo "✅ All tests passed!"

# Quick test (only basic functionality)
quick-test: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🚀 Running quick tests..."
    cd {{BUILD_DIR}}
    ./dist/bin/test_e2e_simple
    echo "✅ Quick tests passed!"

# Run specific test
run-test test_name: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔍 Running test: {{test_name}}..."
    cd {{BUILD_DIR}}
    ./dist/bin/{{test_name}}

# ============================================================================
# Clean Tasks
# ============================================================================

# Clean build artifacts
clean:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🧹 Cleaning build artifacts..."
    rm -rf {{BUILD_DIR}}
    rm -rf build_cov
    rm -rf build32
    rm -rf build_*
    echo "✅ Clean complete!"

# Clean all generated files
clean-all: clean
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🧹 Cleaning all generated files..."
    rm -rf coverage_html
    rm -f coverage.info
    rm -f *.gcov
    rm -rf deps/*/build
    echo "✅ All clean complete!"

# ============================================================================
# Installation Tasks
# ============================================================================

# Install UVHTTP system-wide
install: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "📦 Installing UVHTTP..."
    cd {{BUILD_DIR}}
    sudo make install
    echo "✅ Installation complete!"

# Uninstall UVHTTP
uninstall:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🗑️  Uninstalling UVHTTP..."
    cd {{BUILD_DIR}}
    sudo make uninstall
    echo "✅ Uninstallation complete!"

# ============================================================================
# Special Build Tasks
# ============================================================================

# Build for 32-bit architecture
build-32:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔨 Building for 32-bit architecture..."
    mkdir -p build32
    cd build32
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_FLAGS="-m32" \
        -DCMAKE_CXX_FLAGS="-m32"
    make -j{{JOBS}}
    echo "✅ 32-bit build complete!"

# Build with coverage instrumentation
build-coverage:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔨 Building with coverage..."
    mkdir -p build_cov
    cd build_cov
    cmake .. \
        -DCMAKE_BUILD_TYPE=Debug \
        -DENABLE_COVERAGE=ON \
        -DCMAKE_C_FLAGS="--coverage" \
        -DCMAKE_CXX_FLAGS="--coverage"
    make -j{{JOBS}}
    echo "✅ Coverage build complete!"

# ============================================================================
# Analysis and Quality Tasks
# ============================================================================

# Generate code coverage report
coverage: build-coverage
    #!/usr/bin/env bash
    set -euo pipefail
    echo "📊 Generating coverage report..."
    cd build_cov
    make test
    lcov --capture --directory . --output-file coverage.info
    lcov --remove coverage.info '/usr/*' 'deps/*' 'test/*' --output-file coverage.info
    genhtml coverage.info --output-directory coverage_html
    echo "✅ Coverage report generated in coverage_html/"

# Run static analysis
analyze:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔍 Running static analysis..."
    if command -v cppcheck &> /dev/null; then
        cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem src/ include/
    else
        echo "⚠️  cppcheck not found, skipping static analysis"
    fi
    echo "✅ Static analysis complete!"

# Format code
fmt:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🎨 Formatting code..."
    if command -v clang-format &> /dev/null; then
        find src include test -name '*.c' -o -name '*.h' | xargs clang-format -i
    else
        echo "⚠️  clang-format not found, skipping formatting"
    fi
    echo "✅ Code formatting complete!"

# Check code style
check-style:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔍 Checking code style..."
    if command -v clang-format &> /dev/null; then
        ! find src include test -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror
    else
        echo "⚠️  clang-format not found, skipping style check"
    fi
    echo "✅ Code style check passed!"

# ============================================================================
# Benchmark and Performance Tasks
# ============================================================================

# Run performance benchmarks
bench: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "📈 Running benchmarks..."
    cd {{BUILD_DIR}}
    make benchmark
    ./dist/bin/benchmark
    echo "✅ Benchmarks complete!"

# Run stress test
stress-test: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔥 Running stress test..."
    cd {{BUILD_DIR}}
    ./dist/bin/test_stress
    echo "✅ Stress test complete!"

# ============================================================================
# Memory and Debugging Tasks
# ============================================================================

# Run with valgrind memory checker
memcheck: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔍 Running memory check..."
    if command -v valgrind &> /dev/null; then
        cd {{BUILD_DIR}}
        valgrind --leak-check=full --show-leak-kinds=all ./dist/bin/test_e2e_simple
    else
        echo "⚠️  valgrind not found, skipping memory check"
    fi
    echo "✅ Memory check complete!"

# Run with gdb debugger
debugger: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔧 Running with gdb..."
    if command -v gdb &> /dev/null; then
        cd {{BUILD_DIR}}
        gdb ./dist/bin/test_e2e_simple
    else
        echo "⚠️  gdb not found, skipping debugger"
    fi

# ============================================================================
# Documentation Tasks
# ============================================================================

# Build documentation
docs:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "📚 Building documentation..."
    cd docs
    npm run build
    echo "✅ Documentation build complete!"

# Serve documentation locally
docs-serve:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🌐 Serving documentation at http://localhost:5173..."
    cd docs
    npm run dev

# ============================================================================
# CI/CD Tasks
# ============================================================================

# Full CI pipeline
ci: build test coverage
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🚀 Running CI pipeline..."
    echo "✅ CI pipeline passed!"

# Development workflow
dev: build test quick-test
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🚀 Development workflow complete!"

# Full release workflow
release-workflow: build test clean install
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🎉 Release workflow complete!"

# ============================================================================
# Utility Tasks
# ============================================================================

# Show help information
help:
    @just --list

# Show Just version
version:
    @just --version

# Show project version
show-version:
    @echo "UVHTTP version: {{version}}"

# Check system compatibility
check:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔍 Checking system compatibility..."
    
    # Check CMake
    if command -v cmake &> /dev/null; then
        echo "✅ CMake: $(cmake --version | head -1)"
    else
        echo "❌ CMake not found"
        exit 1
    fi
    
    # Check compiler
    if command -v gcc &> /dev/null; then
        echo "✅ GCC: $(gcc --version | head -1)"
    elif command -v clang &> /dev/null; then
        echo "✅ Clang: $(clang --version | head -1)"
    else
        echo "❌ No C compiler found"
        exit 1
    fi
    
    # Check Make
    if command -v make &> /dev/null; then
        echo "✅ Make: $(make --version | head -1)"
    else
        echo "❌ Make not found"
        exit 1
    fi
    
    echo "✅ All dependencies found!"

# ============================================================================
# Examples Tasks
# ============================================================================

# Build examples
build-examples: build
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔨 Building examples..."
    cd {{BUILD_DIR}}
    make examples
    echo "✅ Examples built successfully!"

# Run simple server example
run-simple: build-examples
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🌐 Running simple server example..."
    cd {{BUILD_DIR}}
    ./dist/bin/simple_server &
    echo "✅ Simple server running on http://127.0.0.1:8080"
    echo "Press Ctrl+C to stop"
    wait

# ============================================================================
# Dependencies Tasks
# ============================================================================

# Update submodules
update-submodules:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "🔄 Updating submodules..."
    git submodule update --init --recursive
    echo "✅ Submodules updated!"

# Show dependency information
show-deps:
    #!/usr/bin/env bash
    set -euo pipefail
    echo "📦 Dependencies:"
    echo "  - libuv: $(cd deps/libuv && git describe --tags --abbrev=0)"
    echo "  - llhttp: $(cd deps/llhttp && git describe --tags --abbrev=0)"
    echo "  - mbedtls: $(cd deps/mbedtls && git describe --tags --abbrev=0)"
    echo "  - mimalloc: $(cd deps/mimalloc && git describe --tags --abbrev=0)"
