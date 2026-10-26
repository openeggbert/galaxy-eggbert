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

- 56 bones (CNA's skinning takes up to 72): one per Ray Dream object of Blupi (body, nose, arm
  segments, hands, legs, feet, eyelashes), the props of the scenes the clips use (`langue`, the
  tongue; `Bouteille`, the bottle; `Sucette`, the lollipop; `Corps et Cube`, the half body the
  saw leaves in Clear4), and `<bone>.shape` leaves carrying
  the parts whose scale or deformer changes (squashed, inflated) or which some clips hide. Every
  part moves rigidly with its bone, as in the original.
- Parts, one per colour or texture: `BLUPI_rd_tex_Corps` (body with the face texture), the
  lollipop's striped `BLUPI_rd_tex_Sucette_Cylindre` and `BLUPI_rd_tex_Sucette_Groupe5_Cylindre`,
  the saw death's half body with its half face `BLUPI_rd_tex_Corps_et_Cube`, `BLUPI_rd_ffff00`,
  `BLUPI_rd_000000`, `BLUPI_rd_ff0000`, `BLUPI_rd_fc7a3b`, `BLUPI_rd_00ffff`. Each part's colour
  is in its texture.
- Clips: one per `BlupiController::AnimState`, named after it (`Stop`, `March`, `Jump`, ...), plus
  `DeathLocked.<cause>` and `PickupBusy.<kind>`; each is the state's sprite icon table played at
  `kAnimFps` (20 frames per second, as in the original), interpolated linearly between the matched
  scene frames; a frame matched turned (front and back views, Hide's somersault) turns the whole
  pose. A state whose icons the saw death's half-body scenes match better still takes the
  best whole-body frame within 0.05 of it (`--whole-icons`), so Stop and the others built from
  them keep both halves of Blupi. Clear2 is invisible throughout, as in the original.
- `clips.txt` lists the face textures (`face_*.png`, the expressions of the scenes used) and, per
  clip, the face of every frame; the game draws the part named on its `part` line with that
  texture. Its `viewer` lines name the clips that turn Blupi toward the sprites' viewer (his
  right, where the original camera stood); the third-person camera goes to that side for them.
- Units: metres at 0.05 m per Ray Dream unit, Y up, facing +Z, feet at y = 0. The game draws it at
  0.94714 so a block keeps its 64-pixel sprite size against him (`GalaxyEggbertGame.cpp`,
  `kRealModelScale`), turned by `pi - yaw` to face the way `BlupiController` moves him. Standing,
  he is then 0.742 blocks from his feet to the top of his body and 0.80 with his eyelashes, as
  in the sprites: 48 and 51 of a block's 64 pixels in `blupi.png`'s standing icon.
- Triangles are clockwise, as XNA's default culling expects (`export_cna_avatar.py`;
  `upstream_bugs.md` UB-003).
- `vehicles/<name>/`: the helicopter, jeep, tank, overcraft and skateboard, each a model of its
  own with the vehicle clips' names (`StopJeep`, `MarchJeep`, ...), built from the same scene
  frames in the same space; the game draws the one whose clip is playing with Blupi's transform
  and clip time, and draws parked vehicle pickups with them too (standing pose). Listed in
  `clips.txt` (`vehicle` lines).

## Regenerate

```bash
B=../speedyblupi-data/originals/Blupi
python3 tools/rd4_to_gltf/match_icons.py --scenes $B \
    --sheet ../mobile-eggbert/Content/icons/blupi.png --reference ../mobile-eggbert \
    --out build-probe/rd4-blupi/match
python3 tools/rd4_to_gltf/match_icons.py --scenes $B \
    --sheet ../mobile-eggbert/Content/icons/blupi.png --reference ../mobile-eggbert \
    --exclude blupi-scie.rds,blupi-demi.rds --yaw-step 0 --tumble "" \
    --out build-probe/rd4-blupi/match-whole
python3 tools/rd4_to_gltf/build_clip_library.py --scenes $B --base marche-profil.rd4 \
    --icons build-probe/rd4-blupi/match/icons.csv \
    --whole-icons build-probe/rd4-blupi/match-whole/icons.csv \
    --controller src/GalaxyEggbert/Game/BlupiController.cpp \
    --sheet ../mobile-eggbert/Content/icons/blupi.png --out build-probe/rd4-blupi/library/blupi.glb
python3 tools/rd4_to_gltf/export_cna_avatar.py --glb build-probe/rd4-blupi/library/blupi.glb \
    --cna-home ../../libcna/cna --out avatars3d/blupi
# The vehicles toned to Blupi's sprites (each scene's own lights greyed them; fitted by
# calibrate_vehicle_tone.py: helicopter 25.6 -> 11.4, overcraft 27.6 -> 11.4, tank 28.9 -> 19.1,
# jeep 7.7 -> 4.3 colour error; the skateboard gains too little and keeps none).
for vt in helico:0.71,50 jeep:0.93,-4 tank:0.94,31 over:0.59,41 skate:; do
    v=${vt%%:*}; t=${vt#*:}
    python3 tools/rd4_to_gltf/export_cna_avatar.py --glb build-probe/rd4-blupi/library/vehicles/$v.glb \
        --cna-home ../../libcna/cna --out avatars3d/blupi/vehicles/$v ${t:+--tone $t}
done
```
