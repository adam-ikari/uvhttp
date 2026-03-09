const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

const patterns = [
    // Connection patterns
    { regex: /initializationtimeout timer/g, replacement: 'initialize timeout timer' },
    { regex: /HTTP\/1\.1optimization：initializationdefaultvalue/g, replacement: 'HTTP/1.1 optimization: initialize default value' },
    { regex: /HTTP解析stateinitialization/g, replacement: 'HTTP parsing state initialization' },
    { regex: /TCPinitialization - 完整implement/g, replacement: 'TCP initialization - complete implementation' },
    { regex: /对于initializationfailure情况，直接freememory即可/g, replacement: 'for initialization failure cases, directly free memory' },
    { regex: /正确initializationrequestobject（containsHTTP解析器）/g, replacement: 'correctly initialize request object (contains HTTP parser)' },
    { regex: /正确initializationresponseobject/g, replacement: 'correctly initialize response object' },
    { regex: /直接free，不needcleanup（becauseinitializationfailure）/g, replacement: 'directly free, no cleanup needed (because initialization failure)' },

    // Static file patterns
    { regex: /initialization sendfile contextconfigurationparameter/g, replacement: 'initialize sendfile context configuration parameters' },
    { regex: /initialization为disable/g, replacement: 'initialized as disabled' },
    { regex: /initializationconfigurationparameter/g, replacement: 'initialize configuration parameters' },
    { regex: /initializationtimeout timer/g, replacement: 'initialize timeout timer' },

    // Router patterns
    { regex: /initializationnew增节点/g, replacement: 'initialize newly added nodes' },
    { regex: /initializationarray路由/g, replacement: 'initialize array router' },
    { regex: /initialization节点池（used forTrie）/g, replacement: 'initialize node pool (used for Trie)' },
    { regex: /将allarray路由migrate到Trie/g, replacement: 'migrate all array routes to Trie' },
    { regex: /构建路由tree/g, replacement: 'build routing tree' },
    { regex: /cleanup已allocatememory/g, replacement: 'cleanup already allocated memory' },
    { regex: /sethandle器/g, replacement: 'set handler' },
    { regex: /切换到Triemode/g, replacement: 'switch to Trie mode' },
    { regex: /free旧array路由memory/g, replacement: 'free old array router memory' },
    { regex: /checkpathwhether contains查询string（不允许）/g, replacement: 'check if path contains query string (not allowed)' },
    { regex: /ifhaveparameteror路由count超过阈value，useTrie/g, replacement: 'if has parameters or route count exceeds threshold, use Trie' },
    { regex: /add到Trie/g, replacement: 'add to Trie' },
    { regex: /saveparameter名 - optimization：只calculate一次strlen/g, replacement: 'save parameter name - optimization: calculate strlen only once' },
    { regex: /add到array/g, replacement: 'add to array' },
    { regex: /ifis叶子节点/g, replacement: 'if is leaf node' },
    { regex: /find匹配子节点/g, replacement: 'find matching child node' },
    { regex: /parameter节点，匹配任意segment/g, replacement: 'parameter node, matches any segment' },
    { regex: /回溯/g, replacement: 'backtrack' },
    { regex: /精确匹配/g, replacement: 'exact match' },
    { regex: /静态filerequesthandle器包装function/g, replacement: 'static file request handler wrapper function' },
    { regex: /获取connection/g, replacement: 'get connection' },
    { regex: /从 client 获取 connection/g, replacement: 'get connection from client' },
    { regex: /获取router/g, replacement: 'get router' },
    { regex: /call静态filehandlefunction/g, replacement: 'call static file handling function' },
    { regex: /静态file服务failure，return 404/g, replacement: 'static file service failed, return 404' },
    { regex: /according towhen前mode选择findway/g, replacement: 'choose find method according to current mode' },
    { regex: /解析pathsegment/g, replacement: 'parse path segments' },
    { regex: /首先check静态路由/g, replacement: 'first check static routes' },
    { regex: /匹配静态路由，return静态filehandle器/g, replacement: 'matched static route, return static file handler' },
    { regex: /匹配路由/g, replacement: 'match route' },
    { regex: /ifwithout匹配路由，check回退路由/g, replacement: 'if no matching route, check fallback route' },
    { regex: /optimization1：fastpath - checkarray路由（适used forfew量路由）/g, replacement: 'optimization 1: fast path - check array routes (suitable for few routes)' },
    { regex: /optimization2：fastpath - check静态路由（无parameter）/g, replacement: 'optimization 2: fast path - check static routes (no parameters)' },
    { regex: /对于withoutparameterpath，usefastfind/g, replacement: 'for paths without parameters, use fast find' },
    { regex: /无parameterpath，usearray路由fastfind/g, replacement: 'path without parameters, use array router fast find' },
    { regex: /但needcheck array_routes is否仍然have效/g, replacement: 'but need to check if array_routes is still valid' },
    { regex: /optimization3：Trietree匹配（supportparameter）/g, replacement: 'optimization 3: Trie tree matching (supports parameters)' },
    { regex: /add静态file路由/g, replacement: 'add static file route' },
    { regex: /freebefore前缀/g, replacement: 'free previous prefix' },
    { regex: /copynew前缀（use uvhttp_alloc 避免混用allocate器）/g, replacement: 'copy new prefix (use uvhttp_alloc to avoid mixing allocators)' },
    { regex: /将use静态filehandle逻辑/g, replacement: 'will use static file handling logic' },
    { regex: /add回退路由/g, replacement: 'add fallback route' },

    // LRU cache patterns
    { regex: /单线程version：不need加锁/g, replacement: 'single-threaded version: no locking needed' },
    { regex: /checkis否过期/g, replacement: 'check if expired' },
    { regex: /remove过期条目/g, replacement: 'remove expired entries' },
    { regex: /更newaccesstime并移到LRUheader部/g, replacement: 'update access time and move to LRU head' },
    { regex: /移动条目到LRU链tableheader部/g, replacement: 'move entry to LRU linked list head' },
    { regex: /if已经isheader部，无需移动/g, replacement: 'if already at head, no need to move' },
    { regex: /checkentryis否at链table中（prevent野pointer）/g, replacement: 'check if entry is in linked list (prevent wild pointers)' },
    { regex: /entry不at链table中，直接add到header部/g, replacement: 'entry not in linked list, directly add to head' },
    { regex: /if链table为空，尾部也指向entry/g, replacement: 'if linked list is empty, tail also points to entry' },
    { regex: /从when前positionremove/g, replacement: 'remove from current position' },
    { regex: /entryisheader部/g, replacement: 'entry is head' },
    { regex: /entryis尾部/g, replacement: 'entry is tail' },
    { regex: /移动到header部/g, replacement: 'move to head' },
    { regex: /从LRU链table尾部remove条目/g, replacement: 'remove entry from LRU linked list tail' },
    { regex: /更new尾部pointer/g, replacement: 'update tail pointer' },
    { regex: /链table为空，header部也置空/g, replacement: 'linked list is empty, set head to NULL' },
    { regex: /cleanup链tablepointer/g, replacement: 'cleanup linked list pointers' },
    { regex: /checkcache条目is否过期/g, replacement: 'check if cache entry is expired' },
    { regex: /supporttesttime Mock function（弱符号）/g, replacement: 'support test time Mock function (weak symbol)' },
    { regex: /NULL 条目视为过期/g, replacement: 'NULL entry is considered expired' },
    { regex: /TTL 为 0 table示永不过期/g, replacement: 'TTL of 0 means never expires' },
    { regex: /use get_current_time\(\) 以support Mock timesystem（used fortest）/g, replacement: 'use get_current_time() to support Mock time system (for testing)' },
    { regex: /addor更newcache条目 - 单线程version/g, replacement: 'add or update cache entry - single-threaded version' },
    { regex: /checkfilelargesmallis否超过限制/g, replacement: 'check if file size exceeds limit' },
    { regex: /calculatememoryuse量，check integer overflow/g, replacement: 'calculate memory usage, check integer overflow' },
    { regex: /checkis否need驱逐条目 - 批量驱逐optimization/g, replacement: 'check if need to evict entries - batch eviction optimization' },
    { regex: /ifcache为空但仍然need驱逐，note无法满足condition，returnerror/g, replacement: 'if cache is empty but still need eviction, note unable to satisfy condition, return error' },
    { regex: /批量驱逐many个条目以reduceloopcount/g, replacement: 'batch evict multiple entries to reduce loop count' },
    { regex: /从哈希table中remove/g, replacement: 'remove from hash table' },
    { regex: /更新statistics/g, replacement: 'update statistics' },
    { regex: /if一次批量驱逐后仍have空间不足，continue下一批/g, replacement: 'if still insufficient space after one batch eviction, continue next batch' },
    { regex: /findis否已存at/g, replacement: 'find if already exists' },
    { regex: /更newexisting条目/g, replacement: 'update existing entry' },
    { regex: /更新memoryuse量/g, replacement: 'update memory usage' },
    { regex: /validatefilepathlength，preventbuffer溢出/g, replacement: 'validate file path length, prevent buffer overflow' },
    { regex: /initializationLRU链tablepointer/g, replacement: 'initialize LRU linked list pointers' },
    { regex: /allocate并copycontent/g, replacement: 'allocate and copy content' },
    { regex: /new条目，need从哈希table中remove并free/g, replacement: 'new entry, need to remove from hash table and free' },
    { regex: /set条目content/g, replacement: 'set entry content' },
    { regex: /移动到LRUheader部/g, replacement: 'move to LRU head' },
    { regex: /deletecache条目 - 单线程version/g, replacement: 'delete cache entry - single-threaded version' },
    { regex: /清空allcache - 单线程version/g, replacement: 'clear all cache - single-threaded version' },
    { regex: /get cache statistics information - 单线程version/g, replacement: 'get cache statistics information - single-threaded version' },
    { regex: /resetstatisticsinformation - 单线程version/g, replacement: 'reset statistics information - single-threaded version' },
    { regex: /cleanup过期条目 - 单线程version/g, replacement: 'cleanup expired entries - single-threaded version' },
    { regex: /calculatecache命中率 - 单线程version/g, replacement: 'calculate cache hit rate - single-threaded version' },

    // Server patterns
    { regex: /requestandresponseobject已atconnectioncreatetimeinitialization/g, replacement: 'request and response objects already initialized at connection creation time' },
    { regex: /initializationTLSmodule（if还withoutinitialization）/g, replacement: 'initialize TLS module (if not yet initialized)' },
    { regex: /initializationconnection限制defaultvalue/g, replacement: 'initialize connection limit default value' },
    { regex: /initializationWebSocket路由table/g, replacement: 'initialize WebSocket routing table' },
    { regex: /initialization限流功canwordsegment/g, replacement: 'initialize rate limiting function segments' },
    { regex: /initialization限流state/g, replacement: 'initialize rate limiting state' },
    { regex: /initialization超time检测定time器/g, replacement: 'initialize timeout detection timer' },
    { regex: /initialization心跳检测定time器/g, replacement: 'initialize heartbeat detection timer' },
    { regex: /内部function：更new WebSocket connection活动time/g, replacement: 'internal function: update WebSocket connection activity time' },
    { regex: /clear待handle Ping mark/g, replacement: 'clear pending Ping mark' },

    // Router cache patterns
    { regex: /- ifneedatmany线程环境中use，请为each线程create独立serverinstanceandrouter/g, replacement: '- if need to use in multi-threaded environment, please create independent server instance and router for each thread' },

    // TLS patterns
    { regex: /if context 中 TLS 资源未initialize，先initialize/g, replacement: 'if TLS resources in context not initialized, initialize first' },
    { regex: /allocate并initialize entropy 上下文/g, replacement: 'allocate and initialize entropy context' },
    { regex: /allocate并initialize DRBG 上下文/g, replacement: 'allocate and initialize DRBG context' },
    { regex: /usecustom熵源initialize DRBG/g, replacement: 'use custom entropy source to initialize DRBG' },

    // Request patterns
    { regex: /initializationHTTP解析器/g, replacement: 'initialize HTTP parser' },
    { regex: /initializationbodybuffer/g, replacement: 'initialize body buffer' }
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