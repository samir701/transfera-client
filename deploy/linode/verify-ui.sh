#!/usr/bin/env bash
set -euo pipefail

OUT="$(cd "$(dirname "$0")/../../client/out" && pwd)"

if [[ ! -f "$OUT/index.html" ]]; then
  echo "Missing $OUT/index.html — run: ./deploy/linode/build-ui.sh YOUR_LINODE_IP"
  exit 1
fi

echo "OK: $OUT/index.html"
