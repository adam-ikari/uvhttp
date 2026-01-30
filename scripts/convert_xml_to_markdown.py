#!/usr/bin/env python3
"""
Convert Doxygen XML documentation to Markdown format.
This script parses Doxygen XML files and generates clean Markdown documentation.
"""

import os
import sys
import xml.etree.ElementTree as ET
from pathlib import Path
from typing import Dict, List, Optional

# Colors for output
RED = '\033[0;31m'
GREEN = '\033[0;32m'
YELLOW = '\033[1;33m'
NC = '\033[0m'  # No Color


class DoxygenXMLParser:
    """Parse Doxygen XML files and convert to Markdown."""

    def __init__(self, xml_dir: str, output_dir: str):
        self.xml_dir = Path(xml_dir)
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)

    def parse_index(self) -> Dict:
        """Parse the index.xml file to get all compounds."""
        index_file = self.xml_dir / "index.xml"
        if not index_file.exists():
            print(f"{RED}Error: index.xml not found{NC}")
            sys.exit(1)

        tree = ET.parse(index_file)
        root = tree.getroot()

        compounds = {
            'structs': [],
            'files': [],
            'dirs': []
        }

        for compound in root.findall('compound'):
            kind = compound.get('kind')
            refid = compound.get('refid')
            name = compound.find('name').text if compound.find('name') is not None else ''

            if kind == 'struct':
                compounds['structs'].append({'refid': refid, 'name': name})
            elif kind == 'file':
                compounds['files'].append({'refid': refid, 'name': name})
            elif kind == 'dir':
                compounds['dirs'].append({'refid': refid, 'name': name})

        return compounds

    def parse_compound(self, refid: str) -> ET.Element:
        """Parse a compound XML file."""
        compound_file = self.xml_dir / f"{refid}.xml"
        if not compound_file.exists():
            return None

        tree = ET.parse(compound_file)
        return tree.getroot()

    def extract_text(self, element: ET.Element) -> str:
        """Extract text content from an element, handling nested elements."""
        if element is None:
            return ''

        text = element.text or ''
        for child in element:
            child_text = self.extract_text(child)
            if child.tag == 'ref':
                refid = child.get('refid', '')
                ref_text = child.text or ''
                text += f"[{ref_text}](#{refid.lower()})"
            elif child.tag == 'para':
                text += '\n\n' + child_text
            else:
                text += child_text
            text += child.tail or ''

        return text.strip()

    def convert_struct_to_markdown(self, refid: str, name: str) -> str:
        """Convert a struct XML to Markdown."""
        root = self.parse_compound(refid)
        if root is None:
            return ''

        compounddef = root.find('compounddef')
        if compounddef is None:
            return ''

        # Extract brief description
        brief = self.extract_text(compounddef.find('briefdescription'))
        detailed = self.extract_text(compounddef.find('detaileddescription'))

        # Build markdown
        md = f"# {name}\n\n"
        if brief:
            md += f"{brief}\n\n"
        if detailed:
            md += f"{detailed}\n\n"

        # Extract members
        sections = compounddef.findall('sectiondef')
        for section in sections:
            kind = section.get('kind')
            if kind in ['public-attrib', 'protected-attrib', 'private-attrib']:
                md += f"## Members\n\n"
            elif kind in ['public-func', 'protected-func', 'private-func']:
                md += f"## Methods\n\n"
            elif kind == 'public-type':
                md += f"## Types\n\n"
            else:
                continue

            for memberdef in section.findall('memberdef'):
                member_name = memberdef.find('name').text if memberdef.find('name') is not None else ''
                member_type = self.extract_text(memberdef.find('type'))
                member_brief = self.extract_text(memberdef.find('briefdescription'))

                md += f"### {member_name}\n\n"
                if member_type:
                    md += f"**Type:** `{member_type}`\n\n"
                if member_brief:
                    md += f"{member_brief}\n\n"

        return md

    def convert_file_to_markdown(self, refid: str, name: str) -> str:
        """Convert a file XML to Markdown."""
        root = self.parse_compound(refid)
        if root is None:
            return ''

        compounddef = root.find('compounddef')
        if compounddef is None:
            return ''

        # Extract brief and detailed description
        brief = self.extract_text(compounddef.find('briefdescription'))
        detailed = self.extract_text(compounddef.find('detaileddescription'))

        # Build markdown
        md = f"# {name}\n\n"
        if brief:
            md += f"{brief}\n\n"
        if detailed:
            md += f"{detailed}\n\n"

        # Extract functions
        sections = compounddef.findall('sectiondef')
        for section in sections:
            kind = section.get('kind')
            if kind == 'func':
                md += "## Functions\n\n"
                for memberdef in section.findall('memberdef'):
                    func_name = memberdef.find('name').text if memberdef.find('name') is not None else ''
                    func_type = self.extract_text(memberdef.find('type'))
                    func_args = self.extract_text(memberdef.find('argsstring'))
                    func_brief = self.extract_text(memberdef.find('briefdescription'))
                    func_detailed = self.extract_text(memberdef.find('detaileddescription'))

                    md += f"### {func_name}{func_args}\n\n"
                    if func_type:
                        md += f"**Return Type:** `{func_type}`\n\n"
                    if func_brief:
                        md += f"{func_brief}\n\n"
                    if func_detailed:
                        md += f"{func_detailed}\n\n"

        return md

    def generate_docs(self):
        """Generate all Markdown documentation."""
        print(f"{GREEN}📝 Converting Doxygen XML to Markdown...{NC}")

        compounds = self.parse_index()

        # Convert structs
        print(f"{YELLOW}Converting structs...{NC}")
        for struct in compounds['structs']:
            print(f"  Converting {struct['name']}...")
            md = self.convert_struct_to_markdown(struct['refid'], struct['name'])
            output_file = self.output_dir / f"struct_{struct['name']}.md"
            output_file.write_text(md, encoding='utf-8')

        # Convert files
        print(f"{YELLOW}Converting files...{NC}")
        for file in compounds['files']:
            print(f"  Converting {file['name']}...")
            md = self.convert_file_to_markdown(file['refid'], file['name'])
            # Clean filename
            clean_name = file['name'].replace('/', '_').replace('.', '_')
            output_file = self.output_dir / f"file_{clean_name}.md"
            output_file.write_text(md, encoding='utf-8')

        # Generate index
        self.generate_index(compounds)

        print(f"{GREEN}✅ Markdown documentation generated successfully!{NC}")
        print(f"  Location: {self.output_dir}/")
        print(f"  Index file: {self.output_dir}/index.md")

    def generate_index(self, compounds: Dict):
        """Generate the main index.md file."""
        md = "# UVHTTP API Documentation\n\n"
        md += "This documentation is generated from Doxygen XML output.\n\n"

        md += "## Structures\n\n"
        for struct in compounds['structs']:
            md += f"- [{struct['name']}](struct_{struct['name']}.md)\n"

        md += "\n## Files\n\n"
        for file in compounds['files']:
            clean_name = file['name'].replace('/', '_').replace('.', '_')
            md += f"- [{file['name']}](file_{clean_name}.md)\n"

        output_file = self.output_dir / "index.md"
        output_file.write_text(md, encoding='utf-8')


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage: convert_xml_to_markdown.py <xml_dir> [output_dir]")
        print("  xml_dir:    Directory containing Doxygen XML files")
        print("  output_dir: Output directory for Markdown files (default: docs/api/markdown_from_xml)")
        sys.exit(1)

    xml_dir = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "docs/api/markdown_from_xml"

    parser = DoxygenXMLParser(xml_dir, output_dir)
    parser.generate_docs()


if __name__ == "__main__":
    main()