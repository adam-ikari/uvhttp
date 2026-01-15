# 测试目录

本目录包含 UVHTTP 项目的所有测试文件和测试脚本。

## 目录结构

```
test/
├── certs/                          # TLS 测试证书
├── cmake_build/                    # CMake 构建测试
├── nginx_single_thread_results/    # Nginx 单线程性能测试结果
├── unit/                           # 单元测试
├── uvhttp_performance_results/     # UVHTTP 性能测试结果
├── uvhttp_single_thread_results/   # UVHTTP 单线程性能测试结果
├── config_invalid.conf             # 无效配置示例
├── config_test.conf                # 测试配置
├── config_valid.conf               # 有效配置示例
├── generate_test_certs.sh          # 生成测试证书脚本
├── PERFORMANCE_COMPARISON_REPORT.md # 性能对比综合报告
├── run_performance_tests.sh        # 性能测试脚本
├── run_rate_limit_tests.sh         # 限流测试脚本
├── run_uvhttp_performance_local.sh # UVHTTP 本地性能测试脚本
└── websocket_test.html             # WebSocket 测试页面
```

## 性能测试

### 性能对比报告

完整的性能对比测试报告请查看: [PERFORMANCE_COMPARISON_REPORT.md](PERFORMANCE_COMPARISON_REPORT.md)

该报告包含以下对比测试：
- Nginx (多线程) vs UVHTTP
- Nginx (单线程) vs UVHTTP
- UVHTTP vs Python HTTP Server

### 运行性能测试

```bash
# 运行 UVHTTP 性能测试
bash test/run_uvhttp_performance_local.sh

# 运行性能测试（包含所有测试）
bash test/run_performance_tests.sh
```

## 单元测试

```bash
# 运行所有单元测试
./run_tests.sh

# 运行特定单元测试
cd build
./uvhttp_unit_tests
```

## 限流测试

```bash
# 运行限流功能测试
bash test/run_rate_limit_tests.sh
```

## 配置测试

测试配置文件用于验证配置解析功能：
- `config_valid.conf` - 有效的配置示例
- `config_invalid.conf` - 无效的配置示例
- `config_test.conf` - 测试配置

## WebSocket 测试

WebSocket 测试页面: `websocket_test.html`

启动 WebSocket 服务器后，在浏览器中打开该页面进行测试。

## TLS 测试

生成测试证书：

```bash
bash test/generate_test_certs.sh
```

测试证书将保存在 `test/certs/` 目录中。

## 测试结果

性能测试结果保存在以下目录：
- `nginx_single_thread_results/` - Nginx 单线程测试结果
- `uvhttp_performance_results/` - UVHTTP 性能测试结果
- `uvhttp_single_thread_results/` - UVHTTP 单线程测试结果

## 清理测试文件

```bash
# 清理构建文件
./run_tests.sh -c

# 清理测试结果
rm -rf test/nginx_single_thread_results/
rm -rf test/uvhttp_performance_results/
rm -rf test/uvhttp_single_thread_results/
```

## 注意事项

- 所有性能测试都在本地环境运行
- 测试端口：Nginx (8888), UVHTTP (8080)
- 测试工具：wrk
- 测试配置：4线程, 100并发连接, 30秒持续时间