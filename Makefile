BUILD_DIR ?= build
BUILD_TYPE ?= Release
CMAKE_ARGS = -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON -DBUILD_WITH_TLS=ON

.PHONY: all clean test help cppcheck install coverage coverage-clean examples build build-deps rebuild docs-site docs-site-build docs-site-clean docs-site-dev

all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

build: build-deps all
	@echo "✅ 构建完成！"
	@echo "可执行文件位置: $(BUILD_DIR)/dist/bin/"

build-deps:
	@echo "🔨 检查并编译依赖..."
	@echo "  - 编译 libuv..."
	@if [ ! -f "deps/libuv/build/libuv.a" ]; then \
		cd deps/libuv && mkdir -p build && cd build && cmake .. -DBUILD_TESTING=OFF && make -j$$(nproc); \
	fi
	@echo "  - 编译 mbedtls..."
	@if [ ! -f "deps/mbedtls/build/library/libmbedtls.a" ]; then \
		cd deps/mbedtls && mkdir -p build && cd build && cmake .. && make -j$$(nproc); \
	fi
	@echo "  - 编译 llhttp..."
	@if [ ! -f "deps/cllhttp/libllhttp.a" ]; then \
		cd deps/cllhttp && gcc -c llhttp.c -o llhttp.o && ar rcs libllhttp.a llhttp.o; \
	fi
	@echo "  - 编译 xxhash..."
	@if [ ! -f "deps/xxhash/libxxhash.a" ]; then \
		cd deps/xxhash && make -j$$(nproc); \
	fi
	@echo "  - 编译 cjson..."
	@if [ ! -f "deps/cjson/build/libcjson.a" ]; then \
		cd deps/cjson && mkdir -p build && cd build && cmake .. && make -j$$(nproc); \
	fi
	@echo "  - 编译 mimalloc..."
	@if [ ! -f "deps/mimalloc/build/libmimalloc.a" ]; then \
		cd deps/mimalloc && mkdir -p build && cd build && cmake .. && make -j$$(nproc); \
	fi
	@echo "  - 编译 googletest..."
	@if [ ! -f "deps/googletest/build/lib/libgtest.a" ]; then \
		cd deps/googletest && mkdir -p build && cd build && cmake .. && make -j$$(nproc); \
	fi
	@echo "✅ 依赖编译完成！"

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake $(CMAKE_ARGS) ..

clean:
	@rm -rf $(BUILD_DIR)

test: all
	@echo "🧪 运行测试..."
	@cd $(BUILD_DIR) && ctest --output-on-failure

coverage: $(BUILD_DIR)/Makefile
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
	@find $(BUILD_DIR) -name "*.gcda" -delete 2>/dev/null || true
	@find $(BUILD_DIR) -name "*.gcno" -delete 2>/dev/null || true
	@find $(BUILD_DIR) -name "coverage.info" -delete 2>/dev/null || true
	@rm -rf $(BUILD_DIR)/coverage_html 2>/dev/null || true

install: all
	@$(MAKE) -C $(BUILD_DIR) install

build-mimalloc:
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DUVHTTP_ALLOCATOR=mimalloc ..
	@$(MAKE) -C $(BUILD_DIR)

cppcheck:
	@cppcheck --enable=warning --std=c11 src/ include/

examples: all
	@$(MAKE) -C $(BUILD_DIR) hello_world simple_routing method_routing cors_rate_limit_demo middleware_demo libuv_data_pointer

run-helloworld: examples
	@cd $(BUILD_DIR) && ./dist/bin/hello_world

run-simple-routing: examples
	@cd $(BUILD_DIR) && ./dist/bin/simple_routing

run-method-routing: examples
	@cd $(BUILD_DIR) && ./dist/bin/method_routing

help:
	@echo "UVHTTP 构建系统"
	@echo "  make                    - 构建项目"
	@echo "  make build              - 构建项目（包括依赖）"
	@echo "  make rebuild            - 完全重新构建"
	@echo "  make clean              - 清理构建"
	@echo "  make test               - 运行测试"
	@echo "  make coverage           - 生成覆盖率报告"
	@echo "  make coverage-clean     - 清理覆盖率数据"
	@echo "  make install            - 安装"
	@echo "  make examples           - 构建示例"
	@echo "  make cppcheck           - 代码检查"
	@echo ""
	@echo "文档网站:"
	@echo "  make docs-site          - 构建文档网站"
	@echo "  make docs-site-dev      - 启动开发服务器"
	@echo "  make docs-site-clean    - 清理文档网站"
	@echo "  BUILD_DIR=$(BUILD_DIR)  BUILD_TYPE=$(BUILD_TYPE)"

rebuild: clean build
	@echo "🔄 重新构建完成！"

# ============================================================================
# 文档网站
# ============================================================================

docs-site:
	@echo "📚 构建文档网站..."
	@cd docs-site && npm install
	@cd docs-site && npm run build
	@echo "✅ 文档网站构建完成！"
	@echo "  静态文件位置: docs-site/docs/.vitepress/dist/"

docs-site-build:
	@echo "📚 构建文档网站（开发模式）..."
	@cd docs-site && npm install
	@cd docs-site && npm run build
	@echo "✅ 文档网站构建完成！"

docs-site-clean:
	@echo "🧹 清理文档网站..."
	@cd docs-site && rm -rf node_modules docs/.vitepress/dist
	@echo "✅ 文档网站清理完成！"

docs-site-dev:
	@echo "🚀 启动文档网站开发服务器..."
	@cd docs-site && npm install
	@cd docs-site && npm run dev