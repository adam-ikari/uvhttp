const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

const patterns = [
    { regex: /initialize为disable/g, replacement: 'initialized as disabled' },
    { regex: /initialize限流/g, replacement: 'initialize rate limiting' },
    { regex: /initializenewly added节点/g, replacement: 'initialize newly added nodes' },
    { regex: /if need atmany线程environment中use，please foreach thread createindependentserver instanceand router/g, replacement: 'if need to use in multi-threaded environment, please create independent server instance and router for each thread' }
];

let totalConverted = 0;

files.forEach(f => {
    const filePath = path.join(srcDir, f);
    const content = fs.readFileSync(filePath, 'utf8');
    let result = content;

    patterns.forEach(({regex, replacement}) => {
        result = result.replace(regex, replacement);
    });

    if (content !== result) {
        fs.writeFileSync(filePath, result, 'utf8');
        console.log('Updated:', f);
        totalConverted++;
    }
});

console.log('Total files updated:', totalConverted);