export default {
  '/': [
    {
      text: '首页',
      link: '/'
    },
    {
      text: '快速开始',
      link: '/guide/getting-started'
    },
    {
      text: 'API 文档',
      link: '/api/introduction'
    }
  ],
  '/guide/': [
    {
      text: '指南',
      collapsible: true,
      items: [
        {
          text: '快速开始',
          link: '/guide/getting-started'
        }
      ]
    },
    {
      text: '教程',
      collapsible: true,
      items: [
        {
          text: '教程',
          link: '/TUTORIAL.md'
        }
      ]
    }
  ],
  '/api/': [
    {
      text: 'API 文档',
      collapsible: true,
      items: [
        {
          text: 'API 介绍',
          link: '/api/introduction'
        },
        {
          text: 'API 参考',
          link: '/API_REFERENCE.md'
        },
        {
          text: '错误码',
          link: '/ERROR_CODES.md'
        },
        {
          text: '限流 API',
          link: '/RATE_LIMIT_API.md'
        }
      ]
    }
  ],
  '/dev/': [
    {
      text: '开发者指南',
      collapsible: true,
      items: [
        {
          text: '开发者指南',
          link: '/DEVELOPER_GUIDE.md'
        },
        {
          text: '开发计划',
          link: '/DEVELOPMENT_PLAN.md'
        },
        {
          text: '测试指南',
          link: '/TESTABILITY_GUIDE.md'
        },
        {
          text: '测试标准',
          link: '/TESTING_STANDARDS.md'
        }
      ]
    },
    {
      text: '架构',
      collapsible: true,
      items: [
        {
          text: '架构设计',
          link: '/ARCHITECTURE.md'
        },
        {
          text: '中间件系统',
          link: '/MIDDLEWARE_SYSTEM.md'
        },
        {
          text: '路由搜索模式',
          link: '/ROUTER_SEARCH_MODES.md'
        }
      ]
    },
    {
      text: '功能模块',
      collapsible: true,
      items: [
        {
          text: '静态文件服务器',
          link: '/STATIC_FILE_SERVER.md'
        },
        {
          text: 'WebSocket 认证',
          link: '/WEBSOCKET_AUTH.md'
        },
        {
          text: '统一响应指南',
          link: '/UNIFIED_RESPONSE_GUIDE.md'
        }
      ]
    },
    {
      text: '性能',
      collapsible: true,
      items: [
        {
          text: '性能基准',
          link: '/PERFORMANCE_BENCHMARK.md'
        },
        {
          text: '性能测试标准',
          link: '/PERFORMANCE_TESTING_STANDARD.md'
        }
      ]
    },
    {
      text: '安全',
      collapsible: true,
      items: [
        {
          text: '安全',
          link: '/SECURITY.md'
        }
      ]
    },
    {
      text: '依赖',
      collapsible: true,
      items: [
        {
          text: '依赖',
          link: '/DEPENDENCIES.md'
        }
      ]
    },
    {
      text: '重构计划',
      collapsible: true,
      items: [
        {
          text: '全局变量重构计划',
          link: '/GLOBAL_VARIABLE_REFACTOR_PLAN.md'
        },
        {
          text: 'libuv 数据指针',
          link: '/LIBUV_DATA_POINTER.md'
        },
        {
          text: 'xxhash 集成',
          link: '/XXHASH_INTEGRATION.md'
        }
      ]
    }
  ]
}