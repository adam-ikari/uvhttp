# UVHTTP 性能测试 Docker 镜像
FROM ubuntu:22.04

# 设置非交互模式
ENV DEBIAN_FRONTEND=noninteractive

# 安装构建和测试工具
RUN apt-get update -qq && \
    apt-get install -y -qq \
    build-essential \
    cmake \
    git \
    wrk \
    apache2-utils \
    cpupower \
    linux-tools-common \
    python3 \
    python3-pip \
    python3-yaml \
    bc \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /workspace

# 验证工具安装
RUN echo "=== 验证工具安装 ===" && \
    echo "GCC: $(gcc --version | head -1)" && \
    echo "CMake: $(cmake --version | head -1)" && \
    echo "wrk: $(wrk --version | head -1)" && \
    echo "Python: $(python3 --version)"

# 默认命令
CMD ["/bin/bash"]