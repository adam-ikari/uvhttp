#!/usr/bin/env python3
"""
Fix the second strlen optimization in uvhttp_protocol_upgrade.c
"""
import sys

def fix_protocol_upgrade():
    """Fix the second strlen optimization"""
    file_path = "/home/zhaodi-chen/project/uvhttp/src/uvhttp_protocol_upgrade.c"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # Find the comment line before the second loop
        if 'Normalize upgrade header to lowercase' in line:
            new_lines.append(line)
            i += 1
            # Add the length caching before the for loop
            new_lines.append('        size_t upgrade_header_len = strlen(upgrade_header);\n')
            # Replace the for loop
            if i < len(lines) and 'for (size_t i = 0;' in lines[i]:
                new_lines.append('        for (size_t i = 0; i < upgrade_header_len && i < sizeof(info->upgrade_header) - 1; i++) {\n')
                i += 3  # Skip the old for loop lines
                continue
        else:
            new_lines.append(line)

        i += 1

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print("✓ Fixed second strlen optimization")

if __name__ == "__main__":
    fix_protocol_upgrade()