/**
 * C++ API server base URL.
 * - Dev (next dev): set in .env.local or defaults to http://localhost:8080
 * - Static export: must point at the running C++ server (cross-origin; CORS enabled)
 */
export const API_BASE_URL =
  process.env.NEXT_PUBLIC_API_BASE_URL?.replace(/\/$/, '') ||
  'http://localhost:8080';

export function apiUrl(path: string): string {
  const normalized = path.startsWith('/') ? path : `/${path}`;
  return `${API_BASE_URL}${normalized}`;
}
