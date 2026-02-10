# LRU Cache Migration Guide

## Overview

This migration guide helps you update your code to use the simplified LRU Cache API introduced in UVHTTP 2.3.0.

## Breaking Changes

### Removed Functions

The following functions have been removed:

1. `uvhttp_lru_cache_set_eviction_mode()` - Eviction mode selection
2. `uvhttp_lru_cache_init_task_queue()` - Task queue initialization
3. `uvhttp_lru_cache_schedule_eviction()` - Schedule eviction
4. `uvhttp_lru_cache_stop_task_queue()` - Stop task queue
5. `uvhttp_lru_cache_perform_eviction()` - Perform eviction

### Removed Features

- **LFU Eviction Mode**: Least Frequently Used eviction
- **Hybrid Eviction Mode**: Combined LRU/LFU eviction
- **Dual-Threshold Eviction**: 80%/95% eviction thresholds
- **Task Queue Mechanism**: Async eviction via uv_async_t

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

Eviction is now performed synchronously in the request path, eliminating the task queue overhead.

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
uvhttp_lru_cache_t* cache = uvhttp_lru_cache_create(1000, 3600);
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

- **Instance overhead**: -132 bytes per cache instance
- **Entry overhead**: -4 bytes per cache entry
- **Code size**: -200 lines of code

### Performance Improvements

- **Zero libuv dependency**: No async handles or task queue
- **Simpler eviction logic**: Faster eviction decisions
- **Reduced overhead**: No task queue scheduling

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

1. Check the [API Reference](./api/API_REFERENCE.md)
2. Review the [Examples](../examples/)
3. Open an issue on [GitHub](https://github.com/adam-ikari/uvhttp/issues)

## Summary

The simplified LRU Cache API provides:

- ✅ **Simpler configuration**: No eviction mode selection
- ✅ **Better performance**: No task queue overhead
- ✅ **Less memory**: Reduced memory footprint
- ✅ **Easier maintenance**: Fewer code paths

For most use cases, the migration is straightforward - just remove calls to the deprecated functions.