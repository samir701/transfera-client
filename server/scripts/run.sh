#!/usr/bin/env bash
# Start PeerLink C++ API — kills any stale process on port 8080 first.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
PORT="${PORT:-8080}"
BIN="$ROOT/build/server"

if [[ ! -x "$BIN" ]]; then
  echo "Building server..."
  cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$ROOT/build" -j
fi

PIDS="$(lsof -ti ":$PORT" 2>/dev/null || true)"
if [[ -n "$PIDS" ]]; then
  echo "Stopping existing process(es) on port $PORT: $PIDS"
  kill -9 $PIDS 2>/dev/null || true
  sleep 0.5
fi

export PORT
exec "$BIN"
