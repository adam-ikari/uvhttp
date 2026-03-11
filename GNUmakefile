# UVHTTP GNU Makefile
# This file provides convenient targets for common development tasks
# Use with: make -f GNUmakefile <target>

.PHONY: help docs docs-clean docs-preview clean-all install-deps test coverage build build-release build-coverage clean rebuild cmake cmake-options bench

# Default target
help:
	@echo "UVHTTP Development Makefile"
	@echo ""
	@echo "Build targets:"
	@echo "  make -f GNUmakefile cmake         - Run CMake configuration"
	@echo "  make -f GNUmakefile cmake-options - Show available CMake options"
	@echo "  make -f GNUmakefile build         - Build project (Debug mode)"
	@echo "  make -f GNUmakefile build-release - Build project (Release mode)"
	@echo "  make -f GNUmakefile bench         - Build benchmark (Release + mimalloc)"
	@echo "  make -f GNUmakefile build-coverage- Build with coverage enabled"
	@echo "  make -f GNUmakefile rebuild       - Clean and rebuild"
	@echo "  make -f GNUmakefile clean         - Clean build artifacts"
	@echo ""
	@echo "Test targets:"
	@echo "  make -f GNUmakefile test          - Run tests"
	@echo "  make -f GNUmakefile coverage      - Generate coverage report"
	@echo ""
	@echo "Documentation targets:"
	@echo "  make -f GNUmakefile docs          - Build all documentation"
	@echo "  make -f GNUmakefile docs-clean    - Clean generated documentation"
	@echo "  make -f GNUmakefile docs-preview  - Preview VitePress docs locally"
	@echo ""
	@echo "Utility targets:"
	@echo "  make -f GNUmakefile clean-all     - Clean all build artifacts"
	@echo "  make -f GNUmakefile install-deps  - Install development dependencies"
	@echo ""
	@echo "Quick start:"
	@echo "  make -f GNUmakefile build && make -f GNUmakefile test"
	@echo ""

# Build all documentation
docs:
	@echo "Building UVHTTP documentation..."
	@./scripts/build_docs.sh

# Clean generated documentation
docs-clean:
	@echo "Cleaning generated documentation..."
	@rm -rf docs/api/xml
	@rm -rf docs/api/markdown_from_xml
	@rm -rf docs/api/html
	@rm -rf docs/api/latex
	@rm -rf docs/.vitepress/dist
	@rm -rf docs/.vitepress/cache
	@echo "Documentation cleaned successfully"

# Preview VitePress documentation locally
docs-preview:
	@echo "Starting VitePress documentation server..."
	@cd docs && npm run dev

# Clean all build artifacts
clean-all:
	@echo "Cleaning all build artifacts..."
	@rm -rf build/
	@rm -rf build_debug/
	@rm -rf build_minimal_debug/
	@rm -rf build_release/
	@$(MAKE) -f GNUmakefile docs-clean
	@echo "All build artifacts cleaned"

# Run tests (requires CMake build)
test:
	@echo "Running tests..."
	@if [ -d build ]; then \
		cd build && ctest --output-on-failure; \
	else \
		echo "Error: build directory not found. Please run 'cmake -B build' first."; \
		exit 1; \
	fi

# Generate coverage report (requires coverage build)
coverage:
	@echo "Generating coverage report..."
	@if [ -d build ]; then \
		cd build && ctest && lcov --capture --directory . --output-file coverage.info && \
		genhtml coverage.info --output-directory coverage_html; \
	else \
		echo "Error: build directory not found. Please run cmake with coverage enabled first."; \
		exit 1; \
	fi

# Install development dependencies
install-deps:
	@echo "Installing development dependencies..."
	@echo "System dependencies:"
	@echo "  Ubuntu/Debian: sudo apt-get install doxygen graphviz"
	@echo "  macOS: brew install doxygen graphviz"
	@echo ""
	@echo "Node.js dependencies:"
	@cd docs && npm install
	@echo "Dependencies installed successfully"

# Build project with CMake (Debug mode)
build:
	@echo "Building UVHTTP in Debug mode..."
	@cmake -B build -DCMAKE_BUILD_TYPE=Debug || (echo "Error: cmake not found. Please install CMake." && exit 1)
	@cmake --build build -j$$(nproc)
	@echo "Build completed successfully"

# Build project with CMake (Release mode)
build-release:
	@echo "Building UVHTTP in Release mode..."
	@cmake -B build_release -DCMAKE_BUILD_TYPE=Release || (echo "Error: cmake not found. Please install CMake." && exit 1)
	@cmake --build build_release -j$$(nproc)
	@echo "Release build completed successfully"

# Build project with coverage enabled
build-coverage:
	@echo "Building UVHTTP with coverage enabled..."
	@cmake -B build_coverage -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON || (echo "Error: cmake not found. Please install CMake." && exit 1)
	@cmake --build build_coverage -j$$(nproc)
	@echo "Coverage build completed successfully"

# Build benchmark with Release mode and mimalloc allocator
bench:
	@echo "Building UVHTTP benchmark (Release + mimalloc)..."
	@echo "Note: This builds a separate build directory 'build_bench' with mimalloc enabled"
	@cmake -B build_bench -DCMAKE_BUILD_TYPE=Release -DUVHTTP_ALLOCATOR_TYPE=1 -DBUILD_BENCHMARKS=ON || (echo "Error: cmake not found. Please install CMake." && exit 1)
	@cmake --build build_bench -j$$(nproc)
	@echo "Benchmark build completed successfully"
	@echo ""
	@echo "Benchmark location: build_bench/dist/bin/"
	@echo "Run benchmark: ./build_bench/dist/bin/benchmark_unified <port>"

# Clean build artifacts (current build directory only)
clean:
	@echo "Cleaning build artifacts..."
	@if [ -d build ]; then \
		cmake --build build --target clean; \
	fi
	@echo "Build artifacts cleaned"

# Rebuild project
rebuild: clean build
	@echo "Rebuild completed"

# Run CMake configuration
cmake:
	@echo "Running CMake configuration..."
	@cmake -B build || (echo "Error: cmake not found. Please install CMake." && exit 1)
	@echo "CMake configuration completed"

# Show CMake options
cmake-options:
	@echo "Available CMake options:"
	@echo "  BUILD_WITH_WEBSOCKET=ON/OFF     - Enable WebSocket support (default: ON)"
	@echo "  BUILD_WITH_MIMALLOC=ON/OFF      - Use mimalloc allocator (default: OFF)"
	@echo "  UVHTTP_ALLOCATOR_TYPE=0/1/2     - 0=system, 1=mimalloc, 2=custom (default: 0)"
	@echo "  ENABLE_COVERAGE=ON/OFF           - Enable code coverage (default: OFF)"
	@echo "  BUILD_EXAMPLES=ON/OFF            - Build example programs (default: OFF)"
	@echo "  ENABLE_DEBUG=ON/OFF              - Enable debug mode (default: OFF)"
	@echo ""
	@echo "Example usage:"
	@echo "  cmake -B build -DBUILD_WITH_WEBSOCKET=ON -DUVHTTP_ALLOCATOR_TYPE=1"