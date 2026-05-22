#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
ENV_FILE=/etc/peerlink/env

mkdir -p /etc/peerlink
if [[ ! -f "$ENV_FILE" ]]; then
  cp "$DIR/peerlink.env.example" "$ENV_FILE"
  echo "Created $ENV_FILE — edit LINODE_IP and NEXT_PUBLIC_API_BASE_URL, then re-run build-ui.sh"
fi

install -m 644 "$DIR/peerlink-api.service" /etc/systemd/system/peerlink-api.service
install -m 644 "$DIR/peerlink-client.service" /etc/systemd/system/peerlink-client.service
chmod +x "$DIR/run-client.sh" "$DIR/map-port-80.sh"

systemctl daemon-reload
systemctl enable peerlink-api peerlink-client
echo "Enabled peerlink-api (8080) and peerlink-client (3000)."
echo "  1. Edit $ENV_FILE"
echo "  2. ./deploy/linode/build-ui.sh YOUR_IP"
echo "  3. systemctl start peerlink-api peerlink-client"
echo "  4. Optional: ./deploy/linode/map-port-80.sh  then open http://YOUR_IP/"
