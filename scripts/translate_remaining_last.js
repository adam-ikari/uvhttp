const fs = require('fs');
const path = require('path');

// Final translations for remaining Chinese characters
const translations = {
    // Server
    '法': 'method',
    '受': 'receive',
    '留': 'keep',
    '备': 'prepare',
    '来': 'come',
    '特': 'special',
    '声': 'declare',
    '明': 'clear',
    '声明': 'declaration',
    '暂': 'temporary',
    
    // Middleware
    '编': 'compile',
    '译': 'translate',
    '编译': 'compile',
    '设': 'set',
    '原': 'original',
    '设原': 'set original',
    '抽': 'abstract',
    '象': 'image',
    '抽象': 'abstract',
    '顺': 'smooth',
    '序': 'order',
    '顺序': 'sequence',
    '洁': 'clean',
    '晰': 'clear',
    '先': 'first',
    '复': 'complex',
    '杂': 'complex',
    '先复': 'first complex',
    '复杂': 'complex',
    '概': 'overview',
    '念': 'concept',
    '概念': 'concept',
    '逻': 'logic',
    '辑': 'logic',
    '逻辑': 'logic',
    '了': 'completed',
    '该': 'should',
    '者': 'person',
    '直': 'straight',
    '推': 'push',
    '荐': 'recommend',
    '立': 'establish',
    '即': 'immediate',
    '该立': 'should establish',
    '该立即': 'should immediately',
    
    // Context
    '采': 'adopt',
    '采编': 'adopt compile',
    '采编译': 'adopt compile',
    '采编译设': 'adopt compile set',
    '采编译设抽': 'adopt compile set abstract',
    '提': 'lift',
    '供': 'provide',
    '提供': 'provide',
    '原因': 'reason',
    '联': 'link',
    '直复': 'straight complex',
    '详': 'detail',
    '见': 'see',
    '详细': 'detail',
    '初': 'initial',
    '化': 'change',
    '初始化': 'initialize',
    '需': 'need',
    '库': 'library',
    '编译库': 'compile library',
    
    // TLS
    '订': 'order',
    '票': 'ticket',
    
    // Constants
    '误': 'error',
    '判': 'judge',
    '差': 'difference',
    '误判': 'misjudge',
    '误判差': 'misjudge difference',
    '般': 'general',
    '良': 'good',
    '好': 'good',
    '良好': 'good',
    '误判良': 'misjudge good',
    '误判良好': 'misjudge good',
    '字': 'character',
    
    // Main header
    '核': 'core',
    '核心': 'core',
    '启': 'start',
    '启服': 'start serve',
    '启服务': 'start service',
    '启服务': 'start service',
    
    // Empty strings for particles
    '法': '',
    '受': '',
    '留': '',
    '备': '',
    '来': '',
    '特': '',
    '声': '',
    '明': '',
    '暂': '',
    '编': '',
    '译': '',
    '设': '',
    '原': '',
    '抽': '',
    '象': '',
    '顺': '',
    '序': '',
    '洁': '',
    '晰': '',
    '先': '',
    '复': '',
    '杂': '',
    '概': '',
    '念': '',
    '逻': '',
    '辑': '',
    '了': '',
    '该': '',
    '者': '',
    '直': '',
    '推': '',
    '荐': '',
    '立': '',
    '即': '',
    '采': '',
    '提': '',
    '供': '',
    '因': '',
    '联': '',
    '详': '',
    '见': '',
    '细': '',
    '初': '',
    '化': '',
    '需': '',
    '库': '',
    '订': '',
    '票': '',
    '误': '',
    '判': '',
    '差': '',
    '般': '',
    '良': '',
    '好': '',
    '字': '',
    '核': '',
    '心': '',
    '启': '',
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