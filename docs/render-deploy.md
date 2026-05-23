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

## Limits (Render free tier)

- Service sleeps when idle (cold start ~30–60s)
- Upload + download must hit the **same** Render instance (in-memory P2P on `127.0.0.1`)
