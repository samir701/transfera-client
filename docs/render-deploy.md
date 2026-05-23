# Deploy: Render (API) + GitHub Pages (UI)

| Component | Host | URL |
|-----------|------|-----|
| **Backend** | Render (Docker) | https://transfera-api.onrender.com |
| **Frontend** | GitHub Pages | https://dasanik2001.github.io/transfera-client/ |

Production API URL is set in:

- `client/.env.production`
- `deploy/api-url.env`
- `.github/workflows/client_deploy.yml`
- `client/src/lib/api.ts` (`PRODUCTION_API_URL`)

## Render (backend)

1. **New Web Service** → connect repo
2. **Root directory:** `server`
3. **Runtime:** Docker (`server/Dockerfile`)
4. **Health check:** `/api/health`
5. Deploy → test:

```bash
curl https://transfera-api.onrender.com/api/health
```

Render sets `PORT` automatically; the server reads it from the environment.

## Docker logs (local or Render)

All logs go to **stdout/stderr** (line-buffered) so `docker logs` / Render **Logs** show everything.

**Build & run with live logs:**

```bash
cd server
chmod +x scripts/docker-run.sh
./scripts/docker-run.sh
```

**Manual:**

```bash
docker build -t transfera-api ./server
docker run --rm -p 8080:8008 -e PORT=8008 -e TRANSFERA_LOG_HTTP=1 -e TRANSFERA_LOG_VERBOSE=1 transfera-api
docker logs -f transfera-api   # if running detached (-d)
```

**Log types:**

| Prefix | Content |
|--------|---------|
| `[INFO]` | Startup, each HTTP request (`GET /api/health -> 200`), upload/download, P2P |
| `[ERROR]` | Bind failures, upload/download errors, httplib errors |

**Disable HTTP access log:** `TRANSFERA_LOG_HTTP=0`  
**Disable verbose (upload paths, client IP):** `TRANSFERA_LOG_VERBOSE=0`

On Render, set these under **Environment** if you want less noise (defaults in Dockerfile: both `1`).

## GitHub Pages (frontend)

Push to `main` (under `client/**`) or run **Client Deploy** workflow.

The build bakes in:

```text
NEXT_PUBLIC_API_BASE_URL=https://transfera-api.onrender.com
```

Enable **Settings → Pages → GitHub Actions** as the source.

## Local dev

```bash
cd client
cp .env.local.example .env.local   # http://127.0.0.1:8080
npm run dev
```

Start C++ API: `cd server && ./scripts/run.sh`

## Product limits (enforced in UI)

| Limit | Value | Where |
|-------|-------|--------|
| Max file size | **5 MB** | `client/src/lib/uploadValidation.ts` |
| Video files | **Blocked** | MIME `video/*` + extensions (MP4, MOV, WebM, …) |
| Max downloads per invite | **1–100** (default **1**) | Upload form field `maxDownloads` |

## Limits (Render free tier)

- Service sleeps when idle (cold start ~30–60s)
- Upload + download must hit the **same** Render instance (in-memory invite map + `/tmp` files)
- Download uses **P2P** per request (`socketpair` + `Filename:` stream); `maxDownloads` from upload (default 1) — invite and file removed after N successful downloads
- Failed downloads do not consume a download slot

## API upload example

```bash
curl -F "file=@./small.bin;filename=doc.bin" \
     -F "maxDownloads=3" \
     https://transfera-api.onrender.com/api/upload
```

## Local P2P test

```bash
cd server
./scripts/test-p2p-local.sh
MAX_DOWNLOADS=3 ./scripts/test-p2p-local.sh
```
