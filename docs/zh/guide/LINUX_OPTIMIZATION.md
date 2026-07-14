# UVHTTP Linux 优化指南

## Linux 专属优化

UVHTTP 主要面向 Linux 平台进行设计与优化。本文档介绍 Linux 专属的优化、配置与最佳实践。

## 内核调优

### 文件描述符限制

```bash
# 查看当前限制
ulimit -n

# 提高文件描述符限制（建议：65536）
ulimit -n 65536

# 在 /etc/security/limits.conf 中永久设置
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf
```

### TCP 协议栈优化

```bash
# 启用 TCP Fast Open
echo 3 | sudo tee /proc/sys/net/ipv4/tcp_fastopen

# 启用 TCP 时间戳
echo 1 | sudo tee /proc/sys/net/ipv4/tcp_timestamps

# 启用 TCP 窗口缩放
echo 1 | sudo tee /proc/sys/net/ipv4/tcp_window_scaling

# 降低 TCP keepalive 时间
echo 600 | sudo tee /proc/sys/net/ipv4/tcp_keepalive_time
echo 60 | sudo tee /proc/sys/net/ipv4/tcp_keepalive_intvl
echo 20 | sudo tee /proc/sys/net/ipv4/tcp_keepalive_probes

# 增大 TCP backlog
echo 4096 | sudo tee /proc/sys/net/core/somaxconn
echo 4096 | sudo tee /proc/sys/net/ipv4/tcp_max_syn_backlog
```

### 永久内核配置

创建 `/etc/sysctl.d/99-uvhttp.conf`：

```bash
cat << 'EOF' | sudo tee /etc/sysctl.d/99-uvhttp.conf
# UVHTTP 优化

# 文件描述符
fs.file-max = 2097152

# TCP 设置
net.ipv4.tcp_fastopen = 3
net.ipv4.tcp_timestamps = 1
net.ipv4.tcp_window_scaling = 1
net.ipv4.tcp_keepalive_time = 600
net.ipv4.tcp_keepalive_intvl = 60
net.ipv4.tcp_keepalive_probes = 20
net.ipv4.tcp_max_syn_backlog = 8192
net.ipv4.tcp_tw_reuse = 1

# Socket 缓冲区
net.ipv4.tcp_rmem = 4096 87380 4194304
net.ipv4.tcp_wmem = 4096 65536 4194304

# 网络
net.core.somaxconn = 8192
net.core.netdev_max_backlog = 16384
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216

# 连接跟踪
net.netfilter.nf_conntrack_max = 524288

# 性能
net.ipv4.tcp_slow_start_after_idle = 1
net.ipv4.tcp_no_metrics_save = 1
