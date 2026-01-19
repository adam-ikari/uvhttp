import { defineConfig } from 'vitepress'

// https://vitepress.vuejs.org/config/app-configs
export default defineConfig({
  title: 'UVHTTP',
  description: '高性能 HTTP/1.1 和 WebSocket 服务器库',
  base: '/uvhttp/',
  lang: 'zh-CN',

  head: [
    ['meta', { name: 'keywords', content: 'HTTP, WebSocket, libuv, C, 高性能, 服务器, 异步 I/O' }],
    ['meta', { name: 'author', content: 'UVHTTP Contributors' }],
    ['meta', { name: 'viewport', content: 'width=device-width,initial-scale=1' }],
    ['link', { rel: 'icon', href: '/favicon.svg', type: 'image/svg+xml' }],
    ['meta', {
      'http-equiv': 'Content-Security-Policy',
      content: "default-src 'self'; script-src 'self' 'unsafe-inline' 'unsafe-eval'; style-src 'self' 'unsafe-inline'; img-src 'self' data: https:;"
    }]
  ],

  themeConfig: {
    nav: [
      { text: '首页', link: '/' },
      { text: '指南', link: '/guide/getting-started' },
      { text: 'API', link: '/api/introduction' },
      { text: '性能', link: '/performance' },
      { text: '版本', link: '/versions' },
      { text: 'GitHub', link: 'https://github.com/adam-ikari/uvhttp' }
    ],
    
    sidebar: {
      '/guide/': [
        {
          text: '指南',
          items: [
            { text: '快速开始', link: '/guide/getting-started' }
          ]
        }
      ],
      '/api/': [
        {
          text: 'API 文档',
          items: [
            { text: 'API 介绍', link: '/api/introduction' }
          ]
        }
      ]
    },
    
    search: {
      provider: 'local',
      options: {
        locales: {
          root: {
            placeholder: '搜索文档',
            translations: {
              button: {
                buttonText: '搜索文档',
                buttonAriaLabel: '搜索文档'
              },
              modal: {
                noResultsText: '无法找到相关结果',
                resetButtonTitle: '清除查询条件',
                footer: {
                  selectText: '选择',
                  navigateText: '切换'
                }
              }
            }
          }
        }
      }
    },
    
    footer: {
      message: '基于 MIT 许可证发布',
      copyright: 'Copyright © 2024-present UVHTTP Contributors'
    },
    
    socialLinks: [
      { icon: 'github', link: 'https://github.com/adam-ikari/uvhttp' }
    ],

    lastUpdated: {
      text: '最后更新',
      formatOptions: {
        dateStyle: 'full',
        timeStyle: 'short'
      }
    },

    markdown: {
      lineNumbers: true
    }
  }
})
