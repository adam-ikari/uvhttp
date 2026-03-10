const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

// Handle all remaining Chinese patterns
const patterns = [
    { regex: /initial化/g, replacement: 'initialize' },
    { regex: /initialization/g, replacement: 'initialize' },
    { regex: /requestandresponseobject/g, replacement: 'request and response objects' },
    { regex: /已at/g, replacement: 'already at' },
    { regex: /createtime/g, replacement: 'creation time' },
    { regex: /TLSmodule（if还without/g, replacement: 'TLS module (if not yet' },
    { regex: /connection限制/g, replacement: 'connection limit' },
    { regex: /defaultvalue/g, replacement: 'default value' },
    { regex: /路由table/g, replacement: 'routing table' },
    { regex: /限流功can/g, replacement: 'rate limiting function' },
    { regex: /wordsegment/g, replacement: 'segments' },
    { regex: /state/g, replacement: 'state' },
    { regex: /超time/g, replacement: 'timeout' },
    { regex: /检测定time器/g, replacement: 'detection timer' },
    { regex: /心跳/g, replacement: 'heartbeat' },
    { regex: /if context 中/g, replacement: 'if in context' },
    { regex: /资源未/g, replacement: 'resources not' },
    { regex: /先/g, replacement: 'first' },
    { regex: /allocate并/g, replacement: 'allocate and' },
    { regex: /entropy 上下文/g, replacement: 'entropy context' },
    { regex: /DRBG 上下文/g, replacement: 'DRBG context' },
    { regex: /usecustom熵源/g, replacement: 'use custom entropy source' },
    { regex: /HTTP解析器/g, replacement: 'HTTP parser' },
    { regex: /bodybuffer/g, replacement: 'body buffer' },
    { regex: /HTTP\/1\.1optimization：/g, replacement: 'HTTP/1.1 optimization: ' },
    { regex: /defaultvalue/g, replacement: 'default value' },
    { regex: /HTTP解析/g, replacement: 'HTTP parsing' },
    { regex: /TCP/g, replacement: 'TCP' },
    { regex: /完整implement/g, replacement: 'complete implementation' },
    { regex: /对于/g, replacement: 'for' },
    { regex: /failure情况/g, replacement: 'failure cases' },
    { regex: /直接freememory即可/g, replacement: 'directly free memory' },
    { regex: /正确/g, replacement: 'correctly' },
    { regex: /requestobject（/g, replacement: 'request object (' },
    { regex: /responseobject/g, replacement: 'response object' },
    { regex: /直接free，不needcleanup（/g, replacement: 'directly free, no cleanup needed (' },
    { regex: /because/g, replacement: 'because' },
    { regex: /new增/g, replacement: 'newly added' },
    { regex: /array路由/g, replacement: 'array router' },
    { regex: /节点池（/g, replacement: 'node pool (' },
    { regex: /used for/g, replacement: 'used for' },
    { regex: /Trie）/g, replacement: 'Trie)' },
    { regex: /ifneedat/g, replacement: 'if need at' },
    { regex: /many线程环境中use/g, replacement: 'multi-threaded environment use' },
    { regex: /请为/g, replacement: 'please for' },
    { regex: /each线程create/g, replacement: 'each thread create' },
    { regex: /独立/g, replacement: 'independent' },
    { regex: /serverinstance/g, replacement: 'server instance' },
    { regex: /androuter/g, replacement: 'and router' },
    { regex: /LRU链/g, replacement: 'LRU linked' },
    { regex: /tablepointer/g, replacement: 'list pointers' }
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