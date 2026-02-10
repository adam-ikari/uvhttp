const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const includeDir = '/home/zhaodi-chen/project/uvhttp/include';

// Get all C and H files
const srcFiles = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));
const includeFiles = fs.readdirSync(includeDir).filter(f => f.endsWith('.h'));
const allFiles = [...srcFiles.map(f => path.join(srcDir, f)), ...includeFiles.map(f => path.join(includeDir, f))];

// Remaining Chinese punctuation patterns
const patterns = [
    { pattern: /（/g, replacement: '(' },
    { pattern: /）/g, replacement: ')' },
    { pattern: /【/g, replacement: '[' },
    { pattern: /】/g, replacement: ']' },
    { pattern: /《/g, replacement: '<' },
    { pattern: /》/g, replacement: '>' },
    { pattern: /"/g, replacement: '"' },
    { pattern: /"/g, replacement: '"' },
    { pattern: /'/g, replacement: "'" },
    { pattern: /'/g, replacement: "'" }
];

let totalFixed = 0;

allFiles.forEach(filePath => {
    const content = fs.readFileSync(filePath, 'utf8');
    let result = content;

    patterns.forEach(({pattern, replacement}) => {
        result = result.replace(pattern, replacement);
    });

    if (content !== result) {
        fs.writeFileSync(filePath, result, 'utf8');
        console.log('Fixed:', path.basename(filePath));
        totalFixed++;
    }
});

console.log('\nTotal files fixed:', totalFixed);