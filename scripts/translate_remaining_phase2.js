const fs = require('fs');
const path = require('path');

// Additional translations for remaining Chinese comments
const translations = {
    // Timer and scheduling
    '定时器': 'timer',
    '定时': 'scheduled',
    '启动定时器': 'start timer',
    '停止定时器': 'stop timer',
    '定时器配置': 'timer configuration',
    '自定义': 'custom',
    '自定义定时': 'custom scheduled',
    '指定': 'specified',
    '指定的时间': 'specified time',
    '时间': 'time',
    '时间内': 'within time',
    '没有': 'no',
    '没有活动': 'no activity',
    '将': 'will',
    '负数': 'negative',
    '负数时间': 'negative time',
    '如果': 'if',
    '如果则': 'if then',
    '此会': 'this will',
    '停止': 'stop',
    '停止并': 'stop and',
    '现有的': 'existing',
    '如果有': 'if there is',
    '自': 'self',
    '定义': 'define',
    '定义间': 'define interval',
    '从': 'from',
    '间隔': 'interval',
    '时间间隔': 'time interval',
    
    // Connection management
    '处理': 'handle',
    '处理连接': 'handle connection',
    '管理': 'manage',
    '管理连接': 'manage connection',
    '大块': 'large chunk',
    '大块内存': 'large chunk memory',
    
    // Particles and grammar
    '的': '',
    '已': '',
    '读': '',
    '处': '',
    '理': '',
    '管': '',
    '启': '',
    '动': '',
    '定': '',
    '器': '',
    '配': '',
    '置': '',
    '中': '',
    '间': '',
    '如': '',
    '果': '',
    '在': '',
    '内': '',
    '没': '',
    '有': '',
    '活': '',
    '将': '',
    '负': '',
    '数': '',
    '从': '',
    '则': '',
    '此': '',
    '会': '',
    '并': '',
    '现': '',
    '自': '',
    '义': '',
    '指': '',
    
    // Logic and conditions
    '如果': 'if',
    '否则': 'else',
    '那么': 'then',
    '当': 'when',
    '当且仅当': 'if and only if',
    '只要': 'as long as',
    '除非': 'unless',
    '无论': 'regardless of',
    '即使': 'even if',
    '虽然': 'although',
    '但是': 'but',
    '然而': 'however',
    '因此': 'therefore',
    '所以': 'so',
    '因为': 'because',
    '由于': 'due to',
    '为了': 'in order to',
    '以便': 'so that',
    '从而': 'thereby',
    '进而': 'and then',
    '此外': 'in addition',
    '另外': 'also',
    '同时': 'at the same time',
    
    // Action verbs
    '执行': 'execute',
    '运行': 'run',
    '启动': 'start',
    '停止': 'stop',
    '暂停': 'pause',
    '恢复': 'resume',
    '继续': 'continue',
    '完成': 'finish',
    '结束': 'end',
    '终止': 'terminate',
    '取消': 'cancel',
    '重试': 'retry',
    '重置': 'reset',
    '刷新': 'refresh',
    '更新': 'update',
    '创建': 'create',
    '删除': 'delete',
    '添加': 'add',
    '移除': 'remove',
    '插入': 'insert',
    '提取': 'extract',
    '获取': 'get',
    '设置': 'set',
    '修改': 'modify',
    '改变': 'change',
    '调整': 'adjust',
    '优化': 'optimize',
    '改进': 'improve',
    '增强': 'enhance',
    '简化': 'simplify',
    '扩展': 'extend',
    '收缩': 'shrink',
    '增长': 'grow',
    '减少': 'reduce',
    '增加': 'increase',
    '提升': 'improve',
    '降低': 'decrease',
    '提高': 'raise',
    
    // Status and state
    '成功': 'success',
    '失败': 'failure',
    '错误': 'error',
    '警告': 'warning',
    '信息': 'info',
    '调试': 'debug',
    '跟踪': 'trace',
    '致命': 'fatal',
    '严重': 'critical',
    '轻微': 'minor',
    '主要': 'major',
    '次要': 'secondary',
    '重要': 'important',
    '紧急': 'urgent',
    '正常': 'normal',
    '异常': 'abnormal',
    '有效': 'valid',
    '无效': 'invalid',
    '启用': 'enabled',
    '禁用': 'disabled',
    '激活': 'active',
    '非激活': 'inactive',
    '在线': 'online',
    '离线': 'offline',
    '连接': 'connected',
    '断开': 'disconnected',
    '就绪': 'ready',
    '未就绪': 'not ready',
    '完成': 'completed',
    '进行中': 'in progress',
    '待处理': 'pending',
    '已处理': 'processed',
    '已取消': 'cancelled',
    '已超时': 'timeout',
    '已过期': 'expired',
    
    // Data types
    '整数': 'integer',
    '浮点数': 'float',
    '字符串': 'string',
    '布尔值': 'boolean',
    '数组': 'array',
    '列表': 'list',
    '集合': 'set',
    '映射': 'map',
    '字典': 'dictionary',
    '对象': 'object',
    '结构体': 'struct',
    '枚举': 'enum',
    '类': 'class',
    '接口': 'interface',
    '函数': 'function',
    '方法': 'method',
    '属性': 'property',
    '字段': 'field',
    '参数': 'parameter',
    '变量': 'variable',
    '常量': 'constant',
    '全局': 'global',
    '局部': 'local',
    '静态': 'static',
    '动态': 'dynamic',
    
    // Common phrases
    '注意': 'note',
    '提示': 'tip',
    '警告': 'warning',
    '错误': 'error',
    '信息': 'info',
    '说明': 'description',
    '描述': 'description',
    '示例': 'example',
    '参考': 'reference',
    '文档': 'documentation',
    '注释': 'comment',
    '代码': 'code',
    '数据': 'data',
    '配置': 'configuration',
    '设置': 'setting',
    '选项': 'option',
    '参数': 'argument',
    '返回值': 'return value',
    '异常': 'exception',
    '错误码': 'error code',
    '状态码': 'status code',
    '响应头': 'response header',
    '请求头': 'request header',
    '响应体': 'response body',
    '请求体': 'request body',
    '查询参数': 'query parameter',
    '路径参数': 'path parameter',
    '表单数据': 'form data',
    'JSON 数据': 'JSON data',
    'XML 数据': 'XML data',
    '二进制数据': 'binary data',
    '文本数据': 'text data',
    '文件数据': 'file data',
    '流数据': 'stream data',
    '缓冲数据': 'buffer data',
    '缓存数据': 'cache data',
    '内存数据': 'memory data',
    '磁盘数据': 'disk data',
    '网络数据': 'network data',
    '系统数据': 'system data',
    '用户数据': 'user data',
    '业务数据': 'business data',
    '日志数据': 'log data',
    '监控数据': 'monitoring data',
    '统计数据': 'statistics data',
    '分析数据': 'analysis data',
    '报告数据': 'report data',
    '历史数据': 'historical data',
    '实时数据': 'real-time data',
    '批量数据': 'batch data',
    '单个数据': 'single data',
    '多个数据': 'multiple data',
    '所有数据': 'all data',
    '部分数据': 'partial data',
    '完整数据': 'complete data',
    '原始数据': 'raw data',
    '处理后数据': 'processed data',
    '格式化数据': 'formatted data',
    '加密数据': 'encrypted data',
    '解密数据': 'decrypted data',
    '压缩数据': 'compressed data',
    '解压数据': 'decompressed data',
    '编码数据': 'encoded data',
    '解码数据': 'decoded data',
    '序列化数据': 'serialized data',
    '反序列化数据': 'deserialized data',
};

// Function to translate a file
function translateFile(filePath) {
    console.log(`Processing: ${filePath}`);
    
    let content = fs.readFileSync(filePath, 'utf8');
    let originalLength = content.length;
    let changes = 0;
    
    // Apply all translations
    for (const [chinese, english] of Object.entries(translations)) {
        const regex = new RegExp(chinese, 'g');
        const matches = content.match(regex);
        if (matches) {
            content = content.replace(regex, english);
            changes += matches.length;
        }
    }
    
    // Only write if changes were made
    if (content !== fs.readFileSync(filePath, 'utf8')) {
        fs.writeFileSync(filePath, content, 'utf8');
        console.log(`  ✓ Translated ${changes} phrases`);
        return changes;
    }
    
    console.log(`  No changes`);
    return 0;
}

// Get all header files
const includeDir = path.join(__dirname, '..', 'include');
const files = fs.readdirSync(includeDir).filter(f => f.endsWith('.h') && f.startsWith('uvhttp_'));

console.log(`Found ${files.length} header files to translate\n`);

let totalChanges = 0;
for (const file of files) {
    const filePath = path.join(includeDir, file);
    totalChanges += translateFile(filePath);
}

console.log(`\nTotal translations: ${totalChanges} phrases`);
