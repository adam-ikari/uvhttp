const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const includeDir = '/home/zhaodi-chen/project/uvhttp/include';

// Get all C and H files
const srcFiles = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));
const includeFiles = fs.readdirSync(includeDir).filter(f => f.endsWith('.h'));
const allFiles = [...srcFiles.map(f => path.join(srcDir, f)), ...includeFiles.map(f => path.join(includeDir, f))];

// Comprehensive mixed phrase translations
const translations = [
    // Common words
    ['约', 'approximately'],
    ['此function', 'this function'],
    ['会', 'will'],
    ['并', 'and'],
    ['重启', 'restart'],
    ['现有的', 'existing'],
    ['定when器', 'timer'],
    ['if有', 'if there is'],
    ['自definition', 'custom'],
    ['timeoutwhen间', 'timeout period'],
    ['toconnection', 'to connect'],
    ['starttimeout', 'start timeout'],
    ['定when器', 'timer'],
    ['use指定的', 'use specified'],
    ['timeoutwhen间', 'timeout period'],
    ['秒', 'seconds'],
    ['范围', 'range'],
    ['timeoutwhen间must', 'timeout period must'],
    ['之间', 'between'],
    ['iftimeoutwhen间过large', 'if timeout period is too large'],
    ['导致', 'causes'],
    ['整数溢出', 'integer overflow'],
    ['return', 'return'],
    ['危险字符constant', 'dangerous character constants'],
    ['内联verificationfunction', 'inline verification functions'],
    ['辅助function', 'helper function'],
    ['checkstring中whetherinclude', 'check if string contains'],
    ['控制字符', 'control characters'],
    ['包括换line符', 'including newline characters'],
    ['checkwhetherinclude', 'check whether includes'],
    ['但排除', 'but excluding'],
    ['制表符', 'tab character'],
    ['空格', 'space'],
    ['include控制字符', 'contains control characters'],
    ['明确check', 'explicitly check'],
    ['回车符', 'carriage return'],
    ['换line符', 'newline'],
    ['防止', 'prevent'],
    ['HTTPresponse拆分攻击', 'HTTP response splitting attack'],
    ['HTTPresponse拆分尝试', 'HTTP response splitting attempt'],
    ['checkdelete字符', 'check for delete character'],
    ['includedelete字符', 'contains delete character'],
    ['遍历现有headers', 'iterate through existing headers'],
    ['securitycheck', 'security check'],
    ['verificationheadervalue不include', 'verify header value does not contain'],
    ['防止response拆分', 'prevent response splitting'],
    ['ifheadervalueinclude', 'if header value contains'],
    ['跳过该header', 'skip this header'],
    ['HTTP/1.1要求', 'HTTP/1.1 requirement'],
    ['must有', 'must have'],
    ['orusechunkedencoding', 'or use chunked encoding'],
    ['这里我们总是add', 'here we always add'],
    ['确保protocol合规性', 'ensure protocol compliance'],
    ['即使没有body也要', 'even if there is no body, still'],
    ['setContent-Length', 'set Content-Length'],
    ['optimize', 'optimize'],
    ['root据', 'based on'],
    ['keep-aliveset', 'keep-alive set'],
    ['结束headers', 'end headers'],
    ['default保持connection', 'default keep connection'],
    ['未send', 'not sent'],
    ['未完成', 'not finished'],
    ['初始capacity', 'initial capacity'],
    ['个内联 headers', 'inline headers'],
    ['verificationstatus code范围', 'verify status code range'],
    ['verificationheader名称和value', 'verify header name and value'],
    ['额外verification', 'additional verification'],
    ['checkheadervaluewhetherinclude', 'check if header value contains'],
    ['checkwhether需要扩容', 'check if expansion is needed'],
    ['计算新capacity', 'calculate new capacity'],
    ['最多', 'maximum'],
    ['if新capacity等于currentcapacity', 'if new capacity equals current capacity'],
    ['说明already达tomaximumvalue', 'indicates already reached maximum value'],
    ['already满', 'already full'],
    
    // Common single characters and short words
    ['约', 'approximately'],
    ['此', 'this'],
    ['会', 'will'],
    ['并', 'and'],
    ['有', 'have'],
    ['自', 'custom'],
    ['秒', 'seconds'],
    ['范围', 'range'],
    ['之间', 'between'],
    ['导致', 'causes'],
    ['包括', 'including'],
    ['排除', 'excluding'],
    ['明确', 'explicitly'],
    ['防止', 'prevent'],
    ['遍历', 'iterate'],
    ['现有', 'existing'],
    ['总是', 'always'],
    ['即使', 'even if'],
    ['也要', 'still'],
    ['结束', 'end'],
    ['未', 'not'],
    ['初始', 'initial'],
    ['额外', 'additional'],
    ['需要', 'need'],
    ['新', 'new'],
    ['最多', 'maximum'],
    ['等于', 'equals'],
    ['说明', 'indicates'],
    ['已经', 'already'],
    ['达到', 'reached'],
    ['满', 'full'],
    
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
