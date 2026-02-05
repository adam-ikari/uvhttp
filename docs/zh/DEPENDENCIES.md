# 依赖版本锁定

UVHTTP 使用 Git Submodules 进行依赖版本锁定，确保所有依赖的版本可追溯和可复现。

## 当前依赖版本

| 依赖 | 版本 | Commit SHA | 用途 |
|------|------|------------|------|
| mimalloc | v3.1.5 | dfa50c37d951128b1e77167dd9291081aa88eea4 | 内存分配器 |
| mbedtls | v3.6.0 | 0bc29f6441561e21cda4aa2ffc3772691dc1166e | TLS/SSL 支持 |
| cjson | v1.7.15 | d348621ca93571343a56862df7de4ff3bc9b5667 | JSON 解析 |
| googletest | release-1.12.1 | 58d77fa8070e8cec2dc1ed015d66b454c8d78850 | 测试框架 |
| libuv | v1.51.0 | 5152db2cbfeb5582e9c27c5ea1dba2cd9e10759b | 异步 I/O |
| uthash | v1.9.9.1 | 22646fcb7ce80be08d8917e153dabb272476c710 | 哈希表 |
| xxhash | v0.7.4 | 173e50be0509c6fb6c1bb74d95049ef61d7fdced | 快速哈希 |

## 自定义依赖

| 依赖 | 说明 |
|------|------|
| cllhttp | 自定义 llhttp 版本，位于 `deps/cllhttp/` |

## 版本锁定机制

Git Submodules 通过以下方式实现版本锁定：

1. **`.gitmodules` 文件**：记录子模块的 URL 和路径
2. **Commit SHA**：每个子模块都有特定的 commit SHA，确保版本固定
3. **自动检出**：克隆项目时，使用 `--recurse-submodules` 自动获取正确版本的依赖

## 使用方法

### 克隆项目（包含子模块）

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

### 如果已克隆项目，初始化子模块

```bash
git submodule update --init --recursive
```

### 更新子模块到特定版本

```bash
# 进入子模块目录
cd deps/libuv

# 检出到特定版本
git checkout v1.51.0

# 返回项目根目录
cd ../..

# 提交子模块版本变更
git add deps/libuv
git commit -m "deps: 更新 libuv 到 v1.51.0"
```

### 更新所有子模块到最新版本

```bash
git submodule update --remote --merge
```

### 查看子模块状态

```bash
git submodule status
```

输出格式：
```
<commit-sha> <path> (<branch>)
```

- 前导 `-` 表示子模块未初始化
- 前导 `+` 表示子模块有新的提交

## 版本锁定最佳实践

1. **始终使用特定的 commit SHA 或标签**，不要使用分支名称
2. **更新依赖前先测试**，确保新版本兼容
3. **记录变更原因**，在提交信息中说明更新原因
4. **定期更新依赖**，获取安全修复和性能改进
5. **使用 `--recurse-submodules`** 克隆项目，确保依赖版本正确

## 依赖更新流程

1. 检查依赖的新版本
2. 在子模块目录中测试新版本
3. 更新子模块到新版本
4. 运行完整测试套件
5. 提交变更

```bash
# 示例：更新 libuv
cd deps/libuv
git fetch origin
git checkout v1.52.0
cd ../..
make clean
make -j$(nproc)
./dist/bin/uvhttp_unit_tests
git add deps/libuv
git commit -m "deps: 更新 libuv 到 v1.52.0

- 修复内存泄漏问题
- 提升性能 10%
- 更新文档"
```

## 故障排除

### 子模块未初始化

```bash
git submodule update --init --recursive
```

### 子模块有未提交的更改

```bash
cd deps/<submodule>
git status
git checkout .
```

### 子模块指向错误的 commit

```bash
git submodule update --checkout deps/<submodule>
```

## 相关文档

- [构建指南](../guide/build.md)
- [依赖说明](DEPENDENCIES.md)