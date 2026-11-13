# Brief: do the models get their shadows at High? (the owner, 2026-11-13)

The owner's choice: the 3D effects stay **Off by default**; check that at **High** (the Setup's
"3D effects" level 3; `GALAXY_EFFECTS=high` overrides the saved level) the models really receive and
cast the sun's shadows, fix what is clearly wrong, and show evidence.

## What exists (read first)

- `src/GalaxyEggbert/Game/SceneShaders.hpp/.cpp`: the sun's shadow map (2 cascades, 1024 texels,
  near span 44 blocks), looked up by the terrain and the models (`kMaxBones`, the model programs).
- `src/GalaxyEggbert/Game/WorldEffects.cpp`: `RenderShadowMap` (the casters: terrain, cubes, sprites
  as upright cut-outs, models within 32 / 110 blocks, translucent ones skipped) and `DrawShadows`
  (the soft round shadows when there is no map).
- `src/GalaxyEggbert/Game/WorldRenderer.cpp`: which programs draw what at each level.
- The original's look: the sprites were rendered with cast shadows (Ray Dream scenes,
  `../speedyblupi-data`, read only). The house `bd_maison`: in the sprite its window recesses are dark
  (the scene's cast shadow inside the house); `handoff.md` around "the window recess is dark in the
  sprite" and `tools/rd4_to_gltf/d11_candidates.py` (sprite vs model sheets).

## What to check, from the player's view in real campaign worlds

Find campaign worlds (`worlds3d/`) that contain the house and other big models (BigDecor: house,
cabin, fountain, column, palace pieces), objects and Blupi. Capture at High (and the same views at
Off for comparison) under Xvfb, silently (`SDL_AUDIODRIVER=dummy`), with the repo's autoplay / capture
tools (`docs/handbook/04-testing.md`, `tools/autoplay/`), from where the player actually stands (the
chase camera as it is; do not change the camera). For each, say what you see:

1. Do models receive shadows from other models and from the terrain (a model standing in the shade of
   a wall is darkened)?
2. Do models shadow themselves (the house's window recesses, an overhang, Blupi's arms on his body)?
3. Do models cast onto the terrain and onto sprites?
4. Defects: acne (striped self-shadowing), peter-panning (shadows detached from their feet), shadows
   cut off at a cascade's edge or popping when the far cascade updates every third frame, missing
   casters (a model that throws nothing), shadows through floors.

## Fixing

Fix only what is clearly a defect (a model that is not a receiver or caster where it should be, a bias
that causes acne or detaches shadows, a culling mistake), in the existing design, at High and Ultra
only: Off, Low and Medium must draw exactly as now (their golden frames must not change). If the
recess stays lit because a recess that small is under one shadow-map texel, say so with numbers and
propose (do not implement) what it would take and cost, e.g. a finer near cascade. Do not change the
default level, the camera, or the sun's direction.

## Rules

- Build only in `build-cna-opengl3` with at most 4 jobs (`export CCACHE_DIR=/rv/cnaccache
  CCACHE_BASEDIR=/rv; cmake --build build-cna-opengl3 --parallel 4`); no new build directories;
  nothing built in /tmp. `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure` must pass. If the
  High/Ultra golden frames change because of a fix, show before/after and re-baseline only those.
- Save every capture under `build-probe/shadows/` (before/ and after/ if you fix something), plus one
  side-by-side sheet per view (Off | High, and High before | after) for the owner.
- Do not commit or push; do not modify anything outside this repository; `../mobile-eggbert` and
  `../speedyblupi-data` are read only. Stop any Xvfb or game you start.
- Report: the worlds and views, the answers to 1-4 with the sheets' paths, what you fixed (files) and
  what you left with reasons.
