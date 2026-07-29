# UVHTTP Makefile
# Direct cmake wrapper — no dependency on GNUmakefile
# Use with: make <target>

.PHONY: help build build-release build-coverage test bench verify-memory-safety coverage \
        check-syntax docs docs-clean docs-preview clean clean-all rebuild cmake cmake-options install-deps

help:
	@echo "UVHTTP Makefile"
	@echo ""
	@echo "Build targets:"
	@echo "  make build              - Debug build (default)"
	@echo "  make build-release      - Release build"
	@echo "  make build-coverage     - Build with coverage enabled"
	@echo "  make rebuild            - Clean and rebuild"
	@echo ""
	@echo "Test targets:"
	@echo "  make test               - Run tests"
	@echo "  make verify-memory-safety - ASan + UBSan gate"
	@echo "  make coverage           - Generate coverage report"
	@echo ""
	@echo "Quality targets:"
	@echo "  make check-syntax       - Syntax check all source files"
	@echo ""
	@echo "Documentation targets:"
	@echo "  make docs               - Build all documentation"
	@echo "  make docs-clean         - Clean generated documentation"
	@echo "  make docs-preview       - Preview VitePress docs locally"
	@echo ""
	@echo "Utility targets:"
	@echo "  make clean              - Clean build artifacts"
	@echo "  make clean-all          - Clean all build artifacts"
	@echo "  make cmake-options      - Show available CMake options"
	@echo "  make install-deps       - Install development dependencies"
	@echo ""
	@echo "Options:"
	@echo "  CMAKE_FLAGS=\"...\"  - Pass extra flags to cmake"
	@echo ""
	@echo "Quick start:"
	@echo "  make build && make test"

# ========== Build targets ==========

build:
	cmake -B build -DCMAKE_BUILD_TYPE=Debug $(CMAKE_FLAGS)
	cmake --build build -j$$(nproc)

build-release:
	cmake -B build_release -DCMAKE_BUILD_TYPE=Release $(CMAKE_FLAGS)
	cmake --build build_release -j$$(nproc)

build-coverage:
	cmake -B build_coverage -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON $(CMAKE_FLAGS)
	cmake --build build_coverage -j$$(nproc)

rebuild: clean build

cmake:
	cmake -B build $(CMAKE_FLAGS)

# ========== Test targets ==========

test:
	@if [ -d build ]; then \
		cd build && ctest --output-on-failure; \
	else \
		echo "Error: build directory not found. Run 'make build' first."; \
		exit 1; \
	fi

verify-memory-safety:
	@echo "==> Verifying memory safety (ASan + UBSan)..."
	@status=0; \
	echo "-- AddressSanitizer (leaks / use-after-free / overflows) --"; \
	cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON >/dev/null 2>&1 || { echo "ASan configure failed"; exit 1; }; \
	cmake --build build_asan -j$$(nproc) >/dev/null 2>&1 || { echo "ASan build failed"; exit 1; }; \
	( cd build_asan && ctest --output-on-failure ) || status=1; \
	echo ""; \
	echo "-- UndefinedBehaviorSanitizer (undefined behavior) --"; \
	cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON >/dev/null 2>&1 || { echo "UBSan configure failed"; exit 1; }; \
	cmake --build build_ubsan -j$$(nproc) >/dev/null 2>&1 || { echo "UBSan build failed"; exit 1; }; \
	( cd build_ubsan && ctest --output-on-failure ) || status=1; \
	echo ""; \
	if [ $$status -eq 0 ]; then \
		echo "==> PASS: full suite clean under ASan and UBSan (all tests)."; \
	else \
		echo "==> FAIL: sanitizer findings detected. See output above."; \
	fi; \
	exit $$status

coverage:
	@echo "Generating coverage report..."
	@if [ -d build_coverage ]; then \
		cd build_coverage && ctest && lcov --capture --directory . --output-file coverage.info && \
		genhtml coverage.info --output-directory coverage_html; \
	else \
		echo "Error: build_coverage directory not found. Run 'make build-coverage' first."; \
		exit 1; \
	fi

# ========== Quality targets ==========

check-syntax:
	@echo "Checking syntax of all source files..."
	@errors=0; \
	for f in src/uvhttp_*.c; do \
		result=$$(gcc -fsyntax-only -std=c99 -pthread -D_GNU_SOURCE \
			-Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
			-Ideps/mbedtls/include -Ideps/libuv/include \
			-DUVHTTP_FEATURE_TLS=1 -DBUILD_WITH_HTTPS=1 \
			$$f 2>&1); \
		if echo "$$result" | grep -q "error:"; then \
			echo "FAIL: $$f"; \
			echo "$$result" | grep "error:"; \
			errors=$$((errors + 1)); \
		else \
			echo "OK: $$f"; \
		fi; \
	done; \
	if [ $$errors -eq 0 ]; then \
		echo "All source files passed syntax check."; \
	else \
		echo "$$errors file(s) failed syntax check."; \
		exit 1; \
	fi

# ========== Documentation targets ==========

docs:
	@echo "Building UVHTTP documentation..."
	@./scripts/build_docs.sh

docs-clean:
	@echo "Cleaning generated documentation..."
	@rm -rf docs/api/xml docs/api/generated docs/api/html docs/api/latex docs/api/.doxygen
	@rm -rf docs/.vitepress/dist docs/.vitepress/cache
	@echo "Documentation cleaned successfully"

docs-preview:
	@echo "Starting VitePress documentation server..."
	@cd docs && npm run dev

# ========== Utility targets ==========

clean:
	@echo "Cleaning build artifacts..."
	@rm -rf build/ build_release/ build_asan/ build_ubsan/
	@echo "Build artifacts cleaned"

clean-all:
	@echo "Cleaning all build artifacts..."
	@rm -rf build/ build_debug/ build_minimal_debug/ build_release/ build_asan/ build_ubsan/ build_coverage/ build_bench/
	@echo "All build artifacts cleaned"

cmake-options:
	@echo "Available CMake options:"
	@echo "  BUILD_WITH_WEBSOCKET=ON/OFF     - Enable WebSocket support (default: ON)"
	@echo "  BUILD_WITH_MIMALLOC=ON/OFF      - Use mimalloc allocator (default: OFF)"
	@echo "  UVHTTP_ALLOCATOR_TYPE=0/1/2     - 0=system, 1=mimalloc, 2=custom (default: 0)"
	@echo "  ENABLE_COVERAGE=ON/OFF           - Enable code coverage (default: OFF)"
	@echo "  BUILD_EXAMPLES=ON/OFF            - Build example programs (default: OFF)"
	@echo "  ENABLE_DEBUG=ON/OFF              - Enable debug mode (default: OFF)"
	@echo ""
	@echo "Example: cmake -B build -DBUILD_WITH_WEBSOCKET=ON -DUVHTTP_ALLOCATOR_TYPE=1"

install-deps:
	@echo "Installing development dependencies..."
	@echo "System dependencies:"
	@echo "  Ubuntu/Debian: sudo apt-get install doxygen graphviz"
	@echo "  macOS: brew install doxygen graphviz"
	@echo ""
	@echo "Node.js dependencies:"
	@cd docs && npm install
	@echo "Dependencies installed successfully"

# ========== Benchmark ==========

bench:
	@echo "Building UVHTTP benchmark (Release)..."
	@cmake -B build_bench -DCMAKE_BUILD_TYPE=Release $(CMAKE_FLAGS) || (echo "Error: cmake not found. Please install CMake." && exit 1)
	@cmake --build build_bench -j$$(nproc)
	@echo "Benchmark build completed successfully"
	@echo ""
	@echo "Run benchmark: ./build_bench/dist/bin/test_performance_e2e <port>"