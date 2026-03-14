#!/usr/bin/env python3
"""
Move static_prefix_len to end of router structure to improve cache locality
"""
import sys

def optimize_router_structure():
    """Move static_prefix_len to reduce cache impact on hot path fields"""
    file_path = "/home/zhaodi-chen/project/uvhttp/include/uvhttp_router.h"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # Remove static_prefix_len from current position
        if 'size_t static_prefix_len;' in line:
            # Skip this line, will add it later
            i += 1
            continue

        # Add static_prefix_len before closing brace of struct
        if line.strip() == '};' and 'static_prefix_len' not in ''.join(new_lines[-20:]):
            new_lines.append('    size_t static_prefix_len;         /* 8 bytes - cache optimization */\n')
            new_lines.append(line)
        else:
            new_lines.append(line)

        i += 1

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print("✓ Moved static_prefix_len to end of router structure for better cache locality")

if __name__ == "__main__":
    optimize_router_structure()