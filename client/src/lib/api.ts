/**
 * C++ API server base URL.
 * - Dev (next dev): .env.local or http://127.0.0.1:8080; rewrites proxy /api in dev
 * - Linode: https://your-host (same origin; Caddy proxies /api → :8080)
 * - GitHub Pages: https://your-api-host (HTTPS required; CORS enabled)
 */
export const API_BASE_URL =
  process.env.NEXT_PUBLIC_API_BASE_URL?.replace(/\/$/, '') ||
  'http://127.0.0.1:8080';

export function apiUrl(path: string): string {
  const normalized = path.startsWith('/') ? path : `/${path}`;
  return `${API_BASE_URL}${normalized}`;
}
