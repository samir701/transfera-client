/** @type {import('next').NextConfig} */
const isProd = process.env.NODE_ENV === 'production'
const repo = 'p2p_file_sharer_in_cpp' // must match GitHub repo name exactly

const nextConfig = {
  reactStrictMode: true,
  output: 'export',
  images: { unoptimized: true },
  basePath: isProd ? `/${repo}` : '',
  assetPrefix: isProd ? `/${repo}/` : '',
  // Optional: proxy /api to C++ server during `next dev` if NEXT_PUBLIC_API_BASE_URL is unset
  async rewrites() {
    if (isProd) return []
    const backend = process.env.API_PROXY_TARGET || 'http://localhost:8080'
    return [
      { source: '/api/:path*', destination: `${backend}/api/:path*` },
    ]
  },
}

module.exports = nextConfig