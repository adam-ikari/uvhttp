#!/usr/bin/env bash
# Long-run memory-stability benchmark for UVHTTP.
#
# Drives sustained load against the built-in test server and samples the
# server's RSS at regular intervals. The invariant we care about (and that
# ASan-clean status implies) is that RSS does not grow unboundedly over time —
# critical for long-running services and embedded devices that cannot restart.
#
# Usage:
#   scripts/performance/long_run_memory.sh [duration_seconds] [server_bin] [port]
#
# Defaults: 120s, build/dist/bin/test_performance_e2e, 18095
# Requires: wrk, the server binary built in Release mode.

set -u

DURATION="${1:-120}"
SERVER_BIN="${2:-build/dist/bin/test_performance_e2e}"
PORT="${3:-18095}"
SAMPLE_INTERVAL=10
URL="http://127.0.0.1:${PORT}/simple"

command -v wrk >/dev/null 2>&1 || { echo "ERROR: wrk not installed"; exit 1; }
[ -x "$SERVER_BIN" ] || { echo "ERROR: server binary not found: $SERVER_BIN"; exit 1; }

# Kill any previous instance bound to our port, without matching this script's
# own command line (avoid self-kill).
existing=$(lsof -ti tcp:"$PORT" 2>/dev/null || true)
[ -n "$existing" ] && kill -9 $existing 2>/dev/null || true
sleep 1

# Start server.
"$SERVER_BIN" "$PORT" >/tmp/uvhttp_longrun.log 2>&1 &
SERVER_PID=$!
sleep 2

if ! curl -s -m 3 "$URL" >/dev/null 2>&1; then
  echo "ERROR: server did not start on port $PORT"
  kill -9 "$SERVER_PID" 2>/dev/null || true
  exit 1
fi

echo "=== UVHTTP long-run memory stability ==="
echo "server:   $SERVER_BIN (pid $SERVER_PID)"
echo "duration: ${DURATION}s, sampling RSS every ${SAMPLE_INTERVAL}s"
echo "load:     wrk -t4 -c100 (sustained)"
echo ""
printf "%-10s %12s %12s\n" "elapsed_s" "RSS_KB" "connections"

# Drive sustained load in the background for the full duration.
( wrk -t4 -c100 -d"${DURATION}s" "$URL" >/tmp/uvhttp_longrun_wrk.log 2>&1 ) &
WRK_PID=$!

# Sample RSS while load runs.
ELAPSED=0
FIRST_RSS=""
LAST_RSS=""
while [ "$ELAPSED" -lt "$DURATION" ]; do
  sleep "$SAMPLE_INTERVAL"
  ELAPSED=$((ELAPSED + SAMPLE_INTERVAL))
  # RSS is field 2 of /proc/<pid>/status (in kB on Linux), second column.
  RSS_KB=$(awk '/VmRSS:/{print $2}' /proc/$SERVER_PID/status 2>/dev/null)
  [ -z "$RSS_KB" ] && break
  [ -z "$FIRST_RSS" ] && FIRST_RSS="$RSS_KB"
  LAST_RSS="$RSS_KB"
  printf "%-10s %12s %12s\n" "$ELAPSED" "$RSS_KB" "100"
done

wait "$WRK_PID" 2>/dev/null
WRK_RPS=$(grep -oE "Requests/sec:[[:space:]]+[0-9.]+" /tmp/uvhttp_longrun_wrk.log | awk '{print $2}')
WRK_ERRS=$(grep -oE "Socket errors:[[:space:]]+[0-9]+" /tmp/uvhttp_longrun_wrk.log | awk '{print $3}')
WRK_ERRS="${WRK_ERRS:-0}"

kill -9 "$SERVER_PID" 2>/dev/null || true

echo ""
echo "=== Summary ==="
echo "first RSS: ${FIRST_RSS:-?} KB"
echo "last  RSS: ${LAST_RSS:-?} KB"
if [ -n "$FIRST_RSS" ] && [ -n "$LAST_RSS" ]; then
  DELTA=$((LAST_RSS - FIRST_RSS))
  echo "delta:     ${DELTA} KB over ${DURATION}s"
  if [ "$DELTA" -lt 0 ]; then DELTA=0; fi
  # Allow small allocator fluctuation; flag meaningful growth.
  if [ "$DELTA" -le 1024 ]; then
    echo "verdict:   STABLE (<=1MB growth) — no memory leak under sustained load"
  else
    echo "verdict:   GROWTH detected — investigate (possible leak)"
  fi
fi
echo "wrk RPS:   ${WRK_RPS:-?}"
echo "wrk socket errors: $WRK_ERRS"
