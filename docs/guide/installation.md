# Installation

This guide covers different ways to install and build UVHTTP.

## System Requirements

- **Operating System**: Linux (x86_64, x86, ARM64, ARM32)
  - **Future Plans**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems
- **Compiler**: GCC 4.8+ or Clang 3.4+
- **CMake**: 3.10 or higher
- **Memory**: At least 1GB RAM
- **Disk Space**: At least 500MB

> **Note**: UVHTTP currently supports Linux platforms only. We plan to expand support to other operating systems in future releases.

## Building from Source

### 1. Clone the Repository

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

### 2. Build Dependencies

UVHTTP includes all necessary dependencies as Git submodules. Initialize them:

```bash
git submodule update --init --recursive
```

### 3. Create Build Directory

```bash
mkdir build && cd build
```

### 4. Configure with CMake

**Basic configuration:**
```bash
cmake ..
```

**With all features:**
```bash
cmake -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_MIMALLOC=ON ..
```

**Custom configuration:**
```bash
cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_WITH_WEBSOCKET=ON \
  -DBUILD_WITH_MIMALLOC=ON \
  -DBUILD_EXAMPLES=ON \
  ..
```

### 5. Build

```bash
make -j$(nproc)
```

### 6. Install (Optional)

```bash
sudo make install
```

## Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_WITH_WEBSOCKET` | ON | Enable WebSocket support |
| `BUILD_WITH_MIMALLOC` | ON | Use mimalloc allocator |
| `BUILD_EXAMPLES` | OFF | Build example programs |
| `ENABLE_COVERAGE` | OFF | Enable code coverage |
| `ENABLE_DEBUG` | OFF | Enable debug mode |

## Running Tests

After building, run the test suite:

```bash
./run_tests.sh
```

For detailed coverage report:

```bash
./run_tests.sh --detailed
```

## Installation Paths

By default, UVHTTP installs to:

- **Library**: `/usr/local/lib/libuvhttp.a`
- **Headers**: `/usr/local/include/uvhttp/`
- **Examples**: `/usr/local/bin/`

You can change these with CMake:

```bash
cmake -DCMAKE_INSTALL_PREFIX=/custom/path ..
```

## Cross-Compilation

### Cross-compile for 32-bit

```bash
cmake \
  -DCMAKE_C_COMPILER=gcc \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_C_FLAGS="-m32" \
  -DCMAKE_CXX_FLAGS="-m32" \
  -DBUILD_WITH_MIMALLOC=OFF \
  ..
```

### Cross-compile for ARM

```bash
cmake \
  -DCMAKE_TOOLCHAIN_FILE=path/to/toolchain.cmake \
  ..
```

## Troubleshooting

### Missing Dependencies

If you see errors about missing libuv or other dependencies:

```bash
git submodule update --init --recursive
```

### Compilation Errors

Make sure you have a C11-compliant compiler:

```bash
gcc --version  # Should be 4.8 or higher
```

### Linker Errors

If you see linker errors, make sure you're linking against the required libraries:

```bash
-luvhttp -lpthread -luv
```

## Next Steps

- [Quick Start](getting-started.md) - Create your first server
- [API Reference](../api/API_REFERENCE.md) - Complete API documentation
- [Examples](../../examples/) - Example programs