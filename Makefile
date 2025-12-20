# UVHTTP 超轻量级HTTP框架 Makefile

CC = gcc
CFLAGS = -std=c11 -Wall -O2
INCLUDES = -I include -I deps/libuv/include -I deps/mimalloc/include
LIBS = -L deps/libuv/.libs -luv -lpthread -lm
BUILDDIR = build

# 分配器选择 (default|pool|stats|mimalloc|custom)
ALLOCATOR ?= default

# 根据分配器类型设置编译标志
ifeq ($(ALLOCATOR),pool)
    CFLAGS += -DUVHTTP_DEFAULT_ALLOCATOR=UVHTTP_ALLOCATOR_POOL
    CFLAGS += -DUVHTTP_ENABLE_POOL_ALLOCATOR=1
else ifeq ($(ALLOCATOR),stats)
    CFLAGS += -DUVHTTP_DEFAULT_ALLOCATOR=UVHTTP_ALLOCATOR_STATS
    CFLAGS += -DUVHTTP_ENABLE_STATS_ALLOCATOR=1
else ifeq ($(ALLOCATOR),mimalloc)
    CFLAGS += -DUVHTTP_DEFAULT_ALLOCATOR=UVHTTP_ALLOCATOR_CUSTOM
    CFLAGS += -DUVHTTP_ENABLE_MIMALLOC=1
    LIBS += -L deps/mimalloc/build -lmimalloc
    $(info 使用mimalloc分配器)
else ifeq ($(ALLOCATOR),custom)
    CFLAGS += -DUVHTTP_DEFAULT_ALLOCATOR=UVHTTP_ALLOCATOR_CUSTOM
    $(info 使用自定义分配器)
else
    $(info 使用系统默认分配器)
endif

# 默认目标
.PHONY: all clean run help

all: $(BUILDDIR)/minimal

# 创建构建目录
$(BUILDDIR):
	mkdir -p $(BUILDDIR)

# 编译最小示例
$(BUILDDIR)/minimal: examples/minimal.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

# 编译简单示例
$(BUILDDIR)/simple: examples/simple_server.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

# 编译分配器示例
$(BUILDDIR)/allocator: examples/allocator_example.c | $(BUILDDIR)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $< $(LIBS)

# 运行最小示例
run: $(BUILDDIR)/minimal
	export LD_LIBRARY_PATH=deps/libuv/.libs:$$LD_LIBRARY_PATH && $(BUILDDIR)/minimal

# 运行简单示例
run-simple: $(BUILDDIR)/simple
	export LD_LIBRARY_PATH=deps/libuv/.libs:$$LD_LIBRARY_PATH && $(BUILDDIR)/simple

# 运行分配器示例
run-allocator: $(BUILDDIR)/allocator
	export LD_LIBRARY_PATH=deps/libuv/.libs:$$LD_LIBRARY_PATH && $(BUILDDIR)/allocator $(ARGS)

# 清理
clean:
	rm -rf $(BUILDDIR)

# 帮助
help:
	@echo "UVHTTP 超轻量级HTTP框架"
	@echo ""
	@echo "编译:"
	@echo "  make                    - 编译最小示例"
	@echo "  make simple            - 编译简单示例"
	@echo "  make allocator         - 编译分配器示例"
	@echo ""
	@echo "分配器选择:"
	@echo "  make ALLOCATOR=pool    - 使用内存池分配器"
	@echo "  make ALLOCATOR=stats   - 使用统计分配器"
	@echo "  make ALLOCATOR=mimalloc- 使用mimalloc分配器"
	@echo "  make ALLOCATOR=custom  - 使用自定义分配器"
	@echo ""
	@echo "运行:"
	@echo "  make run              - 运行最小示例"
	@echo "  make run-simple       - 运行简单示例"
	@echo "  make run-allocator    - 运行分配器示例"
	@echo ""
	@echo "其他:"
	@echo "  make clean    - 清理构建文件"
	@echo "  make help     - 显示帮助"