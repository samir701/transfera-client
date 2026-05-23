(function () {
  const PAGES = [
    { href: 'index.html', label: 'Home' },
    { href: 'features.html', label: 'Product features' },
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

  const APP_URL = 'https://dasanik2001.github.io/transfera-client/';
  const MANUAL_URL = 'https://dasanik2001.github.io/transfera-client/manual/';
  const API_URL = 'https://transfera-api.onrender.com';

  function initMobileNav() {
    const layout = document.querySelector('.layout');
    const sidebar = document.querySelector('nav.sidebar');
    if (!layout || !sidebar) return;

    const toggle = document.createElement('button');
    toggle.type = 'button';
    toggle.className = 'nav-toggle';
    toggle.setAttribute('aria-label', 'Open navigation menu');
    toggle.setAttribute('aria-expanded', 'false');
    toggle.textContent = '☰ Menu';

    const backdrop = document.createElement('button');
    backdrop.type = 'button';
    backdrop.className = 'nav-backdrop';
    backdrop.setAttribute('aria-label', 'Close navigation menu');
    backdrop.hidden = true;

    layout.insertBefore(backdrop, layout.firstChild);
    document.body.insertBefore(toggle, layout);

    function setOpen(open) {
      layout.classList.toggle('nav-open', open);
      toggle.setAttribute('aria-expanded', open ? 'true' : 'false');
      toggle.textContent = open ? '✕ Close' : '☰ Menu';
      backdrop.hidden = !open;
    }

    toggle.addEventListener('click', () => setOpen(!layout.classList.contains('nav-open')));
    backdrop.addEventListener('click', () => setOpen(false));

    sidebar.querySelectorAll('a').forEach((link) => {
      link.addEventListener('click', () => {
        if (window.matchMedia('(max-width: 768px)').matches) setOpen(false);
      });
    });

    window.addEventListener('keydown', (e) => {
      if (e.key === 'Escape') setOpen(false);
    });
  }

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
  const apiLink = document.getElementById('deploy-api-link');
  if (appLink) appLink.href = APP_URL;
  if (manualLink) manualLink.href = MANUAL_URL;
  if (apiLink) apiLink.href = API_URL + '/api/health';

  initMobileNav();
})();
