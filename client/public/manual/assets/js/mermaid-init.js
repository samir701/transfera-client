document.addEventListener('DOMContentLoaded', function () {
  if (typeof mermaid === 'undefined') return;
  mermaid.initialize({
    startOnLoad: true,
    theme: 'dark',
    securityLevel: 'loose',
    flowchart: { curve: 'basis', htmlLabels: true },
    sequence: { mirrorActors: false, wrap: true },
  });
});
