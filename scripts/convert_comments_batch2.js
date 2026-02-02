const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

const patterns = [
    // TLS patterns
    { regex: /if context 中 TLS 资源未initial化，先initialize/g, replacement: 'if TLS resources in context not initialized, initialize first' },
    { regex: /allocate并initialize entropy 上下文/g, replacement: 'allocate and initialize entropy context' },
    { regex: /allocate并initialize DRBG 上下文/g, replacement: 'allocate and initialize DRBG context' },
    { regex: /usecustom熵源initialize DRBG/g, replacement: 'use custom entropy source to initialize DRBG' },

    // Request patterns
    { regex: /initializationHTTP解析器/g, replacement: 'initialize HTTP parser' },
    { regex: /initializationbodybuffer/g, replacement: 'initialize body buffer' },

    // Connection patterns
    { regex: /performanceimprove：每次connection复用节省 277,920 bytememoryoperation/g, replacement: 'performance improvement: each connection reuse saves 277,920 byte memory operations' },
    { regex: /initialization超time定time器/g, replacement: 'initialize timeout timer' },
    { regex: /HTTP\/1\.1optimization：initializationdefaultvalue/g, replacement: 'HTTP/1.1 optimization: initialize default value' },
    { regex: /HTTP解析stateinitialization/g, replacement: 'HTTP parsing state initialization' },
    { regex: /TCPinitialization - 完整implement/g, replacement: 'TCP initialization - complete implementation' },
    { regex: /对于initializationfailure情况，直接freememory即可/g, replacement: 'for initialization failure cases, directly free memory' },
    { regex: /正确initializationrequestobject（containsHTTP解析器）/g, replacement: 'correctly initialize request object (contains HTTP parser)' },
    { regex: /正确initializationresponseobject/g, replacement: 'correctly initialize response object' },
    { regex: /直接free，不needcleanup（becauseinitializationfailure）/g, replacement: 'directly free, no cleanup needed (because initialization failure)' },
    { regex: /at握手responsesend后call，createWebSocketconnectionobject并setcallback/g, replacement: 'call after handshake response sent, create WebSocket connection object and set callback' },
    { regex: /获取requestpath/g, replacement: 'get request path' },
    { regex: /获取client IP address/g, replacement: 'get client IP address' },
    { regex: /从查询parameterorheader部获取 Token/g, replacement: 'get Token from query parameter or header' },
    { regex: /attempt从查询parameter获取/g, replacement: 'attempt to get from query parameter' },
    { regex: /finduser注册WebSockethandle器/g, replacement: 'find user registered WebSocket handler' },
    { regex: /continuecreateconnection，但usedefaultcallback/g, replacement: 'continue creating connection, but use default callback' },
    { regex: /获取TCPfiledescription符/g, replacement: 'get TCP file descriptor' },
    { regex: /createwrapper以saveconnectionobjectanduserhandle器/g, replacement: 'create wrapper to save connection object and user handler' },
    { regex: /setwrapper为WebSocketconnectionuser_data/g, replacement: 'set wrapper as WebSocket connection user_data' },
    { regex: /setcallbackfunction（内部callbackwillcallusercallback）/g, replacement: 'set callback function (internal callback will call user callback)' },
    { regex: /save到connectionobject/g, replacement: 'save to connection object' },
    { regex: /calluser注册connectioncallback/g, replacement: 'call user registered connection callback' },
    { regex: /切换到WebSocketdatareadmode/g, replacement: 'switch to WebSocket data read mode' },
    { regex: /add到connectionmanage器/g, replacement: 'add to connection manager' },
    { regex: /切换到WebSocketdatahandlemode/g, replacement: 'switch to WebSocket data handling mode' },
    { regex: /stopHTTPread，startWebSocket帧read/g, replacement: 'stop HTTP read, start WebSocket frame read' },
    { regex: /从connectionmanage器中remove/g, replacement: 'remove from connection manager' },
    { regex: /close底层TCPconnection/g, replacement: 'close underlying TCP connection' },
    { regex: /connection超timecallbackfunction/g, replacement: 'connection timeout callback function' },
    { regex: /获取超timetime间，if config 为 NULL 则usedefaultvalue/g, replacement: 'get timeout, if config is NULL use default value' },
    { regex: /触发application层超timestatisticscallback/g, replacement: 'trigger application layer timeout statistics callback' },
    { regex: /startconnection超time定time器/g, replacement: 'start connection timeout timer' },
    { regex: /stop旧定time器（ifhave）/g, replacement: 'stop old timer (if exists)' },
    { regex: /获取超timetime间，if config 为 NULL 则usedefaultvalue/g, replacement: 'get timeout, if config is NULL use default value' },
    { regex: /start定time器/g, replacement: 'start timer' },
    { regex: /startconnection超time定time器（custom超timetime间）/g, replacement: 'start connection timeout timer (custom timeout)' },
    { regex: /validate超timetime间range/g, replacement: 'validate timeout range' },

    // Router cache patterns
    { regex: /哈希桶structure/g, replacement: 'hash bucket structure' },
    { regex: /cacheoptimizationrouterstructure - 采用分层cachestrategy提highperformance/g, replacement: 'cache optimization router structure - using layered cache strategy to improve performance' },
    { regex: /线程securitynote：/g, replacement: 'thread safety note:' },
    { regex: /本implement专为单线程eventloop架构设计（基于 libuv）：/g, replacement: 'this implementation is designed for single-threaded event loop architecture (based on libuv):' },
    { regex: /- routermustat单个eventloop线程中use/g, replacement: '- router must be used in a single event loop thread' },
    { regex: /- 不supportmany线程concurrentaccess/g, replacement: '- does not support multi-threaded concurrent access' },
    { regex: /- ifneedatmany线程环境中use，请为each线程create独立serverinstanceandrouter/g, replacement: '- if need to use in multi-threaded environment, please create independent server instance and router for each thread' },
    { regex: /- 采用分层cachestrategy（热path \+ 哈希table）/g, replacement: '- using layered cache strategy (hot path + hash table)' },
    { regex: /- 热pathcache利用 CPU cache局部性/g, replacement: '- hot path cache utilizes CPU cache locality' },
    { regex: /- 无锁设计，避免互斥锁开销/g, replacement: '- lock-free design, avoiding mutex overhead' },
    { regex: /热pathcache：storage最常用16个路由，利用CPUcache局部性/g, replacement: 'hot path cache: store most commonly used 16 routes, utilizing CPU cache locality' },
    { regex: /哈希table：used forfastfindall路由（including冷path）/g, replacement: 'hash table: used for fast finding all routes (including cold paths)' },
    { regex: /路由statistics/g, replacement: 'route statistics' },
    { regex: /use统一hashfunction（内联function）/g, replacement: 'use unified hash function (inline function)' },
    { regex: /fastmethod解析（内联function）/g, replacement: 'fast method parsing (inline function)' },
    { regex: /use前缀fast判断/g, replacement: 'use prefix for fast judgment' },
    { regex: /add到哈希table/g, replacement: 'add to hash table' },
    { regex: /checkis否已存at相同路由/g, replacement: 'check if same route already exists' },
    { regex: /createnew条目/g, replacement: 'create new entry' },
    { regex: /add到热path/g, replacement: 'add to hot path' },
    { regex: /checkis否已存at/g, replacement: 'check if already exists' },
    { regex: /if热path未满，直接add/g, replacement: 'if hot path not full, directly add' },
    { regex: /热pathfull，replaceaccesscount最few（loopreplacestrategy）/g, replacement: 'hot path full, replace least access count (loop replacement strategy)' },
    { regex: /更newloopindex/g, replacement: 'update loop index' },
    { regex: /at哈希table中find/g, replacement: 'find in hash table' },
    { regex: /at热path中find/g, replacement: 'find in hot path' },
    { regex: /========== 公开 API function ==========/g, replacement: '========== Public API functions ==========' },
    { regex: /free哈希table/g, replacement: 'free hash table' },
    { regex: /先at热path中find/g, replacement: 'first find in hot path' },
    { regex: /if热path中没找到，at哈希table中find/g, replacement: 'if not found in hot path, find in hash table' },
    { regex: /暂不support静态file路由/g, replacement: 'temporarily does not support static file routes' },
    { regex: /暂不support回退路由/g, replacement: 'temporarily does not support fallback routes' },

    // Router patterns
    { regex: /fast前缀匹配/g, replacement: 'fast prefix matching' },
    { regex: /optimization：method到stringconvert（use直接index）/g, replacement: 'optimization: method to string conversion (use direct index)' },
    { regex: /use静态常量array，避免重复stringcompare/g, replacement: 'use static constant array, avoid repeated string comparison' },
    { regex: /createnew路由节点/g, replacement: 'create new route node' },
    { regex: /extend节点池/g, replacement: 'extend node pool' },
    { regex: /initializationnew增节点/g, replacement: 'initialize newly added nodes' },
    { regex: /findorcreate子节点/g, replacement: 'find or create child node' },
    { regex: /findexisting子节点/g, replacement: 'find existing child node' },
    { regex: /createnew子节点/g, replacement: 'create new child node' },
    { regex: /解析pathparameter/g, replacement: 'parse path parameters' },
    { regex: /checkis否isparameter（以:开header）/g, replacement: 'check if is parameter (starts with :)' },
    { regex: /initializationarray路由/g, replacement: 'initialize array router' },
    { regex: /initialization节点池（used forTrie）/g, replacement: 'initialize node pool (used for Trie)' },
    { regex: /defaultusearray路由/g, replacement: 'default use array router' },
    { regex: /array路由add/g, replacement: 'array router add' },
    { regex: /array路由find/g, replacement: 'array router find' },
    { regex: /migratearray路由到Trie/g, replacement: 'migrate array router to Trie' },
    { regex: /已经isTriemode/g, replacement: 'already in Trie mode' },
    { regex: /savearray路由pointer，稍后free/g, replacement: 'save array router pointer, free later' }
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