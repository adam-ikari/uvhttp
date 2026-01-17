# 配置文件和文档

本目录包含 UVHTTP 项目的测试配置文件和相关文档。

## 文件列表

- `config_valid.conf` - 有效的配置示例
- `config_invalid.conf` - 无效的配置示例
- `config_test.conf` - 测试配置
- `PERFORMANCE_COMPARISON_REPORT.md` - 性能对比综合报告

## 性能测试报告

完整的性能对比测试报告请查看: [PERFORMANCE_COMPARISON_REPORT.md](PERFORMANCE_COMPARISON_REPORT.md)

该报告包含以下对比测试：
- Nginx (多线程) vs UVHTTP
- Nginx (单线程) vs UVHTTP
- UVHTTP vs Python HTTP Server

## 配置文件用途

测试配置文件用于验证配置解析功能：
- `config_valid.conf` - 有效的配置示例
- `config_invalid.conf` - 无效的配置示例
- `config_test.conf` - 测试配置

## 相关文档

更多测试信息请参考：
- [test/README.md](../README.md) - 测试目录总览
- [test/scripts/](../scripts/) - 测试脚本
- [test/results/](../results/) - 测试结果