#!/bin/bash
# UVHTTP 依赖库构建脚本
# 构建所有必需的依赖库

set -e

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 获取脚本目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# 日志函数
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查命令是否存在
check_command() {
    if ! command -v $1 &> /dev/null; then
        log_error "$1 未安装，请先安装"
        exit 1
    fi
}

# 构建函数
build_libuv() {
    log_info "开始构建 libuv..."

    cd "$PROJECT_ROOT/deps/libuv"
    mkdir -p build && cd build

    if [ -f "libuv.a" ]; then
        log_warn "libuv 已构建，跳过"
        cd "$PROJECT_ROOT"
        return 0
    fi

    cmake .. \
        -DBUILD_TESTING=OFF \
        -DCMAKE_BUILD_TYPE=Release

    make -j$(nproc)

    if [ -f "libuv.a" ]; then
        log_info "libuv 构建成功"
    else
        log_error "libuv 构建失败"
        exit 1
    fi

    cd "$PROJECT_ROOT"
}

build_mbedtls() {
    log_info "开始构建 mbedtls..."

    cd "$PROJECT_ROOT/deps/mbedtls"
    mkdir -p build && cd build

    if [ -f "library/libmbedcrypto.a" ] && [ -f "library/libmbedtls.a" ] && [ -f "library/libmbedx509.a" ]; then
        log_warn "mbedtls 已构建，跳过"
        cd "$PROJECT_ROOT"
        return 0
    fi

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_TESTING=OFF \
        -DENABLE_PROGRAMS=OFF

    make -j$(nproc)

    if [ -f "library/libmbedcrypto.a" ] && [ -f "library/libmbedtls.a" ] && [ -f "library/libmbedx509.a" ]; then
        log_info "mbedtls 构建成功"
    else
        log_error "mbedtls 构建失败"
        exit 1
    fi

    cd "$PROJECT_ROOT"
}

build_llhttp() {
    log_info "开始构建 llhttp..."

    cd "$PROJECT_ROOT/deps/llhttp"

    # 检查是否已构建
    if [ -f "build/libllhttp.a" ]; then
        log_warn "llhttp 已构建，跳过"
        cd "$PROJECT_ROOT"
        return 0
    fi

    mkdir -p build && cd build

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -DLLHTTP_BUILD_SHARED_LIBS=OFF

    make -j$(nproc)

    if [ -f "libllhttp.a" ]; then
        log_info "llhttp 构建成功"
    else
        log_error "llhttp 构建失败"
        exit 1
    fi

    cd "$PROJECT_ROOT"
}

build_xxhash() {
    log_info "开始构建 xxhash..."

    cd "$PROJECT_ROOT/deps/xxhash"

    if [ -f "libxxhash.a" ]; then
        log_warn "xxhash 已构建，跳过"
        cd "$PROJECT_ROOT"
        return 0
    fi

    make

    if [ -f "libxxhash.a" ]; then
        log_info "xxhash 构建成功"
    else
        log_error "xxhash 构建失败"
        exit 1
    fi

    cd "$PROJECT_ROOT"
}

build_mimalloc() {
    log_info "开始构建 mimalloc..."

    cd "$PROJECT_ROOT/deps/mimalloc"
    mkdir -p build && cd build

    if [ -f "libmimalloc.a" ]; then
        log_warn "mimalloc 已构建，跳过"
        cd "$PROJECT_ROOT"
        return 0
    fi

    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DMI_BUILD_STATIC=ON \
        -DMI_BUILD_SHARED=OFF \
        -DMI_BUILD_TESTS=OFF

    make -j$(nproc)

    if [ -f "libmimalloc.a" ]; then
        log_info "mimalloc 构建成功"
    else
        log_error "mimalloc 构建失败"
        exit 1
    fi

    cd "$PROJECT_ROOT"
}

# 主函数
main() {
    log_info "开始构建 UVHTTP 依赖库..."
    log_info "项目根目录: $PROJECT_ROOT"

    # 检查必要的命令
    check_command cmake
    check_command make

    # 构建所有依赖
    build_libuv
    build_mbedtls
    build_llhttp
    build_xxhash
    build_mimalloc

    log_info "所有依赖库构建完成！"
    log_info "现在可以运行: cmake -B build && cmake --build build"
}

# 运行主函数
main "$@"