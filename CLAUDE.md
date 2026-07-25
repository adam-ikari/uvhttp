# UVHTTP Project Context

## Project Overview
A C99 HTTP/1.1 & WebSocket server library built on libuv. Key differentiator:
ASan/UBSan-verified memory safety (91/91 tests, zero findings).

## Current Status
- **Version**: 2.5.0
- **Tests**: 91/91 pass (Debug, Release, ASan, UBSan)
- **Coverage**: ~50% (target: 80%)
- **Build**: CMake, C99, -Werror, -fstack-protector-strong
- **CI**: ci-pr.yml (PR gate), ci-nightly.yml, ci-fuzz.yml, deploy-docs.yml

## Architecture
- **src/**: 17 .c files (server, router, connection, request, response, websocket, static, tls, config, error, context, lru_cache, protocol_upgrade, utils, version)
- **include/**: 28 .h files (public API)
- **test/**: 91 unit tests (Google Test) + 20 integration tests + 1 fuzz harness
- **deps/**: Vendored as git submodules (libuv, llhttp, mbedtls, xxhash, cjson, uthash, mimalloc, zlib, googletest)
- **docs/**: VitePress site, 14 SDD spec documents

## Key Commands
```bash
make -f GNUmakefile test          # Run tests
make -f GNUmakefile verify-memory-safety  # ASan + UBSan gate
cd docs && npm run docs:build     # Build documentation site
cmake -B build && cmake --build build -j$(nproc)  # Build
```

## Development Workflow
1. Branch from `main`: `git checkout -b <type>/<description>`
2. Implement changes
3. Run tests: `make -f GNUmakefile test`
4. Verify memory safety: `make -f GNUmakefile verify-memory-safety`
5. Create PR: `gh pr create --base main`
6. Merge after review: `gh pr merge <number> --merge --delete-branch`

## Documentation Standards
- No AI flavor (hype adjectives, filler transitions, rhetorical questions)
- Terse, factual, technical voice
- Claims must be verifiable from code or test results
- Chinese docs must match English docs in content and accuracy