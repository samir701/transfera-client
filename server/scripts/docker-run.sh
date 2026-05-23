#!/usr/bin/env bash
# Build and run Transfera API with full logs to stdout/stderr.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
IMAGE="${TRANSFERA_IMAGE:-transfera-api}"
HOST_PORT="${HOST_PORT:-8080}"
CONTAINER_PORT="${CONTAINER_PORT:-8008}"

docker build -t "$IMAGE" "$ROOT"

echo "Starting $IMAGE (logs follow — Ctrl+C to stop)"
docker run --rm -it \
  --name transfera-api \
  -p "${HOST_PORT}:${CONTAINER_PORT}" \
  -e PORT="${CONTAINER_PORT}" \
  -e TRANSFERA_LOG_HTTP=1 \
  -e TRANSFERA_LOG_VERBOSE=1 \
  "$IMAGE"
