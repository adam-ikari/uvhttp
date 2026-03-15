#!/usr/bin/env python3
"""
Optimize branch prediction in uvhttp_router_find_handler
"""
import sys

def optimize_router_branches():
    """Add branch prediction hints to hot path"""
    file_path = "/home/zhaodi-chen/project/uvhttp/src/uvhttp_router.c"

    with open(file_path, 'r') as f:
        content = f.read()

    # Optimize static router check (likely to have static routes)
    content = content.replace(
        'if (router->static_prefix && router->static_context) {',
        'if (UVHTTP_LIKELY(router->static_prefix && router->static_context)) {'
    )

    # Optimize use_trie check (most routers will use trie)
    content = content.replace(
        'if (router->use_trie) {',
        'if (UVHTTP_LIKELY(router->use_trie)) {'
    )

    # Optimize NULL checks (unlikely to be NULL)
    content = content.replace(
        'if (!router || !path || !method) {',
        'if (UVHTTP_UNLIKELY(!router || !path || !method)) {'
    )

    # Write back
    with open(file_path, 'w') as f:
        f.write(content)

    print("✓ Added branch prediction hints to uvhttp_router.c")
    print("  - Static router check: LIKELY")
    print("  - Trie mode check: LIKELY")
    print("  - NULL checks: UNLIKELY")

if __name__ == "__main__":
    optimize_router_branches()