# Blupi (third-person model)

Blupi converted from the original Ray Dream scenes his Speedy Blupi sprites were rendered from,
in CNA's `SkinnedModelEXT` format. Generated, do not edit by hand; regenerate with the commands
below.

## Source and license

- Scenes: `speedyblupi-data/originals/Blupi/marche-profil.rd4` (walk, geometry, face) and
  `stop-profil.rd4` (standing pose). GPLv3, © Daniel Roux & EPSITEC SA (the `speedyblupi-data`
  repository's README).
- Converted with `tools/rd4_to_gltf/` (see its README for every verified Ray Dream convention and
  the sprite comparisons) and CNA's `tools/avatar_asset_pipeline/convert_avatar.py`.

## Contents

- 21 bones, one per Ray Dream object (body, nose, arm segments, hands, legs, feet, eyelashes);
  every part moves rigidly with its bone, as in the original.
- Parts: `BLUPI_rd_tex_Corps` (body with the face texture), `BLUPI_rd_ffff00` (yellow parts),
  `BLUPI_rd_000000` (eyelashes). Each part's colour is in its texture.
- Clips, timed at the game's sprite cadence (`BlupiController::kAnimFps`, 8 frames per second, one
  rendered Ray Dream frame each):
  - `March` — the walk cycle, 6 frames (sprite icons 5-10), 0.75 s;
  - `Stop` — the standing pose (sprite icon 0), static.
- Units: metres at 0.05 m per Ray Dream unit, Y up, facing +Z, feet at y = 0. The game draws it at
  0.94714 so a block keeps its 64-pixel sprite size against him (`GalaxyEggbertGame.cpp`,
  `kRealModelScale`), turned by `pi - yaw` to face the way `BlupiController` moves him.
- Triangles are clockwise, as XNA's default culling expects (`export_cna_avatar.py`;
  `upstream_bugs.md` UB-003).

## Regenerate

```bash
B=../speedyblupi-data/originals/Blupi
python3 tools/rd4_to_gltf/rd4_to_gltf.py --scene $B/marche-profil.rd4 \
    --clip March=$B/marche-profil.rd4 --clip Stop=$B/stop-profil.rd4@0 \
    --frame-rate 8 --out build-probe/rd4-blupi/blupi.glb
python3 tools/rd4_to_gltf/export_cna_avatar.py --glb build-probe/rd4-blupi/blupi.glb \
    --cna-home ../../libcna/cna --out avatars3d/blupi
```

Every other animation state shows `Stop` until its scene is converted.
