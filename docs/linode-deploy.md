# Linode: client + API on two ports

| Service | Port | URL |
|---------|------|-----|
| **UI** (`client/out`) | **3000** (default) or **80** | `http://YOUR_IP:3000/` or `http://YOUR_IP/` |
| **C++ API** | **8080** | `http://YOUR_IP:8080/api/health` |

Set `CLIENT_PORT=80` in `/etc/peerlink/env` so `curl http://YOUR_IP/` works. Use `CLIENT_PORT=3000` if you prefer `http://YOUR_IP:3000/`.

The UI is built with `NEXT_PUBLIC_API_BASE_URL=http://YOUR_IP:8080` so the browser calls the API on **8080**.

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

If the UI build fails with `Cannot find module 'tailwindcss'` or `@/components/*`, run from repo root:

```bash
cd client
rm -rf node_modules .next
npm ci
NODE_ENV=production NEXT_PUBLIC_BASE_PATH= NEXT_PUBLIC_API_BASE_URL=http://YOUR_IP:8080 npm run build
```

Do not run `npm ci` with `NODE_ENV=production` (skips dev/build deps).

### Start

```bash
systemctl start peerlink-api peerlink-client
systemctl status peerlink-api peerlink-client
```

### Firewall (required — otherwise curl hangs from your Mac)

```bash
ufw allow OpenSSH
ufw allow 80/tcp
ufw allow 8080/tcp
ufw enable
ufw status
```

**Linode dashboard → Networking → Firewall:** add inbound **TCP 22, 80, 8080** to this instance (if a Cloud Firewall is attached).

### Diagnose on the server

```bash
./deploy/linode/diagnose.sh
```

## Verify

```bash
./deploy/linode/diagnose.sh
curl http://127.0.0.1:8080/api/health
curl -I http://127.0.0.1:80/
```

From your Mac:

```bash
curl -s --connect-timeout 5 http://YOUR_IP:8080/api/health
curl -sI --connect-timeout 5 http://YOUR_IP/
```

- UI: `http://YOUR_IP/` (with `CLIENT_PORT=80`)
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
