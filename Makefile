BUILD_DIR ?= build
BUILD_TYPE ?= Release
CMAKE_ARGS = -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON -DBUILD_WITH_TLS=ON

.PHONY: all clean clean-all clean-build clean-deps clean-temp clean-coverage clean-performance test help cppcheck coverage coverage-clean examples build build-deps rebuild format format-check format-all format-diff docs docs-clean

all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

build: build-deps all
	@echo "✅ 构建完成！"
	@echo "可执行文件位置: $(BUILD_DIR)/dist/bin/"

build-deps:
	@echo "🔨 检查并编译依赖..."
	@$(MAKE) -C deps/libuv -j$$(nproc) 2>/dev/null || true
	@$(MAKE) -C deps/mbedtls -j$$(nproc) 2>/dev/null || true
	@cd deps/cllhttp && gcc -c llhttp.c -o llhttp.o && ar rcs libllhttp.a llhttp.o 2>/dev/null || true
	@$(MAKE) -C deps/xxhash -j$$(nproc) 2>/dev/null || true
	@cd deps/cjson && mkdir -p build && cd build && cmake .. && $(MAKE) -j$$(nproc) 2>/dev/null || true
	@$(MAKE) -C deps/mimalloc -j$$(nproc) 2>/dev/null || true
	@$(MAKE) -C deps/googletest/build -j$$(nproc) 2>/dev/null || true
	@echo "✅ 依赖编译完成！"

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake $(CMAKE_ARGS) ..

clean:
	@rm -rf $(BUILD_DIR)

clean-all:
	@echo "🧹 清理所有构建产物..."
	@./clean.sh --all

clean-build:
	@echo "🧹 清理构建目录..."
	@./clean.sh --build

clean-deps:
	@echo "🧹 清理依赖库构建产物..."
	@./clean.sh --deps

clean-temp:
	@echo "🧹 清理临时文件..."
	@find . -name "*.tmp" -o -name "*.temp" -o -name "*.log" -o -name "*.orig" -o -name "*.rej" -o -name "*.swp" -o -name "*.swo" -o -name "*~" -o -name ".DS_Store" -delete 2>/dev/null || true
	@echo "✅ 临时文件清理完成！"

clean-coverage:
	@echo "🧹 清理覆盖率文件..."
	@find . -name "*.gcov" -o -name "*.gcda" -o -name "*.gcno" -o -name "coverage.info" -delete 2>/dev/null || true
	@rm -rf coverage_html 2>/dev/null || true
	@echo "✅ 覆盖率文件清理完成！"

clean-performance:
	@echo "🧹 清理性能测试结果..."
	@rm -rf test/performance/results/* 2>/dev/null || true
	@find . -name "stress_test_results_*" -type d -exec rm -rf {} + 2>/dev/null || true
	@echo "✅ 性能测试结果清理完成！"

test: all
	@echo "🧪 运行测试..."
	@./test_runner.sh

coverage:
	@if ! command -v lcov >/dev/null 2>&1; then \
		echo "错误: lcov 未安装。请运行以下命令安装:"; \
		echo "  sudo apt-get install lcov"; \
		exit 1; \
	fi
	@if [ "$(BUILD_TYPE)" != "Debug" ]; then \
		rm -rf $(BUILD_DIR); \
	fi
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
	@$(MAKE) -C $(BUILD_DIR) -j$$(nproc)
	@echo "🧪 运行测试以生成覆盖率数据..."
	@cd $(BUILD_DIR) && for test in test_allocator test_async_file_full_coverage test_config_full_coverage test_connection_extended_coverage test_connection_full_coverage test_context_full_coverage test_context_simple test_cors_middleware_full_coverage test_deps_full_coverage test_error_coverage test_error_full_coverage test_error_handler_full_coverage test_error_helpers_full_coverage test_hash_full_coverage test_lru_cache_full_coverage test_mempool_full_coverage test_middleware_full_coverage test_network_full_coverage test_request_full_coverage test_request_null_coverage test_response_full_coverage test_router_full_coverage test_sendfile_timeout test_server_full_coverage test_static_coverage test_static_full_coverage test_tls_full_coverage test_tls_null_coverage test_utils_full_coverage test_validation_full_coverage test_websocket_full_coverage test_websocket_null_coverage test_whitelist_hash; do \
		./dist/bin/$$test > /dev/null 2>&1 || true; \
	done
	@echo "📊 生成覆盖率报告..."
	@cd $(BUILD_DIR) && lcov --capture --directory . --output-file coverage.info --base-directory ..
	@cd $(BUILD_DIR) && lcov --remove coverage.info '*/deps/*' --output-file coverage.info
	@cd $(BUILD_DIR) && lcov --remove coverage.info '*/test/*' --output-file coverage.info
	@cd $(BUILD_DIR) && lcov --list coverage.info
	@cd $(BUILD_DIR) && genhtml coverage.info --output-directory coverage_html --title "UVHTTP Code Coverage"
	@echo "✅ 覆盖率报告已生成: $(BUILD_DIR)/coverage_html/index.html"

coverage-clean:
	@find $(BUILD_DIR) -name "*.gcda" -o -name "*.gcno" -o -name "coverage.info" -delete 2>/dev/null || true
	@rm -rf $(BUILD_DIR)/coverage_html 2>/dev/null || true

cppcheck:
	@cppcheck --enable=warning --std=c11 --xml-version=2 --xml src/ include/ 2> cppcheck-results.xml || true

examples: all
	@$(MAKE) -C $(BUILD_DIR) hello_world simple_routing method_routing json_api_demo

run-helloworld: examples
	@cd $(BUILD_DIR) && ./dist/bin/hello_world

run-simple-routing: examples
	@cd $(BUILD_DIR) && ./dist/bin/simple_routing

run-method-routing: examples
	@cd $(BUILD_DIR) && ./dist/bin/method_routing

run-json-api: examples
	@cd $(BUILD_DIR) && ./dist/bin/json_api_demo

# ============================================================================
# 代码格式化
# ============================================================================

format-check:
	@echo "🔍 检查代码格式..."
	@if ! command -v clang-format >/dev/null 2>&1; then \
		echo "错误: clang-format 未安装。请运行以下命令安装:"; \
		echo "  sudo apt-get install clang-format"; \
		exit 1; \
	fi
	@clang-format --dry-run --Werror ./src/*.c ./include/*.h || \
		(echo "❌ 代码格式检查失败！请运行 'make format-all' 修复格式问题。"; exit 1)
	@echo "✅ 代码格式检查通过！"

format-all:
	@echo "🔧 修复代码格式..."
	@if ! command -v clang-format >/dev/null 2>&1; then \
		echo "错误: clang-format 未安装。请运行以下命令安装:"; \
		echo "  sudo apt-get install clang-format"; \
		exit 1; \
	fi
	@clang-format -i ./src/*.c ./include/*.h
	@echo "✅ 代码格式已修复！"

format-diff:
	@echo "📊 显示格式化差异..."
	@if ! command -v clang-format >/dev/null 2>&1; then \
		echo "错误: clang-format 未安装。请运行以下命令安装:"; \
		echo "  sudo apt-get install clang-format"; \
		exit 1; \
	fi
	@clang-format --dry-run --Werror ./src/*.c ./include/*.h || \
		(echo "❌ 代码格式检查失败！请运行 'make format-all' 修复格式问题。"; exit 1)
	@echo "✅ 代码格式检查通过！"

format: format-check

help:
	@echo "UVHTTP 构建系统"
	@echo ""
	@echo "构建命令:"
	@echo "  make                    - 构建项目"
	@echo "  make build              - 构建项目（包括依赖）"
	@echo "  make rebuild            - 完全重新构建"
	@echo "  make build-deps         - 仅构建依赖"
	@echo "  make examples           - 构建示例"
	@echo ""
	@echo "清理命令:"
	@echo "  make clean              - 清理构建目录 ($(BUILD_DIR))"
	@echo "  make clean-all          - 清理所有构建产物"
	@echo "  make clean-build        - 清理构建目录"
	@echo "  make clean-deps         - 清理依赖库构建产物"
	@echo "  make clean-temp         - 清理临时文件"
	@echo "  make clean-coverage     - 清理覆盖率文件"
	@echo "  make clean-performance  - 清理性能测试结果"
	@echo ""
	@echo "测试命令:"
	@echo "  make test               - 运行测试"
	@echo "  make coverage           - 生成覆盖率报告"
	@echo "  make coverage-clean     - 清理覆盖率数据"
	@echo ""
	@echo "代码检查:"
	@echo "  make cppcheck           - 代码静态检查"
	@echo ""
	@echo "代码格式化:"
	@echo "  make format             - 检查代码格式"
	@echo "  make format-check       - 检查代码格式"
	@echo "  make format-all         - 修复所有代码格式"
	@echo "  make format-diff        - 显示格式化差异"
	@echo ""
	@echo "文档生成:"
	@echo "  make docs               - 生成所有文档（HTML、LaTeX、XML、Markdown、网站）"
	@echo "  make docs-clean         - 清理所有文档"
	@echo ""
	@echo "运行示例:"
	@echo "  make run-helloworld     - 运行 Hello World 示例"
	@echo "  make run-simple-routing - 运行简单路由示例"
	@echo "  make run-method-routing - 运行方法路由示例"
	@echo "  make run-json-api       - 运行 JSON API 示例"
	@echo ""
	@echo "构建选项:"
	@echo "  BUILD_DIR=$(BUILD_DIR)  BUILD_TYPE=$(BUILD_TYPE)"

rebuild: clean build
	@echo "🔄 重新构建完成！"

# ============================================================================
# 文档生成
# ============================================================================

docs:
	@echo "📚 生成所有文档..."
	@if ! command -v doxygen >/dev/null 2>&1; then \
		echo "错误: doxygen 未安装。请运行以下命令安装:"; \
		echo "  sudo apt-get install doxygen graphviz"; \
		exit 1; \
	fi
	@mkdir -p docs/api
	@doxygen Doxyfile
	@python3 scripts/convert_xml_to_markdown.py docs/api/xml docs/api/markdown_from_xml
	@python3 scripts/update_api_sidebar.py
	@cd docs && npm install && npm run build
	@echo "✅ 所有文档生成完成！"
	@echo "  HTML: docs/api/html/index.html"
	@echo "  LaTeX: docs/api/latex/refman.pdf"
	@echo "  XML: docs/api/xml/index.xml"
	@echo "  Markdown: docs/api/markdown_from_xml/index.md"
	@echo "  网站: docs/.vitepress/dist/"

docs-clean:
	@echo "🧹 清理所有文档..."
	@rm -rf docs/api/html docs/api/latex docs/api/xml docs/api/markdown_from_xml
	@cd docs && rm -rf node_modules .vitepress/dist
	@echo "✅ 文档清理完成！"
