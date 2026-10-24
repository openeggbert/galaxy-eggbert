# Blupi (third-person model)

Blupi converted from the original Ray Dream scenes his Speedy Blupi sprites were rendered from,
in CNA's `SkinnedModelEXT` format, with one clip per game animation state. Generated, do not
edit by hand; regenerate with the commands below.

## Source and license

- Scenes: `speedyblupi-data/originals/Blupi/*.rd4`; `marche-profil.rd4` gives the geometry,
  skeleton and camera, every clip frame comes from the scene frame its sprite icon was matched
  to. GPLv3, © Daniel Roux & EPSITEC SA (the `speedyblupi-data` repository's README).
- Converted with `tools/rd4_to_gltf/` (its README records every verified Ray Dream convention,
  the icon matching and the per-clip comparison with the sprites) and CNA's
  `tools/avatar_asset_pipeline/convert_avatar.py`.

## Contents

- 45 bones: one per Ray Dream object of Blupi (body, nose, arm segments, hands, legs, feet,
  eyelashes), the props of the scenes the clips use (`langue`, the tongue; `Bouteille`, the
  bottle; `Sucette`, the lollipop), and `<bone>.shape` leaves carrying the parts whose scale or
  deformer changes (squashed, inflated) or which some clips hide. Every part moves rigidly with
  its bone, as in the original.
- Parts, one per colour: `BLUPI_rd_tex_Corps` (body with the face texture), `BLUPI_rd_ffff00`,
  `BLUPI_rd_000000`, `BLUPI_rd_ff0000`, `BLUPI_rd_fc7a3b`, `BLUPI_rd_00ffff`. Each part's colour is
  in its texture.
- Clips: one per `BlupiController::AnimState`, named after it (`Stop`, `March`, `Jump`, ...), plus
  `DeathLocked.<cause>` and `PickupBusy.<kind>`; each is the state's sprite icon table played at
  `kAnimFps` (8 frames per second), interpolated linearly between the matched scene frames. States
  without a clip (vehicles, Hide, Bye, two death causes) show `Stop`.
- Faces: `faces.txt` lists the face textures (`face_*.png`, the expressions of the scenes used)
  and, per clip, the face of every frame; the game draws the part named on its `part` line with
  that texture.
- Units: metres at 0.05 m per Ray Dream unit, Y up, facing +Z, feet at y = 0. The game draws it at
  0.94714 so a block keeps its 64-pixel sprite size against him (`GalaxyEggbertGame.cpp`,
  `kRealModelScale`), turned by `pi - yaw` to face the way `BlupiController` moves him.
- Triangles are clockwise, as XNA's default culling expects (`export_cna_avatar.py`;
  `upstream_bugs.md` UB-003).

## Regenerate

```bash
B=../speedyblupi-data/originals/Blupi
python3 tools/rd4_to_gltf/match_icons.py --scenes $B \
    --sheet ../mobile-eggbert/Content/icons/blupi.png --reference ../mobile-eggbert \
    --out build-probe/rd4-blupi/match
python3 tools/rd4_to_gltf/build_clip_library.py --scenes $B --base marche-profil.rd4 \
    --icons build-probe/rd4-blupi/match/icons.csv \
    --controller src/GalaxyEggbert/Game/BlupiController.cpp \
    --sheet ../mobile-eggbert/Content/icons/blupi.png --out build-probe/rd4-blupi/library/blupi.glb
python3 tools/rd4_to_gltf/export_cna_avatar.py --glb build-probe/rd4-blupi/library/blupi.glb \
    --cna-home ../../libcna/cna --out avatars3d/blupi
```
