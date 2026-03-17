#!/usr/bin/env python3
"""
Analyze optimization opportunities in UVHTTP
"""
import re

def analyze_hot_paths():
    """Analyze hot paths for optimization opportunities"""

    opportunities = []

    # 1. Check for repeated strlen() calls
    print("=== 检查重复的 strlen() 调用 ===")
    strlen_patterns = {
        'router.c': ['strlen(router->static_prefix)', 'strlen(path)'],
        'request.c': ['strlen(header_name)', 'strlen(header_value)'],
        'response.c': ['strlen(status_text)', 'strlen(header_name)'],
    }

    for file, patterns in strlen_patterns.items():
        for pattern in patterns:
            count = count_pattern_in_file(f'src/uvhttp_{file}', pattern)
            if count > 1:
                opportunities.append({
                    'type': 'Repeated strlen',
                    'file': file,
                    'pattern': pattern,
                    'count': count,
                    'suggestion': f'Cache length of {pattern}'
                })
                print(f"  ✓ {file}: {pattern} called {count} times")

    # 2. Check for string comparisons that could use hash
    print("\n=== 检查字符串比较 ===")
    comparison_patterns = {
        'router.c': ['strcmp(segment, ', 'strncmp(path, '],
        'request.c': ['strcmp(method, ', 'strcmp(header_name, '],
    }

    for file, patterns in comparison_patterns.items():
        for pattern in patterns:
            count = count_pattern_in_file(f'src/uvhttp_{file}', pattern)
            if count > 0:
                print(f"  ✓ {file}: {pattern} found {count} times")

    # 3. Check for memory allocations in hot paths
    print("\n=== 检查热路径内存分配 ===")
    allocation_patterns = {
        'router.c': ['uvhttp_alloc('],
        'request.c': ['uvhttp_alloc('],
        'response.c': ['uvhttp_alloc('],
    }

    for file, patterns in allocation_patterns.items():
        for pattern in patterns:
            count = count_pattern_in_file(f'src/uvhttp_{file}', pattern)
            if count > 0:
                print(f"  ✓ {file}: {pattern} found {count} times")

    # 4. Check for unnecessary copies
    print("\n=== 检查不必要的复制 ===")
    copy_patterns = {
        'response.c': ['memcpy(', 'strcpy('],
        'request.c': ['memcpy(', 'strcpy('],
    }

    for file, patterns in copy_patterns.items():
        for pattern in patterns:
            count = count_pattern_in_file(f'src/uvhttp_{file}', pattern)
            if count > 0:
                print(f"  ✓ {file}: {pattern} found {count} times")

    return opportunities

def count_pattern_in_file(filepath, pattern):
    """Count pattern occurrences in file"""
    try:
        with open(f'/home/zhaodi-chen/project/uvhttp/{filepath}', 'r') as f:
            content = f.read()
            return content.count(pattern)
    except FileNotFoundError:
        return 0

if __name__ == "__main__":
    opportunities = analyze_hot_paths()

    print("\n=== 优化建议 ===")
    if opportunities:
        for opp in opportunities:
            print(f"\n类型: {opp['type']}")
            print(f"  文件: {opp['file']}")
            print(f"  模式: {opp['pattern']}")
            print(f"  次数: {opp['count']}")
            print(f"  建议: {opp['suggestion']}")
    else:
        print("未发现明显的优化机会")
