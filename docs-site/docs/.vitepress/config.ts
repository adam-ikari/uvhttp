import { defineConfig } from 'vitepress'

// https://vitepress.vuejs.org/config/app-configs
export default defineConfig({
  title: 'UVHTTP',
  description: '高性能 HTTP/1.1 和 WebSocket 服务器库',
  base: '/uvhttp/',
  lang: 'zh-CN',
  
  themeConfig: {
    nav: [
      { text: '首页', link: '/' },
      { text: '指南', link: '/guide/getting-started' },
      { text: 'API', link: '/api/introduction' },
      { text: '性能', link: '/performance' },
      { text: '版本', link: '/versions' },
      { text: 'GitHub', link: 'https://github.com/adam-ikari/uvhttp' }
    ],
    
    sidebar: false,
    
    search: {
      provider: 'local'
    },
    
    footer: {
      message: '基于 MIT 许可证发布',
      copyright: 'Copyright © 2024-present UVHTTP Contributors'
    },
    
    socialLinks: [
      { icon: 'github', link: 'https://github.com/adam-ikari/uvhttp' }
    ]
  }
})
