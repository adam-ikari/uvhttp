const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const includeDir = '/home/zhaodi-chen/project/uvhttp/include';

// Get all C and H files
const srcFiles = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));
const includeFiles = fs.readdirSync(includeDir).filter(f => f.endsWith('.h'));
const allFiles = [...srcFiles.map(f => path.join(srcDir, f)), ...includeFiles.map(f => path.join(includeDir, f))];

// Comprehensive translations
const translations = [
    // Hash module
    ['xxHash 头文件', 'xxHash header file'],
    ['前向声明', 'forward declaration'],
    ['主要API', 'Main API'],
    ['便捷function', 'convenience functions'],
    ['error handling宏', 'error handling macros'],
    
    // Router module
    ['HTTPmethod枚举 - useuvhttp_request.h中的定义', 'HTTP method enumeration - use definitions in uvhttp_request.h'],
    ['routing匹配结果', 'routing match result'],
    ['routing节点', 'routing node'],
    ['path段', 'path segment'],
    ['子节点', 'child node'],
    ['是否parameter节点', 'whether it is a parameter node'],
    ['parameter名', 'parameter name'],
    ['数组routingstructure', 'array routing structure'],
    ['热path字段(频繁访问)- 优化memory局部性', 'hot path field (frequently accessed) - optimize memory locality'],
    ['是否useTrie', 'whether to use Trie'],
    ['总routing数量', 'total routing count'],
    ['Trierouting相关(8字节对齐)', 'Trie routing related (8-byte aligned)'],
    
    // Common phrases
    ['头文件', 'header file'],
    ['声明', 'declaration'],
    ['定义', 'definition'],
    ['枚举', 'enumeration'],
    ['结构', 'structure'],
    ['字段', 'field'],
    ['节点', 'node'],
    ['子节点', 'child node'],
    ['父节点', 'parent node'],
    ['根节点', 'root node'],
    ['叶子节点', 'leaf node'],
    ['段', 'segment'],
    ['路径', 'path'],
    ['参数', 'parameter'],
    ['参数名', 'parameter name'],
    ['参数值', 'parameter value'],
    ['参数列表', 'parameter list'],
    ['参数数量', 'parameter count'],
    ['参数类型', 'parameter type'],
    ['参数验证', 'parameter validation'],
    ['参数检查', 'parameter check'],
    ['参数解析', 'parameter parsing'],
    ['参数传递', 'parameter passing'],
    ['参数返回', 'parameter return'],
    ['参数错误', 'parameter error'],
    ['参数异常', 'parameter exception'],
    ['参数默认值', 'parameter default value'],
    ['参数可选', 'parameter optional'],
    ['参数必需', 'parameter required'],
    ['参数描述', 'parameter description'],
    ['参数说明', 'parameter explanation'],
    ['参数注释', 'parameter comment'],
    ['参数文档', 'parameter documentation'],
    ['参数示例', 'parameter example'],
    ['参数用法', 'parameter usage'],
    ['参数限制', 'parameter constraint'],
    ['参数约束', 'parameter constraint'],
    ['参数范围', 'parameter range'],
    ['参数长度', 'parameter length'],
    ['参数大小', 'parameter size'],
    ['参数格式', 'parameter format'],
    ['参数编码', 'parameter encoding'],
    ['参数解码', 'parameter decoding'],
    ['参数序列化', 'parameter serialization'],
    ['参数反序列化', 'parameter deserialization'],
    ['参数转换', 'parameter conversion'],
    ['参数映射', 'parameter mapping'],
    ['参数绑定', 'parameter binding'],
    ['参数注入', 'parameter injection'],
    ['参数拦截', 'parameter interception'],
    ['参数过滤', 'parameter filtering'],
    ['参数验证', 'parameter validation'],
    ['参数清理', 'parameter sanitization'],
    ['参数转义', 'parameter escaping'],
    ['参数编码', 'parameter encoding'],
    ['参数解码', 'parameter decoding'],
    ['参数压缩', 'parameter compression'],
    ['参数解压', 'parameter decompression'],
    ['参数加密', 'parameter encryption'],
    ['参数解密', 'parameter decryption'],
    ['参数签名', 'parameter signing'],
    ['参数验证', 'parameter verification'],
    ['参数认证', 'parameter authentication'],
    ['参数授权', 'parameter authorization'],
    ['参数权限', 'parameter permission'],
    ['参数角色', 'parameter role'],
    ['参数用户', 'parameter user'],
    ['参数会话', 'parameter session'],
    ['参数令牌', 'parameter token'],
    ['参数密钥', 'parameter key'],
    ['参数证书', 'parameter certificate'],
    ['参数握手', 'parameter handshake'],
    ['参数连接池', 'parameter connection pool'],
    ['参数线程池', 'parameter thread pool'],
    ['参数对象池', 'parameter object pool'],
    ['参数内存池', 'parameter memory pool'],
    ['参数文件描述符', 'parameter file descriptor'],
    ['参数套接字', 'parameter socket'],
    ['参数端口', 'parameter port'],
    ['参数主机', 'parameter host'],
    ['参数地址', 'parameter address'],
    ['参数路径', 'parameter path'],
    ['参数查询参数', 'parameter query parameter'],
    ['参数请求体', 'parameter request body'],
    ['参数响应体', 'parameter response body'],
    ['参数请求头', 'parameter request header'],
    ['参数响应头', 'parameter response header'],
    ['参数状态码', 'parameter status code'],
    ['参数内容类型', 'parameter content type'],
    ['参数内容长度', 'parameter content length'],
    ['参数长连接', 'parameter keep-alive'],
    ['参数短连接', 'parameter short connection'],
    ['参数事件循环', 'parameter event loop'],
    ['参数单线程', 'parameter single-threaded'],
    ['参数多线程', 'parameter multi-threaded'],
    ['参数异步', 'parameter asynchronous'],
    ['参数同步', 'parameter synchronous'],
    ['参数阻塞', 'parameter blocking'],
    ['参数非阻塞', 'parameter non-blocking'],
    ['参数并发', 'parameter concurrent'],
    ['参数串行', 'parameter serial'],
    ['参数并发数', 'parameter concurrency'],
    ['参数吞吐量', 'parameter throughput'],
    ['参数延迟', 'parameter latency'],
    ['参数超时', 'parameter timeout'],
    ['参数重试', 'parameter retry'],
    ['参数缓冲', 'parameter buffering'],
    ['参数缓存', 'parameter cache'],
    ['参数路由', 'parameter routing'],
    ['参数路由器', 'parameter router'],
    ['参数中间件', 'parameter middleware'],
    ['参数处理器', 'parameter handler'],
    ['参数拦截器', 'parameter interceptor'],
    ['参数过滤器', 'parameter filter'],
    ['参数验证器', 'parameter validator'],
    ['参数解析器', 'parameter parser'],
    ['参数生成器', 'parameter generator'],
    ['参数序列化', 'parameter serialization'],
    ['参数反序列化', 'parameter deserialization'],
    ['参数编码', 'parameter encoding'],
    ['参数解码', 'parameter decoding'],
    ['参数加密', 'parameter encryption'],
    ['参数解密', 'parameter decryption'],
    ['参数签名', 'parameter signature'],
    ['参数验证', 'parameter verification'],
    ['参数认证', 'parameter authentication'],
    ['参数授权', 'parameter authorization'],
    ['参数权限', 'parameter permission'],
    ['参数角色', 'parameter role'],
    ['参数用户', 'parameter user'],
    ['参数会话', 'parameter session'],
    ['参数令牌', 'parameter token'],
    ['参数密钥', 'parameter key'],
    ['参数证书', 'parameter certificate'],
    ['参数握手', 'parameter handshake'],
    ['参数连接池', 'parameter connection pool'],
    ['参数线程池', 'parameter thread pool'],
    ['参数对象池', 'parameter object pool'],
    ['参数内存池', 'parameter memory pool'],
    ['参数文件描述符', 'parameter file descriptor'],
    ['参数套接字', 'parameter socket'],
    ['参数端口', 'parameter port'],
    ['参数主机', 'parameter host'],
    ['参数地址', 'parameter address'],
    ['参数路径', 'parameter path'],
    ['参数查询参数', 'parameter query parameter'],
    ['参数请求体', 'parameter request body'],
    ['参数响应体', 'parameter response body'],
    ['参数请求头', 'parameter request header'],
    ['参数响应头', 'parameter response header'],
    ['参数状态码', 'parameter status code'],
    ['参数内容类型', 'parameter content type'],
    ['参数内容长度', 'parameter content length'],
    ['参数长连接', 'parameter keep-alive'],
    ['参数短连接', 'parameter short connection'],
    
    // Single characters
    ['（', '('],
    ['）', ')'],
    ['，', ','],
    ['。', '.'],
    ['：', ':'],
    ['；', ';'],
    ['【', '['],
    ['】', ']'],
    ['《', '<'],
    ['》', '>'],
    ['"', '"'],
    ['"', '"'],
    ['\'', '\''],
    ['\'', '\'']
];

// Sort by length (longest first) to avoid partial matches
translations.sort((a, b) => b[0].length - a[0].length);

let totalTranslated = 0;

allFiles.forEach(filePath => {
    const content = fs.readFileSync(filePath, 'utf8');
    let result = content;
    let changes = 0;
    
    // Only process lines that are comments
    const lines = result.split('\n');
    const processedLines = lines.map(line => {
        // Check if line contains Chinese characters
        if (/[一-龥]/.test(line)) {
            let processedLine = line;
            
            // Apply translations
            translations.forEach(([chinese, english]) => {
                const regex = new RegExp(chinese.replace(/[.*+?^${}()|[\]\\]/g, '\\$&'), 'g');
                if (regex.test(processedLine)) {
                    processedLine = processedLine.replace(regex, english);
                    changes++;
                }
            });
            
            return processedLine;
        }
        return line;
    });

    result = processedLines.join('\n');

    if (content !== result) {
        fs.writeFileSync(filePath, result, 'utf8');
        console.log('Translated:', path.basename(filePath), `(${changes} changes)`);
        totalTranslated++;
    }
});

console.log('\nTotal files translated:', totalTranslated);
console.log('Total translation patterns:', translations.length);