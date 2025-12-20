# UVHTTP 项目 Makefile

BUILD_DIR ?= build
BUILD_TYPE ?= Release

CMAKE_ARGS = -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

.PHONY: all clean test help cppcheck install

# 默认目标
all: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR)

# 创建CMake配置
$(BUILD_DIR)/Makefile:
	@echo "配置CMake构建..."
	@mkdir -p $(BUILD_DIR)
	@cd $(BUILD_DIR) && cmake $(CMAKE_ARGS) ..

# 清理
clean:
	@rm -rf $(BUILD_DIR)
	@echo "清理完成"

# 运行所有测试
test: $(BUILD_DIR)/Makefile
	@cd $(BUILD_DIR) && ./uvhttp_unit_tests && ./uvhttp_test && ./test_websocket_basic
	@echo "所有测试完成"

# 安装
install: $(BUILD_DIR)/Makefile
	@$(MAKE) -C $(BUILD_DIR) install
	@echo "安装完成"

# 代码检查
cppcheck:
	@cppcheck --enable=warning --std=c11 src/ include/

# 帮助
help:
	@echo "UVHTTP HTTP框架构建系统"
	@echo ""
	@echo "基本用法:"
	@echo "  make                    - 构建项目"
	@echo "  make clean              - 清理构建文件"
	@echo "  make test               - 运行所有测试"
	@echo ""
	@echo "代码质量:"
	@echo "  make cppcheck          - 运行代码检查"
	@echo ""
	@echo "其他:"
	@echo "  make install           - 安装库和头文件"
	@echo ""
	@echo "参数:"
	@echo "  BUILD_DIR  - 构建目录（默认：build）"
	@echo "  BUILD_TYPE - 构建类型（默认：Release）"