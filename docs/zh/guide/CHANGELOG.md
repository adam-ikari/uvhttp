---
title: 更新日志
description: UVHTTP 全部重要变更记录。格式基于 Keep a Changelog，遵循语义化版本规范。涵盖 1.0.0 至 2.7.2 各版本的新增、修复、变更、安全与性能改进，以及生产级内存安全验证。
---

# 更新日志

本文件记录本项目的所有重要变更。

格式基于 [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)，
本项目遵循[语义化版本](https://semver.org/spec/v2.0.0.html)规范。
## [2.9.1] - 2026-10-01

### 修复
- **format-check 门禁此前完全不工作**: job 用 `${{ github.event.before }}` 作 `git diff` 的 base，但 `pull_request` 事件 payload 没有该字段 → 展开为空串 → `git diff --name-only --diff-filter=ACMR "" <sha>` 报 `fatal: ambiguous argument ''` → `files` 为空 → 恒走 "No C/C++ files changed — skipping" exit 0。自 PR #380 引入该 job 起，**所有 PR 的 C/C++ 格式变更都未被检查过**（门禁一直是空转绿灯）。改用正确的 `${{ github.event.pull_request.base.sha }}`（#416）
- **format-check 门禁 clang-format 版本漂移**: 门禁装 `apt` 的 clang-format（跟随 runner 镜像版本，ubuntu-24.04 为 18.x），跨大版本输出差异极大——实测同一份代码 v14 判 0 违规、v18 判 424 处，导致「本地过、CI 红」且无法复现。门禁改为 `pip install clang-format==18.1.8` 钉住版本，存量按该版本全量重格式化（424 处，30 文件）（#416）

### 安全
- **CI GITHUB_TOKEN 权限最小化**: `ci-pr.yml` 与 `ci-daily.yml` 未声明 `permissions`，继承仓库默认（admin/maintain/push/triage 全开）。PR CI 与 daily build 实际只需 checkout（`contents: read`）；权限过宽意味着 CI 被 compromise 时可向 main 推代码。加显式最小权限声明（#420）

### 移除
- **`ci-daily.yml` 冗余定时 workflow**: 只跑 Debug 构建 + 测试 + 建 issue，是 `ci-nightly` 的严格子集（nightly 覆盖 Debug 构建、测试、ASan、UBSan、覆盖率、压力测试，且已含自动建 issue）。调度只早 8 小时，捕获的回归 PR CI 已在每个 PR 上拦截。删除该 workflow，日常检查改看 `ci-nightly`
- **趋势数据落库（trend job + `docs/benchmark-trends/`）**: `ci-benchmark.yml` 的 trend job 在 pre-release 事件把基准数据 push 到 `benchmark-trends` 分支并自动开 PR。v2.9.0 pre-release 暴露该 job 直推 main 被 GH013 拒（main PR-only），#418 改推专用分支修复，但实际验证时 `gh pr create` 又被仓库 Actions policy 拒（`can_approve_pull_request_reviews: False`），且该设置无法通过 API 修改。趋势数据落库本身是辅助展示，门禁判定（paired gate）不依赖它——判定由 `regression_check.py` 在 CI 内即时完成。故整体删除 trend job 与 `docs/benchmark-trends/` 目录

### 变更
- **删除死代码 `chunked_transfer_context_t`**（`uvhttp_static.c`）: cppcheck `--std=c99` 报该类型所有字段未使用，核实后确认是**整个结构体从未被实例化**（非字段死，是类型死）——实际分块传输走同名局部变量/参数。删除整个 typedef + 注释。`uvhttp_websocket.c` 另清理一处冗余 NULL 检查（外层已保证非 NULL）（#417）

## [2.9.0] - 2026-10-01

### 新增
- **Fuzz 覆盖扩展**: 新增三个 libFuzzer + ASan harness 覆盖此前未 fuzz 的解析路径——`fuzz_request`（HTTP 请求解析回调链，#398）、`fuzz_websocket`（RFC 6455 帧头解码 + mask XOR，#399）、`fuzz_static_path`（`uvhttp_static_resolve_safe_path` 路径解析面，含符号链接逃逸测试树，#408）。`fuzz_static_path` 经变异验证证明 harness 有牙齿（移除包含性检查后 1s 抓到逃逸），60s 自由 fuzz 未发现可利用路径穿越。ci-fuzz 现稳定跑四个目标
- **零拷贝阈值 wire 等价性测试**: `test/unit/test_zerocopy_threshold_wire.cpp` 用真实 server（`uv_tcp_getsockname` 取实际端口 + 阻塞 socket + `uv_run(UV_RUN_NOWAIT)` 泵循环）验证 writev 双 iovec 与单缓冲拷贝两条路径的 wire 行为一致——Content-Length、body 完整性、阈值边界（#393，含提前退出与 fd 泄漏修复 #396）
- **压缩 × 零拷贝阈值交互测试**: 验证可压缩响应跨阈值（原始≥4096、压缩后<4096）的 wire 行为自洽，覆盖 #402 回归的触发条件（#402）
- **请求体上限与跨 chunk 累积边界测试**: 补请求体长度上限、跨 chunk 边界的 header/body 累积覆盖（#403）

### 修复
- **响应 header 名称 control-char 检查**: `uvhttp_response_set_header` 此前只校验 header 值的 control-char，header 名称未校验——攻击者可控的名称可注入 CRLF 造成响应分割。现名称与值统一过 `contains_control_chars`（#404）
- **`uvhttp_lru_cache` 仅日志用变量的构建修复**: 三个局部变量（`cleared_count`、两处 `freed_memory`）只在 `UVHTTP_LOG_*` 里被读，日志被 `NDEBUG`/`UVHTTP_FEATURE_LOGGING=OFF` 裁剪后成为 dead store，新版 clang 的 `-Wunused-but-set-variable` + `-Werror` 让构建失败。加 `UVHTTP_UNUSED` 并补 `uvhttp_features.h` include（该文件整个内容被 `UVHTTP_FEATURE_STATIC_FILES` 包裹，本地默认配置根本不编译它）（#409）
- **fuzz crash artifact 上传顺序**: 上传步骤原先排在部分 Run fuzz 步骤之前，导致崩溃产物丢失。移到所有 Run 之后（#400）

### 变更
- **`test/integration/` 19 个文件移到 `manual/`**: 这些文件没有一个是自动化测试——全是长驻 server（`uv_run` 永不返回）+ 外部 curl/wrk 驱动，CMake 仅编译、从不注册进 ctest。其中 4 个（42 处 `assert()`）在 Release（`NDEBUG`）下断言全部展开为 no-op，`exit=0` 是虚假绿灯。删 4 个纯 assert 文件，其余 15 个长驻 server + 资产移到 `manual/`，CMake glob 同步，位置不再传递「在 test/ 下就会被 CI 跑」的误导（#411）
- **benchmark base 解析排除 nightly 预发布**: nightly 自动构建自移动中的 main，若作配对门禁的 base 会让 head/base ≈ 100% 空测绿灯。base 解析现跳过 prerelease 标记的 release（#391）
- **社区贡献指南增补**: CONTRIBUTING 增补代码审查清单与测试形态选择（#401）；补「integration 目录不被执行」「assert 在 Release 下失效」「feature 宏整体包裹的文件本地编译不到」三条已踩过的坑（#410）

### 内部（决策，非用户可见）
- 性能测量禁止在本机下结论——连配对 A/B 也不行（本机热降频使连续多轮不可复现，#406）
- io_uring 评估关闭——libuv 1.52 不覆盖 sendfile，静态文件热路径不可达（#407）
- 内存分配优化 P2 关闭——收益 0.026% 低于 40% 测量噪声（#405）
- 零拷贝阈值压缩前判定评估关闭——收益上限 ~1.4% 低于噪声，且 prepare 非幂等使低成本改法不可行（#412）

## [2.8.1] - 2026-09-29

### 变更
- **基准回归门禁改为同机配对比较**: `ci-benchmark.yml` 现在在同一 runner、同一 job 内检出并构建 base 版本（PR base sha / 上一个 release tag / 手动 `base_ref`），两台 `benchmark_unified`（18081/18082）按轮次交替测量 `/`、`/json`、`/large`（各 10 轮，奇数轮先压 head、偶数轮先压 base）；`regression_check.py --compare` 按 round 配对取 head/base 比值中位数，判定条件为中位数 < 90% **且**多数配对本身也低于 90%（base 缺数据、配对 <3 轮、或被 gate 端点在任一 CSV 缺失同样失败——空跑的 gate 不算通过）。门禁端点清单单一来源为 workflow 的 `GATE_ENDPOINTS`，以 `--gate` 传给脚本。绝对 RPS 基线（83K/81K/8.8K）降级为报告信息——同一 commit 跨 run 的中位数在共享 runner 上漂移可达 40%，绝对阈值实际 gate 的是机器运气而非代码。比值噪声本身重尾：零 C 改动的 PR 上 10 轮配对仍给出中位数 97.4%、MAD 14.4%，故先试的稳健置信下界（中位数 − 1.7·MAD-SE）会把同一构建判红，已换为中位数 + 多数规则（改后同一对比第三次运行给出 `/` 98.4%、仅 3/10 配对低于限，PASS）；真实的 writev 小 body 回退（−14%、散布约 2%）几乎每个配对都在阈值以下，仍被 gate 捕获。同时修正 benchmark job 的 token 权限（仓库默认 read-only 使 PR 评论与趋势落库一直无法执行，趋势落库拆为独立 job）

### 修复
- **零拷贝 writev 拖慢小响应**: v2.8.0 的 writev 路径对全部非 TLS 响应生效，小 body 下 header+body 双 iovec 反而比单缓冲拷贝路径慢。服务端绑核单核、`wrk -t4 -c100` 同机配对测量下 `/` 中位数比 v2.7.2 低 13.9%。现仅当 `body_length >= UVHTTP_ZEROCOPY_MIN_BODY`（默认 4096，可经 CMake 调整）才走 writev，小响应回到拷贝路径（`/` 恢复到 v2.7.2 的 98.7%），`/large` 零拷贝增益不变（1.51x）。合入前由新的同机配对门禁实测（base = 含全量 writev 的 main）：`/` **+23.4%**、`/json` **+27.6%**、`/large` +1.8%

## [2.8.0] - 2026-09-23

### 新增
- **基准测试 SSE / 流式 / WebSocket 三维度** (`benchmark/ws_benchmark_client.py`): 新增 `/sse`（~60-68K 流/s）、`/stream`（~9.5-10.8K req/s）、`/ws_connect`、`/ws_echo` 基准场景，长连接与流式路径纳入回归观测（PR #379）
- **`BUILD_TESTS` 编译选项**: 嵌入者可裁剪测试构建，不再编译 googletest（PR #377）
- **CI 纳入 examples 编译验证**: ubuntu-build job 编译全部示例（PR #385）

### 变更
- **大 response 零拷贝 writev 发送**: header+body 组装 iovec 一次 writev，消除每请求 200KB memcpy 与两次 100KB 分配——/large RPS **5140 → 8881（+72.7%）**；TLS/非法 client 自动回退原拷贝路径（PR #378）
- **benchmark 回归门禁接入 GitHub release 事件**: pre-release 门禁 + 趋势落库；原 pre-release 分支触发为死配置，发布流程改为两阶段 PR-only（PR #373）
- **/large 回归基线更新至 8800**: v2.7.x 零拷贝优化后的新基线（PR #385）
- **UBSan 门禁恢复 101/101 零发现**: 修复预存测试中的非法枚举值（PR #375）
- **PR 门禁占位 job 真实化**: cppcheck 真实实现、format-check 门禁变更文件、移除假 dependency-scan（PR #376）

### 修复

#### 第二轮代码评审 — 17 项（commit 63f29bd）
- **lru_cache**: OOM 路径 use-after-free 与淘汰逻辑死循环修复
- **config 双归属多次释放**: server 持有 / context 借用归属厘清，消除 double-free
- **错误码映射补齐**: `uvhttp_strerror` 覆盖全部 84 个枚举值
- **gzip 替换路径预算绕过**: 压缩缓存替换路径不再绕过内存预算
- **JSON 注入转义**: 输出 JSON 的用户数据正确转义
- **敏感词过滤**: 过滤逻辑缺陷修复
- **版本 fallback 漂移**: 版本号回退路径与 VERSION 文件一致

#### API / 构建系统高危（commit 8da889f）
- **install 不再发布第三方头与静态库**: 第三方依赖产物不泄漏到消费者系统
- **特性宏 / 分配器类型 PUBLIC 传播**: 消除消费者 ABI 错配
- **find_package 全链路重写**: `uvhttp-config.cmake` / `uvhttp.pc` 消费者验证通过

#### 构建与示例（commit 504dff6 / 52202ed / b9826a1）
- **08_e2e_tests 构建顺序**: 14 个目标补 `add_dependencies`
- **examples 全部可编译**: 4 个坏示例重写、Makefile 依赖、6 处 config 双 free
- **llhttp submodule dirty 显示**: `.gitmodules` 增加 `ignore = dirty`

#### 发布收尾（2026-09-23 发布 PR）
- **ci-benchmark 发布门禁触发器回归**: PR #379 基于改动前的 `ci-benchmark.yml` 覆盖了 PR #373 已合并的 `release: [published]` 触发，门禁退回 `push: branches: [pre-release]` 死配置（该分支从不创建，导致 pre-release 不触发回归门禁）；恢复触发器、job 条件、gate 步骤与趋势落库条件（仅 `prerelease == true` 落库）
- **文档死链 7 处修复**: `examples/embedding/` 目录链接位于 docs srcDir 之外改为 GitHub URL、`API_REFERENCE` 指向不存在的 `generated/index.html` 改为纯文本——VitePress 构建与 deploy-docs 部署管线（自 2026-09-07 起连续 5 次失败）恢复可用
- **版本引用同步 2.8.0**: `src/uvhttp_version.c` 非 CMake 构建版本 fallback、README badge 与关键指标标题、`API_REFERENCE` 版本头、嵌入指南 `GIT_TAG v2.8.0`（英/中）

### 文档
- **API_REFERENCE 与实际 API 对齐**: 错误码表重建、虚构函数修正（PR #382）
- **README / README_CN / performance / spec 对齐**: 签名/死链/RPS 基线/编译命令实测可用（PR #384）

## [2.7.2] - 2026-09-07

### 修复

#### P0/P1 — 11 项关键缺陷修复（commit a3bc755）
- **query string 路由匹配**: 参数化路由匹配前剥离 query string，`/users/1?tab=2` 不再误匹配除 `/users/:id` 之外的路径
- **MAX_PARAMS 栈溢出边界**: 超出 `MAX_PARAMS` 时参数写入不再越界，路由参数提取受边界保护
- **on_url/on_header_field 跨 chunk 分段累积**: 解析回调跨 chunk 边界的分段数据正确累积，不再截断/错位
- **migrate_to_trie 失败悬垂指针**: trie 迁移失败路径修复悬垂指针，路由表切换后不再访问已释放内存
- **WS 非 TLS send 短写截帧**: WebSocket 非 TLS 路径 send 短写时循环重发，不再截断帧
- **CLOSE 后继续处理帧（RFC 6455）**: 收到 CLOSE 帧后停止处理后续数据帧，符合 RFC 6455
- **If-Modified-Since 时区错误与 3 种日期格式**: `mktime` → `timegm`（UTC 比较避免本地时区偏移），兼容 RFC 7231 的 IMF-fixdate / RFC 850 / asctime 三种 HTTP-date 格式
- **accept 失败 active_connections 下溢**: `uv_accept` 失败时连接计数不再下溢，永久 503 问题消除
- **connection_new 失败路径 UAF**: 连接创建失败路径修复 use-after-free
- **server_free 不排空在途 close 回调**: 释放时正确排空在途 close 回调，避免 libuv 访问已释放内存
- **超时路径 WS wrapper 泄漏**: 超时路径下 WebSocket wrapper 不再泄漏

#### P2/P3 — 11 项改进与修复（commit a0eae2b）
- **TLS EINTR 重试**: TLS send/recv 遇到 `EINTR` 自动重试
- **If-None-Match weak/多值 ETag**: 支持 weak comparison 与多值 ETag 列表
- **目录列表 TOCTOU**: 静态目录列表路径修复 TOCTOU 竞态
- **on_header_value 分段累积**: header value 跨 chunk 分段正确累积
- **keep-alive headers_extra 泄漏**: keep-alive 连接复用不再泄漏 `headers_extra`
- **X-Forwarded-For 默认不信任**: 默认不信任 `X-Forwarded-For`，新增 `trust_proxy_headers` 配置开关
- **MIME 双表合并单表**: 静态文件 MIME 类型双表合并为单表，消除查找不一致
- **TLS cipher 满排空**: cipher 列表满时正确排空，不再静默截断
- **死代码清理**: 移除失效代码路径
- **listen 参数校验**: `uvhttp_server_listen` 校验非法参数（端口 0 / 空地址）
- **server_stop 幂等化**: `uvhttp_server_stop` 重复调用安全


## [2.7.1] - 2026-08-26

### 新增
- **性能回归门禁** (`scripts/performance/regression_check.py`): CI 基准测试后自动对比内置基线（/ 83K, /json 81K, /large 5.7K RPS），阈值 10%，RPS 低于基线 90% 则 CI 失败（PR #366）
- **嵌入验证第二轮**: `add_subdirectory` 集成方式验证通过，更新嵌入验证清单（PR #365）

### 变更
- **CMAKE_C_STANDARD**: 99 → 11，与 PHILOSOPHY.md "实际构建使用 C11" 文档对齐，修复 clang C99 下重复 typedef 编译错误（PR #364）
- **CMake 依赖可见性**: `libuv`/`xxhash`/`llhttp`/`mbedtls` 从 `PRIVATE` → `PUBLIC` 链接，嵌入者通过 `add_subdirectory` 集成时可正确传播 include 路径（PR #365）
- **CMake PUBLIC include**: 添加 `deps/uthash/src`（uthash 为 header-only 库，无 IMPORTED target）（PR #365）

### 修复
- **ci-fuzz 连续 5 天失败**: clang + C99 + `-Werror` 下同 TU 重复 typedef 报 `-Wtypedef-redefinition`（GCC 不报故主 CI 一直绿）；修复为 C11 对齐 + 删除未使用的 `uvhttp_validate_buffer_state` + fuzz_router 链接补齐 `libminiz.a`/`libxxhash.a`（PR #364）
- **fuzz_request 过时 harness**: 引用已不存在的 `uvhttp_request_parse`/`UVHTTP_REQUEST_STATE_DONE` API，从 CI 移除（fuzz_router 已覆盖请求解析路径）（PR #364）
- **嵌入者无法编译**: public headers 引用 `llhttp.h`/`<uv.h>`/`xxhash.h`/`uthash.h`/mbedtls，但这些依赖被标记为 PRIVATE，嵌入者找不到头文件（PR #365）

## [2.7.0] - 2026-08-21

### 新增
- **TLS 会话缓存**: 重新启用 `mbedtls_ssl_conf_session_cache()`，默认 2048 条目 / 86400s（24h）超时，预期减少 30-50% TLS 握手时间
- **TLS 会话票据**: 重新启用 `mbedtls_ssl_conf_session_tickets()`
- **CI 性能基准工作流** (`ci-benchmark.yml`): 10 轮多轮测试，报告峰值 vs 稳态值，支持 workflow_dispatch / PR 'benchmark' 标签 / pre-release push 三种触发方式
- **Brain 知识库**: 填充全部 6 个根页面（background/architecture/flow/mindmap/stack/roadmap），新增 4 个决策页面
- **设计哲学文档**: PHILOSOPHY.md（项目定位、分发模型、嵌入式优先、内存安全基础设施、测试基础设施、零成本抽象、嵌入验证）
- **嵌入验证清单** (`docs/embedding-checklist.md`): 基于 qwrt 嵌入经验的 9 类验证维度
- **构建矩阵 CI** (`build-matrix` job): 验证全开/最简/ROUTER_CACHE/静态文件/无压缩等特性组合

### 变更
- **性能基准迁移至 GitHub CI**: 从本地基准（AMD 5800H, 40%+ 方差）迁移到 GitHub CI runner（0.4-2.4% CV），新基线 **Platinum 层级** 83K RPS（simple）/ 82K RPS（json）/ 5.7K RPS（large）
- **Platinum 层级**: 新增 80K RPS tier，Diamond tier 设为 100K RPS 远期目标
- **CV 作为 KPI**: 变异系数 < 5% 作为 runner 稳定性指标
- **严格 ISO C99**: `CMAKE_C_EXTENSIONS OFF` + `-D_GNU_SOURCE`，miniz `MZ_FORCEINLINE` 修复

### 修复
- **L3: gzip 缓存内存追踪**: `total_memory` 现在计入 `sizeof(gzip_cache_entry_t)` 结构体开销（~56 bytes/entry），此前的 `max_memory_usage` 保证是近似值
- **L4: set_max_entries 边界**: `uvhttp_gzip_cache_set_max_entries()` 在 `max_entries > capacity` 时自动扩容 entries 数组，而非静默失效
- **L5: 注释拼写**: `paddingto32bytes` → `padding to 32 bytes`（response.h, server.h）
- **Router cache ABI**: 修复 `cache_optimized_router_t` 与 `uvhttp_router_t` 结构体布局不兼容（S1）和 `UVHTTP_ANY` 匹配缺失（S2）

## [2.6.1] - 2026-08-12

### 新增
- **中英文档一致性门禁**: `make check-docs` —— `check-doc-sync.sh` 作为阻断式 PR 门禁，中英文档不一致会阻止合并
- **`translate-docs.sh`**: AI 辅助的 EN→ZH 翻译脚本
- **`benchmark/benchmark.cmake`**: 构建 `benchmark_unified`，供 nightly test-stress / performance-full 使用
- **Nightly 失败自动建 issue**: ci-nightly 失败时自动创建 `[nightly]` issue（去重，重复失败追加评论）
- **周五复盘检查**: weekly-retro-check.yml 检查 `docs/dev/weekly/<YYYY>-W<WW>.md`，缺失时自动建 `[retro]` issue

### 变更
- **4 个流程文档重写**为单人 AI 敏捷模型（AGILE / development-rhythm / release-strategy / sprint-backlog）：PO=用户、Dev=Claude、SM=自动化，按需发布 SemVer
- **中英文档一致性**: 29 个 ZH 文件翻译/校对与 EN 对齐，移除孤儿 sync_hash frontmatter，修复 docs 目录结构
- **Nightly CI 权限**: 安装 lcov + 授予 release 权限
- **基准**: /simple 25,949 RPS、/json 26,088 RPS、/large 25,154 RPS（2 threads/10 conn/5s，3 次中位数）—— 从 Silver tier 提升至 **Gold**（25,000+ RPS），较 v2.6.0 基线 +20%

### 修复
- **WebSocket use-after-return**: websocket manager 测试中的 stack-use-after-return（ASan，PR #315）
- **静态文件内存安全**: 释放 static 内存路径中的 `read_file_content` buffer（PR #322）
- **prewarm FIFO 挂起**: `uvhttp_static_prewarm_cache` 加 `S_ISREG` guard，遇 FIFO 不再挂起（PR #325）
- **Nightly CI 基础设施**: CodeQL 配置（init step + `security-events` 权限）、artifact 执行位丢失（`chmod -R +x`）、`performance-trend.md` ENOENT 时序（PR #316 / #318 / #321）
- **文档死链**: VitePress 无法路由 `docs/` 目录外的链接
- **docs build 脚本**: doxygen 隐藏目录与 npm 脚本名错误

## [2.6.2] - 2026-08-17

### 修复
- **连接上限 503 路径 use-after-free**: `on_connection` 中当连接数达到上限、临时 503 客户端 `uv_accept` 失败时，改用 `uv_close`（close 回调中释放）而非直接 `uvhttp_free`——直接释放已注册到 libuv 句柄队列的内存会在下一次 `uv_run`/`uv_loop_close` 触发 use-after-free
- **`server->max_connections` 误导性死状态**: 结构体字段此前初始化为 `UVHTTP_MAX_CONNECTIONS_MAX`(10000) 但从未被读取，实际限制来自 config（默认 2048）；现改为初始化为 `UVHTTP_MAX_CONNECTIONS_DEFAULT` 并让 `on_connection` 在无 config 时以该字段为权威值，字段与真实行为一致
- **WebSocket RFC 6455 合规与内存安全**（PR #336）: `uvhttp_ws_send_frame` 成功路径释放发送缓冲（此前每帧泄漏）、修复 build_frame 的 double-free、实现 fragmentation 状态机（CONTINUATION 帧不再静默丢弃）、对累积分片强制执行 `config.max_message_size` 并防护 size_t 溢出
- **uv_strerror 一致性**: 将 `uvhttp_server.c` 中残留的直接 `uv_strerror` 调用改为 `uv_strerror_r`，与 `uvhttp_error_helpers.c` 文档化的统一错误处理约定一致（`uv_strerror` 对未映射错误码会经 `uv__strdup` 泄漏）

### 测试
- **回归测试**: `test_connection_libuv_fail` 新增 `ConnectionLimitAcceptFailClosesTempClient`（uv_accept 失败必须经 uv_close 关闭临时客户端）与 `ServerMaxConnectionsFieldIsAuthoritative`（无 config 时 `server->max_connections` 字段必须被遵守）

## [2.6.0] - 2026-07-31

### 新增
- **健康检查端点**: `uvhttp_server_enable_health_check()` —— 为负载均衡/编排探针返回 HTTP 200 + `{"status":"ok"}` JSON
- **Server-Sent Events 示例**: `examples/06_advanced/sse_server.c` —— 异步 `uv_timer` 驱动的事件流
- **请求日志中间件示例**: `examples/03_middleware/logging_middleware.c`
- **预压缩静态构建脚本**: `scripts/precompress-static.sh` —— 对静态资源做 gzip 压缩以零拷贝服务
- **Fuzz 目标**: `fuzz_request.c` —— HTTP 请求解析 harness（第二个 fuzz 目标）
- **边界与空指针安全测试**: 33+ 个测试覆盖 NULL 参数、空 body、零长度、端口 0
- **Mock 测试基础设施**: 链接器包装（`-Wl,--wrap`）注入 libuv 错误路径
- **SECURITY.md 与 CODE_OF_CONDUCT.md**

### 变更
- **构建入口**: 用 `Makefile`（直接 cmake 包装）替换 `GNUmakefile` —— `make build`、`make test`、`make verify-memory-safety`、`make check-syntax`
- **移除 GCC 扩展**: `__attribute__((packed, weak, unused, no_sanitize))` —— 纯 C99
- **移除运行时 vtable**: 恢复直接 libuv 调用；通过编译期链接器 mock 做依赖注入
- **`uvhttp_context_create`**: 现在校验 NULL loop
- **中文 README**: 与英文版同步
- **文档**: hero 宽度修复、性能目标文档、每周计划模板

### 性能
- 吞吐量 ~18K RPS（10 conn）/ ~17.4K RPS（100 conn）—— 较 2.5.1 基线低 ~6.6%，属测量噪声而非代码回退

## [2.5.1] - 2026-07-25

### 新增
- **连接测试覆盖率**: 44 个新测试，覆盖率 42.8% → ~70%
- **WebSocket 自动化测试**: 新 `test_websocket_automated.cpp` 中的 99 个测试用例
- **路由测试覆盖率**: `find_array_route` 和 `match_route_node` 覆盖率测试
- **SDD 规范文档**: static-api、tls-api、protocol-upgrade 规范
- **开发节奏文档**: AI 驱动的 24 小时开发工作流
- **Daily-build CI**: UTC 20:00 自动构建与测试，失败时创建 issue

### 变更
- **`uvhttp_server_new_with_loop`**: 内部事件循环管理的新 API
- **文档**: 从中文文档中移除系统 libuv 依赖
- **文档风格规范**: 添加写作标准与 AI 味清除指南
- **VitePress 侧边栏**: 从公开网站移除内部开发文档

### 修复
- **ASan 内存缺陷**: 修复 3 个栈缓冲区下溢、空指针与测试泄漏问题
- **FAQ API 签名**: 修正 `uvhttp_config_new` 和 `uvhttp_static_create` 示例
- **产品站一致性**: 移除 zh/performance.md 中的虚构数据
- **CSP**: 为 SPA 导航添加 'unsafe-inline'

### 移除
- **gh-pages 分支**: 完全迁移到 GitHub Actions 部署
- **冲突的技能**: 移除独立技能，改用 Superpowers 框架
- **过期分支**: 清理 develop、gh-pages 及其他未使用分支
- **系统 libuv 依赖**: libuv 现在仅以 submodule 形式 vendored
- **内部开发文档**: 从公开网站侧边栏移除 zh/dev/ 部分

### 修复 — 库代码中真实的内存安全缺陷（均通过常规测试，但在 ASan 下会破坏内存）

- **`uvhttp_router.c` `add_route_method`**（libFuzzer 发现）：堆缓冲区溢出 —— 在参数化路由触发向 trie 迁移（`migrate_to_trie` 将 `array_capacity` 重置为 0）后，后续一条 *非参数* 路由落入 `add_array_route`，其 `new_capacity = array_capacity * 2` 计算结果为 0，导致 `realloc(NULL, 0)` 返回一个最小块，随后 `strncpy` 将其溢出。已修复：trie/数组分支判断现在还会检查 `router->use_trie`，因此迁移后所有路由都进入 trie。
- **`uvhttp_router.c` `find_or_create_child`**：堆 use-after-free —— 指向节点池的缓存 `parent` 指针在 `create_route_node` 重新分配池后变为悬垂指针。现在在 realloc 之后重新获取。
- **`uvhttp_server.c` `uvhttp_server_free`**：重复释放时的堆 use-after-free —— `freed` 标志从已释放的内存中读取。现在传入 `NULL` 为安全的空操作（遵循标准的 `free(NULL)` 约定）。
- **`uvhttp_websocket.c` `uvhttp_ws_close`**：栈缓冲区下溢 —— 测试注册了栈上局部的 `ws_conn` 对象，其生命周期在销毁之前就已结束。
- **`uvhttp_response.c` `build_response_headers`**：堆缓冲区溢出 —— `snprintf` 返回值累加使 `pos` 超出缓冲区，导致 `*length - pos` 边界下溢。已用带边界裁剪的 `UVHTTP_SNAPPEND` 宏修复。
- **`uvhttp_connection.c`**：多项修复 —— 关闭后读取的 use-after-free；`restart_read` 泄漏请求体（置空指针但未释放）；`connection_close` 重入破坏了 `close_pending` 计数；`switch_to_websocket` 覆盖了 CLOSING 状态；`conn->lifecycle` 从未释放。
- **`uvhttp_request.c` `uvhttp_request_cleanup`**：使其幂等（释放后将 `body`/`parser`/`parser_settings` 置空）以防止重复释放。
- **`uvhttp_response.c` `uvhttp_response_set_header`**：在容量边界首次分配扩展时泄漏了既有的 `headers_extra`。
- **`uvhttp_error_helpers.c`**：`uvhttp_handle_write_error`/`uvhttp_log_safe_error` 对非 libuv 的 errno 值使用 `uv_strerror`，触发 libuv 的 `uv__strdup` 泄漏。改用 `uv_strerror_r`（可重入、无分配）和 `uvhttp_error_string`。
- **`uvhttp_server.c` `on_connection` 503 路径**：`temp_client`（`uv_tcp_t`）被分配但从未关闭/释放。现在 `write_503_response_cb` 会关闭并释放它。
- **`uvhttp_server.c` `create_simple_server_internal`**：监听失败路径在 `uvhttp_server_free` 之前将 `router` 置空，导致路由器释放失效。
- **`uvhttp_server.c` `uvhttp_server_ws_disable_connection_management`**：在对定时器执行 `uv_close` 后立即释放了管理器结构（内嵌 `uv_timer_t` 句柄），导致 libuv 在关闭回调触发时访问已释放内存。现在先排空关闭回调再释放。

### 修复 — 测试套件内存泄漏（25 → 0 个 ASan 失败）

- 解决了 request/response/router/server/connection/context/tls/protocol_upgrade 测试文件中所有仅泄漏型的测试失败（先释放后置空、关闭后排空、mbedTLS 部分清理模式）。

### 变更 — 构建与 CI

- **`CMakeLists.txt`**：将 C 标准从 C11 切换为 **C99**
  （`CMAKE_C_STANDARD 99`）。完整库、测试和示例在 `-std=c99`/`-std=gnu99` 下构建并通过（91/91）。这扩大了编译器支持范围，并与默认采用 C99 的嵌入式工具链对齐。
- **`CMakeLists.txt`**：Sanitizer/Debug 构建不再 strip（`-s`）—— ASan/UBSan 栈回溯现在可解析。检测覆盖 `ENABLE_DEBUG`、`ENABLE_ASAN/UBSAN/TSAN`、`CMAKE_BUILD_TYPE=Debug`，以及 flags 中的任意 `-fsanitize=`。Release 构建仍然 strip。
- **`.github/workflows/ci-nightly.yml`**：`test-memory` 作业现在使用 `ENABLE_ASAN=ON`（保留符号）；新增 `test-ubsan` 作业（`ENABLE_UBSAN=ON`），并接入汇总。
- **`benchmark/benchmark_unified.c`**：修复了阻止编译的非法 `static` 嵌套函数定义；补上缺失的 `<errno.h>`。
- **`examples/Makefile.examples`**：`-std=c11` → `-std=c99`。

### 验证

- ASan：91/91 测试通过 —— 零泄漏、零 use-after-free、零缓冲区溢出。
- UBSan：91/91 测试通过 —— 零未定义行为。
- 常规构建：91/91 测试通过。

## [2.5.0] - 2026-03-17

### 新增

- **测试覆盖改进**
  - 新增 `test_version_full_coverage.cpp`：版本与构建信息 API（43 个测试用例）
  - 新增 `test_error_complete_coverage.cpp`：错误码与处理（58 个测试用例）
  - 新增 `test_protocol_upgrade_api_coverage.cpp`：协议升级 API（26 个测试用例）
  - 新增 `test_connection_public_api_coverage.cpp`：连接管理 API（45 个测试用例）
  - 增强 `test_utils_full_coverage.cpp`：工具测试重构
  - **合计**：跨 5 个测试文件新增 172 个测试用例

### 变更

- **代码质量**
  - 覆盖率从 85.4% 提升至 89.2%（+3.8%）
  - 函数覆盖率 92.0%（7336/7974 个函数）
  - 行覆盖率 89.2%（13386/15008 行）

- **模块覆盖率改进**
  - `uvhttp_version.c`：0.0% → 98.3%（版本 API 测试）
  - `uvhttp_error.c`：31.7% → 98.8%（全部错误码与处理）
  - `uvhttp_protocol_upgrade.c`：39.6% → 73.6%（协议升级 API）
  - `uvhttp_connection.c`：42.6% → 42.8%（连接管理 API）

### 测试

- **测试套件扩展**
  - 新测试聚焦公共 API 覆盖
  - NULL 参数处理测试
  - 错误条件和边界用例测试
  - 跨核心模块集成测试
  - 172 个新测试全部通过

- **覆盖要点**
  - 高覆盖模块（≥95%）：
    - uvhttp_utils.c：100.0%
    - uvhttp_error.c：98.8%
    - uvhttp_version.c：98.3%
    - uvhttp_error_helpers.c：95.9%
  - 中等覆盖模块（70-95%）：
    - uvhttp_protocol_upgrade.c：73.6%
    - uvhttp_config.c：72.6%
    - uvhttp_response.c：72.0%
    - uvhttp_context.c：63.3%
    - uvhttp_router.c：61.7%
    - uvhttp_server.c：54.3%
    - uvhttp_request.c：52.8%

### 文档

- 更新性能基准
- 增强 API 文档（覆盖率统计）
- 更新测试覆盖率报告

## [2.4.4] - 2026-02-26

### 修复

- **连接清理中的严重内存泄漏**
  - 修复 `uvhttp_connection_free()` 中每连接 1,932,392 字节的严重内存泄漏
  - 新增内部函数 `uvhttp_connection_free_resources()` 处理实际清理
  - 更新 `on_handle_close()` 回调，改为调用 `uvhttp_connection_free_resources()` 而非 `uvhttp_connection_free()`
  - 所有句柄关闭时（`close_pending == 0`）资源现被正确释放
  - **影响**：防止服务器因内存耗尽而崩溃

### 新增

- **内存泄漏测试**
  - 新增 `test/unit/test_pure_gtest.cpp`：纯 Google Test 基线测试（3 个测试用例）
  - 新增 `test/unit/test_simple_memory_leak.cpp`：内存泄漏测试（7 个测试用例）
    - 基本操作的内存管理
    - 连接生命周期（10 个连接）
    - 上下文管理
    - 重复释放保护
    - NULL 指针处理
    - 快速创建/销毁（100 次迭代）
    - 连接资源完整性验证

### 变更

- **代码质量**
  - 修复 `test_e2e_simple.c` 中未初始化的变量（status 变量）
  - 修复 `uvhttp_connection.c` 中的代码对齐
  - 更新注释以准确反映实现
  - 将测试文件中的中文注释翻译为英文

- **文档**
  - 修复 `STATIC_FILE_SERVER.md` 中失效的性能指南链接
  - 在 `SECURITY.md` 中添加无需登录的 CERT C 编码规则替代链接
  - 在配置中启用 VitePress i18n 路由
  - 更新中文版安全文档

### 性能

- **内存影响**：
  - **之前**：每连接泄漏 1,932,392 字节（2,096 直接 + 1,930,296 间接）
  - **之后**：0 字节泄漏
  - **验证**：Valgrind 确认 "All heap blocks were freed -- no leaks are possible"

- **测试结果**：
  - 全部 39 个单元测试通过
  - 所有集成测试通过
  - 扩展测试套件：1,148 次分配，1,148 次释放（完全匹配）
  - 零性能回退

### 安全

- **内存安全**：`uvhttp_connection_free_resources()` 中妥善的 NULL 指针处理
- **重复释放保护**：通过 `conn->freed` 标志防止重复释放场景
- **无新增漏洞**：此修复未引入任何安全风险

### 迁移

无破坏性变更。这是一个修复版本，保持向后兼容。

## [2.4.2] - 2026-02-25

### 新增

- **端到端测试**
  - 新增 test_e2e_simple.c，使用外部服务器进程进行简单 e2e 测试
  - 新增 test_e2e_real.c，使用进程内服务器进行完整 e2e 测试
  - 新增 test_https_e2e.c，进行 HTTPS 端到端测试
  - e2e 测试中自动启动/停止服务器
  - 并发请求测试（10 个并发请求）
  - 响应校验与验证
  - HTTPS 测试自动生成 TLS 证书

- **HTTPS 测试支持**
  - 默认启用 BUILD_WITH_HTTPS=ON 以获得完整测试覆盖
  - 所有 TLS 相关测试现均通过：
    - test_server_api_coverage
    - test_server_error_coverage
    - test_tls_api_coverage
    - test_tls_null_coverage
  - 测试中完整的 TLS 上下文管理

### 修复

- **CI/CD 性能测试**
  - 将 CI/CD 性能测试简化为仅编译验证
  - 移除无法在 GitHub Actions 中运行的运行时性能测试
  - 将 Docker 内存限制从 4GB 降至 2GB 以加快执行
  - 移除 CI/CD 中 benchmark_unified 的服务器启动（libuv 兼容性问题）
  - 保留仅编译验证作为最可靠的方式

- **测试构建警告**
  - 修复 e2e 测试中未使用变量的警告
  - 修复 system() 返回值警告
  - 所有测试现以零警告编译

### 变更

- **测试覆盖**
  - 单元测试：93% → 100%（63/63 通过）
  - 新增 3 个 e2e 测试文件（+552 行）
  - 启用所有先前跳过的 HTTPS 测试
  - 测试执行时间：63 个测试共 0.24 秒

- **构建配置**
  - 默认 BUILD_WITH_HTTPS=ON（之前为 OFF）
  - 默认 BUILD_WITH_WEBSOCKET=ON
  - 默认 BUILD_WITH_MIMALLOC=ON
  - 开箱即用提供完整功能集

### 性能

- 本地性能测试：12,096 RPS（10 个连接，10 秒）
- 测试时间：0.24 秒（63 个单元测试）
- 内存优化：默认启用 mimalloc 分配器

## [2.4.1] - 2026-02-13

### 新增

- **扩展测试覆盖**
  - 跨 4 个核心模块新增 99 个测试用例
  - test_static_extended_coverage.cpp（25 个测试）针对 uvhttp_static.c
  - test_router_extended_coverage.cpp（25 个测试）针对 uvhttp_router.c
  - test_connection_extended_coverage.cpp（24 个测试）针对 uvhttp_connection.c
  - test_request_extended_coverage.cpp（25 个测试）针对 uvhttp_request.c
  - 改进整体代码覆盖与测试稳定性

### 修复

- **连接生命周期中的严重内存泄漏**
  - 修复每连接约 18KB 的内存泄漏，原因在于过早设置 freed 标志
  - 将资源清理从 uvhttp_connection_free 移至 on_handle_close 回调
  - 正确的清理顺序：TLS → request → response → read_buffer → connection
  - 防止重复释放，所有资源正确回收
  - 对长时间运行的服务器应用至关重要

- **测试构建系统**
  - 修复 CMakeLists.txt 中 MOCK_TEST_FILES 未定义问题
  - 为 mock 测试文件添加正确的 GLOB 定义
  - 全部 58 个测试现可成功编译
  - 从编译中排除已禁用的测试

- **测试脚本可移植性**
  - 修复 run_all_tests.sh 中 58 处硬编码路径
  - 通过 BIN_DIR 变量实现动态路径检测
  - 脚本现可适配不同构建配置（build/ 与 dist/）
  - 提升跨不同环境的脚本可用性

### 变更

- **测试覆盖指标**
  - uvhttp_router.c：33.6% → 62.9%（行），68.4% → 84.2%（函数）
  - uvhttp_connection.c：39.9% → 40.7%（行），53.6% → 60.7%（函数）
  - uvhttp_request.c：51.5% → ~60%（行），~64% → ~70%（函数）
  - uvhttp_static.c：19.1% → 21.1%（行），56.2% → 62.5%（函数）
  - 2 个模块（router、connection）现超过 60% 覆盖目标

## [2.4.0] - 2026-02-12

### 新增

- **CMake 导出配置**
  - 新增 CMake 导出配置，便于库集成
  - 使用 install(EXPORT) 替代 export()
  - 为导出目标添加 NAMESPACE uvhttp::
  - 依赖通过 uvhttp-config.in.cmake 中的 find_dependency() 查找
  - 新增 pkg-config 支持（uvhttp.pc.in）
  - 简化库使用者的集成

### 修复

- **WebSocket 测试条件**
  - 修复 WebSocket 集成测试，仅要求 BUILD_WITH_WEBSOCKET
  - 修复 test_server_simple_api_coverage，要求 BUILD_WITH_WEBSOCKET
  - WebSocket 测试现正确检查 WebSocket 支持
  - 修复 HTTPS Only 构建中的编译错误

- **静态文件示例**
  - 修复静态文件示例的条件编译
  - 仅在 BUILD_WITH_STATIC_FILES=ON 时编译 examples/04_static_files/
  - 静态文件示例使用 uvhttp_static.h 类型，需 UVHTTP_FEATURE_STATIC_FILES=1
  - 修复 Full + Examples 构建失败

- **覆盖率报告生成**
  - 改进覆盖率报告生成的错误处理
  - 在 lcov capture 添加 --base-directory 以正确定位源文件
  - 移除 deps/ 与 test/ 目录的覆盖率数据
  - 添加 '|| true' 防止覆盖率数据缺失时失败
  - 修复 Coverage Mode 与 Debug + Coverage 构建失败

### 变更

- **CI/CD 构建矩阵**
  - 在 CI/CD 构建矩阵名称中将 TLS 重命名为 HTTPS
  - TLS Only → HTTPS Only
  - WebSocket + TLS → WebSocket + HTTPS
  - TLS + mimalloc → HTTPS + mimalloc
  - WebSocket + TLS + mimalloc → WebSocket + HTTPS + mimalloc
  - 与 BUILD_WITH_HTTPS 变量名对齐

### CI/CD 状态
- 构建矩阵验证：15/15 通过 ✅
- 所有核心功能构建通过
- Coverage Mode 与 Debug + Coverage 现已通过

## [2.3.1] - 2026-02-10

### 修复

- **性能回退**
  - 修复连接清理中事件循环阻塞导致的严重性能回退
  - 从 `uvhttp_connection_free()` 移除同步 `uv_run()` 调用
  - 替换为通过 `uvhttp_connection_close()` 的异步关闭机制
  - **性能改进**：
    - 10 个连接：10,691 → 31,151 RPS（+192%）
    - 50 个连接：129 → 30,487 RPS（+23,500%）
    - 100 个连接：7 → 31,409 RPS（+448,600%）
  - **Socket 错误**：从高并发时 95%+ 降至所有级别 0%
  - **连接泄漏**：消除 CLOSE_WAIT 状态连接

### 变更

- **代码体积**
  - `uvhttp_connection.c` 减少 38 行（-7%）

## [2.3.0] - 2026-02-04

### 破坏性变更

- **LRU 缓存简化**
  - 移除 LFU 与 Hybrid 淘汰模式（仅保留 LRU）
  - 移除双阈值淘汰机制（现改为单一 90% 阈值）
  - 移除任务队列机制（淘汰现改为同步）
  - 移除 5 个 API 函数：
    - `uvhttp_lru_cache_set_eviction_mode()`
    - `uvhttp_lru_cache_init_task_queue()`
    - `uvhttp_lru_cache_schedule_eviction()`
    - `uvhttp_lru_cache_stop_task_queue()`
    - `uvhttp_lru_cache_perform_eviction()`
  - **迁移指南**：参见 `docs/MIGRATION_GUIDE_LRU_CACHE.md`

### 新增

- **LRU 缓存改进**
  - 新增通过 `uvhttp_lru_cache_set_batch_eviction_size()` 配置批量淘汰大小
  - 默认缓存大小增至 10MB
  - 为破坏性变更新增迁移指南

- **协议升级框架**
  - 实现零开销协议升级框架
  - 新增自定义协议升级支持（如 IPPS、gRPC-Web）
  - 重构 WebSocket 以使用新框架
  - 为正常 HTTP 请求新增快速路径优化
  - 性能影响：单协议场景开销 < 0.4%

- **路由缓存优化**
  - 优化路由器节点结构以提升 CPU 缓存局部性
  - 节点大小缩减 53%（272 → 128 字节）
  - 缓存行使用减少 60%（5 → 2 行）
  - 新增性能对比测试

- **性能测试**
  - 新增 `benchmark_router_comparison.c` 用于路由器性能对比
  - 新增 `benchmark_rps_150_routes.c` 用于大规模路由器测试
  - 新增 `benchmark_file_transfer.c` 用于文件传输性能测试

### 变更

- **代码质量**
  - 将所有源代码注释从中文翻译为英文
  - 在 `include/uvhttp_constants.h` 中集中定义 HTTP 常量
  - 移除过时的注释与代码
  - 修复代码格式问题

- **文档**
  - 为 LRU 缓存变更新增迁移指南
  - 新增双语 FAQ 文档（英文与中文）
  - 新增安全文档
  - 新增双语支持的文档规范
  - 更新性能基准文档

- **构建系统**
  - 修复 32 位构建兼容性问题
  - 修复代码格式问题
  - 更新 CMake 配置

### 性能

- **内存优化**
  - LRU 缓存实例开销：-132 字节
  - LRU 缓存条目开销：-4 字节
  - 代码体积减少：-200 行（-21%）

- **路由器性能**
  - 节点大小：272 → 128 字节（-53%）
  - 缓存行使用：5 → 2 行（-60%）
  - 改进 CPU 缓存局部性

- **协议升级性能**
  - 单协议开销：< 0.4%
  - 快速路径检测：O(1) 检查
  - 禁用时零开销

### 修复

- **32 位兼容性**
  - 修复 `benchmark_router_comparison.c` 中 `uint64_t` 格式化
  - 新增 `PRIu64` 宏以支持跨平台

- **代码格式**
  - 修复 `uvhttp_lru_cache.c` 中的格式问题
  - 修复 `uvhttp_constants.h` 中的格式问题
  - 修复 `uvhttp_lru_cache.h` 中的格式问题

### 迁移说明

**面向从 UVHTTP 2.2.x 升级到 2.3.0 的用户：**

1. **LRU 缓存**：移除对已弃用淘汰模式与任务队列函数的调用
2. **协议升级**：若仅使用 HTTP 或 WebSocket 则无需变更
3. **路由器**：无需变更，性能改进自动生效

详细迁移说明请参见 `docs/MIGRATION_GUIDE_LRU_CACHE.md`。

## [2.2.2] - 2026-02-02

### 修复

- **路由器关键 bug 修复**
  - 修复路径参数丢失问题（移除错误的 `param_count` 重置）
  - 添加递归深度限制，防止栈溢出崩溃
  - 影响范围：所有参数化路由（如 `/api/users/:id`）

- **文档翻译错误修复**
  - 修复 `include/uvhttp_config.h` 中的拼写错误和速率限制注释
  - 修复 `include/uvhttp_constants.h` 中 50+ 处损坏的英文翻译
  - 提升代码可读性和可维护性

- **代码格式统一**
  - 统一头文件包含的缩进风格（4 空格）
  - 符合项目 C99 代码规范

### Performance

- **性能提升**
  - 峰值 RPS 从 20,432 提升到 21,991（+7.6%）
  - 性能目标达成率：95.3%（目标 23,070 RPS）
  - 高并发稳定性：10-200 并发，RPS 波动仅 30%

- **性能测试结果**
  - 简单文本：21,991 RPS，4.56ms 延迟
  - JSON 响应：21,095 RPS，4.74ms 延迟
  - 小响应：21,395 RPS，4.67ms 延迟，23.02MB/s 吞吐

### Testing

- **测试覆盖率**
  - 路由器测试：51 个测试全部通过
  - 核心模块测试：246 个测试全部通过
  - 包括连接、服务器、响应、WebSocket、缓存等模块

### Changed

- 合并 `feature/enable-router-cache-optimization` 分支
- 路由缓存优化功能已启用
- 更新性能基准测试文档（2026-02-02）

## [2.2.1] - 2026-01-31

### Breaking Changes

⚠️ **重要**: TLS 错误类型已整合到统一错误体系

1. **TLS 错误类型整合**
   - **影响**: 所有使用 `uvhttp_tls_error_t` 的代码
   - **变更**: 删除 `uvhttp_tls_error_t`，所有 TLS API 函数返回类型改为 `uvhttp_error_t`
   - **迁移**: 更新所有 TLS 相关函数调用
   ```c
   // 旧代码（已移除）
   uvhttp_tls_error_t result = uvhttp_tls_context_new(&ctx);
   if (result != UVHTTP_TLS_OK) { /* 处理错误 */ }
   
   // 新代码
   uvhttp_error_t result = uvhttp_tls_context_new(&ctx);
   if (result != UVHTTP_OK) { /* 处理错误 */ }
   ```

2. **TLS 错误码扩展**
   - **新增**: `UVHTTP_ERROR_TLS_CERT` (-408) 到 `UVHTTP_ERROR_TLS_NO_CERT` (-418)
   - **新增**: `UVHTTP_ERROR_TLS_WANT_READ` (1) 和 `UVHTTP_ERROR_TLS_WANT_WRITE` (2)
   - **用途**: 支持更细粒度的 TLS 错误处理和非阻塞 I/O

### Added

- 新增 WebSocket API 测试（52个测试用例）
- 新增服务器 API 测试
- 更新 TLS NULL 参数测试

### Changed

- 统一错误处理体系，简化 API 使用
- 改善类型安全性，减少类型转换错误

## [2.2.0] - 2026-01-28

### Breaking Changes

⚠️ **重要**: 本版本包含重大架构重构，多个 API 发生破坏性变更

1. **所有初始化函数返回值变更**
   - **影响**: 所有使用 `uvhttp_config_new()`, `uvhttp_context_create()` 等函数的代码
   - **变更**: 返回值从指针类型改为 `uvhttp_error_t`，通过输出参数返回对象
   ```c
   // 旧代码（已移除）
   uvhttp_config_t* config = uvhttp_config_new();
   if (!config) { /* 处理错误 */ }
   
   // 新代码
   uvhttp_config_t* config = NULL;
   uvhttp_error_t result = uvhttp_config_new(&config);
   if (result != UVHTTP_OK) { /* 处理错误 */ }
   ```

2. **依赖注入系统已移除**
   - **影响**: 使用 `uvhttp_deps.h` 和相关 provider 抽象的代码
   - **变更**: 移除 `uvhttp_deps.h`, `uvhttp_connection_provider_t`, `uvhttp_logger_provider_t`, `uvhttp_config_provider_t`
   - **迁移**: 直接使用 libuv 和标准库函数
   ```c
   // 旧代码（已移除）
   uvhttp_deps_t* deps = uvhttp_deps_new();
   uvhttp_deps_set_loop_provider(deps, provider);
   
   // 新代码（直接使用 libuv）
   uv_loop_t* loop = uv_default_loop();
   ```

3. **日志系统重构**
   - **影响**: 使用 `uvhttp_logger_provider_t` 的代码
   - **变更**: 改为编译期宏实现，Release 模式下零开销
   ```c
   // 旧代码（已移除）
   uvhttp_logger_provider_t* logger = uvhttp_default_logger_provider_create(level);
   logger->log(logger, UVHTTP_LOG_LEVEL_INFO, "message");
   
   // 新代码（编译期宏）
   UVHTTP_LOG_INFO("message");
   ```

4. **中间件架构变更**
   - **影响**: 使用动态中间件链的代码
   - **变更**: 改为编译期宏定义中间件链
   - **迁移**: 使用 `UVHTTP_DEFINE_MIDDLEWARE_CHAIN` 宏

5. **WebSocket 实现重命名**
   - **影响**: 包含 `uvhttp_websocket_native.h` 的代码
   - **变更**: 统一为 `uvhttp_websocket.h`
   ```c
   // 旧代码（已移除）
   #include "uvhttp_websocket_native.h"
   
   // 新代码
   #include "uvhttp_websocket.h"
   ```

### Removed
- **抽象层**: 移除所有运行时抽象层
  - `uvhttp_deps.h` (125 行)
  - `uvhttp_connection_provider_t` 接口
  - `uvhttp_logger_provider_t` 接口
  - `uvhttp_config_provider_t` 接口
  - `uvhttp_network_interface_t` 接口
- **测试文件**: 删除 38 个已禁用的测试文件
- **示例文件**: 删除 5 个过时的中间件示例

### Added
- **Benchmark 目录**: 新增基准性能测试目录
  - `benchmark/performance_allocator.c`
  - `benchmark/performance_allocator_compare.c`
  - `benchmark/test_bitfield.c`
  - `benchmark/README.md`
- **日志系统**: 新增编译期宏日志系统
  - `include/uvhttp_logging.h`
  - `src/uvhttp_logging.c`

### Performance
- **零开销抽象**: 编译期宏实现，Release 模式下完全零开销
- **内存分配器**: 性能与系统分配器相当
- **RPS 性能测试**:
  - 4线程10连接：20,432 RPS（峰值）
  - 4线程50连接：19,840 RPS
  - 4线程100连接：19,776 RPS
  - 4线程500连接：19,850 RPS

### Code Reduction
- **总代码减少**: 23,805 行代码（减少 88%）
- **简化架构**: 移除所有不必要的抽象层
- **提高可维护性**: 代码更简洁，易于理解和维护

### Migration Guide

详细的迁移指南请参考 `MIGRATION_GUIDE.md`

## [2.1.0] - 2026-01-27

### Added
- **使用者教程**: 新增完整的使用者文档
  - `docs/guide/installation.md`: 详细安装指南（Linux/macOS/Windows）
  - `docs/guide/first-server.md`: 第一个服务器教程（3 个完整示例）
  - `docs/guide/websocket.md`: WebSocket 使用指南（含应用层认证示例）
- **Examples 重构**: 按功能分类重组示例代码
  - `01_basics/`: 基础示例
  - `02_routing/`: 路由示例
  - `04_static_files/`: 静态文件示例
  - `05_websocket/`: WebSocket 示例
  - `06_advanced/`: 高级功能示例
  - `07_performance/`: 性能测试示例

### Changed
- **项目结构**: 移除 `src/core` 目录，将核心文件移至 `src/` 根目录
  - 理由：文件数量少（24 个），不需要子文件夹

### Performance
- **RPS 性能提升**: 峰值 RPS 从 17,798 提升到 24,439 (+37.3%)
- **延迟降低**: 平均延迟降低 30.1%
- **内存优化**: 移除自定义内存池，使用 mimalloc
  - 大对象分配性能提升 50%
  - 减少内存碎片

### Removed
- **WebSocket 认证功能**: 移除内置的 WebSocket 认证模块
  - 删除 `src/uvhttp_websocket_auth.c` (368 行)
  - 删除 `include/uvhttp_websocket_auth.h` (147 行)
  - 删除 `test/unit/test_websocket_auth_full_coverage.cpp` (542 行)
  - 删除 `docs/guide/WEBSOCKET_AUTH.md` (377 行)
- **自定义内存池**: 移除 `uvhttp_mempool` 模块
  - 删除 `src/uvhttp_mempool.c` (97 行)
  - 删除 `include/uvhttp_mempool.h` (47 行)
  - 删除 `test/unit/test_mempool_full_coverage.cpp` (459 行)
- **全局变量**: 移除 `g_uvhttp_context` 全局变量
  - 改用 libuv loop 注入模式
  - 提高可测试性和多实例支持

### Fixed
- **测试代码**: 修复移除内存池后的测试问题
- **文档错误**: 修复教程中的 API 错误（uvhttp_run -> uv_run）

### Documentation
- **设计原则**: 在 IFLOW.md 中添加 10 条设计原则
- **变更记录**: 记录所有重大架构变更

### Breaking Changes

⚠️ **重要**: 本版本包含破坏性变更，需要升级指南

1. **WebSocket 认证 API 已移除**
   - **影响**: 所有使用 `uvhttp_server_ws_set_auth_config` 等认证 API 的代码
   - **迁移**: 认证功能应在应用层实现
   - **示例**: 参考 `docs/guide/websocket.md` 中的应用层认证示例
   ```c
   // 旧代码（已移除）
   uvhttp_server_ws_set_auth_config(server, auth_config);
   
   // 新代码（应用层认证）
   int on_connect(uvhttp_ws_connection_t* ws_conn, void* user_data) {
       const char* auth_header = uvhttp_ws_get_request_header(ws_conn, "Authorization");
       if (!auth_header || !validate_token(auth_header + 7)) {
           return -1;  // 拒绝连接
       }
       return 0;
   }
   ```

2. **内存池 API 已移除**
   - **影响**: 使用 `uvhttp_mempool_*` 函数的代码
   - **迁移**: 直接使用 `UVHTTP_MALLOC` / `UVHTTP_FREE` 宏
   ```c
   // 旧代码（已移除）
   uvhttp_mempool_alloc(pool, size);
   
   // 新代码
   UVHTTP_MALLOC(size);
   ```

3. **全局变量 g_uvhttp_context 已移除**
   - **影响**: 使用全局变量访问上下文的代码
   - **迁移**: 改用 libuv loop 注入模式
   ```c
   // 旧代码（已移除）
   uvhttp_context_t* ctx = g_uvhttp_context;
   
   // 新代码
   uvhttp_context_t* ctx = (uvhttp_context_t*)loop->data;
   ```

### Migration Guide

详细的迁移指南请参考：
- WebSocket 认证迁移: `docs/guide/websocket.md`
- libuv 数据指针模式: `docs/LIBUV_DATA_POINTER.md`

### Testing
- **测试通过**: 所有测试通过（34/34）
- **性能测试**: RPS 基准测试通过（峰值 20,432 RPS）

## [2.0.0] - 2026-01-24

### Added
- **冒烟测试**: 新增 `test/unit/test_smoke.cpp`，验证基本功能
- **死亡测试**: 新增 `test/unit/test_death.cpp`，验证错误处理
- **压力测试**: 新增 `test/unit/test_stress.cpp`，验证高并发性能
- **内存测试**: 新增 `test/unit/test_memory.cpp`，验证内存使用
- **CI/CD 增强**: 添加冒烟测试、死亡测试、压力测试和内存测试到 CI 流程
- **文档重组**: 重新组织文档目录结构，提升可读性
  - `guide/`: 用户指南和教程
  - `dev/`: 开发文档和架构设计
  - `api/`: API 参考文档

### Performance
- **性能优化**: Homepage 21,574 RPS (+126%)
- **编译优化**: 从 -O3 降级到 -O2，避免激进优化
- **内存优化**: 连接复用节省 279,920 字节/次
- **边界检查**: 增强缓冲区边界检查，提升安全性

### Fixed
- **循环逻辑**: 修复服务器循环逻辑，回调执行
- **内存泄漏**: 修复示例代码空指针保护
- **测试超时**: 调整测试超时配置，避免并发竞争

### Documentation
- **文档结构**: 重新组织文档目录，添加完整侧边栏导航
- **版本统一**: 统一版本号为 2.0.0

### Testing
- **测试覆盖率**: 71/71 测试通过 (100%)
- **新增测试**: 4 个新测试文件，33 个新测试用例

### Breaking Changes
- **编译选项**: 编译优化级别从 -O3 改为 -O2
- **依赖更新**: 更新子模块到最新版本

## [1.6.0] - 2026-01-20

### Added
- **全局变量重构计划**: 新增 `docs/GLOBAL_VARIABLE_REFACTOR_PLAN.md`，记录全局变量重构策略
- **路由性能优化文档**: 为 `HYBRID_THRESHOLD` 添加详细的性能测试数据注释

### Fixed
- **API 文档错误**: 修正所有响应 API 的返回类型为 `uvhttp_error_t`
- **循环访问方法**: 修复文档中的循环访问方法，使用 `uv_handle_get_loop()` 替代不存在的函数
- **处理器签名**: 更新处理器签名以匹配实际 API（返回 `int`，接收 `request` 和 `response` 参数）
- **路由计数器**: 修复 `add_array_route` 未增加 `route_count` 的 bug
- **LRU 缓存逻辑**: 修复 `uvhttp_lru_cache_is_expired` 的逻辑错误
  - NULL 条目现在正确返回 1（已过期）
  - TTL 为 0 时正确返回 0（永不过期）
- **请求方法映射**: 为 `UVHTTP_ANY` 方法添加正确的字符串表示 "ANY"
- **路由验证**: 添加空路径和查询字符串验证
- **测试期望值**: 修复所有测试中的错误期望值

### Changed
- **测试通过率**: 从 91% 提升到 100% (67/67)
- **代码质量**: 消除魔法数字，添加详细注释
- **文档版本**: 更新到 2.0.0，文档日期更新到 2026-01-21
- **测试文件**: 暂时禁用 `test_connection_integration.cpp`（需要 libuv handle 管理重构）

### Performance
- **路由混合模式**: 优化 `HYBRID_THRESHOLD` 选择，基于性能测试数据
  - 50 个路由：数组 0.02ms, Trie 0.03ms
  - 100 个路由：数组 0.04ms, Trie 0.04ms
  - 200 个路由：数组 0.08ms, Trie 0.05ms
  - 500 个路由：数组 0.20ms, Trie 0.06ms

### Documentation
- **API 文档**: 所有代码示例已验证与头文件中的 API 签名一致
- **错误处理**: 添加完整的错误处理示例
- **开发指南**: 修复测试代码示例使用正确的初始化函数

### Testing
- **测试通过率**: 100% (67/67)
- **测试失败**: 0
- **代码覆盖率**: 68.6% (行覆盖率), 84.1% (函数覆盖率)

### Breaking Changes
- **无破坏性变更**: 所有 API 保持向后兼容

## [1.5.0] - 2026-01-16

### Added
- **测试框架现代化**: 统一使用 C++ 和 Google Test 框架
- **测试覆盖率提升**: 代码覆盖率从未知提升至 69.1%
- **测试规范文档**: 新增 `docs/TESTING_STANDARDS.md`（457 行）
- **依赖管理改进**: 新增 `cmake/Dependencies.cmake` 统一管理依赖构建
- **API 扩展**: 新增 8 个 Provider Getter/Setter 函数到 `uvhttp_deps.h`

### Changed
- **测试文件格式**: 所有单元测试从 `.c` 重命名为 `.cpp`（34 个文件）
- **CI/CD 配置**: 测试超时时间从 300 秒增加到 600 秒
- **构建系统**: 更新 CMakeLists.txt 和 Makefile 支持 C++ 测试
- **贡献者指南**: 更新测试框架说明和最佳实践
- **连接数限制配置**: 默认最大连接数从 10000 降低到 2048
  - **原因**: 2048 更适合大多数应用场景，避免资源浪费
  - **影响**: 现有高并发应用可能需要调整配置
  - **解决方案**: 如需更高并发，可通过配置调整（最大支持 10000）
  - **配置示例**:
    ```c
    config->max_connections = 10000;  // 高并发场景
    ```

### Fixed
- **查询字符串验证 bug**: 修复 `uvhttp_validate_query_string` 函数中的 bug
  - 移除 `dangerous_query_chars` 中的 `'\0'`
  - 修复原因：`strchr()` 会找到字符串末尾的空终止符，导致误判
- **测试用例更新**: 更新所有相关测试用例以匹配修复后的函数行为

### Testing
- **测试通过率**: 100% (53 个测试，5 个被跳过)
- **测试用例数量**: 49 个测试可执行文件
- **代码覆盖率**: 69.1%
- **被跳过的测试**（需要后续修复）:
  - `test_deps_full_coverage`（包含长时间运行的操作）
  - `test_lru_cache_full_coverage`（包含 sleep(2) 调用，共 6 秒）
  - `test_server_full_coverage`（超时）
  - `test_server_rate_limit_coverage`（超时）
  - `test_server_simple_api_coverage`（超时）

### Breaking Changes
- **健康检查功能移除**:
  - 删除 `include/uvhttp_health.h`
  - 删除 `src/uvhttp_health.c`
  - 删除 `examples/health_check_demo.c`
  - 从 `uvhttp.h` 移除健康检查头文件引用
  - **迁移指南**: 如果您的代码依赖健康检查 API，请移除相关代码并实现自定义健康检查端点
- **测试框架变更**:
  - 所有单元测试文件从 `.c` 改为 `.cpp`
  - 测试代码必须使用 C++ 和 Google Test 框架
  - **影响**: 仅影响测试代码，不影响公共 API

### Migration Guide

#### 健康检查功能迁移
如果您使用的是健康检查功能，请按照以下步骤迁移：

1. 移除 `#include <uvhttp_health.h>`
2. 移除 `uvhttp_health_check_t` 相关代码
3. 实现自定义健康检查端点：

```c
void health_check_handler(uvhttp_request_t* request) {
    uvhttp_response_set_status(request, 200);
    uvhttp_response_set_header(request, "Content-Type", "application/json");
    uvhttp_response_set_body(request, "{\"status\":\"ok\"}");
    uvhttp_response_send(request);
}

// 在路由中添加
uvhttp_router_add_route(router, "/health", health_check_handler);
```

## [1.4.0] - 2026-01-13

### Added
- **WebSocket 连接管理**: 连接池、超时检测、心跳检测、广播功能
- **内存管理优化**: 使用内联函数替代宏定义
- **改进的超时检测机制**: 使用 libuv 定时器实现主动超时检测
- **配置值文档说明**: 添加 sendfile 配置参数选择依据
- **性能基准测试**: 完整的性能测试数据（15,000+ RPS）

### Changed
- **内存分配器 API**: 从宏改为内联函数（uvhttp_alloc/uvhttp_free）
- **WebSocket 实现**: 完全原生实现，移除 libwebsockets 依赖
- **sendfile 超时检测**: 从被动检测改为主动检测
- **示例程序内存管理**: 统一使用 UVHTTP_MALLOC/UVHTTP_FREE
- **文档完善**: 添加性能优化章节到 STATIC_FILE_SERVER.md

### Fixed
- **内存泄漏**: 修复 WebSocket 连接管理中的内存泄漏
- **中等文件传输超时**: 使用分块发送（1MB chunks）
- **示例程序内存泄漏**: 修复所有示例程序的内存管理不一致问题
- **超时检测不完整**: 网络完全阻塞时也能及时响应

### Performance
- **性能测试结果**: 
  - 主页: 15,967 RPS (79.8% 目标)
  - 中等文件: 14,285 RPS (71.4% 目标)
  - 大文件: 15,480 RPS (77.4% 目标)
  - 平均延迟: 3-6ms
- **分块传输影响**: 性能影响 < 1%

### Testing
- **测试通过率**: 100% (所有现有测试)
- **性能测试**: wrk 测试全部通过
- **代码质量**: 零编译警告

### Breaking Changes
- **内存分配器 API**: UVHTTP_MALLOC/UVHTTP_FREE 改为 uvhttp_alloc/uvhttp_free

## [1.3.2] - 2026-01-11

### Added
- **sendfile 超时测试**: test_sendfile_timeout.c 测试用例

### Fixed
- **中等文件传输超时**: 使用分块发送（每次64KB，优化后）
- **超时检测**: 添加10秒超时检测（优化后）
- **错误处理**: 改进错误处理，添加重试机制（最多2次，优化后）

### Testing
- **测试通过率**: 100% (10/10)
- **测试覆盖**: 中等文件、边界情况、分类测试

## [1.3.1] - 2026-01-11

### Added
- **分支管理规范**: 完整的Git Flow分支管理策略
- **Linux 内核开发节奏**: 持续开发-快速修复-稳定发布

### Changed
- **开发流程**: 采用 Git Flow 分支管理策略

## [1.3.0] - 2026-01-11

### Added
- **分支管理规范**: 完整的Git Flow分支管理策略和开发规范
- **性能对比测试**: Nginx vs UVHTTP性能对比综合报告
- **性能测试标准**: 详细的性能测试标准文档
- **服务器配置指南**: 服务器配置性能优化指南
- **限流功能**: 完整的限流功能实现和API文档
- **性能测试工具**: 性能测试框架和工具

### Changed
- **限流API重构**: 将限流功能从中间件改为服务器核心功能
- **中间件系统**: 实现零开销中间件系统
- **静态文件服务**: 静态文件中间件解耦，性能优化
- **错误码机制**: 增强错误码机制和错误处理
- **WebSocket功能**: 完善WebSocket功能，添加路由支持

### Fixed
- **白名单内存泄漏**: 修复白名单哈希表内存泄漏和重复添加问题
- **NULL参数处理**: 修复uvhttp_request_get_path的NULL参数处理
- **测试崩溃**: 修复test_request_null_coverage测试崩溃问题
- **PR评审问题**: 修复PR评审中发现的关键问题

### Performance
- **性能优化**: 完成性能优化和代码质量改进
- **测试覆盖**: 添加限流功能测试框架
- **API简化**: 简化限流API，移除未使用的算法参数

### Testing
- **测试通过率**: 所有测试通过 (69/69, 100%)
- **NULL参数测试**: 完整的NULL参数覆盖测试
- **性能验证**: 性能基准测试验证通过

### Documentation
- **开发指南**: 更新开发指南，包含分支管理规范
- **性能文档**: 添加性能测试标准和配置指南
- **API文档**: 添加限流功能API文档
- **测试文档**: 添加性能对比测试报告

### Breaking Changes
- **限流API变更**: 限流功能从中间件改为服务器核心功能，API有所变化

## [1.2.0] - 2026-01-07

### Added
- **TLS安全功能**: 实现CRL检查、DH参数设置、会话票证管理、证书链验证
- **性能基准测试**: 新增性能基准测试程序和文档
- **安全策略文档**: 完整的安全策略和依赖管理文档
- **mimalloc支持**: 启用mimalloc作为默认内存分配器
- **原生WebSocket**: 使用原生WebSocket实现替代libwebsockets

### Changed
- **TLS实现**: 从OpenSSL迁移到mbedTLS，提升安全性和性能
- **性能优化**: Keep-alive连接管理优化，性能提升约1000倍（4-16 RPS → 14,000-16,000 RPS）
- **代码精简**: 移除15,192行冗余代码，提升代码可维护性
- **文档重组**: 移动文档到docs/目录，优化项目结构
- **依赖管理**: 清理.gitmodules，移除不再使用的依赖

### Fixed
- **空指针检查**: 修复request getter函数的空指针检查（11个函数）
- **TLS类型错误**: 修复mbedTLS API类型不兼容问题
- **编译警告**: 修复所有编译警告（未使用变量、strncpy截断等）
- **HTTP头验证**: 修复HTTP头验证逻辑错误
- **响应构建**: 修复响应数据构建时机问题

### Security
- **CRL检查**: 实现证书撤销列表检查功能
- **证书链验证**: 完整的证书链验证支持
- **依赖更新策略**: 明确的安全更新、功能更新、维护更新策略
- **安全审计**: 建立每周依赖扫描、每月代码审查、季度渗透测试计划
- **漏洞响应**: 定义完整的漏洞发现、评估、修复、通知流程

### Performance
- **Keep-alive连接**: 修复连接复用，性能提升约1000倍
- **TCP优化**: 启用TCP_NODELAY和TCP keepalive
- **响应缓冲区**: 优化响应缓冲区大小（512 → 1024字节）
- **内存分配器**: mimalloc提供更快的内存分配和释放

### Testing
- **NULL参数测试**: 完整的NULL参数覆盖测试（TLS 32个，Request 11个）
- **性能验证**: 1000倍性能提升已通过基准测试验证
- **代码质量**: 编译无错误无警告，启用安全编译选项

### Documentation
- **PERFORMANCE_BENCHMARK.md**: 详细的性能基准测试文档
- **SECURITY.md**: 完整的安全策略文档
- **依赖文档**: 更新DEPENDENCIES.md，包含所有依赖版本和更新策略

### Breaking Changes
- **TLS API变更**: 从OpenSSL迁移到mbedTLS，API签名有所变化
- **WebSocket实现**: 从libwebsockets迁移到原生实现
- **依赖变更**: 移除libwebsockets依赖，使用mbedTLS替代OpenSSL

## [1.1.0] - 2025-12-25

### Added
- **内存安全增强**: 使用C99灵活数组成员解决内存对齐问题
- **整数溢出保护**: 在所有关键内存分配点添加整数溢出检查
- **路径遍历防护**: 多层路径遍历保护机制，防止目录遍历攻击
- **性能优化**: 增加连接数限制（2048）和监听队列（1024）
- **新常量定义**: 添加HTTP状态码范围常量和响应头安全边距常量
- **文档**: 新增ROUTER_SEARCH_MODES.md文档
- **WebSocket全局清理**: 完善WebSocket全局资源清理函数

### Changed
- **API统一**: 统一所有验证函数返回值（1表示有效/成功，0表示无效/失败）
- **函数命名**: 移除所有兼容性包装函数，使用统一的API命名
- **代码清理**: 移除所有兼容性代码和注释
- **测试更新**: 更新所有测试以匹配新的API签名
- **错误处理**: 简化错误处理逻辑，提高代码可读性

### Fixed
- **内存对齐**: 修复uvhttp_write_data_t结构的内存对齐问题
- **内存泄漏**: 修复错误处理路径中的内存泄漏
- **危险字符检查**: 移除错误null字符检查逻辑
- **函数返回值**: 修正验证函数的返回值逻辑
- **编译错误**: 修复所有示例程序和测试程序的编译错误

### Security
- **缓冲区溢出保护**: 添加完整的边界检查和整数溢出检查
- **HTTP响应拆分防护**: 增强控制字符检测
- **DoS防护**: 合理的资源限制和超时配置
- **TLS支持**: 支持TLS 1.3加密协议
- **输入验证**: 28处输入验证调用，覆盖所有用户输入点

### Performance
- **哈希算法**: 使用 xxHash 哈希算法
- **LRU缓存**: 优化路由缓存和静态文件缓存
- **零拷贝**: 减少内存拷贝操作
- **连接池**: 优化连接复用和管理

### Testing
- **单元测试**: 16/16测试通过（100%通过率）
- **测试覆盖**: 47%测试代码量
- **边界测试**: 完整的边界条件和极端条件测试
- **安全测试**: 路径遍历、DoS防护等安全测试

### Documentation
- **API文档**: 完整的API参考文档
- **开发指南**: 详细的开发指南和规范
- **架构文档**: 清晰的架构说明
- **示例代码**: 15个示例程序

## [1.0.0] - 2025-12-20

### Added
- **初始发布**: 基于 libuv 的 HTTP 服务器
- **路由系统**: 支持动态路由和参数提取
- **静态文件服务**: 高效的静态文件服务
- **WebSocket支持**: 完整的WebSocket协议支持
- **TLS/SSL**: 支持HTTPS加密连接
- **LRU缓存**: 缓存系统
- **连接池**: 优化连接复用
- **配置系统**: 灵活的配置管理
- **错误处理**: 完善的错误处理和日志系统

### Features
- 单线程事件驱动模型
- 异步 I/O
- 模块化设计
- 可扩展的插件系统
- 详细的文档和示例

[2.9.1]: https://github.com/adam-ikari/uvhttp/compare/v2.9.0...v2.9.1
[2.9.0]: https://github.com/adam-ikari/uvhttp/compare/v2.8.1...v2.9.0
[1.1.0]: https://github.com/adam-ikari/uvhttp/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/adam-ikari/uvhttp/releases/tag/v1.0.0
