const fs = require('fs');
const path = require('path');

// Final translations for remaining Chinese comments
const translations = {
    // Range and scope
    '范围': 'scope',
    '范围之内': 'within scope',
    '之外': 'outside',
    '之内': 'within',
    '之间': 'between',
    '超过': 'exceed',
    '过': 'over',
    '过大': 'too large',
    '导致': 'cause',
    '整': 'integer',
    '溢出': 'overflow',
    '溢': 'overflow',
    
    // Logging
    '日志': 'log',
    '宏': 'macro',
    '简单': 'simple',
    '仅': 'only',
    '模式': 'mode',
    '下': 'under',
    '且': 'and',
    '功能': 'function',
    
    // LRU cache
    '件': 'file',
    '型': 'type',
    '访问': 'access',
    '量': 'amount',
    '是否': 'whether',
    '压缩': 'compress',
    '哈希': 'hash',
    '希': 'hash',
    '表': 'table',
    '链表': 'linked list',
    '链': 'link',
    '尾': 'tail',
    '总': 'total',
    '限': 'limit',
    '最大限制': 'maximum limit',
    '条目': 'entry',
    '命': '命',
    '未': 'not',
    '驱逐': 'evict',
    '逐': 'evict',
    '永不': 'never',
    '过期': 'expire',
    '针': 'pointer',
    '查': 'check',
    '找': 'find',
    '查找': 'find',
    '未找到': 'not found',
    
    // Connection
    '大块': 'large chunk',
    
    // Particles
    '之': '',
    '导': '',
    '致': '',
    '整': '',
    '日': '',
    '志': '',
    '宏': '',
    '简': '',
    '单': '',
    '仅': '',
    '模': '',
    '式': '',
    '下': '',
    '且': '',
    '功': '',
    '能': '',
    '件': '',
    '型': '',
    '访': '',
    '问': '',
    '量': '',
    '是': '',
    '否': '',
    '压': '',
    '缩': '',
    '哈': '',
    '希': '',
    '表': '',
    '链': '',
    '尾': '',
    '总': '',
    '最': '',
    '大': '',
    '限': '',
    '条': '',
    '目': '',
    '命': '',
    '未': '',
    '驱': '',
    '逐': '',
    '永': '',
    '不': '',
    '过': '',
    '期': '',
    '针': '',
    '查': '',
    '找': '',
    '范': '',
    '围': '',
    '之': '',
    '过': '',
    '导': '',
    '致': '',
    '整': '',
    '溢': '',
    '日': '',
    '志': '',
    '宏': '',
    '简': '',
    '单': '',
    '仅': '',
    '模': '',
    '式': '',
    '下': '',
    '且': '',
    '功': '',
    '能': '',
    
    // Common technical terms
    '指针': 'pointer',
    '引用': 'reference',
    '地址': 'address',
    '偏移': 'offset',
    '长度': 'length',
    '大小': 'size',
    '容量': 'capacity',
    '数量': 'count',
    '总数': 'total',
    '索引': 'index',
    '键': 'key',
    '值': 'value',
    '对': 'pair',
    '映射': 'mapping',
    '集合': 'set',
    '列表': 'list',
    '数组': 'array',
    '向量': 'vector',
    '队列': 'queue',
    '栈': 'stack',
    '树': 'tree',
    '图': 'graph',
    '节点': 'node',
    '边': 'edge',
    '根': 'root',
    '叶': 'leaf',
    '分支': 'branch',
    '路径': 'path',
    '循环': 'loop',
    '递归': 'recursion',
    '迭代': 'iteration',
    '遍历': 'traverse',
    '搜索': 'search',
    '查找': 'find',
    '排序': 'sort',
    '过滤': 'filter',
    '映射': 'map',
    '减少': 'reduce',
    '聚合': 'aggregate',
    '分组': 'group',
    '连接': 'join',
    '合并': 'merge',
    '分割': 'split',
    '切片': 'slice',
    '子集': 'subset',
    '超集': 'superset',
    '交集': 'intersection',
    '并集': 'union',
    '差集': 'difference',
    '补集': 'complement',
    '空集': 'empty set',
    '全集': 'universal set',
    '真': 'true',
    '假': 'false',
    '空': 'null',
    '未定义': 'undefined',
    '零': 'zero',
    '一': 'one',
    '二': 'two',
    '三': 'three',
    '四': 'four',
    '五': 'five',
    '六': 'six',
    '七': 'seven',
    '八': 'eight',
    '九': 'nine',
    '十': 'ten',
    '百': 'hundred',
    '千': 'thousand',
    '万': 'ten thousand',
    '百万': 'million',
    '十亿': 'billion',
};

// Function to translate a file
function translateFile(filePath) {
    console.log(`Processing: ${filePath}`);
    
    let content = fs.readFileSync(filePath, 'utf8');
    let originalLength = content.length;
    let changes = 0;
    
    // Apply all translations
    for (const [chinese, english] of Object.entries(translations)) {
        const regex = new RegExp(chinese, 'g');
        const matches = content.match(regex);
        if (matches) {
            content = content.replace(regex, english);
            changes += matches.length;
        }
    }
    
    // Only write if changes were made
    if (content !== fs.readFileSync(filePath, 'utf8')) {
        fs.writeFileSync(filePath, content, 'utf8');
        console.log(`  ✓ Translated ${changes} phrases`);
        return changes;
    }
    
    console.log(`  No changes`);
    return 0;
}

// Get all header files
const includeDir = path.join(__dirname, '..', 'include');
const files = fs.readdirSync(includeDir).filter(f => f.endsWith('.h') && f.startsWith('uvhttp_'));

console.log(`Found ${files.length} header files to translate\n`);

let totalChanges = 0;
for (const file of files) {
    const filePath = path.join(includeDir, file);
    totalChanges += translateFile(filePath);
}

console.log(`\nTotal translations: ${totalChanges} phrases`);
