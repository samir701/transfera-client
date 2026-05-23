# Transfera UI

Next.js static-export frontend for Transfera P2P file sharing (TypeScript + Tailwind CSS).

## Features

- Drag-and-drop upload with **5 MB** max size and **no video** files
- Configurable **max downloads (P2P)** per share (default 1, range 1–100)
- Invite code display and copy
- Download by invite code with quota-aware error messages
- Responsive layout; production API on Render (HTTPS)

## Getting started

```bash
cd client
cp .env.local.example .env.local   # http://127.0.0.1:8080 for local API
npm install
npm run dev
```

Open http://localhost:3000. Start the C++ API first: `cd server && ./scripts/run.sh`.

## Production

- **App:** https://dasanik2001.github.io/transfera-client/
- **API:** https://transfera-api.onrender.com
- **Manual:** https://dasanik2001.github.io/transfera-client/manual/

Built via GitHub Actions (`.github/workflows/client_deploy.yml`) with `NEXT_PUBLIC_API_BASE_URL` and `basePath=/transfera-client`.

## Documentation

Technical manual source: `public/manual/` — includes [features.html](public/manual/features.html) and mobile-friendly navigation.
