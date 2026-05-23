/** GitHub Pages repo name (must match GitHub repository name exactly). */
export const GITHUB_PAGES_REPO = 'transfera-client'

/** Site base path (empty on Linode root deploy; /transfera-client on GitHub Pages). */
export const BASE_PATH =
  process.env.NEXT_PUBLIC_BASE_PATH !== undefined
    ? process.env.NEXT_PUBLIC_BASE_PATH
    : process.env.NODE_ENV === 'production'
      ? `/${GITHUB_PAGES_REPO}`
      : ''

export const GITHUB_PAGES_ORIGIN = `https://dasanik2001.github.io/${GITHUB_PAGES_REPO}`

export function sitePath(path: string): string {
  const normalized = path.startsWith('/') ? path : `/${path}`
  if (!BASE_PATH) return normalized
  return `${BASE_PATH}${normalized}`
}

export function githubPagesUrl(path: string): string {
  const normalized = path.startsWith('/') ? path : `/${path}`
  return `${GITHUB_PAGES_ORIGIN}${normalized}`
}
