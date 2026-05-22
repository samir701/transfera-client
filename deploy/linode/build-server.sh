#!/usr/bin/env bash
# Build C++ API on Linode (use -j1 on 1GB VMs).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../../server" && pwd)"
JOBS="${JOBS:-1}"

cmake -S "$ROOT" -B "$ROOT/build" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/build" -j"$JOBS"
echo "Built: $ROOT/build/server"
