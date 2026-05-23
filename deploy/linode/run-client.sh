#!/usr/bin/env bash
set -euo pipefail

ENV_FILE=/etc/peerlink/env
if [[ -f "$ENV_FILE" ]]; then
  # shellcheck disable=SC1091
  source "$ENV_FILE"
fi

CLIENT_PORT="${CLIENT_PORT:-3000}"

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT/client"

if [[ ! -f out/index.html ]]; then
  echo "Missing client/out — run: ./deploy/linode/build-ui.sh YOUR_LINODE_IP" >&2
  exit 1
fi

echo "Starting UI on 0.0.0.0:${CLIENT_PORT}"

# serve@14 crashes on some hosts when binding port 80; Python http.server is reliable as root.
if [[ "${CLIENT_PORT}" -eq 80 ]] || [[ "${CLIENT_PORT}" -lt 1024 ]]; then
  cd out
  exec python3 -m http.server "${CLIENT_PORT}" --bind 0.0.0.0
fi

# Ports 1024+ — serve (must show 0.0.0.0:PORT in logs, not localhost:RANDOM)
exec npx --yes serve@14.2.6 out --listen "${CLIENT_PORT}"
