---
id: sanitizer-feature-blindspot
title: "Sanitizer 门禁的 feature 盲区：STATIC_FILES 默认 OFF 致 static 代码未被检测"
category: project
status: active
tags: [ci, asan, ubsan, coverage, static-files]
created: "2026-10-03T03:34:47"
updated: "2026-10-07T06:36:58"
---

<!-- compiled_truth -->
# Sanitizer 门禁的 feature 盲区

## 结论

`BUILD_WITH_STATIC_FILES` 与 `BUILD_WITH_ROUTER_CACHE` 在 CMakeLists.txt 中
默认 **OFF**。而 sanitizer job（PR 的 asan-gate、nightly 的 ASan 与 UBSan）
**不传任何 BUILD_WITH_\* 参数**，全用默认值 —— 于是这两个 feature 守卫的
源文件整文件不编译，sanitizer 完全看不到它们。

2026-10-03 修复前实测：

```
uvhttp_static_resolve_safe_path        ✗ 缺失（文件未编译）
uvhttp_router_cache_lookup             ✗ 缺失
uvhttp_lru_cache_put                   ✗ 缺失
```

8 个 static/LRU 测试可执行文件运行 0 个用例，仍报 PASS。其中
`resolve_safe_path` 是路径穿越防护的核心函数。

**这类盲区不产生任何失败信号**——门禁是绿的，只是根本没测。这与
「假阳性测试」同源：绿灯不代表验证过。

## 修复后暴露的三个真实缺陷

1. **`uvhttp_static_prewarm_cache` 泄漏整个文件缓冲** —— 只在失败路径
   free `file_content`。`uvhttp_lru_cache_put` 是**拷贝语义**（内部 alloc +
   memcpy，src/uvhttp_lru_cache.c:470-471），不接管调用方指针。
2. **e2e fixture 泄漏 server + loop** —— `stop()` 只在 `running_` 为 true
   时 free server，而 `StaticFileServing` 创建 server 后从不 listen。
3. **测试用 `char[sizeof(uvhttp_response_t)]` 冒充 response** —— 函数收
   `void*` 并直接转发给 `uvhttp_response_set_header`，读写结构体字段。

## 陷阱：修 server 泄漏会暴露 router 泄漏

直接把 `uvhttp_server_free` 移出 `if (running_)` → router 泄漏 36536 字节。

原因：`uvhttp_server_free` 只释放**自己持有**的 router（`server->router`），
而测试的 router 是独立 `uvhttp_router_new` 创建、仅在 `set_router()` 时
移交。正确解法是用归属标志（如 `router_attached_`）区分释放方。

原代码注释「zero-initialised server 不能 free」针对的是**从未
`uvhttp_server_new` 过**的结构，与「new 过但没 listen」无关——混淆这两者
会导致错误结论。

## 仍未覆盖

`BUILD_WITH_ROUTER_CACHE` 默认 OFF，`uvhttp_router_cache.c` 不在 sanitizer
构建里。它与 `uvhttp_router.c` 是**互斥的两套实现**（后者被
`#if !UVHTTP_FEATURE_ROUTER_CACHE` 包裹），单个 job 无法同时覆盖。需要
额外 job 或 nightly 轮换。

## 方法论：查 sanitizer 盲区的方法

```bash
# 1. 哪些源文件被 feature 守卫
for f in src/*.c; do
  head -1 "$f" | grep -qE '^#if (!?)(UVHTTP_FEATURE_)' && echo "$(basename $f): $(head -1 $f)"
done

# 2. 库中是否真的有该文件的符号（比 .o 文件检查可靠）
nm dist/lib/libuvhttp.a | grep -w uvhttp_static_resolve_safe_path

# 3. 哪些测试是空壳（运行 0 用例）
./dist/bin/test_static_api_coverage   # → 0 tests from 0 test suite
```

**符号检查比检查 .o 文件可靠**——`find CMakeFiles/*.dir -name '*.c.o'`
会误匹配。


## Timeline

- time: 2026-10-03T03:34:47
  kind: decision
  summary: "Created this page: Sanitizer 门禁的 feature 盲区：STATIC_FILES 默认 OFF 致 static 代码未被检测"
  source: "2026-10-03 修复 PR #449"
  affects: [sanitizer-feature-blindspot]

- time: 2026-10-03T03:34:47
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #449"
  affects: [sanitizer-feature-blindspot]

- time: 2026-10-07T06:36:58
  kind: reversal
  summary: "盲区已修：ci-release-gate 的 asan-gate 与 ci-nightly 的 test-ubsan / coverage(ubuntu-build-all) 三处 cmake 已加 -DBUILD_WITH_ROUTER_CACHE=ON。至此 BUILD_WITH_STATIC_FILES 与 BUILD_WITH_ROUTER_CACHE 两个默认 OFF 的 feature 在 ASan/UBSan/coverage 下均纳入编译——router_cache/lru_cache 约 772 行代码不再处于 sanitizer 盲区。本地 ASan+ROUTER_CACHE=ON 跑 119 测试全过、零泄漏验证安全。"
  source: "2026-10-07 修复"
  affects: [sanitizer-feature-blindspot]
