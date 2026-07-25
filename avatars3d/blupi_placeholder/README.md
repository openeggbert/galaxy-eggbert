# Placeholder third-person model (NOT Blupi)

Temporary stand-in used by `GalaxyEggbertCNA`'s third-person camera mode
(NEXT.md §3, 2026-07-09) until a real 3D Blupi model with real animations
exists. Not a mobile-eggbert asset, not part of the faithful-remake asset
set — purely an engine-plumbing placeholder, swap out wholesale once a real
model is ready.

## Source and license

Converted from Khronos Group's glTF Sample Assets "Fox" model
(https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/Fox) via
CNA's own `tools/avatar_asset_pipeline/convert_avatar.py --embedded-clips`
(see `../../cna/docs/avatar-real-rendering-ext.md`). The original glTF's
single mesh primitive had no index accessor (non-indexed geometry), which
the converter doesn't handle — patched with a synthetic sequential index
buffer (0..N-1, matching the implicit non-indexed vertex order, no geometry
change) before conversion, not by modifying the converter itself.

- Model (low-poly fox mesh): CC0 1.0 Universal, by PixelMannen
  (https://opengameart.org/content/fox-and-shiba)
- Rigging and animation: CC BY 4.0, by @tomkranis on Sketchfab
  (https://sketchfab.com/models/371dea88d7e04a76af5763f2a36866bc) —
  **attribution required** for the rig/animation portion specifically.
- glTF conversion: by @AsoboStudio and @scurest

## Contents

- `avatar.skinnedmodel.json` / `skeleton.bin` / `fox1.*` — the mesh + skeleton
  (24 bones), CNA's native `SkinnedModelEXT` format.
- `clips/Survey.clip.bin`, `clips/Walk.clip.bin`, `clips/Run.clip.bin` — the
  3 animation clips embedded in the original glTF, converted as-is.

## Animation mapping (placeholder, best-effort)

`BlupiController::AnimState` has no equivalent to this fox's 3 clips —
`GalaxyEggbertGame`'s mapping (documented in its own source) is a rough
placeholder, not a faithful behavioral match:

| AnimState | Clip used | Note |
|---|---|---|
| Stop | Survey | closest to an idle pose |
| March | Walk | direct match |
| Jump | Run | no jump clip exists; closest available motion |
| Down (crouch) | Survey | no crouch clip exists |
| Up (look up) | Survey | no look-up clip exists |
