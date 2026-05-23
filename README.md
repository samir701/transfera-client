// Uses Socket Programming
// Uses HTTP library

// Future Work - make a CLI using which files can be transferred 


## Server (C++ API)

Build and run:

```bash
cd server
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./scripts/run.sh
```

`run.sh` stops any old process on port 8080 before starting (multiple stale servers will break uploads).

If port 8080 is stuck: `lsof -ti :8080 | xargs kill -9`

Default port is **8080** (override with `PORT=9000 ./build/server`).

API endpoints (used by the Next.js client):

| Method | Path | Description |
|--------|------|-------------|
| GET | `/api/health` | Health check |
| POST | `/api/upload` | Multipart field `file` → `{ "port": <invite-code> }` |
| GET | `/api/download/:port` | Download file for invite code |

CORS is enabled for browser requests from the frontend.

## Client (Next.js)

```bash
cd client
cp .env.local.example .env.local   # optional; defaults to http://localhost:8080
npm install
npm run dev
```

1. Start the C++ server (`./build/server`).
2. Start the client (`npm run dev` → usually http://localhost:3000).
3. Upload a file, share the invite code, download on another tab/machine (same host for P2P peer socket).

`NEXT_PUBLIC_API_BASE_URL` must point at the C++ server (required for static export / production hosting).

Production default: **https://transfera-api.onrender.com** (Render). See **[docs/render-deploy.md](docs/render-deploy.md)**.

```bash
curl https://transfera-api.onrender.com/api/health
```

## Deploy on Linode (optional self-host)

See **[docs/linode-deploy.md](docs/linode-deploy.md)** — client and C++ API on separate ports; optional map **:80 → :3000**.

```bash
chmod +x deploy/linode/*.sh
./deploy/linode/install-services.sh
# edit /etc/transfera/env
JOBS=1 ./deploy/linode/build-server.sh
./deploy/linode/build-ui.sh YOUR_LINODE_IP
systemctl start transfera-api transfera-client
```

Open `http://YOUR_LINODE_IP:3000/` (UI calls API on `:8080`).

## Technical manual (GitHub Pages)

Full architecture documentation (threads, sockets, FDs, upload/download pipelines):

- **Manual:** https://dasanik2001.github.io/transfera-client/manual/
- **Live app:** https://dasanik2001.github.io/transfera-client/

Source: `client/public/manual/` (deployed with the client static build).