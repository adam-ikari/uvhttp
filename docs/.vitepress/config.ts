import { defineConfig } from 'vitepress'
import DefaultTheme from 'vitepress/theme-without-fonts'
// 导入自动生成的 API 侧边栏配置
import apiSidebar from '../api/sidebar.js'

// https://vitepress.vuejs.org/config/app-configs
export default defineConfig({
  title: 'UVHTTP',
  description: 'High-performance HTTP/1.1 and WebSocket server library with 32-bit support and compression',
  // 本地开发使用 '/'，GitHub Pages 使用 '/uvhttp/'
  base: process.env.DEPLOY === 'gh-pages' ? '/uvhttp/' : '/',
  lang: 'en-US',
  defaultLang: 'en-US',
  i18nRouting: true,

  ignoreDeadLinks: true,

  head: [
    ['meta', { name: 'keywords', content: 'HTTP, WebSocket, libuv, C, high-performance, server, async I/O, 32-bit, embedded systems, compression' }],
    ['meta', { name: 'author', content: 'UVHTTP Contributors' }],
    ['meta', { name: 'viewport', content: 'width=device-width,initial-scale=1' }],
    ['link', { rel: 'icon', href: '/favicon.svg', type: 'image/svg+xml' }],
    ['meta', {
      'http-equiv': 'Content-Security-Policy',
      content: "default-src 'self'; script-src 'self' 'unsafe-eval'; style-src 'self' 'unsafe-inline'; img-src 'self' data: https://adam-ikari.github.io; object-src 'none'; base-uri 'self'; form-action 'self';"
    }],
    ['meta', { property: 'og:title', content: 'UVHTTP v2.5.0 - High-performance HTTP server with 32-bit support and compression' }],
    ['meta', { property: 'og:description', content: 'Peak throughput: 23,226 RPS | 32-bit embedded support | Zero-overhead compression' }],
    ['meta', { property: 'og:type', content: 'website' }],
    ['meta', { property: 'og:url', content: 'https://adam-ikari.github.io/uvhttp' }],
    ['meta', { property: 'og:site_name', content: 'UVHTTP' }],
    ['meta', { name: 'twitter:card', content: 'summary_large_image' }],
    ['meta', { name: 'twitter:title', content: 'UVHTTP v2.5.0 - High-performance HTTP server with 32-bit support and compression' }],
    ['meta', { name: 'twitter:description', content: 'Peak throughput: 23,226 RPS | 32-bit embedded support | Zero-overhead compression' }],
    ['link', { rel: 'canonical', href: 'https://adam-ikari.github.io/uvhttp/' }]
  ],

  locales: {
    root: {
      label: 'English',
      lang: 'en-US',
      themeConfig: {
        nav: [
          { text: 'Home', link: '/' },
          { text: 'Guide', link: '/guide/getting-started' },
          { text: 'API', link: '/api/introduction' },
          { text: 'Performance', link: '/guide/performance' },
          { text: 'Versions', link: '/guide/versions' },
          { 
            text: 'Download', 
            items: [
              { text: 'GitHub', link: 'https://github.com/adam-ikari/uvhttp' },
              { text: 'Release v2.5.0', link: 'https://github.com/adam-ikari/uvhttp/releases/tag/v2.5.0' }
            ]
          }
        ],
        
        sidebar: { auto: false,
          '/guide/': [
            {
              text: 'Getting Started',
              items: [
                { text: 'Introduction', link: '/guide/introduction' },
                { text: 'Quick Start', link: '/guide/getting-started' },
                { text: 'Build Guide', link: '/guide/build' },
                { text: 'Installation', link: '/guide/INSTALL_CMAKE' }
              ]
            },
            {
              text: 'Features',
              items: [
                { text: 'Static File Server', link: '/guide/STATIC_FILE_SERVER' }
              ]
            },
            {
              text: 'Advanced',
              items: [
                { text: 'Advanced Build Options', link: '/guide/ADVANCED_BUILD_OPTIONS' },
                { text: 'Build Configuration Matrix', link: '/guide/BUILD_CONFIGURATION_MATRIX' },
                { text: 'Linux Optimization', link: '/guide/LINUX_OPTIMIZATION' },
                { text: 'LRU Cache Migration', link: '/guide/MIGRATION_GUIDE_LRU_CACHE' }
              ]
            },
            {
              text: 'Reference',
              items: [
                { text: 'FAQ', link: '/guide/FAQ' },
                { text: 'Security', link: '/guide/SECURITY' },
                { text: 'Roadmap', link: '/guide/ROADMAP' },
                { text: 'Changelog', link: '/guide/CHANGELOG' },
                { text: 'Versions', link: '/guide/versions' }
              ]
            }
          ],
          '/api/': apiSidebar
        },
        
        search: {
          provider: 'local',
          options: {
            locales: {
              root: {
                translations: {
                  button: {
                    buttonText: 'Search',
                    buttonAriaLabel: 'Search'
                  },
                  modal: {
                    noResultsText: 'No results found',
                    resetButtonTitle: 'Clear query',
                    footer: {
                      selectText: 'Select',
                      navigateText: 'Navigate'
                    }
                  }
                }
              }
            }
          }
        },
        
        footer: {
          message: 'Built with ❤️ for high-performance applications | Released under MIT License',
          copyright: 'Copyright © 2024-present UVHTTP Contributors'
        },
        
        lastUpdated: {
          text: 'Last updated',
          formatOptions: {
            dateStyle: 'full',
            timeStyle: 'short'
          }
        }
      }
    },
    
    zh: {
      label: '简体中文',
      lang: 'zh-CN',
      link: '/zh/',
      themeConfig: {
        nav: [
          { text: '首页', link: '/zh/' },
          { text: '指南', link: '/zh/guide/getting-started' },
          { text: 'API', link: '/zh/api/introduction' },
          { text: '性能', link: '/zh/guide/performance' },
          { text: '版本', link: '/zh/guide/versions' },
          { 
            text: '下载', 
            items: [
              { text: 'GitHub', link: 'https://github.com/adam-ikari/uvhttp' },
              { text: 'v2.5.0 发布', link: 'https://github.com/adam-ikari/uvhttp/releases/tag/v2.5.0' }
            ]
          }
        ],
        
        sidebar: { auto: false,
          '/zh/guide/': [
            {
              text: '入门指南',
              items: [
                { text: '介绍', link: '/zh/guide/introduction' },
                { text: '快速开始', link: '/zh/guide/getting-started' },
                { text: '安装指南', link: '/zh/guide/installation' },
                { text: 'CMake 配置', link: '/zh/guide/CMAKE_CONFIGURATION' },
                { text: '第一个服务器', link: '/zh/guide/first-server' },
                { text: 'WebSocket', link: '/zh/guide/websocket' }
              ]
            },
            {
              text: '核心概念',
              items: [
                { text: '教程', link: '/zh/guide/TUTORIAL' },
                { text: 'libuv 数据指针', link: '/zh/guide/LIBUV_DATA_POINTER' },
                { text: '中间件系统', link: '/zh/guide/MIDDLEWARE_SYSTEM' },
                { text: '统一响应指南', link: '/zh/guide/UNIFIED_RESPONSE_GUIDE' }
              ]
            },
            {
              text: '功能模块',
              items: [
                { text: '限流 API', link: '/zh/guide/RATE_LIMIT_API' },
                { text: '静态文件服务', link: '/zh/guide/STATIC_FILE_SERVER' },
                { text: 'WebSocket 认证', link: '/zh/guide/WEBSOCKET_AUTH' }
              ]
            },
            {
              text: '开发指南',
              items: [
                { text: '开发者指南', link: '/zh/guide/DEVELOPER_GUIDE' }
              ]
            }
          ],
          '/zh/api/': apiSidebar,
          '/zh/dev/': [
            {
              text: '架构设计',
              items: [
                { text: '架构设计', link: '/zh/dev/ARCHITECTURE' },
                { text: '依赖说明', link: '/zh/dev/DEPENDENCIES' },
                { text: 'XXHash 集成', link: '/zh/dev/XXHASH_INTEGRATION' }
              ]
            },
            {
              text: '开发计划',
              items: [
                { text: '开发计划', link: '/zh/dev/DEVELOPMENT_PLAN' },
                { text: '全局变量重构', link: '/zh/dev/GLOBAL_VARIABLE_REFACTOR_PLAN' },
                { text: '路线图', link: '/zh/dev/ROADMAP' }
              ]
            },
            {
              text: 'CI/CD',
              items: [
                { text: 'CI/CD 工作流', link: '/zh/dev/CI_CD' }
              ]
            },
            {
              text: '文档规范',
              items: [
                { text: 'Markdown 样式指南', link: '/zh/dev/MARKDOWN_STYLE_GUIDE' }
              ]
            },
            {
              text: '测试与质量',
              items: [
                { text: '可测试性指南', link: '/zh/dev/TESTABILITY_GUIDE' },
                { text: '测试标准', link: '/zh/dev/TESTING_STANDARDS' },
                { text: '性能测试标准', link: '/zh/dev/PERFORMANCE_TESTING_STANDARD' },
                { text: '性能基准', link: '/zh/dev/PERFORMANCE_BENCHMARK' }
              ]
            },
            {
              text: '参考文档',
              items: [
                { text: '错误码参考', link: '/zh/dev/ERROR_CODES' },
                { text: '路由搜索模式', link: '/zh/dev/ROUTER_SEARCH_MODES' },
                { text: '安全指南', link: '/zh/dev/SECURITY' },
                { text: '构建配置矩阵', link: '/zh/BUILD_CONFIGURATION_MATRIX' },
                { text: '高级构建选项', link: '/zh/ADVANCED_BUILD_OPTIONS' }
              ]
            }
          ]
        },
        
        search: {
          provider: 'local',
          options: {
            locales: {
              root: {
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
          message: '用 ❤️ 构建高性能应用 | 基于 MIT 许可证发布',
          copyright: 'Copyright © 2024-present UVHTTP Contributors'
        },
        
        lastUpdated: {
          text: '最后更新',
          formatOptions: {
            dateStyle: 'full',
            timeStyle: 'short'
          }
        }
      }
    }
  },

  themeConfig: {
    socialLinks: [
      { icon: 'github', link: 'https://github.com/adam-ikari/uvhttp' },
      { icon: 'twitter', link: 'https://twitter.com/uvhttp_lib' }
    ]
  }
})