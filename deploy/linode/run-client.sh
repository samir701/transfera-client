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

echo "Starting UI on 0.0.0.0:${CLIENT_PORT} (from CLIENT_PORT)"
# Numeric listen arg — "tcp://0.0.0.0:PORT" makes serve pick a random localhost port on some versions
exec npx --yes serve@14.2.6 out --listen "0.0.0.0:${CLIENT_PORT}"
