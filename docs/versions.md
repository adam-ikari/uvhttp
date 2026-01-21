# 文档版本

本文档提供 UVHTTP 的版本信息。

## 当前版本

**2.0.0** - 最新稳定版本

- [查看文档](/)
- [GitHub Release](https://github.com/adam-ikari/uvhttp/releases/tag/v2.0.0)
- [更新日志](https://github.com/adam-ikari/uvhttp/blob/main/CHANGELOG.md)

## 版本说明

**2.0.0** 是 UVHTTP 的最新稳定版本，推荐用于生产环境。该版本完全支持，持续更新。

## 历史版本

当前没有历史版本。历史版本将在发布新版本后添加到此页面。

## 查看历史版本

如果您需要查看历史版本的信息，请访问 [GitHub Releases](https://github.com/adam-ikari/uvhttp/releases) 页面。

## 版本管理说明

### 添加新版本

当发布新版本时，需要更新以下文件：

1. **更新版本配置** (`docs/.vitepress/versions.json`)
   ```json
   {
     "current": "2.1.0",
     "versions": [
       {
         "version": "2.1.0",
         "status": "current",
         "releaseDate": "2026-02-01",
         "url": "/",
         "githubUrl": "https://github.com/adam-ikari/uvhttp/releases/tag/v2.1.0"
       },
       {
         "version": "2.0.0",
         "status": "limited",
         "releaseDate": "2026-01-21",
         "url": "https://github.com/adam-ikari/uvhttp/releases/tag/v2.0.0",
         "githubUrl": "https://github.com/adam-ikari/uvhttp/releases/tag/v2.0.0"
       }
     ]
   }
   ```

2. **更新版本页面** (`docs/versions.md`)
   - 在"历史版本"部分添加新版本信息
   - 更新版本状态

### 版本状态说明

- **current** - 当前版本，完全支持，持续更新
- **security** - 安全更新版本，仅提供关键安全修复
- **limited** - 有限支持版本，仅维护关键安全更新
- **deprecated** - 不再支持版本，存在已知问题

## 获取帮助

如果您在使用过程中遇到问题：

- 📖 查看文档：[API 参考](/api/introduction) | [开发者指南](/DEVELOPER_GUIDE.md)
- 💬 提交问题：[GitHub Issues](https://github.com/adam-ikari/uvhttp/issues)
- 🗣️ 社区讨论：[GitHub Discussions](https://github.com/adam-ikari/uvhttp/discussions)