#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "$0")" && pwd)"
ENV_FILE=/etc/transfera/env

mkdir -p /etc/transfera
if [[ ! -f "$ENV_FILE" ]]; then
  cp "$DIR/transfera.env.example" "$ENV_FILE"
  echo "Created $ENV_FILE — edit LINODE_IP and NEXT_PUBLIC_API_BASE_URL, then re-run build-ui.sh"
fi

install -m 644 "$DIR/transfera-api.service" /etc/systemd/system/transfera-api.service
install -m 644 "$DIR/transfera-client.service" /etc/systemd/system/transfera-client.service
chmod +x "$DIR/run-client.sh" "$DIR/map-port-80.sh"

systemctl daemon-reload
systemctl enable transfera-api transfera-client
echo "Enabled transfera-api (:8080) and transfera-client (CLIENT_PORT from env, default 80)."
echo "  1. Edit $ENV_FILE (set LINODE_IP, CLIENT_PORT=80 for http://IP/)"
echo "  2. ./deploy/linode/build-ui.sh YOUR_IP"
echo "  3. ufw allow 80/tcp && ufw allow 8080/tcp"
echo "  4. systemctl start transfera-api transfera-client"
echo "  5. ./deploy/linode/diagnose.sh"
