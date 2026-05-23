#!/usr/bin/env bash
# Upload once, download up to MAX_DOWNLOADS (default 1), then invite must 404.
set -euo pipefail

API="${API:-http://127.0.0.1:8080}"
MAX_DOWNLOADS="${MAX_DOWNLOADS:-1}"

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

echo "==> upload (maxDownloads=$MAX_DOWNLOADS)"
UPLOAD_JSON="$(curl -sf -F "file=@${FIXTURE};filename=test.bin" -F "maxDownloads=${MAX_DOWNLOADS}" "$API/api/upload")"
echo "$UPLOAD_JSON"
PORT="$(echo "$UPLOAD_JSON" | sed -n 's/.*"port"[[:space:]]*:[[:space:]]*\([0-9][0-9]*\).*/\1/p')"
if [[ -z "$PORT" ]]; then
  echo "Could not parse invite port from: $UPLOAD_JSON" >&2
  exit 1
fi

OUT="$(mktemp)"
trap 'rm -f "$FIXTURE" "$OUT"' EXIT

for i in $(seq 1 "$MAX_DOWNLOADS"); do
  echo "==> download #$i (invite_port=$PORT)"
  HTTP="$(curl -s -o "$OUT" -w "%{http_code}" "$API/api/download/$PORT")"
  echo "http=$HTTP bytes: $(wc -c <"$OUT" | tr -d ' ')"
  [[ "$HTTP" == "200" ]] || { cat "$OUT" >&2; exit 1; }
  cmp -s "$FIXTURE" "$OUT"
done

echo "==> download #$((MAX_DOWNLOADS + 1)) (must fail — invite removed)"
HTTP_EXTRA="$(curl -s -o "$OUT" -w "%{http_code}" "$API/api/download/$PORT")"
echo "http=$HTTP_EXTRA body: $(head -c 120 "$OUT")"
[[ "$HTTP_EXTRA" == "404" ]] || exit 1

echo "OK: $MAX_DOWNLOADS download(s) OK; next correctly rejected"
