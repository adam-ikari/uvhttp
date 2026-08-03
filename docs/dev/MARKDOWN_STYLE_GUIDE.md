# Markdown Style Guide

This document provides style guidelines for writing Markdown documentation, ensuring good readability in both Markdown readers and the VitePress site.

## Basic Principles

1. **Clear and concise**: use simple, clear language
2. **Structured**: use headings, lists, tables, etc. to organize content
3. **Consistent**: follow a uniform format and style
4. **Accessible**: ensure all users can understand the content

## Heading Hierarchy

### Usage

```markdown
# H1 (document title)

## H2 (main sections)

### H3 (sub-sections)

#### H4 (details)

##### H5 (supplementary information)

###### H6 (rarely used)
```

### Notes

- Each document has exactly one H1 heading
- Do not skip heading levels (e.g. from H1 directly to H3)
- Headings should be clear and concise, accurately describing the content
- Using `-` or `=` for H1 and H2 is optional

## Paragraphs and Line Breaks

### Paragraphs

```markdown
This is the first paragraph.

This is the second paragraph.

Paragraphs are separated by a blank line.
```

### Line Breaks

```markdown
This is the first line,  
this is the second line (two spaces + carriage return).

Or use an HTML tag:
First line<br>Second line.
```

### Notes

- Separate paragraphs with a blank line
- Avoid consecutive blank lines (at most one)
- Consider splitting long paragraphs into multiple short ones

## Text Formatting

### Emphasis

```markdown
**Bold text**
*Italic text*
***Bold italic text***
~~Strikethrough text~~
```

### Code

```markdown
Inline code: `code`

Code block:
```
Code content
```

Code block with language identifier:
```c
int main() {
    return 0;
}
```
```

### Links

```markdown
[Link text](https://example.com)
[Link text](/relative/path)
[Link text](/relative/path "tooltip text")

Automatic links:
<https://example.com>
```

### Notes

- Link text should clearly describe the link target
- Use relative paths to link to project-internal documents
- Add `target="_blank"` for external links (requires HTML)

## Lists

### Unordered Lists

```markdown
- Item 1
- Item 2
  - Sub-item 2.1
  - Sub-item 2.2
- Item 3
```

### Ordered Lists

```markdown
1. First step
2. Second step
3. Third step
```

### Task Lists

```markdown
- [x] Completed task
- [ ] Incomplete task
- [ ] To-do item
```

### Notes

- Separate list items with a blank line (optional)
- Indent list items with 2 or 4 spaces
- Avoid overly deep nesting (at most 3 levels)

## Tables

### Basic Table

```markdown
| Column 1 | Column 2 | Column 3 |
|------|------|------|
| Data 1 | Data 2 | Data 3 |
| Data 4 | Data 5 | Data 6 |
```

### Alignment

```markdown
| Left-aligned | Centered | Right-aligned |
|:-------|:----:|-------:|
| Data 1 | Data 2 | Data 3 |
| Data 4 | Data 5 | Data 6 |
```

### Notes

- Separate the header row with `|`
- Use `-` to separate header and content
- Column widths adjust automatically
- Consider using HTML for complex tables

## Block Quotes

### Basic Quotes

```markdown
> This is a quoted paragraph.
>
> It can span multiple lines.
```

### Nested Quotes

```markdown
> Outer quote
>
> > Inner quote
```

### Notes

- Use block quotes to emphasize important content
- Avoid overusing block quotes
- Block quotes can contain other Markdown elements

## Code Blocks

### Basic Syntax

```markdown
```
Code content
```
```

### Specifying Language

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

### Line Numbers (VitePress)

```markdown
```c {1,3-5}
int main() {
    printf("Hello");
    return 0;
}
```
```

### Notes

- Always specify the code language to enable syntax highlighting
- Use 4-space indentation in code blocks
- Consider splitting or collapsing long code

## Horizontal Rules

```markdown
---
***
___
```

### Notes

- Use horizontal rules to separate major content
- Do not overuse horizontal rules
- Use a consistent style (`---` is recommended)

## Images

### Basic Syntax

```markdown
![Alt text](/path/to/image.png)
![Alt text](/path/to/image.png "tooltip text")
```

### HTML Syntax (More Control)

```html
<img src="/path/to/image.png" alt="Alt text" width="500">

<figure>
  <img src="/path/to/image.png" alt="Alt text">
  <figcaption>Image caption</figcaption>
</figure>
```

### Notes

- Always provide alt text
- Place images in the `docs/public/` directory
- Use relative paths
- Consider compressing large images

## Links and References

### Internal Links

```markdown
[Document title](/path/to/document.md)
[Document title](./relative-path.md)
```

### External Links

```markdown
[GitHub](https://github.com)
[GitHub](https://github.com "GitHub Home")
```

### Anchor Links

```markdown
[Jump to a heading](#heading)
[Jump to a heading in another document](/path/to/document.md#heading)
```

### Notes

- Use relative paths for internal links
- Add descriptions to external links
- Avoid bare URLs

## Special Characters

### Escaped Characters

```markdown
\*not italic\*
\`not code\`
\\not a line break
```

### HTML Entities

```markdown
&copy; 2024
&trade;
&reg;
```

### Notes

- Special characters need escaping
- Use HTML entities to display special symbols
- Characters inside code blocks do not need escaping

## VitePress-Specific Syntax

### Custom Containers

```markdown
::: tip Tip
This is a tip message.
:::

::: warning Warning
This is a warning message.
:::

::: danger Danger
This is a danger message.
:::

::: info Info
This is an informational message.
:::

::: details Details
<summary>Click to view details</summary>

This is the detailed content.
:::
```

### Badges

```html
<span class="badge">Default</span>
<span class="badge success">Success</span>
<span class="badge warning">Warning</span>
<span class="badge error">Error</span>
<span class="badge info">Info</span>
```

### Notes

- Use custom containers to emphasize important content
- Use badges to mark status or type
- Avoid overusing special styling

## Best Practices

### 1. Document Structure

```markdown
# Document Title

Brief description of the document content.

## Table of Contents

This document contains the following:
- Section 1
- Section 2
- Section 3

## Section 1

Content...

## Section 2

Content...

## Related Documents

- [Related document 1](/path/to/doc1.md)
- [Related document 2](/path/to/doc2.md)

## References

- [Reference 1](https://example.com)
- [Reference 2](https://example.com)
```

### 2. Code Examples

```markdown
### Example Code

```c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

**Explanation**:
- Line 1: includes the standard input/output library
- Line 3: defines the main function
- Line 4: prints a message
- Line 5: returns a success status
```

### 3. Notes

- Use a clear heading hierarchy
- Provide code examples and explanations
- Add links to related documents
- Use consistent formatting
- Avoid overly deep nesting
- Avoid HTML unless necessary
- Avoid overusing formatting

## Accessibility

### 1. Alt Text

```markdown
Good practice
![Server architecture diagram](/images/architecture.png "System architecture diagram")

Bad practice
![Image](/images/architecture.png)
```

### 2. Link Descriptions

```markdown
Good practice
[View GitHub Actions documentation](https://docs.github.com/actions)

Bad practice
Click [here](https://docs.github.com/actions)
```

### 3. Code Comments

```markdown
```c
// Calculate the sum of two numbers
int sum = a + b;
```
```

## Checklist

Before committing documentation, check the following items:

- [ ] Headings use correct levels and do not skip levels
- [ ] Paragraphs have appropriate blank lines between them
- [ ] Code blocks specify a language
- [ ] Table formatting is correct
- [ ] Links work
- [ ] Images have alt text
- [ ] Special characters are escaped
- [ ] Code examples run
- [ ] Related document links are correct
- [ ] Preview in VitePress to check rendering

## Recommended Tools

### Markdown Editors

- **VS Code**: with the Markdown All in One extension
- **Typora**: WYSIWYG editor
- **Obsidian**: knowledge management tool

### Validation Tools

- **Markdownlint**: checks Markdown syntax
- **VitePress**: previews the site
- **GitHub**: previews the Markdown reader rendering

## Related Resources

- [Markdown Specification](https://commonmark.org/)
- [VitePress Documentation](https://vitepress.vuejs.org/)
- [GitHub Flavored Markdown](https://github.github.com/gfm/)
- [Markdownlint Rules](https://github.com/DavidAnson/markdownlint)

---

**Last Updated**: 2026-01-26
**Maintainer**: UVHTTP Team
