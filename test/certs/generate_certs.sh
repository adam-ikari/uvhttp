#!/bin/bash

# 生成测试用的mTLS证书

echo "Generating mTLS test certificates..."

# 生成CA私钥
openssl genrsa -out ca.key 2048

# 生成CA证书
openssl req -new -x509 -days 365 -key ca.key -out ca.crt -subj "/C=CN/ST=State/L=City/O=Organization/CN=Test CA"

# 生成服务器私钥
openssl genrsa -out server.key 2048

# 生成服务器证书请求
openssl req -new -key server.key -out server.csr -subj "/C=CN/ST=State/L=City/O=Organization/CN=localhost"

# 使用CA签名服务器证书
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365

# 生成客户端私钥
openssl genrsa -out client.key 2048

# 生成客户端证书请求
openssl req -new -key client.key -out client.csr -subj "/C=CN/ST=State/L=City/O=Organization/CN=Test Client"

# 使用CA签名客户端证书
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 365

# 清理临时文件
rm server.csr client.csr ca.srl

echo "Certificates generated successfully!"
echo "Files created:"
echo "  ca.crt, ca.key - CA certificate and private key"
echo "  server.crt, server.key - Server certificate and private key"
echo "  client.crt, client.key - Client certificate and private key"
echo ""
echo "Test with:"
echo "  curl --cert client.crt --key client.key --cacert ca.crt https://localhost:8443/"