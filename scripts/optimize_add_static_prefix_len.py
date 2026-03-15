#!/usr/bin/env python3
"""
Add static_prefix_len field to router structure
"""
import sys

def add_static_prefix_len():
    file_path = "/home/zhaodi-chen/project/uvhttp/include/uvhttp_router.h"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    # Find the line with "char* static_prefix;"
    new_lines = []
    for i, line in enumerate(lines):
        new_lines.append(line)
        if 'char* static_prefix;' in line:
            # Add static_prefix_len after static_prefix
            new_lines.append('    size_t static_prefix_len;         /* 8 bytes */\n')

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print("✓ Added static_prefix_len field to router structure")

if __name__ == "__main__":
    add_static_prefix_len()