#!/usr/bin/env python3
"""
Optimize router to cache static_prefix_len and avoid repeated strlen() calls
"""
import sys

def optimize_router():
    file_path = "/home/zhaodi-chen/project/uvhttp/src/uvhttp_router.c"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # Replace strlen(router->static_prefix) with router->static_prefix_len
        if 'size_t prefix_len = strlen(router->static_prefix);' in line:
            new_lines.append('            size_t prefix_len = router->static_prefix_len;\n')
            print(f"✓ Replaced strlen call at line {i+1}")
        # Cache the length when setting static_prefix
        elif line.strip() == 'memcpy(router->static_prefix, prefix_path, prefix_len + 1);':
            new_lines.append(line)
            # Add cache length after memcpy
            new_lines.append('    router->static_prefix_len = prefix_len;\n')
            print(f"✓ Added cache after memcpy at line {i+2}")
        # Reset to 0 when freeing
        elif line.strip() == 'uvhttp_free(router->static_prefix);' and 'router->static_prefix = NULL;' in lines[i+1]:
            new_lines.append(line)
            new_lines.append(lines[i+1])
            # Reset length to 0
            new_lines.append('    router->static_prefix_len = 0;\n')
            print(f"✓ Reset length after free at line {i+3}")
            i += 1
        else:
            new_lines.append(line)

        i += 1

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print("✓ Optimized router to cache static_prefix_len")

if __name__ == "__main__":
    optimize_router()