# page `memory_allocator_usage` {#memory_allocator_usage}

Allocator SelectionChoose allocator type at compile time: 
```cpp
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..  # System allocator (default)
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..  # mimalloc allocator
```

Usage
```cpp
void* ptr = [uvhttp_alloc](#uvhttp__allocator_8h_1a867d677c1938e5194ab4362b1fc5182c)(size);     // Allocate memory
ptr = [uvhttp_realloc](#uvhttp__allocator_8h_1afc9d5e69f0b13522101e526c3309d627)(ptr, new_size); // Reallocate
[uvhttp_free](#uvhttp__allocator_8h_1a956d07931818b449298b1649e830b3a5)(ptr);                    // Free memory
ptr = [uvhttp_calloc](#uvhttp__allocator_8h_1a0f936ce1981cc3d1bea42ed1095ff2ec)(count, size);   // Allocate and initialize
```

Performance Characteristics

* System allocator: Stable and reliable, no extra dependencies

* mimalloc: High performance, optimized for multi-threading, built-in small object optimization

Compile-time Optimization

* All functions are inline

* Zero runtime overhead

* Compiler can fully optimize

Notes

* mimalloc already optimizes small object allocation, no extra memory pool layer needed

* If application needs memory pool, implement at application layer based on specific requirements

* Choose appropriate allocator type for best performance

