# UVHTTP GNU Makefile
# This file provides convenient targets for common development tasks
# Use with: make -f GNUmakefile <target>

.PHONY: help docs docs-clean docs-preview clean-all install-deps test coverage

# Default target
help:
	@echo "UVHTTP Development Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  make -f GNUmakefile help          - Show this help message"
	@echo "  make -f GNUmakefile docs          - Build all documentation"
	@echo "  make -f GNUmakefile docs-clean    - Clean generated documentation"
	@echo "  make -f GNUmakefile docs-preview  - Preview VitePress docs locally"
	@echo "  make -f GNUmakefile clean-all     - Clean all build artifacts"
	@echo "  make -f GNUmakefile test          - Run tests (requires CMake build)"
	@echo "  make -f GNUmakefile coverage      - Generate coverage report"
	@echo ""
	@echo "Documentation workflow:"
	@echo "  1. Write code with Doxygen comments"
	@echo "  2. Run: make -f GNUmakefile docs"
	@echo "  3. Preview: make -f GNUmakefile docs-preview"
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
	@echo "  Ubuntu/Debian: sudo apt-get install doxygen graphiz"
	@echo "  macOS: brew install doxygen graphviz"
	@echo ""
	@echo "Node.js dependencies:"
	@cd docs && npm install
	@echo "Dependencies installed successfully"