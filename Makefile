BUILD_DIR ?= build
BUILD_TYPE ?= Release
CMAKE_ARGS = -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

.PHONY: all clean test help cppcheck install coverage coverage-clean examples

all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

$(BUILD_DIR)/Makefile:
	@mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake $(CMAKE_ARGS) ..

clean:
	@rm -rf $(BUILD_DIR)

test: all
	@cd $(BUILD_DIR)/dist/test && ./uvhttp_test

coverage: $(BUILD_DIR)/Makefile
	@if ! command -v lcov >/dev/null 2>&1; then \
		echo "错误: lcov 未安装。请运行以下命令安装:"; \
		echo "  sudo apt-get install lcov"; \
		exit 1; \
	fi
	@if [ "$(BUILD_TYPE)" != "Debug" ]; then \
		rm -rf $(BUILD_DIR); \
		mkdir -p $(BUILD_DIR); \
		cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..; \
	fi
	@$(MAKE) -C $(BUILD_DIR) all
	@find $(BUILD_DIR) -name "*.gcda" -delete 2>/dev/null || true
	@cd $(BUILD_DIR)/dist/test && ./uvhttp_test > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_lru_cache_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_basic_functionality > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_utils_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_error_hash_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_response_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_hash_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_response_send_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_context_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_error_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_config_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_error_helpers_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR)/dist/test && ./test_error_handler_coverage > /dev/null 2>&1 || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_utils_coverage.dir --output-file coverage_utils.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_error_hash_coverage.dir --output-file coverage_error_hash.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_hash_coverage.dir --output-file coverage_hash.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_response_send_coverage.dir --output-file coverage_response_send.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_context_coverage.dir --output-file coverage_context.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_lru_cache_coverage.dir --output-file coverage_lru.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_basic_functionality.dir --output-file coverage_basic.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_response_coverage.dir --output-file coverage_response.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_error_coverage.dir --output-file coverage_error.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_config_coverage.dir --output-file coverage_config.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_error_helpers_coverage.dir --output-file coverage_error_helpers.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/test_error_handler_coverage.dir --output-file coverage_error_handler.info --base-directory .. || true
	@cd $(BUILD_DIR) && lcov --capture --directory CMakeFiles/uvhttp_test.dir --output-file coverage_uvhttp_test.info --base-directory .. || true
	@cd $(BUILD_DIR) && cat coverage_utils.info coverage_lru.info coverage_response.info coverage_error.info coverage_config.info coverage_error_helpers.info coverage_error_handler.info coverage_hash.info coverage_response_send.info coverage_context.info > coverage_combined.info 2>/dev/null || cp coverage_utils.info coverage_combined.info
	@cd $(BUILD_DIR) && lcov --remove coverage_combined.info '*/deps/*' --output-file coverage.info
	@cd $(BUILD_DIR) && lcov --remove coverage.info '*/test/*' --output-file coverage.info
	@cd $(BUILD_DIR) && lcov --list coverage.info
	@cd $(BUILD_DIR) && genhtml coverage.info --output-directory coverage_html --title "UVHTTP Code Coverage"
	@echo "覆盖率报告已生成: $(BUILD_DIR)/coverage_html/index.html"

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
	@$(MAKE) -C $(BUILD_DIR) helloworld simple_test static_file_server advanced_static_server config_demo simple_config cache_test_server simple_api_demo ultra_simple_demo json_api_demo

run-simple-config: examples
	@cd $(BUILD_DIR) && ./examples/simple_config

run-config-demo: examples
	@cd $(BUILD_DIR) && ./examples/config_demo

run-helloworld: examples
	@cd $(BUILD_DIR) && ./examples/helloworld

run-advanced-static: examples
	@cd $(BUILD_DIR) && ./examples/advanced_static_server

run-json-api: examples
	@cd $(BUILD_DIR) && ./examples/json_api_demo

help:
	@echo "UVHTTP 构建系统"
	@echo "  make                    - 构建项目"
	@echo "  make clean              - 清理构建"
	@echo "  make test               - 运行测试"
	@echo "  make coverage           - 生成覆盖率报告"
	@echo "  make coverage-clean     - 清理覆盖率数据"
	@echo "  make install            - 安装"
	@echo "  make examples           - 构建示例"
	@echo "  make cppcheck           - 代码检查"
	@echo "  BUILD_DIR=$(BUILD_DIR)  BUILD_TYPE=$(BUILD_TYPE)"