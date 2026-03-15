#!/usr/bin/env python3
"""
Optimize strlen calls in loops in uvhttp_protocol_upgrade.c
"""
import sys

def optimize_protocol_upgrade():
    """Cache strlen result in loop condition"""
    file_path = "/home/zhaodi-chen/project/uvhttp/src/uvhttp_protocol_upgrade.c"

    with open(file_path, 'r') as f:
        content = f.read()

    # Replace strlen in loop conditions
    # First loop
    old_pattern1 = '''for (size_t i = 0; i < strlen(protocol_name) && i < sizeof(info->name) - 1;
         i++) {
        info->name[i] = (char)tolower((unsigned char)protocol_name[i]);
    }'''

    new_pattern1 = '''size_t protocol_name_len = strlen(protocol_name);
    for (size_t i = 0; i < protocol_name_len && i < sizeof(info->name) - 1; i++) {
        info->name[i] = (char)tolower((unsigned char)protocol_name[i]);
    }'''

    # Second loop
    old_pattern2 = '''for (size_t i = 0;
             i < strlen(upgrade_header) && i < sizeof(info->upgrade_header) - 1;
             i++) {
        info->upgrade_header[i] =
            (char)tolower((unsigned char)upgrade_header[i]);
    }'''

    new_pattern2 = '''size_t upgrade_header_len = strlen(upgrade_header);
    for (size_t i = 0; i < upgrade_header_len && i < sizeof(info->upgrade_header) - 1; i++) {
        info->upgrade_header[i] =
            (char)tolower((unsigned char)upgrade_header[i]);
    }'''

    content = content.replace(old_pattern1, new_pattern1)
    content = content.replace(old_pattern2, new_pattern2)

    # Write back
    with open(file_path, 'w') as f:
        f.write(content)

    print("✓ Optimized strlen calls in loops (uvhttp_protocol_upgrade.c)")
    print("  - Cached protocol_name length")
    print("  - Cached upgrade_header length")

if __name__ == "__main__":
    optimize_protocol_upgrade()