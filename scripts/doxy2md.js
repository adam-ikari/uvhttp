#!/usr/bin/env node
/**
 * Doxygen XML to Markdown 转换器
 * 使用 Nunjucks 模板引擎生成拆分的 Markdown 文件
 * 生成所有类型：结构体、函数、宏、枚举等
 * 
 * @example
 * node doxy2md.js
 */

const fs = require('fs');
const path = require('path');
const xml2js = require('xml2js');
const nunjucks = require('nunjucks');

// 配置 Nunjucks
const templateDir = path.join(__dirname, '.doxygen_templates');
const env = nunjucks.configure(templateDir, { autoescape: false });

// 解析 XML 文件
function parseXML(filePath) {
  const content = fs.readFileSync(filePath, 'utf-8');
  return xml2js.parseStringPromise(content);
}

// 提取结构体信息
function extractStructInfo(compound) {
  const name = compound.compoundname[0];
  const location = compound.location?.[0];
  const sections = compound.sectiondef || [];
  
  // 提取成员
  const members = [];
  for (const section of sections) {
    if (section.memberdef) {
      for (const member of section.memberdef) {
        const memberType = extractType(member.type);
        members.push({
          name: member.name?.[0] || '',
          type: memberType,
          brief: extractDescription(member.briefdescription),
          detailed: extractDescription(member.detaileddescription)
        });
      }
    }
  }
  
  return {
    name,
    location: location ? `${location.file}:${location.line}` : null,
    members
  };
}

// 提取函数信息
function extractFunctionInfo(compound) {
  const name = compound.compoundname[0];
  const location = compound.location?.[0];
  const sections = compound.sectiondef || [];
  
  const functions = [];
  for (const section of sections) {
    if (section.memberdef) {
      for (const member of section.memberdef) {
        if (member.$.kind === 'function') {
          const returnType = extractType(member.type);
          const params = member.param || [];
          const paramList = params.map(p => ({
            type: extractType(p.type),
            name: p.declname?.[0] || ''
          }));
          
          functions.push({
            name: member.name?.[0] || '',
            returnType,
            params: paramList,
            brief: extractDescription(member.briefdescription),
            detailed: extractDescription(member.detaileddescription)
          });
        }
      }
    }
  }
  
  return {
    name,
    location: location ? `${location.file}:${location.line}` : null,
    functions
  };
}

// 提取宏定义信息
function extractDefineInfo(compound) {
  const name = compound.compoundname[0];
  const location = compound.location?.[0];
  const sections = compound.sectiondef || [];
  
  const defines = [];
  for (const section of sections) {
    if (section.memberdef) {
      for (const member of section.memberdef) {
        if (member.$.kind === 'define') {
          defines.push({
            name: member.name?.[0] || '',
            value: member.initializer?.[0] || '',
            brief: extractDescription(member.briefdescription),
            detailed: extractDescription(member.detaileddescription)
          });
        }
      }
    }
  }
  
  return {
    name,
    location: location ? `${location.file}:${location.line}` : null,
    defines
  };
}

// 提取类型文本（处理 ref 标签）
function extractType(typeArray) {
  if (!typeArray || typeArray.length === 0) return '';
  
  return typeArray.map(item => {
    if (typeof item === 'string') {
      return item;
    } else if (typeof item === 'object' && item !== null) {
      if (item.ref && Array.isArray(item.ref)) {
        return item.ref.map(r => r._ || r).join('');
      }
      return Object.values(item).map(v => extractType(Array.isArray(v) ? v : [v])).join('');
    }
    return '';
  }).join('').trim();
}

// 提取描述文本
function extractDescription(descArray) {
  if (!descArray) return '';
  if (!Array.isArray(descArray)) return '';
  if (descArray.length === 0) return '';
  
  return descArray.map(item => {
    if (typeof item === 'string') {
      return item;
    } else if (typeof item === 'object') {
      return Object.values(item).map(v => {
        if (typeof v === 'string') return v;
        if (typeof v === 'object' && Array.isArray(v)) {
          return v.map(extractDescription).join('');
        }
        return extractDescription(v);
      }).join('');
    }
    return '';
  }).join('').trim();
}

// Nunjucks 过滤器：将类型转换为链接
env.addFilter('linkTypes', function(text, structNames) {
  if (!text) return '';
  
  let result = text;
  const sortedNames = [...structNames].sort((a, b) => b.length - a.length);
  
  for (const name of sortedNames) {
    const escapedName = name.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
    const regex = new RegExp(`\\b${escapedName}\\b`, 'g');
    const linkPath = name.toLowerCase().replace(/ /g, '_');
    result = result.replace(regex, `[\`${name}\`](./structs/${linkPath}.md)`);
  }
  
  return result;
});

// 主函数
async function main() {
  const xmlDir = path.join(process.cwd(), 'docs/api/.doxygen/xml');
  const structsDir = path.join(process.cwd(), 'docs/api/structs');
  const functionsDir = path.join(process.cwd(), 'docs/api/functions');
  const definesDir = path.join(process.cwd(), 'docs/api/defines');
  
  // 创建输出目录
  if (!fs.existsSync(structsDir)) {
    fs.mkdirSync(structsDir, { recursive: true });
  }
  if (!fs.existsSync(functionsDir)) {
    fs.mkdirSync(functionsDir, { recursive: true });
  }
  if (!fs.existsSync(definesDir)) {
    fs.mkdirSync(definesDir, { recursive: true });
  }
  
  // 查找所有 XML 文件
  const files = fs.readdirSync(xmlDir).filter(f => f.endsWith('.xml') && !f.startsWith('index') && !f.includes('.xsl') && !f.includes('.xsd'));
  
  const structs = [];
  const functions = [];
  const defines = [];
  
  // 解析每个 XML 文件
  for (const file of files) {
    const filePath = path.join(xmlDir, file);
    const xml = await parseXML(filePath);
    
    if (xml.doxygen?.compounddef) {
      const compound = xml.doxygen.compounddef[0];
      const kind = compound.$.kind;
      
      if (kind === 'struct') {
        const struct = extractStructInfo(compound);
        structs.push(struct);
      } else if (kind === 'file') {
        // 提取文件中的函数和宏
        const funcInfo = extractFunctionInfo(compound);
        if (funcInfo.functions.length > 0) {
          functions.push(funcInfo);
        }
        
        const defineInfo = extractDefineInfo(compound);
        if (defineInfo.defines.length > 0) {
          defines.push(defineInfo);
        }
      }
    }
  }
  
  // 提取所有结构体名称用于链接
  const structNames = structs.map(s => s.name);
  
  // 生成结构体文件
  for (const struct of structs) {
    const fileName = struct.name.toLowerCase().replace(/ /g, '_');
    const md = nunjucks.render('struct.njk', { struct, structNames });
    fs.writeFileSync(path.join(structsDir, `${fileName}.md`), md, 'utf-8');
  }
  
  // 生成函数文件
  for (const funcFile of functions) {
    const fileName = funcFile.name.toLowerCase().replace(/ /g, '_');
    const md = nunjucks.render('functions.njk', { funcFile, structNames });
    fs.writeFileSync(path.join(functionsDir, `${fileName}.md`), md, 'utf-8');
  }
  
  // 生成宏定义文件
  for (const defineFile of defines) {
    const fileName = defineFile.name.toLowerCase().replace(/ /g, '_');
    const md = nunjucks.render('defines.njk', { defineFile, structNames });
    fs.writeFileSync(path.join(definesDir, `${fileName}.md`), md, 'utf-8');
  }
  
  // 生成索引文件
  const indexContent = nunjucks.render('index.njk', { structs, functions, defines });
  fs.writeFileSync(path.join(process.cwd(), 'docs/api/api.md'), indexContent, 'utf-8');
  
  console.log(`✅ 已生成 ${structs.length} 个结构体文件`);
  console.log(`✅ 已生成 ${functions.length} 个函数文件`);
  console.log(`✅ 已生成 ${defines.length} 个宏定义文件`);
  console.log(`📁 输出目录: structs/, functions/, defines/`);
}

main().catch(err => {
  console.error('❌ 错误:', err);
  process.exit(1);
});