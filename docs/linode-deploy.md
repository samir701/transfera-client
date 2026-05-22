# Deploy frontend + backend on Linode (one VM)

Single Linode serves:

- **UI** — static Next.js export at `/` (Caddy `file_server`)
- **API** — C++ server on `127.0.0.1:8080`, proxied as `/api/*` (HTTPS via Caddy)

P2P download uses localhost on the same VM, so this layout matches the server design.

## Prerequisites

- Ubuntu Linode (2 GB RAM recommended for building; 1 GB works with swap + `JOBS=1`)
- Repo at `/opt/p2p_file_sharer_in_cpp`
- Free hostname: [DuckDNS](https://www.duckdns.org) (e.g. `peerlink-api.duckdns.org` → your Linode IP)

## 1. System packages

```bash
apt update && apt install -y git cmake g++ nodejs npm caddy

# 1GB RAM: add swap before compiling
fallocate -l 2G /swapfile && chmod 600 /swapfile && mkswap /swapfile && swapon /swapfile
```

## 2. Clone / update repo

```bash
mkdir -p /opt && cd /opt
git clone https://github.com/dasanik2001/p2p_file_sharer_in_cpp.git
cd p2p_file_sharer_in_cpp
git pull
```

## 3. DuckDNS (free HTTPS hostname)

1. Create subdomain `peerlink-api` at duckdns.org
2. Set IP to your Linode public IPv4
3. Confirm: `dig +short peerlink-api.duckdns.org`

## 4. Build & run C++ API

```bash
cd /opt/p2p_file_sharer_in_cpp
chmod +x deploy/linode/*.sh
JOBS=1 ./deploy/linode/build-server.sh
./deploy/linode/install-services.sh
```

Edit `/etc/caddy/Caddyfile` — set your DuckDNS host (see `deploy/linode/Caddyfile.example`).

```bash
nano /etc/caddy/Caddyfile
systemctl restart peerlink-api
systemctl start peerlink-api
systemctl status peerlink-api
curl http://127.0.0.1:8080/api/health
```

## 5. Firewall

```bash
ufw allow OpenSSH
ufw allow 80/tcp
ufw allow 443/tcp
ufw enable
```

Linode Cloud Firewall: allow inbound **22**, **80**, **443**. You do not need public **8080** (API is behind Caddy).

## 6. Build & install UI

Use the **same origin** as browsers will use (HTTPS):

```bash
./deploy/linode/build-ui.sh https://peerlink-api.duckdns.org
systemctl reload caddy
```

## 7. Verify

```bash
curl https://peerlink-api.duckdns.org/api/health
```

Browser: `https://peerlink-api.duckdns.org/` — status should show API connected.

## 8. Updates

```bash
cd /opt/p2p_file_sharer_in_cpp
git pull
JOBS=1 ./deploy/linode/build-server.sh
systemctl restart peerlink-api
./deploy/linode/build-ui.sh https://peerlink-api.duckdns.org
systemctl reload caddy
```

## Environment variables (Linode UI build)

| Variable | Linode value |
|----------|----------------|
| `NEXT_PUBLIC_BASE_PATH` | empty (site at `/`) |
| `NEXT_PUBLIC_API_BASE_URL` | `https://your-host` (same as Caddy host) |

See `client/.env.linode.example`.

## GitHub Pages vs Linode

| Target | `NEXT_PUBLIC_BASE_PATH` | `NEXT_PUBLIC_API_BASE_URL` |
|--------|-------------------------|----------------------------|
| GitHub Pages | `/p2p_file_sharer_in_cpp` (CI default) | Your HTTPS API URL |
| Linode | `` (empty) | `https://same-host-as-ui` |

## Troubleshooting

| Issue | Fix |
|-------|-----|
| `cmake` appears hung | `JOBS=1`, add swap |
| `mixed-content` in browser | Use `https://` for UI and API host; rebuild UI after changing URL |
| API offline in UI | `systemctl status peerlink-api`, `curl http://127.0.0.1:8080/api/health` |
| Caddy cert errors | DNS must point to this server; ports 80/443 open |
| 404 on assets | Rebuild with `NEXT_PUBLIC_BASE_PATH=` (Linode), not GitHub Pages path |
