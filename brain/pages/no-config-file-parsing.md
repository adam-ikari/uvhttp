---
id: no-config-file-parsing
title: "不做配置文件解析：YAML/JSON/TOML 等配置格式不归本库"
category: decision
status: active
tags: [config, scope, roadmap]
created: "2026-10-17T17:24:20"
updated: "2026-10-17T17:47:47"
---

<!-- compiled_truth -->
## 约束（2026-10-17 用户确认，记录为项目约束）

**本库不引入任何配置文件解析**：不做 YAML / TOML / INI 解析，也不提供"从配置文件读服务器参数"的能力。已核查全库（src/、include/、scripts/、.github/）确无 YAML 解析代码，本条为约束声明而非清理任务。

## 边界

- 运行时参数只经公开 C API 设置（`uvhttp_server_config*` 系列 setter）。配置文件的**格式、解析、校验归嵌入方应用自己拥有**——与 `docs/guide/ROADMAP.md` 既有表述一致（"YAML/JSON config files: The framework or application embedding this library owns its own configuration format"）。
- `.yaml` / `.yml` 出现在压缩扩展名名单里（`src/uvhttp_response.c` 的文本类型列表）只是按扩展名判定 MIME 类型，不是解析，不违反本约束。
- cJSON 是依赖选型（栈内已定），用途为构造/解析 **HTTP 报文体**；除非未来出现真实需求，不把它升级为配置文件 parser。
- HTTP/3、QUIC、per-user 限流等已一并砍出的 roadmap 承诺（#431、#433）不因本页面重开。

## 理由

引入配置 parser 会带来新依赖、新攻击面与长期维护负担，且与"库被嵌入、宿主掌管策略"的定位冲突：宿主框架各有配置习惯（Spring 用 YAML、Go 服务用 TOML、C 项目多用 CLI/env），库替宿主选格式必然选错。


## Timeline

- time: 2026-10-17T17:24:20
  kind: decision
  summary: "Created this page: 不做配置文件解析：YAML/JSON/TOML 等配置格式不归本库"
  source: "用户约束 2026-10-17"
  affects: [no-config-file-parsing]

- time: 2026-10-17T17:47:47
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "用户约束 2026-10-17；brain create-page 不读 stdin，首次创建只落了脚手架"
  affects: [no-config-file-parsing]
