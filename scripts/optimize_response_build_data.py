#!/usr/bin/env python3
"""
Optimize uvhttp_response_build_data to reduce memory allocations
"""
import sys

def optimize_response_build_data():
    """Optimize the function to calculate required size upfront and allocate once"""
    file_path = "/home/zhaodi-chen/project/uvhttp/src/uvhttp_response.c"

    with open(file_path, 'r') as f:
        lines = f.readlines()

    # Find the function start
    start_line = None
    for i, line in enumerate(lines):
        if 'uvhttp_error_t uvhttp_response_build_data' in line:
            start_line = i
            break

    if start_line is None:
        print("Error: Function not found")
        return

    # Find the function end (next function or end of file)
    end_line = None
    for i in range(start_line + 1, len(lines)):
        if lines[i].strip().startswith('uvhttp_error_t ') and 'uvhttp_response_build_data' not in lines[i]:
            end_line = i
            break
        if lines[i].strip().startswith('static ') and 'uvhttp_error_t ' in lines[i]:
            end_line = i
            break

    if end_line is None:
        end_line = len(lines)

    # Find the line with "build complete HTTP response"
    build_comment_line = None
    for i in range(start_line, end_line):
        if 'build complete HTTP response' in lines[i]:
            build_comment_line = i
            break

    if build_comment_line is None:
        print("Error: Build comment not found")
        return

    # Find the line with "allocate complete response data"
    alloc_line = None
    for i in range(build_comment_line, end_line):
        if 'char* response_data' in lines[i] and 'uvhttp_alloc(total_size' in lines[i+1]:
            alloc_line = i
            break

    if alloc_line is None:
        print("Error: Allocation line not found")
        return

    # Create optimized version
    new_lines = lines[:build_comment_line]

    # Add optimized code
    new_lines.append('    /* build complete HTTP response - optimized single allocation */\n')
    new_lines.append('    /* optimization: calculate headers size first, allocate once */\n')
    new_lines.append('    size_t headers_size = 0;\n')
    new_lines.append('    build_response_headers(response, NULL, &headers_size);\n')
    new_lines.append('\n')
    new_lines.append('    /* calculate total size */\n')
    new_lines.append('    size_t total_size = headers_size + response->body_length;\n')
    new_lines.append('    if (total_size > UVHTTP_MAX_BODY_SIZE * 2) {\n')
    new_lines.append('        return UVHTTP_ERROR_OUT_OF_MEMORY;\n')
    new_lines.append('    }\n')
    new_lines.append('\n')
    new_lines.append('    /* allocate complete response data */\n')
    new_lines.append('    char* response_data = uvhttp_alloc(total_size + 1); /* +1 for null terminator */\n')
    new_lines.append('    if (!response_data) {\n')
    new_lines.append('        return UVHTTP_ERROR_OUT_OF_MEMORY;\n')
    new_lines.append('    }\n')
    new_lines.append('\n')
    new_lines.append('    /* build headers directly into response_data */\n')
    new_lines.append('    size_t headers_length = total_size;\n')
    new_lines.append('    build_response_headers(response, response_data, &headers_length);\n')
    new_lines.append('\n')
    new_lines.append('    /* copy body */\n')
    new_lines.append('    if (response->body && response->body_length > 0) {\n')
    new_lines.append('        memcpy(response_data + headers_length, response->body,\n')
    new_lines.append('               response->body_length);\n')
    new_lines.append('    }\n')
    new_lines.append('\n')
    new_lines.append('    /* null terminator */\n')
    new_lines.append('    response_data[total_size] = \'\\0\';\n')
    new_lines.append('\n')

    # Find the line after "null terminator"
    null_terminator_line = None
    for i in range(alloc_line, end_line):
        if 'response_data[total_size]' in lines[i] or 'null terminator' in lines[i].lower():
            null_terminator_line = i
            break

    if null_terminator_line is None:
        # Find the line that sets *out_data
        for i in range(alloc_line, end_line):
            if '*out_data = response_data' in lines[i]:
                null_terminator_line = i
                break

    if null_terminator_line is None:
        print("Error: Cannot find where to resume")
        return

    # Find the return statement
    return_line = None
    for i in range(null_terminator_line, end_line):
        if '*out_length = total_size' in lines[i]:
            return_line = i + 1
            break

    if return_line is None:
        return_line = null_terminator_line + 5

    # Add the rest of the function
    new_lines.extend(lines[return_line:])

    # Write back
    with open(file_path, 'w') as f:
        f.writelines(new_lines)

    print(f"✓ Optimized uvhttp_response_build_data (line {build_comment_line + 1} to {return_line + 1})")
    print("  Changes:")
    print("  - Calculate headers size first (no buffer)")
    print("  - Allocate once for complete response")
    print("  - Build headers directly into final buffer")

if __name__ == "__main__":
    optimize_response_build_data()
