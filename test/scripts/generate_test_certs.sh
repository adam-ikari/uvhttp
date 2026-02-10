#!/bin/bash
# 生成测试用 TLS 证书

set -e

CERT_DIR="./test/certs"
CERT_FILE="$CERT_DIR/server.crt"
KEY_FILE="$CERT_DIR/server.key"

# 创建证书目录
mkdir -p "$CERT_DIR"

echo "生成测试用 TLS 证书..."
echo "证书目录: $CERT_DIR"

# 生成自签名证书
openssl req -x509 -newkey rsa:4096 \
    -keyout "$KEY_FILE" \
    -out "$CERT_FILE" \
    -days 365 \
    -nodes \
    -subj "/C=CN/ST=Beijing/L=Beijing/O=UVHTTP/OU=Testing/CN=localhost"

# 设置权限
chmod 600 "$KEY_FILE"
chmod 644 "$CERT_FILE"

echo ""
echo " TLS 证书生成成功！"
echo "证书文件: $CERT_FILE"
echo "密钥文件: $KEY_FILE"
echo ""
echo "现在可以运行 TLS 性能测试了"