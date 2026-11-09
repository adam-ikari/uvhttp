# uvhttp Router Search Mode Configuration Guide

## Overview

uvhttp offers three router search modes, selected at compile time.

Mode 0 searches routes linearly and suits small applications. Mode 1 keeps a pure hash table and suits medium-sized deployments. Mode 2 mixes a hash table with a hot-path cache, ships as the default, and targets large, high-concurrency deployments.

## Build Configuration

### Basic Usage

```bash
# Default hybrid strategy
gcc ...

# Pure linear search (embedded/small-scale applications)
gcc -DUVHTTP_ROUTER_SEARCH_MODE=0 ...

# Pure hash search (medium-scale applications)
gcc -DUVHTTP_ROUTER_SEARCH_MODE=1 ...

# Hybrid strategy (large-scale applications)
gcc -DUVHTTP_ROUTER_SEARCH_MODE=2 ...
```

### Related Configuration Options

```bash
# Disable route cache optimization
gcc -DUVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION=0 ...

# Enable statistics
gcc -DUVHTTP_ENABLE_ROUTER_CACHE_STATS=1 ...

# Enable dynamic adjustment
gcc -DUVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC=1 ...

# Enable performance monitoring
gcc -DUVHTTP_ENABLE_ROUTER_CACHE_MONITORING=1 ...
```

## Mode Comparison

| Mode | Memory Overhead | Lookup Performance | Use Case | Recommended Configuration |
|------|---------|---------|---------|---------|
| 0 - Linear | Minimal | O(n) | Embedded, <10 routes | `SEARCH_MODE=0` |
| 1 - Hash | Medium | O(1) | Medium scale, 10-50 routes | `SEARCH_MODE=1` |
| 2 - Hybrid | Larger | O(1) optimal | Large scale, >50 routes | `SEARCH_MODE=2` |

## Scenario Recommendations

### Embedded Devices
```bash
gcc -DUVHTTP_ROUTER_SEARCH_MODE=0 \
     -DUVHTTP_ENABLE_ROUTER_CACHE_OPTIMIZATION=0 \
     -Os -DCONFIG_SMALL_MEMORY
```

### Microservice Architecture
```bash
gcc -DUVHTTP_ROUTER_SEARCH_MODE=1 \
     -DUVHTTP_ENABLE_ROUTER_CACHE_STATS=1 \
     -O2
```

### High-Concurrency Servers
```bash
gcc -DUVHTTP_ROUTER_SEARCH_MODE=2 \
     -DUVHTTP_ENABLE_ROUTER_CACHE_STATS=1 \
     -DUVHTTP_ENABLE_ROUTER_CACHE_DYNAMIC=1 \
     -O3 -march=native
```

## Performance Tuning

### Memory Optimization

Use `SEARCH_MODE=0` to keep the footprint smallest. Disabling cache optimization saves 4KB+.

### Performance Optimization

Use `SEARCH_MODE=2` for the fastest lookup. Statistics show what the cache is doing. Compile with `-O3 -march=native` when you want the last bit of speed.

### Debug Friendly

Use `SEARCH_MODE=0` when you want the simplest control flow. Leave the cache logic out.

## Performance Benchmarks

Theoretical performance estimates (50 routes):

| Mode | Average Lookup Time | Memory Overhead | Cache Hit Rate |
|------|-------------|---------|-----------|
| Linear | ~1500ns | 0KB | N/A |
| Hash | ~200ns | ~4KB | N/A |
| Hybrid | ~180ns | ~8KB | ~60% |

## Best Practices

1. During development, use `SEARCH_MODE=0`; the control flow is easiest to follow.
2. In testing, switch to `SEARCH_MODE=1` to exercise the hash path.
3. In production, keep `SEARCH_MODE=2`.
4. On embedded targets, stay on `SEARCH_MODE=0`.
5. Under high concurrency, run `SEARCH_MODE=2` with statistics enabled.

---

*Each mode targets a different deployment shape. Pick the one that matches your route count and memory budget.*
