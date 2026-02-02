#!/usr/bin/env node
/**
 * Convert Doxygen XML documentation to Markdown format.
 * This script parses Doxygen XML files and generates clean Markdown documentation.
 * 
 * @example
 * node convert_xml_to_markdown.js docs/api/xml docs/api/markdown_from_xml
 */

const fs = require('fs');
const path = require('path');
const { XMLParser } = require('fast-xml-parser');

// ANSI color codes
const colors = {
  red: '\x1b[0;31m',
  green: '\x1b[0;32m',
  yellow: '\x1b[1;33m',
  nc: '\x1b[0m'
};

/**
 * Validate a path to prevent directory traversal attacks
 * @param {string} inputPath - The path to validate
 * @param {string} type - The type of path (e.g., 'xml_dir', 'output_dir')
 * @throws {Error} If path is invalid
 */
function validatePath(inputPath, type) {
  if (!inputPath || typeof inputPath !== 'string') {
    throw new Error(`${type} must be a non-empty string`);
  }
  
  // Check for path traversal attempts
  const normalizedPath = path.normalize(inputPath);
  if (normalizedPath.includes('..')) {
    throw new Error(`${type} cannot contain parent directory references (..)`);
  }
  
  // Check for absolute paths (only allow relative paths from project root)
  if (path.isAbsolute(inputPath)) {
    throw new Error(`${type} must be a relative path`);
  }
}

/**
 * Doxygen XML Parser class
 * Parses Doxygen XML files and converts them to Markdown format
 */
class DoxygenXMLParser {
  /**
   * Create a new DoxygenXMLParser instance
   * @param {string} xmlDir - Directory containing Doxygen XML files
   * @param {string} outputDir - Directory where Markdown files will be written
   * @throws {Error} If paths are invalid
   */
  constructor(xmlDir, outputDir) {
    validatePath(xmlDir, 'xml_dir');
    validatePath(outputDir, 'output_dir');
    
    this.xmlDir = xmlDir;
    this.outputDir = outputDir;
    this.outputDir = path.resolve(outputDir);
    
    if (!fs.existsSync(this.outputDir)) {
      try {
        fs.mkdirSync(this.outputDir, { recursive: true });
      } catch (error) {
        throw new Error(`Failed to create output directory: ${error.message}`);
      }
    }
    
    if (!fs.existsSync(this.xmlDir)) {
      throw new Error(`XML directory does not exist: ${this.xmlDir}`);
    }
    
    this.parser = new XMLParser({
      ignoreAttributes: false,
      attributeNamePrefix: '',
      textNodeName: 'text',
      parseAttributeValue: true,
      trimValues: true,
      ignoreDeclaration: true,
      ignorePiTags: true
    });
  }

  /**
   * Extract type information from a type element
   * Handles references and creates appropriate markdown links
   * @param {Object} element - The type element to extract from
   * @returns {string} Extracted type text with markdown links
   */
  extractType(element) {
    if (!element) return '';

    // Special handling for type element with ref and text
    if (element.ref && element.text) {
      const refid = element.ref.refid || '';
      const refText = this.extractText(element.ref);
      const suffix = this.extractText({ text: element.text });
      if (refid && refText) {
        // Add space between link and pointer symbol
        return `[${refText}](#${refid.toLowerCase()}) ${suffix}`;
      }
      return refText + ' ' + suffix;
    }

    return this.extractText(element);
  }

  /**
   * Parse the index.xml file to get all compounds
   * @returns {Object} Object containing structs, files, and dirs arrays
   * @throws {Error} If index.xml is missing or parsing fails
   */
  parseIndex() {
    const indexPath = path.join(this.xmlDir, 'index.xml');
    if (!fs.existsSync(indexPath)) {
      throw new Error(`index.xml not found at: ${indexPath}`);
    }

    let xmlContent;
    try {
      xmlContent = fs.readFileSync(indexPath, 'utf-8');
    } catch (error) {
      throw new Error(`Failed to read index.xml: ${error.message}`);
    }

    let result;
    try {
      result = this.parser.parse(xmlContent);
    } catch (error) {
      throw new Error(`Failed to parse index.xml: ${error.message}`);
    }

    const compounds = {
      structs: [],
      files: [],
      dirs: []
    };

    // Handle both doxygenindex and doxygen root elements
    const root = result.doxygenindex || result.doxygen;
    if (!root) return compounds;

    const compoundArray = Array.isArray(root.compound)
      ? root.compound
      : root.compound ? [root.compound] : [];

    for (const compound of compoundArray) {
      const kind = compound.kind;
      const refid = compound.refid;
      const name = compound.name?.text || compound.name || '';

      if (kind === 'struct') {
        compounds.structs.push({ refid, name });
      } else if (kind === 'file') {
        compounds.files.push({ refid, name });
      } else if (kind === 'dir') {
        compounds.dirs.push({ refid, name });
      }
    }

    return compounds;
  }

  /**
   * Parse a compound XML file
   * @param {string} refid - The reference ID of the compound
   * @returns {Object|null} Parsed compound object or null if file not found
   */
  parseCompound(refid) {
    const compoundFile = path.join(this.xmlDir, `${refid}.xml`);
    if (!fs.existsSync(compoundFile)) {
      console.warn(`${colors.yellow}Warning: Compound file not found: ${compoundFile}${colors.nc}`);
      return null;
    }

    try {
      const xmlContent = fs.readFileSync(compoundFile, 'utf-8');
      return this.parser.parse(xmlContent);
    } catch (error) {
      console.error(`${colors.red}Error parsing ${compoundFile}: ${error.message}${colors.nc}`);
      return null;
    }
  }

  /**
   * Extract text content from an XML element
   * Handles nested elements, references, and arrays
   * @param {*} element - The element to extract text from
   * @returns {string} Extracted text content
   */
  extractText(element) {
    if (!element) return '';

    // If element is a string, return it
    if (typeof element === 'string') return element.trim();

    // If element has text property, use it
    if (element.text) return element.text.trim();

    // If element has para property, extract text from it
    if (element.para) {
      return this.extractText(element.para);
    }

    // If element is a ref element, create a link
    if (element.kindref) {
      const refid = element.refid || '';
      const text = this.extractText(element);
      if (refid && text) {
        return `[${text}](#${refid.toLowerCase()})`;
      }
      return text;
    }

    // If element is an array, extract text from each item
    if (Array.isArray(element)) {
      let text = '';
      for (const item of element) {
        const itemText = this.extractText(item);
        if (itemText) {
          text += itemText;
        }
      }
      return text.trim();
    }

    // If element has children, recursively extract text
    if (element.children && Array.isArray(element.children)) {
      let text = '';
      for (const child of element.children) {
        const childText = this.extractText(child);
        if (childText) {
          text += childText + ' ';
        }
      }
      return text.trim();
    }

    return '';
  }

  /**
   * Extract parameter list from detailed description
   * @param {*} detaileddescription - The detailed description element
   * @returns {string} Markdown formatted parameter list
   */
  extractParameterList(detaileddescription) {
    if (!detaileddescription) return '';

    // Find parameterlist in the description
    let paramList = null;
    if (detaileddescription.parameterlist) {
      paramList = detaileddescription.parameterlist;
    } else if (Array.isArray(detaileddescription.children)) {
      paramList = detaileddescription.children.find(
        child => child.name === 'parameterlist'
      );
    } else if (Array.isArray(detaileddescription)) {
      // para is an array, search for parameterlist in array elements
      for (const para of detaileddescription) {
        if (typeof para === 'object' && para.parameterlist) {
          paramList = para.parameterlist;
          break;
        }
      }
    }

    if (!paramList) return '';

    let md = '**Parameters:**\n\n';
    let items = [];
    if (Array.isArray(paramList.parameteritem)) {
      items = paramList.parameteritem;
    } else if (paramList.parameteritem) {
      items = [paramList.parameteritem];
    } else if (paramList.children) {
      items = paramList.children.filter(child => child.name === 'parameteritem');
    }

    for (const item of items) {
      const paramNameElem = item.parameternamelist || item.children?.find(
        child => child.name === 'parameternamelist'
      );
      const paramDescElem = item.parameterdescription || item.children?.find(
        child => child.name === 'parameterdescription'
      );

      const paramName = this.extractText(paramNameElem?.parametername || paramNameElem?.children?.find(c => c.name === 'parametername'));
      const paramDesc = this.extractText(paramDescElem);

      if (paramName) {
        md += `- \`${paramName}\`: ${paramDesc}\n`;
      }
    }

    return md + '\n';
  }

  /**
   * Extract return value from detailed description
   * @param {*} detaileddescription - The detailed description element
   * @returns {string} Markdown formatted return value
   */
  extractReturnValue(detaileddescription) {
    if (!detaileddescription) return '';

    let simplesects = [];
    if (Array.isArray(detaileddescription.simplesect)) {
      simplesects = detaileddescription.simplesect;
    } else if (Array.isArray(detaileddescription.children)) {
      simplesects = detaileddescription.children.filter(child => child.name === 'simplesect');
    } else if (Array.isArray(detaileddescription)) {
      // para is an array, search for simplesect in array elements
      for (const para of detaileddescription) {
        if (typeof para === 'object' && para.simplesect) {
          if (Array.isArray(para.simplesect)) {
            simplesects.push(...para.simplesect);
          } else {
            simplesects.push(para.simplesect);
          }
        }
      }
    }

    for (const simplesect of simplesects) {
      if (simplesect.kind === 'return') {
        const desc = this.extractText(simplesect.para || simplesect);
        if (desc) {
          return `**Returns:** ${desc}\n\n`;
        }
      }
    }

    return '';
  }

  /**
   * Extract notes from detailed description
   * @param {*} detaileddescription - The detailed description element
   * @returns {string} Markdown formatted notes
   */
  extractNotes(detaileddescription) {
    if (!detaileddescription) return '';

    let simplesects = [];
    if (Array.isArray(detaileddescription.simplesect)) {
      simplesects = detaileddescription.simplesect;
    } else if (Array.isArray(detaileddescription.children)) {
      simplesects = detaileddescription.children.filter(child => child.name === 'simplesect');
    } else if (Array.isArray(detaileddescription)) {
      // para is an array, search for simplesect in array elements
      for (const para of detaileddescription) {
        if (typeof para === 'object' && para.simplesect) {
          if (Array.isArray(para.simplesect)) {
            simplesects.push(...para.simplesect);
          } else {
            simplesects.push(para.simplesect);
          }
        }
      }
    }

    const notes = [];
    for (const simplesect of simplesects) {
      if (['note', 'warning', 'attention', 'remark'].includes(simplesect.kind)) {
        const desc = this.extractText(simplesect.para || simplesect);
        if (desc) {
          notes.push(`**${simplesect.kind.charAt(0).toUpperCase() + simplesect.kind.slice(1)}:** ${desc}`);
        }
      }
    }

    if (notes.length > 0) {
      return notes.join('\n') + '\n\n';
    }

    return '';
  }

  /**
   * Convert a struct compound to Markdown
   * @param {string} refid - The reference ID of the struct
   * @param {string} name - The name of the struct
   * @returns {string} Markdown formatted struct documentation
   */
  convertStructToMarkdown(refid, name) {
    const root = this.parseCompound(refid);
    if (!root) return '';

    const compounddef = root.doxygen?.compounddef;
    if (!compounddef) return '';

    const brief = this.extractText(compounddef.briefdescription);
    const detailed = this.extractText(compounddef.detaileddescription);

    let md = `# ${name}\n\n`;
    if (brief) md += `${brief}\n\n`;
    if (detailed) md += `${detailed}\n\n`;

    const sections = Array.isArray(compounddef.sectiondef)
      ? compounddef.sectiondef
      : [compounddef.sectiondef];

    for (const section of sections) {
      const kind = section.kind;
      if (['public-attrib', 'protected-attrib', 'private-attrib'].includes(kind)) {
        md += `## Members\n\n`;
      } else if (['public-func', 'protected-func', 'private-func'].includes(kind)) {
        md += `## Methods\n\n`;
      } else if (kind === 'public-type') {
        md += `## Types\n\n`;
      } else {
        continue;
      }

      const members = Array.isArray(section.memberdef)
        ? section.memberdef
        : [section.memberdef];

      for (const memberdef of members) {
        const memberName = memberdef.name?.text || '';
        const memberType = this.extractText(memberdef.type);
        const memberBrief = this.extractText(memberdef.briefdescription);

        md += `### ${memberName}\n\n`;
        if (memberType) md += `**Type:** \`${memberType}\`\n\n`;
        if (memberBrief) md += `${memberBrief}\n\n`;
      }
    }

    return md;
  }

  /**
   * Convert a file compound to Markdown
   * @param {string} refid - The reference ID of the file
   * @param {string} name - The name of the file
   * @returns {string} Markdown formatted file documentation
   */
  convertFileToMarkdown(refid, name) {
    const root = this.parseCompound(refid);
    if (!root) return '';

    const compounddef = root.doxygen?.compounddef;
    if (!compounddef) return '';

    const brief = this.extractText(compounddef.briefdescription);
    const detailed = this.extractText(compounddef.detaileddescription);

    let md = `# ${name}\n\n`;
    if (brief) md += `${brief}\n\n`;
    if (detailed) md += `${detailed}\n\n`;

    // Handle sectiondef - it could be an array or a single element
    const sectiondefRaw = compounddef.sectiondef;
    if (!sectiondefRaw) return md;

    const sections = Array.isArray(sectiondefRaw)
      ? sectiondefRaw
      : [sectiondefRaw];

    for (const section of sections) {
      if (!section || !section.kind) continue;

      const kind = section.kind;
      if (kind === 'func') {
        md += '## Functions\n\n';
        const memberdefRaw = section.memberdef;
        if (!memberdefRaw) continue;

        const members = Array.isArray(memberdefRaw)
          ? memberdefRaw
          : [memberdefRaw];

        for (const memberdef of members) {
          if (!memberdef) continue;

          const funcName = memberdef.name?.text || memberdef.name || '';
          const funcType = this.extractType(memberdef.type);
          const funcArgs = this.extractText(memberdef.argsstring);
          const funcBrief = this.extractText(memberdef.briefdescription);
          const funcDetailed = memberdef.detaileddescription;

          // Skip macro definitions
          if (funcName.startsWith('UVHTTP_')) continue;

          md += `### ${funcName}${funcArgs}\n\n`;
          if (funcType) md += `**Return Type:** \`${funcType}\`\n\n`;
          if (funcBrief) md += `${funcBrief}\n\n`;

          if (funcDetailed) {
            // Pass the para element to extraction methods
            const para = funcDetailed.para || funcDetailed;
            md += this.extractParameterList(para);
            md += this.extractReturnValue(para);
            md += this.extractNotes(para);
          }
        }
      }
    }

    return md;
  }

  /**
   * Generate the main index.md file
   * @param {Object} compounds - Object containing structs, files, and dirs
   * @throws {Error} If writing index.md fails
   */
  generateIndex(compounds) {
    let md = '# UVHTTP API Documentation\n\n';
    md += 'This directory contains the API documentation for UVHTTP, organized by type.\n\n';
    
    if (compounds.structs.length > 0) {
      md += '## Structures\n\n';
      for (const struct of compounds.structs) {
        const filename = `struct_${struct.name}.md`;
        md += `- [${struct.name}](${filename})\n`;
      }
      md += '\n';
    }
    
    if (compounds.files.length > 0) {
      md += '## Files\n\n';
      for (const file of compounds.files) {
        const cleanName = file.name.replace(/\//g, '_').replace(/\./g, '_');
        const filename = `file_${cleanName}.md`;
        md += `- [${file.name}](${filename})\n`;
      }
      md += '\n';
    }
    
    const indexPath = path.join(this.outputDir, 'index.md');
    try {
      fs.writeFileSync(indexPath, md, 'utf-8');
    } catch (error) {
      throw new Error(`Failed to write index.md: ${error.message}`);
    }
  }

  /**
   * Generate all Markdown documentation
   * @throws {Error} If generation fails
   */
  generateDocs() {
    console.log(`${colors.green}📝 Converting Doxygen XML to Markdown...${colors.nc}`);

    let compounds;
    try {
      compounds = this.parseIndex();
    } catch (error) {
      console.error(`${colors.red}Error: ${error.message}${colors.nc}`);
      process.exit(1);
    }

    // Convert structs
    console.log(`${colors.yellow}Converting structs...${colors.nc}`);
    for (const struct of compounds.structs) {
      console.log(`  Converting ${struct.name}...`);
      try {
        const md = this.convertStructToMarkdown(struct.refid, struct.name);
        const outputFile = path.join(this.outputDir, `struct_${struct.name}.md`);
        fs.writeFileSync(outputFile, md, 'utf-8');
      } catch (error) {
        console.error(`${colors.red}Error converting ${struct.name}: ${error.message}${colors.nc}`);
      }
    }

    // Convert files
    console.log(`${colors.yellow}Converting files...${colors.nc}`);
    for (const file of compounds.files) {
      console.log(`  Converting ${file.name}...`);
      try {
        const md = this.convertFileToMarkdown(file.refid, file.name);
        const cleanName = file.name.replace(/\//g, '_').replace(/\./g, '_');
        const outputFile = path.join(this.outputDir, `file_${cleanName}.md`);
        fs.writeFileSync(outputFile, md, 'utf-8');
      } catch (error) {
        console.error(`${colors.red}Error converting ${file.name}: ${error.message}${colors.nc}`);
      }
    }

    // Generate index
    try {
      this.generateIndex(compounds);
      console.log(`${colors.yellow}Generating index.md...${colors.nc}`);
    } catch (error) {
      console.error(`${colors.red}Error generating index.md: ${error.message}${colors.nc}`);
    }

    console.log(`${colors.green}✅ Markdown documentation generated successfully!${colors.nc}`);
    console.log(`  Location: ${this.outputDir}`);
    console.log(`  Structs: ${compounds.structs.length}`);
    console.log(`  Files: ${compounds.files.length}`);
  }
}

// Main execution
try {
  const args = process.argv.slice(2);
  if (args.length !== 2) {
    console.log('Usage: node convert_xml_to_markdown.js <xml_dir> <output_dir>');
    console.log('Example: node convert_xml_to_markdown.js docs/api/xml docs/api/markdown_from_xml');
    process.exit(1);
  }

  const [xmlDir, outputDir] = args;
  const parser = new DoxygenXMLParser(xmlDir, outputDir);
  parser.generateDocs();
} catch (error) {
  console.error(`${colors.red}Fatal error: ${error.message}${colors.nc}`);
  process.exit(1);
}