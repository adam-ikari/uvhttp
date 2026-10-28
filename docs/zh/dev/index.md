# 开发者文档

如果你要参与库本身的开发（而不是嵌入使用），从这里开始。

## 快速开始

```bash
git switch main && git pull --ff-only
git switch -c fix/your-fix

make build          # Debug 构建
make test           # ctest
make check-docs     # 中英文同步 + 内部链接

git push -u origin fix/your-fix
gh pr create --base main
```

PR CI 只做编译和静态检查，**不执行测试**。开 PR 前请在本地跑 `make test`；
完整矩阵在 prerelease 门禁中执行。

## 核心指南

| 文档 | 内容 |
|---|---|
| [开发者指南](../guide/DEVELOPER_GUIDE.md) | 环境、构建命令、代码规范、内存管理、审查清单 |
| [测试指南](../guide/TESTING_GUIDE.md) | 测试发现、ctest、GoogleTest、libuv mock、sanitizer、覆盖率 |
| [构建模式](BUILD_MODES.md) | Release / Debug / Coverage 的语义与参数 |
| [CI/CD](CI_CD.md) | 每个工作流的触发条件、job 与本地复现命令 |
| [依赖说明](DEPENDENCIES.md) | 内置 submodule 与升级流程 |

## 架构与设计

| 文档 | 内容 |
|---|---|
| [架构设计](ARCHITECTURE.md) | 分层结构与模块职责 |
| [路由设计哲学](ROUTER_DESIGN_PHILOSOPHY.md) | 路由模型与取舍 |
| [路由查找模式](ROUTER_SEARCH_MODES.md) | 查找策略及其成本 |
| [头文件包含顺序](HEADER_INCLUDE_ORDER.md) | include 顺序规则 |
| [前向声明](FORWARD_DECLARATION_AND_CIRCULAR_DEPENDENCY.md) | 打破循环依赖 |
| [XXHash 集成](XXHASH_INTEGRATION.md) | 哈希依赖与用法 |

## 质量与流程

| 文档 | 内容 |
|---|---|
| [测试规范](TESTING_STANDARDS.md) | 测试命名、结构与有效性要求 |
| [可测试性指南](TESTABILITY_GUIDE.md) | 为确定性测试设计代码 |
| [性能测试标准](PERFORMANCE_TESTING_STANDARD.md) | 基准构造与 CI 门禁规则 |
| [Markdown 风格指南](MARKDOWN_STYLE_GUIDE.md) | 文档格式规范 |
| [开发节奏](../../development-rhythm.md) | 每周节奏与复盘产物 |
| [发布策略](../../release-strategy.md) | 预发布、性能门禁与转正流程 |

## 历史文档

`DEVELOPMENT_PLAN.md`、`GLOBAL_VARIABLE_REFACTOR_PLAN.md`、`CI_CD_DESIGN.md`
描述的是早期计划或设计，仅作背景保留；当前行为以上面的指南为准。
