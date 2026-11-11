# Galaxy Eggbert — web

This folder is the game for a web browser: `GalaxyEggbert.html` with `GalaxyEggbert.js`,
`GalaxyEggbert.wasm`, `GalaxyEggbert.data` (the game's data) and `Content/music/` (fetched when a
world first plays a track).

Serve the whole folder over HTTP or HTTPS and open `GalaxyEggbert.html`; opening the file straight
from disk (`file:`) does not work. To try it on your own computer:

```sh
python3 -m http.server 8000      # in this folder, then open http://localhost:8000/GalaxyEggbert.html
```

The browser needs WebGL 2 (current Firefox, Chrome, Edge, Safari). The first start downloads about
100 MB. Saved games and your own worlds stay in the browser's storage for that address. The browser
may wait for a click or a key before it plays sound. The network game is not available on the web.

`LICENSE` applies to Galaxy Eggbert; `THIRD_PARTY.txt` lists what it is built on and where its data
comes from, `RELEASE_NOTES.md` what this release holds.
