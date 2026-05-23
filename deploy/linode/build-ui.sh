#!/usr/bin/env bash
# Build static UI pointing at the production API (Render by default).
# Usage: ./build-ui.sh
# Override: NEXT_PUBLIC_API_BASE_URL=https://... ./build-ui.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
API_URL_FILE="$ROOT/deploy/api-url.env"
if [[ -f "$API_URL_FILE" ]]; then
  # shellcheck disable=SC1090
  source "$API_URL_FILE"
fi

API_ORIGIN="${NEXT_PUBLIC_API_BASE_URL:-https://transfera-api.onrender.com}"
CLIENT="$(cd "$(dirname "$0")/../../client" && pwd)"

cd "$CLIENT"

if [[ ! -f src/app/page.tsx ]] || [[ ! -d src/components ]]; then
  echo "Missing client/src — run git pull" >&2
  exit 1
fi

export NEXT_PUBLIC_BASE_PATH=
export NEXT_PUBLIC_API_BASE_URL="$API_ORIGIN"

npm ci
NODE_ENV=production npm run build

if [[ ! -f out/index.html ]]; then
  echo "Build failed: missing out/index.html" >&2
  exit 1
fi

echo "Built client/out — API URL: $API_ORIGIN"
