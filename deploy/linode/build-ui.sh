#!/usr/bin/env bash
# Build static Next.js UI and install files for PEERLINK_WEB_ROOT.
# Usage: ./build-ui.sh http://172.104.206.154:8080
set -euo pipefail

PUBLIC_ORIGIN="${1:?Usage: $0 http://YOUR_LINODE_IP:8080 (no trailing slash)}"
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
rm -rf "${WWW:?}"/*
cp -r out/* "$WWW/"
echo "Installed UI to $WWW"
echo "Start server with PEERLINK_WEB_ROOT=$WWW and open ${PUBLIC_ORIGIN}/"
