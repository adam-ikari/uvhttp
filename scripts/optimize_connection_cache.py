#!/usr/bin/env python3
"""
Optimize uvhttp_connection_t cache layout by moving hot fields to cache line 1
"""
import sys

def optimize_connection_cache():
    """Move current_header_field_len to cache line 1"""
    file_path = "/home/zhaodi-chen/project/uvhttp/include/uvhttp_connection.h"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # Find the line with read_buffer_size (end of cache line 1)
        if 'size_t read_buffer_size;' in line and 'Cache line 1 total' not in lines[i+1]:
            new_lines.append(line)
            # Add current_header_field_len to cache line 1
            new_lines.append('    size_t current_header_field_len; /* 8 bytes - hot: header field length */\n')
            new_lines.append('    /* Cache line 1 total: 64 bytes (perfect fit) */\n')
        # Remove current_header_field_len from cache line 4
        elif 'size_t current_header_field_len;' in line and 'currentheaderFieldlength' in line:
            i += 1
            continue
        # Adjust padding comment for cache line 4
        elif 'int _padding3[14]' in line:
            new_lines.append('    int _padding3[15];               /* 60 bytes - padding to 64 bytes */\n')
        else:
            new_lines.append(line)

        i += 1

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print("✓ Optimized uvhttp_connection_t cache layout")
    print("  - Moved current_header_field_len to cache line 1")
    print("  - This field is accessed during HTTP parsing (hot path)")

if __name__ == "__main__":
    optimize_connection_cache()