#!/usr/bin/env python3
"""
Add branch prediction macros to uvhttp_platform.h and optimize hot paths
"""
import sys

def add_branch_prediction():
    """Add likely/unlikely macros to platform header"""
    file_path = "/home/zhaodi-chen/project/uvhttp/include/uvhttp_platform.h"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    new_lines = []
    i = 0
    while i < len(lines):
        line = lines[i]

        # Add branch prediction macros before cache macros
        if 'CacheMacro definition' in line:
            new_lines.append(line)
            new_lines.append('\n')
            new_lines.append('/* ========== Branch Prediction Macros ========== */\n')
            new_lines.append('/* Optimize branch prediction for hot paths */\n')
            new_lines.append('#if defined(__GNUC__) || defined(__clang__)\n')
            new_lines.append('#    define UVHTTP_LIKELY(x)   __builtin_expect(!!(x), 1)\n')
            new_lines.append('#    define UVHTTP_UNLIKELY(x) __builtin_expect(!!(x), 0)\n')
            new_lines.append('#else\n')
            new_lines.append('#    define UVHTTP_LIKELY(x)   (x)\n')
            new_lines.append('#    define UVHTTP_UNLIKELY(x) (x)\n')
            new_lines.append('#endif\n')
            new_lines.append('\n')
        else:
            new_lines.append(line)

        i += 1

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print("✓ Added branch prediction macros to uvhttp_platform.h")

if __name__ == "__main__":
    add_branch_prediction()
