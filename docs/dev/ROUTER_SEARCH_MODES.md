# uvhttp Router Search Mode Configuration Guide

## Overview

uvhttp provides three router search modes:

- **Mode 0**: pure linear search - suitable for small-scale applications
- **Mode 1**: pure hash search - suitable for medium-scale applications
- **Mode 2**: hybrid strategy - suitable for large-scale high-concurrency applications (default)

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
- Use `SEARCH_MODE=0` to reduce memory usage
- Disable cache optimization to save 4KB+ of memory

### Performance Optimization
- Use `SEARCH_MODE=2` for the best lookup performance
- Enable statistics to monitor performance
- Use `-O3 -march=native` for optimized compilation

### Debug Friendly
- Use `SEARCH_MODE=0` to simplify debugging
- Disable complex cache logic

## Performance Benchmarks

Theoretical performance estimates (50 routes):

| Mode | Average Lookup Time | Memory Overhead | Cache Hit Rate |
|------|-------------|---------|-----------|
| Linear | ~1500ns | 0KB | N/A |
| Hash | ~200ns | ~4KB | N/A |
| Hybrid | ~180ns | ~8KB | ~60% |

## Best Practices

1. **Development phase**: use `SEARCH_MODE=0` for easier debugging
2. **Testing phase**: use `SEARCH_MODE=1` to verify functionality
3. **Production**: use `SEARCH_MODE=2` to ensure performance
4. **Embedded**: always use `SEARCH_MODE=0`
5. **High concurrency**: use `SEARCH_MODE=2` + statistics

---

*Based on the uvhttp HTTP server design, providing an optimal router lookup strategy for different scenarios.*
