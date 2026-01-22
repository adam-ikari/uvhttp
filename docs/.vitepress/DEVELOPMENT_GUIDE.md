# UVHTTP 网站开发规范

本文档记录 UVHTTP 文档网站的开发规范，包括文档编写、代码示例、提交规范等。

## 📚 文档编写规范

### 1. 文档语言

- **主要语言**: 中文（简体）
- **技术术语**: 保留英文原文，首次出现时提供中文解释
- **代码注释**: 使用中文注释

### 2. 文档格式

#### Markdown 规范

- 使用标准的 Markdown 语法
- 标题层级清晰，最多使用 3 级标题（`###`）
- 使用列表、表格、代码块等增强可读性
- 使用适当的空行分隔段落

#### 标题规范

```markdown
# 一级标题（仅用于文档标题）
## 二级标题（主要章节）
### 三级标题（子章节）
```

**注意事项:**
- 每个文档只有一个一级标题
- 标题使用简洁明了的中文
- 避免使用空格和特殊字符

#### 列表规范

**无序列表:**
```markdown
- 项目 1
- 项目 2
  - 子项目 2.1
  - 子项目 2.2
```

**有序列表:**
```markdown
1. 第一步
2. 第二步
3. 第三步
```

**注意事项:**
- 列表项之间用空行分隔
- 子列表缩进 2 个空格
- 列表项末尾使用句号

### 3. 代码示例规范

#### 代码块语法

使用三个反引号包裹代码，并指定语言：

```markdown
\`\`\`c
#include "uvhttp.h"

int main() {
    return 0;
}
\`\`\`
```

**支持的语言:**
- `c` - C 语言
- `bash` - Shell 命令
- `json` - JSON 数据
- `markdown` - Markdown 代码
- `text` - 纯文本

#### 代码注释

- **C 代码**: 使用中文注释
- **Shell 命令**: 使用中文注释
- **示例代码**: 必须有注释说明

**示例:**
```c
#include "uvhttp.h"

// 创建服务器
uvhttp_server_t* server = uvhttp_server_new(loop);

// 设置路由
uvhttp_router_add_route(router, "/", handler);

// 启动服务器
uvhttp_server_listen(server, "0.0.0.0", 8080);
```

#### 代码示例要求

1. **完整性**: 代码示例必须完整可运行
2. **简洁性**: 避免冗余代码，突出核心逻辑
3. **注释**: 关键步骤必须有注释
4. **格式**: 使用 4 空格缩进，遵循 C 代码规范
5. **错误处理**: 示例代码应包含基本的错误处理

**错误处理示例:**
```c
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: %s\n", uvhttp_error_string(result));
    return 1;
}
```

### 4. 文档结构规范

#### 标准文档结构

```markdown
# 文档标题

## 简介
简要说明文档内容和目标读者

## 前置条件
列出阅读本文档需要的先决条件

## 主要内容
文档的核心内容

## 示例
提供完整可运行的示例

## 注意事项
重要提示和警告

## 相关链接
相关文档和资源
```

#### 文档元数据

在文档顶部添加元数据（如果需要）：

```markdown
---
title: 文档标题
description: 文档描述
---
```

### 5. 链接规范

#### 内部链接

使用相对路径链接到其他文档：

```markdown
[快速开始](/guide/getting-started)
[API 参考](/api/introduction)
```

#### 外部链接

使用完整的 URL：

```markdown
[GitHub](https://github.com/adam-ikari/uvhttp)
[libuv](https://libuv.org/)
```

#### 注意事项

- 链接文本使用简洁的中文描述
- 避免使用"点击这里"等模糊描述
- 外部链接在新标签页打开（VitePress 默认行为）

### 6. 图片规范

#### 图片格式

- 使用 SVG 格式（图标、logo）
- 使用 PNG/JPEG 格式（截图、照片）

#### 图片路径

```markdown
![图片描述](/path/to/image.png)
```

**注意事项:**
- 图片放在 `docs/public/` 目录
- 使用相对路径引用
- 图片必须有描述文本

### 7. 表格规范

#### 表格格式

```markdown
| 列1 | 列2 | 列3 |
|-----|-----|-----|
| 数据1 | 数据2 | 数据3 |
| 数据4 | 数据5 | 数据6 |
```

**注意事项:**
- 表格必须有表头
- 列对齐使用 `-` 符号
- 避免过宽的表格

## 🎨 样式规范

### 1. 文本样式

```markdown
**粗体文本** - 强调重要内容
*斜体文本* - 强调术语或变量
`代码文本` - 标识代码、命令、文件名
~~删除线~~ - 标识已删除或过时的内容
```

### 2. 提示框

使用 VitePress 的自定义提示框：

```markdown
::: tip 提示
这是一条提示信息
:::

::: warning 警告
这是一条警告信息
:::

::: danger 危险
这是一条危险警告
:::

::: info 信息
这是一条信息
:::

::: details 详情
<summary>点击查看详情</summary>
详细内容
:::
```

### 3. Emoji 使用

适当使用 emoji 增强可读性：

```markdown
✅ 完成
❌ 错误
⚠️ 警告
💡 提示
📚 文档
🚀 开始
```

**注意事项:**
- 不要过度使用 emoji
- 在标题和列表中使用
- 保持一致性

## 📝 提交规范

### 1. 提交信息格式

使用约定式提交（Conventional Commits）：

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Type 类型:**
- `docs`: 文档更新
- `feat`: 新功能
- `fix`: 修复 bug
- `refactor`: 重构
- `style`: 样式调整
- `chore`: 构建/工具链更新
- `test`: 测试

**Scope 范围:**
- `guide`: 使用者文档
- `dev`: 开发者文档
- `api`: API 文档
- `config`: 配置文件
- `site`: 网站配置

**示例:**
```
docs(guide): 添加快速开始指南

- 新增安装步骤
- 添加第一个服务器示例
- 更新相关链接

Closes #123
```

### 2. 提交频率

- 每完成一个完整的功能或章节提交一次
- 避免频繁的小提交
- 提交前确保文档可以正常构建

### 3. 分支管理

- `main` - 主分支，稳定版本
- `dev` - 开发分支
- `feature/*` - 功能分支
- `fix/*` - 修复分支

## 🔧 配置规范

### 1. VitePress 配置

配置文件位置: `docs/.vitepress/config.ts`

**配置要求:**
- 使用 TypeScript 编写
- 遵循项目现有配置风格
- 更新配置后测试网站构建

### 2. 侧边栏配置

配置文件位置: `docs/.vitepress/sidebar.js`

**配置要求:**
- 使用 JavaScript 编写
- 保持清晰的层级结构
- 使用 emoji 图标分组
- 定期同步文档结构

### 3. 包管理

- 使用 `pnpm` 作为包管理器
- 定期更新依赖
- 记录版本变更

## 🧪 测试规范

### 1. 本地测试

```bash
# 安装依赖
cd docs
pnpm install

# 启动开发服务器
pnpm dev

# 构建生产版本
pnpm build

# 预览生产版本
pnpm preview
```

### 2. 链接检查

```bash
# 检查死链
pnpm build
```

### 3. 浏览器测试

- Chrome/Edge
- Firefox
- Safari
- 移动端浏览器

## 📊 质量标准

### 1. 文档质量

- ✅ 内容准确、完整
- ✅ 示例代码可运行
- ✅ 链接有效
- ✅ 格式规范
- ✅ 无错别字

### 2. 可读性

- ✅ 语言简洁明了
- ✅ 逻辑清晰
- ✅ 结构合理
- ✅ 适当的示例

### 3. 维护性

- ✅ 易于更新
- ✅ 版本信息清晰
- ✅ 变更记录完整

## 🚀 发布流程

### 1. 预发布检查

- [ ] 所有文档更新完成
- [ ] 链接检查通过
- [ ] 本地构建成功
- [ ] 浏览器测试通过
- [ ] 更新版本信息

### 2. 发布步骤

```bash
# 1. 构建网站
cd docs
pnpm build

# 2. 提交更改
git add .
git commit -m "docs: 更新文档到 v2.0.0"

# 3. 推送到远程
git push origin main

# 4. GitHub Actions 自动部署
```

### 3. 发布后验证

- [ ] 访问网站确认部署成功
- [ ] 检查所有链接
- [ ] 验证搜索功能
- [ ] 测试移动端显示

## 📞 联系方式

如有问题或建议，请联系：
- GitHub Issues: https://github.com/adam-ikari/uvhttp/issues
- GitHub Discussions: https://github.com/adam-ikari/uvhttp/discussions

## 📚 参考资料

- [VitePress 官方文档](https://vitepress.dev/)
- [Markdown 指南](https://www.markdownguide.org/)
- [约定式提交](https://www.conventionalcommits.org/)