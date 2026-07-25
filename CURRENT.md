# Galaxy Eggbert — Current Truth

_Last verified: 2026-07-25. Update this file whenever a change alters the current state, a known
limitation, a supported build, or the actionable backlog. Keep historical detail in `NEXT.md` and
`plan.md`, not here._

## What this project is

Galaxy Eggbert is a faithful 3D remake of `mobile-eggbert` / *Speedy Blupi*. The active and sole
maintained target is **`GalaxyEggbertCNA`**, built directly on CNA with Easy3D as a small helper
library. `GalaxyEggbertSimple3D` is read-only historical reference: do not build, fix, or extend it.

The in-game 3D world editor is the one approved exception to the faithful-remake rule: it may add
content-authoring UX, but not new gameplay mechanics.

## Shipped, working state

- Playable 3D worlds: terrain, animated tiles, objects, enemies, hazards, pickups, vehicles,
  doors, switches, bridges, teleporters, HUD, sound, menus, save/progress, and hub progression.
- All approved world-editor milestones and follow-ups through `EDITOR-123` are complete: per-gamer
  custom worlds, Eggbert-ordered palette, raycast editing, object editing, undo/redo, box fill,
  functional touch deletion, verified Inverter/crate/Overcraft entries, menu-based sky-region
  thumbnails, save and play-test loop, and unsaved-change protection.
- Linux native, Vulkan native, and the debug build are maintained. A MinGW-w64 cross-build of
  `GalaxyEggbertCNA.exe` (SDL_Renderer) is confirmed. Its output now stages SDL3 and MinGW thread
  runtime DLLs beside the executable while statically linking GCC/C++. A Wine launch reaches
  window creation and asset/world loading, then correctly exposes the current blocker:
  CNA's `SDL_RENDERER` backend is 2D-only and throws on `CreateVertexBuffer`. The Web/Emscripten
  build has been verified manually, but is not part of CI or a release pipeline.
- Linux provides a reproducible `GalaxyEggbertCNA-linux-x86_64.tar.gz` runtime bundle through
  CMake's `package` target. It contains source-tracked assets and bundled SDL libraries; actual
  desktop launch remains a host-environment check, not a CI/release gate.
- GitHub Actions CI builds and tests the CNA target from a clean sibling-checkout layout. It excludes
  one known, unrelated `easy-gl` test failure.

## Verification baseline

As of the date above, rebuilding and testing the current source succeeds on all native builds:

| Build | Result |
|---|---|
| `build-cna` (EasyGL) | 84 tests pass when the known external `easy-gl-resource-smoke-tests` is excluded |
| `build-cna-vulkan` | 79/79 pass |
| `cmake-build-debug` | 81 tests pass when the known external `easy-gl-resource-smoke-tests` is excluded |

The deterministic golden-frame and behavioral-trace harnesses are manual `xvfb-run` checks, not
default CTest cases. Full commands and sibling-repository pins live in `NEXT.md` §2 and §7.

## Known limitations

- Blupi has no finished faithful 3D model; first-person uses an invisible collision point and
  third-person uses a temporary placeholder. A real model/rig needs user input.
- `easy-gl-resource-smoke-tests` fails inside the `easy-gl` sibling repository, not here; CI excludes
  it. A different test failure is a regression signal.
- Vulkan `BasicEffect` draws with `Alpha < 1` were last recorded as an upstream CNA issue; recheck
  only when Vulkan work otherwise needs a live run.
- The packaged Windows `SDL_RENDERER` build cannot run the 3D game yet: Wine reaches real startup,
  then CNA throws `SDL_Renderer does not support 3D: CreateVertexBuffer`. Choose and verify a real
  Windows 3D backend before calling this platform supported.
- Editor text is deliberately limited to functional readouts/notices (XYZ coordinates, background
  ids, and the temporary not-implemented message); the main tool palette remains icon-driven.
- Intermittent window-focus/input loss has occurred under Xvfb/live verification. It has not been
  reproduced as a Galaxy Eggbert code defect.

## Open work, in priority classes

### Active and explicitly approved

- `EDITOR-124` through `EDITOR-127`: finish the 15 remaining Eggbert 2 source-menu entries.
  This includes 6 special objects, a persisted Level start, a persisted editable BigDecor layer
  for 8 scenery entries, and an exhaustive final menu test.

### Needs user direction or visual judgement

- `AscenseurVertigo` platform geometry, `ThinMechanical` geometry, water treatment, architectural
  kit assembly, and residual seam-line transparency.
- The real Blupi model/rig and its target presentation.
- `TILE-005`: author the remaining full 3D worlds. This is valid but sizeable content work.

### Ready for an explicitly chosen engineering session

- `INFRA-006`: partial `ObjectType` dispatch migration. Seven isolated families/extractions are done;
  the remaining cases were surveyed and mostly have genuine per-type divergence. Select a specific
  candidate before changing it—do not turn it into a broad refactor.

## Non-negotiable boundaries

- Never modify `../mobile-eggbert`; treat it as read-only reference and test data.
- Do not transcribe its code, tables, enum values, or byte formats without explicit user approval.
- Do not invent gameplay mechanics; confirm every gameplay feature against the reference first.
- Keep `GEInteractionSystem` independent of `GEBlupiController`, camera, and graphics. Use its
  established event/request boundary for cross-system behavior.
- Keep the engine-agnostic `include/GalaxyEggbert/` world/data model free of CNA/Easy3D dependencies.

## Document ownership

| Document | Use it for |
|---|---|
| `CURRENT.md` | Current status, tested baseline, active limitations, and actionable work |
| `NEXT.md` | Commands, architecture notes, recent chronological write-ups, detailed constraints |
| `plan.md` | Historical implementation log, research evidence, and the complete task inventory |
| `README.md` | Project introduction and onboarding; it links here for current status |

When an entry here conflicts with old prose elsewhere, treat this file as the status source and
correct the stale document in the same change when practical.
