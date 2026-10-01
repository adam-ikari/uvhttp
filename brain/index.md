# Brain Index

_Auto-generated. Last updated 2026-10-01T16:08:00.893Z._

- [alloc-hotpath-measured](pages/alloc-hotpath-measured.md) — category: decision | tags: [performance, benchmark, allocator, methodology, yagni] | # 每请求分配已实测：省一次仅 0.026%，内存分配优化不做
- [benchmark-thermal-throttling](pages/benchmark-thermal-throttling.md) — category: decision | tags: [performance, benchmark, methodology] | # 禁止在本机做性能测量 —— 连配对 A/B 也不行
- [build-system-make-cmake](pages/build-system-make-cmake.md) — category: decision | tags: [build-system, cmake, make, just, embedded] | ## 决策
- [ci-format-check-gate-noop](pages/ci-format-check-gate-noop.md) — category: decision | tags: [ci, gates, format] | ## 结论
- [ci-fuzz-c11-fix](pages/ci-fuzz-c11-fix.md) — category: decision | tags: [ci, fuzz, c11, clang] | ## 根因（连续失败 5 天：2026-08-20 ~ 08-24）
- [ci-gate-real-fixes](pages/ci-gate-real-fixes.md) — category: decision | tags: [ci, gate, format, clang-format, cppcheck, dead-code, trend] | # CI 门禁修复（#416/#417/#418，2026-10-01）
- [code-quality-l3-l5-fixes](pages/code-quality-l3-l5-fixes.md) — category: decision | tags: [code-quality, gzip-cache, comments] | ## 内容（v2.7.0，commit aecb053）
- [connection-limit-503-memory-safety](pages/connection-limit-503-memory-safety.md) — category: decision | tags: [memory-safety, server, connection-limit] | # 结论
- [embedding-cmake-public-deps](pages/embedding-cmake-public-deps.md) — category: decision | tags: [embedding, cmake, build, add_subdirectory] | ## 发现
- [embedding-guide-examples](pages/embedding-guide-examples.md) — category: decision | tags: [embedding, guide, examples, add_subdirectory] | ## 交付物
- [integration-tests-are-servers](pages/integration-tests-are-servers.md) — category: decision | tags: [test, integration, assert, ci, cleanup] | # test/integration 下 19 个文件不是测试
- [io-uring-evaluation](pages/io-uring-evaluation.md) — category: decision | tags: [performance, architecture, io-uring, libuv, yagni] | # io_uring 评估：当前架构下不可达，待办关闭
- [perf-regression-gate](pages/perf-regression-gate.md) — category: decision | tags: [performance, benchmark, ci, gate] | ## 现状
- [performance-benchmark-update](pages/performance-benchmark-update.md) — category: decision | tags: [performance, benchmark, release-v2.6.2] | ## 变更（v2.7.0，commit aecb053，2026-08-21）
- [release-process-benchmark-gate](pages/release-process-benchmark-gate.md) — category: decision | tags: [release, ci, benchmark] | ## 决定
- [release-v262](pages/release-v262.md) — category: decision | tags: [release, memory-safety, websocket] | # v2.6.2 发布记录
- [release-v271](pages/release-v271.md) — category: decision | tags: [release, embedding, ci] | # v2.7.1 发布记录
- [release-v272](pages/release-v272.md) — category: decision | tags: [release, code-review] | # v2.7.2 发布记录
- [release-v280](pages/release-v280.md) — category: decision | tags: [release, performance, quality] | # v2.8.0 发布记录
- [release-v290](pages/release-v290.md) — category: decision | tags: [release, v2.9.0, benchmark, quality] | # 发布 v2.9.0
- [release-v291-prep](pages/release-v291-prep.md) — category: decision | tags: [release, v2.9.1, ci, quality, pre-release] | ## 预发布转正式的条件（来源：docs/release-strategy.md）
- [router-cache-parity](pages/router-cache-parity.md) — category: decision | tags: [router, cache, ci] | # 路由缓存实现必须与非缓存路由器行为对齐
- [tls-session-cache](pages/tls-session-cache.md) — category: decision | tags: [tls, performance, session-cache] | ## 决策（v2.7.0，commit aecb053，P0）
- [websocket-tls-pr335-merge](pages/websocket-tls-pr335-merge.md) — category: decision | tags: [websocket, tls, pr] | ## 背景
- [zerocopy-small-body-regression](pages/zerocopy-small-body-regression.md) — category: decision | tags: [performance, benchmark, writev, methodology] | 评估完毕（2026-09-30）：维持「代价大于收益」，正式关闭重构。
