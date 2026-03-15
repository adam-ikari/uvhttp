const fs = require('fs');
const path = require('path');

// Translations for single characters
const translations = {
    // Characters
    '盖': 'cover',
    '务': 'service',
    '相': 'related',
    '无': 'no',
    '安': 'safe',
    '后': 'after',
    '关': 'related',
    '至': 'to',
    '少': 'at least',
    '行': 'line',
    '程': 'process',
    '句': 'sentence',
    '柄': 'handle',
    '突': 'conflict',
    '流': 'flow',
    '频': 'frequency',
    '繁': 'frequent',
    '兼': 'compatible',
    '但': 'but',
    '仍': 'still',
    '然': 'however',
    '确': 'ensure',
    '保': 'guarantee',
    '齐': 'align',
    '正': 'correct',
    '降': 'reduce',
    '填': 'fill',
    '充': 'fill',
    '代': 'replace',
    '常': 'constant',
    '伪': 'fake',
    '倍': 'times',
    '界': 'boundary',
    '书': 'certificate',
    '吊': 'revoke',
    '装': 'install',
    
    // Empty strings for particles
    '盖': '',
    '务': '',
    '相': '',
    '无': '',
    '安': '',
    '后': '',
    '关': '',
    '至': '',
    '少': '',
    '行': '',
    '程': '',
    '句': '',
    '柄': '',
    '突': '',
    '流': '',
    '频': '',
    '繁': '',
    '兼': '',
    '但': '',
    '仍': '',
    '然': '',
    '确': '',
    '保': '',
    '齐': '',
    '正': '',
    '降': '',
    '填': '',
    '充': '',
    '代': '',
    '常': '',
    '伪': '',
    '倍': '',
    '界': '',
    '书': '',
    '吊': '',
    '装': '',
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
