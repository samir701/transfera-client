/**
 * C++ API server base URL.
 * - Dev: .env.local → http://127.0.0.1:8080 (next dev rewrites /api)
 * - Production: .env.production / CI → https://transfera-api.onrender.com
 */
export const PRODUCTION_API_URL = 'https://transfera-api.onrender.com';

export const API_BASE_URL =
  process.env.NEXT_PUBLIC_API_BASE_URL?.replace(/\/$/, '') ||
  (process.env.NODE_ENV === 'production'
    ? PRODUCTION_API_URL
    : 'http://127.0.0.1:8080');

export function apiUrl(path: string): string {
  const normalized = path.startsWith('/') ? path : `/${path}`;
  return `${API_BASE_URL}${normalized}`;
}
