#!/usr/bin/env bash
# Upload once, download once (invite burned), second download must 404.
set -euo pipefail

API="${API:-http://127.0.0.1:8080}"

# Avoid hitting a stale binary from an old server still bound to :8080
if [[ "${KILL_STALE:-1}" == "1" ]] && command -v lsof >/dev/null; then
  stale="$(lsof -ti :8080 2>/dev/null || true)"
  if [[ -n "$stale" ]]; then
    echo "==> killing stale listener(s) on :8080: $stale"
    kill -9 $stale 2>/dev/null || true
    sleep 0.3
  fi
fi
FIXTURE="${1:-}"

if [[ -z "$FIXTURE" ]]; then
  FIXTURE="$(mktemp /tmp/transfera-test-XXXXXX.bin)"
  dd if=/dev/urandom of="$FIXTURE" bs=1024 count=64 status=none 2>/dev/null || \
    head -c 65536 /dev/urandom >"$FIXTURE"
  trap 'rm -f "$FIXTURE"' EXIT
fi

echo "==> health"
curl -sf "$API/api/health" | head -c 80
echo

echo "==> upload"
UPLOAD_JSON="$(curl -sf -F "file=@${FIXTURE};filename=test.bin" "$API/api/upload")"
echo "$UPLOAD_JSON"
PORT="$(echo "$UPLOAD_JSON" | sed -n 's/.*"port"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p')"
if [[ -z "$PORT" ]]; then
  echo "Could not parse invite port from: $UPLOAD_JSON" >&2
  exit 1
fi

OUT1="$(mktemp)"
OUT2="$(mktemp)"
trap 'rm -f "$FIXTURE" "$OUT1" "$OUT2"' EXIT

echo "==> download #1 (invite_port=$PORT)"
HTTP1="$(curl -s -o "$OUT1" -w "%{http_code}" "$API/api/download/$PORT")"
echo "http=$HTTP1 bytes: $(wc -c <"$OUT1" | tr -d ' ')"
[[ "$HTTP1" == "200" ]] || { cat "$OUT1" >&2; exit 1; }

echo "==> download #2 (same invite — must fail, one-time link)"
HTTP2="$(curl -s -o "$OUT2" -w "%{http_code}" "$API/api/download/$PORT")"
echo "http=$HTTP2 body: $(head -c 120 "$OUT2")"
[[ "$HTTP2" == "404" ]] || exit 1

cmp -s "$FIXTURE" "$OUT1"
echo "OK: first download OK; second correctly rejected (one-time invite)"
