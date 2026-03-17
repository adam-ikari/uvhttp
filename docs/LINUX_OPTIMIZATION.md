# UVHTTP Linux Optimization Guide

## Linux-Specific Optimizations

UVHTTP is primarily designed and optimized for Linux platforms. This document covers Linux-specific optimizations, configurations, and best practices.

## Kernel Tuning

### File Descriptor Limits

```bash
# Check current limits
ulimit -n

# Increase file descriptor limit (recommended: 65536)
ulimit -n 65536

# Permanent setting in /etc/security/limits.conf
echo "* soft nofile 65536" | sudo tee -a /etc/security/limits.conf
echo "* hard nofile 65536" | sudo tee -a /etc/security/limits.conf
```

### TCP Stack Optimization

```bash
# Enable TCP Fast Open
echo 3 | sudo tee /proc/sys/net/ipv4/tcp_fastopen

# Enable TCP timestamps
echo 1 | sudo tee /proc/sys/net/ipv4/tcp_timestamps

# Enable TCP window scaling
echo 1 | sudo tee /proc/sys/net/ipv4/tcp_window_scaling

# Reduce TCP keepalive time
echo 600 | sudo tee /proc/sys/net/ipv4/tcp_keepalive_time
echo 60 | sudo tee /proc/sys/net/ipv4/tcp_keepalive_intvl
echo 20 | sudo tee /proc/sys/net/ipv4/tcp_keepalive_probes

# Increase TCP backlog
echo 4096 | sudo tee /proc/sys/net/core/somaxconn
echo 4096 | sudo tee /proc/sys/net/ipv4/tcp_max_syn_backlog
```

### Permanent Kernel Configuration

Create `/etc/sysctl.d/99-uvhttp.conf`:

```bash
cat << 'EOF' | sudo tee /etc/sysctl.d/99-uvhttp.conf
# UVHTTP Optimization

# File Descriptors
fs.file-max = 2097152

# TCP Settings
net.ipv4.tcp_fastopen = 3
net.ipv4.tcp_timestamps = 1
net.ipv4.tcp_window_scaling = 1
net.ipv4.tcp_keepalive_time = 600
net.ipv4.tcp_keepalive_intvl = 60
net.ipv4.tcp_keepalive_probes = 20
net.ipv4.tcp_max_syn_backlog = 8192
net.ipv4.tcp_tw_reuse = 1

# Socket Buffers
net.ipv4.tcp_rmem = 4096 87380 4194304
net.ipv4.tcp_wmem = 4096 65536 4194304

# Network
net.core.somaxconn = 8192
net.core.netdev_max_backlog = 16384
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216

# Connection Tracking
net.netfilter.nf_conntrack_max = 524288

# Performance
net.ipv4.tcp_slow_start_after_idle = 1
net.ipv4.tcp_no_metrics_save = 1
