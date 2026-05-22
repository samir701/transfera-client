#!/usr/bin/env bash
# Install systemd unit + Caddyfile template on Linode.
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"

install -m 644 "$DIR/peerlink-api.service" /etc/systemd/system/peerlink-api.service
systemctl daemon-reload
systemctl enable peerlink-api

if [[ ! -f /etc/caddy/Caddyfile ]] || ! grep -q reverse_proxy /etc/caddy/Caddyfile 2>/dev/null; then
  echo "Installing Caddyfile from example (edit hostname in /etc/caddy/Caddyfile):"
  cp "$DIR/Caddyfile.example" /etc/caddy/Caddyfile
fi

systemctl enable caddy 2>/dev/null || true
echo "Done. Edit /etc/caddy/Caddyfile hostname, then:"
echo "  systemctl restart peerlink-api"
echo "  systemctl reload caddy"
