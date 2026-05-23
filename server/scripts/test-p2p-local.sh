#!/usr/bin/env bash
# Start API, run double-download P2P test, print P2P log lines.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="${PORT:-18080}"
API="http://127.0.0.1:${PORT}"
LOG="${TMPDIR:-/tmp}/transfera-test-${PORT}.log"
BIN="$ROOT/build/server"

if [[ ! -x "$BIN" ]]; then
  cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$ROOT/build" -j
fi

if command -v lsof >/dev/null; then
  stale="$(lsof -ti ":$PORT" 2>/dev/null || true)"
  [[ -n "$stale" ]] && kill -9 $stale 2>/dev/null || true
fi

export PORT TRANSFERA_LOG_VERBOSE=1 TRANSFERA_LOG_HTTP=1
"$BIN" >"$LOG" 2>&1 &
PID=$!
trap 'kill -9 $PID 2>/dev/null || true' EXIT

for _ in $(seq 1 50); do
  curl -sf "$API/api/health" >/dev/null 2>&1 && break
  sleep 0.05
done

KILL_STALE=0 API="$API" "$ROOT/scripts/test-p2p-download.sh"
echo "--- P2P log (${LOG}) ---"
grep P2P "$LOG" || true
