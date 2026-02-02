const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

const patterns = [
    { regex: /v2\.0\.0: 强制requirement上下文，不再support NULL/g, replacement: 'v2.0.0: mandatory context, no longer support NULL' },
    { regex: /if context 中 TLS 资源未initialize，先initialize/g, replacement: 'if TLS resources in context not initialized, initialize first' },
    { regex: /allocate并initialize entropy 上下文/g, replacement: 'allocate and initialize entropy context' },
    { regex: /allocate并initialize DRBG 上下文/g, replacement: 'allocate and initialize DRBG context' },
    { regex: /usecustom熵源initialize DRBG/g, replacement: 'use custom entropy source to initialize DRBG' },
    { regex: /free entropy and DRBG 上下文/g, replacement: 'free entropy and DRBG context' },
    { regex: /TLS上下文manage/g, replacement: 'TLS context management' },
    { regex: /暂timedisable/g, replacement: 'temporarily disabled' },
    { regex: /证书configuration/g, replacement: 'certificate configuration' },
    { regex: /mbedTLS 3\.x 中 DH parameterthrough ECDH configuration/g, replacement: 'in mbedTLS 3.x, DH parameters through ECDH configuration' },
    { regex: /defaultuse ECDHE-ECDSA and ECDHE-RSA 密码套件/g, replacement: 'default use ECDHE-ECDSA and ECDHE-RSA cipher suites' },
    { regex: /如需custom DH parameter，needconfiguration ECDH 组/g, replacement: 'if custom DH parameters needed, configure ECDH groups' },
    { regex: /when前versionusedefault ECDH 组（推荐）/g, replacement: 'current version uses default ECDH groups (recommended)' },
    { regex: /如需custom DH parameter，canadd以下configuration：/g, replacement: 'if custom DH parameters needed, can add following configuration:' },
    { regex: /证书validate/g, replacement: 'certificate validation' },
    { regex: /use mbedtls 提供host名validatefunction/g, replacement: 'use mbedtls provided hostname validation function' },
    { regex: /简单精确匹配（生产environment应support通配符）/g, replacement: 'simple exact match (production environment should support wildcards)' },
    { regex: /check证书is否尚未生效/g, replacement: 'check if certificate is not yet valid' },
    { regex: /check证书is否已过期/g, replacement: 'check if certificate is expired' },
    { regex: /证书吊销check/g, replacement: 'certificate revocation check' },
    { regex: /mbedTLS 3\.x version中enable CRL check/g, replacement: 'enable CRL check in mbedTLS 3.x version' },
    { regex: /将 CRL add到validateconfiguration/g, replacement: 'add CRL to validation configuration' },
    { regex: /OCSP装订/g, replacement: 'OCSP stapling' },
    { regex: /mbedTLS 3\.x 中 OCSP response获取needadditionalconfiguration/g, replacement: 'OCSP response retrieval needs additional configuration in mbedTLS 3.x' },
    { regex: /when前versionreturn未implement，建议use CRL check作为替代/g, replacement: 'current version returns not implemented, suggest using CRL check as alternative' },
    { regex: /mbedTLS 3\.x 中validate OCSP response/g, replacement: 'validate OCSP response in mbedTLS 3.x' },
    { regex: /注意：这needadditional OCSP staterequestconfiguration/g, replacement: 'note: this needs additional OCSP state request configuration' },
    { regex: /mbedTLS 3\.x 中早期data（0-RTT）support/g, replacement: 'early data (0-RTT) support in mbedTLS 3.x' },
    { regex: /注意：早期data可can带来重放attack风险/g, replacement: 'note: early data can bring replay attack risk' },
    { regex: /when前versiondisable早期data以ensuresecurity性/g, replacement: 'current version disables early data to ensure security' },
    { regex: /will话票证optimization/g, replacement: 'session ticket optimization' },
    { regex: /mbedTLS 3\.x 中will话restorethroughwill话cacheimplement/g, replacement: 'session restore through session cache in mbedTLS 3.x' },
    { regex: /will话票证密钥由内部manage，无需手动set/g, replacement: 'session ticket key is managed internally, no need to manually set' },
    { regex: /use mbedtls_ssl_cache_context 进linewill话cache/g, replacement: 'use mbedtls_ssl_cache_context for session cache' },
    { regex: /mbedTLS 3\.x 中will话票证密钥轮换由内部manage/g, replacement: 'session ticket key rotation is managed internally in mbedTLS 3.x' },
    { regex: /will话cachewill自动handle密钥轮换/g, replacement: 'session cache will automatically handle key rotation' },
    { regex: /setwill话cache超timetime间/g, replacement: 'set session cache timeout' },
    { regex: /证书链validate/g, replacement: 'certificate chain validation' },
    { regex: /解析additional证书file/g, replacement: 'parse additional certificate file' },
    { regex: /将证书add到证书链/g, replacement: 'add certificate to certificate chain' },
    { regex: /从 SSL 上下文中获取对等证书链/g, replacement: 'get peer certificate chain from SSL context' },
    { regex: /initializationHTTP解析器/g, replacement: 'initialize HTTP parser' },
    { regex: /enable lenient keep-alive mode以正确handle Connection: close 后data/g, replacement: 'enable lenient keep-alive mode to correctly handle data after Connection: close' },
    { regex: /initializationbodybuffer/g, replacement: 'initialize body buffer' },
    { regex: /HTTP解析器callbackfunctionimplement/g, replacement: 'HTTP parser callback function implementation' },
    { regex: /reset解析state/g, replacement: 'reset parsing state' },
    { regex: /ensureURLlength不超过限制/g, replacement: 'ensure URL length does not exceed limit' },
    { regex: /checkis否超出goalbufferlargesmall，ensuresecurity性/g, replacement: 'check if exceeds target buffer size, ensure security' },
    { regex: /performanceoptimization：只setlengthmark，避免清零整个buffer（256byte）/g, replacement: 'performance optimization: only set length mark, avoid zeroing entire buffer (256 bytes)' },
    { regex: /checkheaderwordsegment名length限制/g, replacement: 'check header field name length limit' },
    { regex: /wordsegment名太长/g, replacement: 'field name too long' },
    { regex: /copyheaderwordsegment名/g, replacement: 'copy header field name' },
    { regex: /checkheadervaluelength限制/g, replacement: 'check header value length limit' },
    { regex: /value太长/g, replacement: 'value too long' },
    { regex: /checkwhen前headerwordsegment名is否存at/g, replacement: 'check if current header field name exists' },
    { regex: /without对应headerwordsegment名/g, replacement: 'without corresponding header field name' },
    { regex: /构造 header nameandvalue/g, replacement: 'construct header name and value' },
    { regex: /calculatenewcapacity（至fewexpand到before两倍or满足所需largesmall）/g, replacement: 'calculate new capacity (at least expand to twice before or meet required size)' },
    { regex: /body太large/g, replacement: 'body too large' },
    { regex: /重newallocatememory/g, replacement: 'reallocate memory' },
    { regex: /checkclientis否at白名单中/g, replacement: 'check if client is in whitelist' },
    { regex: /check并执line限流/g, replacement: 'check and execute rate limiting' },
    { regex: /handle WebSocket 握手request/g, replacement: 'handle WebSocket handshake request' },
    { regex: /ensure URL have效，if为空则set为 "\/" /g, replacement: 'ensure URL is valid, if empty set to "/"' },
    { regex: /单线程event驱动HTTPrequestcompletehandle/g, replacement: 'single-threaded event-driven HTTP request complete handle' },
    { regex: /atlibuveventloop线程中执line，handle完整HTTPrequest/g, replacement: 'execute in libuv event loop thread, handle complete HTTP request' },
    { regex: /单线程优势：无竞态condition，requesthandlesequential可预测/g, replacement: 'single-threaded advantage: no race condition, request handling is sequential and predictable' },
    { regex: /prevent重复handle/g, replacement: 'prevent duplicate handling' },
    { regex: /限流check/g, replacement: 'rate limiting check' },
    { regex: /WebSocket 握手/g, replacement: 'WebSocket handshake' },
    { regex: /路由handle/g, replacement: 'route handling' },
    { regex: /ifwithout找到 handler 但have静态file上下文，attempt静态filehandle/g, replacement: 'if handler not found but have static file context, attempt static file handling' },
    { regex: /checkis否为WebSocket握手request/g, replacement: 'check if is WebSocket handshake request' },
    { regex: /check必需header部/g, replacement: 'check required headers' },
    { regex: /checkUpgradeheader部（不区分largesmall写）/g, replacement: 'check Upgrade header (case insensitive)' },
    { regex: /checkConnectionheader部（可cancontainsmany个value）/g, replacement: 'check Connection header (can contain multiple values)' },
    { regex: /将 uvhttp_method_t map到 llhttp_method_t/g, replacement: 'map uvhttp_method_t to llhttp_method_t' },
    { regex: /check header namewhether contains非法character/g, replacement: 'check if header name contains illegal characters' },
    { regex: /HTTP header name只cancontains特定character/g, replacement: 'HTTP header name can only contain specific characters' },
    { regex: /find header（不区分largesmall写）/g, replacement: 'find header (case insensitive)' },
    { regex: /returnpathpart（不contains查询parameter）/g, replacement: 'return path part (without query parameters)' },
    { regex: /ensurepathlength不超过bufferlargesmall/g, replacement: 'ensure path length does not exceed buffer size' },
    { regex: /path太长，return根path/g, replacement: 'path too long, return root path' },
    { regex: /copypathpart（不contains查询parameter）/g, replacement: 'copy path part (without query parameters)' },
    { regex: /validatepathsecurity性/g, replacement: 'validate path security' },
    { regex: /validate查询stringsecurity性/g, replacement: 'validate query string security' },
    { regex: /简单查询parameter解析/g, replacement: 'simple query parameter parsing' },
    { regex: /attempt从X-Forwarded-Forheader部获取（proxy\/load balance器）/g, replacement: 'attempt to get from X-Forwarded-For header (proxy/load balancer)' },
    { regex: /X-Forwarded-For可cancontainsmany个IP，取第一个/g, replacement: 'X-Forwarded-For can contain multiple IPs, take the first one' },
    { regex: /attempt从X-Real-IPheader部获取/g, replacement: 'attempt to get from X-Real-IP header' },
    { regex: /从TCPconnection获取真实IP（needaccess底层socket）/g, replacement: 'get real IP from TCP connection (need access to underlying socket)' },
    { regex: /add header（内部use，自动expand）/g, replacement: 'add header (internal use, auto expand)' },
    { regex: /calculatenewcapacity（最many MAX_HEADERS）/g, replacement: 'calculate new capacity (max MAX_HEADERS)' },
    { regex: /add计数/g, replacement: 'add count' },
    { regex: /获取errorfix建议/g, replacement: 'get error fix suggestion' },
    { regex: /写requestobject/g, replacement: 'write request object' }
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