# UVHTTP Embedding Validation Checklist

> This checklist is for embedders: developers who integrate uvhttp into their own project as a submodule.
> Run every item after each significant release or embedding.

---

## 1. Build integration

- [x] The project clones as a submodule: `git clone --recurse-submodules`
- [x] The project builds: `mkdir build && cd build && cmake .. && make`
- [x] An embedder can integrate it with CMake `add_subdirectory`
- [x] An embedder can trim features: `cmake .. -DBUILD_WITH_WEBSOCKET=OFF`
- [ ] Every feature-switch combination builds (see [BUILD_CONFIGURATION_MATRIX.md](guide/BUILD_CONFIGURATION_MATRIX.md))

## 2. Method enum mapping

- [ ] All HTTP methods (GET/POST/PUT/DELETE/HEAD/OPTIONS/PATCH) register and match correctly in the router
- [ ] Routes registered with the `UVHTTP_ANY` method in a `uvhttp_server_listen_routes()` route table match every request method
- [ ] Routes registered with `uvhttp_router_add_route()` (implicit UVHTTP_ANY) match every request method
- [ ] Request methods sent by the embedder's HTTP client library (such as undici or fetch) parse correctly

## 3. Standards compatibility

- [ ] gzip output with `Content-Encoding: gzip` decodes with standard `gunzip`
- [ ] The gzip stream magic bytes are `0x1f8b` (not zlib's `0x78`)
- [ ] WebSocket handshake header names are case-insensitive (`Sec-WebSocket-Key:` and `sec-websocket-key:` are both accepted)
- [ ] HTTP header names are case-insensitive (`Content-Type:` and `content-type:` are both accepted)
- [ ] `Connection: Upgrade`, `connection: upgrade` and `CONNECTION: UPGRADE` are all recognized
- [ ] Control frames (PING/PONG/CLOSE) payload length check (RFC 6455 §5.5: ≤125 bytes)
- [ ] RSV bit validation (RFC 6455 §5.2: with no negotiated extension, a non-zero RSV closes the connection)

## 4. Static file serving

- [ ] The root path `/` returns the index file (for example index.html)
- [ ] The subpath `/static/file.txt` returns the file
- [ ] A missing path returns 404 without crashing
- [ ] An embedder-configured index_file works with or without a leading `/`
- [ ] Large files (>1MB) go through the sendfile path without memory blowup

## 5. Processing pipeline

- [ ] A matching route calls its handler
- [ ] No route matches but static serving hits, so the static handler runs
- [ ] No route matches and static serving misses: if `server->handler` is set, call that handler instead of a hard-coded 404
- [ ] No route matches, static misses and no handler is set: return 404
- [ ] An embedder handler can send a response normally (set_status, set_header, set_body, send)

## 6. Memory safety

- [ ] The embedder's project runs under ASan with no leaks
- [ ] The embedder's project runs under UBSan with no undefined behavior
- [ ] Long runs (>1 hour) show no RSS growth
- [ ] High concurrency (100 connections) produces no socket errors
- [ ] `make verify-memory-safety` passes

## 7. Multiple instances and isolation

- [ ] Several uvhttp_server_t instances in one process work
- [ ] Each instance has its own gzip cache (no static global conflicts)
- [ ] Each instance has its own router
- [ ] Stopping one instance does not affect the others

## 8. Feature combination validation

| Feature combination | Verified |
|---------|------|
| Plain HTTP | [ ] |
| HTTP + WebSocket | [ ] |
| HTTP + static files | [ ] |
| HTTP + compression | [ ] |
| HTTP + TLS | [ ] |
| HTTP + WebSocket + TLS | [ ] |
| HTTP + router cache (ROUTER_CACHE=1) | [ ] |
| All features on | [ ] |

## 9. Embedder onboarding checklist (new embedders)

When adding uvhttp as a submodule for the first time, verify these steps.
The full integration guide is [EMBEDDING_GUIDE.md](guide/EMBEDDING_GUIDE.md),
and a runnable standalone example is [examples/embedding/](https://github.com/adam-ikari/uvhttp/tree/main/examples/embedding).

1. [ ] Add `add_subdirectory` or `FetchContent` to the embedder's `CMakeLists.txt`
2. [ ] Build the embedding project and confirm the uvhttp headers are reachable
3. [ ] Create a minimal HTTP server (~20 lines; `examples/embedding/main.c` is a direct reference) and confirm a `hello world` response
4. [ ] Run uvhttp's test suite: `cd <uvhttp>/build && ctest --output-on-failure`
5. [ ] Run ASan on the embedding project: `cmake -DENABLE_ASAN=ON .. && make && ./your-server`
6. [ ] Send a complete HTTP request and verify the response

---

## Version history

| Version | Date | Change |
|------|------|------|
| 1.0 | 2026-08-20 | Initial version, based on embedding uvhttp in qwrt |
| 1.1 | 2026-08-25 | Second validation round: add_subdirectory integration, CMake dependency visibility fix |
| 1.2 | 2026-08-27 | New embedder onboarding: standalone `examples/embedding/` example and full `EMBEDDING_GUIDE.md` |
