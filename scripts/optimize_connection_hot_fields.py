#!/usr/bin/env python3
"""
Optimize uvhttp_connection_t by moving current_header_field_len to cache line 2
"""
import sys

def optimize_connection_hot_fields():
    """Move current_header_field_len to cache line 2 for better cache locality"""
    file_path = "/home/zhaodi-chen/project/uvhttp/include/uvhttp_connection.h"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # Add current_header_field_len after parsing_header_field in cache line 2
        if 'int parsing_header_field;' in line and 'useparsingparsingheaderField' in line:
            new_lines.append(line)
            new_lines.append('    size_t current_header_field_len; /* 8 bytes - hot: header field length */\n')
        # Remove current_header_field_len from cache line 4
        elif 'size_t current_header_field_len;' in line and 'currentheaderFieldlength' in line:
            i += 1
            continue
        # Adjust padding in cache line 4
        elif 'int _padding3[14]' in line:
            new_lines.append('    int _padding3[14];               /* 56 bytes - padding to 64 bytes */\n')
        else:
            new_lines.append(line)

        i += 1

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print("✓ Optimized uvhttp_connection_t cache layout")
    print("  - Moved current_header_field_len to cache line 2")
    print("  - This field is accessed during HTTP header parsing (hot path)")

if __name__ == "__main__":
    optimize_connection_hot_fields()
