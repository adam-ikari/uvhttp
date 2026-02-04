# Router Cache Optimization

## Overview

This document describes the optimization plan for the router cache system to improve routing performance, especially for applications with many routes.

## Current Implementation Analysis

### Architecture

The router cache uses a **hierarchical caching strategy**:

1. **Hot Path Cache**: Stores the 16 most frequently used routes (CPU cache optimized)
2. **Hash Table**: Fast lookup for all routes (including cold paths)

### Performance Issues

| Issue | Impact | Severity |
|-------|--------|----------|
| Fixed hash table size (256) | High collision rate for >100 routes | High |
| Small hot path cache (16) | Misses for frequently accessed routes | Medium |
| Access counter overflow | Incorrect eviction decisions | Low |
| Linked list collision handling | Poor cache locality | High |

### Benchmark Results

Current performance (from `benchmark_router_comparison.c`):

| Test | Routes | Mode | Avg Time | Ops/sec |
|------|--------|------|----------|---------|
| Small Router | 10 | Array | ~100 ns | ~10M |
| Medium Router | 80 | Array | ~150 ns | ~6.7M |
| Large Router | 150 | Trie | ~200 ns | ~5M |
| Parameter Router | 50 | Trie | ~180 ns | ~5.6M |

## Optimization Plan

### Phase 1: Hash Table Optimization

**Goal**: Reduce hash collision rate and improve lookup speed

#### 1.1 Dynamic Hash Table Sizing

```c
/* Auto-scaling hash table based on route count */
#define UVHTTP_ROUTER_HASH_BASE_SIZE 256
#define UVHTTP_ROUTER_HASH_LOAD_FACTOR 0.75
#define UVHTTP_ROUTER_HASH_MAX_SIZE 4096

typedef struct {
    route_hash_entry_t** table;
    size_t size;          /* Current table size */
    size_t count;         /* Number of entries */
    size_t threshold;     /* Resize threshold (size * load_factor) */
} hash_table_t;
```

**Benefits**:
- Reduces collision rate from ~30% to <5%
- Improves lookup performance by 2-3x
- Maintains memory efficiency with load factor

#### 1.2 Open Addressing Collision Resolution

```c
/* Replace linked list with open addressing */
typedef struct {
    char path[UVHTTP_MAX_ROUTE_PATH_LEN];
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
    uint32_t access_count;
    uint8_t distance;     /* Probe distance */
} hash_entry_t;

/* Linear probing with Robin Hood optimization */
static inline uint32_t find_slot(hash_table_t* table, const char* path, 
                                uvhttp_method_t method) {
    uint32_t hash = route_hash(path);
    uint32_t index = hash % table->size;
    uint32_t distance = 0;
    
    while (1) {
        hash_entry_t* entry = &table->entries[index];
        
        if (entry->distance == 0) {
            return index;  /* Empty slot */
        }
        
        if (entry->distance == distance && 
            strcmp(entry->path, path) == 0 && 
            entry->method == method) {
            return index;  /* Found match */
        }
        
        if (entry->distance < distance) {
            return index;  /* Robin Hood: swap with shorter distance */
        }
        
        index = (index + 1) % table->size;
        distance++;
    }
}
```

**Benefits**:
- Better cache locality (contiguous memory)
- No pointer chasing
- 30-50% faster than linked list

### Phase 2: Hot Path Cache Enhancement

**Goal**: Increase hit rate for frequently accessed routes

#### 2.1 Adaptive Hot Path Size

```c
/* Dynamically adjust hot path cache size based on access pattern */
#define UVHTTP_ROUTER_HOT_MIN_SIZE 16
#define UVHTTP_ROUTER_HOT_MAX_SIZE 64
#define UVHTTP_ROUTER_HOT_HIT_THRESHOLD 0.8  /* 80% hit rate */

typedef struct {
    hot_route_entry_t* routes;
    size_t capacity;      /* Current capacity */
    size_t count;         /* Number of entries */
    size_t hits;          /* Hit counter */
    size_t misses;        /* Miss counter */
} hot_cache_t;

/* Auto-adjust capacity based on hit rate */
static void adjust_hot_cache_size(hot_cache_t* cache) {
    double hit_rate = (double)cache->hits / (cache->hits + cache->misses);
    
    if (hit_rate < UVHTTP_ROUTER_HOT_HIT_THRESHOLD && 
        cache->capacity < UVHTTP_ROUTER_HOT_MAX_SIZE) {
        /* Increase size */
        size_t new_capacity = cache->capacity * 2;
        hot_route_entry_t* new_routes = uvhttp_realloc(
            cache->routes, new_capacity * sizeof(hot_route_entry_t));
        if (new_routes) {
            cache->routes = new_routes;
            cache->capacity = new_capacity;
        }
    } else if (hit_rate > 0.95 && cache->capacity > UVHTTP_ROUTER_HOT_MIN_SIZE) {
        /* Decrease size */
        size_t new_capacity = cache->capacity / 2;
        hot_route_entry_t* new_routes = uvhttp_realloc(
            cache->routes, new_capacity * sizeof(hot_route_entry_t));
        if (new_routes) {
            cache->routes = new_routes;
            cache->capacity = new_capacity;
            cache->count = new_capacity;  /* Evict excess entries */
        }
    }
}
```

**Benefits**:
- Adaptive to workload patterns
- Reduces memory waste for small applications
- Improves hit rate from ~60% to ~85%

#### 2.2 LFU Eviction Policy

```c
/* Replace LRU with LFU for better hot route detection */
typedef struct {
    char path[UVHTTP_MAX_ROUTE_PATH_LEN];
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
    uint32_t frequency;   /* Access frequency (exponential decay) */
    uint64_t last_access; /* Last access timestamp */
} hot_route_entry_t;

/* Update frequency with exponential decay */
static inline void update_frequency(hot_route_entry_t* entry, uint64_t now) {
    uint64_t time_delta = now - entry->last_access;
    
    /* Decay factor: 10% per second */
    if (time_delta > 1000000000ULL) {
        entry->frequency = entry->frequency * 9 / 10;
    }
    
    entry->frequency++;
    entry->last_access = now;
}
```

**Benefits**:
- Better detection of truly hot routes
- Handles burst traffic patterns
- 15-25% higher hit rate than LRU

### Phase 3: Access Counter Optimization

**Goal**: Prevent overflow and improve accuracy

#### 3.1 Saturating Counter

```c
/* Use saturating counter to prevent overflow */
#define UVHTTP_ACCESS_COUNTER_MAX 0xFFFFFFFF

typedef struct {
    char path[UVHTTP_MAX_ROUTE_PATH_LEN];
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
    uint32_t access_count;  /* Saturating counter */
} route_entry_t;

static inline void increment_access(route_entry_t* entry) {
    if (entry->access_count < UVHTTP_ACCESS_COUNTER_MAX) {
        entry->access_count++;
    }
}
```

**Benefits**:
- No overflow risk
- Consistent eviction decisions
- Minimal performance impact

### Phase 4: Memory Layout Optimization

**Goal**: Improve cache locality and reduce memory footprint

#### 4.1 Compact Entry Structure

```c
/* Optimize memory layout for cache line efficiency */
typedef struct __attribute__((packed)) {
    char path[UVHTTP_MAX_ROUTE_PATH_LEN];  /* 256 bytes */
    uvhttp_method_t method;                /* 4 bytes */
    uvhttp_request_handler_t handler;      /* 8 bytes */
    uint32_t access_count;                 /* 4 bytes */
    uint8_t distance;                      /* 1 byte */
    uint8_t _padding[3];                   /* 3 bytes */
} hash_entry_t;  /* Total: 276 bytes → fits in 5 cache lines (320 bytes) */
```

**Benefits**:
- Better cache line utilization
- Reduced memory fragmentation
- 10-15% faster memory access

## Expected Performance Improvements

| Metric | Current | Optimized | Improvement |
|--------|---------|-----------|-------------|
| Hash collision rate | ~30% | <5% | 6x better |
| Hot cache hit rate | ~60% | ~85% | 1.4x better |
| Lookup time (100 routes) | ~150 ns | ~80 ns | 1.9x faster |
| Lookup time (500 routes) | ~200 ns | ~90 ns | 2.2x faster |
| Memory overhead | +50% | +20% | 2.5x less |

## Implementation Plan

### Milestone 1: Hash Table Optimization (Week 1)
- [ ] Implement dynamic hash table sizing
- [ ] Replace linked list with open addressing
- [ ] Add hash table resize logic
- [ ] Write unit tests for hash table
- [ ] Benchmark hash table performance

### Milestone 2: Hot Path Cache Enhancement (Week 2)
- [ ] Implement adaptive hot path sizing
- [ ] Replace LRU with LFU eviction
- [ ] Add hit rate tracking
- [ ] Write unit tests for hot cache
- [ ] Benchmark hot cache performance

### Milestone 3: Access Counter Optimization (Week 3)
- [ ] Implement saturating counter
- [ ] Update all counter usage
- [ ] Add overflow tests
- [ ] Verify eviction correctness

### Milestone 4: Memory Layout Optimization (Week 4)
- [ ] Optimize entry structure layout
- [ ] Add cache line alignment
- [ ] Benchmark memory access patterns
- [ ] Verify no regressions

### Milestone 5: Integration and Testing (Week 5)
- [ ] Integrate all optimizations
- [ ] Run full benchmark suite
- [ ] Performance regression testing
- [ ] Memory leak testing
- [ ] Documentation updates

## Migration Guide

### Breaking Changes

None. The optimization is fully backward compatible.

### API Changes

No API changes. All optimizations are internal implementation details.

### Configuration Changes

New configuration options in `uvhttp_constants.h`:

```c
/* Hash table configuration */
#define UVHTTP_ROUTER_HASH_BASE_SIZE 256
#define UVHTTP_ROUTER_HASH_LOAD_FACTOR 0.75
#define UVHTTP_ROUTER_HASH_MAX_SIZE 4096

/* Hot cache configuration */
#define UVHTTP_ROUTER_HOT_MIN_SIZE 16
#define UVHTTP_ROUTER_HOT_MAX_SIZE 64
#define UVHTTP_ROUTER_HOT_HIT_THRESHOLD 0.8

/* Access counter configuration */
#define UVHTTP_ACCESS_COUNTER_MAX 0xFFFFFFFF
```

### Testing Recommendations

1. Run `benchmark_router_comparison` to verify performance improvements
2. Run all existing unit tests to ensure no regressions
3. Test with your specific route patterns to validate improvements
4. Monitor memory usage in production

## Performance Monitoring

### Metrics to Track

- Hash collision rate: `router->hash_table->collisions / router->hash_table->lookups`
- Hot cache hit rate: `hot_cache->hits / (hot_cache->hits + hot_cache->misses)`
- Average lookup time: Benchmark with `benchmark_router_comparison.c`
- Memory usage: Monitor via system tools (valgrind, etc.)

### Expected Metrics

| Application Type | Hash Collision Rate | Hot Cache Hit Rate | Avg Lookup Time |
|------------------|-------------------|-------------------|-----------------|
| Small (<50 routes) | <2% | ~90% | <50 ns |
| Medium (50-200 routes) | <5% | ~85% | <80 ns |
| Large (>200 routes) | <10% | ~80% | <100 ns |

## References

- Original router implementation: `src/uvhttp_router.c`, `src/uvhttp_router_cache.c`
- Benchmark suite: `benchmark/benchmark_router*.c`
- Configuration: `include/uvhttp_constants.h`
- API reference: `include/uvhttp_router.h`