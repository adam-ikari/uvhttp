#!/usr/bin/env python3
"""
Fix indentation for static_prefix_len reset
"""
import sys

def fix_indent():
    file_path = "/home/zhaodi-chen/project/uvhttp/src/uvhttp_router.c"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # Fix indentation for static_prefix_len reset
        if line.strip() == 'router->static_prefix_len = 0;':
            new_lines.append('            router->static_prefix_len = 0;\n')
        else:
            new_lines.append(line)

        i += 1

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print("✓ Fixed indentation for static_prefix_len reset")

if __name__ == "__main__":
    fix_indent()