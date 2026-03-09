const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

const patterns = [
    // Static file patterns
    { regex: /initialization sendfile contextconfigurationparameter/g, replacement: 'initialize sendfile context configuration parameters' },
    { regex: /initialization为disable/g, replacement: 'initialized as disabled' },
    { regex: /initializationconfigurationparameter/g, replacement: 'initialize configuration parameters' },
    { regex: /initializationtimeout timer/g, replacement: 'initialize timeout timer' },

    // Server patterns
    { regex: /requestandresponseobject已atconnectioncreatetimeinitialization/g, replacement: 'request and response objects already initialized at connection creation time' },
    { regex: /initializationTLSmodule（if还withoutinitialization）/g, replacement: 'initialize TLS module (if not yet initialized)' },
    { regex: /initializationconnection限制defaultvalue/g, replacement: 'initialize connection limit default value' },
    { regex: /initializationWebSocket路由table/g, replacement: 'initialize WebSocket routing table' },
    { regex: /initialization限流功canwordsegment/g, replacement: 'initialize rate limiting function segments' },
    { regex: /initialization限流state/g, replacement: 'initialize rate limiting state' },
    { regex: /initialization超time检测定time器/g, replacement: 'initialize timeout detection timer' },
    { regex: /initialization心跳检测定time器/g, replacement: 'initialize heartbeat detection timer' },

    // TLS patterns
    { regex: /if context 中 TLS 资源未initialize，先initialize/g, replacement: 'if TLS resources in context not initialized, initialize first' },
    { regex: /allocate并initialize entropy 上下文/g, replacement: 'allocate and initialize entropy context' },
    { regex: /allocate并initialize DRBG 上下文/g, replacement: 'allocate and initialize DRBG context' },
    { regex: /usecustom熵源initialize DRBG/g, replacement: 'use custom entropy source to initialize DRBG' },

    // Request patterns
    { regex: /initializationHTTP解析器/g, replacement: 'initialize HTTP parser' },
    { regex: /initializationbodybuffer/g, replacement: 'initialize body buffer' },

    // Connection patterns
    { regex: /initializationtimeout timer/g, replacement: 'initialize timeout timer' },
    { regex: /HTTP\/1\.1optimization：initializationdefaultvalue/g, replacement: 'HTTP/1.1 optimization: initialize default value' },
    { regex: /HTTP解析stateinitialization/g, replacement: 'HTTP parsing state initialization' },
    { regex: /TCPinitialization - 完整implement/g, replacement: 'TCP initialization - complete implementation' },
    { regex: /对于initializationfailure情况，直接freememory即可/g, replacement: 'for initialization failure cases, directly free memory' },
    { regex: /正确initializationrequestobject（containsHTTP解析器）/g, replacement: 'correctly initialize request object (contains HTTP parser)' },
    { regex: /正确initializationresponseobject/g, replacement: 'correctly initialize response object' },
    { regex: /直接free，不needcleanup（becauseinitializationfailure）/g, replacement: 'directly free, no cleanup needed (because initialization failure)' },

    // Router patterns
    { regex: /initializationnew增节点/g, replacement: 'initialize newly added nodes' },
    { regex: /initializationarray路由/g, replacement: 'initialize array router' },
    { regex: /initialization节点池（used forTrie）/g, replacement: 'initialize node pool (used for Trie)' },

    // LRU cache patterns
    { regex: /initializationLRU链tablepointer/g, replacement: 'initialize LRU linked list pointers' },

    // Router cache patterns
    { regex: /- ifneedatmany线程环境中use，请为each线程create独立serverinstanceandrouter/g, replacement: '- if need to use in multi-threaded environment, please create independent server instance and router for each thread' }
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