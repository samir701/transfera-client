# Linode: one process (C++ API + static UI on port 8080)

No Caddy, no DuckDNS. The C++ server serves:

- **UI** — files from `PEERLINK_WEB_ROOT` (default `/var/www/peerlink`)
- **API** — `/api/health`, `/api/upload`, `/api/download/:port`

Open **`http://YOUR_LINODE_IP:8080/`** in a browser (HTTP only avoids mixed-content issues).

## 1. Packages + swap (1 GB VMs)

```bash
apt update && apt install -y git cmake g++ nodejs npm
fallocate -l 2G /swapfile && chmod 600 /swapfile && mkswap /swapfile && swapon /swapfile
```

## 2. Repo

```bash
cd /opt/p2p_file_sharer_in_cpp
git pull
chmod +x deploy/linode/*.sh
```

## 3. Build API

```bash
JOBS=1 ./deploy/linode/build-server.sh
```

## 4. Build UI (set your Linode IP)

```bash
./deploy/linode/build-ui.sh http://172.104.206.154:8080
```

Replace `172.104.206.154` with your public IPv4.

## 5. systemd

```bash
./deploy/linode/install-services.sh
systemctl start peerlink-api
systemctl status peerlink-api
```

Unit sets `PORT=8080` and `PEERLINK_WEB_ROOT=/var/www/peerlink`.

## 6. Firewall

```bash
ufw allow OpenSSH
ufw allow 8080/tcp
ufw enable
```

Linode Cloud Firewall: inbound **22**, **8080**.

## 7. Verify

On the server:

```bash
curl http://127.0.0.1:8080/api/health
```

From your laptop:

```bash
curl http://YOUR_IP:8080/api/health
```

Browser: **`http://YOUR_IP:8080/`**

## 8. Updates

```bash
cd /opt/p2p_file_sharer_in_cpp && git pull
JOBS=1 ./deploy/linode/build-server.sh
./deploy/linode/build-ui.sh http://YOUR_IP:8080
systemctl restart peerlink-api
```

## Environment

| Variable | Purpose |
|----------|---------|
| `PORT` | Listen port (default `8080`) |
| `PEERLINK_WEB_ROOT` | Directory with `index.html`, `_next/`, `manual/` |
| `NEXT_PUBLIC_API_BASE_URL` | Set at UI **build** time to `http://IP:8080` |

## Optional: run without systemd

```bash
export PORT=8080
export PEERLINK_WEB_ROOT=/var/www/peerlink
./server/build/server
```

## GitHub Pages

Still supported via CI with `NEXT_PUBLIC_BASE_PATH=/p2p_file_sharer_in_cpp` and a separate API URL. Linode uses empty base path and same-origin `http://IP:8080`.
