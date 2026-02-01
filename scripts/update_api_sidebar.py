#!/usr/bin/env python3
"""
自动更新 VitePress API 文档侧边栏配置
"""
import os
import glob
from pathlib import Path

def update_api_sidebar():
    docs_dir = Path("docs/api")
    markdown_dir = docs_dir / "markdown_from_xml"
    
    if not markdown_dir.exists():
        print(f"❌ Markdown 目录不存在: {markdown_dir}")
        return False
    
    # 收集所有结构体和文件文档
    structs = []
    files = []
    
    for md_file in sorted(markdown_dir.glob("struct_*.md")):
        name = md_file.stem.replace("struct_", "").replace("_", "::")
        display_name = name.replace("::", " ")
        structs.append({
            "text": display_name,
            "link": f"/api/markdown_from_xml/{md_file.name}"
        })
    
    for md_file in sorted(markdown_dir.glob("file_*.md")):
        name = md_file.stem.replace("file_", "").replace("_", "::")
        display_name = name.replace("::", " ")
        files.append({
            "text": display_name,
            "link": f"/api/markdown_from_xml/{md_file.name}"
        })
    
    # 生成 sidebar 配置
    sidebar_config = """// 自动生成的 API 文档侧边栏配置
// 由 scripts/update_api_sidebar.py 生成
export default [
  {
    text: 'API 文档',
    items: [
      { text: 'API 介绍', link: '/api/introduction' },
      { text: 'API 参考', link: '/api/API_REFERENCE' },
    ]
  },
  {
    text: '结构体文档',
    items: [
"""
    
    for item in structs:
        sidebar_config += f"      {{ text: '{item['text']}', link: '{item['link']}' }},\n"
    
    sidebar_config += """    ]
  },
  {
    text: '文件文档',
    items: [
"""
    
    for item in files:
        sidebar_config += f"      {{ text: '{item['text']}', link: '{item['link']}' }},\n"
    
    sidebar_config += """    ]
  }
]
"""
    
    # 写入 sidebar.js
    sidebar_file = docs_dir / "sidebar.js"
    with open(sidebar_file, 'w', encoding='utf-8') as f:
        f.write(sidebar_config)
    
    print(f"✅ API 侧边栏配置已更新: {sidebar_file}")
    print(f"   - {len(structs)} 个结构体文档")
    print(f"   - {len(files)} 个文件文档")
    return True

if __name__ == "__main__":
    update_api_sidebar()
