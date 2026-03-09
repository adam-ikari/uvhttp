const fs = require('fs');
const path = require('path');

// Final translations for constants
const translations = {
    // Configuration constants
    '称': 'name',
    '要': 'require',
    '求': 'request',
    '要求': 'requirement',
    '符': 'character',
    '足': 'sufficient',
    '结': 'result',
    '硬': 'hard',
    '控': 'control',
    '拒': 'reject',
    '绝': 'absolute',
    '拒绝': 'reject',
    '占': 'occupy',
    '更': 'more',
    '态': 'state',
    '网': 'network',
    '络': 'network',
    '网络': 'network',
    '页': 'page',
    '页面': 'page',
    '版': 'version',
    '这': 'this',
    '些': 'some',
    '这些': 'these',
    '密': 'secret',
    '钥': 'key',
    '密钥': 'secret key',
    '慎': 'careful',
    '纯': 'pure',
    '线': 'line',
    '纯线': 'pure line',
    '转': 'convert',
    '换': 'change',
    '转换': 'convert',
    '依': 'depend',
    '基': 'base',
    '准': 'standard',
    '试': 'test',
    '依基': 'depend base',
    '依基准': 'depend base standard',
    '试结': 'test result',
    '试结了': 'test result completed',
    '资': 'resource',
    '源': 'source',
    '资源': 'resource',
    '论': 'theory',
    '结论': 'conclusion',
    '拟': 'simulate',
    '抖': 'shake',
    '丢': 'lose',
    '包': 'packet',
    '丢包': 'packet loss',
    '平': 'flat',
    '均': 'average',
    '平均': 'average',
    '益': 'benefit',
    '递': 'recursive',
    '减': 'reduce',
    '递减': 'decrease',
    '益递减': 'benefit decrease',
    '取': 'get',
    '得': 'obtain',
    '佳': 'best',
    '取得': 'obtain',
    '取得佳': 'obtain best',
    '持': 'hold',
    '续': 'continue',
    '写': 'write',
    '入': 'enter',
    '写入': 'write',
    '持续写入': 'continuous write',
    '批': 'batch',
    '批平': 'batch flat',
    '批平均': 'batch average',
    '触': 'touch',
    '拟触': 'simulate touch',
    
    // Empty strings for particles
    '称': '',
    '要': '',
    '求': '',
    '符': '',
    '足': '',
    '结': '',
    '硬': '',
    '控': '',
    '拒': '',
    '绝': '',
    '占': '',
    '更': '',
    '态': '',
    '网': '',
    '络': '',
    '页': '',
    '面': '',
    '版': '',
    '这': '',
    '些': '',
    '密': '',
    '钥': '',
    '慎': '',
    '纯': '',
    '线': '',
    '转': '',
    '换': '',
    '依': '',
    '基': '',
    '准': '',
    '试': '',
    '资': '',
    '源': '',
    '论': '',
    '拟': '',
    '抖': '',
    '丢': '',
    '包': '',
    '平': '',
    '均': '',
    '益': '',
    '递': '',
    '减': '',
    '取': '',
    '得': '',
    '佳': '',
    '持': '',
    '续': '',
    '写': '',
    '入': '',
    '批': '',
    '触': '',
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