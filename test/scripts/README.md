# 内存泄漏测试

## 概述

本目录包含用于检测 UVHTTP 项目内存泄漏的脚本和工具。

## 本地测试

### 运行内存泄漏测试

使用提供的脚本运行完整的内存泄漏测试：

```bash
./test/scripts/run_memory_tests.sh
```

此脚本将：

1. 使用 AddressSanitizer (ASan) 编译项目
2. 运行所有测试并检测内存问题
3. 使用 Valgrind 对关键测试进行详细的内存泄漏检查
4. 生成测试报告

### 手动运行 Valgrind

对单个测试运行 Valgrind：

```bash
cd build
valgrind --leak-check=full \
          --error-exitcode=1 \
          --show-leak-kinds=all \
          --track-origins=yes \
          ./dist/bin/test_connection_full_coverage
```

### 手动运行 AddressSanitizer

使用 AddressSanitizer 编译和运行：

```bash
cmake -B build-asan \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
    -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g"

cmake --build build-asan --config Debug -j$(nproc)

cd build-asan
export ASAN_OPTIONS=detect_leaks=1:halt_on_error=0
ctest --output-on-failure -j1 --timeout 120
```

## CI/CD 集成

### 自动化测试

内存泄漏测试已集成到 CI/CD 流程中：

1. **memory-test job** - 在 main 和 develop 分支运行
   - 使用 AddressSanitizer 运行所有测试
   - 使用 Valgrind 检查关键测试
   - 上传日志作为 artifacts

2. **code-quality job** - 在 PR 和 feature 分支运行
   - 使用 Valgrind 检查关键测试
   - 发现泄漏时失败构建

3. **nightly job** - 每天运行
   - 对 7 个关键测试运行 Valgrind
   - 生成详细的内存泄漏报告

### 关键测试列表

以下测试在 CI/CD 中进行内存泄漏检查：

- `test_connection_full_coverage`
- `test_server_full_coverage`
- `test_request_full_coverage`
- `test_response_full_coverage`
- `test_config_full_coverage`
- `test_static_full_coverage`
- `test_router_full_coverage`

## Valgrind 选项说明

### 常用选项

- `--leak-check=full` - 完整的内存泄漏检查
- `--error-exitcode=1` - 发现泄漏时返回错误码 1
- `--show-leak-kinds=all` - 显示所有类型的泄漏
- `--track-origins=yes` - 追踪未初始化值的来源
- `--log-file=filename` - 将输出保存到文件

### 泄漏类型

- **definitely lost** - 确定的内存泄漏
- **indirectly lost** - 间接内存泄漏
- **possibly lost** - 可能的内存泄漏
- **still reachable** - 仍然可达的内存（通常不是问题）

## AddressSanitizer 选项

### 环境变量

- `ASAN_OPTIONS=detect_leaks=1` - 启用内存泄漏检测
- `ASAN_OPTIONS=halt_on_error=0` - 遇到错误时不停止
- `ASAN_OPTIONS=log_path=asan.log` - 保存日志到文件

### 编译选项

- `-fsanitize=address` - 启用地址消毒器
- `-fno-omit-frame-pointer` - 保留帧指针以便更好的调用栈
- `-g` - 包含调试信息

## 故障排除

### Valgrind 未找到

```bash
sudo apt-get install valgrind
```

### ASan 报告错误

ASan 报告的错误通常表示：
- 内存泄漏
- 使用已释放的内存
- 缓冲区溢出
- 使用未初始化的内存

查看 ASan 日志以获取详细信息。

### 测试超时

增加测试超时时间：

```bash
ctest --output-on-failure -j1 --timeout 300
```

## 最佳实践

1. **在提交前运行内存泄漏测试**
   ```bash
   ./test/scripts/run_memory_tests.sh
   ```

2. **修复所有内存泄漏**
   - 使用 Valgrind 日志定位泄漏位置
   - 确保每个分配都有对应的释放
   - 使用智能指针或 RAII 模式

3. **定期运行完整测试**
   - 在 CI/CD 中自动运行
   - 在本地运行以快速发现问题

4. **查看 CI/CD artifacts**
   - 下载 Valgrind 日志
   - 下载 ASan 日志
   - 分析内存问题

## 相关文档

- [Valgrind 用户手册](https://valgrind.org/docs/manual/manual.html)
- [AddressSanitizer 文档](https://github.com/google/sanitizers/wiki/AddressSanitizer)
- [UVHTTP 开发指南](../../docs/guide/DEVELOPER_GUIDE.md)