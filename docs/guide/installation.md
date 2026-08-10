# Installation Guide

This guide explains how to install and build UVHTTP.

## Platform Support

**Currently supported**: Linux

**Planned**: macOS, Windows, FreeBSD, WebAssembly (WASM), and other Unix-like systems

UVHTTP is currently optimized for the Linux platform. Support for other operating systems and platforms is planned for future releases.

## System Requirements

### Minimum Requirements
- **CMake**: 3.10 or higher
- **C compiler**:
  - GCC 4.9+ (Linux)
  - Clang 3.5+ (Linux)
- **Operating System**: Linux

### Recommended Requirements
- **CMake**: 3.15 or higher
- **C compiler**:
  - GCC 7+ (Linux)
  - Clang 10+ (Linux)

## Building from Source

### 1. Clone the Repository

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp
```

> **Note**: The `--recurse-submodules` argument automatically clones all dependencies. If you forget to use it, run `git submodule update --init --recursive` to fetch them.

### 2. Configure and Build the Project

```bash
# Basic configuration and build (Release mode)
make build
```

## Platform-Specific Instructions

### Ubuntu/Debian

#### Install Dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
    cmake \
    build-essential
```

> Note: libuv is included in the project as a submodule and does not need to be installed separately. It is built automatically during compilation.

#### Build

```bash
# Initialize submodules (required on first clone)
git submodule update --init --recursive

make build
```

### CentOS/RHEL

#### Install Dependencies

```bash
sudo yum groupinstall "Development Tools"
sudo yum install -y \
    cmake3 \
    openssl-devel
```

> Note: libuv is included in the project as a submodule and does not need to be installed separately. It is built automatically during compilation.

#### Build

```bash
make build
```

### macOS

#### Using Homebrew

```bash
# Install Homebrew (if not already installed)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install cmake openssl

# Build
make build
```

#### Using MacPorts

```bash
# Install MacPorts (if not already installed)
# Then install dependencies
sudo port install cmake libuv openssl

# Build
make build
```

### Windows

#### Using vcpkg

```bash
# Install vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.bat
./vcpkg integrate install

# Install dependencies
vcpkg install libuv openssl:x64-windows

# Build
make build
```

#### Using Prebuilt Dependencies

1. Download and install libuv: https://github.com/libuv/libuv/releases
2. Download and install OpenSSL: https://slproweb.com/products/Win32OpenSSL.html
3. Specify the library paths in `CMakeLists.txt`, then run `make build`:
   ```cmake
   set(LIBUV_INCLUDE_DIR "[libuv include path]" CACHE PATH "")
   set(LIBUV_LIBRARY "[libuv library path]" CACHE FILEPATH "")
   set(OPENSSL_INCLUDE_DIR "[OpenSSL include path]" CACHE PATH "")
   set(OPENSSL_LIBRARY "[OpenSSL library path]" CACHE FILEPATH "")
   ```

## Build Options

### Common CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `BUILD_WITH_WEBSOCKET` | `ON` | Enable WebSocket support |
| `BUILD_WITH_MIMALLOC` | `ON` | Enable the mimalloc memory allocator |
| `BUILD_WITH_HTTPS` | `ON` | Enable TLS support |
| `BUILD_EXAMPLES` | `ON` | Build the example programs |
| `ENABLE_DEBUG` | `OFF` | Enable Debug mode (-O0) |
| `ENABLE_COVERAGE` | `OFF` | Enable code coverage |

### Example Configurations

Edit the `option()` defaults in `CMakeLists.txt`, then run `make build`:

```bash
# Minimal configuration (core features only) - set the relevant options to OFF in CMakeLists.txt
make build

# Full configuration (all features)
make build

# Debug configuration - set ENABLE_DEBUG and ENABLE_COVERAGE to ON in CMakeLists.txt
make build
```

## Verifying the Installation

### Running Tests

```bash
cd build
ctest --output-on-failure
```

### Running Examples

```bash
# Build the examples
make

# Run the Hello World example
./dist/bin/hello_world

# Run the WebSocket example
./dist/bin/websocket_echo_server
```

### Checking the Version

```bash
./dist/bin/hello_world --version
```

## Troubleshooting

### Compilation Errors

**Problem**: Dependencies not found

**Solution**:
```bash
# Ensure submodules are initialized
git submodule update --init --recursive
```

### Linker Errors

**Problem**: undefined reference to `uv_*`

**Solution**:
```bash
# Ensure the correct libraries are linked
# Add to CMakeLists.txt:
target_link_libraries(your_target ${LIBUV_LIB} ${MBEDTLS_LIBS} ...)
```

### CMake Version Too Old

**Problem**: CMake 3.10+ required

**Solution**:
```bash
# Linux
sudo apt-get install cmake3

# macOS
brew install cmake

# Install from source
wget https://github.com/Kitware/CMake/releases/download/v3.28.0/cmake-3.28.0.tar.gz
tar -xzf cmake-3.28.0.tar.gz
cd cmake-3.28.0
./bootstrap
make build
```

## Next Steps

After installation, continue with:
- [Getting Started](./getting-started.md) - 5-minute quick start
- [First Server](./first-server.md) - Create your first HTTP server
- [Full Tutorial](./TUTORIAL.md) - Complete tutorial from basics to advanced

## Getting Help

If you encounter installation problems:
- See [FAQ](./FAQ.md)
- Open an [Issue](https://github.com/adam-ikari/uvhttp/issues)
- See the Error Code Reference
