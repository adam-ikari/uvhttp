#!/usr/bin/env python3
"""
Find hot path optimizations in UVHTTP
"""
import re

def find_hot_paths():
    """Find functions called frequently in hot paths"""
    hot_functions = [
        'uvhttp_router_find_handler',
        'uvhttp_router_match',
        'uvhttp_response_build_data',
        'uvhttp_response_send',
        'uvhttp_request_parse',
        'uvhttp_request_get_header',
        'uvhttp_request_get_body',
    ]

    print("=== 热路径函数分析 ===\n")

    for func in hot_functions:
        print(f"函数: {func}")

        # Find the file
        filepath = None
        for module in ['router', 'response', 'request']:
            test_path = f'/home/zhaodi-chen/project/uvhttp/src/uvhttp_{module}.c'
            try:
                with open(test_path, 'r') as f:
                    content = f.read()
                    if func in content:
                        filepath = test_path
                        break
            except FileNotFoundError:
                continue

        if not filepath:
            print("  未找到\n")
            continue

        with open(filepath, 'r') as f:
            lines = f.readlines()

        # Find function definition
        func_start = None
        for i, line in enumerate(lines):
            if func in line and ('{' in line or line.strip().endswith('{')):
                func_start = i
                break

        if func_start is None:
            print("  未找到函数定义\n")
            continue

        # Count potential optimizations
        strlen_count = 0
        strcmp_count = 0
        memcpy_count = 0
        alloc_count = 0
        loop_count = 0

        # Find function end (count braces)
        brace_count = 0
        in_function = False
        for i in range(func_start, len(lines)):
            line = lines[i]

            if '{' in line:
                brace_count += line.count('{')
                in_function = True
            if '}' in line:
                brace_count -= line.count('}')

            if in_function:
                if 'strlen(' in line:
                    strlen_count += 1
                if 'strcmp(' in line or 'strncmp(' in line:
                    strcmp_count += 1
                if 'memcpy(' in line:
                    memcpy_count += 1
                if 'uvhttp_alloc(' in line:
                    alloc_count += 1
                if 'while (' in line or 'for (' in line:
                    loop_count += 1

            if in_function and brace_count == 0:
                break

        print(f"  strlen(): {strlen_count}")
        print(f"  strcmp/strncmp: {strcmp_count}")
        print(f"  memcpy(): {memcpy_count}")
        print(f"  uvhttp_alloc(): {alloc_count}")
        print(f"  循环: {loop_count}")

        # Optimization suggestions
        suggestions = []
        if strlen_count > 2:
            suggestions.append(f"考虑缓存字符串长度（{strlen_count}次调用）")
        if strcmp_count > 3:
            suggestions.append(f"考虑使用哈希表优化字符串比较（{strcmp_count}次调用）")
        if alloc_count > 2:
            suggestions.append(f"考虑减少内存分配（{alloc_count}次分配）")

        if suggestions:
            print("  优化建议:")
            for s in suggestions:
                print(f"    - {s}")

        print()

if __name__ == "__main__":
    find_hot_paths()
