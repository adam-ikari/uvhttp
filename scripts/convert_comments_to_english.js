#!/usr/bin/env node

/**
 * Convert Chinese comments to English in header files
 * This script processes all header files in include/ directory
 */

const fs = require('fs');
const path = require('path');

// Chinese to English comment translations
const translations = {
    // Common phrases
    '前向声明': 'Forward declarations',
    '主要API': 'Main API',
    '便捷函数': 'Convenience functions',
    '错误处理宏': 'Error handling macros',
    '核心 HTTP 功能 - 始终启用': 'Core HTTP functionality - always enabled',
    '中间件支持 - 编译期配置': 'Middleware support - compile-time configuration',
    '中间件支持 - 已启用': 'Middleware support - enabled',
    '可选功能模块': 'Optional feature modules',
    'WebSocket 功能模块': 'WebSocket feature module',
    '静态文件功能模块': 'Static file feature module',
    '日志中间件 - 已禁用以提高性能': 'Logging middleware - disabled for performance',
    'TLS/SSL 支持': 'TLS/SSL support',
    '路由缓存支持': 'Router cache support',
    'LRU缓存支持': 'LRU cache support',
    'CORS 支持': 'CORS support',
    '限流支持 - 默认启用': 'Rate limiting support - enabled by default',
    '自定义分配器支持': 'Custom allocator support',
    '条件编译宏': 'Conditional compilation macros',
    '基础功能宏': 'Basic feature macros',
    '编译时断言宏': 'Compile-time assertion macros',
    '注意：UVHTTP_STATIC_ASSERT 已在 uvhttp_common.h 中定义': 'Note: UVHTTP_STATIC_ASSERT is defined in uvhttp_common.h',
    '属性宏': 'Attribute macros',
    '函数属性': 'Function attributes',
    '内存对齐': 'Memory alignment',
    '分支预测优化': 'Branch prediction optimization',
    '依赖注入和上下文管理': 'Dependency injection and context management',
    '内存分配器说明': 'Memory allocator description',
    '内存分配器采用编译时宏设计，零开销抽象': 'Memory allocator uses compile-time macro design, zero overhead abstraction',
    '不使用运行时分配器提供者接口，原因：': 'Runtime allocator provider interface is not used because:',
    '性能优先：避免函数指针调用开销': 'Performance first: avoid function pointer call overhead',
    '编译时优化：编译器可以内联和优化分配调用': 'Compile-time optimization: compiler can inline and optimize allocation calls',
    '简单直接：减少复杂性，提高可维护性': 'Simple and direct: reduce complexity, improve maintainability',
    '内存分配器类型通过 UVHTTP_ALLOCATOR_TYPE 编译宏选择：': 'Memory allocator type is selected through UVHTTP_ALLOCATOR_TYPE compile macro:',
    '系统默认分配器 (malloc/free)': 'System default allocator (malloc/free)',
    'mimalloc 高性能分配器': 'mimalloc high-performance allocator',
    '自定义分配器 (外部链接)': 'Custom allocator (external linking)',
    '使用方式：': 'Usage:',
    '详见 uvhttp_allocator.h 中的详细说明': 'See uvhttp_allocator.h for detailed information',
    '主上下文结构': 'Main context structure',
    '核心组件': 'Core components',
    '上下文状态': 'Context state',
    '统计信息': 'Statistics',
    '全局变量替代字段': 'Global variable replacement fields',
    'TLS 模块状态': 'TLS module state',
    'WebSocket 模块状态': 'WebSocket module state',
    '配置管理': 'Configuration management',
    '用户数据（用于存储应用特定的上下文）': 'User data (for storing application-specific context)',
    '上下文管理函数': 'Context management functions',
    '创建新的上下文': 'Create new context',
    '销毁上下文': 'Destroy context',
    '初始化上下文（设置默认提供者）': 'Initialize context (set default providers)',
    '全局变量替代字段初始化函数': 'Global variable replacement field initialization functions',
    '初始化 TLS 模块状态': 'Initialize TLS module state',
    '清理 TLS 模块状态': 'Cleanup TLS module state',
    '初始化 WebSocket 模块状态': 'Initialize WebSocket module state',
    '清理 WebSocket 模块状态': 'Cleanup WebSocket module state',
    '初始化配置管理': 'Initialize configuration management',
    '清理配置管理': 'Cleanup configuration management',
    '默认提供者实现': 'Default provider implementations',
    '注意：内存分配器使用编译时宏，无需创建提供者': 'Note: Memory allocator uses compile-time macros, no need to create providers',
    '系统默认分配器：直接使用 malloc/free': 'System default allocator: directly use malloc/free',
    'mimalloc分配器：编译时链接 mimalloc 库': 'mimalloc allocator: link mimalloc library at compile time',
    '自定义分配器：编译时链接用户实现': 'Custom allocator: link user implementation at compile time',
    'xxHash 头文件': 'xxHash header file',
    // Request structure comments
    'HTTP方法枚举': 'HTTP method enumeration',
    '缓存行1（0-63字节）：热路径字段 - 最频繁访问': 'Cache line 1 (0-63 bytes): hot path fields - most frequently accessed',
    '在 HTTP 解析、路由匹配中频繁访问': 'Frequently accessed in HTTP parsing and route matching',
    '4 字节 - HTTP 方法': '4 bytes - HTTP method',
    '4 字节 - 解析是否完成': '4 bytes - whether parsing is complete',
    '8 字节 - 填充到16字节': '8 bytes - padding to 16 bytes',
    '8 字节 - header 数量': '8 bytes - header count',
    '8 字节 - body 长度': '8 bytes - body length',
    '8 字节 - body 容量': '8 bytes - body capacity',
    '8 字节 - TCP 客户端句柄': '8 bytes - TCP client handle',
    '8 字节 - HTTP 解析器': '8 bytes - HTTP parser',
    '缓存行1总计：56字节（剩余8字节填充）': 'Cache line 1 total: 56 bytes (remaining 8 bytes padding)',
    '缓存行2（64-127字节）：指针字段 - 次频繁访问': 'Cache line 2 (64-127 bytes): pointer fields - second most frequently accessed',
    '在请求处理、响应构建中频繁访问': 'Frequently accessed in request processing and response building',
    '8 字节 - 解析器设置': '8 bytes - parser settings',
    '8 字节 - 请求路径': '8 bytes - request path',
    '8 字节 - 查询字符串': '8 bytes - query string',
    '8 字节 - 请求体': '8 bytes - request body',
    '8 字节 - 用户数据': '8 bytes - user data',
    '8 字节 - 额外 headers（动态扩容）': '8 bytes - extra headers (dynamic expansion)',
    '8 字节 - headers 总容量': '8 bytes - headers total capacity',
    '8 字节 - 填充到64字节': '8 bytes - padding to 64 bytes',
    '缓存行2总计：64字节': 'Cache line 2 total: 64 bytes',
    '缓存行3+（128+字节）：大块缓冲区': 'Cache line 3+ (128+ bytes): large buffers',
    '放在最后，避免影响热路径字段的缓存局部性': 'Placed at the end to avoid affecting cache locality of hot path fields',
    '2048 字节 - URL 缓冲区': '2048 bytes - URL buffer',
    'Headers - 混合分配：内联 + 动态扩容（优化内存局部性）': 'Headers - mixed allocation: inline + dynamic expansion (optimize memory locality)',
    '内联，减少动态分配': 'Inline, reduce dynamic allocation',
    '内存布局验证静态断言': 'Memory layout verification static assertions',
    '验证指针对齐（平台自适应）': 'Verify pointer alignment (platform adaptive)',
    // Doxygen comments
    '* 配置建议：': '* Configuration recommendations:',
    '* CMake 配置：': '* CMake configuration:',
    '* - 通过 CMakeLists.txt 或命令行参数配置': '* - Configure through CMakeLists.txt or command line parameters',
    '* @param ctx 静态文件服务上下文': '* @param ctx Static file service context',
    '* @param cache 缓存管理器': '* @param cache Cache manager',
    '* @return UVHTTP_OK成功，其他值表示失败': '* @return UVHTTP_OK on success, other values indicate failure',
    '* @param file_path 文件路径': '* @param file_path File path',
    '默认值定义在 uvhttp_defaults.h 中': 'Default values are defined in uvhttp_defaults.h',
    '/* 验证size_t对齐（平台自适应） */': '/* Verify size_t alignment (platform adaptive) */',
    '* - 默认值适合大多数应用': '* - Default values are suitable for most applications',
    '*    - 测试结果：': '*    - Test results:',
    '* @return UVHTTP_OK 成功，其他值表示失败': '* @return UVHTTP_OK on success, other values indicate failure',
    '* @param last_modified 最后修改时间': '* @param last_modified Last modified time',
    '/* 验证大型缓冲区在结构体末尾 */': '/* Verify large buffers at end of struct */',
    '* 影响：': '* Impact:',
    '* @param response HTTP响应': '* @param response HTTP response',
    '* @param buffer_size 缓冲区大小': '* @param buffer_size Buffer size',
    '* - 高并发场景可以增加 UVHTTP_DEFAULT_KEEP_ALIVE_MAX': '* - Increase UVHTTP_DEFAULT_KEEP_ALIVE_MAX for high concurrency scenarios',
    '限流配置': 'Rate limiting configuration',
    '* - 长时间处理应用：120-300秒': '* - Long-running applications: 120-300 seconds',
    // More common comments
    '/* 错误处理 */': '/* Error handling */',
    '* 释放': '* Free',
    '/* 遍历所有 headers */': '/* Iterate through all headers */',
    '/* 请使用 #include "uvhttp_validation.h" 来访问验证函数 */': '/* Please use #include "uvhttp_validation.h" to access validation functions */',
    '* 获取缓存Statistics': '* Get cache statistics',
    '/* 获取指定索引的 header（内部使用） */': '/* Get header at specified index (internal use) */',
    '/* 获取 header 数量 */': '/* Get header count */',
    '/* 缓存行4总计：64字节 */': '/* Cache line 4 total: 64 bytes */',
    '/* ========== 缓存行3（128-191字节）：libuv 句柄 ========== */': '/* ========== Cache line 3 (128-191 bytes): libuv handles ========== */',
    '/* 缓存行1总计：64字节 */': '/* Cache line 1 total: 64 bytes */',
    '* - 普通应用：60秒（默认）': '* - Normal applications: 60s (default)',
    '* - 快速响应应用：30秒': '* - Fast response applications: 30s',
    '* - 建议结合限流中间件使用': '* - Recommended to use with rate limiting middleware',
    '* - 小型应用：1024': '* - Small applications: 1024',
    '* 如果连接在超时时间内没有活动，将自动关闭连接。': '* If connection has no activity within timeout, it will be automatically closed.',
    '* - 大多数应用：32-64 足够': '* - Most applications: 32-64 is sufficient',
    '* - 复杂应用：可以增加到 128': '* - Complex applications: can increase to 128',
    'WebSocket 默认配置': 'WebSocket default configuration',
    'void* timeout_callback_user_data; /* 8 字节 - 回调用户数据 */': 'void* timeout_callback_user_data; /* 8 bytes - callback user data */',
    'uvhttp_timeout_callback_t timeout_callback; /* 8 字节 - 超时统计回调 */': 'uvhttp_timeout_callback_t timeout_callback; /* 8 bytes - timeout statistics callback */',
    '* Sendfile 配置': '* Sendfile configuration',
    '* @return 清理的条目数量': '* @return Number of entries cleaned',
    '* @return 命中率（0.0-1.0）': '* @return Hit rate (0.0-1.0)',
    '* @return UVHTTP_OK 成功，其他值表示错误': '* @return UVHTTP_OK on success, other values indicate error',
    '* @param total_memory_usage 输出总内存使用量': '* @param total_memory_usage Output total memory usage',
    '* @param request HTTP请求': '* @param request HTTP request',
    '* @param miss_count 输出未命中次数': '* @param miss_count Output miss count',
    '* @param hit_count 输出命中次数': '* @param hit_count Output hit count',
    '* @param file_size 文件大小': '* @param file_size File size',
    '* @param eviction_count 输出驱逐次数': '* @param eviction_count Output eviction count',
    '* @param etag ETag值': '* @param etag ETag value',
    '* @param entry 缓存条目': '* @param entry Cache entry',
    '* @param entry_count 输出条目数量': '* @param entry_count Output entry count',
    '* @param conn 连接对象': '* @param conn Connection object',
    '* @note 此函数会停止并重启现有的定时器（如果有）': '* @note This function will stop and restart existing timer (if any)',
    '/* libuv 内部结构体，大小固定 */': '/* libuv internal struct, fixed size */',
    '* Keep-Alive 配置': '* Keep-Alive configuration',
    '/* ========== Headers 操作 API ========== */': '/* ========== Headers manipulation API ========== */',
    '* 默认网络配置': '* Default network configuration',
    '* 默认监听地址和端口': '* Default listen address and port',
    '* - 默认值已经足够大，不建议减小': '* - Default value is already large enough, not recommended to decrease',
    '* - 默认值已经足够，不建议修改': '* - Default value is already sufficient, not recommended to modify',
    '* - 默认值：64（基于真实网站分析）': '* - Default value: 64 (based on real website analysis)',
    '* - 默认值：32（优化小 header 场景）': '* - Default value: 32 (optimized for small header scenarios)',
    '* - 默认值：256 字节': '* - Default value: 256 bytes',
    '* 高级选项：': '* Advanced options:',
    '*    - 高性能环境：增加缓冲区大小，启用所有缓存': '*    - High performance environment: increase buffer size, enable all caches',
    '/* ========== 验证配置 ========== */': '/* ========== Validation configuration ========== */',
    '* 验证握手响应 (客户端)': '* Verify handshake response (client)',
    '/* ============ 验证函数 ============ */': '/* ============ Validation functions ============ */',
    // More translations
    '* 验证 Sec-WebSocket-Accept': '* Verify Sec-WebSocket-Accept',
    '/* ========== 静态断言宏定义 ========== */': '/* ========== Static assertion macro definitions ========== */',
    '/* 静态文件配置结构 - CPU 缓存优化布局 */': '/* Static file configuration structure - CPU cache optimized layout */',
    '/* 静态文件路由支持（8字节对齐） */': '/* Static file routing support (8-byte aligned) */',
    '/* 静态文件路由支持 */': '/* Static file routing support */',
    '* 静态文件缓存配置': '* Static file cache configuration',
    '/* ========== 静态文件服务配置 ========== */': '/* ========== Static file service configuration ========== */',
    '/* 静态文件服务上下文 */': '/* Static file service context */',
    '* 静态文件大小阈值': '* Static file size threshold',
    '/* ========== 静态文件中间件接口 ========== */': '/* ========== Static file middleware interface ========== */',
    '* - 需要调整系统文件描述符限制': '* - Need to adjust system file descriptor limit',
    '* - 需要调整 ulimit -n': '* - Need to adjust ulimit -n',
    '/* 需要确保结构体对齐正确，避免性能下降 */': '/* Need to ensure struct alignment is correct to avoid performance degradation */',
    '* - 零开销抽象：使用编译期宏，无运行时动态分配': '* - Zero overhead abstraction: uses compile-time macros, no runtime dynamic allocation',
    '* 限流时间窗口（秒）': '* Rate limiting time window (seconds)',
    '/* ========== 限流 API（核心功能） ========== */': '/* ========== Rate limiting API (core feature) ========== */',
    '/* ========== 错误码范围 ========== */': '/* ========== Error code ranges ========== */',
    '* 错误消息缓冲区大小': '* Error message buffer size',
    '* 错误恢复配置': '* Error recovery configuration',
    '/* ========== 错误处理配置 ========== */': '/* ========== Error handling configuration ========== */',
    '* 错误响应消息': '* Error response message',
    '/* 链式路由API */': '/* Chained routing API */',
    '* 重要说明：': '* Important note:',
    '* 重置Statistics': '* Reset statistics',
    '/* ============ 重构后的函数：分离纯函数和副作用 ============ */': '/* ============ Refactored functions: separate pure functions and side effects ============ */',
    '* 配置错误码范围': '* Configure error code range',
    '* ==================== 配置示例 ====================': '* ==================== Configuration examples ====================',
    '* ==================== 配置指南 ====================': '* ==================== Configuration guide ====================',
    '*    通过 CMake 编译选项或直接修改本文件中的宏定义来配置': '*    Configure through CMake compilation options or directly modify macro definitions in this file',
    '* 连接错误码范围': '* Connection error code range',
    '* 连接超时配置': '* Connection timeout configuration',
    '* 连接超时时间': '* Connection timeout time',
    '/* ========== 连接管理配置 ========== */': '/* ========== Connection management configuration ========== */',
    '/* 连接管理函数 */': '/* Connection management functions */',
    '/* 连接管理 API */': '/* Connection management API */',
    '* - 过小可能导致连接被拒绝': '* - Too small may cause connections to be rejected',
    '* - 过大的值可能影响性能': '* - Too large may affect performance',
    '* - 过大的值可能导致内存耗尽攻击': '* - Too large may cause memory exhaustion attack',
    '* - 过大的值可能导致内存不足': '* - Too large may cause insufficient memory',
    '* - 过大占用更多内存': '* - Too large uses more memory',
    '* - 较小的缓冲区节省内存，但可能增加重分配开销': '* - Smaller buffer saves memory, but may increase reallocation overhead',
    '* - 较小的缓冲区节省内存': '* - Smaller buffer saves memory',
    '* - 较大的缓冲区提升网络 I/O 性能': '* - Larger buffer improves network I/O performance',
    '* - 较大的缓冲区减少内存重分配次数，提升性能': '* - Larger buffer reduces memory reallocation count, improves performance',
    '* 路由错误码范围': '* Router error code range',
    '/* ========== 路由配置 ========== */': '/* ========== Router configuration ========== */',
    '* 路由路径最大长度': '* Router path maximum length',
    '// 路由节点': '// Router node',
    '/* ========== 路由缓存配置（性能优化） ========== */': '/* ========== Router cache configuration (performance optimization) ========== */',
    '* 路由缓存开关': '* Router cache switch',
    '/* 路由缓存大小配置 */': '/* Router cache size configuration */',
    '/* 路由缓存优化功能 */': '/* Router cache optimization feature */',
    '/* 路由添加（支持HTTP方法） */': '/* Route addition (supporting HTTP methods) */',
    '* 路由查找模式': '* Route search mode',
    '/* 路由查找 */': '/* Route search */',
    '* - 路由数量少（< 10）：可以禁用以节省内存': '* - Few routes (< 10): can disable to save memory',
    '* - 路由数量多（> 10）：启用以提升性能（默认）': '* - Many routes (> 10): enable to improve performance (default)',
    '// 路由器结构': '// Router structure',
    '// 路由参数': '// Route parameters',
    '/* 路由匹配（获取参数） */': '/* Route matching (get parameters) */',
    '// 路由匹配结果': '// Route match result',
    '/* 路由API函数 */': '/* Route API functions */',
    '/* ========== 超时配置默认值 ========== */': '/* ========== Timeout configuration defaults ========== */',
    '/* ========== 超时统计回调 ========== */': '/* ========== Timeout statistics callback ========== */',
    '/* 超时统计回调 */': '/* Timeout statistics callback */',
    '* - 超大文件：64KB': '* - Very large files: 64KB',
    '* - 超大文件：256KB': '* - Very large files: 256KB',
    '* - 超大文件：128KB': '* - Very large files: 128KB',
    '* 调用者负责释放返回的 *out_data 内存': '* Caller is responsible for freeing returned *out_data memory',
    '* 读取缓冲区大小（字节）- 性能优化': '* Read buffer size (bytes) - performance optimization',
    '* 读取缓冲区大小': '* Read buffer size',
    '* 请求错误码范围': '* Request error code range',
    '* 请求超时时间（秒）': '* Request timeout time (seconds)',
    '* 请求头最大大小（字节）': '* Request header maximum size (bytes)',
    '/* 请求处理器类型 */': '/* Request handler type */',
    '* 请求/响应缓冲区大小': '* Request/response buffer size',
    '* 请求体最大大小（字节）': '* Request body maximum size (bytes)',
    '/* 证书验证 */': '/* Certificate verification */',
    '/* 证书链验证 */': '/* Certificate chain verification */',
    '/* 证书配置 */': '/* Certificate configuration */',
};

function convertComments(content) {
    let result = content;

    // Apply translations
    for (const [chinese, english] of Object.entries(translations)) {
        const regex = new RegExp(chinese.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g');
        result = result.replace(regex, english);
    }

    return result;
}

function processFile(filePath) {
    console.log(`Processing: ${filePath}`);
    
    const content = fs.readFileSync(filePath, 'utf8');
    const converted = convertComments(content);
    
    if (content !== converted) {
        fs.writeFileSync(filePath, converted, 'utf8');
        console.log(`  ✓ Updated`);
    } else {
        console.log(`  - No changes`);
    }
}

function main() {
    const includeDir = path.join(__dirname, '..', 'include');
    
    // Find all header files
    const files = fs.readdirSync(includeDir)
        .filter(file => file.endsWith('.h'))
        .map(file => path.join(includeDir, file));
    
    console.log(`Found ${files.length} header files\n`);
    
    for (const file of files) {
        processFile(file);
    }
    
    console.log('\n All files processed');
}

main();