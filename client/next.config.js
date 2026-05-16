/** @type {import('next').NextConfig} */
const isProd = process.env.NODE_ENV === 'production'
const repo = 'PeerLink' // your repo name (for project Pages)

const nextConfig = {
  reactStrictMode: true,
  output: 'export',
  images: { unoptimized: true },
  // Only if the site is NOT at https://<user>.github.io/ (project site):
  basePath: isProd ? `/${repo}` : '',
  assetPrefix: isProd ? `/${repo}/` : '',
  // Remove or replace rewrites — they don't work on static hosting.
  // Point the UI at a real API URL via env vars instead.
}

module.exports = nextConfig