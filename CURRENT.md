# Galaxy Eggbert — Current Truth

_Last verified: 2026-10-20. Update this file whenever a change alters the current state, a known
limitation, a supported build, or the actionable backlog. Keep historical detail in `plan.md` (and the
frozen `NEXT.md`), where work stands in `handoff.md`, not here._

## What this project is

Galaxy Eggbert is a faithful 3D remake of `mobile-eggbert` / *Speedy Blupi*. The sole game target
is **`GalaxyEggbertCNA`**, built directly on CNA with cna-engine's strict modules as helpers (easy-3d
until 2026-10-20). The retired pre-CNA implementation was removed from the live tree on 2026-07-25
and remains available in git history at commit `4afd53e`.

The in-game 3D world editor is the one approved exception to the faithful-remake rule: it may add
content-authoring UX, but not new gameplay mechanics.

## Shipped, working state

- Playable 3D worlds: terrain, animated tiles, objects, enemies, hazards, pickups, vehicles,
  doors, switches, bridges, teleporters, HUD, sound, menus, save/progress, and hub progression.
- The canonical game implementation lives in `src/GalaxyEggbert/Game/` under
  `GalaxyEggbert::Game`, without legacy `GE` prefixes on game-owned components; the CNA-specific
  source tree now contains only the `GalaxyEggbertGame` application host/platform wiring.
- Block and MoveObject type metadata are centralized in backend-independent
  `BlockDefinitionRegistry` and `ObjectDefinitionRegistry` tables. Rendering, collision, imported
  world support, editor validation, animation, texture-sheet selection, and shared declarative
  MoveObject interaction capabilities consume these definitions instead of maintaining parallel
  icon/type classification lists.
- All approved world-editor milestones and follow-ups through `EDITOR-130` are complete: per-gamer
  custom worlds, Eggbert-ordered palette, raycast editing, object editing, undo/redo, box fill,
  functional touch deletion, verified Inverter/crate/Overcraft entries, menu-based sky-region
  thumbnails, save and play-test loop, unsaved-change protection, correctly sourced BigDecor
  scenery, a visible/Escape return from the editor browser to the main menu, and a backend-neutral
  `GalaxyEggbert::Editor` namespace/source tree without legacy `GE` prefixes on editor components.
- Linux native uses CNA's current OpenGL-family renderer identities. Windows packaging support
  stages SDL3 and MinGW thread runtime DLLs while statically linking GCC/C++, but the current
  `SDL_GPU` path still needs a fresh build and live 3D verification. The Web/Emscripten build uses
  `WEBGL2`; it is manually verified rather than part of CI or a release pipeline.
- Linux provides a reproducible `GalaxyEggbertCNA-linux-x86_64.tar.gz` runtime bundle through
  CMake's `package` target. It contains source-tracked assets and bundled SDL libraries; actual
  desktop launch remains a host-environment check, not a CI/release gate.
- GitHub Actions CI builds and tests the CNA target from a clean sibling-checkout layout. It excludes
  one known, unrelated `easy-gl` test failure.

## Verification baseline

As of the date above, rebuilding, testing, and live-running the current source succeeds for the
verified native and WebGL 2 configurations:

| Build | Result |
|---|---|
| `build-cna-opengl3` (`OPENGL33`) | Build succeeds; 93/93 CTest cases pass with the then-quarantined `easy-gl-resource-smoke-tests` excluded; normal startup and the three-frame golden capture run successfully under Xvfb (2026-10-17, before cna-engine; on 2026-10-20 an `OPENGL33` build on cna-engine ran the menus and the editor under Xvfb) |
| `OPENGLES3` on cna-engine (2026-10-20) | 93/93 CTest cases pass; golden frames, behavioral trace and startup screenshots byte-identical to the easy-3d build under Xvfb and on the GPU (`NEXT.md` §3, "Moved from easy-3d to cna-engine") |
| `build-cna-webgl2` (`WEBGL2`) | Emscripten release build succeeds and emits `.html`, `.js`, `.wasm`, and `.data`; a 12-second final-artifact headless-Chrome run creates a real WebGL 2 context, loads `world001`, and renders without an application error (2026-10-17, before cna-engine; since then only compiled with `em++`, not linked or run) |

The deterministic golden-frame and behavioral-trace harnesses are manual `xvfb-run` checks, not
default CTest cases. Full commands and sibling-repository pins live in `handoff.md`.

## Known limitations

- Third person shows the real Blupi converted from the original Ray Dream scenes
  (`avatars3d/blupi/`, `tools/rd4_to_gltf/`) with a clip, expression, props and vehicle per
  animation state, and parked vehicles as 3D models; the overcraft matches its sprites only
  roughly. The treasure, egg, level exit, drink, ammo pack and enemy projectile are 3D models
  converted the same way (`avatars3d/objects/`); the other enemies and pickups, big decor and the
  other Blupis are still billboards, being converted (`plan.md` Phase 7B, `handoff.md`); effects
  stay billboards. First person still uses an invisible collision point. The fox placeholder remains as a fallback when `avatars3d/blupi/` is absent.
- `tests/golden/` no longer matches `develop` with CNA `900d0db3b` (the trace runs one tick ahead);
  the references need regenerating once the CNA revision is settled.
- The current Windows `SDL_GPU` path has not yet had a fresh cross-build and live 3D run; do not
  call Windows supported until both pass.
- Editor text is deliberately limited to functional readouts/notices (XYZ coordinates, background
  ids, and the temporary not-implemented message); the main tool palette remains icon-driven.
- Intermittent window-focus/input loss has occurred under Xvfb/live verification. It has not been
  reproduced as a Galaxy Eggbert code defect.

## Open work, in priority classes

### Active and explicitly approved

- None. The approved editor menu completion work is finished.

### Needs user direction or visual judgement

- `AscenseurVertigo` platform geometry, `ThinMechanical` geometry, water treatment, architectural
  kit assembly, and residual seam-line transparency.
- The real Blupi model/rig and its target presentation.
- `TILE-005`: author the remaining full 3D worlds. This is valid but sizeable content work.

### Ready for an explicitly chosen engineering session

- None. `INFRA-006`'s final ObjectType audit is complete: pure type membership and parameter rules
  are registry-backed, while the retained branches are distinct state machines or explicitly scoped
  cheat commands. Choose a new feature or content task before changing gameplay architecture.

## Non-negotiable boundaries

- Never modify `../mobile-eggbert`; treat it as read-only reference and test data.
- Do not transcribe its code, tables, enum values, or byte formats without explicit user approval.
- Do not invent gameplay mechanics; confirm every gameplay feature against the reference first.
- Keep `InteractionSystem` independent of `BlupiController`, camera, and graphics. Use its
  established event/request boundary for cross-system behavior.
- Keep the engine-agnostic `include/GalaxyEggbert/` world/data model free of CNA/cna-engine dependencies.

## Document ownership

| Document | Use it for |
|---|---|
| `CURRENT.md` | Current status, tested baseline, active limitations, and actionable work |
| `handoff.md` | Where work stands, what to run next, commands, sibling pins, gotchas; updated after every task |
| `NEXT.md` | Obsolete since 2026-10-24: frozen chronological write-ups and architecture notes up to then |
| `plan.md` | Historical implementation log, research evidence, and the complete task inventory |
| `README.md` | Project introduction and onboarding; it links here for current status |

When an entry here conflicts with old prose elsewhere, treat this file as the status source and
correct the stale document in the same change when practical.
