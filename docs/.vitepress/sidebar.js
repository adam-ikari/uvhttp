export default {
  '/': [
    {
      text: '首页',
      link: '/'
    },
    {
      text: '📚 使用者文档',
      link: '/guide/'
    },
    {
      text: ' 开发者文档',
      link: '/dev/'
    }
  ],
  '/guide/': [
    {
      text: '📖 快速开始',
      collapsible: true,
      items: [
        {
          text: '简介',
          link: '/guide/introduction'
        },
        {
          text: '安装',
          link: '/guide/installation'
        },
        {
          text: '快速开始',
          link: '/guide/getting-started'
        },
        {
          text: '第一个服务器',
          link: '/guide/first-server'
        }
      ]
    },
    {
      text: ' 基础教程',
      collapsible: true,
      items: [
        {
          text: '教程',
          link: '/TUTORIAL.md'
        },
        {
          text: '路由系统',
          link: '/guide/routing'
        },
        {
          text: '请求处理',
          link: '/guide/requests'
        },
        {
          text: '响应处理',
          link: '/guide/responses'
        }
      ]
    },
    {
      text: ' 高级功能',
      collapsible: true,
      items: [
        {
          text: '中间件系统',
          link: '/MIDDLEWARE_SYSTEM.md'
        },
        {
          text: '静态文件服务',
          link: '/STATIC_FILE_SERVER.md'
        },
        {
          text: 'WebSocket',
          link: '/guide/websocket'
        },
        {
          text: 'WebSocket 认证',
          link: '/WEBSOCKET_AUTH.md'
        },
        {
          text: '限流功能',
          link: '/RATE_LIMIT_API.md'
        }
      ]
    },
    {
      text: '📋 API 参考',
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
          text: '统一 API',
          link: '/guide/unified-api'
        },
        {
          text: '错误码',
          link: '/ERROR_CODES.md'
        }
      ]
    },
    {
      text: ' 性能优化',
      collapsible: true,
      items: [
        {
          text: '性能基准',
          link: '/PERFORMANCE_BENCHMARK.md'
        },
        {
          text: '性能优化指南',
          link: '/guide/performance'
        }
      ]
    },
    {
      text: ' 安全',
      collapsible: true,
      items: [
        {
          text: '安全指南',
          link: '/SECURITY.md'
        }
      ]
    },
    {
      text: '📖 其他',
      collapsible: true,
      items: [
        {
          text: '常见问题',
          link: '/guide/faq'
        },
        {
          text: '最佳实践',
          link: '/guide/best-practices'
        }
      ]
    }
  ],
  '/dev/': [
    {
      text: '🏗️ 开发指南',
      collapsible: true,
      items: [
        {
          text: '开发者指南',
          link: '/DEVELOPER_GUIDE.md'
        },
        {
          text: '开发环境搭建',
          link: '/dev/setup'
        },
        {
          text: '代码规范',
          link: '/dev/coding-standards'
        },
        {
          text: '贡献指南',
          link: '/dev/contributing'
        }
      ]
    },
    {
      text: '📐 架构设计',
      collapsible: true,
      items: [
        {
          text: '架构设计',
          link: '/ARCHITECTURE.md'
        },
        {
          text: '模块设计',
          link: '/dev/modules'
        },
        {
          text: '路由搜索模式',
          link: '/ROUTER_SEARCH_MODES.md'
        },
        {
          text: '统一响应指南',
          link: '/UNIFIED_RESPONSE_GUIDE.md'
        }
      ]
    },
    {
      text: '🧪 测试',
      collapsible: true,
      items: [
        {
          text: '测试指南',
          link: '/TESTABILITY_GUIDE.md'
        },
        {
          text: '测试标准',
          link: '/TESTING_STANDARDS.md'
        },
        {
          text: '性能测试标准',
          link: '/PERFORMANCE_TESTING_STANDARD.md'
        }
      ]
    },
    {
      text: ' 性能分析',
      collapsible: true,
      items: [
        {
          text: '性能分析',
          link: '/dev/performance-analysis'
        },
        {
          text: '内存分析',
          link: '/dev/memory-analysis'
        }
      ]
    },
    {
      text: ' 开发工具',
      collapsible: true,
      items: [
        {
          text: '依赖管理',
          link: '/DEPENDENCIES.md'
        },
        {
          text: '构建系统',
          collapsible: true,
          items: [
            {
              text: '构建系统',
              link: '/dev/build-system'
            },
            {
              text: '构建配置矩阵',
              link: '/BUILD_CONFIGURATION_MATRIX.md'
            },
            {
              text: '高级构建选项',
              link: '/ADVANCED_BUILD_OPTIONS.md'
            }
          ]
        },
        {
          text: '调试技巧',
          link: '/dev/debugging'
        }
      ]
    },
    {
      text: '📋 开发计划',
      collapsible: true,
      items: [
        {
          text: '开发计划',
          link: '/DEVELOPMENT_PLAN.md'
        },
        {
          text: '路线图',
          link: '/ROADMAP.md'
        },
        {
          text: '变更日志',
          link: '/CHANGELOG.md'
        }
      ]
    },
    {
      text: '🔄 重构计划',
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