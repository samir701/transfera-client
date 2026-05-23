#!/usr/bin/env bash
# Run on the Linode VM to see why http://IP/ or :3000/:8080 fail.
set -euo pipefail

ENV_FILE=/etc/transfera/env
if [[ -f "$ENV_FILE" ]]; then
  # shellcheck disable=SC1091
  source "$ENV_FILE"
fi

IP="${LINODE_IP:-$(curl -4 -s --connect-timeout 2 ifconfig.me || hostname -I | awk '{print $1}')}"
API_PORT="${API_PORT:-8080}"
CLIENT_PORT="${CLIENT_PORT:-80}"

echo "=== Transfera diagnose (this host) ==="
echo "Public IP guess: $IP"
echo "API_PORT=$API_PORT  CLIENT_PORT=$CLIENT_PORT"
echo

echo "--- systemd ---"
systemctl is-active transfera-api 2>/dev/null || echo "transfera-api: not active"
systemctl is-active transfera-client 2>/dev/null || echo "transfera-client: not active"
echo

echo "--- listening ports ---"
ss -tlnp | grep -E ":${API_PORT}|:${CLIENT_PORT}|:3000|:80 |serve|node" || echo "(no listeners on 80/3000/8080)"
echo "(transfera-client must log: Accepting connections at http://0.0.0.0:PORT — not localhost:RANDOM)"
echo

echo "--- local API ---"
curl -s --connect-timeout 2 "http://127.0.0.1:${API_PORT}/api/health" || echo "FAIL: API not responding on 127.0.0.1:${API_PORT}"
echo

echo "--- local UI ---"
CODE=$(curl -sI --connect-timeout 2 "http://127.0.0.1:${CLIENT_PORT}/" | head -1 || true)
echo "${CODE:-FAIL: UI not responding on 127.0.0.1:${CLIENT_PORT}}"
echo

echo "--- UI files ---"
OUT=/opt/p2p_file_sharer_in_cpp/client/out/index.html
[[ -f "$OUT" ]] && echo "OK: $OUT" || echo "MISSING: $OUT — run ./deploy/linode/build-ui.sh $IP"
echo

echo "--- firewall (ufw) ---"
ufw status 2>/dev/null || echo "ufw not installed"
echo

echo "=== From your Mac, test ==="
echo "  curl -s http://${IP}:${API_PORT}/api/health"
echo "  curl -sI http://${IP}:${CLIENT_PORT}/"
if [[ "$CLIENT_PORT" != "80" ]]; then
  echo "  (Port 80 only works if CLIENT_PORT=80 or you ran map-port-80.sh)"
fi
