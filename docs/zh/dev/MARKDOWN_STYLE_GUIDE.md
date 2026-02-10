# Markdown 样式指南

本文档提供了编写 Markdown 文档的样式指南，确保文档在 Markdown 阅读器和 VitePress 网站中都有良好的可读性。

## 基本原则

1. **清晰简洁**: 使用简单明了的语言
2. **结构化**: 使用标题、列表、表格等组织内容
3. **一致性**: 遵循统一的格式和风格
4. **可访问性**: 确保所有用户都能理解内容

## 标题层级

### 使用规范

```markdown
# 一级标题（文档标题）

## 二级标题（主要章节）

### 三级标题（子章节）

#### 四级标题（细节说明）

##### 五级标题（补充信息）

###### 六级标题（极少使用）
```

### 注意事项

- 每个文档只有一个一级标题
- 标题层级不要跳过（如从 H1 直接跳到 H3）
- 标题要简洁明了，能准确描述内容
- 使用 `-` 或 `=` 标记 H1 和 H2（可选）

## 段落和换行

### 段落

```markdown
这是第一个段落。

这是第二个段落。

段落之间用空行分隔。
```

### 换行

```markdown
这是第一行，  
这是第二行（使用两个空格 + 回车）。

或者使用 HTML 标签：
这是第一行<br>这是第二行。
```

### 注意事项

- 段落之间用空行分隔
- 避免使用连续的空行（最多一个空行）
- 长段落考虑拆分为多个短段落

## 文本格式

### 强调

```markdown
**粗体文本**
*斜体文本*
***粗斜体文本***
~~删除线文本~~
```

### 代码

```markdown
行内代码：`code`

代码块：
```
代码内容
```

带语言标识的代码块：
```c
int main() {
    return 0;
}
```
```

### 链接

```markdown
[链接文本](https://example.com)
[链接文本](/relative/path)
[链接文本](/relative/path "提示文本")

自动链接：
<https://example.com>
```

### 注意事项

- 链接文本要描述清楚链接内容
- 使用相对路径链接到项目内文档
- 外部链接添加 `target="_blank"`（需要 HTML）

## 列表

### 无序列表

```markdown
- 项目 1
- 项目 2
  - 子项目 2.1
  - 子项目 2.2
- 项目 3
```

### 有序列表

```markdown
1. 第一步
2. 第二步
3. 第三步
```

### 任务列表

```markdown
- [x] 已完成任务
- [ ] 未完成任务
- [ ] 待办事项
```

### 注意事项

- 列表项之间用空行分隔（可选）
- 列表缩进使用 2 或 4 个空格
- 避免过深的嵌套（最多 3 层）

## 表格

### 基本表格

```markdown
| 列 1 | 列 2 | 列 3 |
|------|------|------|
| 数据 1 | 数据 2 | 数据 3 |
| 数据 4 | 数据 5 | 数据 6 |
```

### 对齐方式

```markdown
| 左对齐 | 居中 | 右对齐 |
|:-------|:----:|-------:|
| 数据 1 | 数据 2 | 数据 3 |
| 数据 4 | 数据 5 | 数据 6 |
```

### 注意事项

- 表格标题行使用 `|` 分隔
- 使用 `-` 分隔标题和内容
- 列宽会自动调整
- 复杂表格考虑使用 HTML

## 引用块

### 基本引用

```markdown
> 这是一段引用文本。
>
> 可以包含多行。
```

### 嵌套引用

```markdown
> 外层引用
>
> > 内层引用
```

### 注意事项

- 引用块用于强调重要内容
- 避免过度使用引用块
- 引用块可以包含其他 Markdown 元素

## 代码块

### 基本语法

```markdown
```
代码内容
```
```

### 指定语言

```markdown
```c
int main() {
    return 0;
}
```

```yaml
name: Example Workflow
on: [push]
```
```

### 行号（VitePress）

```markdown
```c {1,3-5}
int main() {
    printf("Hello");
    return 0;
}
```
```

### 注意事项

- 始终指定代码语言以启用语法高亮
- 代码块使用 4 个空格缩进
- 长代码考虑拆分或折叠

## 分隔线

```markdown
---
***
___
```

### 注意事项

- 分隔线用于分隔主要内容
- 不要过度使用分隔线
- 使用一致的样式（推荐 `---`）

## 图片

### 基本语法

```markdown
![替代文本](/path/to/image.png)
![替代文本](/path/to/image.png "提示文本")
```

### HTML 语法（更多控制）

```html
<img src="/path/to/image.png" alt="替代文本" width="500">

<figure>
  <img src="/path/to/image.png" alt="替代文本">
  <figcaption>图片说明</figcaption>
</figure>
```

### 注意事项

- 始终提供替代文本
- 图片放在 `docs/public/` 目录
- 使用相对路径
- 大图片考虑压缩

## 链接和引用

### 内部链接

```markdown
[文档标题](/path/to/document.md)
[文档标题](./relative-path.md)
```

### 外部链接

```markdown
[GitHub](https://github.com)
[GitHub](https://github.com "GitHub 首页")
```

### 锚点链接

```markdown
[跳转到标题](#标题)
[跳转到其他文档的标题](/path/to/document.md#标题)
```

### 注意事项

- 内部链接使用相对路径
- 外部链接添加描述
- 避免使用裸 URL

## 特殊字符

### 转义字符

```markdown
\*不是斜体\*
\`不是代码\`
\\不是换行
```

### HTML 实体

```markdown
&copy; 2024
&trade;
&reg;
```

### 注意事项

- 特殊字符需要转义
- 使用 HTML 实体显示特殊符号
- 代码块中的字符不需要转义

## VitePress 特定语法

### 自定义容器

```markdown
::: tip 提示
这是一条提示信息。
:::

::: warning 警告
这是一条警告信息。
:::

::: danger 危险
这是一条危险信息。
:::

::: info 信息
这是一条普通信息。
:::

::: details 详情
<summary>点击查看详情</summary>

这是详细内容。
:::
```

### 徽章

```html
<span class="badge">默认</span>
<span class="badge success">成功</span>
<span class="badge warning">警告</span>
<span class="badge error">错误</span>
<span class="badge info">信息</span>
```

### 注意事项

- 自定义容器用于强调重要内容
- 徽章用于标记状态或类型
- 避免过度使用特殊样式

## 最佳实践

### 1. 文档结构

```markdown
# 文档标题

简要描述文档内容。

## 目录

本文档包含以下内容：
- 章节 1
- 章节 2
- 章节 3

## 章节 1

内容...

## 章节 2

内容...

## 相关文档

- [相关文档 1](/path/to/doc1.md)
- [相关文档 2](/path/to/doc2.md)

## 参考资料

- [参考 1](https://example.com)
- [参考 2](https://example.com)
```

### 2. 代码示例

```markdown
### 示例代码

```c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

**说明**：
- 第 1 行：包含标准输入输出库
- 第 3 行：定义主函数
- 第 4 行：打印消息
- 第 5 行：返回成功状态
```

### 3. 注意事项

- ✅ 使用清晰的标题层级
- ✅ 提供代码示例和说明
- ✅ 添加相关文档链接
- ✅ 使用一致的格式
- ❌ 避免过深的嵌套
- ❌ 避免使用 HTML（除非必要）
- ❌ 避免过度使用格式

## 可访问性

### 1. 替代文本

```markdown
✅ 好的做法
![服务器架构图](/images/architecture.png "系统架构图")

❌ 不好的做法
![图片](/images/architecture.png)
```

### 2. 链接描述

```markdown
✅ 好的做法
[查看 GitHub Actions 文档](https://docs.github.com/actions)

❌ 不好的做法
点击[这里](https://docs.github.com/actions)
```

### 3. 代码注释

```markdown
```c
// 计算两个数的和
int sum = a + b;
```
```

## 检查清单

在提交文档前，检查以下项目：

- [ ] 标题层级正确且不跳级
- [ ] 段落之间有适当的空行
- [ ] 代码块指定了语言
- [ ] 表格格式正确
- [ ] 链接可以正常访问
- [ ] 图片有替代文本
- [ ] 特殊字符已转义
- [ ] 代码示例可以运行
- [ ] 相关文档链接正确
- [ ] 在 VitePress 中预览效果

## 工具推荐

### Markdown 编辑器

- **VS Code**: 配合 Markdown All in One 插件
- **Typora**: 所见即所得编辑器
- **Obsidian**: 知识管理工具

### 验证工具

- **Markdownlint**: 检查 Markdown 语法
- **VitePress**: 预览网站效果
- **GitHub**: 预览 Markdown 阅读器效果

## 相关资源

- [Markdown 官方规范](https://commonmark.org/)
- [VitePress 文档](https://vitepress.vuejs.org/)
- [GitHub Flavored Markdown](https://github.github.com/gfm/)
- [Markdownlint 规则](https://github.com/DavidAnson/markdownlint)

---

**最后更新**: 2026-01-26
**维护者**: UVHTTP Team