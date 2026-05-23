/** @type {import('next').NextConfig} */
const isProd = process.env.NODE_ENV === 'production'
const repo = 'transfera-client' // GitHub Pages repo name

// Linode / same-origin: NEXT_PUBLIC_BASE_PATH= (empty)
// GitHub Pages: unset in prod → /transfera-client, or set explicitly in CI
const basePath =
  process.env.NEXT_PUBLIC_BASE_PATH !== undefined
    ? process.env.NEXT_PUBLIC_BASE_PATH
    : isProd
      ? `/${repo}`
      : ''

const assetPrefix = basePath ? `${basePath}/` : undefined

const nextConfig = {
  reactStrictMode: true,
  output: 'export',
  images: { unoptimized: true },
  basePath: basePath || undefined,
  assetPrefix,
  async rewrites() {
    if (isProd) return []
    const backend = process.env.API_PROXY_TARGET || 'http://localhost:8080'
    return [{ source: '/api/:path*', destination: `${backend}/api/:path*` }]
  },
}

module.exports = nextConfig
