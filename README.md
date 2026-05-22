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

## Technical manual (GitHub Pages)

Full architecture documentation (threads, sockets, FDs, upload/download pipelines):

- **Manual:** https://dasanik2001.github.io/p2p_file_sharer_in_cpp/manual/
- **Live app:** https://dasanik2001.github.io/p2p_file_sharer_in_cpp/

Source: `client/public/manual/` (deployed with the client static build).