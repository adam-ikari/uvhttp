const fs = require('fs');
const path = require('path');

// Final translations for remaining Chinese comments
const translations = {
    // LRU cache operations
    '清': 'clear',
    '清理': 'clear',
    '统计': 'statistics',
    '统计计': 'statistics count',
    '重': 'reset',
    '重统计': 'reset statistics',
    '检': 'check',
    '移': 'remove',
    '部': 'part',
    '计算': 'calculate',
    '率': 'rate',
    '计算率': 'calculate rate',
    
    // Server operations
    '毫': 'milli',
    '检测': 'detect',
    '心': 'heart',
    '跳': 'beat',
    '心跳': 'heartbeat',
    '心跳检测': 'heartbeat detection',
    '由': 'by',
    '层': 'layer',
    '由层统计': 'statistics by layer',
    '记录': 'record',
    '监听': 'listen',
    '拥': 'congestion',
    '拥塞': 'congestion',
    '跃': 'active',
    '活跃': 'active',
    '上': 'up',
    '废弃': 'deprecate',
    '弃': 'abandon',
    '窗口': 'window',
    '窗口开始': 'window start',
    '组': 'group',
    '可': 'can',
    '新': 'new',
    '可新': 'can renew',
    '级': 'level',
    '级别': 'level',
    '例': 'example',
    '允许': 'allow',
    '效': 'effective',
    '效级别': 'effective level',
    '共': 'shared',
    '共享': 'shared',
    '适': 'adapt',
    '防止': 'prevent',
    '攻': 'attack',
    '击': 'attack',
    '攻击': 'attack',
    '适防': 'adaptive defense',
    '按': 'press',
    '按议': 'press protocol',
    '前': 'front',
    '协议前': 'protocol front',
    '固': 'fix',
    '固窗口': 'fixed window',
    
    // Particles
    '统': '',
    '计': '',
    '清': '',
    '重': '',
    '检': '',
    '移': '',
    '部': '',
    '算': '',
    '毫': '',
    '测': '',
    '心': '',
    '跳': '',
    '由': '',
    '层': '',
    '记': '',
    '录': '',
    '监': '',
    '听': '',
    '拥': '',
    '跃': '',
    '上': '',
    '废': '',
    '弃': '',
    '窗': '',
    '口': '',
    '开': '',
    '始': '',
    '组': '',
    '可': '',
    '新': '',
    '级': '',
    '别': '',
    '例': '',
    '允': '',
    '许': '',
    '效': '',
    '共': '',
    '享': '',
    '适': '',
    '防': '',
    '止': '',
    '攻': '',
    '击': '',
    '按': '',
    '议': '',
    '前': '',
    '固': '',
    
    // Common operations
    '初始化': 'initialize',
    '启动': 'start',
    '停止': 'stop',
    '暂停': 'pause',
    '恢复': 'resume',
    '重置': 'reset',
    '刷新': 'refresh',
    '更新': 'update',
    '同步': 'synchronize',
    '异步': 'asynchronous',
    '阻塞': 'block',
    '非阻塞': 'non-blocking',
    '超时': 'timeout',
    '重试': 'retry',
    '回滚': 'rollback',
    '提交': 'commit',
    '撤销': 'rollback',
    '保存': 'save',
    '加载': 'load',
    '导出': 'export',
    '导入': 'import',
    '序列化': 'serialize',
    '反序列化': 'deserialize',
    '编码': 'encode',
    '解码': 'decode',
    '加密': 'encrypt',
    '解密': 'decrypt',
    '压缩': 'compress',
    '解压': 'decompress',
    '验证': 'validate',
    '校验': 'verify',
    '检查': 'check',
    '测试': 'test',
    '调试': 'debug',
    '优化': 'optimize',
    '重构': 'refactor',
    '维护': 'maintain',
    '部署': 'deploy',
    '发布': 'release',
    '版本': 'version',
    '升级': 'upgrade',
    '降级': 'downgrade',
    '迁移': 'migrate',
    '备份': 'backup',
    '恢复': 'restore',
    '监控': 'monitor',
    '日志': 'log',
    '审计': 'audit',
    '追踪': 'trace',
    '分析': 'analyze',
    '报告': 'report',
    '统计': 'statistics',
    '度量': 'metric',
    '指标': 'indicator',
    '性能': 'performance',
    '可靠性': 'reliability',
    '可用性': 'availability',
    '可扩展性': 'scalability',
    '安全性': 'security',
    '隐私性': 'privacy',
    '兼容性': 'compatibility',
    '互操作性': 'interoperability',
    '可维护性': 'maintainability',
    '可测试性': 'testability',
    '可读性': 'readability',
    '可理解性': 'understandability',
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
