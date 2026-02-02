#!/usr/bin/env node
/**
 * 简单拆分 API 文档为多个文件
 * 只做拆分，不添加任何复杂功能
 * 
 * @example
 * node split_api_md.js
 */

const fs = require('fs');
const path = require('path');

// 读取 API 文档
const apiFile = path.join(process.cwd(), 'docs/api/api.md');
const content = fs.readFileSync(apiFile, 'utf-8');

// 提取所有结构体
const structPattern = /# struct `([^`]+)`\n([\s\S]*?)(?=# struct `|$)/g;
const structs = [];
let match;

while ((match = structPattern.exec(content)) !== null) {
  structs.push({
    name: match[1],
    content: match[2]
  });
}

// 创建输出目录
const outputDir = path.join(process.cwd(), 'docs/api/structs');
if (!fs.existsSync(outputDir)) {
  fs.mkdirSync(outputDir, { recursive: true });
}

// 为每个结构体创建独立文件
for (const struct of structs) {
  const fileName = struct.name.toLowerCase().replace(/ /g, '_');
  const filePath = path.join(outputDir, `${fileName}.md`);
  
  const fileContent = `# ${struct.name}

${struct.content}
`;
  
  fs.writeFileSync(filePath, fileContent, 'utf-8');
}

// 创建主索引文件
const indexContent = `# API 文档索引

## 结构体列表

${structs.map(s => `- [\`${s.name}\`](./structs/${s.name.toLowerCase().replace(/ /g, '_')}.md)`).join('\n')}
`;

fs.writeFileSync(apiFile, indexContent, 'utf-8');

console.log(`✅ 已拆分为 ${structs.length} 个结构体文件`);
console.log(`📁 输出目录: ${outputDir}`);
