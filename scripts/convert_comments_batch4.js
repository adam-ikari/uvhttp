const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

const patterns = [
    // TLS patterns
    { regex: /if context 中 TLS 资源未initialize，先initialize/g, replacement: 'if TLS resources in context not initialized, initialize first' },
    { regex: /allocate并initialize entropy 上下文/g, replacement: 'allocate and initialize entropy context' },
    { regex: /allocate并initialize DRBG 上下文/g, replacement: 'allocate and initialize DRBG context' },
    { regex: /usecustom熵源initialize DRBG/g, replacement: 'use custom entropy source to initialize DRBG' },

    // Connection patterns
    { regex: /initialize超time定time器/g, replacement: 'initialize timeout timer' },
    { regex: /HTTP\/1\.1optimization：initializationdefaultvalue/g, replacement: 'HTTP/1.1 optimization: initialize default value' },
    { regex: /HTTP解析stateinitialization/g, replacement: 'HTTP parsing state initialization' },
    { regex: /TCPinitialization - 完整implement/g, replacement: 'TCP initialization - complete implementation' },
    { regex: /对于initializationfailure情况，直接freememory即可/g, replacement: 'for initialization failure cases, directly free memory' },
    { regex: /正确initializationrequestobject（containsHTTP解析器）/g, replacement: 'correctly initialize request object (contains HTTP parser)' },
    { regex: /正确initializationresponseobject/g, replacement: 'correctly initialize response object' },
    { regex: /直接free，不needcleanup（becauseinitializationfailure）/g, replacement: 'directly free, no cleanup needed (because initialization failure)' },
    { regex: /start connection timeout timer（custom超timetime间）/g, replacement: 'start connection timeout timer (custom timeout)' },

    // Static file patterns
    { regex: /TCP_CORK is否enable/g, replacement: 'whether TCP_CORK is enabled' },
    { regex: /超time定time器/g, replacement: 'timeout timer' },
    { regex: /sendfile defaultconfiguration（宏定义） - performanceoptimization/g, replacement: 'sendfile default configuration (macro definition) - performance optimization' },
    { regex: /initialization sendfile 上下文configurationparameter/g, replacement: 'initialize sendfile context configuration parameters' },
    { regex: /sendfile 上下文/g, replacement: 'sendfile context' },
    { regex: /configurationparameter（可为 NULL）/g, replacement: 'configuration parameters (can be NULL)' },
    { regex: /从configuration中read sendfile parameter/g, replacement: 'read sendfile parameters from configuration' },
    { regex: /performanceoptimization：according tofilelargesmall动态adjustchunk size/g, replacement: 'performance optimization: dynamically adjust chunk size according to file size' },
    { regex: /smallfile（< 1MB）：use较small分block，reducememory占用/g, replacement: 'small file (< 1MB): use smaller chunks, reduce memory usage' },
    { regex: /中等file（1-10MB）：usedefault分block/g, replacement: 'medium file (1-10MB): use default chunks' },
    { regex: /largefile（> 10MB）：use更large分block，reducesystemcallcount/g, replacement: 'large file (> 10MB): use larger chunks, reduce system call count' },
    { regex: /超timecallbackfunction/g, replacement: 'timeout callback function' },
    { regex: /mark为complete，prevent duplicate handling/g, replacement: 'mark as complete, prevent duplicate handling' },
    { regex: /stop定time器/g, replacement: 'stop timer' },
    { regex: /closefile并cleanup资源/g, replacement: 'close file and cleanup resources' },
    { regex: /free上下文memory/g, replacement: 'free context memory' },
    { regex: /获取event loop/g, replacement: 'get event loop' },
    { regex: /stop超time定time器（if仍at运line）/g, replacement: 'stop timeout timer (if still running)' },
    { regex: /checkis否sendcompleteor出错/g, replacement: 'check if send complete or error' },
    { regex: /checkis否canretry/g, replacement: 'check if can retry' },
    { regex: /sendfailure，closefile并cleanup/g, replacement: 'send failed, close file and cleanup' },
    { regex: /checkis否sendcomplete/g, replacement: 'check if send complete' },
    { regex: /performanceoptimization：disable TCP_CORK 以send缓冲data/g, replacement: 'performance optimization: disable TCP_CORK to send buffered data' },
    { regex: /continuesend剩余data/g, replacement: 'continue sending remaining data' },
    { regex: /重启超time定time器/g, replacement: 'restart timeout timer' },
    { regex: /setconfigurationvalue（0 table示usedefaultvalue）/g, replacement: 'set configuration value (0 means use default value)' },
    { regex: /内部function：带configuration sendfile/g, replacement: 'internal function: sendfile with configuration' },
    { regex: /strategy选择/g, replacement: 'strategy selection' },
    { regex: /smallfile：useoptimizationsystemcall（open \+ read \+ close），避免 stdio 开销/g, replacement: 'small file: use optimized system call (open + read + close), avoid stdio overhead' },
    { regex: /use open\(\) 代替 fopen\(\)，reducesystemcall开销/g, replacement: 'use open() instead of fopen(), reduce system call overhead' },
    { regex: /中等file：use分blockasync sendfile（与 largefile一致）/g, replacement: 'medium file: use chunked async sendfile (same as large file)' },
    { regex: /create sendfile 上下文/g, replacement: 'create sendfile context' },
    { regex: /initialization为disable/g, replacement: 'initialized as disabled' },
    { regex: /initializationconfigurationparameter/g, replacement: 'initialize configuration parameters' },
    { regex: /获取output file descriptor/g, replacement: 'get output file descriptor' },
    { regex: /sendresponseheader（use send_raw，不markresponse为已complete）/g, replacement: 'send response header (use send_raw, do not mark response as complete)' },
    { regex: /performanceoptimization：enable TCP_CORK 以optimizationlargefiletransfer/g, replacement: 'performance optimization: enable TCP_CORK to optimize large file transfer' },
    { regex: /initialization超time定time器/g, replacement: 'initialize timeout timer' },
    { regex: /start分block sendfile（每次sendconfigurationchunk size）/g, replacement: 'start chunked sendfile (send configured chunk size each time)' },
    { regex: /check sendfile is否syncfailureor无data/g, replacement: 'check if sendfile sync failed or no data' },
    { regex: /cleanup资源/g, replacement: 'cleanup resources' },
    { regex: /largefile：use sendfile 零拷贝optimization/g, replacement: 'large file: use sendfile zero-copy optimization' },
    { regex: /check sendfile is否syncfailure/g, replacement: 'check if sendfile sync failed' },
    { regex: /零拷贝send静态file（混合strategy）- usedefaultconfiguration/g, replacement: 'zero-copy send static file (mixed strategy) - use default configuration' },
    { regex: /call内部function，use NULL configuration（usedefaultvalue）/g, replacement: 'call internal function, use NULL configuration (use default value)' },

    // Request patterns
    { regex: /initializationHTTP解析器/g, replacement: 'initialize HTTP parser' },
    { regex: /initializationbodybuffer/g, replacement: 'initialize body buffer' },

    // Router cache patterns
    { regex: /- ifneedatmany线程环境中use，请为each线程create独立serverinstanceandrouter/g, replacement: '- if need to use in multi-threaded environment, please create independent server instance and router for each thread' },
    { regex: /先find in hot path/g, replacement: 'first find in hot path' },
    { regex: /if热path中没找到，find in hash table/g, replacement: 'if not found in hot path, find in hash table' },

    // Server patterns
    { regex: /写operationstate/g, replacement: 'write operation state' },
    { regex: /单线程event驱动connectionhandlecallback/g, replacement: 'single-threaded event-driven connection handling callback' },
    { regex: /这islibuveventloop核心callbackfunction，handleallnewconnection/g, replacement: 'this is the core callback function of libuv event loop, handles all new connections' },
    { regex: /server句柄/g, replacement: 'server handle' },
    { regex: /单线程connection数check - useserver特定configuration/g, replacement: 'single-threaded connection count check - use server specific configuration' },
    { regex: /回退到全局configuration（use server->context）/g, replacement: 'fallback to global configuration (use server->context)' },
    { regex: /create临timeconnection以send503response/g, replacement: 'create temporary connection to send 503 response' },
    { regex: /sendHTTP 503response - use静态常量避免重复allocate/g, replacement: 'send HTTP 503 response - use static constants to avoid repeated allocation' },
    { regex: /ifwritefailure，立即freewrite_req并closeconnection/g, replacement: 'if write failed, immediately free write_req and close connection' },
    { regex: /createnewconnectionobject - 单线程allocate，无需sync/g, replacement: 'create new connection object - single-threaded allocation, no synchronization needed' },
    { regex: /requestandresponseobject已atconnectioncreatetimeinitialization/g, replacement: 'request and response objects already initialized at connection creation time' },
    { regex: /单线程securityconnection计数递增/g, replacement: 'single-threaded safe connection count increment' },
    { regex: /startconnectionhandle（TLS握手orHTTPread）/g, replacement: 'start connection handling (TLS handshake or HTTP read)' },
    { regex: /all后续handle都throughlibuvcallbackateventloop中async进line/g, replacement: 'all subsequent handling is done asynchronously through libuv callbacks in event loop' },
    { regex: /create基于单线程event驱动HTTPserver/g, replacement: 'create single-threaded event-driven HTTP server' },
    { regex: /libuveventloop，if为NULL则createneweventloop/g, replacement: 'libuv event loop, if NULL create new event loop' },
    { regex: /serverobject，alloperation都at单个eventloop线程中进line/g, replacement: 'server object, all operations are done in single event loop thread' },
    { regex: /单线程设计优势：/g, replacement: 'single-threaded design advantages:' },
    { regex: /1\. 无需锁机制，避免死锁and竞态condition/g, replacement: '1. No lock mechanism needed, avoid deadlock and race conditions' },
    { regex: /2\. memoryaccess更security，无需原子operation/g, replacement: '2. Memory access is safer, no atomic operations needed' },
    { regex: /3\. performance可预测，避免线程切换开销/g, replacement: '3. Performance is predictable, avoid thread switching overhead' },
    { regex: /4\. debug简单，执line流清晰/g, replacement: '4. Debug is simple, execution flow is clear' },
    { regex: /initializationTLSmodule（if还withoutinitialization）/g, replacement: 'initialize TLS module (if not yet initialized)' },
    { regex: /use全局变量以保持向后兼容性/g, replacement: 'use global variable to maintain backward compatibility' },
    { regex: /new项目应use uvhttp_context 进line TLS configuration/g, replacement: 'new projects should use uvhttp_context for TLS configuration' },
    { regex: /initializationconnection限制defaultvalue/g, replacement: 'initialize connection limit default value' },
    { regex: /default最largeconnection数/g, replacement: 'default maximum connection count' },
    { regex: /default最largemessagelargesmall1MB/g, replacement: 'default maximum message size 1MB' },
    { regex: /initializationWebSocket路由table/g, replacement: 'initialize WebSocket routing table' },
    { regex: /initialization限流功canwordsegment/g, replacement: 'initialize rate limiting function segments' },
    { regex: /ifwithout提供loop，内部createnewloop/g, replacement: 'if loop not provided, internally create new loop' },
    { regex: /运lineloopmany次以handleclosecallback/g, replacement: 'run loop multiple times to handle close callbacks' },
    { regex: /fix：无论is否拥haveloop，都need运lineloophandleclosecallback/g, replacement: 'fix: whether owns loop or not, need to run loop to handle close callbacks' },
    { regex: /use UV_RUN_ONCE 而不is UV_RUN_NOWAIT，ensurecallback被执line/g, replacement: 'use UV_RUN_ONCE instead of UV_RUN_NOWAIT, ensure callbacks are executed' },
    { regex: /cleanupconnection池/g, replacement: 'cleanup connection pool' },
    { regex: /cleanup上下文/g, replacement: 'cleanup context' },
    { regex: /freeWebSocket路由table/g, replacement: 'free WebSocket routing table' },
    { regex: /cleanup限流白名单/g, replacement: 'cleanup rate limiting whitelist' },
    { regex: /cleanup白名单哈希table/g, replacement: 'cleanup whitelist hash table' },
    { regex: /限流state已嵌入到structurebody中，无需additionalcleanup/g, replacement: 'rate limiting state is embedded in structure body, no additional cleanup needed' },
    { regex: /if拥haveloop，needclose并free/g, replacement: 'if owns loop, need to close and free' },
    { regex: /Nginx optimization：绑定port/g, replacement: 'Nginx optimization: bind port' },
    { regex: /set TCP_CORK（延迟send以optimizationsmall包）- 仅used forsendlargefiletime/g, replacement: 'set TCP_CORK (delay send to optimize small packets) - only used when sending large files' },
    { regex: /use server->context 而非 loop->data，避免独占 loop->data/g, replacement: 'use server->context instead of loop->data, avoid monopolizing loop->data' },
    { regex: /========== 统一APIimplement ==========/g, replacement: '========== Unified API Implementation ==========' },
    { regex: /内部helper function/g, replacement: 'internal helper function' },
    { regex: /获取orcreateeventloop/g, replacement: 'get or create event loop' },
    { regex: /set为 NULL 避免双重free/g, replacement: 'set to NULL to avoid double free' },
    { regex: /atcall uvhttp_server_free before，将 config set为 NULL/g, replacement: 'before calling uvhttp_server_free, set config to NULL' },
    { regex: /create并setdefaultconfiguration/g, replacement: 'create and set default configuration' },
    { regex: /atcall uvhttp_server_free before，将 config and router set为 NULL/g, replacement: 'before calling uvhttp_server_free, set config and router to NULL' },
    { regex: /because它们willat uvhttp_server_free 中被free/g, replacement: 'because they will be freed in uvhttp_server_free' },
    { regex: /start监听/g, replacement: 'start listening' },
    { regex: /路由addhelper function/g, replacement: 'route add helper function' },
    { regex: /链式路由API/g, replacement: 'chained routing API' },
    { regex: /简化configurationAPI/g, replacement: 'simplified configuration API' },
    { regex: /便捷requestparameter获取/g, replacement: 'convenient request parameter retrieval' },
    { regex: /server运lineandcleanup/g, replacement: 'server run and cleanup' },
    { regex: /注意：routerandconfig由server负责free，不要重复free/g, replacement: 'note: router and config are freed by server, do not free them again' },
    { regex: /defaulthandle器（used for一键start）/g, replacement: 'default handler (used for one-click start)' },
    { regex: /UVHTTP 统一APIserver/g, replacement: 'UVHTTP unified API server' },
    { regex: /time间/g, replacement: 'time' },
    { regex: /欢迎use UVHTTP 统一API!/g, replacement: 'Welcome to use UVHTTP unified API!' },
    { regex: /一键startfunction（最简API）/g, replacement: 'one-click start function (simplest API)' },
    { regex: /error: port号mustat 1-65535 range内/g, replacement: 'error: port number must be in 1-65535 range' },
    { regex: /warning: host parameter为 NULL，usedefaultvalue 0.0.0.0/g, replacement: 'warning: host parameter is NULL, use default value 0.0.0.0' },
    { regex: /adddefault路由/g, replacement: 'add default routes' },
    { regex: /UVHTTP server运lineat/g, replacement: 'UVHTTP server running at' }
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