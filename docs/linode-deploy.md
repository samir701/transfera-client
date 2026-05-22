# Linode: client + API on two ports

| Service | Port | URL |
|---------|------|-----|
| **UI** (static Next export via `serve`) | **3000** | `http://YOUR_IP:3000/` |
| **C++ API** | **8080** | `http://YOUR_IP:8080/api/health` |

The UI is built with `NEXT_PUBLIC_API_BASE_URL=http://YOUR_IP:8080` so the browser calls the API on **8080**.

Optional: map **port 80 → 3000** so `http://YOUR_IP/` opens the UI without `:3000`.

## Setup

```bash
cd /opt/p2p_file_sharer_in_cpp
git pull
chmod +x deploy/linode/*.sh
apt update && apt install -y git cmake g++ nodejs npm iptables

# 1GB: add swap before compiling
fallocate -l 2G /swapfile && chmod 600 /swapfile && mkswap /swapfile && swapon /swapfile
```

### Configure

```bash
./deploy/linode/install-services.sh
nano /etc/peerlink/env   # set LINODE_IP, NEXT_PUBLIC_API_BASE_URL=http://IP:8080
```

### Build

```bash
JOBS=1 ./deploy/linode/build-server.sh
./deploy/linode/build-ui.sh YOUR_LINODE_IP
./deploy/linode/verify-ui.sh
```

### Start

```bash
systemctl start peerlink-api peerlink-client
systemctl status peerlink-api peerlink-client
```

### Firewall

```bash
ufw allow OpenSSH
ufw allow 3000/tcp
ufw allow 8080/tcp
ufw allow 80/tcp
ufw enable
```

Linode Cloud Firewall: **22, 80, 3000, 8080**.

### Map port 80 → UI (optional)

```bash
./deploy/linode/map-port-80.sh
```

Then open **`http://YOUR_IP/`** (port 80) or **`http://YOUR_IP:3000/`**.

## Verify

```bash
curl http://127.0.0.1:8080/api/health
curl -I http://127.0.0.1:3000/
```

From your laptop:

- UI: `http://YOUR_IP:3000/` or `http://YOUR_IP/` after port map
- API: `http://YOUR_IP:8080/api/health`

## Updates

```bash
git pull
JOBS=1 ./deploy/linode/build-server.sh
./deploy/linode/build-ui.sh YOUR_LINODE_IP
systemctl restart peerlink-api peerlink-client
```

## Dev mode (optional, not systemd)

Terminal 1 — API:

```bash
cd server && PORT=8080 ./build/server
```

Terminal 2 — Next dev with proxy:

```bash
cd client
API_PROXY_TARGET=http://127.0.0.1:8080 NEXT_PUBLIC_API_BASE_URL=http://YOUR_IP:3000 npm run dev -- -H 0.0.0.0 -p 3000
```

`next dev` rewrites `/api` → `8080` so you can use `NEXT_PUBLIC_API_BASE_URL=http://YOUR_IP:3000`.

## Architecture

```text
Browser → http://IP:3000 (or :80 mapped)  →  serve (client/out)
Browser → http://IP:8080/api/*            →  C++ server
```
