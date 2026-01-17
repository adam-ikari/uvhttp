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
      { 
        text: '语言 / Language', 
        items: [
          { text: '中文', link: '/' },
          { text: 'English', link: '/en/' }
        ]
      },
      { text: 'GitHub', link: 'https://github.com/adam-ikari/uvhttp' }
    ],
    
    sidebar: {
      '/guide/': [
        { text: '快速开始', link: '/guide/getting-started' },
        { text: '架构设计', link: '/guide/architecture' },
        { text: '路由', link: '/guide/routing' },
        { text: '中间件', link: '/guide/middleware' },
        { text: 'WebSocket', link: '/guide/websocket' },
        { text: '性能优化', link: '/guide/performance' },
        { text: '最佳实践', link: '/guide/best-practices' }
      ],
      '/api/': [
        { text: 'API 介绍', link: '/api/introduction' }
      ],
      '/en/guide/': [
        { text: 'Quick Start', link: '/en/guide/getting-started' },
        { text: 'Architecture', link: '/en/guide/architecture' },
        { text: 'Routing', link: '/en/guide/routing' },
        { text: 'Middleware', link: '/en/guide/middleware' },
        { text: 'WebSocket', link: '/en/guide/websocket' },
        { text: 'Performance', link: '/en/guide/performance' },
        { text: 'Best Practices', link: '/en/guide/best-practices' }
      ],
      '/en/api/': [
        { text: 'API Introduction', link: '/en/api/introduction' }
      ]
    },
    
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
