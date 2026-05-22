/** Site base path (empty on Linode root deploy; /p2p_file_sharer_in_cpp on GitHub Pages). */
export const BASE_PATH =
  process.env.NEXT_PUBLIC_BASE_PATH !== undefined
    ? process.env.NEXT_PUBLIC_BASE_PATH
    : process.env.NODE_ENV === 'production'
      ? '/p2p_file_sharer_in_cpp'
      : ''

export function sitePath(path: string): string {
  const normalized = path.startsWith('/') ? path : `/${path}`
  if (!BASE_PATH) return normalized
  return `${BASE_PATH}${normalized}`
}
