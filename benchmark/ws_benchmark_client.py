#!/usr/bin/env python3
"""
WebSocket benchmark client for the uvhttp benchmark_unified server.

Pure-stdlib RFC 6455 client (no external deps): performs the client
handshake (Sec-WebSocket-Key -> Sec-WebSocket-Accept), then drives the
/ws echo endpoint with masked text frames.

Measures two axes; each round emits one row compatible with the existing
benchmark CSV (endpoint,round,rps,avg_latency,p99_latency):
  /ws_connect - connection establishment throughput: N concurrent handshakes,
                rps = N / wall_seconds (latency = per-handshake ms)
  /ws_echo    - message echo throughput over the established connections:
                rps = total_messages / wall_seconds (latency = per-echo ms)

CSV rows are appended to the same benchmark-raw.csv consumed by
scripts/performance/regression_check.py.

Usage:
  python3 ws_benchmark_client.py [--host 127.0.0.1] [--port 18081]
      [--connections 10] [--messages 200] [--rounds 3] [--csv benchmark-raw.csv]
"""

import argparse
import base64
import hashlib
import os
import socket
import statistics
import struct
import sys
import threading
import time

WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
DEFAULT_PAYLOAD = b"Hello from uvhttp WebSocket benchmark 0123456789"  # 48 bytes


def percentile(values, pct):
    """Linear-interpolated percentile like wrk's latency summary."""
    if not values:
        return 0.0
    s = sorted(values)
    if len(s) == 1:
        return s[0]
    k = (len(s) - 1) * pct / 100.0
    lo = int(k)
    hi = lo + 1
    frac = k - lo
    return s[lo] * (1 - frac) + s[hi] * frac


def recv_exact(sock, n, timeout=30.0):
    data = b""
    while len(data) < n:
        chunk = sock.recv(n - len(data))
        if not chunk:
            raise ConnectionError("connection closed while reading")
        data += chunk
    return data


def ws_handshake(sock, host, port, path="/ws", timeout=30.0):
    """Perform the RFC 6455 client handshake. Returns any bytes that arrived
    after the response head (start of the first frame, if pipelined)."""
    sock.settimeout(timeout)
    key = base64.b64encode(os.urandom(16)).decode()
    request = (
        "GET %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: %s\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n" % (path, host, port, key)
    )
    sock.sendall(request.encode())

    response = b""
    while b"\r\n\r\n" not in response:
        chunk = sock.recv(4096)
        if not chunk:
            raise ConnectionError("connection closed during handshake")
        response += chunk

    head, _, leftover = response.partition(b"\r\n\r\n")
    lines = head.split(b"\r\n")
    status = lines[0].decode(errors="replace")
    if b" 101 " not in head and not head.startswith(b"HTTP/1.1 101"):
        raise ConnectionError("handshake failed: %s" % status)

    headers = {}
    for line in lines[1:]:
        if b":" in line:
            k, _, v = line.partition(b":")
            headers[k.strip().lower()] = v.strip()

    expected = base64.b64encode(
        hashlib.sha1((key + WS_GUID).encode()).digest()
    ).decode()
    accept = headers.get(b"sec-websocket-accept", b"").decode()
    if accept != expected:
        raise ConnectionError("Sec-WebSocket-Accept mismatch (got %r)" % accept)
    return leftover


def build_frame(payload, opcode=0x1):
    """Client frame: FIN + opcode, masked payload."""
    mask = os.urandom(4)
    n = len(payload)
    header = bytearray([0x80 | opcode])
    if n < 126:
        header.append(0x80 | n)
    elif n < 65536:
        header.append(0x80 | 126)
        header += struct.pack(">H", n)
    else:
        header.append(0x80 | 127)
        header += struct.pack(">Q", n)
    header += mask
    masked = bytes(b ^ mask[i % 4] for i, b in enumerate(payload))
    return bytes(header) + masked


def read_frame(sock, timeout=30.0):
    """Read one complete frame; returns (opcode, payload). The server never
    masks, so the mask bit is parsed but masked payloads are still handled."""
    b0 = recv_exact(sock, 2, timeout)
    opcode = b0[0] & 0x0F
    length = b0[1] & 0x7F
    if length == 126:
        length = struct.unpack(">H", recv_exact(sock, 2, timeout))[0]
    elif length == 127:
        length = struct.unpack(">Q", recv_exact(sock, 8, timeout))[0]
    masked = bool(b0[1] & 0x80)
    mask_key = recv_exact(sock, 4, timeout) if masked else b""
    payload = recv_exact(sock, length, timeout) if length else b""
    if masked:
        payload = bytes(b ^ mask_key[i % 4] for i, b in enumerate(payload))
    return opcode, payload


def connect_round(host, port, connections, path="/ws", timeout=30.0):
    """Open `connections` concurrent WS connections. Returns
    (sockets, per-connection latencies_ms, wall_seconds)."""
    socks = [None] * connections
    latencies = [0.0] * connections
    errors = [None] * connections

    def worker(i):
        t0 = time.perf_counter()
        try:
            s = socket.create_connection((host, port), timeout=timeout)
            ws_handshake(s, host, port, path, timeout)
            socks[i] = s
        except Exception as e:  # noqa: BLE001 - report per-connection error
            errors[i] = e
        finally:
            latencies[i] = (time.perf_counter() - t0) * 1000.0

    wall_start = time.perf_counter()
    threads = [threading.Thread(target=worker, args=(i,))
               for i in range(connections)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    wall = time.perf_counter() - wall_start

    failed = sum(1 for s in socks if s is None)
    if failed:
        for s in socks:
            if s is not None:
                try:
                    s.close()
                except Exception:
                    pass
        raise RuntimeError(
            "%d/%d connections failed (first: %r)" % (failed, connections, errors))
    return socks, latencies, wall


def echo_round(host, port, connections, messages, payload=DEFAULT_PAYLOAD,
               timeout=30.0):
    """Connect `connections` sockets, send `messages` echoed frames per socket.
    Returns (rps, avg_ms, p99_ms, total_ok)."""
    socks, _, _ = connect_round(host, port, connections)
    latencies = []
    err = []
    lock = threading.Lock()

    def worker(sock, count):
        frame = build_frame(payload)
        try:
            for _ in range(count):
                t0 = time.perf_counter()
                sock.sendall(frame)
                opcode, data = read_frame(sock, timeout)
                t1 = time.perf_counter()
                if opcode != 0x1 or data != payload:
                    raise RuntimeError(
                        "echo mismatch (opcode=%#x len=%d)" % (opcode, len(data)))
                latencies.append((t1 - t0) * 1000.0)
        except Exception as e:  # noqa: BLE001
            with lock:
                err.append(repr(e))
        finally:
            try:
                sock.close()
            except Exception:
                pass

    wall_start = time.perf_counter()
    threads = [threading.Thread(target=worker, args=(sock, messages))
               for sock in socks]
    for t in threads:
        t.start()
    for t in threads:
        t.join()
    wall = time.perf_counter() - wall_start

    if err:
        print("WARNING: %d echo worker error(s), first: %s" % (len(err), err[0]),
              file=sys.stderr)

    total = len(latencies)
    rps = total / wall if wall > 0 else 0.0
    return rps, statistics.mean(latencies) if latencies else 0.0, \
        percentile(latencies, 99) if latencies else 0.0, total


def append_csv(path, rows):
    """rows: list of (endpoint, round, rps, avg_latency, p99_latency)."""
    header = "endpoint,round,rps,avg_latency,p99_latency"
    write_header = (not os.path.exists(path)) or os.path.getsize(path) == 0
    with open(path, "a") as f:
        if write_header:
            f.write(header + "\n")
        for ep, r, rps, avg, p99 in rows:
            f.write("%s,%d,%.2f,%.3f,%.3f\n" % (ep, r, rps, avg, p99))


def main():
    parser = argparse.ArgumentParser(description="uvhttp WebSocket benchmark client")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=18081)
    parser.add_argument("--connections", type=int, default=10,
                        help="concurrent WebSocket connections (default: 10)")
    parser.add_argument("--messages", type=int, default=200,
                        help="echoed messages per connection (default: 200)")
    parser.add_argument("--rounds", type=int, default=3)
    parser.add_argument("--csv", help="append CSV rows to this file (optional)")
    args = parser.parse_args()

    rows = []
    print("=== WebSocket benchmark: %s:%d (%d conns x %d msgs x %d rounds) ==="
          % (args.host, args.port, args.connections, args.messages, args.rounds))

    # 1) Connection establishment throughput
    for r in range(1, args.rounds + 1):
        socks, latencies, wall = connect_round(args.host, args.port,
                                               args.connections)
        for s in socks:
            try:
                s.close()
            except Exception:
                pass
        rps = args.connections / wall if wall > 0 else 0.0
        avg = statistics.mean(latencies)
        p99 = percentile(latencies, 99)
        rows.append(("/ws_connect", r, rps, avg, p99))
        print("Round %d /ws_connect: %d conns in %.1fms -> %.0f conns/s "
              "(avg %.2fms, p99 %.2fms)" % (r, args.connections, wall * 1000,
                                            rps, avg, p99))

    # 2) Message echo throughput
    for r in range(1, args.rounds + 1):
        rps, avg, p99, total = echo_round(args.host, args.port,
                                          args.connections, args.messages)
        rows.append(("/ws_echo", r, rps, avg, p99))
        print("Round %d /ws_echo: %d echoes in %.2fs -> %.0f msg/s "
              "(avg %.3fms, p99 %.3fms)" % (r, total,
                                            total / rps if rps > 0 else 0,
                                            rps, avg, p99))

    if args.csv:
        append_csv(args.csv, rows)
        print("Appended %d rows to %s" % (len(rows), args.csv))

    return 0


if __name__ == "__main__":
    sys.exit(main())
