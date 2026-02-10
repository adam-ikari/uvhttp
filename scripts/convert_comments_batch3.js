const fs = require('fs');
const path = require('path');

const srcDir = '/home/zhaodi-chen/project/uvhttp/src';
const files = fs.readdirSync(srcDir).filter(f => f.endsWith('.c'));

const patterns = [
    // Static file patterns
    { regex: /音频type/g, replacement: 'audio type' },
    { regex: /视频type/g, replacement: 'video type' },
    { regex: /获取fileextend名/g, replacement: 'get file extension' },
    { regex: /前向声明/g, replacement: 'forward declaration' },
    { regex: /HTML转义function - preventXSSattack/g, replacement: 'HTML escape function - prevent XSS attack' },
    { regex: /according tofileextend名获取MIMEtype/g, replacement: 'get MIME type according to file extension' },
    { regex: /获取filelargesmall/g, replacement: 'get file size' },
    { regex: /allocatememory并readfile/g, replacement: 'allocate memory and read file' },
    { regex: /获取fileinformation/g, replacement: 'get file information' },
    { regex: /不is常规file/g, replacement: 'not a regular file' },
    { regex: /calculatedirectorycolumntable所需bufferlargesmall/g, replacement: 'calculate buffer size needed for directory column table' },
    { regex: /directory条目structure/g, replacement: 'directory entry structure' },
    { regex: /收集directory条目information/g, replacement: 'collect directory entry information' },
    { regex: /先calculate条目count/g, replacement: 'first calculate entry count' },
    { regex: /收集条目information/g, replacement: 'collect entry information' },
    { regex: /usesecuritystring拷贝，file名太长time自动截断/g, replacement: 'use secure string copy, auto truncate when filename too long' },
    { regex: /获取fileinformation/g, replacement: 'get file information' },
    { regex: /sortdirectory条目/g, replacement: 'sort directory entries' },
    { regex: /directory优先/g, replacement: 'directory first' },
    { regex: /同type按namesort/g, replacement: 'same type sort by name' },
    { regex: /生成directorycolumntableHTML/g, replacement: 'generate directory column table HTML' },
    { regex: /start生成HTML/g, replacement: 'start generating HTML' },
    { regex: /add父directory链接/g, replacement: 'add parent directory link' },
    { regex: /收集directory条目/g, replacement: 'collect directory entries' },
    { regex: /sort条目/g, replacement: 'sort entries' },
    { regex: /生成HTMLtable格line/g, replacement: 'generate HTML table row' },
    { regex: /格式化modifytime间/g, replacement: 'format modification time' },
    { regex: /HTML转义file名/g, replacement: 'HTML escape filename' },
    { regex: /生成table格line/g, replacement: 'generate table row' },
    { regex: /生成ETagvalue/g, replacement: 'generate ETag value' },
    { regex: /简单ETag生成：filelargesmall-modifytime间/g, replacement: 'simple ETag generation: file size-modification time' },
    { regex: /set静态file相关responseheader/g, replacement: 'set static file related response headers' },
    { regex: /needreturn完整content/g, replacement: 'need return complete content' },
    { regex: /create静态file服务上下文/g, replacement: 'create static file service context' },
    { regex: /最largememoryuse量/g, replacement: 'maximum memory usage' },
    { regex: /最large条目数/g, replacement: 'maximum number of entries' },
    { regex: /free静态file服务上下文/g, replacement: 'free static file service context' },
    { regex: /checkfilepathis否security（preventpathtraverseattack）/g, replacement: 'check if file path is safe (prevent path traversal attack)' },
    { regex: /构建完整path/g, replacement: 'build complete path' },
    { regex: /ensurepath不will溢出buffer/g, replacement: 'ensure path will not overflow buffer' },
    { regex: /usesecuritystringoperationcopy根directory/g, replacement: 'use secure string operation to copy root directory' },
    { regex: /addpath分隔符（ifneed）/g, replacement: 'add path separator (if needed)' },
    { regex: /use realpath 进linepath规范化，preventpathtraverseattack/g, replacement: 'use realpath for path normalization, prevent path traversal attack' },
    { regex: /path不存ator无效/g, replacement: 'path does not exist or is invalid' },
    { regex: /将根directory也convert为绝对path，以便compare/g, replacement: 'also convert root directory to absolute path for comparison' },
    { regex: /根directory不存ator无效/g, replacement: 'root directory does not exist or is invalid' },
    { regex: /ensure规范化后pathat根directory内/g, replacement: 'ensure normalized path is within root directory' },
    { regex: /path不at根directory内/g, replacement: 'path is not within root directory' },
    { regex: /ensurepathat根directory下（不is根directory本身or父directory）/g, replacement: 'ensure path is under root directory (not root directory itself or parent directory)' },
    { regex: /将规范化后pathcopy回outputbuffer/g, replacement: 'copy normalized path back to output buffer' },
    { regex: /handle静态filerequest主要function/g, replacement: 'main function for handling static file requests' },
    { regex: /解析URLpath，remove查询parameter/g, replacement: 'parse URL path, remove query parameters' },
    { regex: /handle根path/g, replacement: 'handle root path' },
    { regex: /index_file 太长，usedefaultvalue/g, replacement: 'index_file too long, use default value' },
    { regex: /构建securityfilepath/g, replacement: 'build secure file path' },
    { regex: /从cachesendresponse/g, replacement: 'send response from cache' },
    { regex: /cache未命中，readfile/g, replacement: 'cache miss, read file' },
    { regex: /获取fileinformation/g, replacement: 'get file information' },
    { regex: /checkis否为directory/g, replacement: 'check if is directory' },
    { regex: /生成directorycolumntable/g, replacement: 'generate directory column table' },
    { regex: /usesecuritystring拷贝，path太长time自动截断/g, replacement: 'use secure string copy, auto truncate when path too long' },
    { regex: /checkfilelargesmall限制/g, replacement: 'check file size limit' },
    { regex: /对于中large型file（> 64KB），usesendfile零拷贝optimization - performanceoptimization/g, replacement: 'for medium-large files (> 64KB), use sendfile zero-copy optimization - performance optimization' },
    { regex: /获取MIMEtype/g, replacement: 'get MIME type' },
    { regex: /生成ETag/g, replacement: 'generate ETag' },
    { regex: /usesendfilesend（传递configuration）/g, replacement: 'use sendfile to send (pass configuration)' },
    { regex: /回退到传统way/g, replacement: 'fallback to traditional method' },
    { regex: /注意：sendfile 可can已经set了responseheader，needreset/g, replacement: 'note: sendfile may have already set response headers, need reset' },
    { regex: /传统waywill重newsetresponseheader/g, replacement: 'traditional method will reset response headers' },
    { regex: /readfilecontent（smallfileorsendfilefailuretime回退）/g, replacement: 'read file content (small file or fallback when sendfile fails)' },
    { regex: /获取MIMEtype/g, replacement: 'get MIME type' },
    { regex: /生成ETag/g, replacement: 'generate ETag' },
    { regex: /add到cache/g, replacement: 'add to cache' },
    { regex: /cacheaddfailure，但仍要returncontent/g, replacement: 'cache add failed, but still need to return content' },
    { regex: /注意：filecontentmemory现at由cachemanage，不要atherefree/g, replacement: 'note: file content memory is now managed by cache, do not free here' },
    { regex: /获取cachestatisticsinformation/g, replacement: 'get cache statistics information' },
    { regex: /获取cache命中率/g, replacement: 'get cache hit rate' },
    { regex: /cleanup过期cache条目/g, replacement: 'cleanup expired cache entries' },
    { regex: /========== 静态file中间件implement ==========/g, replacement: '========== Static file middleware implementation ==========' },
    { regex: /cache预热：预load指定file到cache中/g, replacement: 'cache prewarming: preload specified file into cache' },
    { regex: /构建完整filepath/g, replacement: 'build complete file path' },
    { regex: /checkfileis否存at/g, replacement: 'check if file exists' },
    { regex: /获取fileinformation/g, replacement: 'get file information' },
    { regex: /checkfilelargesmall限制/g, replacement: 'check file size limit' },
    { regex: /获取MIMEtype/g, replacement: 'get MIME type' },
    { regex: /生成ETag/g, replacement: 'generate ETag' },
    { regex: /add到cache/g, replacement: 'add to cache' },
    { regex: /cache预热：预loaddirectory中allfile/g, replacement: 'cache prewarming: preload all files in directory' },
    { regex: /构建完整directorypath/g, replacement: 'build complete directory path' },
    { regex: /checkdirectoryis否存at/g, replacement: 'check if directory exists' },
    { regex: /checkfilecount限制/g, replacement: 'check file count limit' },
    { regex: /构建相对path/g, replacement: 'build relative path' },
    { regex: /预热file/g, replacement: 'prewarm file' },
    { regex: /=========== 零拷贝optimization：sendfile implement ============/g, replacement: '=========== Zero-copy optimization: sendfile implementation ============ ' },
    { regex: /sendfile 上下文structure/g, replacement: 'sendfile context structure' },
    { regex: /outputfiledescription符/g, replacement: 'output file descriptor' },
    { regex: /starttime间（used for超time检测）/g, replacement: 'start time (used for timeout detection)' },
    { regex: /超timetime间（毫秒）/g, replacement: 'timeout (milliseconds)' },
    { regex: /最largeretrycount/g, replacement: 'maximum retry count' },
    { regex: /分blocklargesmall/g, replacement: 'chunk size' }
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