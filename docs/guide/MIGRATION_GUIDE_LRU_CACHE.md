# LRU Cache Migration Guide

## Overview

UVHTTP 2.3.0 replaces the LRU Cache API. This page lists the removed calls and what to do with each.

## Breaking Changes

### Removed Functions

The following functions have been removed:

1. `uvhttp_lru_cache_set_eviction_mode()` - Eviction mode selection
2. `uvhttp_lru_cache_init_task_queue()` - Task queue initialization
3. `uvhttp_lru_cache_schedule_eviction()` - Schedule eviction
4. `uvhttp_lru_cache_stop_task_queue()` - Stop task queue
5. `uvhttp_lru_cache_perform_eviction()` - Perform eviction

### Removed Features

- LFU eviction mode
- Hybrid LRU/LFU eviction
- Dual thresholds at 80% and 95%
- An async eviction task queue built on uv_async_t

## Migration Steps

### Step 1: Remove Eviction Mode Configuration

**Before:**
```c
uvhttp_lru_cache_set_eviction_mode(cache, UVHTTP_CACHE_EVICTION_MODE_LFU);
```

**After:**
```c
// No action needed - LRU is now the only eviction mode
```

### Step 2: Remove Task Queue Initialization

**Before:**
```c
uvhttp_lru_cache_init_task_queue(cache, loop);
```

**After:**
```c
// No action needed - task queue mechanism removed
```

### Step 3: Remove Eviction Scheduling

**Before:**
```c
uvhttp_lru_cache_schedule_eviction(cache);
```

**After:**
```c
// No action needed - eviction is now synchronous and automatic
```

### Step 4: Remove Task Queue Cleanup

**Before:**
```c
uvhttp_lru_cache_stop_task_queue(cache);
```

**After:**
```c
// No action needed - no task queue to stop
```

## New Behavior

### Single Threshold Eviction

The cache now uses a single threshold (90%) instead of dual thresholds (80%/95%).

**Before:**
- Eviction starts at 80% capacity
- Aggressive eviction at 95% capacity

**After:**
- Eviction starts at 90% capacity
- Batch size: 2 entries (configurable via `uvhttp_lru_cache_set_batch_eviction_size()`)

### Synchronous Eviction

Eviction now runs synchronously in the request path. There is no task queue and no scheduling overhead.

**Before:**
```c
// Async eviction via task queue
uvhttp_lru_cache_schedule_eviction(cache);
```

**After:**
```c
// Automatic synchronous eviction
// No manual scheduling needed
```

### Simplified Configuration

**Before:**
```c
uvhttp_lru_cache_t* cache = uvhttp_lru_cache_create(1000, 3600);
uvhttp_lru_cache_set_eviction_mode(cache, UVHTTP_CACHE_EVICTION_MODE_LRU);
uvhttp_lru_cache_init_task_queue(cache, loop);
```

**After:**
```c
cache_manager_t* cache = NULL;
uvhttp_lru_cache_create(1024 * 1024, 1000, 3600, &cache);
// Cache is ready to use immediately
```

## Configuration Options

### Batch Eviction Size

You can configure the number of entries to evict per batch:

```c
// Set batch eviction size (default: 2)
uvhttp_lru_cache_set_batch_eviction_size(cache, 5);
```

### Cache TTL

Set the time-to-live for cache entries:

```c
// Set cache TTL (default: 3600 seconds)
uvhttp_lru_cache_set_cache_ttl(cache, 7200);
```

## Performance Impact

### Memory Savings

- Instance overhead drops by 132 bytes per cache instance
- Entry overhead drops by 4 bytes per cache entry
- Code size drops by 200 lines

### Performance Improvements

- No libuv dependency, so no async handles
- Simpler eviction logic makes decisions faster
- No task queue scheduling

## Testing

Update your tests to remove calls to removed functions:

```c
// Remove these test cases
TEST(LRUCacheTest, EvictionMode) { /* ... */ }
TEST(LRUCacheTest, TaskQueue) { /* ... */ }

// Add new test cases
TEST(LRUCacheTest, BatchEvictionSize) { /* ... */ }
TEST(LRUCacheTest, SingleThresholdEviction) { /* ... */ }
```

## Compatibility Matrix

| UVHTTP Version | Eviction Modes | Task Queue | Migration Required |
|----------------|----------------|------------|-------------------|
| 2.2.x | LRU, LFU, Hybrid | Yes | - |
| 2.3.0 | LRU only | No | Yes |

## Rollback Plan

If you need to rollback to UVHTTP 2.2.x:

1. Restore removed function calls
2. Re-add eviction mode configuration
3. Re-add task queue initialization
4. Update version dependency

## Support

If you encounter issues during migration:

1. Check the [API Reference](../api/API_REFERENCE.md)
2. Review the [Examples](https://github.com/adam-ikari/uvhttp/tree/main/examples)
3. Open an issue on [GitHub](https://github.com/adam-ikari/uvhttp/issues)

## Summary

The simplified API removes eviction mode selection and the task queue. Cache instances and entries take less memory, and there are fewer code paths to maintain.

For most callers the migration is removing calls to the removed functions.