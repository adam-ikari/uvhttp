# UVHTTP Justfile 快速参考指南

## 安装 Just

### 自动安装
```bash
./install_just.sh
```

### 手动安装
```bash
# 使用 curl
curl -fsSL https://just.systems/install.sh | bash

# 使用 wget
wget -qO - https://just.systems/install.sh | bash

# 使用 cargo（需要 Rust）
cargo install just
```

## 基本使用

### 查看所有可用任务
```bash
just --list
# 或
just help
```

### 常用任务

#### 构建任务
```bash
just              # 默认构建（release）
just build        # 显式构建
just debug        # 调试构建
just release      # 发布构建
just build-32     # 32位构建
```

#### 测试任务
```bash
just test         # 运行所有测试
just quick-test   # 快速测试
just run-test test_name  # 运行特定测试
```

#### 清理任务
```bash
just clean        # 清理构建产物
just clean-all    # 清理所有生成文件
```

#### 安装任务
```bash
just install      # 安装系统-wide
just uninstall    # 卸载
```

#### 分析和调试
```bash
just coverage     # 生成覆盖率报告
just analyze      # 静态分析
just memcheck     # 内存检查
just fmt          # 格式化代码
```

#### 性能测试
```bash
just bench        # 性能基准测试
just stress-test  # 压力测试
```

#### 开发工作流
```bash
just dev          # 开发工作流（构建+测试）
just ci           # CI/CD 流程
just release-workflow  # 完整发布流程
```

## 高级用法

### 带参数执行
```bash
just build type="debug" jobs=8
just test
```

### 查看任务内容
```bash
just --show build
```

### 选择执行
```bash
just choose  # 如果有多个同名任务
```

## 任务依赖关系

### 主要依赖链
```
build → test → clean → install
  ↓      ↓      ↓       ↓
debug  quick-test clean-all uninstall
```

### CI/CD 流程
```
ci: build → test → coverage
```

### 开发流程
```
dev: build → test → quick-test
```

## 配置变量

### 默认配置
```makefile
BUILD_DIR = "build"
BUILD_TYPE = "release"
JOBS = (nproc)
INSTALL_PREFIX = "/usr/local"
```

### 功能标志
```makefile
FEATURE_WEBSOCKET = "ON"
FEATURE_STATIC_FILES = "ON"
FEATURE_RATE_LIMIT = "ON"
FEATURE_TLS = "ON"
```

## 与 Python 工具对比

| 功能 | Python | Just | 改进 |
|------|--------|------|------|
| 启动速度 | 0.1s | 0.001s | 快100倍 |
| 依赖 | Python 3.6+ | 无 | 零依赖 |
| 代码量 | 573行 | 409行 | 减少29% |
| 文件数 | 多个 | 1个 | 简化 |
| 分发复杂度 | 高 | 低 | 单文件 |

## 迁移指南

### Python 命令 → Just 命令

| Python 命令 | Just 命令 |
|-------------|-----------|
| `./quickstart.py` | `just` |
| `./quickstart.py --test` | `just test` |
| `./quickstart.py --debug` | `just debug` |
| `./configure.py --check` | `just check` |

### 完全兼容
Python 工具仍然可用，Just 提供了更快的替代方案。

## 故障排除

### Just 命令未找到
```bash
# 检查 PATH
which just

# 添加到 PATH
export PATH="$HOME/.local/bin:$PATH"

# 重新登录或 source 配置文件
source ~/.bashrc
```

### 权限错误
```bash
chmod +x justfile
```

### 版本冲突
```bash
# 重新安装 Just
./install_just.sh
```

## 示例工作流

### 新项目设置
```bash
# 1. 安装 Just
./install_just.sh

# 2. 检查系统兼容性
just check

# 3. 构建项目
just build

# 4. 运行测试
just test
```

### 日常开发
```bash
# 快速构建和测试
just dev

# 调试问题
just debug
just memcheck

# 代码质量检查
just analyze
just fmt
```

### 发布准备
```bash
# 完整发布流程
just release-workflow

# 生成覆盖率报告
just coverage

# 性能测试
just bench
```

## 性能对比

### 构建时间
- **Python**: 初始化 0.1s + 执行时间
- **Just**: 初始化 0.001s + 执行时间
- **节省**: ~0.1s 每次

### 内存使用
- **Python**: ~30MB (解释器)
- **Just**: ~5MB (静态二进制)
- **节省**: 83%

## 最佳实践

1. **使用 Just 进行日常构建** - 更快更简单
2. **保留 Python 工具作为后备** - 兼容性考虑
3. **使用参数化任务** - 灵活性
4. **利用任务依赖** - 自动化工作流
5. **定期更新 Just** - 获得新特性

## 相关资源

- **Just 官方文档**: https://just.systems/
- **Just GitHub**: https://github.com/casey/just
- **Just 语法参考**: https://just.systems/man/en/chapter_3.html

## 贡献

如果要添加新任务到 justfile，请遵循以下模式：

```makefile
# 任务描述
task-name: dependency1 dependency2
    #!/usr/bin/env bash
    set -euo pipefail
    echo "Task description..."
    # 任务实现
    echo "✅ Task complete!"
```

## 支持

遇到问题？检查：
1. Just 是否正确安装
2. justfile 语法是否正确
3. 依赖是否满足
4. 权限是否正确

## 版本信息

- **Just 版本**: 1.47.0
- **UVHTTP 版本**: 2.5.0
- **更新日期**: 2026-03-15