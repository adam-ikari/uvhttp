# 测试目录

自动化测试全部位于 `test/unit/`，由顶层 CMake 自动发现并注册进 ctest。
`manual/` 下是手动 server，不属于自动化测试。

## 目录结构

```text
test/
├── unit/       # GoogleTest *.cpp；每个文件编译为独立可执行文件并 add_test
├── mock/       # libuv_mock.c/.h；文件名匹配 test_*_mock*.cpp 时自动链接
├── certs/      # TLS 测试证书
├── scripts/    # 证书等辅助脚本
├── coverage/   # 覆盖率辅助文件
└── README.md
```

不存在 `test/CMakeLists.txt`，也不需要手工登记测试文件。

## 运行测试

```bash
make build
make test

# 单个测试
ctest --test-dir build -R test_router --output-on-failure
build/dist/bin/test_router_full_coverage

# ASan + UBSan
make verify-memory-safety

# 覆盖率
make build-coverage && make coverage
```

测试二进制位于 `build/dist/bin/`。完整规范见
[docs/guide/TESTING_GUIDE.md](../docs/guide/TESTING_GUIDE.md)。

## 手动测试工具

`manual/*.c` 会被编译，但**不注册进 ctest**：它们运行 `uv_run(UV_RUN_DEFAULT)`
永不返回，需要人用 curl/wrk 驱动。用法与限制见
[manual/README.md](../manual/README.md)。

## 性能测试

性能基准不在 `test/` 下。使用 `make bench` 构建 Release 版
`benchmark_unified`，或走 CI 的同机配对门禁。不要用本机连续多轮数字下结论，
原因见 [CONTRIBUTING.md](../CONTRIBUTING.md) 的性能测量章节。

## 注意事项

- 新增测试必须放 `test/unit/test_*.cpp`，否则 CI 不会执行。
- 裁剪构建（WebSocket/HTTPS/static/compression 关闭）下测试仍需能编译，
  因此要直接 include 自己用到的头文件。
- 新测试要做变异验证：改坏被测实现时测试必须变红。
