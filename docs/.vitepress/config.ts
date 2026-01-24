import { defineConfig } from 'vitepress'

// https://vitepress.vuejs.org/config/app-configs
export default defineConfig({
  title: 'UVHTTP',
  description: '高性能 HTTP/1.1 和 WebSocket 服务器库',
  // 开发环境使用根路径，生产环境（GitHub Pages）使用 /uvhttp/
  // 修改为生产环境时，将下面的 '/' 改为 '/uvhttp/'
  base: '/uvhttp',
  lang: 'zh-CN',

  ignoreDeadLinks: true,

  head: [
    ['meta', { name: 'keywords', content: 'HTTP, WebSocket, libuv, C, 高性能, 服务器, 异步 I/O' }],
    ['meta', { name: 'author', content: 'UVHTTP Contributors' }],
    ['meta', { name: 'viewport', content: 'width=device-width,initial-scale=1' }],
    ['link', { rel: 'icon', href: '/favicon.svg', type: 'image/svg+xml' }],
    ['meta', {
      'http-equiv': 'Content-Security-Policy',
      content: "default-src 'self'; script-src 'self' 'unsafe-eval'; style-src 'self' 'unsafe-inline'; img-src 'self' data: https://adam-ikari.github.io; object-src 'none'; base-uri 'self'; form-action 'self';"
    }],
    ['meta', { property: 'og:title', content: 'UVHTTP - 高性能 HTTP/1.1 和 WebSocket 服务器库' }],
    ['meta', { property: 'og:description', content: '基于 libuv 事件驱动架构，峰值吞吐量达 16,832 RPS' }],
    ['meta', { property: 'og:type', content: 'website' }],
    ['meta', { property: 'og:url', content: 'https://adam-ikari.github.io/uvhttp' }],
    ['meta', { property: 'og:site_name', content: 'UVHTTP' }],
    ['meta', { name: 'twitter:card', content: 'summary_large_image' }],
    ['meta', { name: 'twitter:title', content: 'UVHTTP - 高性能 HTTP/1.1 和 WebSocket 服务器库' }],
    ['meta', { name: 'twitter:description', content: '基于 libuv 事件驱动架构，峰值吞吐量达 16,832 RPS' }],
    ['link', { rel: 'canonical', href: 'https://adam-ikari.github.io/uvhttp/' }]
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
          text: '入门指南',
          items: [
            { text: '介绍', link: '/guide/introduction' },
            { text: '快速开始', link: '/guide/getting-started' }
          ]
        },
        {
          text: '核心概念',
          items: [
            { text: '教程', link: '/guide/TUTORIAL' },
            { text: 'libuv 数据指针', link: '/guide/LIBUV_DATA_POINTER' },
            { text: '中间件系统', link: '/guide/MIDDLEWARE_SYSTEM' },
            { text: '统一响应指南', link: '/guide/UNIFIED_RESPONSE_GUIDE' }
          ]
        },
        {
          text: '功能模块',
          items: [
            { text: '限流 API', link: '/guide/RATE_LIMIT_API' },
            { text: '静态文件服务', link: '/guide/STATIC_FILE_SERVER' },
            { text: 'WebSocket 认证', link: '/guide/WEBSOCKET_AUTH' }
          ]
        },
        {
          text: '开发指南',
          items: [
            { text: '开发者指南', link: '/guide/DEVELOPER_GUIDE' }
          ]
        }
      ],
      '/api/': [
        {
          text: 'API 文档',
          items: [
            { text: 'API 介绍', link: '/api/introduction' },
            { text: 'API 参考', link: '/api/API_REFERENCE' }
          ]
        }
      ],
      '/dev/': [
        {
          text: '架构设计',
          items: [
            { text: '架构设计', link: '/dev/ARCHITECTURE' },
            { text: '依赖说明', link: '/dev/DEPENDENCIES' },
            { text: 'XXHash 集成', link: '/dev/XXHASH_INTEGRATION' }
          ]
        },
        {
          text: '开发计划',
          items: [
            { text: '开发计划', link: '/dev/DEVELOPMENT_PLAN' },
            { text: '全局变量重构', link: '/dev/GLOBAL_VARIABLE_REFACTOR_PLAN' },
            { text: '路线图', link: '/dev/ROADMAP' }
          ]
        },
        {
          text: '测试与质量',
          items: [
            { text: '可测试性指南', link: '/dev/TESTABILITY_GUIDE' },
            { text: '测试标准', link: '/dev/TESTING_STANDARDS' },
            { text: '性能测试标准', link: '/dev/PERFORMANCE_TESTING_STANDARD' },
            { text: '性能基准', link: '/dev/PERFORMANCE_BENCHMARK' }
          ]
        },
        {
          text: '参考文档',
          items: [
            { text: '错误码参考', link: '/dev/ERROR_CODES' },
            { text: '路由搜索模式', link: '/dev/ROUTER_SEARCH_MODES' },
            { text: '安全指南', link: '/dev/SECURITY' }
          ]
        }
      ]
    },
    
    search: {
      provider: 'local',
      options: {
        locales: {
          root: {
            // placeholder: '搜索文档',
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
    }
  }
})
