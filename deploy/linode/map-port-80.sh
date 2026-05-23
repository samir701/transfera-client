#!/usr/bin/env bash
# Map inbound port 80 → client port so http://LINODE_IP/ opens the UI.
# Requires PUBLIC_CLIENT_PORT=80 and CLIENT_PORT=3000 in /etc/peerlink/env
set -euo pipefail

if [[ -f /etc/peerlink/env ]]; then
  # shellcheck disable=SC1091
  source /etc/peerlink/env
fi

PUBLIC_PORT="${PUBLIC_CLIENT_PORT:-80}"
CLIENT_PORT="${CLIENT_PORT:-3000}"

if [[ "$EUID" -ne 0 ]]; then
  echo "Run as root: sudo $0" >&2
  exit 1
fi

iptables -t nat -C PREROUTING -p tcp --dport "$PUBLIC_PORT" -j REDIRECT --to-ports "$CLIENT_PORT" 2>/dev/null &&
  echo "Rule already exists: $PUBLIC_PORT -> $CLIENT_PORT" && exit 0

iptables -t nat -A PREROUTING -p tcp --dport "$PUBLIC_PORT" -j REDIRECT --to-ports "$CLIENT_PORT"
echo "Mapped tcp/$PUBLIC_PORT -> $CLIENT_PORT"
echo "Open http://\${LINODE_IP}/ in a browser (also keep http://\${LINODE_IP}:$CLIENT_PORT/)"
