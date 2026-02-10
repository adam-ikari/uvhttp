const fs = require('fs');
const path = require('path');

// Translations for defaults and static file comments
const translations = {
    // Defaults
    '集': 'collection',
    '便': 'convenient',
    '于': 'for',
    '都': 'all',
    '以': 'can',
    '通': 'through',
    '覆': 'cover',
    '覆盖': 'override',
    '小': 'small',
    '合': 'combine',
    '多': 'multiple',
    '高': 'high',
    '场': 'field',
    '景': 'scene',
    '场景': 'scenario',
    '快': 'fast',
    '速': 'speed',
    '快速': 'fast',
    '普': 'general',
    '普通': 'general',
    '长': 'long',
    '超': 'super',
    '增': 'increase',
    '性': 'property',
    '体': 'body',
    '传': 'transfer',
    '操': 'operate',
    '作': 'operation',
    '每': 'each',
    '个': 'one',
    '池': 'pool',
    '阈': 'threshold',
    '种': 'kind',
    '子': 'child',
    
    // Static files
    '名': 'name',
    '服': 'serve',
    '服务': 'service',
    '拷': 'copy',
    '贝': 'shell',
    '拷贝': 'copy',
    '混': 'mix',
    '合': 'combine',
    '混合': 'mixed',
    '选': 'select',
    '择': 'choose',
    '选择': 'select',
    '优': 'excellent',
    '方': 'method',
    '避': 'avoid',
    '免': 'prevent',
    '避免': 'avoid',
    '销': 'destroy',
    '等': 'wait',
    '预': 'pre',
    '热': 'heat',
    '预热': 'preheat',
    
    // Particles
    '集': '',
    '便': '',
    '于': '',
    '都': '',
    '以': '',
    '通': '',
    '覆': '',
    '小': '',
    '合': '',
    '多': '',
    '高': '',
    '场': '',
    '景': '',
    '快': '',
    '速': '',
    '普': '',
    '长': '',
    '超': '',
    '增': '',
    '性': '',
    '体': '',
    '传': '',
    '操': '',
    '作': '',
    '每': '',
    '个': '',
    '池': '',
    '阈': '',
    '种': '',
    '子': '',
    '名': '',
    '服': '',
    '拷': '',
    '贝': '',
    '混': '',
    '选': '',
    '择': '',
    '优': '',
    '方': '',
    '避': '',
    '免': '',
    '销': '',
    '等': '',
    '预': '',
    '热': '',
    
    // Common phrases
    '集合': 'collection',
    '便于': 'convenient for',
    '都可以': 'all can',
    '通过': 'through',
    '覆盖': 'override',
    '小尺寸': 'small size',
    '适合多种': 'suitable for multiple',
    '高并发场景': 'high concurrency scenario',
    '快速': 'fast',
    '普通': 'general',
    '长连接': 'long connection',
    '超时': 'timeout',
    '增加': 'increase',
    '以性': 'with property',
    '整体': 'overall',
    '小传输': 'small transfer',
    '传输': 'transfer',
    '操作性': 'operability',
    '每个': 'each',
    '连接池': 'connection pool',
    '阈值': 'threshold',
    '种子': 'seed',
    '文件名': 'filename',
    '服务': 'service',
    '拷贝': 'copy',
    '混合': 'mixed',
    '选择': 'select',
    '优化': 'optimize',
    '方法': 'method',
    '避免': 'avoid',
    '销毁': 'destroy',
    '等待': 'wait',
    '预热': 'preheat',
    '预热服务': 'preheat service',
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