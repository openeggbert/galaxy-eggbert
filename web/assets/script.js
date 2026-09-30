/* Galaxy Eggbert Documentation — shared JavaScript */

(function () {
  'use strict';

  /* ── sidebar toggle (mobile) ── */
  const toggle = document.getElementById('menu-toggle');
  const sidebar = document.getElementById('sidebar');

  if (toggle && sidebar) {
    toggle.addEventListener('click', () => {
      sidebar.classList.toggle('open');
    });

    document.addEventListener('click', (e) => {
      if (sidebar.classList.contains('open') &&
          !sidebar.contains(e.target) &&
          e.target !== toggle) {
        sidebar.classList.remove('open');
      }
    });
  }

  /* ── mark active sidebar link ── */
  const links = document.querySelectorAll('#sidebar a');
  const currentPath = window.location.pathname.replace(/\/+$/, '') || '/';

  links.forEach((link) => {
    const href = link.getAttribute('href');
    if (!href) return;

    // Resolve relative href against current page URL.
    const resolved = new URL(href, window.location.href).pathname.replace(/\/+$/, '') || '/';

    if (resolved === currentPath ||
        (currentPath.endsWith('/index.html') && resolved === currentPath.replace('/index.html', '')) ||
        (resolved.endsWith('/index.html') && currentPath === resolved.replace('/index.html', ''))) {
      link.classList.add('active');
    }
  });

  /* ── simple client-side search ── */
  const searchInput = document.getElementById('search-input');
  const resultsBox  = document.getElementById('search-results');

  // Build the search index lazily from meta tags in the current page.
  const pages = [
    { title: 'Home',                   url: resolveRoot('index.html'),                     tags: 'overview galaxy eggbert home' },
    { title: 'Project Overview',       url: resolveRoot('overview.html'),                  tags: 'overview speedy blupi remake 3d' },
    { title: 'Getting Started',        url: resolveRoot('getting-started.html'),            tags: 'start install setup prerequisites' },
    { title: 'Build Guide',            url: resolveRoot('build.html'),                     tags: 'cmake build compile linux windows emscripten' },
    { title: 'Running the Game',       url: resolveRoot('running.html'),                   tags: 'run execute launch binary' },
    { title: 'Gameplay',               url: resolveRoot('gameplay.html'),                  tags: 'gameplay loop player lives treasures keys' },
    { title: 'Controls',               url: resolveRoot('controls.html'),                  tags: 'controls keyboard wasd arrows space jump' },
    { title: 'Architecture',           url: resolveRoot('architecture.html'),              tags: 'architecture design pattern class diagram' },
    { title: 'Project Structure',      url: resolveRoot('project-structure.html'),         tags: 'structure directories files layout' },
    { title: 'Engine Integration',     url: resolveRoot('engine-integration.html'),        tags: 'urho3d u3d nova3d engine backend' },
    { title: 'Assets',                 url: resolveRoot('assets.html'),                    tags: 'assets textures sprites sounds images png wav' },
    { title: 'Levels / Worlds',        url: resolveRoot('levels.html'),                    tags: 'levels worlds vwr txt format loading' },
    { title: 'Rendering',              url: resolveRoot('rendering.html'),                 tags: 'rendering 3d scene billboard skydome material' },
    { title: 'Audio',                  url: resolveRoot('audio.html'),                     tags: 'audio sound wav channels soundmanager' },
    { title: 'Input',                  url: resolveRoot('input.html'),                     tags: 'input keyboard mouse keys mapping' },
    { title: 'Game Loop',              url: resolveRoot('game-loop.html'),                 tags: 'game loop update timestep event' },
    { title: 'Save System',            url: resolveRoot('save-system.html'),               tags: 'save load gamedata 640 bytes binary' },
    { title: 'Configuration',          url: resolveRoot('configuration.html'),             tags: 'configuration cmake options settings' },
    { title: 'Platform Support',       url: resolveRoot('platform-support.html'),          tags: 'platform linux windows web android emscripten' },
    { title: 'Module Index',           url: resolveRoot('modules/index.html'),             tags: 'modules subsystems index' },
    { title: 'Class Index',            url: resolveRoot('classes/index.html'),             tags: 'classes api reference index' },
    { title: 'Systems Index',          url: resolveRoot('systems/index.html'),             tags: 'systems physics audio rendering index' },
    { title: 'Tutorials',              url: resolveRoot('tutorials/index.html'),           tags: 'tutorials how to guide step by step' },
    { title: 'Internals',              url: resolveRoot('internals/index.html'),           tags: 'internals startup lifecycle memory' },
    { title: 'Development Guide',      url: resolveRoot('development/index.html'),         tags: 'development contributing guide' },
    { title: 'Coding Style',           url: resolveRoot('development/coding-style.html'),  tags: 'coding style conventions c++ standard' },
    { title: 'Debugging',              url: resolveRoot('development/debugging.html'),     tags: 'debugging debug f1 log' },
    { title: 'Extending the Game',     url: resolveRoot('development/extending-the-game.html'), tags: 'extend add feature new object level' },
    { title: 'Known Issues',           url: resolveRoot('development/known-issues.html'),  tags: 'known issues bugs todo limitations' },
    { title: 'GalaxyEggbertApp',       url: resolveRoot('classes/GalaxyEggbertApp.html'),  tags: 'app application urho3d class' },
    { title: 'GalaxyEggbertGame',      url: resolveRoot('classes/GalaxyEggbertGame.html'), tags: 'game class coordinator scene' },
    { title: 'Blupi',                  url: resolveRoot('classes/Blupi.html'),             tags: 'blupi player character physics' },
    { title: 'Decor',                  url: resolveRoot('classes/Decor.html'),             tags: 'decor objects enemies collectibles pool' },
    { title: 'World',                  url: resolveRoot('classes/World.html'),             tags: 'world voxel chunk data model' },
    { title: 'Chunk',                  url: resolveRoot('classes/Chunk.html'),             tags: 'chunk palette bit packing voxel' },
    { title: 'GameData',               url: resolveRoot('classes/GameData.html'),          tags: 'gamedata save binary 640 bytes slots' },
    { title: 'SoundManager',           url: resolveRoot('classes/SoundManager.html'),      tags: 'sound manager audio wav channels' },
    { title: 'PhaseManager',           url: resolveRoot('classes/PhaseManager.html'),      tags: 'phase manager state machine overlay' },
    { title: 'HUD',                    url: resolveRoot('classes/HUD.html'),               tags: 'hud heads up display lives keys' },
    { title: 'CameraController',       url: resolveRoot('classes/CameraController.html'),  tags: 'camera controller 3rd person orbit' },
    { title: 'Startup Sequence',       url: resolveRoot('internals/startup.html'),         tags: 'startup init sequence flow' },
    { title: 'World Loading',          url: resolveRoot('internals/world-loading.html'),   tags: 'world loading vwr txt mobile eggbert' },
    { title: 'VWR Format',             url: resolveRoot('internals/vwr-format.html'),      tags: 'vwr binary format world file' },
    { title: 'Object Lifecycle',       url: resolveRoot('internals/object-lifecycle.html'),tags: 'object lifecycle decor pool spawn' },
    { title: 'Collision Logic',        url: resolveRoot('internals/collision.html'),       tags: 'collision aabb resolve voxel' },
    { title: 'Tutorial: Building',     url: resolveRoot('tutorials/building.html'),        tags: 'tutorial build cmake compile' },
    { title: 'Tutorial: First Object', url: resolveRoot('tutorials/first-object.html'),    tags: 'tutorial add object type gameplay' },
    { title: 'Tutorial: Modifying Levels', url: resolveRoot('tutorials/modifying-levels.html'), tags: 'tutorial modify level world txt' },
  ];

  function resolveRoot(rel) {
    // Work out how many directories deep we are.
    const path = window.location.pathname;
    // Find the /src/ segment and count depth from there.
    const parts = path.split('/').filter(Boolean);
    const srcIdx = parts.lastIndexOf('src');
    const depth = srcIdx >= 0 ? parts.length - srcIdx - 1 : 0;
    const prefix = '../'.repeat(depth);
    return prefix + rel;
  }

  if (searchInput && resultsBox) {
    searchInput.addEventListener('input', () => {
      const q = searchInput.value.trim().toLowerCase();
      resultsBox.innerHTML = '';

      if (q.length < 2) {
        resultsBox.classList.remove('open');
        return;
      }

      const matches = pages.filter((p) =>
        p.title.toLowerCase().includes(q) || p.tags.includes(q)
      ).slice(0, 8);

      if (matches.length === 0) {
        resultsBox.innerHTML = '<em style="color:var(--text-dim);font-size:.9rem">No results found.</em>';
        resultsBox.classList.add('open');
        return;
      }

      matches.forEach((p) => {
        const a = document.createElement('a');
        a.href = p.url;
        a.textContent = p.title;
        resultsBox.appendChild(a);
      });

      resultsBox.classList.add('open');
    });

    document.addEventListener('click', (e) => {
      if (!resultsBox.contains(e.target) && e.target !== searchInput) {
        resultsBox.classList.remove('open');
      }
    });
  }
})();
