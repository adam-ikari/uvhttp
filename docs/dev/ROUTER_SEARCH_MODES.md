# uvhttp Router Search Mode Configuration Guide

## Overview

Route lookup is selected at build time by one CMake option: `BUILD_WITH_ROUTER_CACHE`, default `OFF`. It defines `UVHTTP_FEATURE_ROUTER_CACHE` as 0 or 1, and exactly one implementation is compiled:

| Value | Source file | Route storage |
|-------|-------------|---------------|
| `OFF` (0) | `src/uvhttp_router.c` | Array of routes, migrating to a trie |
| `ON` (1) | `src/uvhttp_router_cache.c` | Open-addressing hash table |

There is no runtime switch and no separate search-mode macro.

## Build Configuration

```bash
# Default: array + trie
cmake -B build -DBUILD_WITH_ROUTER_CACHE=OFF ..

# Hash table implementation
cmake -B build -DBUILD_WITH_ROUTER_CACHE=ON ..
```

## How Routes Are Stored

### Array + Trie (`BUILD_WITH_ROUTER_CACHE=OFF`)

- `uvhttp_router_new` allocates `HYBRID_THRESHOLD` (100) `array_route_t` slots and a 64-node trie node pool.
- Routes are appended to the array while the router is in array mode. Lookup scans the array and compares path and method.
- A route containing `:` (path parameter), `array_route_count` reaching `HYBRID_THRESHOLD`, or `use_trie` already being set selects the trie path. On the first switch, `migrate_to_trie` copies every array route into the trie; the new route is then added there.
- Migration is all-or-nothing. If it fails, `use_trie` stays 0, existing routes keep working, and the route that triggered migration returns the error.
- Trie lookup walks path segments; a parameter segment matches any value.

### Hash Table (`BUILD_WITH_ROUTER_CACHE=ON`)

- All routes are stored in one hash table with open addressing and linear probing. Entries are keyed by path plus method; the hash is computed from the path with XXH64.
- The table starts at `UVHTTP_ROUTER_HASH_BASE_SIZE` (256) slots and doubles when the count reaches `UVHTTP_ROUTER_HASH_LOAD_FACTOR` (0.75) of capacity, capped at `UVHTTP_ROUTER_HASH_MAX_SIZE` (4096).
- Exact paths are matched by hash. Parameterized routes (`:`) are matched by a second linear pass over the table entries.
- `uvhttp_router_add_route_method` applies the same threshold test and sets `use_trie` at 100 routes or on the first parameterized route, keeping the public `uvhttp_router_t` fields consistent with the array implementation. This build has no trie; lookups stay in the hash table.

## Threshold and Measured Costs

`HYBRID_THRESHOLD` is a per-file constant (100 in both implementations), not a build option. To change the crossover, edit it in the source and re-run the benchmarks.

Timings recorded in `src/uvhttp_router.c` (1000 lookups, array vs trie):

| Routes | Array | Trie |
|--------|-------|------|
| 50 | 0.02 ms | 0.03 ms |
| 100 | 0.04 ms | 0.04 ms |
| 200 | 0.08 ms | 0.05 ms |
| 500 | 0.20 ms | 0.06 ms |

No equivalent timings are recorded for the hash table implementation.

## Choosing an Implementation

- Fewer than 100 static routes: leave the option `OFF`. The array scan is competitive (faster in the recorded 50-route measurement), and memory use is bounded by the preallocated array and node pool.
- 100 or more routes, or parameterized routes: `OFF` gives a trie whose lookup cost follows path depth; `ON` gives average O(1) exact-path lookup with a linear scan for parameterized routes.
- Memory: `OFF` preallocates 100 array slots plus a 64-node pool. `ON` starts with a 256-entry hash table, each entry holding a full 256-byte path buffer, so it uses more memory at low route counts and grows dynamically.
- Do not change the option per request; it is fixed for the whole build.
