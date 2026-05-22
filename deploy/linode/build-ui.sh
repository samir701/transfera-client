#!/usr/bin/env bash
# Build static UI; browser will call the API on port 8080.
# Usage: ./build-ui.sh 172.104.206.154
set -euo pipefail

LINODE_IP="${1:?Usage: $0 LINODE_PUBLIC_IP}"
API_PORT="${API_PORT:-8080}"
API_ORIGIN="http://${LINODE_IP}:${API_PORT}"

CLIENT="$(cd "$(dirname "$0")/../../client" && pwd)"

cd "$CLIENT"

if [[ ! -f src/app/page.tsx ]] || [[ ! -d src/components ]]; then
  echo "Missing client/src — run git pull in /opt/p2p_file_sharer_in_cpp" >&2
  exit 1
fi

export NEXT_PUBLIC_BASE_PATH=
export NEXT_PUBLIC_API_BASE_URL="$API_ORIGIN"

# Install devDependencies (tailwind, typescript). Do NOT set NODE_ENV=production before npm ci.
npm ci
NODE_ENV=production npm run build

if [[ ! -f out/index.html ]]; then
  echo "Build failed: missing out/index.html" >&2
  exit 1
fi

echo "Built client/out — API URL: $API_ORIGIN"
echo "Start UI: ./deploy/linode/run-client.sh  (port \${CLIENT_PORT:-3000})"
