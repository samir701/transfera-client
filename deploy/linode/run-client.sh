#!/usr/bin/env bash
set -euo pipefail

: "${CLIENT_PORT:=3000}"

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT/client"

if [[ ! -f out/index.html ]]; then
  echo "Missing client/out — run: ./deploy/linode/build-ui.sh YOUR_LINODE_IP" >&2
  exit 1
fi

exec npx --yes serve@14 out -l "tcp://0.0.0.0:${CLIENT_PORT}"
