const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const includeDir = '/home/zhaodi-chen/project/uvhttp/include';

// Get all C and H files
const srcFiles = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));
const includeFiles = fs.readdirSync(includeDir).filter(f => f.endsWith('.h'));
const allFiles = [...srcFiles.map(f => path.join(srcDir, f)), ...includeFiles.map(f => path.join(includeDir, f))];

// Mixed phrase translations - handle common Chinese-English mixed patterns
const translations = [
    // Router module
    ['数组routingrelated', 'array routing related'],
    ['回退routingsupport', 'fallback routing support'],
    ['回退routingsupport', 'fallback routing support'],
    
    // Connection module - cache lines
    ['forward declaration(避免loop引用)', 'forward declaration (avoid circular references)'],
    ['cache行1(0-63bytes):热pathfield - 最频繁访问', 'cache line 1 (0-63 bytes): hot path field - most frequently accessed'],
    ['在 on_read、on_write、connection management中频繁访问', 'frequently accessed in on_read, on_write, connection management'],
    ['parse是否完成', 'whether parsing is complete'],
    ['是否保持connect', 'whether to keep connection alive'],
    ['是否use分块传输', 'whether to use chunked transfer encoding'],
    ['待close的 handle 计数', 'count of handles pending close'],
    ['是否需要重启read', 'whether read needs to be restarted'],
    ['TLS 是否enable', 'whether TLS is enabled'],
    ['填充到32bytes', 'padding to 32 bytes'],
    ['已receive的 body 长度', 'length of body received'],
    ['读buffer已use', 'read buffer used'],
    ['读buffer大小', 'read buffer size'],
    ['cache行1总计:56bytes(剩余8bytes填充)', 'cache line 1 total: 56 bytes (remaining 8 bytes padding)'],
    ['cache行2(64-127bytes):pointerfield - 次频繁访问', 'cache line 2 (64-127 bytes): pointer field - second most frequently accessed'],
    ['在connectcreate、销毁、requesthandle中频繁访问', 'frequently accessed in connection creation, destruction, request handling'],
    ['所属server', 'server this connection belongs to'],
    ['读bufferpointer', 'read buffer pointer'],
    ['when前头部是否重要', 'whether current header is important'],
    ['是否正在parse头部field', 'whether currently parsing header field'],
    ['最后的error码', 'last error code'],
    ['是否to WebSocket connect', 'whether to upgrade to WebSocket connection'],
    ['填充到64bytes', 'padding to 64 bytes'],
    ['cache行2总计:约64bytes(取决于 WebSocket 是否enable)', 'cache line 2 total: approximately 64 bytes (depends on whether WebSocket is enabled)'],
    ['cache行3(128-191bytes):libuv 句柄', 'cache line 3 (128-191 bytes): libuv handles'],
    ['libuv 内部structure体,大小固定', 'libuv internal structure, fixed size'],
    ['cache行3总计:约88-112bytes', 'cache line 3 total: approximately 88-112 bytes'],
    ['cache行4(192-255bytes):HTTP parsestate', 'cache line 4 (192-255 bytes): HTTP parsing state'],
    ['在 HTTP parse过程中频繁访问', 'frequently accessed during HTTP parsing'],
    ['when前头部field长度', 'current header field length'],
    ['填充到64bytes', 'padding to 64 bytes'],
    ['cache行4总计:64bytes', 'cache line 4 total: 64 bytes'],
    ['cache行5+(256+bytes):大块buffer', 'cache line 5+ (256+ bytes): large buffers'],
    ['放在最后,避免影响热pathfield的cache局部性', 'placed at the end to avoid affecting cache locality of hot path fields'],
    ['大块memory', 'large memory block'],
    ['verification大型buffer在structure体末尾', 'verify large buffers are at the end of structure'],
    ['state管理', 'state management'],
    ['WebSockethandlefunction(内部)', 'WebSocket handler function (internal)'],
    
    // Connection timeout
    ['startconnecttimeout定when器', 'start connection timeout timer'],
    ['toconnectstarttimeout定when器,useconfiguration中的defaulttimeoutwhen间.', 'to connect and start timeout timer, use default timeout from configuration.'],
    ['ifconnect在timeoutwhen间内没有活动,将自动closeconnect.', 'if connection has no activity within timeout period, will automatically close connection.'],
    ['successreturn UVHTTP_OK,failurereturn负数error码', 'success returns UVHTTP_OK, failure returns negative error code'],
    ['timeoutwhen间从 conn->server->config->connection_timeout read,', 'timeout period is read from conn->server->config->connection_timeout,'],
    ['if config to NULL,则use UVHTTP_CONNECTION_TIMEOUT_DEFAULT', 'if config is NULL, use UVHTTP_CONNECTION_TIMEOUT_DEFAULT'],
    
    // Common mixed phrases
    ['是否', 'whether'],
    ['是否to', 'whether to'],
    ['是否use', 'whether to use'],
    ['是否enable', 'whether enabled'],
    ['是否正在', 'whether currently'],
    ['when前', 'current'],
    ['to', 'to'],
    ['use', 'use'],
    ['connect', 'connection'],
    ['handle', 'handle'],
    ['field', 'field'],
    ['state', 'state'],
    ['buffer', 'buffer'],
    ['memory', 'memory'],
    ['bytes', 'bytes'],
    ['cache', 'cache'],
    ['行', 'line'],
    ['总计', 'total'],
    ['剩余', 'remaining'],
    ['填充', 'padding'],
    ['到', 'to'],
    ['已', 'already'],
    ['读', 'read'],
    ['写', 'write'],
    ['大', 'large'],
    ['小', 'small'],
    ['热', 'hot'],
    ['冷', 'cold'],
    ['频繁', 'frequent'],
    ['访问', 'access'],
    ['管理', 'management'],
    ['内部', 'internal'],
    ['外部', 'external'],
    ['开始', 'start'],
    ['停止', 'stop'],
    ['创建', 'create'],
    ['销毁', 'destroy'],
    ['初始化', 'initialize'],
    ['清理', 'cleanup'],
    ['释放', 'release'],
    ['获取', 'get'],
    ['设置', 'set'],
    ['更新', 'update'],
    ['删除', 'delete'],
    ['添加', 'add'],
    ['移除', 'remove'],
    ['查找', 'find'],
    ['搜索', 'search'],
    ['匹配', 'match'],
    ['解析', 'parse'],
    ['编码', 'encode'],
    ['解码', 'decode'],
    ['加密', 'encrypt'],
    ['解密', 'decrypt'],
    ['压缩', 'compress'],
    ['解压', 'decompress'],
    ['发送', 'send'],
    ['接收', 'receive'],
    ['连接', 'connect'],
    ['断开', 'disconnect'],
    ['重连', 'reconnect'],
    ['超时', 'timeout'],
    ['错误', 'error'],
    ['警告', 'warning'],
    ['信息', 'info'],
    ['调试', 'debug'],
    ['跟踪', 'trace'],
    ['日志', 'log'],
    ['记录', 'record'],
    ['统计', 'statistics'],
    ['性能', 'performance'],
    ['优化', 'optimize'],
    ['测试', 'test'],
    ['验证', 'verify'],
    ['检查', 'check'],
    ['确认', 'confirm'],
    ['判断', 'judge'],
    ['决定', 'decide'],
    ['选择', 'choose'],
    ['配置', 'configure'],
    ['设置', 'setting'],
    ['选项', 'option'],
    ['参数', 'parameter'],
    ['变量', 'variable'],
    ['常量', 'constant'],
    ['类型', 'type'],
    ['结构', 'structure'],
    ['对象', 'object'],
    ['实例', 'instance'],
    ['引用', 'reference'],
    ['指针', 'pointer'],
    ['地址', 'address'],
    ['大小', 'size'],
    ['长度', 'length'],
    ['数量', 'count'],
    ['索引', 'index'],
    ['键', 'key'],
    ['值', 'value'],
    ['对', 'pair'],
    ['列表', 'list'],
    ['数组', 'array'],
    ['集合', 'set'],
    ['映射', 'map'],
    ['字典', 'dictionary'],
    ['队列', 'queue'],
    ['栈', 'stack'],
    ['树', 'tree'],
    ['图', 'graph'],
    ['节点', 'node'],
    ['边', 'edge'],
    ['根', 'root'],
    ['叶子', 'leaf'],
    ['分支', 'branch'],
    ['路径', 'path'],
    ['路由', 'route'],
    ['请求', 'request'],
    ['响应', 'response'],
    ['消息', 'message'],
    ['数据', 'data'],
    ['信息', 'information'],
    ['内容', 'content'],
    ['格式', 'format'],
    ['编码', 'encoding'],
    ['版本', 'version'],
    ['协议', 'protocol'],
    ['标准', 'standard'],
    ['规范', 'specification'],
    ['文档', 'document'],
    ['注释', 'comment'],
    ['代码', 'code'],
    ['函数', 'function'],
    ['方法', 'method'],
    ['类', 'class'],
    ['接口', 'interface'],
    ['模块', 'module'],
    ['组件', 'component'],
    ['库', 'library'],
    ['框架', 'framework'],
    ['系统', 'system'],
    ['平台', 'platform'],
    ['环境', 'environment'],
    ['配置', 'configuration'],
    ['部署', 'deployment'],
    ['发布', 'release'],
    ['版本', 'version'],
    ['更新', 'update'],
    ['升级', 'upgrade'],
    ['修复', 'fix'],
    ['改进', 'improve'],
    ['增强', 'enhance'],
    ['优化', 'optimize'],
    ['重构', 'refactor'],
    ['测试', 'test'],
    ['调试', 'debug'],
    ['分析', 'analyze'],
    ['监控', 'monitor'],
    ['日志', 'log'],
    ['审计', 'audit'],
    ['安全', 'security'],
    ['权限', 'permission'],
    ['认证', 'authentication'],
    ['授权', 'authorization'],
    ['加密', 'encryption'],
    ['解密', 'decryption'],
    ['签名', 'signature'],
    ['验证', 'verification'],
    ['校验', 'checksum'],
    ['哈希', 'hash'],
    ['摘要', 'digest'],
    ['压缩', 'compression'],
    ['解压', 'decompression'],
    ['归档', 'archive'],
    ['备份', 'backup'],
    ['恢复', 'restore'],
    ['迁移', 'migration'],
    ['同步', 'synchronization'],
    ['异步', 'asynchronous'],
    ['并发', 'concurrency'],
    ['并行', 'parallel'],
    ['串行', 'serial'],
    ['顺序', 'sequential'],
    ['随机', 'random'],
    ['确定', 'deterministic'],
    ['动态', 'dynamic'],
    ['静态', 'static'],
    ['自动', 'automatic'],
    ['手动', 'manual'],
    ['交互', 'interactive'],
    ['批处理', 'batch'],
    ['实时', 'real-time'],
    ['延迟', 'latency'],
    ['吞吐量', 'throughput'],
    ['带宽', 'bandwidth'],
    ['容量', 'capacity'],
    ['负载', 'load'],
    ['压力', 'stress'],
    ['性能', 'performance'],
    ['效率', 'efficiency'],
    ['可扩展性', 'scalability'],
    ['可靠性', 'reliability'],
    ['可用性', 'availability'],
    ['稳定性', 'stability'],
    ['兼容性', 'compatibility'],
    ['互操作性', 'interoperability'],
    ['可维护性', 'maintainability'],
    ['可测试性', 'testability'],
    ['可读性', 'readability'],
    ['可理解性', 'understandability'],
    
    // Single characters
    ['（', '('],
    ['）', ')'],
    ['，', ','],
    ['。', '.'],
    ['：', ':'],
    ['；', ';'],
    ['【', '['],
    ['】', ']'],
    ['《', '<'],
    ['》', '>'],
    ['"', '"'],
    ['"', '"'],
    ['\'', '\''],
    ['\'', '\'']
];

// Sort by length (longest first) to avoid partial matches
translations.sort((a, b) => b[0].length - a[0].length);

let totalTranslated = 0;

allFiles.forEach(filePath => {
    const content = fs.readFileSync(filePath, 'utf8');
    let result = content;
    let changes = 0;
    
    // Only process lines that are comments
    const lines = result.split('\n');
    const processedLines = lines.map(line => {
        // Check if line contains Chinese characters
        if (/[一-龥]/.test(line)) {
            let processedLine = line;
            
            // Apply translations
            translations.forEach(([chinese, english]) => {
                const regex = new RegExp(chinese.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g');
                if (regex.test(processedLine)) {
                    processedLine = processedLine.replace(regex, english);
                    changes++;
                }
            });
            
            return processedLine;
        }
        return line;
    });

    result = processedLines.join('\n');

    if (content !== result) {
        fs.writeFileSync(filePath, result, 'utf8');
        console.log('Translated:', path.basename(filePath), `(${changes} changes)`);
        totalTranslated++;
    }
});

console.log('\nTotal files translated:', totalTranslated);
console.log('Total translation patterns:', translations.length);