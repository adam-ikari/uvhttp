const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

const patterns = [
    // Static file patterns
    { regex: /initialization sendfile contextconfigurationparameter/g, replacement: 'initialize sendfile context configuration parameters' },
    { regex: /stoptimeout timer（if仍at运line）/g, replacement: 'stop timeout timer (if still running)' },
    { regex: /重启timeout timer/g, replacement: 'restart timeout timer' },
    { regex: /initialization为disable/g, replacement: 'initialized as disabled' },
    { regex: /initializationconfigurationparameter/g, replacement: 'initialize configuration parameters' },
    { regex: /initializationtimeout timer/g, replacement: 'initialize timeout timer' },

    // Server patterns
    { regex: /requestandresponseobject已atconnectioncreatetimeinitialization/g, replacement: 'request and response objects already initialized at connection creation time' },
    { regex: /initializationTLSmodule（if还withoutinitialization）/g, replacement: 'initialize TLS module (if not yet initialized)' },
    { regex: /initializationconnection限制defaultvalue/g, replacement: 'initialize connection limit default value' },
    { regex: /initializationWebSocket路由table/g, replacement: 'initialize WebSocket routing table' },
    { regex: /initialization限流功canwordsegment/g, replacement: 'initialize rate limiting function segments' },
    { regex: /按 Ctrl\+C stopserver/g, replacement: 'Press Ctrl+C to stop server' },
    { regex: /只atsuccesscreateserver后才free/g, replacement: 'only free after successfully creating server' },
    { regex: /WebSocket握手validate（单线程security）/g, replacement: 'WebSocket handshake validation (single-threaded safe)' },
    { regex: /注册WebSockethandle器（add到server路由table中）/g, replacement: 'register WebSocket handler (add to server routing table)' },
    { regex: /createnew路由条目/g, replacement: 'create new route entry' },
    { regex: /allocate并copypath（use uvhttp_alloc 避免混用allocate器）/g, replacement: 'allocate and copy path (use uvhttp_alloc to avoid mixing allocators)' },
    { regex: /add到serverWebSocket路由table（单线程security）/g, replacement: 'add to server WebSocket routing table (single-threaded safe)' },
    { regex: /findWebSockethandle器（according topath）/g, replacement: 'find WebSocket handler (according to path)' },
    { regex: /traverseWebSocket路由table/g, replacement: 'traverse WebSocket routing table' },
    { regex: /找到匹配path，returnhandle器pointer/g, replacement: 'found matching path, return handler pointer' },
    { regex: /未找到匹配handle器/g, replacement: 'no matching handler found' },
    { regex: /获取 context/g, replacement: 'get context' },
    { regex: /call原生WebSocket APIsend text message/g, replacement: 'call native WebSocket API to send text message' },
    { regex: /call原生WebSocket APIcloseconnection/g, replacement: 'call native WebSocket API to close connection' },
    { regex: /========== 限流功canimplement（核心功can） ==========/g, replacement: '========== Rate Limiting Implementation (Core Feature) ==========' },
    { regex: /========== 限流功canimplement ==========/g, replacement: '========== Rate Limiting Implementation ==========' },
    { regex: /限流parameter限制/g, replacement: 'rate limiting parameter limits' },
    { regex: /最largerequest数：100万/g, replacement: 'maximum request count: 1 million' },
    { regex: /最largetime窗口：24smalltime/g, replacement: 'maximum time window: 24 hours' },
    { regex: /enable限流功can/g, replacement: 'enable rate limiting function' },
    { regex: /initialization限流state/g, replacement: 'initialize rate limiting state' },
    { regex: /disable限流功can/g, replacement: 'disable rate limiting function' },
    { regex: /check限流state/g, replacement: 'check rate limiting state' },
    { regex: /限流未enable，允许request/g, replacement: 'rate limiting not enabled, allow request' },
    { regex: /获取when前time（毫秒）/g, replacement: 'get current time (milliseconds)' },
    { regex: /checktime窗口is否过期/g, replacement: 'check if time window has expired' },
    { regex: /reset计数器/g, replacement: 'reset counter' },
    { regex: /checkis否超过限制/g, replacement: 'check if exceeds limit' },
    { regex: /add限流白名单IPaddress/g, replacement: 'add rate limiting whitelist IP address' },
    { regex: /validateIPaddress格式/g, replacement: 'validate IP address format' },
    { regex: /无效 IP address/g, replacement: 'invalid IP address' },
    { regex: /checkis否已经存at于哈希table中（避免重复add）/g, replacement: 'check if already exists in hash table (avoid duplicate addition)' },
    { regex: /已经存at，无需重复add/g, replacement: 'already exists, no need to add again' },
    { regex: /重newallocate白名单array/g, replacement: 'reallocate whitelist array' },
    { regex: /回退：restore原来arraylargesmall/g, replacement: 'fallback: restore original array size' },
    { regex: /回退：cleanup已allocateIPstring/g, replacement: 'fallback: cleanup already allocated IP string' },
    { regex: /获取client限流state/g, replacement: 'get client rate limiting state' },
    { regex: /限流未enable/g, replacement: 'rate limiting not enabled' },
    { regex: /清空all限流state/g, replacement: 'clear all rate limiting state' },
    { regex: /resetclient限流state/g, replacement: 'reset client rate limiting state' },
    { regex: /简化implement：reset整个server限流计数器/g, replacement: 'simplified implementation: reset entire server rate limiting counter' },
    { regex: /空 TLS function定义，used fordisable TLS time链接/g, replacement: 'empty TLS function definition, used for linking when TLS is disabled' },
    { regex: /超time检测定time器callback/g, replacement: 'timeout detection timer callback' },
    { regex: /checkallconnection活动time，close超timeconnection/g, replacement: 'check all connection activity time, close timeout connections' },
    { regex: /convert为毫秒/g, replacement: 'convert to milliseconds' },
    { regex: /checkconnectionis否超time/g, replacement: 'check if connection has timed out' },
    { regex: /close超timeconnection/g, replacement: 'close timeout connection' },
    { regex: /从链table中remove/g, replacement: 'remove from linked list' },
    { regex: /free节点/g, replacement: 'free node' },
    { regex: /心跳检测定time器callback/g, replacement: 'heartbeat detection timer callback' },
    { regex: /定期send Ping 帧以检测connection活跃state/g, replacement: 'periodically send Ping frames to detect connection active state' },
    { regex: /checkis否needsend Ping/g, replacement: 'check if need to send Ping' },
    { regex: /send Ping 帧/g, replacement: 'send Ping frame' },
    { regex: /check Ping is否超time（未收到 Pong response）/g, replacement: 'check if Ping timed out (did not receive Pong response)' },
    { regex: /close无responseconnection/g, replacement: 'close connection with no response' },
    { regex: /超timetime（秒），range：10-3600/g, replacement: 'timeout (seconds), range: 10-3600' },
    { regex: /心跳间隔（秒），range：5-300/g, replacement: 'heartbeat interval (seconds), range: 5-300' },
    { regex: /UVHTTP_OK success，其他valuetable示failure/g, replacement: 'UVHTTP_OK success, other values indicate failure' },
    { regex: /if已经enable，先disable/g, replacement: 'if already enabled, disable first' },
    { regex: /createconnectionmanage器/g, replacement: 'create connection manager' },
    { regex: /default10秒 Ping 超time/g, replacement: 'default 10 seconds Ping timeout' },
    { regex: /initialization超time检测定time器/g, replacement: 'initialize timeout detection timer' },
    { regex: /initialization心跳检测定time器/g, replacement: 'initialize heartbeat detection timer' },
    { regex: /未enable，直接returnsuccess/g, replacement: 'not enabled, directly return success' },
    { regex: /freemanage器/g, replacement: 'free manager' },
    { regex: /获取 WebSocket connection总数/g, replacement: 'get total WebSocket connection count' },
    { regex: /获取指定path WebSocket connectioncount/g, replacement: 'get WebSocket connection count for specified path' },
    { regex: /向指定pathallconnection广播message/g, replacement: 'broadcast message to all connections on specified path' },
    { regex: /path（NULL table示广播到allconnection）/g, replacement: 'path (NULL means broadcast to all connections)' },
    { regex: /checkpathis否匹配（if指定了path）/g, replacement: 'check if path matches (if path specified)' },
    { regex: /close指定pathallconnection/g, replacement: 'close all connections on specified path' },
    { regex: /path（NULL table示closeallconnection）/g, replacement: 'path (NULL means close all connections)' },
    { regex: /从链table中remove/g, replacement: 'remove from linked list' },
    { regex: /内部function：add WebSocket connection到manage器/g, replacement: 'internal function: add WebSocket connection to manager' },
    { regex: /createconnection节点/g, replacement: 'create connection node' },
    { regex: /convert为毫秒/g, replacement: 'convert to milliseconds' },
    { regex: /add到链tableheader部/g, replacement: 'add to linked list header' },
    { regex: /内部function：从manage器中remove WebSocket connection/g, replacement: 'internal function: remove WebSocket connection from manager' },
    { regex: /从链table中remove/g, replacement: 'remove from linked list' }
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