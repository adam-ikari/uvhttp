# UVHTTP 开发计划

**创建日期**: 2026-01-13
**当前版本**: v1.4.0
**计划周期**: 2026-01-13 - 2026-02-28 (6 周)
**负责人**: Development Team

---

## 执行摘要

本开发计划针对 UVHTTP 项目当前状态（代码覆盖率 42.7%，目标 80%）制定，分为 3 个优先级级别：

- **P0 (高优先级)**: 1-2 周完成 - 修复编译问题、设置 CI/CD、提升覆盖率到 60%
- **P1 (中优先级)**: 1 个月完成 - 提升覆盖率到 80%、性能优化、云原生特性
- **P2 (低优先级)**: 后续完成 - 文档完善、示例补充

---

## 项目状态评估

### 当前指标

| 指标 | 当前值 | 目标值 | 差距 |
|------|--------|--------|------|
| 代码覆盖率 | 42.7% | 80% | -37.3% |
| 测试通过率 | 100% (69/69) | 100% | |
| 编译警告 | 30+ | 0 | |
| 编译错误 | 示例程序无法编译 | 0 | |
| CI/CD | 无 | 完整 | |
| 性能 (RPS) | 12,026 | 20,000+ | -40% |

### 低覆盖率模块

| 模块 | 当前覆盖率 | 目标 | 优先级 |
|------|-----------|------|--------|
| uvhttp_websocket_wrapper.c | 5.1% | 80% | P0 |
| uvhttp_server.c | 16.7% | 80% | P0 |
| uvhttp_request.c | 28.3% | 80% | P1 |
| uvhttp_tls_openssl.c | 28.1% | 80% | P1 |
| uvhttp_connection.c | 0% | 80% | P0 |
| uvhttp_static.c | 11.7% | 80% | P0 |

### 编译问题

1. **依赖库未构建**: libuv、mbedtls、llhttp 等依赖库未编译
2. **示例程序链接错误**: `cannot find -luv`
3. **编译警告**: 30+ 个未使用变量警告

---

## P0 任务 (高优先级 - 1-2 周)

### 任务 1: 修复依赖库构建问题 (2-3 天)

**状态**: 阻塞中
**优先级**: P0
**负责人**: 开发团队
**预计工作量**: 2-3 天

#### 问题描述

示例程序无法编译，因为依赖库（libuv、mbedtls、llhttp）未构建。

#### 当前状态

```bash
# 错误信息
/usr/bin/ld: cannot find -luv
collect2: error: ld returned 1 exit status
```

#### 解决方案

1. **构建 libuv**
   ```bash
   cd deps/libuv
   mkdir build && cd build
   cmake .. -DBUILD_TESTING=OFF
   make build
   ```

2. **构建 mbedtls**
   ```bash
   cd deps/mbedtls
   mkdir build && cd build
   make build
   ```

3. **构建 llhttp**
   ```bash
   cd deps/llhttp
   mkdir build && cd build
   make build
   ```

4. **构建 xxhash**
   ```bash
   cd deps/xxhash
   make
   ```

5. **构建 mimalloc**
   ```bash
   cd deps/mimalloc
   mkdir build && cd build
   make build
   ```

6. **更新 CMakeLists.txt**
   - 添加依赖库的查找逻辑
   - 确保库路径正确
   - 添加构建依赖检查

#### 验收标准

- [ ] 所有依赖库成功编译
- [ ] 示例程序能够编译通过
- [ ] 无链接错误
- [ ] 运行示例程序无错误

#### 风险

- **依赖库版本兼容性**: 可能需要调整 CMake 配置
- **构建时间**: 依赖库构建可能需要较长时间

---

### 任务 2: 修复编译警告 (1-2 天)

**状态**: 待开始
**优先级**: P0
**负责人**: 开发团队
**预计工作量**: 1-2 天

#### 问题描述

当前有 30+ 个编译警告，主要是未使用变量警告。

#### 警告列表

| 文件 | 警告类型 | 数量 |
|------|---------|------|
| test_static_full_coverage.c | unused-but-set-variable | 9 |
| test_server_full_coverage.c | unused-but-set-variable | 3 |
| test_context_simple.c | unused-but-set-variable | 1 |
| test_websocket_full_coverage.c | unused-but-set-variable | 17 |

#### 解决方案

1. **删除未使用的变量**
   ```c
   // 修改前
   int result = some_function();
   // result 未被使用

   // 修改后
   some_function();
   ```

2. **使用 (void) 标记有意未使用的变量**
   ```c
   int result = some_function();
   (void)result;  // 明确表示有意忽略
   ```

3. **添加断言确保变量被使用**
   ```c
   int result = some_function();
   assert(result == 0);  // 使用变量进行验证
   ```

#### 验收标准

- [ ] 编译警告数量降至 0
- [ ] 所有测试仍然通过
- [ ] 代码逻辑不变

#### 风险

- **测试逻辑变更**: 修改变量使用可能影响测试逻辑

---

### 任务 3: 设置 GitHub Actions CI/CD (3-5 天)

**状态**: 待开始
**优先级**: P0
**负责人**: DevOps
**预计工作量**: 3-5 天

#### 问题描述

项目缺少 CI/CD 流水线，无法自动化测试和部署。

#### CI/CD 需求

1. **多平台支持**
   - Linux (Ubuntu 20.04, 22.04)
   - macOS (12, 13)
   - Windows (2019, 2022)

2. **测试流程**
   - 编译检查
   - 单元测试
   - 覆盖率报告
   - 性能基准测试

3. **代码质量**
   - 静态分析 (clang-tidy)
   - 内存泄漏检测 (valgrind)
   - 编译警告检查

#### 实现方案

创建 `.github/workflows/ci.yml`:

```yaml
name: CI/CD Pipeline

on:
  push:
    branches: [ main, develop, release/* ]
  pull_request:
    branches: [ main, develop ]

jobs:
  build-and-test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-20.04, ubuntu-22.04, macos-12, macos-13, windows-2019, windows-2022]
        build_type: [Debug, Release]

    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive

    - name: Install dependencies (Linux)
      if: runner.os == 'Linux'
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake build-essential valgrind

    - name: Install dependencies (macOS)
      if: runner.os == 'macOS'
      run: |
        brew install cmake valgrind

    - name: Install dependencies (Windows)
      if: runner.os == 'Windows'
      run: |
        choco install cmake

    - name: Build dependencies
      run: |
        ./scripts/build_deps.sh

    - name: Configure CMake
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}

    - name: Build
      run: |
        cmake --build build --config ${{ matrix.build_type }} -j$(nproc)

    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure

    - name: Generate coverage report
      if: matrix.build_type == 'Debug' && runner.os == 'Linux'
      run: |
        cd build
        make coverage
        bash <(curl -s https://codecov.io/bash)

    - name: Memory leak check
      if: matrix.build_type == 'Debug' && runner.os == 'Linux'
      run: |
        cd build
        valgrind --leak-check=full ./dist/bin/uvhttp_unit_tests

  performance-test:
    runs-on: ubuntu-22.04
    needs: build-and-test

    steps:
    - uses: actions/checkout@v3
      with:
        submodules: recursive

    - name: Build project
      run: |
        cmake -B build -DCMAKE_BUILD_TYPE=Release
        cmake --build build --config Release -j$(nproc)

    - name: Run performance tests
      run: |
        cd build
        ./scripts/run_performance_tests.sh

    - name: Upload performance results
      uses: actions/upload-artifact@v3
      with:
        name: performance-results
        path: build/performance_results/
```

#### 验收标准

- [ ] CI 流水线成功运行
- [ ] 所有平台测试通过
- [ ] 覆盖率报告自动生成
- [ ] 性能测试结果自动上传

#### 风险

- **构建时间**: 多平台测试可能耗时较长
- **资源限制**: GitHub Actions 有时间限制

---

### 任务 4: 提升代码覆盖率到 60% (1 周)

**状态**: 待开始
**优先级**: P0
**负责人**: 开发团队
**预计工作量**: 1 周

#### 问题描述

当前代码覆盖率 42.7%，需要提升到 60%。

#### 低覆盖率模块分析

**uvhttp_websocket_wrapper.c (5.1%)**

需要测试的功能:
- WebSocket 连接建立
- 消息发送/接收
- 连接关闭
- 错误处理

**uvhttp_server.c (16.7%)**

需要测试的功能:
- 服务器启动/停止
- 连接管理
- 路由处理
- 错误处理

**uvhttp_connection.c (0%)**

需要测试的功能:
- 连接建立
- 连接关闭
- 超时处理
- 错误处理

**uvhttp_static.c (11.7%)**

需要测试的功能:
- 静态文件服务
- 缓存管理
- sendfile 优化
- 错误处理

#### 实现方案

1. **创建测试文件**
   - `test/unit/test_websocket_wrapper_coverage.c`
   - `test/unit/test_server_coverage.c`
   - `test/unit/test_connection_coverage.c`
   - `test/unit/test_static_coverage.c`

2. **测试用例设计**

**WebSocket 测试用例**:
```c
// 连接建立测试
TEST(WebSocketWrapperTest, ConnectionEstablishment) {
    // 测试正常连接建立
    // 测试连接失败
    // 测试超时
}

// 消息处理测试
TEST(WebSocketWrapperTest, MessageHandling) {
    // 测试发送消息
    // 测试接收消息
    // 测试大消息
    // 测试二进制消息
}

// 连接关闭测试
TEST(WebSocketWrapperTest, ConnectionClose) {
    // 测试正常关闭
    // 测试异常关闭
    // 测试超时关闭
}
```

**Server 测试用例**:
```c
// 服务器启动测试
TEST(ServerTest, ServerStartStop) {
    // 测试正常启动
    // 测试端口冲突
    // 测试重复启动
}

// 连接管理测试
TEST(ServerTest, ConnectionManagement) {
    // 测试最大连接数
    // 测试连接超时
    // 测试连接清理
}

// 路由处理测试
TEST(ServerTest, Routing) {
    // 测试精确匹配
    // 测试通配符匹配
    // 测试路由优先级
}
```

3. **覆盖率目标分解**

| 模块 | 当前覆盖率 | 目标覆盖率 | 需要增加的测试 |
|------|-----------|-----------|---------------|
| uvhttp_websocket_wrapper.c | 5.1% | 60% | 15+ 测试用例 |
| uvhttp_server.c | 16.7% | 60% | 20+ 测试用例 |
| uvhttp_connection.c | 0% | 60% | 25+ 测试用例 |
| uvhttp_static.c | 11.7% | 60% | 20+ 测试用例 |

#### 验收标准

- [ ] 整体代码覆盖率 ≥ 60%
- [ ] 所有新增测试通过
- [ ] 无回归问题
- [ ] 覆盖率报告生成成功

#### 风险

- **测试编写时间**: 可能需要更多时间
- **复杂测试场景**: 某些场景难以模拟

---

## P1 任务 (中优先级 - 1 个月)

### 任务 5: 提升代码覆盖率到 80% (2-3 周)

**状态**: 未开始
**优先级**: P1
**负责人**: 开发团队
**预计工作量**: 2-3 周

#### 问题描述

在 P0 任务完成后，继续提升代码覆盖率到 80%。

#### 剩余低覆盖率模块

| 模块 | 当前覆盖率 | 目标覆盖率 | 优先级 |
|------|-----------|-----------|--------|
| uvhttp_request.c | 28.3% | 80% | P1 |
| uvhttp_tls_openssl.c | 28.1% | 80% | P1 |
| uvhttp_response.c | 40% | 80% | P1 |
| uvhttp_router.c | 45% | 80% | P1 |

#### 实现方案

1. **Request 模块测试**
   - 请求解析
   - 请求头处理
   - 请求体处理
   - 错误处理

2. **TLS 模块测试**
   - TLS 握手
   - 证书验证
   - 加密/解密
   - 错误处理

3. **Response 模块测试**
   - 响应构建
   - 响应头设置
   - 响应体发送
   - 错误响应

4. **Router 模块测试**
   - 路由注册
   - 路由匹配
   - 路由参数
   - 路由缓存

#### 验收标准

- [ ] 整体代码覆盖率 ≥ 80%
- [ ] 所有模块覆盖率 ≥ 70%
- [ ] 核心模块覆盖率 ≥ 85%
- [ ] 所有测试通过

---

### 任务 6: 性能优化 (2 周)

**状态**: 未开始
**优先级**: P1
**负责人**: 开发团队
**预计工作量**: 2 周

#### 问题描述

当前性能 12,026 RPS，目标 20,000+ RPS。

#### 性能瓶颈分析

1. **内存分配**
   - 频繁的内存分配/释放
   - 缺少对象池

2. **锁竞争**
   - 连接管理中的锁竞争
   - 路由缓存中的锁竞争

3. **I/O 操作**
   - 非最优的缓冲区大小
   - 缺少批量处理

#### 优化方案

1. **内存池优化**
   ```c
   // 实现连接对象池
   typedef struct {
       uvhttp_connection_t* pool;
       size_t pool_size;
       size_t used_count;
   } connection_pool_t;

   uvhttp_connection_t* connection_pool_acquire(connection_pool_t* pool);
   void connection_pool_release(connection_pool_t* pool, uvhttp_connection_t* conn);
   ```

2. **无锁数据结构**
   - 使用原子操作
   - 使用 RCU (Read-Copy-Update)

3. **批量处理**
   - 批量发送响应
   - 批量处理请求

4. **零拷贝优化**
   - 使用 sendfile
   - 使用 splice

#### 验收标准

- [ ] 性能 ≥ 20,000 RPS
- [ ] 内存占用减少 20%
- [ ] CPU 使用率降低 15%
- [ ] 无性能回归

---

### 任务 7: 实现云原生特性 (1-2 周)

**状态**: 未开始
**优先级**: P1
**负责人**: 开发团队
**预计工作量**: 1-2 周

#### 问题描述

实现云原生场景所需的核心特性。

#### 功能列表

1. **健康检查端点**
   ```c
   // GET /health
   // 返回: {"status": "healthy", "timestamp": "..."}
   ```

2. **Prometheus 指标导出**
   ```c
   // GET /metrics
   // 返回 Prometheus 格式的指标
   ```

3. **优雅关闭**
   ```c
   uvhttp_error_t uvhttp_server_graceful_shutdown(
       uvhttp_server_t* server,
       int timeout_seconds
   );
   ```

4. **环境变量配置**
   ```c
   uvhttp_error_t uvhttp_config_load_from_env(
       uvhttp_config_t* config
   );
   ```

#### 实现方案

1. **健康检查**
   - 检查服务器状态
   - 检查连接数
   - 检查内存使用

2. **Prometheus 指标**
   - 请求计数器
   - 响应时间直方图
   - 错误率

3. **优雅关闭**
   - 停止接受新连接
   - 等待现有请求完成
   - 超时后强制关闭

4. **环境变量配置**
   - 支持常用配置项
   - 提供默认值
   - 验证配置有效性

#### 验收标准

- [ ] 健康检查端点正常工作
- [ ] Prometheus 指标正确导出
- [ ] 优雅关闭无数据丢失
- [ ] 环境变量配置生效

---

### 任务 8: 实现待办事项 (1 周)

**状态**: 未开始
**优先级**: P1
**负责人**: 开发团队
**预计工作量**: 1 周

#### 待办事项列表

1. **HTTP/1.1 流式传输** (计划中)
2. **WebSocket 压缩** (permessage-deflate)
3. **请求限流增强** (滑动窗口)
4. **连接复用优化**
5. **Server-Sent Events (SSE)** (研究阶段)

#### 实现方案

1. **HTTP/1.1 流式传输**
   - 实现分块传输编码
   - 流式上传 API
   - 流式下载 API
   - 进度回调

2. **WebSocket 压缩**
   - 实现 permessage-deflate
   - 添加压缩级别配置
   - 性能测试

3. **请求限流增强**
   - 实现滑动窗口算法
   - 支持分布式限流
   - 添加限流统计

3. **连接复用优化**
   - 改进 Keep-Alive 策略
   - 优化连接池管理
   - 添加连接预热

#### 验收标准

- [ ] WebSocket 压缩正常工作
- [ ] 滑动窗口限流正确
- [ ] 连接复用性能提升

---

## P2 任务 (低优先级 - 后续完成)

### 任务 9: 文档完善 (2 周)

**状态**: 未开始
**优先级**: P2
**负责人**: 文档团队
**预计工作量**: 2 周

#### 待完善文档

1. **API 文档**
   - WebSocket 认证 API
   - 连接管理 API
   - 错误码解读 API

2. **教程**
   - WebSocket 教程
   - 性能优化教程
   - 云原生部署教程

3. **示例**
   - WebSocket 认证示例
   - 连接管理示例
   - 广播示例

#### 验收标准

- [ ] 所有 API 有文档
- [ ] 所有功能有教程
- [ ] 所有功能有示例

---

### 任务 10: 测试增强 (1 周)

**状态**: 未开始
**优先级**: P2
**负责人**: 测试团队
**预计工作量**: 1 周

#### 测试增强

1. **集成测试**
   - 端到端测试
   - 压力测试
   - 兼容性测试

2. **性能测试**
   - 基准测试
   - 回归测试
   - 对比测试

3. **安全测试**
   - 模糊测试
   - 渗透测试
   - 依赖扫描

#### 验收标准

- [ ] 集成测试覆盖率 90%
- [ ] 性能测试自动化
- [ ] 安全测试通过

---

## 时间表

### 第 1-2 周 (P0 任务)

| 周次 | 任务 | 负责人 | 状态 |
|------|------|--------|------|
| Week 1 | 修复依赖库构建 | 开发团队 | 阻塞中 |
| Week 1 | 修复编译警告 | 开发团队 | 待开始 |
| Week 1-2 | 设置 GitHub Actions CI/CD | DevOps | 待开始 |
| Week 2 | 提升覆盖率到 60% | 开发团队 | 待开始 |

### 第 3-4 周 (P1 任务 - 第一批)

| 周次 | 任务 | 负责人 | 状态 |
|------|------|--------|------|
| Week 3-4 | 提升覆盖率到 80% | 开发团队 | 未开始 |
| Week 3-4 | 性能优化 | 开发团队 | 未开始 |

### 第 5-6 周 (P1 任务 - 第二批)

| 周次 | 任务 | 负责人 | 状态 |
|------|------|--------|------|
| Week 5 | 实现云原生特性 | 开发团队 | 未开始 |
| Week 6 | 实现待办事项 | 开发团队 | 未开始 |

### 后续 (P2 任务)

| 周次 | 任务 | 负责人 | 状态 |
|------|------|--------|------|
| Week 7-8 | 文档完善 | 文档团队 | 未开始 |
| Week 9 | 测试增强 | 测试团队 | 未开始 |

---

## 依赖关系

```
任务 1 (依赖库构建)
  └─> 任务 2 (修复编译警告)
  └─> 任务 3 (设置 CI/CD)
  └─> 任务 4 (提升覆盖率到 60%)

任务 4 (覆盖率 60%)
  └─> 任务 5 (覆盖率 80%)

任务 5 (覆盖率 80%)
  └─> 任务 6 (性能优化)
  └─> 任务 7 (云原生特性)
  └─> 任务 8 (待办事项)

任务 6-8
  └─> 任务 9 (文档完善)
  └─> 任务 10 (测试增强)
```

---

## 风险管理

### 高风险项

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| 依赖库构建失败 | 高 | 中 | 提前测试、准备备选方案 |
| 性能目标未达成 | 高 | 中 | 提前性能分析、迭代优化 |
| CI/CD 配置复杂 | 中 | 高 | 分阶段实施、参考最佳实践 |

### 中风险项

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| 测试编写耗时 | 中 | 中 | 优先覆盖核心功能 |
| 文档更新滞后 | 低 | 高 | 自动化文档生成 |
| 人员变动 | 中 | 低 | 知识共享、代码审查 |

---

## 资源需求

### 人力资源

| 角色 | 人数 | 投入时间 |
|------|------|---------|
| 开发工程师 | 3-4 | 全职 |
| DevOps 工程师 | 1 | 50% |
| 测试工程师 | 1 | 50% |
| 文档工程师 | 1 | 30% |

### 硬件资源

| 资源 | 数量 | 用途 |
|------|------|------|
| 开发机器 | 4 | 日常开发 |
| 测试服务器 | 2 | 性能测试 |
| CI/CD 服务器 | 1 | 持续集成 |

### 软件资源

| 软件 | 用途 |
|------|------|
| GitHub Actions | CI/CD |
| Codecov | 覆盖率报告 |
| SonarQube | 代码质量 |

---

## 验收标准

### 整体目标

- [ ] 代码覆盖率 ≥ 80%
- [ ] 编译警告 = 0
- [ ] 所有测试通过
- [ ] 性能 ≥ 20,000 RPS
- [ ] CI/CD 完整运行
- [ ] 文档完整

### 阶段性目标

#### P0 完成 (Week 2)

- [ ] 所有示例程序编译通过
- [ ] 编译警告 = 0
- [ ] CI/CD 基础功能运行
- [ ] 代码覆盖率 ≥ 60%

#### P1 完成 (Week 6)

- [ ] 代码覆盖率 ≥ 80%
- [ ] 性能 ≥ 20,000 RPS
- [ ] 云原生特性完成
- [ ] 所有待办事项完成

#### P2 完成 (Week 9)

- [ ] 文档完整
- [ ] 测试完善
- [ ] 项目就绪发布

---

## 沟通计划

### 会议安排

| 会议 | 频率 | 参与者 | 目的 |
|------|------|--------|------|
| 每日站会 | 每天 | 开发团队 | 进度同步 |
| 周例会 | 每周 | 全员 | 进度汇报 |
| 技术评审 | 按需 | 开发团队 | 技术决策 |
| 代码审查 | 持续 | 开发团队 | 代码质量 |

### 报告机制

- **每日进度**: Slack/Teams 状态更新
- **周报**: 邮件发送给所有利益相关者
- **里程碑报告**: 关键节点详细报告

---

## 成功指标

### 定量指标

| 指标 | 当前值 | 目标值 | 测量方法 |
|------|--------|--------|---------|
| 代码覆盖率 | 42.7% | 80% | gcov/lcov |
| 性能 (RPS) | 12,026 | 20,000+ | wrk/ab |
| 编译警告 | 30+ | 0 | 编译输出 |
| 测试通过率 | 100% | 100% | ctest |
| 文档完整度 | 70% | 95% | 文档审查 |

### 定性指标

- 代码质量提升
- 团队效率提升
- 项目可维护性提升
- 用户体验改善

---

## 附录

### A. 依赖库构建脚本

```bash
#!/bin/bash
# scripts/build_deps.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

echo "Building dependencies..."

# Build libuv
echo "Building libuv..."
cd "$PROJECT_ROOT/deps/libuv"
mkdir -p build && cd build
cmake .. -DBUILD_TESTING=OFF
make build

# Build mbedtls
echo "Building mbedtls..."
cd "$PROJECT_ROOT/deps/mbedtls"
mkdir -p build && cd build
make build

# Build llhttp
echo "Building llhttp..."
cd "$PROJECT_ROOT/deps/llhttp"
mkdir -p build && cd build
make build

# Build xxhash
echo "Building xxhash..."
cd "$PROJECT_ROOT/deps/xxhash"
make

# Build mimalloc
echo "Building mimalloc..."
cd "$PROJECT_ROOT/deps/mimalloc"
mkdir -p build && cd build
make build

echo "All dependencies built successfully!"
```

### B. CI/CD 配置参考

详见任务 3 中的 CI/CD 配置示例。

### C. 覆盖率报告生成

```bash
# 生成覆盖率报告
cd build
make coverage

# 查看报告
lcov --list coverage.info
genhtml coverage.info --output-directory coverage_html

# 上传到 Codecov
bash <(curl -s https://codecov.io/bash)
```

---

## 附录 D: 编译优化设计原则

### 设计原则

1. **单线程环境不需要 volatile**
   - 所有代码在同一线程中执行
   - 编译器优化不会破坏单线程程序的正确性
   - volatile 不能解决任何实际问题

2. **使用 -O2 而不是 -O3**
   - -O3 可能导致过度优化和时序问题
   - -O2 提供良好的性能和稳定性平衡
   - 避免激进的循环展开

3. **不限制内联数量**
   - 让编译器自动决定内联策略
   - 只对热路径函数使用 `inline` 关键字提示
   - 避免手动限制导致的性能损失

4. **完整的边界检查**
   - 添加空指针检查
   - 添加缓冲区溢出检查
   - 避免未定义行为

5. **正确的循环逻辑**
   - 使用 `UV_RUN_ONCE` 而不是 `UV_RUN_NOWAIT`
   - 确保回调被执行
   - 无论 `owns_loop` 状态都运行循环

### 热路径函数

以下函数使用 `inline` 关键字提示编译器内联：

- `route_hash()` - 路由哈希计算
- `fast_method_parse()` - 快速方法解析
- `find_in_hot_routes()` - 热路径查找

### 测试配置

为慢速测试添加串行运行标志，避免并发竞争：

```cmake
set_tests_properties(${test_name} PROPERTIES 
    TIMEOUT 300 
    LABELS "slow" 
    RUN_SERIAL TRUE)
```

---

**文档版本**: 1.2
**最后更新**: 2026-01-24
**下次审查**: 2026-01-27
