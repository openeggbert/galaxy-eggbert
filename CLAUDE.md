# CLAUDE.md — Galaxy Eggbert contributor instructions

## Current direction lock

Galaxy Eggbert is a faithful 3D remake of `mobile-eggbert` / *Speedy Blupi*. The sole game target
is `GalaxyEggbertCNA`: CNA is the engine/API surface and Easy3D is a small helper library beside
it. Easy3D must not hide CNA or grow into a scene graph, ECS, or replacement engine.

The retired pre-CNA implementation was removed from the live tree on 2026-07-25 after playable
parity and explicit user approval (`E3D-MIG-110..112`, `CLEANUP-S3D-001`). Its last state is
recoverable at git commit `4afd53e`. Do not resurrect that dependency path or add alternate engine
targets.

Read `CURRENT.md` first for verified current state, `NEXT.md` for detailed commands/history, and
`plan.md` for the backlog. `easy3d.md` is the historical migration analysis, not current build
guidance.

## Faithful-remake rules

- Every gameplay feature must trace to confirmed `mobile-eggbert` behavior. Do not invent
  mechanics from plausibility.
- `../mobile-eggbert` is read-only: never modify it, even temporarily. Copying code/data tables,
  enum values, byte layouts, or sprite-frame tables into this repository requires explicit
  per-instance user approval.
- Reusing approved media assets by their existing paths is allowed where already established.
  Do not add new copied asset sets without approval.
- Preserve real numeric IDs (`ObjectType`, block icons, sound channels); they encode reference
  data.
- The in-game 3D world editor is the one approved exception: editor UX may be invented, but it
  must not introduce new gameplay behavior. Its palette/order and interaction should follow the
  Eggbert 2 editor where specified, with documented Galaxy-only additions such as background
  selection.
- Real 3D worlds are hand-authored. Do not add a flat automatic 2D-to-3D world converter.

## Repository boundaries

- Do not modify sibling repositories (`../cna`, `../easy-3d`, `../easy-gl`, `../sharp-runtime`,
  `../meta-gl`, `../mobile-eggbert`) unless the user explicitly scopes that repository into the
  task.
- Keep `InteractionSystem` free of controller, camera, and graphics dependencies.
- Keep engine-independent world data under `include/GalaxyEggbert/` and
  `src/GalaxyEggbert/Worlds/`.
- Avoid backend-specific branches in game logic. `GalaxyEggbertCNA` speaks CNA directly.
- Reuse existing engine-independent code and verification tests before re-deriving math or
  reference behavior; see `missing.md`.

## Source layout

- `src/GalaxyEggbertCNA/` — thin CNA application host (`GalaxyEggbertGame`) and platform wiring.
- `include/GalaxyEggbert/`, `src/GalaxyEggbert/Worlds/` — engine-independent world/data model.
- `include/GalaxyEggbert/BlockDefinitionRegistry.hpp` — authoritative backend-independent voxel
  definition table; render/collision/gameplay consumers must not duplicate its classifications.
- `src/GalaxyEggbert/Game/` — canonical game implementation in namespace
  `GalaxyEggbert::Game`; `ObjectDefinitionRegistry` is the authoritative MoveObject type/visual
  table. Keep game-owned names free of the legacy `GE` prefix.
- `src/GalaxyEggbert/Editor/` — shared editor implementation in namespace
  `GalaxyEggbert::Editor`; keep editor-owned names free of the legacy `GE` prefix.
- `include/GalaxyEggbert/Def/` — shared game definitions in namespace `GalaxyEggbert::Def`.
- `tools/` — generators and scripted verification executables.
- `mobile-eggbert-reference/` — researched behavioral/source-of-truth notes.
- `worlds3d/` — hand-authored `.vwr` worlds.
- `Content/`, `textures3d/`, `avatars3d/` — runtime assets.

## Build and verification

Use at most two parallel compile jobs on this machine:

```bash
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON
cmake --build build-cna --target GalaxyEggbertCNA -j2
ctest --test-dir build-cna -E easy-gl-resource-smoke-tests --output-on-failure
```

`easy-gl-resource-smoke-tests` is a known external EasyGL failure and is excluded by CI. A new or
different failure is a real signal. Run the game from its build directory because runtime asset
paths are relative:

```bash
cd build-cna
./GalaxyEggbertCNA
```

For risky rendering/editor changes, add or update a pure-logic verifier when possible and perform
a proportional live/screenshot check. Do not claim visual parity from compilation alone.

## Working practices

- Preserve unrelated user changes in a dirty worktree.
- Keep changes scoped and update `CURRENT.md`, `NEXT.md`, or `plan.md` only where their documented
  ownership requires it.
- Use `-j2` maximum for every build, including packaging and cross-builds.
- Avoid redundant fresh build trees and compiler caches; SSD write volume matters on this
  workstation.
- Commit discrete completed tasks. Push only when the user explicitly requests it.
