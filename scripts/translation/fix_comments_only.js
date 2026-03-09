const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const includeDir = '/home/zhaodi-chen/project/uvhttp/include';

// Get all C and H files
const srcFiles = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));
const includeFiles = fs.readdirSync(includeDir).filter(f => f.endsWith('.h'));
const allFiles = [...srcFiles.map(f => path.join(srcDir, f)), ...includeFiles.map(f => path.join(includeDir, f))];

// Only fix Chinese punctuation in comments (safe operation)
const punctuationPatterns = [
    { pattern: /（/g, replacement: '(' },
    { pattern: /）/g, replacement: ')' },
    { pattern: /【/g, replacement: '[' },
    { pattern: /】/g, replacement: ']' },
    { pattern: /《/g, replacement: '<' },
    { pattern: /》/g, replacement: '>' },
    { pattern: /"/g, replacement: '"' },
    { pattern: /"/g, replacement: '"' },
    { pattern: /'/g, replacement: "'" },
    { pattern: /'/g, replacement: "'" },
    { pattern: /，/g, replacement: ',' },
    { pattern: /。/g, replacement: '.' },
    { pattern: /：/g, replacement: ':' },
    { pattern: /；/g, replacement: ';' }
];

let totalFixed = 0;

allFiles.forEach(filePath => {
    const content = fs.readFileSync(filePath, 'utf8');
    let result = content;

    // Only process lines that are comments
    const lines = result.split('\n');
    const processedLines = lines.map(line => {
        // Check if line is a comment (starts with // or contains /*)
        if (line.trim().startsWith('//') || line.includes('/*')) {
            let processedLine = line;
            punctuationPatterns.forEach(({pattern, replacement}) => {
                processedLine = processedLine.replace(pattern, replacement);
            });
            return processedLine;
        }
        return line;
    });

    result = processedLines.join('\n');

    if (content !== result) {
        fs.writeFileSync(filePath, result, 'utf8');
        console.log('Fixed:', path.basename(filePath));
        totalFixed++;
    }
});

console.log('\nTotal files fixed:', totalFixed);