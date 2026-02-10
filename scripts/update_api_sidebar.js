#!/usr/bin/env node
/**
 * 自动更新 VitePress API 文档侧边栏配置
 * 
 * @example
 * node update_api_sidebar.js
 */

const fs = require('fs');
const path = require('path');

/**
 * Update API sidebar configuration
 * @returns {boolean} True if successful, false otherwise
 */
function updateApiSidebar() {
  const docsDir = path.join(process.cwd(), 'docs/api');
  
  if (!fs.existsSync(docsDir)) {
    console.error(` Markdown 目录不存在: ${docsDir}`);
    return false;
  }
  
  // Check if api.md exists
  const apiMdPath = path.join(docsDir, 'api.md');
  if (!fs.existsSync(apiMdPath)) {
    console.error(` api.md 不存在: ${apiMdPath}`);
    return false;
  }
  
  // Read structs directory
  const structsDir = path.join(docsDir, 'structs');
  let structItems = [];
  
  if (fs.existsSync(structsDir)) {
    const files = fs.readdirSync(structsDir).filter(f => f.endsWith('.md'));
    structItems = files.map(file => {
      const name = file.replace('.md', '').replace(/_/g, ' ');
      return { text: name, link: `/api/structs/${file.replace('.md', '')}` };
    }).sort((a, b) => a.text.localeCompare(b.text));
  }
  
  // Generate sidebar configuration
  let sidebarConfig = `// 自动生成的 API 文档侧边栏配置
// 由 scripts/update_api_sidebar.js 生成
export default [
  {
    text: 'API Reference',
    items: [
      { text: 'API 介绍', link: '/api/introduction' },
      { text: 'API 参考', link: '/api/API_REFERENCE' },
      { text: 'API 文档', link: '/api/api' },
      
      {
        text: '结构体',
        collapsed: false,
        items: [
${structItems.map(item => `          { text: '${item.text}', link: '${item.link}' },`).join('\n')}
        ]
      },

    ]
  }
]
`;
  
  // Write sidebar.js
  const sidebarFile = path.join(docsDir, 'sidebar.js');
  try {
    fs.writeFileSync(sidebarFile, sidebarConfig, 'utf-8');
    console.log(` API 侧边栏配置已更新: ${sidebarFile}`);
    return true;
  } catch (error) {
    console.error(` 写入侧边栏配置失败: ${error.message}`);
    return false;
  }
}

// Main execution
try {
  const success = updateApiSidebar();
  process.exit(success ? 0 : 1);
} catch (error) {
  console.error(` 错误: ${error.message}`);
  process.exit(1);
}
