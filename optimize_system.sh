#!/bin/bash
# UVHTTP 系统优化脚本
# 用于提升高并发性能

echo "=== UVHTTP 系统优化 ==="
echo ""

# 检查是否以 root 权限运行
if [ "$EUID" -ne 0 ]; then 
    echo "请使用 sudo 运行此脚本"
    exit 1
fi

# 优化网络参数
echo "1. 优化网络参数..."

# 增加连接队列限制
echo "   - 增加 somaxconn (当前: $(cat /proc/sys/net/core/somaxconn))"
echo 8192 > /proc/sys/net/core/somaxconn
echo "     -> 新值: $(cat /proc/sys/net/core/somaxconn)"

# 增加 SYN 队列限制
echo "   - 增加 tcp_max_syn_backlog (当前: $(cat /proc/sys/net/ipv4/tcp_max_syn_backlog))"
echo 8192 > /proc/sys/net/ipv4/tcp_max_syn_backlog
echo "     -> 新值: $(cat /proc/sys/net/ipv4/tcp_max_syn_backlog)"

# 启用 TIME_WAIT 重用
echo "   - 启用 TIME_WAIT 重用 (当前: $(cat /proc/sys/net/ipv4/tcp_tw_reuse))"
echo 1 > /proc/sys/net/ipv4/tcp_tw_reuse
echo "     -> 新值: $(cat /proc/sys/net/ipv4/tcp_tw_reuse)"

# 增加 TCP 读写缓冲区
echo "   - 增加 TCP 读写缓冲区"
echo 87380 > /proc/sys/net/ipv4/tcp_rmem_min
echo 87380 > /proc/sys/net/ipv4/tcp_wmem_min
echo "     -> 新值: 87380"

# 增加文件描述符限制
echo "2. 增加文件描述符限制..."
echo "   - 当前软限制: $(ulimit -Sn)"
echo "   - 当前硬限制: $(ulimit -Hn)"
ulimit -n 100000
echo "     -> 新软限制: $(ulimit -Sn)"

echo ""
echo "=== 优化完成 ==="
echo "注意：这些优化在系统重启后会失效"
echo "如需永久生效，请修改 /etc/sysctl.conf"