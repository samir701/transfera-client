export const DEFAULT_MAX_DOWNLOADS = 1;
export const MIN_MAX_DOWNLOADS = 1;
export const MAX_MAX_DOWNLOADS = 100;

export function clampMaxDownloads(value: number): number {
  if (!Number.isFinite(value)) return DEFAULT_MAX_DOWNLOADS;
  const n = Math.trunc(value);
  if (n < MIN_MAX_DOWNLOADS) return MIN_MAX_DOWNLOADS;
  if (n > MAX_MAX_DOWNLOADS) return MAX_MAX_DOWNLOADS;
  return n;
}

export function parseMaxDownloadsInput(raw: string): number {
  const trimmed = raw.trim();
  if (!trimmed) return DEFAULT_MAX_DOWNLOADS;
  const n = Number.parseInt(trimmed, 10);
  return clampMaxDownloads(Number.isNaN(n) ? DEFAULT_MAX_DOWNLOADS : n);
}
