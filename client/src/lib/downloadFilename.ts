/** Parse filename from Content-Disposition (quoted or unquoted). */
export function parseContentDispositionFilename(
  contentDisposition: string
): string | null {
  if (!contentDisposition) return null;

  const star = /filename\*=(?:UTF-8''|utf-8'')([^;\s]+)/i.exec(contentDisposition);
  if (star?.[1]) {
    try {
      return decodeURIComponent(star[1].replace(/"/g, ''));
    } catch {
      return star[1];
    }
  }

  const quoted = /filename="([^"]+)"/i.exec(contentDisposition);
  if (quoted?.[1]) return quoted[1];

  const unquoted = /filename=([^;\s]+)/i.exec(contentDisposition);
  if (unquoted?.[1]) return unquoted[1].replace(/"/g, '');

  return null;
}

/** Strip server storage prefix: 32 hex chars + underscore. */
export function stripStoragePrefix(name: string): string {
  const m = /^[0-9a-f]{32}_(.+)$/i.exec(name);
  return m?.[1] ?? name;
}

/** Read a response header from axios (handles AxiosHeaders and plain objects). */
export function getResponseHeader(
  headers: Record<string, unknown> | { get?: (name: string) => string | null },
  name: string
): string {
  const lower = name.toLowerCase();
  const h = headers as {
    get?: (n: string) => string | null | undefined;
    [key: string]: unknown;
  };
  if (typeof h.get === 'function') {
    const v = h.get(name) ?? h.get(lower);
    if (typeof v === 'string') return v;
  }
  for (const key of Object.keys(h)) {
    if (key.toLowerCase() === lower && typeof h[key] === 'string') {
      return h[key] as string;
    }
  }
  return '';
}

export function resolveDownloadFilename(
  headers: Record<string, unknown> | { get?: (name: string) => string | null }
): string {
  const disposition = getResponseHeader(headers, 'content-disposition');
  const xFilename = getResponseHeader(headers, 'x-filename');

  let filename =
    parseContentDispositionFilename(disposition) ||
    (xFilename ? xFilename.trim() : '') ||
    'downloaded-file';

  filename = stripStoragePrefix(filename);
  return filename;
}
