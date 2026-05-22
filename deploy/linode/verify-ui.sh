#!/usr/bin/env bash
# Quick check that the static UI is installed before starting the server.
set -euo pipefail

WWW="${WWW_ROOT:-/var/www/peerlink}"

if [[ ! -f "$WWW/index.html" ]]; then
  echo "Missing $WWW/index.html"
  echo "Run: ./deploy/linode/build-ui.sh http://YOUR_LINODE_IP:8080"
  exit 1
fi

echo "OK: $WWW/index.html ($(wc -c < "$WWW/index.html") bytes)"
ls -la "$WWW" | head -15
