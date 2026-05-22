(function () {
  const PAGES = [
    { href: 'index.html', label: 'Home' },
    { href: 'architecture.html', label: 'System architecture' },
    { href: 'frontend.html', label: 'Frontend (Next.js)' },
    { href: 'http-server.html', label: 'HTTP server (FileController)' },
    { href: 'main-lifecycle.html', label: 'main() & process lifecycle' },
    { href: 'threads.html', label: 'Threads & flowcharts' },
    { href: 'sockets-fds.html', label: 'Sockets & file descriptors' },
    { href: 'upload-flow.html', label: 'Upload pipeline' },
    { href: 'download-flow.html', label: 'Download pipeline' },
    { href: 'p2p-sharer.html', label: 'P2P FileSharer' },
    { href: 'syntax.html', label: 'Syntax glossary' },
    { href: 'operations.html', label: 'Operations & troubleshooting' },
  ];

  const APP_URL = 'https://dasanik2001.github.io/p2p_file_sharer_in_cpp/';
  const MANUAL_URL = 'https://dasanik2001.github.io/p2p_file_sharer_in_cpp/manual/';

  const navEl = document.getElementById('manual-nav-list');
  if (navEl) {
    const current = location.pathname.split('/').pop() || 'index.html';
    PAGES.forEach((p) => {
      const li = document.createElement('li');
      const a = document.createElement('a');
      a.href = p.href;
      a.textContent = p.label;
      if (p.href === current || (current === '' && p.href === 'index.html')) {
        a.classList.add('active');
      }
      li.appendChild(a);
      navEl.appendChild(li);
    });
  }

  const appLink = document.getElementById('deploy-app-link');
  const manualLink = document.getElementById('deploy-manual-link');
  if (appLink) appLink.href = APP_URL;
  if (manualLink) manualLink.href = MANUAL_URL;
})();
