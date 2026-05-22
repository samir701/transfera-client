#!/usr/bin/env bash
# Install systemd unit for PeerLink (API + static UI on one port).
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"

install -m 644 "$DIR/peerlink-api.service" /etc/systemd/system/peerlink-api.service
systemctl daemon-reload
systemctl enable peerlink-api
echo "Installed peerlink-api.service"
echo "  systemctl start peerlink-api"
echo "  Open http://YOUR_LINODE_IP:8080/"
