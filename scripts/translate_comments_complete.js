const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const includeDir = '/home/zhaodi-chen/project/uvhttp/include';

// Get all C and H files
const srcFiles = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));
const includeFiles = fs.readdirSync(includeDir).filter(f => f.endsWith('.h'));
const allFiles = [...srcFiles.map(f => path.join(srcDir, f)), ...includeFiles.map(f => path.join(includeDir, f))];

// Comprehensive translations
const translations = [
    // Common words
    ['字节', 'bytes'],
    ['memory', 'memory'],
    ['verification', 'verification'],
    ['support', 'support'],
    ['create', 'create'],
    ['新的', 'new'],
    ['router', 'router'],
    ['输出', 'output'],
    ['parameter', 'parameter'],
    ['used for', 'used for'],
    ['receive', 'receive'],
    ['pointer', 'pointer'],
    ['return', 'return'],
    ['success', 'success'],
    ['其他', 'other'],
    ['value', 'value'],
    ['表示', 'indicates'],
    ['failure', 'failure'],
    ['note', 'note'],
    ['时', 'when'],
    ['被', 'is'],
    ['set', 'set'],
    ['为', 'to'],
    ['有效的', 'valid'],
    ['object', 'object'],
    ['必须', 'must'],
    ['use', 'use'],
    ['free', 'free'],
    ['NULL', 'NULL'],
    ['查找', 'lookup'],
    ['匹配', 'match'],
    ['get', 'get'],
    ['解析', 'parse'],
    ['method', 'method'],
    ['字符串', 'string'],
    ['转换', 'convert'],
    ['静态', 'static'],
    ['文件', 'file'],
    ['routing', 'routing'],
    ['相关', 'related'],
    ['layout', 'layout'],
    ['静态断言', 'static assertion'],
    ['pointer', 'pointer'],
    ['对齐', 'alignment'],
    ['平台', 'platform'],
    ['自适应', 'adaptive'],
    ['size_t', 'size_t'],
    ['brief', 'brief'],
    ['create', 'create'],
    ['新的', 'new'],
    ['输出', 'output'],
    ['parameter', 'parameter'],
    ['used', 'used'],
    ['for', 'for'],
    ['receive', 'receive'],
    ['router', 'router'],
    ['pointer', 'pointer'],
    ['return', 'return'],
    ['success', 'success'],
    ['其他', 'other'],
    ['value', 'value'],
    ['表示', 'indicates'],
    ['failure', 'failure'],
    ['note', 'note'],
    ['success', 'success'],
    ['时', 'when'],
    ['被', 'is'],
    ['set', 'set'],
    ['为', 'to'],
    ['有效的', 'valid'],
    ['router', 'router'],
    ['object', 'object'],
    ['必须', 'must'],
    ['use', 'use'],
    ['uvhttp_router_free', 'uvhttp_router_free'],
    ['failure', 'failure'],
    ['时', 'when'],
    ['被', 'is'],
    ['set', 'set'],
    ['为', 'to'],
    ['NULL', 'NULL'],
    ['查找', 'lookup'],
    ['匹配', 'match'],
    ['get', 'get'],
    ['parameter', 'parameter'],
    ['解析', 'parse'],
    ['parameter', 'parameter'],
    ['method', 'method'],
    ['字符串', 'string'],
    ['转换', 'convert'],
    ['静态', 'static'],
    ['文件', 'file'],
    ['routing', 'routing'],
    ['support', 'support'],
    ['memory', 'memory'],
    ['布局', 'layout'],
    ['verification', 'verification'],
    ['静态断言', 'static assertion'],
    ['pointer', 'pointer'],
    ['对齐', 'alignment'],
    ['平台', 'platform'],
    ['自适应', 'adaptive'],
    ['size_t', 'size_t'],
    ['对齐', 'alignment'],
    ['平台', 'platform'],
    ['自适应', 'adaptive'],
    
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
    ["'", "'"],
    ["'", "'"]
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
