#!/usr/bin/env bash
# Build static Next.js export for Linode (site at /, API via Caddy /api proxy).
# Usage: ./build-ui.sh https://peerlink-api.duckdns.org
set -euo pipefail

PUBLIC_ORIGIN="${1:?Usage: $0 https://your-host.example (no trailing slash)}"
PUBLIC_ORIGIN="${PUBLIC_ORIGIN%/}"

CLIENT="$(cd "$(dirname "$0")/../../client" && pwd)"
WWW="${WWW_ROOT:-/var/www/peerlink}"

cd "$CLIENT"
export NODE_ENV=production
export NEXT_PUBLIC_BASE_PATH=
export NEXT_PUBLIC_API_BASE_URL="$PUBLIC_ORIGIN"

npm ci
npm run build

mkdir -p "$WWW"
cp -r out/* "$WWW/"
echo "Installed UI to $WWW"
echo "Open: ${PUBLIC_ORIGIN}/"
