const fs = require('fs');
const path = require('path');

// WebSocket-specific translations
const translations = {
    // WebSocket protocol
    '实现': 'implementation',
    '完全': 'complete',
    '主': 'main',
    '协议': 'protocol',
    '握手': 'handshake',
    '握手端': 'handshake endpoint',
    '验证握手': 'validate handshake',
    '接收': 'receive',
    '发送': 'send',
    '发送文本消息': 'send text message',
    '发送二进制消息': 'send binary message',
    '二进制': 'binary',
    '进制': '进制',
    '消息': 'message',
    '接收数据': 'receive data',
    '回调': 'callback',
    '帧头': 'frame header',
    '构建帧': 'build frame',
    '应用掩码': 'apply mask',
    '生成': 'generate',
    '验证': 'validate',
    
    // Request related
    '索引': 'index',
    '扩展': 'extension',
    '扩容': 'expand',
    '容': 'capacity',
    
    // Allocator related
    '分': 'allocate',
    '分配': 'allocate',
    
    // Common particles
    '实': '',
    '完': '',
    '全': '',
    '主': '',
    '协': '',
    '握': '',
    '手': '',
    '端': '',
    '验': '',
    '证': '',
    '接': '',
    '收': '',
    '发': '',
    '送': '',
    '文': '',
    '本': '',
    '消': '',
    '二': '',
    '进': '',
    '制': '',
    '据': '',
    '回': '',
    '调': '',
    '帧': '',
    '头': '',
    '构': '',
    '建': '',
    '应': '',
    '用': '',
    '掩': '',
    '码': '',
    '生': '',
    '成': '',
    '索': '',
    '引': '',
    '扩': '',
    '容': '',
    '分': '',
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