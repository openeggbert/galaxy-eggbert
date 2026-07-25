# NEXT.md — Galaxy Eggbert

_Session paused 2026-07-23 (updated): since the note below was first written, a separate
user-directed pass closed out the `INFRA-*` doc-audit follow-ups and `BUILD-009` (CI). Concretely:
the P0-1 behavioral trace, `INFRA-003`'s programmatic reference-doc check, and `INFRA-005`'s merged
X+Z+Y resolve were all confirmed genuinely done (not overclaims); one real doc mismatch (24 vs 25
rows) was fixed; golden-capture/trace were confirmed to actually run in this container via
`xvfb-run`; and `BUILD-009` (GitHub Actions CI for `GalaxyEggbertCNA`) was built, live-debugged
through 4 real pushes (abbreviated-SHA, missing `meta-gl` sibling, missing `mobile-eggbert` test
data — see §3), and confirmed fully green on a real run. Two items from that pass remain
deliberately open, not silently closed: `INFRA-004`'s unverified-icon table is still bookkeeping
only, not wired into the renderer/CI as an enforcement gate; `INFRA-006` is now complete after its
final behavior audit. Its former pure cross-cutting membership lists and remaining uniform
parameters now live in `ObjectDefinitionRegistry`; the deliberately retained conditions are
state-machine branches or narrowly specified cheats, not uncentralized type classifications.
`INFRA-004` remains intentionally untouched.

Everything else in the backlog still needs the user's own input before proceeding. The user will
author `TILE-005`'s real 3D worlds manually in the finished in-game editor, so do not generate or
pre-author those levels without a later explicit request. The remaining engineering candidates need
visual/design judgment (`AscenseurVertigo`/`Suspended` render geometry, seam-line transparency) or
an asset (the real Blupi model/rig). The code-quality/edge-case audit category is confirmed
exhausted for the prior session (8 rounds, last one genuinely clean — see §3/§10). Do not
manufacture more audit rounds by default; ask the user which backlog item to pick up next, or wait
for new direction._

_Last updated: 2026-07-25. The in-game 3D world editor's original plan is **COMPLETE**: all 13
approved milestones (EDITOR-100 through EDITOR-112) are implemented and verified. User-directed
follow-up work through EDITOR-130 is also complete, including the Eggbert-ordered palette cleanup,
XYZ placement controls/readout, object cell alignment, the Galaxy-only background-thumbnail menu,
functional touch deletion, all 96 source mappings, persisted Level start, editable BigDecor,
correct `explo.png` scenery rendering, and a world-browser return to the main menu. Editor work resumed
2026-07-23 by explicit user authorization (was paused 2026-07-19 — see
plan.md §6's
own status note for the full history), then finished the same session: EDITOR-111 (sky-region
picker) and EDITOR-112 (unsaved-changes guard + a boundary-straddling box-fill test + section
consolidation) both landed, verified on all 3 native backends. The keyboard-input/window-focus
concern noted when work was paused is still unresolved — re-confirmed twice more this same session
(once before resuming, once again during EDITOR-112's own live-verification attempt, where BOTH
keyboard and mouse input stopped reaching the game entirely, despite working fine for EDITOR-111's
live check earlier the same session) — genuinely intermittent, not a code bug, see plan.md §6's
"Known problems" for the full history. No work remains on the original approved plan; any further
editor work is a separately selected follow-up. `EDITOR-127` is now complete: exhaustive static
and real-pointer checks cover all 96 source cells, the full suite is clean, and the final 800×480
editor menu was inspected from a live capture. The subsequent live-reported Palmtree blue-tile
regression and missing browser Back path are fixed as EDITOR-128/129._

_Architecture cleanup 2026-07-25: `E3D-MIG-110..112` / `CLEANUP-S3D-001` is complete. The
retired pre-CNA source tree, its CMake option/targets, and its two obsolete verifier tools were
removed after CNA reached playable parity. The last pre-removal state is recoverable at git commit
`4afd53e`; a clean CMake inventory contains no retired target, the full `-j2` build succeeds, and
all 90 applicable CTest tests pass. The ignored 277MB legacy `build/` tree was also removed;
historical entries below are intentionally retained as migration history._

_Definition cleanup 2026-07-25: `INFRA-011` is complete. The 12 shared definition headers moved
from `include/GalaxyEggbert/def/` to `include/GalaxyEggbert/Def/`, and their declarations now live
in `GalaxyEggbert::Def`. All consumers use the new namespace directly, without legacy aliases.
The duplicate controller-local `SecretPower` enum was removed in favor of the shared definition.
The full `-j2` build succeeds and all 90 applicable CTest tests pass._

_Editor ownership cleanup 2026-07-25: `EDITOR-130` is complete. All 25 editor implementation
files moved from `src/GalaxyEggbertCNA/Editor/` to `src/GalaxyEggbert/Editor/`; editor declarations
now live in `GalaxyEggbert::Editor`, and editor-owned filenames/types no longer carry the legacy
`GE` prefix (`WorldEditor`, `EditCommandStack`, `EditorPalette`, `BoxRegion`, and so on). The
game-layer names used by that tree are explicit dependencies, not editor ownership. The full `-j2`
build succeeds and all 90 applicable CTest tests pass._

_Game ownership cleanup 2026-07-25: `INFRA-012` is complete. All 45 game implementation files
moved from `src/GalaxyEggbertCNA/Game/` to `src/GalaxyEggbert/Game/`; their declarations now live
in `GalaxyEggbert::Game` (including `GalaxyEggbert::Game::QuadBatch`), and game-owned filenames,
classes, and namespaces no longer carry the legacy `GE` prefix (`BlupiController`, `WorldRuntime`,
`TerrainRenderer`, `InputPad`, `InteractionSystem`, `SaveData`, `Sound`, `Hud`, and so on).
`src/GalaxyEggbertCNA/` now contains only the CNA application host and entry point. The full `-j2`
build succeeds and all 90 applicable CTest tests pass._

_CNA host naming cleanup 2026-07-25: `INFRA-013` is complete. The thin application-host class and
files are now named `GalaxyEggbertGame`; the redundant `Cna` fragment was removed because the
enclosing `GalaxyEggbert::CNA` namespace and `src/GalaxyEggbertCNA/` path already state the backend.
The executable target remains `GalaxyEggbertCNA`. CMake, the entry point, documentation, comments,
and type registration use the new name. The full `-j2` build succeeds and all 90 applicable CTest
tests pass._

_Central definition cleanup 2026-07-25: `INFRA-014/015/016` are complete.
`BlockDefinitionRegistry` is now the authoritative CNA-independent definition of all supported
voxel ids, including render mode, texture, animation, collision, alpha, and gameplay semantics.
`ObjectDefinitionRegistry` likewise covers `ObjectType0..203`, including placement, semantic and
 patrol policy, shared declarative interaction capabilities, and phase/override-aware resolved
 visuals. Terrain/game/editor consumers use these registries; old `BlockTypes`/`ObjectIcons`
 classification helpers are thin compatibility accessors.
The new exhaustive `VerifyDefinitionRegistries` test covers the complete domains and parity
decisions. The full `-j2` build succeeds and all 91 applicable CTest tests pass._

_`INFRA-006` final audit 2026-07-25 — complete._ The original instruction to replace every
`ObjectType` condition with a generic handler table was narrowed to the only safe interpretation:
centralize pure membership/constant rules, but preserve distinct state machines as readable code.
The audit migrated the shared small-enemy set (`4/32/33`) for Cloud aura and Perso-decoy detection,
the existing avatar semantic category (`200..203`) for the other side of that trap, generic-hazard
feedback (Small/Big shake plus the matching explosion), the type-3 crouch exception, lethal-decor
contact (`201..203`), level-exit membership (`7/21`), and the signed conveyor direction of lifts
`47/48`. `VerifyDefinitionRegistries` exhaustively checks every `ObjectType0..203`; existing
interaction tests exercise each consuming mechanism.

The remaining explicit conditions are intentionally retained: door slide, dynamite fuse/blast
sequencing, bridge construction, follower wake/homing, Blupih/Blupit firing, projectile contact,
wasp balloon interaction, large-creature turn-state contact, and mockery-taunt selection are
different state machines or per-type control flow, not constant dispatch. `TryPerso` and the
normal-exit/treasure/clean-all cheats are likewise single-purpose commands, not parallel runtime
classification lists. No generic handler abstraction is justified after this audit._

_2026-07-20 update: the Saw blade render-orientation bug (§4/§5/§8/§9's own old entries) is now
**resolved** — see §3's own writeup for the full 6-round history. `plan.md` §7 ("Correctness
Infrastructure & Dual-Renderer — Vision") is new: a non-binding assessment of merged
`REMAKE-ANALYSIS.md`/`renderers.md` material, read it before starting anything framed as
"modernization" or "reduce the bug factory."_

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert**, itself a C++ port of *Speedy
Blupi*, a 2013 Windows Phone XNA game. The main goal is to reimplement mobile-eggbert's exact game
logic, levels, and assets in 3D (perspective camera, billboard sprites, 3D-rendered tiles)
**without inventing new mechanics** — every feature must trace to a confirmed mobile-eggbert
behavior (see `CLAUDE.md`'s "Faithful Remake" rule).

**Current development phase:** `GalaxyEggbertCNA` is the sole actively-developed build target. It
has reached a genuinely playable state: real terrain, objects, enemies, hazards, HUD, sound,
save system, and menus all work. The single largest remaining gap is that Blupi himself has no
visible 3D model yet (invisible collision point in first-person, a temporary placeholder model in
third-person).

**In-game 3D world editor (COMPLETE, see plan.md §6):** `GamePhase::Editor`,
`src/GalaxyEggbert/Editor/`, a user-requested feature letting each player create, edit, save
and play-test their own `.vwr` worlds — inspired by free-eggbert's "Own mission" editor. The user
approved a 13-milestone plan (EDITOR-100..112); all 13 are done, and later user-directed
follow-ups through `EDITOR-130` are implemented — see plan.md §6 for the full write-up (what's
built, and a real open concern about keyboard input
possibly being a window-focus issue, re-confirmed twice more as of 2026-07-23, still not a code bug,
not yet confirmed fixed on a real desktop).
Note this editor is an explicit, user-approved **exception** to the faithful-remake rule (see
`plan.md` §6): it is content-creation tooling, not a mobile-eggbert feature, so inventing editor
UX is fine — inventing new *gameplay* mechanics is still not.

**Important architectural decisions:**
- **Direct CNA + Easy3D is the locked, sole target** (decided 2026-07-05). The retired pre-CNA
  implementation was removed from the live tree on 2026-07-25 and remains in git history at
  `4afd53e`; do not resurrect its dependency path.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching) — it must not
  hide CNA; game code calls CNA directly.
- `../mobile-eggbert` is **read-only, never modified, even temporarily**. Assets (PNGs, sounds,
  world files) are freely reused by direct path; code/data (tables, enum values, byte layouts) are
  reference-only and require explicit user approval to transcribe.
- No `#ifdef` guards for hypothetical engine differences — the game speaks CNA directly.
- Worlds are hand-authored via `tools/GenerateSampleWorld3D.cpp` (writes all 79 `worlds3d/*.vwr`
  files, including `world999.vwr`, this engine's own quarantined mechanics-showcase/test world);
  there is no automatic 2D→3D world converter and none is planned.
- `SaveData`/`InputPad`/`InteractionSystem` are deliberately **not** byte-compatible with
  real mobile-eggbert's own formats — real *behavior* is ported faithfully, byte layout is not.

## 2. Current status

### Build status
`build-cna` builds cleanly and was re-verified after every editor milestone (2026-07-18).
`build-cna-vulkan` was rebuilt and re-tested 2026-07-19 (through the skate/tank animation-wiring
work) — 76/76 (100%) tests pass, cleaner than `build-cna`'s own 78/79 (the unrelated `easy-gl`
smoke-test failure doesn't reproduce there, presumably a different target set). The editor's own
code is confirmed backend-agnostic through this.
- `build-cna/` — EasyGL backend (`CNA_GRAPHICS_BACKEND=EASYGL`, the default — switched back from
  Vulkan 2026-07-18, see §3; still fully overridable at configure time).
- `build-cna-vulkan/` — Vulkan backend (`CNA_GRAPHICS_BACKEND=VULKAN`). Has a known, unrelated CNA
  engine bug: `BasicEffect` draws with `Alpha < 1` don't render at all under this backend (see §5;
  not independently re-verified this session — status as last checked). The second real bug found
  the same day (missing NDC Y-flip in `SkinnedEffect` shaders, making the third-person placeholder
  model float) — **confirmed committed in `../cna`** (checked 2026-07-21, `INFRA-009`: all 4
  originally-reported Vulkan `skinned3d*.vert.glsl` shaders carry the `gl_Position.y = -gl_Position.y`
  fix line; this had previously sat uncommitted, see `plan.md` for the earlier writeup). Note in
  passing (not otherwise investigated, out of `INFRA-009`'s bookkeeping scope): `pbr3d_skinned.vert.glsl`
  does not have the same line — unclear if that effect is on `AvatarRenderer`'s actual render path
  or a separate, unrelated pipeline; flag if a floating-model bug ever resurfaces specifically under
  a PBR-skinned material.
### Sibling-repo commit pins (`INFRA-009`, plan.md §7)

Galaxy Eggbert depends on 5 sibling repositories, each under independent development (see the
`sharp-runtime` note above) — the commits below are what this repo was last verified to build/pass
against. Update this list whenever a sibling repo is rebuilt against (a full local rebuild + `ctest`
pass), not on every unrelated galaxy-eggbert commit.

| Repo | Commit (as of 2026-07-21) | Date |
|---|---|---|
| `../easy-3d` | `e2d1cfa2` | 2026-07-11 |
| `../cna` | `ac3aaaeb` | 2026-07-18 |
| `../easy-gl` | `62c0a248` | 2026-07-19 |
| `../sharp-runtime` | `d2cc9cce` | 2026-07-16 |
| `../meta-gl` | `d51fcd7f` | 2026-07-18 |

`../meta-gl` was added to this table 2026-07-23, found missing while setting up `BUILD-009`'s CI
(`../easy-gl/CMakeLists.txt` unconditionally `add_subdirectory(../meta-gl)` — a genuine transitive
build dependency of `../easy-gl`, not previously tracked here even though it's always been required
locally too).

`../mobile-eggbert` is deliberately NOT in the table above — it's a **test-data**, not build,
dependency: `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` read real
`../mobile-eggbert/worlds/*.txt` files directly (found missing the same way, 2026-07-23, when
`BUILD-009`'s CI ran `ctest` for the first time against a fresh checkout with no `../mobile-eggbert`
at all). `cna-ci.yml` sparse-checks-out only `worlds/` (a few MB, not the full ~2.3 GB repo —
`Content/` is irrelevant to these 2 tests), pinned to `07e0a673`, 2026-07-16.

**Known upstream failures, quarantined (not Galaxy Eggbert regressions if they reproduce again):**
- `easy-gl-resource-smoke-tests` (ctest) — root-caused entirely in `../easy-gl`
  (`test_texture_upload_sets_unpack_alignment_wrap_and_unit0_binding`, see §5 for the full
  root-cause writeup). Every full `ctest` run this session shows exactly this one failure and no
  others — if a future run shows a *different* failure count/name here, treat that as a real signal
  worth investigating, not more of the same known issue.
- `build-cna-vulkan`'s `BasicEffect` `Alpha < 1` non-render bug (see §5/above) — CNA engine bug, not
  galaxy-eggbert. Still open as far as this repo's records show; worth a fresh live check next time
  `build-cna-vulkan` is touched for an unrelated reason, since `../cna`'s blend-state handling has
  changed since this was first found (e.g. `ddef5e8f`, a Vulkan blend-function-mapping fix already
  merged before this bug was even reported) — plausible but **not confirmed** to be related.

### Web build (Emscripten/WebAssembly, 2026-07-17)
`GalaxyEggbertCNA` also builds and runs in-browser via Emscripten — see `plan.md` `BUILD-003` for
the full writeup. Configure/build:
```bash
source <path-to-emsdk>/emsdk_env.sh
cmake -S . -B build-web \
  -DCMAKE_TOOLCHAIN_FILE=<path-to-emsdk>/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
  -DCMAKE_BUILD_TYPE=Release -DGALAXY_EGGBERT_BUILD_CNA=ON
cmake --build build-web --target GalaxyEggbertCNA -j2
```
Produces `build-web/GalaxyEggbertCNA.{html,js,wasm,data}` — serve that directory over plain HTTP
(not `file://`, browsers block `.wasm`/`.data` fetches from local files) and open the `.html`.
`CNA_ENABLE_NET` must stay `ON` (the default) even for the web build — `GalaxyEggbertGame.cpp`
unconditionally references `AvatarRenderer` symbols from `CNA_GamerServices`, which is gated behind
it. Verified working via a real headless-Chrome/WebGL2 run (see `BUILD-003`); it is not wired into
a hosting pipeline, so publishing the built files remains a manual step.

**Re-verified 2026-07-19** (user request: rebuild for their own site) — `build-web` was stale
(last built 2026-07-17, before EDITOR-100..109 landed 2026-07-18); the editor sources were already
wired into the `EMSCRIPTEN` branch of the `GalaxyEggbertCNA` target in `CMakeLists.txt`, so this
was a plain reconfigure + rebuild, no CMake changes needed. Rebuilt cleanly (`-j2`, only
pre-existing harmless warnings), produced a fresh ~24MB `.data` + `.html`/`.js`/`.wasm`, and
re-verified with a real headless-Chrome/WebGL2 run driven over the DevTools protocol (served from
`build-web/` over local HTTP): console showed a real WebGL2 context, `world001.vwr` loading,
terrain/background/sound assets loading (93/93 sound channels), a terrain mesh uploading, and a
screenshot confirmed the real Init/gamer-select "Speedy Blupi" screen rendering correctly. (A
headless run using Chrome's `--virtual-time-budget` flag instead produced a black canvas — a
headless-testing-harness artifact from virtual time desyncing with the app's real async asset
loads, not a build regression; the DevTools-protocol approach with a real wall-clock wait, same as
the original `BUILD-003` verification, is the reliable way to test this.)

Note observed this session: a `sharp-runtime` (external sibling dependency, `../sharp-runtime`)
build once failed with a duplicate `Environment::SetEnvironmentVariable` declaration/definition
conflict, then succeeded on an immediate retry with no changes on this side — that repository
appears to be under active, independent development and the failure was transient. If it recurs,
it is not caused by anything in this repository; check `../sharp-runtime`'s own git log first.

### Test status
Last full run (2026-07-21, `build-cna` only, after `INFRA-004`):
- `build-cna`: **81 tests, 80 pass (99%)** — the only failure is `easy-gl-resource-smoke-tests`, a
  **pre-existing, unrelated** failure in the `easy-gl` dependency, root-caused entirely to that
  repository (see §5). It has been the same single failure across every verification pass this
  session.
- `GalaxyEggbertWorldsTests` (gtest, engine-agnostic `World`/`Chunk`/`MoveObjectRecord` model):
  **66/66 pass**, included in the total above.
- `VerifyGEWorldEditor`: **189 `PASS:` assertions**, all passing (165 through EDITOR-109, +24 for
  EDITOR-110's select/edit/remove tool).
- `build-cna-vulkan`: **not re-run since the editor work began** — last known result was 73/73, but
  that predates EDITOR-100..110. Re-running it is part of EDITOR-112 (§8).

### Tools/binaries available (see `CMakeLists.txt` for exact target names)
- `GalaxyEggbertCNA` — the main game executable.
- `GenerateSampleWorld3D` — writes all 79 `worlds3d/*.vwr` files: the real 78-world mobile-eggbert
  structure (global hub, 12 world hubs, 64 sublevels, world199) plus `world999.vwr`, this engine's
  own hand-authored mechanics-showcase/test world (found/split out 2026-07-17).
- Scripted verification tools (each an `add_test()`-registered ctest case): `VerifyBlupiMovement`,
  `VerifyInteractionSystem` (largest suite — pickups, hazards, cheats, secret powers, death/respawn
  timing), `VerifyGEInputPad`, `VerifyGESaveData`, `VerifyMoveObjectTypesCna`,
  `VerifyBigDecorParsingCna`, `VerifyTerrainAnimDivisor`, `VerifyTileUvBounds`, `VerifyCameraShake`,
  `VerifyGEWorldEditor` (the whole editor's pure logic: camera math, voxel raycast, box region math,
  undo/redo, palette data + click hit-testing, custom-world storage, browser screen, object
  placement), `VerifyGetObjIcon` (`INFRA-003` — `GetObjIcon()` data-integrity regression test), and
  `VerifyUnverifiedRenderMapping` (`INFRA-004` — locks the ~131-icon "unverified render-mode guess"
  table's shape).
### Recently implemented (2026-07-18/19: the in-game 3D world editor — see §3 for detail)

**EDITOR-100..110 are complete, verified and pushed.** What works today, end to end:
- Enter the editor from the real Init/main menu ("Editor" button), scoped to the selected gamer
  slot. A browser screen lists that slot's worlds under `customworlds/gamer<N>/`, with create /
  open / two-tap-delete.
- Free-fly camera: WASD + Space/Ctrl, hold RMB for mouse-look, scroll adjusts fly speed.
- Voxel raycast crosshair (Amanatides–Woo DDA) with a live highlight of the aimed-at cell.
- Left click places the selected block on the aimed-at face; middle click removes a block.
- `F` (or the palette's BoxFill button) starts a box-fill: first press/click marks corner A, the
  highlight tracks a live cuboid, a second press/click fills it as ONE undo command. `Escape`
  cancels.
- Undo/redo (`U`/`R` or toolbar), 200-command depth cap.
- Palette UI: a single vertical column of solid-green buttons at the left edge (2026-07-19 redesign
  to match free-eggbert's own editor palette, a user-supplied reference screenshot — supersedes the
  earlier left-toolbar + right-grid layout) — 8 fixed action buttons paired 2-per-row (Undo/Redo,
  Save/Back, Play-Test/mode-toggle, BoxFill/Confirmed-All-tab-toggle), a paging row, then the
  current page's block/object icons one per row. Both Blocks and Objects mode draw the real
  per-type sprite (object-m.png/element.png/explo.png/blupi.png/blupi1.png, whichever
  `ObjectIcons` says the type actually lives on) on a solid green backing — see §3.
- Objects mode places real `MoveObjectRecord`s (enemies, pickups, lifts) that appear immediately as
  billboards — no save/reload round-trip.
- `G` selects whichever already-placed `MoveObjectRecord`'s billboard projects nearest screen center
  (reuses `Hud::ProjectWorldToHudSpace()`, no new projection code), shown with a magenta highlight
  cube. `T` sets its `posEnd` to the current aim point, giving it a real patrol path; `Tab` cycles
  which of its 5 fields (speed + the 4 patrol-timing fields) `OemPlus`/`OemMinus` nudge; `Delete`
  removes it. All keyboard-only (no new toolbar button), matching the box-fill tool's own
  keyboard-only precedent; each edit is a real `MoveObjectEdit` undo command (`U`/`R` undo/redo them
  like any other edit).
- Save (`Enter` or toolbar) and a full Play-Test loop: saves, launches a real gameplay session
  against the custom world, and returns to the editor on win/loss/pause-back without touching real
  save progress.

**Two real bugs found and fixed via this work** (both pre-existing, neither editor-specific):
- `TerrainRenderer` built and drew an empty static mesh for an all-air world, throwing
  `ArgumentOutOfRangeException` (`primitiveCount` must be positive). Only reachable once "New World"
  could create a genuinely blank world. Fixed by guarding construction, matching sibling renderers.
- The palette fired on mouse **release** but world edits are edge-triggered on **press**, so every
  palette icon/toolbar click also placed a block at the crosshair behind the palette. Present since
  EDITOR-106; fixed by claiming the mouse per-press, with a regression guard confirmed to fail
  without the fix.

### Previously implemented (2026-07-16 — see §3 for detail)
**Two related bug families found and fixed 12 times total** that session, both from the same root
cause: real per-mechanic gate clauses that were correct when first ported but never retrofitted
once a LATER feature (vehicles, secret powers) shipped and should have applied to them too. Each
verified directly against `Decor.cpp`, not guessed.

**Family 1 — vehicle-mode gates (11 fixes)**: Helicopter/Overcraft/Jeep/Tank/Skateboard exclusion/
immunity clauses that predate Phase 17's vehicle implementation. The 4 most recent, on top of the 7
below:
- Springs now forcibly dismount any vehicle first (unless Shield/Hide active) before applying the
  bounce, matching real behavior — new shared `DismountAndDepositVehicle()` helper.
- **Most significant fix**: `TriggerDeathLock()` (every real death cause) now also unconditionally
  clears vehicle mount/Balloon/Ecrase/every secret power/Invert/Nage/Surf/Suspend/Ghost, matching
  real `BlupiDead()` exactly — previously ALL of this state silently survived death/respawn.
- Vehicles/Balloon/Ecrase now correctly skip water Surf/Nage detection entirely (not a "forced
  dismount" as a stale comment claimed — a vehicle just drives over/through water).
- Corrected 2 more stale "vehicles aren't modeled" comments found during this audit.

The original 7:
- Sucette(26)/Drink(30)/Charge(31) pickups now correctly exclude every vehicle mode + Balloon/
  Ecrase (Shield/Invert confirmed to have NO such clause, unlike what an earlier note claimed).
- `TriggerTeleport()` now also excludes vehicle mode (previously only checked Balloon/Ecrase).
- Ground jump now excludes Jeep/Tank/Helicopter/Overcraft/Balloon entirely; Skateboard gets its
  own real distinct jump velocity instead of the ordinary headroom-modulated one.
- Spike/Drip/Saw hazards now grant real immunity while riding Overcraft/Jeep/Tank specifically
  (not Helicopter/Skateboard; Lava/Blitz/Crusher deliberately have no such clause).
- Switch activation now excludes Overcraft/Jeep/Tank/Skateboard/Balloon (Helicopter IS exempt here
  — real source allows it, unlike every other gate above).
- Dynamite/Perso placement now excludes every vehicle mode + Balloon/Ecrase (Perso's separate
  pickup-an-already-placed-decoy path deliberately does NOT get this gate — real source has none
  there either).
- `TriggerMount()` (mounting a NEW vehicle) now also excludes Balloon/Ecrase (its own header
  comment wrongly claimed this was already handled elsewhere).
- Crate push now excludes every vehicle mode + Balloon/Ecrase.

**Audit scope note**: also checked platform-lift boarding/riding (`Decor.cpp:3013-3018`) and egg/
door-key pickups directly — confirmed these genuinely have NO vehicle-mode clause in real source,
so no fix was needed there. Not exhaustively re-verified against all ~56 vehicle-exclusion-pattern
occurrences in `Decor.cpp` (only the ones reachable from already-implemented mechanics), so a
residual few may still exist if new mechanics get implemented later — re-check any NEW
mechanic against this same pattern before assuming it's complete.

**Family 2 — Shield/Hide immunity (1 code fix + 3 doc corrections)**: `SecretPower::Shield`/`Hide`
grant real hazard immunity (`blupi_.IsInvincible()`) — already correctly wired at all 8 real hazard
contact sites (Lava/Spike/Drip/Blitz/Crusher/Saw/Fan/Drown) plus fired-projectiles/dynamite-blast/
wasp/large-creature (verified directly, all consistent). The one genuine gap: the water breath
gauge decremented unconditionally regardless of Shield/Hide, now fixed. 3 plan.md notes still
claimed this immunity "wasn't modeled" for fired-projectiles/Spikes/Fan — stale since `170` shipped
2026-07-12, corrected.

**Family 3 — 3 genuinely new gameplay features (not a gate retrofit)**: `ObjectType201-203` lethal
contact (`PICKUP-069`), the Perso-decoy/lethal-decor enemy trap mechanic, and secret-exit
(`ObjectType21`) contact (`PICKUP-009`/`083`) — real, previously-undiscovered/unimplemented
mechanics found via direct `Decor.cpp` reads, not vehicle/immunity retrofits like Families 1-2. The
trap mechanic resolves a "what does placing a decoy actually DO" mystery flagged back on
2026-07-13.

**Also this session**: 2 more hallucinated-feature cancellations (PICKUP-086/087, "shield trail
sound") and 2 clarified-not-fixable notes (PICKUP-027/035, both genuinely blocked on a bigger
prerequisite — `AscenseurVertigo`'s render decision and crate fall-physics respectively — not
independent gaps as previously stated).

(Prior session, 2026-07-13/14): real deferred "Voyage" pickup-reward timing, Clear2/3/4 death VFX,
the death-lock + life-loss-Voyage system, Sucette/Drink/Charge's 2-stage pickup delay.

### Known working demo
`worlds3d/world999.vwr`, this engine's own quarantined mechanics-showcase/test world (split out of
`world001.vwr` 2026-07-17 — see `plan.md` `SCORE-013`'s writeup — when `world001.vwr` became the
real, lean global hub). Reachable in-game from the global hub (mission 1) via its own `DemoPortal`
marker, or directly via `GE_DEBUG`-style tooling/tests. Playable with first-/third-person camera
toggle (`C` key), tank-control movement, and the full interactive-object system (pickups, hazards,
enemies, doors, lifts, crates). A second wasp (`ObjectType44`) sits 5 tiles east of spawn on the
flat corridor floor (added 2026-07-18) specifically so the Balloon status can be triggered and
observed within seconds of loading the world, with zero platforming — the original wasp is on a
north-hill plateau reachable only via a staircase + terraced ascent. Both wasps (and the
object-exhibition grid's bird/wasp specimens) now have a real patrol range and actually
move/animate (fixed 2026-07-20, see §3) — they were silently stationary before.

### What does not work yet
- **No visible 3D Blupi model** — invisible collision point in first-person; a placeholder model
  in third-person. Blocked on the user providing a real model/rig.
- ~~World editor: the hardening pass (EDITOR-112) is the one remaining milestone~~ — **done
  2026-07-23**. All 13 approved editor milestones (EDITOR-100..112) are now complete.
- **The editor has no text rendering** — toolbar buttons are distinguished by position and color
  only (object palette cells now draw real per-type sprites, not flat colors, since the 2026-07-19
  redesign — see §3). A documented, deliberate simplification, not an oversight (see §5).
- Several real HUD buttons render but are intentionally inert (no desktop equivalent exists yet):
  `SetupJump`/`SetupZoom`/`SetupAccel`, `PauseBack`, `InitRanking`/`InitBuy`.
- Idle "fidget" periodic sounds — blocked on `AnimState` values this engine doesn't have (same
  prerequisite as the 3D Blupi model).
## 3. Recent changes

Most recent first. Full history: `git log`.

### feat: BUILD-009 — GitHub Actions CI for GalaxyEggbertCNA, confirmed green on a real run (2026-07-23)

New `.github/workflows/cna-ci.yml`, closing the last open item in `plan.md`'s BUILD-00x list
besides Windows cross-compile/packaging. Checks out `galaxy-eggbert` plus 5 pinned sibling repos
(`cna`/`easy-3d`/`easy-gl`/`sharp-runtime`/`meta-gl`, same commits as this file's "Sibling-repo
commit pins" table above) as directory siblings, plus a sparse (`worlds/`-only) checkout of
`../mobile-eggbert` (test data, not a build dependency), reuses `../cna`'s own already-proven
`devices-tests.yml` apt package list + vendored-SDL cache pattern, adds a ccache cache on top,
configures `-DGALAXY_EGGBERT_BUILD_CNA=ON`, builds, then runs
`ctest --test-dir build-cna -E 'easy-gl-resource-smoke-tests'` (excluding the one known,
pre-existing, unrelated upstream failure documented above).

Scoped out by explicit user decision: no golden-frame/golden-trace steps in CI (no `xvfb-run`) —
those stay a manual local step, see the entry directly below and §7's commands.

**Actually triggered 4 live pushes to `develop` and let each real failure drive the next fix**,
rather than stopping at "looks right by inspection" — 3 real gaps found this way, invisible from
this environment where every sibling repo already happens to be checked out locally:
1. Abbreviated 8-char sibling SHAs (`ac3aaaeb` etc.) failed — `actions/checkout` needs the full
   40-char commit hash for an arbitrary-commit fetch. Expanded all 4 original pins.
2. Configure failed: `../easy-gl` unconditionally needs `../meta-gl` too (a real transitive build
   dependency, missing from this file's own pin table until now — added above, pinned `d51fcd7f`).
3. `ctest` failed 2/81 — `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` read real
   `../mobile-eggbert/worlds/*.txt` off disk, which nothing had checked out. Added a sparse
   `worlds/`-only checkout (a few MB, not the full ~2.3 GB repo), pinned `07e0a673`.

**Final result, run `29944268525`: fully green, `ctest` 79/81 (2 correctly excluded), ~6.5 minutes
cold-cache end-to-end.** The earlier local-only check (`DISPLAY` unset, no `xvfb-run`, 81/82 in
this container) correctly predicted the `easy-gl-resource-smoke-tests` exclusion but could not
have caught the meta-gl/mobile-eggbert gaps — both are workspace-layout issues only a genuine
clean-checkout run surfaces. Full `plan.md` `BUILD-009` entry has the complete writeup.

### docs: confirm golden-capture/trace scripts actually run in this container via `xvfb-run` (2026-07-23)

A prior conversational claim (not written into this file) said golden capture "couldn't be
completed here" due to a missing SDL/X video device. Re-checked directly rather than trusting that:
`xvfb-run`/`Xvfb` ARE installed in this container, and `xvfb-run -a ./GalaxyEggbertCNA
--golden-capture` (using the existing `build-cna` binary) succeeds cleanly — no display error at
all. Both wrapper scripts confirmed green the same way: `xvfb-run -a tools/verify_golden_frames.sh
build-cna` → `ALL CHECKS PASSED` (all 3 frames byte-match); `xvfb-run -a tools/verify_golden_trace.sh
build-cna` → `ALL CHECKS PASSED`. This file's own §7 commands were already correct (they specify
`xvfb-run`) — nothing here was actually wrong in the docs, the earlier claim was just an
unverified/incorrect statement made in conversation, not a doc bug. Noted here so it isn't
re-asserted as an open blocker again without re-checking.

### chore: editor undo/redo/command-stack invariant audit — clean, nothing found (2026-07-23)

One more fresh-angle audit round, this time targeted rather than a generic sweep: the world
editor's `EditCommandStack`/`WorldEditor` dirty-tracking/undo-redo state machine, which reached
feature completeness this session (EDITOR-100..112) but had never had this specific lens applied.
Checked, by direct code reading (not speculation): every `EditCommand::Kind` variant's `Undo()`/
`Redo()` correctly inverts what its matching `Push()` site populated (no stale/default field found
in any variant); `Push()` unconditionally clears the redo stack so a post-undo edit can never let
redo resurrect a stale command (already tested); undo/redo on an empty stack are no-op `false`
returns (already tested); `dirty_`'s sticky-until-Save behavior (even across Undo) is a deliberate,
documented design choice, not a bug, since an undo doesn't guarantee the on-disk file matches;
`backConfirmArmed_` is correctly reset by every path that should reset it, including both Undo and
Redo; every `EditCommand` field is a plain value type (`Worlds::Block`, `MoveObjectRecord`,
scalars) with no pointers/references, so no dangling-reference risk exists.
**Result: no genuine bug found — nothing changed, nothing committed.** One coverage-only note (not
a defect): `RefreshSelectedObjectAfterHistoryChange()` (WorldEditor.cpp:644-659), which re-syncs
`hasSelectedObject_`/`selectedObject_` after every undo/redo, is verifiably correct by inspection
but has no test at the `WorldEditor::Update()` level (no public accessor exists to inspect
selection state from a test) — left as a documented gap, not invented a test around a nonexistent
accessor just to have one. Together with the file-I/O trust-boundary audit above (which DID find a
real bug) and the immediately-prior public-API sweep, this confirms the code-quality/edge-case audit
category has now genuinely reached diminishing returns rather than just "narrowing" — see §10.

### fix: uncaught `std::stoi` crash in `LoadFromMobileEggbertFile()` (2026-07-23)

A follow-up file-I/O trust-boundary audit (a different angle from the public-API sweep below —
specifically looking for external/parsed data flowing unvalidated into array indices or numeric
conversions, the same shape as the `SaveData` bug) checked every disk-reading path in
`src/GalaxyEggbertCNA/` and `src/GalaxyEggbert/`. The core `.vwr`/`.vch` binary loaders
(`World::loadFromFile()`, `Chunk::read()`) were already exhaustively guarded at every parsed field.
One real gap found: `WorldRuntime::LoadFromMobileEggbertFile()`'s `Decor:`/`BigDecor:` cell
parser called `std::stoi()` directly with no try/catch anywhere in the function or its callers — a
non-numeric or over-long cell throws, uncaught, terminating the process. **Confirmed as an actual
crash** via deliberate bug injection (removing the fix's own try/catch): `terminate()` from an
uncaught `std::invalid_argument`.

Not reachable from any real gameplay path today (this function's only actual callers are 4 test
tools, all reading trusted `../mobile-eggbert` reference files — confirmed by grepping every call
site) — lower severity than the `SaveData` crash, which was reachable through live save-file
loading. Fixed anyway since it's real defect in a genuine public API this project deliberately keeps
callable (not dead code). Fix: a `SafeStoi()` wrapper returning 0 on parse failure, which the
existing `if (tileId > 0)` gate at both call sites already treats as "nothing here" — no separate
error path needed. New regression test (a synthetic malformed file with a non-numeric cell,
confirming the loader survives it) in `tools/VerifyBigDecorParsingCna.cpp`; verified real teeth via
bug injection (reproduced the actual crash), reverted. Full regression clean on all 3 native
backends. 2 other minor hardening opportunities were found and consciously left alone (an unbounded
`chunksPerAxis` resource-exhaustion shape in `World.cpp`, and a defense-in-depth-only gap in
`BlockTypes::fromMobileIconId()`) — see `plan.md`'s own note for the reasoning.

### test: close 3 more zero-coverage gaps found by a systematic public-API sweep (2026-07-23)

A follow-up audit systematically grepped every public method in `BlupiController.hpp`/
`ObjectIcons.hpp`/`InputPad.hpp`/`WorldRuntime.hpp` against its own test file, treating zero
hits (after ruling out coverage split across a sibling test file) as a real finding. `InputPad`
and `WorldRuntime` came back fully covered; 3 real gaps closed:

- **`ObjectIcons`'s 5 texture-atlas-selection predicates** (`IsUniformCubeObject`/
  `IsObjectMPngSourced`/`IsExploPngSourced`/`IsBlupiPngSourced`/`UsesBlupi1Texture`) had zero
  coverage anywhere — these gate which of 5 real sprite sheets an `ObjectType`'s billboard samples
  from; a type moved between lists would render with a completely wrong/garbage texture, silently,
  with no assertion catching it. Added a regression lock on today's exact membership plus 2
  exhaustive invariant checks across every real `ObjectType` (0-255): the 4 predicates are mutually
  exclusive, and `UsesBlupi1Texture` is always a subset of `IsBlupiPngSourced`.
- **`BlupiController::TriggerBye()`** (real "Bye" farewell freeze) had zero coverage — added the
  same "freeze, count down, auto-resume" test shape already used for `TriggerOneShotAnim()`/
  `TriggerTeleport()`, plus 2 of its 7 real exclusion-condition checks.
- **`BlupiController::TriggerMockery()`** (real taunt animations) had zero coverage — added
  success/already-mocking/cooldown/variant-duration checks. One test-design mistake caught and
  fixed while writing it: `TriggerMockery()` has no internal balloon/vehicle exclusion check by
  design (its own comment says the real gate is satisfied by the CALLER only invoking it while
  `GetAnimState()==Stop`) — a first draft wrongly expected an internal check; fixed to instead
  verify the actual relied-upon invariant (ballooned state never reads as `Stop`).

Verified real teeth via bug injection on all 3 (texture-predicate exclusivity/subset checks, and
`TriggerBye()`'s exclusion gate), all reverted. Full regression clean on all 3 native backends.

### chore: unify `kMaxEggCount`'s 2 duplicate definitions (2026-07-23)

Same code audit that found the `SaveData` bug also flagged `kMaxEggCount` (real `MAX_EGG_COUNT`,
Decor.cpp:96) defined twice in `InteractionSystem.cpp` — once as `float` at the egg-touch gate,
once as `int` at the voyage-completion gate. Harmless while both agree (both correctly 10), but a
future rebalance touching only one site would silently desync the two independently-enforced gates.
Hoisted to one shared `constexpr int` both sites now use. Behavior-preserving (both were already
10); full regression clean on all 3 native backends.

### fix: real out-of-bounds crash in `SaveData::Load()`'s selectedGamer parsing (2026-07-23)

A fresh code audit found `SaveData::Load()`'s `selectedGamer` parser had zero bounds validation,
unlike the sibling `gamer.N.field` parser right next to it. Every accessor indexing the selected
slot (`GetLives()`/`SetLives()`/etc.) reads `gamers_[selectedGamer_]` with no check of its own, and
`gamers_` is a fixed 3-element array — a hand-edited or corrupted save file with an out-of-range
`selectedGamer` is a real out-of-bounds access, not just a latent bug. **Confirmed as an actual
crash**: a deliberate bug-injection test (disabling the new guard) segfaulted `VerifyGESaveData`
outright.

Fixed both `Load()` (rejects an out-of-range value, same shape as the sibling parser) and the
public `SetSelectedGamer(int)` setter (now clamps instead of assigning unchecked). 5 new
`VerifyGESaveData` checks; verified real teeth via 2 separate bug injections (one reproducing the
actual segfault), both reverted. Full regression clean on all 3 native backends. See `plan.md`'s
`MENU-058..069` area for the full writeup.

### test: close the 3-pickup-wide "second instance at the cap" test-coverage gap (2026-07-23)

A gap noted but deliberately left open during INFRA-006's basic-pickup migration: Dynamite's "can't
carry a second stick" gate and Egg's `MAX_EGG_COUNT=10` cap both had zero test coverage for the
"touch a second one while already at the limit" case (only BulletPack's own cap already had this,
turns out — the old note may have been stale even when written). Added both, matching BulletPack's
own "push a fresh synthetic instance" pattern.

Found and fixed a real test-harness bug (not production) while writing the Egg test: a first draft
reactivated the SAME real egg object in place between touches; the sample world actually has 3 real
eggs, and once the reused object got pruned from the mobile-object vector after enough real
`world.Update()` ticks, the next "first ObjectType6" search silently matched a different egg
elsewhere, whose position no longer matched the touch position — later iterations silently touched
nothing, which read exactly like a stuck cap until traced with temporary debug prints. Fixed by
switching to fresh synthetic pushes throughout.

Verified real teeth via bug injection on both new tests (Dynamite: disabling its gate caught all 3
new checks; Egg: disabling its touch-time gate caught 1 of 3 — the other 2 stayed green because
`ApplyVoyageReward()`'s own separate completion-time cap check still blocked the reward, a genuine
defense-in-depth pair in the real code, not a test weakness). Both reverted. Full regression clean
on all 3 native backends. See `plan.md`'s `INFRA-006` entry for the full writeup.

### chore: fix a `-Wtype-limits` dead-code warning, sweep for more (2026-07-23)

With the editor plan complete, swept every `galaxy-eggbert`-only source file (engine-agnostic
`src/GalaxyEggbert/Worlds/`, `MoveObjectRecord.cpp`, plus the handful of CNA-side files with no
Easy3D/CNA/Microsoft::Xna header dependency — most CNA game code couldn't be checked this way
without pulling in sibling-repo headers, out of this session's repo-only scope) for compiler
warnings via `g++ -Wall -Wextra -fsyntax-only`. Found exactly one, in
`BlupiController::IsPointSolid()`: `blockType < 0` where `blockType` is `world.getBlock(...).type()`
(`std::uint16_t`, unsigned) — always false, dead code, functionally harmless (the upper-bound half
of the same check already covered every actually-reachable case). Removed the dead clause; full
regression clean on all 3 native backends (behavior-preserving by construction, confirmed unchanged
test results).

### test: re-investigated "grass-topped cubes walkable-through", does not reproduce (2026-07-23)

With the editor plan complete, swept `plan.md`'s "Known open bugs" for stale entries. Found one
(`plan.md`'s pre-`## 1` preamble list) referencing a `NEXT.md §5/§8 task 4` pointer that no longer
exists in this file, and a prior repro attempt in `tools/VerifyBlupiMovement.cpp` that only tested
VERTICAL collision (landing on top, already passing) — the literal "walkable-through" wording more
naturally means walking INTO the block horizontally, never tested. Added that missing case: a
single-height grass-topped block (icon 107) correctly climbs via the existing `kStepLimit` step-up
mechanic (same as any curb-height obstacle — likely what the original report actually saw), while a
3-tall wall of the same icon (too tall to step up) correctly blocks like any other wall. Verified
real teeth via bug injection (disabled the step-up branch, caught this new test plus 1 pre-existing
staircase test, reverted). Two new permanent regression checks in `VerifyBlupiMovement.cpp`, full
suite clean on all 3 native backends. See `plan.md`'s own updated entry for the full writeup — as
far as this investigation could determine, this was never a reproducible bug.

### feat: EDITOR-112 — hardening pass, editor plan COMPLETE (2026-07-23)

The last of the 13 approved editor milestones. Three parts:

1. **Unsaved-changes guard.** New `dirty_`/`backConfirmArmed_` state on `WorldEditor`, centralized
   through a new `MarkMutated()` helper (replaces all 10 `needsPresentationRebuild_ = true;` call
   sites) so `dirty_` can't drift out of sync. The Back toolbar button is now a real 2-tap when
   dirty: first press only arms (brightens, reusing the existing mode-toggle "on"-state color — no
   text labels anywhere in this class), second actually leaves, discarding unsaved changes; a
   single immediate tap when there's nothing unsaved. Save/PlayTest clear the flag on an actual
   write to disk; a further edit while armed disarms it (still working, not confirming). Scoped to
   Back only after confirming it's the ONE real path out of an active editing session in this
   codebase (the browser's Open/New are only reachable after Back already ran) — "Quit" (a real
   window-manager close) is a separately known, already-declined-to-fix limitation, unrelated here.
2. **A box-fill test straddling the world's own Z=0/Z=99 bounds** — a real `WorldEditor`-level
   integration case (distinct from `BoxRegion::NormalizeAndClamp`'s own EDITOR-105 unit test),
   confirming the fill loop covers every cell edge-to-edge with no off-by-one. Getting both corners
   to land on EXACT edge cells needed real care (hit.z is affine in camera height at fixed
   pitch/yaw; shifting between corners needed 2 `Update()` calls, not one, to cancel an unwanted
   height drift from the movement key tied to the pitched-down forward vector) — a first, simpler
   attempt failed for exactly this reason, root-caused via a standalone scratch diagnostic.
3. **Consolidated `VerifyGEWorldEditor` into clearly-named sections** — found the file already
   consistently used clear section markers throughout; completed the top-of-file index with
   accurate EDITOR-111/112 entries rather than a risky wholesale physical reorder of ~1500 lines of
   already-passing test code for purely cosmetic adjacency benefit.

Verified real teeth on both new test additions via deliberate bug injection (disabled the guard's
arm-on-first-press branch: caught 4 failures; introduced an off-by-one in the real fill loop: caught
3 failures including 2 pre-existing sibling tests), both reverted. Live verification of the Back
button's brightened-color rendering was attempted (same Xvfb+`xdotool` technique that worked for
EDITOR-111's own live check earlier this session) but blocked by the SAME known keyboard/window-focus
issue recurring — neither keyboard nor mouse input reached the game at all this time, despite
working fine earlier the same session with no code change in between. A further, independent data
point for that already-tracked, out-of-scope issue, not a new blocker — the underlying logic is
still thoroughly verified via the headless automated suite. Full regression clean on all 3 native
backends. See `plan.md`'s `EDITOR-112` entry (§6) for the full writeup.

**This closes the editor plan — all 13 approved milestones (EDITOR-100 through EDITOR-112) are now
done and pushed.**

### feat: EDITOR-111 — sky-region picker (2026-07-23)

World editor work resumed this session (was paused 2026-07-19) after re-confirming the "Known
problems" keyboard-input/window-focus concern below still holds (fresh re-read of the input
pipeline, same conclusion as before: not a galaxy-eggbert code bug). A stepper cycling
`world.skyRegion()` 0-31: Left/Right arrow keys (confirmed unused anywhere in `WorldEditor.cpp`
before this) plus a new palette toolbar-button pair (`SkyRegionPrev`/`SkyRegionNext`), wrapping
0<->31 rather than clamping. New `Kind::SkyRegionEdit` handling in `EditCommandStack` (declared
since EDITOR-105, unhandled until now) — 2 scalar before/after fields, Undo/Redo call
`world.setSkyRegion()`. No text readout needed: each step requests a presentation rebuild, so the
real background itself IS the feedback, reusing the exact mechanism a fresh world-load already has
(falls back to a flat clear color for the 4 ids with no real art, confirmed against the actual
copied `Content/backgrounds/` directory, not just the reference doc).

Adding the new toolbar-button row shifted every content-cell/paging hit-test coordinate down one
row; fixed 13 tests broken by the shift (updated hardcoded pixel constants, not reverted the new
row). Verified real teeth via bug injection (wraparound formula briefly replaced with unwrapped
subtraction — caught 2 precise failures, reverted). Live end-to-end verification: a temporary
env-var-gated debug scaffold + real `xdotool` key sends to the live window under Xvfb confirmed 3
real Right-arrow presses visibly swap the background from region 0's art to region 3's completely
different image; scaffold fully reverted (zero diff in `GalaxyEggbertGame.cpp` afterward). Full
regression clean on all 3 native backends. See `plan.md`'s `EDITOR-111` entry (§6) for the full
writeup.

### refactor: INFRA-006 7th family — table-driven terminal patrol arrival (2026-07-23)

Moved the three `ObjectType` exceptions at the shared `AdvancePatrolStep()` arrival junction into
`kPatrolArrivalHandlers[]`: projectile 23 and water bubble 15 deactivate at `posEnd`; goo 34
sticks there by collapsing its patrol endpoints. The default remains the normal transition into the
end-dwell phase. Existing direct coverage already checked bubble deletion and goo sticking; added a
synthetic projectile-arrival test. Removing its table row deliberately caused precisely the new
assertion to fail, then was reverted. Full regression passed on all three native backends (81/81,
79/79 Vulkan, 81/81 debug; the sole known unrelated easy-gl test is excluded where present). See
`plan.md`'s `INFRA-006` entry for the full detail.

### refactor: INFRA-006 — extract `IsDestructibleByDynamite()` (2026-07-23)

A fresh survey for a 6th family found only one clean candidate, and it's smaller than the 5 prior
handler-table families: the dynamite-blast victim-membership check (27 types) was an inline 27-way
`||` chain at its one call site — every member gets identical treatment (crates as a linked group,
everything else a plain deactivate), so it's a pure membership predicate, not per-type divergent
behavior. Extracted verbatim into `IsDestructibleByDynamite(ObjectType)`, matching the file's
existing `IsPlatformLift()`/`IsCrate()`/`IsGenericHazard()` pattern rather than a new table.

Verified real teeth: temporarily removed the crate (ObjectType12) from the predicate, confirmed the
existing dynamite-blast test's crate-destruction check failed with a precise message, reverted. The
other 26 members have no dedicated test either way — same coverage as before, not a regression.
Two other candidates surfaced by the survey were explicitly NOT taken: the pickup-touch-radius gate
(mostly already covered by the 2 migrated pickup tables, marginal DRY benefit on working code) and
the mockery-taunt qualifying list (real per-type divergence, same shape that ruled out the wider
enemy/hazard family). Full regression clean on all 3 native backends. See `plan.md`'s `INFRA-006`
entry for the full writeup.

### feat: INFRA-006 5th family — handler-table dispatch for the 7 patrol enemies' icon selection (2026-07-23)

The other candidate from the survey that found the vehicle mapping (below), taken after explicit
go-ahead given its enemy-adjacent-code caveat. Bulldozer4/fish17/bird20/blupih32/blupit33/wasp44/
creature54's per-direction/turn-transition icon dispatch in `GalaxyEggbertGame.cpp`'s billboard-
render loop — purely cosmetic icon selection, NOT the kill/damage/contact logic the wider enemy/
hazard family was ruled out for. New `kPatrolIconHandlers[]` (function-pointer table) +
`TryGetPatrolIcon()` replace the 7-case switch; `GetCreatureIcon()` alone drops the shared
signature's direction bool, so it gets a thin same-signature wrapper purely to fit the table.

No existing test covers this exact dispatch switch (`VerifyInteractionSystem`'s icon assertions
call each `GetXIcon()` directly) — verified via direct value-for-value comparison against the old
switch instead. Unlike the vehicle mapping, this code path IS exercised by the golden harnesses
(`world999.vwr`'s wasp/creature/blupih/blupit patrol objects run every tick during golden capture)
— `golden_trace.txt` stayed byte-identical (only logs Blupi's own state, so an exercise-only check
here); golden-frame screenshots failed all 3, reproduced identically with the change reverted (same
pre-existing flakiness as before, confirmed unrelated). Full regression clean on all 3 native
backends. See `plan.md`'s `INFRA-006` entry for the full writeup.

### feat: INFRA-006 4th family — handler-table dispatch for the vehicle ObjectType↔VehicleMode mapping (2026-07-23)

Continued the migration after a fresh survey of both god-methods (enemy/hazard family already ruled
out, see the entry below). Picked the cleanest of 2 new candidates: the vehicle mount/dismount
mapping in `GalaxyEggbertGame.cpp` (Helicopter13/Jeep19/Tank28/Skateboard24/Overcraft46) — 2
separate 5-case switches doing the SAME bijection in opposite directions (mode→type in
`DismountAndDepositVehicle()`, type→mode in the action-button mount scan), not a per-type behavior
dispatch like the prior 3 families. New `kVehicleModeTable[]` + `VehicleModeToObjectType()`/
`ObjectTypeToVehicleMode()` replace both switches with one shared lookup. Skateboard's real
TakeSkate/DeposeSkate anim (the only real per-vehicle divergence) stays its own explicit check at
each call site, outside the table — unchanged from before.

Stated plainly: this exact dispatch has no existing automated test (`VerifyBlupiMovement`/
`VerifyInteractionSystem` both exercise `TriggerMount()` directly, bypassing this file's mapping
entirely — confirmed by grep). Verified instead by direct value-for-value comparison against the
old switches (both directions, including the `default` fallback) — a provably pure bijective
refactor with no new combined behavior, a proportionate verification bar unlike `INFRA-005`'s merge.
Full regression clean on all 3 native backends; golden-trace byte-match unchanged (its scripted
input never presses the action button, so this path is provably unreached); golden-frame flakiness
reproduced identically with the change reverted (confirmed unrelated, same pre-existing issue).
See `plan.md`'s `INFRA-006` entry for the full writeup.

### feat: INFRA-005 follow-up — true merged X+Z+Y `ResolveMove()` (2026-07-23)

Closes the last of `INFRA-005`'s corrected deviations (see the 2026-07-22 audit entry below): the
user chose the most ambitious of 3 offered scopes — a real single merged 3D resolve, not just X+Z —
after 2 further conflicts were surfaced and approved individually first: (1) splitting `checkSubcell`
into separate horizontal/vertical flags (X/Z need fine sub-tile precision for WorldSelect/DemoPortal;
Y needs coarse whole-block precision so Lava/Crusher/Saw/Blitz/Drip still read as solid ground —
one shared flag can't do both in the same march), and (2) computing `dy` before the merged call
(verified safe except one accepted narrow edge case in `HasJumpHeadroom()`, step-up + jump same-tick).

Found and fixed a real bug during implementation, not by research: the first draft's march loop
returned the WHOLE result the instant ANY single axis blocked. Since a grounded landing zeroes
`m_velocityY` and gravity re-accumulates for exactly one frame before the next landing, the per-tick
`dy` is small but genuinely nonzero almost every tick while standing still — and the per-step Y check
has no ground-clearance epsilon (unlike the final `onGround` probe's own `y - 0.05f`), so Y read as
blocked on the FIRST micro-step nearly every tick. The early return threw away ~18/19 of that tick's
`dx`/`dz` too, even though X/Z were never blocked — an observed ~19x walking-speed regression, caught
by `VerifyBlupiMovement`'s existing WorldSelect-contact test (the walker never covered enough ground
to reach the marker). Root-caused via a standalone scratch reproduction tracing X per tick, fixed by
letting each axis march to its own full distance independently within the one shared loop (freezing
only itself on its own block, never stopping the other two).

Golden-trace reference (`tests/golden/golden_trace.txt`) regenerated: a real, intended behavior
change vs. the pre-merge trace — resting height on ground settles ~0.002 units lower, the direct
consequence of `checkSubcellVertical=false` (coarse) superseding the old fine per-icon-mask resting
height — plus ~1e-6 rounding noise during airborne arcs from the merged step count. Re-verified
byte-identical across all 3 native backends after regenerating. Also found `--golden-capture-trace`
itself is occasionally non-deterministic under system load (independent of this change — reproduced
identically with the fix fully reverted via `git stash`), extending the already-known
`--golden-capture` screenshot flakiness to the trace mode too; not chased further, out of scope here.

Full regression clean on all 3 native backends (`VerifyBlupiMovement` all-pass, `ctest` 81/82, only
the pre-existing unrelated `easy-gl-resource-smoke-tests` failure). See `plan.md`'s `INFRA-005`
entry for the full writeup.

### feat: INFRA-003's programmatic reference-doc cross-check (2026-07-22)

Second of the 3 gaps the audit surfaced, now closed. P0-2 asked for GetObjIcon() to be
cross-checked against `08-animations.md` *programmatically* — new `tools/
VerifyObjIconAgainstReferenceDoc.cpp` parses that doc's §3.1/§3.2 tables at runtime (regex, not
hand-copied numbers) and black-box-measures GetObjIcon()'s actual frame count against what the doc
claims, for all 24 rows.

Found and worked through a genuine information-theoretic limit along the way: a first draft using
pure black-box period/segment-counting (zero source knowledge) got 6 of 24 rows "wrong" — several
tables (`kBulldozer = {66,66,67,67,66,66,65,65}`, etc.) deliberately hold the same icon across
adjacent slots as an authoring choice, and a repeated-value array is output-indistinguishable from
a genuinely shorter one. Resolved by explicit user decision: read just the per-type hold divisor
from the source (a small structural fact, not the expected data) — with that, 23/24 rows match
exactly, and the 24th (ObjectType25/shield) is the same already-known, already-accepted doc
staleness `VerifyGetObjIcon.cpp` documents, asserted as a named exception rather than a fresh
failure. Verified real teeth via a deliberate array-size mutation (caught, precise message,
reverted). Full regression clean on all 3 native builds. See `plan.md`'s `INFRA-003` entry for the
full writeup.

### feat: INFRA-002's "behavioral trace" half — --golden-capture-trace + verify_golden_trace.sh (2026-07-22)

Closes the first of the 3 gaps the same-day audit (below) surfaced. `REMAKE-ANALYSIS.md`'s P0-1
asked for TWO harnesses — golden-image (done) and a deterministic per-tick behavioral trace
(never built). While designing this, found the passive `--golden-capture` mode never fed Blupi
any simulated input at all (he sits at spawn the whole 180-tick window) — so a meaningful
behavioral trace needed its own scripted input, kept in a genuinely separate mode
(`EnableGoldenTraceMode()`/`--golden-capture-trace`) so the already-approved `golden_frame_*.png`
references stay completely untouched.

New mode drives Blupi through a small fixed script (walk west down the same "tested corridor"
`VerifyBlupiMovement.cpp` depends on staying unchanged, jump once at tick 40-46, keep walking,
150 ticks), recording one line per tick (position/velocity-Y/grounded/anim-state/anim-icon) to
`golden_trace.txt`. New sibling script `tools/verify_golden_trace.sh`, same shape as
`verify_golden_frames.sh`. Determinism confirmed two ways: identical md5sum across 2 independent
runs (same bar `INFRA-001` set), and — stronger than the screenshot check got — byte-identical
across all 3 native backends (`build-cna` EasyGL, `cmake-build-debug` EasyGL, `build-cna-vulkan`
Vulkan), confirming the simulation itself is genuinely backend-agnostic. Full regression clean on
all 3 builds. See `plan.md`'s `INFRA-002` entry for the full writeup.

### docs+fix: external audit of INFRA-001..005 "done" claims — 4 confirmed overclaims corrected (2026-07-22)

An independent audit challenged several `plan.md` `[x] done` markers against `REMAKE-ANALYSIS.md`'s
actual requirement text. Rather than accept or dismiss it, independently re-verified every specific
claim by reading the cited file:line locations and the original P0-1/P0-2/P1-1 spec wording directly
— all held up. Confirmed and fixed:

- **`INFRA-002`**: `verify_golden_frames.sh` ran under `set -euo pipefail` with no `|| true` after
  the game-launch line — if the binary can't even start (no display/GPU), the script died there,
  before reaching its own per-frame `FAIL` reporting, producing zero output instead of the intended
  "FAIL: golden_frame_NNNN.png was not captured" per frame. Reproduced with a fake binary that exits
  1 (old script: silent; fixed script: correct 3 `FAIL` lines) — a real bug, now fixed. Separately,
  P0-1's "behavioral trace" (deterministic per-tick position/velocity/anim-state log, diffed) was
  never built — only the screenshot half of P0-1 exists. Left open, not silently folded in.
- **`INFRA-003`**: P0-2 asked for a *programmatic* cross-check of every `GetObjIcon` entry against
  `08-animations.md`. What exists is a regression lock (data copied from the source file itself)
  plus one manual spot-check — real value, but not what P0-2 describes. Left open.
- **`INFRA-004`**: confirmed via `grep` that `UnverifiedRenderMapping.hpp` is used only by its own
  test, nowhere in the actual renderer — a documented reference list a human can check against, not
  automatic enforcement. Already implicit in the task's own "bookkeeping only" line, now stated
  explicitly.
- **`INFRA-005`**: the biggest one. P1-1 asked for ONE merged-position resolve covering both axes;
  the actual code is 3 sequential per-axis `ResolveMove()` calls (X, then Z, then Y) — a real,
  reasoned design choice (preserves slide-along-wall behavior) that was never disclosed as a
  deviation from spec. The airborne-wall-clip bug this task actually targeted is still genuinely
  fixed either way — this correction is about algorithm shape, not that fix being wrong.

All 4 corrections written directly into `plan.md`'s own `INFRA-002`/`003`/`004`/`005` entries (dated
2026-07-22, not silently edited into the original "done" text) so the history stays honest. No
functional code changed except the one-line script fix.

### fix: third-person fox placeholder wall-clip (2026-07-21)

Live user bug report: "liska... muze vejit skoro cela do zdi" (the fox can walk almost entirely
into a wall) — turned out to be about Blupi's own third-person placeholder model, not an enemy.
Blupi's collision is a single point, stopping exactly flush with a wall's face; the fox mesh has
real nose-to-tail depth with no compensating render offset, so roughly half its body visually
projected past the collision point into any touched wall. Measured the model's real depth from
`fox1.verts.bin` (same method as the existing `kPlaceholderModelScale`): local Z span ~154.72
units, by far the largest axis, consistent with a quadruped body-length axis. Fixed with a new
3rd tuning constant, `kPlaceholderModelDepthOffset = 0.6f`, applied through the yaw rotation so it
always points opposite the model's current facing. Tuned by eye via live screenshot iteration
(same rigor as the existing `kPlaceholderModelScale`/`kPlaceholderModelYOffset`) — confirmed fixed
against a wall and unchanged/correct in open floor. Render-only, doesn't touch collision. Full
regression clean on all 3 native builds. See `plan.md`'s `069` entry for the full writeup.

### fix: WorldSelect/DemoPortal hub-navigation contact detection (2026-07-21)

Live user bug report ("teleporty nefunguji a blupi jimi prochazi skrz" — turned out to mean the
hub-navigation "booth" markers, not the actual Teleport1-4 pillars, which were separately verified
working). Root-caused as a real regression from `INFRA-005` (unrelated commit, same day): these
markers' real per-icon quarter-cell mask is genuinely thin (same category as Lava/Crusher/Saw), so
INFRA-005's sub-tile-precision collision correctly stopped treating them as solid — which broke an
*accidental* dependency, the old coarse collision's step-up mechanic used to elevate Blupi onto
them on contact, which is what let `GetGroundBlockType()` (gated on `IsOnGround()`) see them at
all. Confirmed via a direct git-worktree A/B test against the pre-INFRA-005 commit. Real
`Decor::IsWorld(m_blupiPos)` was never a grounded-check to begin with — fixed by swapping the
trigger site to the already-existing `GetBlockTypeAt()` (built for water detection, same
"not gated on IsOnGround()" shape) instead of `GetGroundBlockType()`.

Verified live end-to-end twice with temporary instrumentation (reverted before commit): standing
already-on-a-marker still works, and — the actual reported bug — walking into one from a few tiles
away now correctly triggers mission 1 → 10. New `VerifyBlupiMovement` test documents the exact
distinction (318/318, was 311/311). Full regression clean on all 3 native build dirs.

Also found, unrelated: golden-frame capture (`INFRA-001`/`002`) is genuinely non-deterministic in
this environment on repeat runs with zero code change — confirmed via `git stash` that it's
unrelated to this fix. Contradicts the original "confirmed deterministic" claim; flagged for a
fresh look next time golden-frame work is touched, not chased down here. See `plan.md`'s
`SCORE-013` entry for the full writeup.

### chore: INFRA-006 enemy/hazard/combat family investigated, confirmed not a fit (2026-07-21)

After 2 low-risk families migrated, the user asked to attempt the explicitly-flagged-risky
enemy/hazard/combat family (blupih/blupit/wasp/creature/follower/projectiles) anyway, carefully.
Read the full ~350-line block line by line before writing anything — confirmed concretely (not
just per the earlier survey's guess) that 5 of its 6 distinct behaviors are genuinely different in
KIND: fired-projectile is always-fatal-and-self-destroys, wasp is never-lethal-never-destroys,
creature is lethal only mid-patrol-turn, blupih/blupit aren't contact hazards at all (their firing
is woven into the shared patrol-turn/dwell-frame timing), follower is a wake-then-home state
machine with its own self-destruct. The one uniform sub-part (the 8-type generic-hazard list) is
already exactly as data-driven as it should be via existing `IsGenericHazard()`/
`IsBalloonPoppableHazard()` predicates. Forcing a single handler table over the rest would add
indirection without removing real complexity, risking exactly the kind of subtle bug this project
has worked to avoid. No code changed — this is a deliberate "don't migrate" verdict backed by
having actually read the code, not a shortcut. See `plan.md`'s `INFRA-006` entry for the full
writeup.

### feat: INFRA-006 3rd family — handler-table dispatch for the "basic" pickups (2026-07-21)

Migrated Treasure(5)/Key1(49)/Key2(50)/Key3(51)/Dynamite(55) — pickups that self-delete on contact
and defer their reward to voyage completion, no gate or one plain bool gate — to
`kBasicPickupHandlers[]` + `TryCollectBasicPickup()`, replacing 5 near-identical `case` bodies.
Egg(6) and BulletPack(29) deliberately stay open-coded: Egg's voyage endpoint depends on live
`lifeEggCount_` state at grant time, and BulletPack has an immediate side effect
(`bulletCount_ = kBulletCap`) before deactivating — neither fits fixed table data without adding
more complexity than migrating one type is worth.

**Verification found an honest, pre-existing gap, not introduced here**: bug-injection testing
caught a flipped sparkle-burst flag (1 precise failure), but disabling Dynamite's carry-cap gate
entirely was caught by zero tests — `VerifyInteractionSystem`'s dynamite coverage only exercises
the single-pickup-then-place path, never "touch a second stick while already carrying one." The
same gap likely applies to Egg's/BulletPack's own caps. Left undocumented-as-a-task deliberately
(a 3-pickup-wide test-coverage gap, not something to fold into this one scoped migration) —
verified the gate is correct by direct code inspection instead. `VerifyInteractionSystem` unchanged
at 547/547; full regression clean on `build-cna`/`cmake-build-debug`; golden-frame byte-match
unchanged. See `plan.md`'s `INFRA-006` entry for the full writeup.

### feat: INFRA-006 2nd family — handler-table dispatch for the 17 self-expiring particle types (2026-07-21)

Continued the migration started by the secret-power-pickup pilot. Surveyed both god-methods fresh
(a dedicated scoping pass, matching the project's own "needs a fresh go-ahead per family"
convention) — `GalaxyEggbertGame.cpp`'s own `ObjectType` handling turned out thin (37 refs,
mostly already-clean lookup tables); nearly all remaining work is in
`InteractionSystem.cpp::Update()`. Picked the 17 purely-cosmetic self-expiring particle/effect
types (explosion flashes, splash/sparkle/magic-trail/splat/teleporter-arc effects — full
`ObjectType` list in `plan.md`'s `INFRA-006` entry) as the lowest-risk next family: no Blupi
interaction, no kill/hazard logic, confined to one ~190-line region of the switch. New
`kExpiringParticleHandlers[]` table + `TryTickExpiringParticle()` replace the 17 near-identical
`if` blocks with one lookup + one dispatch.

**Real wrinkle preserved, not smoothed over**: the original code had two genuinely different
shapes — 7 types always `continue`d regardless of expiry state; the other 10 only `continue`d once
expired, otherwise falling through to the shared `AdvancePatrolStep()` call (load-bearing for 4 of
them with real posStart→posEnd slides). A per-entry `alwaysContinueEvenBeforeExpiry` bool keeps
both shapes intact — collapsing them into one would have silently broken the slide types.

**Verification**: `VerifyInteractionSystem` unchanged at 547/547 (nearly all 17 types already had
direct expiry-boundary test pairs from earlier work). Confirmed the table has teeth: temporarily
shifted one entry's expiry phase by 1, exactly 1 test failed with the precise message, reverted.
Full regression clean on `build-cna` (80/81, pre-existing unrelated failure only) and
`cmake-build-debug`; golden-frame byte-match unchanged (pure dispatch-logic reshaping, no rendering
touched). See `plan.md`'s `INFRA-006` entry for the full writeup, including which family (core
enemy/hazard/combat logic) was surveyed and explicitly ruled out as a poor near-term fit.

### fix: BUILD-011 — web ctest console tools actually run under Node (2026-07-21)

Found while re-verifying INFRA-005/006 on `build-web`: `ctest`'s own `add_test()` registrations for
`VerifyBlupiMovement`/`VerifyInteractionSystem`/etc. have always been unconditional (no
`NOT EMSCRIPTEN` guard), but nobody had ever actually run `ctest --test-dir build-web` before —
prior web verification (`BUILD-003`) only checked the live app via headless Chrome. Doing so
aborted immediately: only the main `GalaxyEggbertCNA` target gets `--preload-file`'d assets, so
these standalone console tools got an empty virtual filesystem.

Fixed with `-sNODERAWFS=1` (real host filesystem access, matching native exactly, zero source
changes) for the 4 tools with relative repo/sibling-dir path reads
(`VerifyBlupiMovement`/`VerifyInteractionSystem`/`VerifyMoveObjectTypesCna`/
`VerifyBigDecorParsingCna`); `VerifyGESaveData` needed a different fix instead (its `/save` path is
an absolute IDBFS mount point, not a relative repo path — NODERAWFS would wrongly map it onto the
real host's own `/save`) — a new minimal `cmake/web/pre-test-save-dir.js` just creates `/save` in
that tool's own in-memory MEMFS.

**Second, unrelated bug found along the way**: `VerifyGEWorldEditor`'s `CustomWorldsDir()` check
hardcoded the native-only expected path, missing `CustomWorldStorage.cpp`'s own
`#if defined(__EMSCRIPTEN__)` branch (`/save/customworlds`) — fixed by mirroring the same branch in
the test. Confirmed pre-existing (not something the NODERAWFS change touched) and re-verified clean
on `build-cna`/`build-cna-vulkan`/`cmake-build-debug` too after the fix (shared test file).

Full `ctest --test-dir build-web`: **15/15 (100%)**, up from an immediate abort. See `plan.md`'s
`BUILD-011` entry for the full writeup.

### chore: INFRA-005/INFRA-006 verified backend-agnostic on build-cna-vulkan and cmake-build-debug (2026-07-21)

Following the same precedent already used for the world editor, rebuilt (incremental, `-j2`,
already-configured targets per CLAUDE.md's "reuse before rebuilding") and re-tested the two other
native CNA build directories after the collision-resolver rewrite and pickup-handler-table pilot
landed, since both had so far only been built/tested against `build-cna` (EasyGL):

- `build-cna-vulkan` (Vulkan backend): clean incremental rebuild; `VerifyBlupiMovement` and
  `VerifyInteractionSystem` both fully pass standalone; full `ctest` **78/78 (100%)**.
- `cmake-build-debug` (EasyGL, the "default" build target per project docs): clean incremental
  rebuild; both Verify binaries pass standalone; full `ctest` **80/81**, the one failure being the
  same pre-existing, already-quarantined `easy-gl-resource-smoke-tests` upstream bug (see "Known
  upstream failures, quarantined" below) — not a regression.

Confirms the unified movement/collision resolver (`IsPointSolid`/`ResolveMove`/
`RecoverFromPenetration`) and the secret-power-pickup handler table are genuinely backend-agnostic,
with no graphics-backend-specific assumptions leaking into either.

### feat: INFRA-006 pilot — handler-table dispatch for the secret-power pickup family (2026-07-21)

First `ObjectType` family migrated off the open-coded `if (obj.type == ObjectTypeN)` god-methods
(`REMAKE-ANALYSIS.md` P1-2). Chose the 5 secret-power pickups (Shield/Power/Cloud/Hide/Invert,
ObjectType 25/26/30/31/40) as the pilot — lowest risk (not core kill/hazard logic), freshest
familiarity (just touched in `INFRA-007`). New `kSecretPowerPickupHandlers[]` table
(`InteractionSystem.cpp`'s own anonymous namespace) + one `TryGrantSecretPowerPickup()` member
function replace what used to be 5 near-identical `case` bodies in `Update()`'s big switch with a
single 5-label group calling one shared dispatch — "add/fix a 6th secret-power pickup" now touches
only the table, not the god-method's own case list.

**Real scope boundary found while investigating** (kept deliberately out of this pilot): 2 OTHER
dispatch sites in the same file reference these same 5 types — a dynamite-blast destructible-type
list (~27 types) and a pickup-touch-radius gate (~13 types) — but both are cross-cutting concepts
spanning far more types than just this family. Fully replacing either would need every other family
sharing them migrated too; left as open-coded lists for now. This is the honest shape of "migrate
one family at a time" — a first pilot can only absorb dispatch logic entirely confined to its own
family.

Behavior-neutral by design: `VerifyInteractionSystem` unchanged at 547/547. Verified the new
dispatch has real teeth — temporarily flipped one handler's `requiresActionButton` flag, confirmed 3
tests failed with precise messages, reverted. Full regression clean (80/81, pre-existing unrelated
failure only); golden-frame byte-match unchanged. See `plan.md`'s `INFRA-006` entry (marked `[~]`,
pilot done, full migration still open — ~65 more `ObjectType`s remain across both god-methods) for
the full writeup.

### feat: INFRA-005 — unified movement/collision resolver, closes the airborne-wall-clip gap (2026-07-21)

Scoped with the user first (full unified resolver + sub-tile fidelity + `BlupiAdjust`-style
penetration recovery, the more ambitious options each time), then researched real mobile-eggbert
(`Decor::TestPath`/`BlupiAdjust`/`DecorDetect`) before writing any code. Real `TestPath` is a
Bresenham pixel march (one rect, X+Y resolved together, rewinds to last clear position on the first
hit) with **no grounded/airborne branch anywhere** — called unconditionally for every mode including
every vehicle, confirming this engine's old `!m_onGround` bypass in `TryMoveAxis` was a real,
confirmed bug, not a simplification. Real solidity (`DecorDetect`) is a 4x4 (16px) per-icon
quarter-cell mask (`Tables::table_decor_quart`, 7056 entries), not whole-tile.

New `BlupiController` API: `IsPointSolid()` (sub-tile precision via the real quarter-cell mask,
extruded uniformly along this engine's own Z axis since the real mask is inherently 2D),
`ResolveMove()` (one march per call, mirrors real `TestPath`, reports which axis blocked +
`onGround`), `RecoverFromPenetration()` (real `BlupiAdjust` equivalent, called unconditionally at
the top of `Step()`, 3 real distinct attempts — down/right/left — since this engine's Blupi has
always been a point, not the real rect `BlupiAdjust` pushes). `GroundHeightAt`/`CeilingHeightAt`/
`TryMoveAxis` removed entirely. New `src/GalaxyEggbert/Game/DecorQuartTable.hpp` holds the
transcribed quarter-cell data (mechanically extracted, cross-checked via a second independent
extraction — byte-identical).

**Two real problems found and fixed during implementation** (not by the research):
1. **A coordinate-convention bug this refactor itself introduced, then found and fixed**: this
   engine's Y axis uses a bottom-anchored grid convention (`[g, g+1)`, confirmed against
   `GetGroundBlockType()`'s own `lround(m_y) - 1`) that's DIFFERENT from X/Z's center-anchored one
   (`[g-0.5, g+0.5)`, matching the terrain renderer's own `CubeMesh`) — a real, pre-existing
   asymmetry already known and worked around elsewhere in this codebase
   (`GalaxyEggbertGame.cpp`'s `kPlaceholderModelYOffset` comment literally documents this same
   0.5-unit collision-vs-render gap). First draft used the wrong (center-anchored) formula for Y,
   breaking 59 of 311 `VerifyBlupiMovement` checks; fixed by using `floor(y)` for Y specifically.
2. **A real design tension, resolved via a `checkSubcell` parameter**: several hazard tiles Blupi's
   own hazard-detection depends on physically resting on (Lava/Crusher/Saw/Blitz/Drip) have an
   all-zero real quarter-cell mask (genuinely thin, independently corroborating
   `15-3d-render-mapping-design.md`'s own "ThinMechanical" findings for the exact same icons) —
   applying sub-tile precision to ground detection would make Blupi fall through already-verified
   hazards. Ground/ceiling resolution now explicitly uses the coarse (non-sub-tile) check;
   horizontal wall collision keeps the fine one — a deliberate, documented scope boundary.

`kStepLimit` step-up (already a CNA-only invention, not a real `TestPath` concept) kept as its own
explicit mechanism (`FindColumnGroundY`), not folded into `ResolveMove` itself.

**Verification**: `VerifyBlupiMovement` 252→311/311 after both fixes above. One pre-existing test's
own expected value corrected (not a regression): the balloon-ceiling-stop boundary moved from ~2.5
to ~3.0 once ground/ceiling used one consistent convention instead of two subtly different ones (the
old `-0.5` was itself a minor, previously-unnoticed inconsistency). New dedicated test for the actual
airborne-wall-clip bug — verified it has real teeth (a first draft passed even with the old bug
deliberately reintroduced since the jump landed before reaching the wall; redesigned to contact the
wall within the airborne window, then reconfirmed FAIL-with-bug/PASS-with-fix before reverting the
temporary re-injection). Full regression clean (80/81, pre-existing unrelated failure only);
golden-frame byte-match unchanged; live smoke run clean. See `plan.md`'s `INFRA-005` entry for the
full writeup.

### feat: INFRA-007 step 3/3 — Voyage/DeathLock moved to the typed event queue, INFRA-007 fully complete (2026-07-21)

Completes the migration (steps 1/2 below): all 17 of `InteractionSystem`/`BlupiController`'s
original `*ThisFrame()` flags now go through a typed event queue. Voyage/DeathLock turned out to
carry a much bigger payload than earlier estimated — Voyage has **11** fields (kind, iconId,
isButtonChannel, worldX/Y/Z, fixedX/Y, worldIsStart, isAscend, ascendOffsetY), DeathLock has **2**
(kind, shouldRespawn) — the approved tagged-struct shape absorbed this fine, no design change
needed. New wrinkle this step had to solve: unlike every other Kind, at most ONE
`VoyageRequested`/`DeathLockRequested` event can exist per frame (the old flat members gave this
"last request wins" behavior for free via plain assignment) — solved with a new private
`InteractionSystem::ReplaceEvent(EventKind, Event)` that removes any existing entry of that Kind
before appending, used by `RequestVoyage()`/`RequestClear2Ascend()` and all 4 death-lock trigger
sites. Two `enum class ... : std::uint8_t;` forward declarations (`VoyageKind`, `PendingDeathKind`)
let `Event`'s new fields be typed by them ahead of their full definitions, which stay exactly where
they've always lived. Consumer side: `GalaxyEggbertGame::ResolvePendingVoyage()`/
`ResolveDeathLock()` rewritten against `FindInteractionEvent()`, reading payload off the returned
pointer instead of ~9 individual getters each. `VerifyInteractionSystem.cpp`'s 2 helper lambdas
(mirroring the real consumer functions) plus ~10 direct call sites rewritten the same way; all
stale doc comments updated; zero references to any old getter/member name remain anywhere in
`src`/`include`/`tools` (confirmed via `grep`). Full regression clean (80/81, only the pre-existing
unrelated `easy-gl-resource-smoke-tests` failure); `VerifyInteractionSystem`: still 547 checks, all
passing (same as step 2 — no coverage lost). Live smoke check (`--golden-capture` + golden-frame
byte-compare) clean. **Honest gap:** the real camera-projection consumer path
(`ResolvePendingVoyage()`'s `Hud::ProjectWorldToHudSpace()`/`BeginVoyage()` call) has no dedicated
test coverage (predates this refactor — needs a full `GalaxyEggbertGame` with a real graphics
device) and wasn't separately live-instrumented this step, since every line touched there is a
mechanical getter-to-pointer-member substitution with no logic change, on an unmodified projection
call chain. See `plan.md`'s `INFRA-007` entry for the full writeup.

### feat: INFRA-007 step 2/3 — InteractionSystem's 11 remaining flags moved to the typed event queue (2026-07-21)

Continues step 1 (below). Migrated `InteractionSystem`'s 11 flags (everything except
`CrateBeingPushedThisFrame` — stays a bool, see step 1 — and the Voyage/DeathLock pair, step 3)
to `EventKind`/`Event`/`EventsThisFrame()`. Re-verifying the exact payload shape while implementing
this caught a real correction to step 1's own design summary: 3 of the 11
(`PowerGranted`/`CloudGranted`/`HideGranted`) actually carry a payload — the pickup's world
position, previously 9 separate float members (`Power/Cloud/HidePickupX/Y/Z()`) — only 8 of the 11
are truly payload-free. The already-approved tagged-struct shape absorbed this without needing a
design change. `DiedThisFrame()`'s "tested, no live-game-loop consumer" status carried over
unchanged, as decided in step 1. New `GalaxyEggbertGame::FindInteractionEvent(EventKind)`
private helper (`const Event*`, `nullptr` if absent) lets payload-carrying kinds read their pickup
position at the same call site. All 8 scattered consumption sites in `GalaxyEggbertGame.cpp`
rewritten in place — same order/interleaving, only the storage mechanism changed.
`tools/VerifyInteractionSystem.cpp` (~64 references — this task's largest test-file impact)
rewritten via two small local helpers (`hasEvent`/`findEvent`). Confirmed via `grep`: zero
references to any of the 11 old getter/member names remain anywhere in `src/`/`include/`/`tools/`.
Full regression clean (80/81, only the pre-existing unrelated `easy-gl-resource-smoke-tests`
failure); `VerifyInteractionSystem` itself: 547 checks, all passing. Live smoke check:
`--golden-capture` against `worlds3d/world999.vwr` (exercises `InteractionSystem::Update()` every
frame) completes cleanly, and all 3 golden reference frames still byte-match exactly — the render
pipeline is untouched. See `plan.md`'s `INFRA-007` entry for the full writeup. Step 3
(Voyage/DeathLock, the largest payloads, explicitly saved for last) not started yet.

### feat: INFRA-007 step 1/3 — BlupiController's 3 sound-cue flags moved to a typed event queue (2026-07-21)

Design proposed to and approved by the user before any code changed (this task's own
precondition, since it touches already-working signal plumbing). Surveyed all 17 `*ThisFrame()`
flags first (14 in `InteractionSystem`, 3 in `BlupiController`) and found 3 different real
shapes hiding under one name: 14 simple no-payload one-shots, 2 that carry extra payload fields
and can co-occur in the same frame (Voyage/DeathLock), and 1 (`CrateBeingPushedThisFrame()`) that
isn't a one-shot event at all — it's documented as continuous per-frame state with the caller
doing its own edge detection, so it's explicitly excluded from the queue. Also found
`DiedThisFrame()` is tested but has no real consumer in the shipped game — decided to carry it
into the new queue unchanged rather than use this transport-only refactor as cover to also drop
it. Approved shape: two independent event queues (one per emitting class, preserving their
existing decoupling), each a tagged struct (not `std::variant` — no precedent for it in this
codebase). Migration ordered smallest-risk first. **Step 1 (this entry):**
`BlupiController::DownEntrySoundFiredThisFrame()`/`UpEntrySoundFiredThisFrame()`/
`DownReleaseSoundFiredThisFrame()` replaced by `EventKind`/`Event`/`EventsThisFrame()`
(`std::vector<Event>`, cleared/refilled every `Step()`); the 3 `if` checks in
`GalaxyEggbertGame.cpp` became one loop + `switch`; `VerifyBlupiMovement.cpp`'s 10 assertions
rewritten against a small `hasEvent()` helper, same coverage. Clean 4-file diff, no old
getters/members left behind. Full regression clean (80/81, only the pre-existing unrelated
`easy-gl-resource-smoke-tests` failure). See `plan.md` `INFRA-007` for the full design writeup.
Steps 2/3 (`InteractionSystem`'s 13 remaining flags, including the much larger
`VerifyInteractionSystem.cpp` test-file impact) not started yet.

### feat: INFRA-004 — mark ~131 unverified 2D→3D render-mapping icons as an explicit, queryable set (2026-07-21)

Fourth item from `plan.md` §7's task breakdown. Bookkeeping only, no rendering code touched. New
`include/GalaxyEggbert/UnverifiedRenderMapping.hpp` (header-only, engine-agnostic): a table of every
icon ID whose 3D render-mode recommendation (`mobile-eggbert-reference/
15-3d-render-mapping-design.md` §10.2-§10.5, as applied per-icon in `02-tiles.md`'s Category column)
is still only a first-pass agent visual guess, never confirmed by direct user identification the
way §11's other 34 icons were — **131 icons** (generated straight from `02-tiles.md`'s actual
`"(§10.N)"`-cited rows, the current applied ground truth, not the design doc's older ~137 prose
estimate; the gap is explained by icons already resolved by direct identification — 61/62/65/67 and
Saw/SawStopped 378/379 — correctly excluded). New `tools/VerifyUnverifiedRenderMapping.cpp`, wired
into `ctest` (pure data table, no display needed): asserts the exact count, no duplicate/
out-of-range IDs, and spot-checks `IsUnverified()` both ways. **Verified the test has teeth**:
deliberately duplicated an entry, confirmed both the count and duplicate checks failed with precise
messages, reverted, confirmed a clean re-run. Full regression clean (80/81, only the pre-existing
unrelated `easy-gl-resource-smoke-tests` failure).

### feat: INFRA-003 — GetObjIcon() data-integrity regression test, wired into ctest (2026-07-21)

Third item from `plan.md` §7's task breakdown. New `tools/VerifyGetObjIcon.cpp` (headless,
engine-agnostic, registered with `ctest` — unlike `INFRA-001`/`002` this one needs no display, so
it runs in the normal suite): 232 checks covering every real `ObjectType` case in
`ObjectIcons::GetObjIcon()`'s switch, not just the 34 with a historical `Fixed 202...` note —
exact per-tick sequence for table-driven types, exact formula for ascending/descending/
triangle-wave types, fixed icon for static types, transcribed directly from the production code's
own already-committed values. Cross-checked frame-array length against
`mobile-eggbert-reference/08-animations.md` §3.1/§3.2/§4 wherever documented, surfacing one real
discrepancy: the doc's own `ObjectType25` row still says "8 frames" but flags itself as stale in
its own text (real `table_shield` has 16 — matches the current, already-fixed code); the test
asserts 16. **Verified the test actually catches regressions**, not just trivially passing:
deliberately mutated a divisor in `ObjectIcons.cpp`, confirmed a precise `FAIL` message, reverted
(clean `git diff`). Full regression clean (79/80 counting the new test, only the pre-existing
unrelated `easy-gl-resource-smoke-tests` failure).

### feat: INFRA-002 — golden-image diffing on top of INFRA-001 (2026-07-21)

Second item from `plan.md` §7's task breakdown, straight after `INFRA-001`. `tests/golden/
golden_frame_0060/0120/0180.png` are the approved reference frames (captured the same
`--golden-capture` way `INFRA-001` proved reproducible). New `tools/verify_golden_frames.sh
<build-dir>`: runs the capture, then exact-byte-compares (`cmp`) each fresh frame against its
reference, printing `PASS`/`FAIL` per frame and an `ALL CHECKS PASSED`/`SOME CHECKS FAILED`
summary with a matching exit code — same convention as the `VerifyXxx` C++ tools, just a shell
script since this one launches a subprocess and diffs files rather than exercising game logic
directly. Verified both directions live: passes against the real references, and correctly reports
`FAIL` against a deliberately corrupted reference copy (restored afterward, confirmed via `git
status` showing no unintended changes). Not wired into the default `ctest` run, same reason as
`INFRA-001` (needs a real display/GL context) — see §7's own new command.

### feat: INFRA-001 — permanent deterministic golden-screenshot capture mode (2026-07-21)

First implemented item from `plan.md` §7's correctness-infrastructure task breakdown. A real,
committed `--golden-capture` CLI flag (`main.cpp`, parsed and passed to a new
`GalaxyEggbertGame::EnableGoldenCaptureMode()`) — not another throwaway env-var hack. Skips to
Play, loads the fixed `worlds3d/world999.vwr` demo world, lets the game's existing 60Hz fixed
timestep run, writes `golden_frame_0060/0120/0180.png` at 3 fixed tick indices, then self-terminates
via `Exit()` (confirmed: process exits 0 on its own, not killed by an external `timeout`).
Reproducibility verified empirically, not assumed: 2 independent runs under `xvfb-run` produced
byte-identical PNGs (`md5sum` match on all 3 files) — the actual bar `INFRA-001` set for itself
("proves the capture itself is stable/reproducible"). Deliberately not wired into the default
`ctest` (needs a real display/GL context, same reason the existing live-headless-check step has
always been separate from `ctest` in this project) — documented as its own command in §7 above.
`INFRA-002` (golden-image diffing on top of this) is the natural next step, not done yet. Full
regression clean (78/79, only the pre-existing unrelated `easy-gl-resource-smoke-tests` failure).

### docs: merge REMAKE-ANALYSIS.md/renderers.md from a separate branch; record vision in plan.md §7 (2026-07-20)

User asked to merge `origin/claude/galaxy-eggbert-3d-remake-n0ga3y` into `develop` (a separate
session's exploratory work) and review the result. Merged cleanly (purely additive — 3 new docs
plus one new, currently-unwired header, no conflicts with this session's own Saw/wasp work):
`REMAKE-ANALYSIS.md` (root-cause analysis of the recurring bug pattern — correctness is verified by
a human looking at a screenshot, not automated, plus 6 other root causes), `renderers.md` +
`renderers-next-steps.md` (a dual-renderer architecture proposal, path A = same CNA/Easy3D base),
and `src/GalaxyEggbert/Game/SceneFrame.hpp` (the first data-contract artifact of that proposal
— unwired, nothing includes it yet).

Independently spot-checked the analysis' headline numbers against this codebase before writing
anything up (34 `Fixed` notes in `ObjectIcons.cpp`, `Update()`/`Draw()` line counts) — all
confirmed accurate, not exaggerated. Recorded the assessment as `plan.md` §7 ("Correctness
Infrastructure & Dual-Renderer — Vision"): the diagnosis is accurate (the same-day Saw-blade saga
below is a live instance of it), a small P0 slice (permanent golden-screenshot harness +
`GetObjIcon()` data-integrity checks) is cheap and worth doing when there's appetite, but the larger
items (a shared collision resolver, an `ObjectType` handler-table refactor) and the separate
dual-renderer direction change each need their own scoped task and explicit user sign-off — not to
be bundled into one "modernization" effort. Written as direction, not a task queue. The foreign
branch was deleted from the remote after merging (explicit user request).

### fix: bird + wasp exhibition specimens patrol instead of standing frozen (2026-07-20)

Follow-up to the wasp-patrol fix below, same session, user-requested: the object-exhibition grid's
bird (`ObjectType20`) and wasp (`ObjectType44`) specimens — built via a loop that places every
confirmed `ObjectType` as a static catalog piece (`posEnd==posStart` by explicit design, "nothing
patrols") — now get a small real ±1-unit patrol range instead, so their real per-direction/
turn-transition icon animation (`GetBirdIcon()`/`GetWaspIcon()`, both `patrolStep`/`patrolTime`
driven) actually plays. Kept small enough to stay inside each specimen's own 3-unit-spaced grid
cell. Verified live (Xvfb): both now show `patrolStep`/`patrolTime` advancing exactly like the
already-fixed gameplay wasps. The rest of the ~40-type exhibition grid is unchanged (still static
by design).

### fix: both gameplay wasp placements were silently frozen — no real patrol range (2026-07-20)

User report: "wasp and bird are frozen." Root cause found: both `ObjectType44` (wasp) placements in
the demo world (`tools/GenerateSampleWorld3D.cpp`) were built via the `place()` helper, which always
sets `posStart==posEnd`. That's a real, faithful no-op for `InteractionSystem`'s shared
patrol-turn mechanic (`AdvancePatrolStep()`'s own "no-op if posStart==posEnd" guard, already
verified against the real source's own equivalent guard in an earlier session) — but the wasp is a
confirmed real "patrol walker enemy" (`mobile-eggbert-reference/03-objects.md`) that's never
stationary in mobile-eggbert, so the zero-range placement also froze its per-direction/
turn-transition icon animation (`GetWaspIcon()` reads `patrolStep`/`patrolTime`, both permanently
stuck at their initial values without a real patrol tick), not just its movement. Same bug already
found and fixed for the platform lift (2026-07-10) — both wasps now built directly via
`PlaceMoveObject` with a real `posStart != posEnd`, matching that precedent, instead of `place()`.
Verified live (Xvfb): both wasps now show `patrolStep` advancing 1→2→3→4 and position interpolating
smoothly over their real patrol range. (No bird is placed anywhere via a named `ObjectType20` token
in this generator — the "bird" half of the report turned out to be the object-exhibition grid's own
dynamically-typed specimen, see the follow-up entry above.)

### fix: Saw/SawStopped (icon 378/379) render orientation — 6-round live-feedback saga, resolved (2026-07-20)

The single most-argued-over item in the whole session. Starting state: 2 prior rounds (2026-07-11)
had only ever adjusted the VERTICAL position of a vertical `InnerFlatPlate` (bottom-anchored, then
top-anchored-hanging-down); this session picked it up again after a live screenshot showed it as a
thin diagonal wedge jammed into a corridor's side wall.

Round-by-round (each against a fresh live Xvfb screenshot, most reverted before the next):
1. Read at the time as "should lie FLAT on the floor" (`PlateAxis::Y`) — implemented, but as an
   EXCLUSIVE `InnerFlatPlate` (hollow block, no solid cube), which read as a hole/pit in the floor.
2. Fixed additively instead (same idiom as the existing grass-top overlay): a normal, fully opaque
   RockPile-textured floor cube (NOT the confusingly-named `Ground`/icon 10, actually a
   machine-piece graphic per `BlockTypes.hpp`'s own NOTE) PLUS the blade as a horizontal overlay
   plate. This is when the icon's real, previously-unnoticed transparency was found: the top half of
   every 378-383 animation frame is fully transparent (confirmed via direct alpha-channel
   inspection), blade only in the bottom half.
3. User's final word, against this round's own screenshot: the blade should be **upright after
   all** (round 1's diagonal-wedge problem was the vertical PLATE'S position/hole, not the vertical
   axis itself) — reverted back to vertical, rooted at the floor's walkable surface, extending
   upward, still additive on the round-2 solid floor cube. **This is the confirmed-stable
   orientation** committed as `d4cc7f5`.
4. A further live-only investigation chased an exact-180-degree in-plane texture rotation, a
   precise centering position, and a size increase, each shown live and each reverted at the user's
   explicit instruction — none of that survived to the commit. Two genuine, reusable findings from
   that investigation, kept only as code comments (not applied): (a) rotating a `PlateItem`'s UV by
   180° via `U0<->U1`/`V0<->V1` swap or negative Width/Height both silently vanish the whole plate
   with THIS icon specifically — isolated to atlas mip-level bleeding from its mostly-transparent
   tile (confirmed via a decisive test: the identical rotation renders fine on an opaque tile, and
   fine on this same tile cropped to just its own opaque bounding box); (b) the wasp/bird patrol fix
   above was actually found as a side effect of this same live-testing session.

Also moved `Saw`/`SawStopped` out of `InnerFlatPlateTiles`'s shared icon list into their own
`IsGroundAnchoredPlateIcon()` handling directly in `TerrainRenderer.cpp`'s `RebuildAnimatedRenderer()`
(not the constructor's main `Build()` loop, since both icons are always animated and routed there
first) — the only 2 icons needing the additive solid-cube-plus-overlay treatment. Committed as
`d4cc7f5`. Full regression clean throughout (78/79, only the pre-existing unrelated
`easy-gl-resource-smoke-tests` failure). See `TerrainRenderer.cpp`'s own
`IsGroundAnchoredPlateIcon` comment block for the complete citation trail if this needs revisiting.

### feat: wired JumpSkate/AirSkate, TakeSkate/DeposeSkate, FireTank anim icons; fixed a Down frame-count bug (2026-07-19)

Follow-up to the animation-icon-sheet fix below, same session, done autonomously overnight per the
user's explicit request to continue implementing everything possible from
`mobile-eggbert-reference/08-animations.md` §8's "not wired" list. Extracted and cross-validated
the real `table_blupi` frame arrays (a from-scratch parser, validated byte-for-byte against 37
already-approved arrays before trusting any new one) for the 5 items research forks confirmed were
genuinely wireable (see `plan.md` `BLUPI-053/054/055/058` and their `BLUPI-047` follow-up note):
- **JumpSkate/AirSkate** (`table_blupi` 40/41) — Skateboard is the only vehicle mode with its own
  real airborne icon pair; `vehicleAnimState()`'s `Skateboard` case now splits by `m_velocityY`
  sign, same as the base `Jump`/`Air` split.
- **TakeSkate/DeposeSkate** (42/43) — wired via `TriggerOneShotAnim()` at the real Skateboard
  mount/dismount hook points in `GalaxyEggbertGame.cpp` (confirmed the only vehicle with a
  dedicated mount/dismount pose).
- **FireTank** (53) — new `InteractionSystem::TankFiredThisFrame()` per-frame signal (mirrors
  `CrateBeingPushedThisFrame()`, since that class has no `BlupiController` access), true only the
  frame a bullet actually launches, not the empty-clip click.
- **Bonus fix**: `kDownFrames` was `{33}` (1 frame) but the real table (and the reference doc) both
  say Down(6) has 3 frames (`33,34,35`) — found while cross-validating the parser, fixed
  independently.

Also re-confirmed (via 2 research forks, not implemented — deliberately deferred/dead):
Turn variants' real trigger condition (`Decor::BlupiStep()`'s `m_blupiSpeedX`-sign-vs-`m_blupiDir`
mismatch, a genuine edge-detection problem) documented for a future pass; Clear5-Clear8 reconfirmed
as real dead code (`Decor::BlupiDead()` never assigns them). See
`mobile-eggbert-reference/08-animations.md` §8 for the updated full status (46/84 wired).

New test coverage: `VerifyBlupiMovement` (Down cycling, JumpSkate/AirSkate state+icon+apex/landing,
TakeSkate/DeposeSkate one-shot trigger+resume, FireTank one-shot trigger+resume) and
`VerifyInteractionSystem` (`TankFiredThisFrame()` true/false across the not-in-tank/cooldown/
empty-clip cases). Full regression clean (only the pre-existing unrelated
`easy-gl-resource-smoke-tests` failure), built/tested at `-j2` throughout per the user's standing
CPU-core-limit instruction.

### fix: HUD's bottom-right animation icon used the wrong sprite sheet for 4 death causes (2026-07-19)

User request: "make sure the bottom-right animation icon actually corresponds to what should be
animated" (the interim debug indicator, `Hud.hpp`'s own class comment, not part of the real
mobile-eggbert HUD). Cross-checked against `mobile-eggbert-reference/08-animations.md` §2's own
"Channel selection" note (verified there directly against `Decor.cpp`'s real `BlupiSearchIcon()`):
only `Clear1`/`Clear2`/`Clear3`/`Glu`/`Electro` use `element.png`, every other real `BlupiAction`
uses `blupi.png`. `Hud`'s indicator always sampled `blupi.png` regardless — since `Clear1`/
`Clear2`/`Clear3`/`Glu` are 4 of this engine's real, already-wired `DeathLocked` causes, all 4 were
showing whatever unrelated `blupi.png` pixels happened to sit at that numeric icon index (`Electro`
has no modeled mechanic here, so it never came up). Fixed with new
`BlupiController::AnimIconUsesElementSheet()` (true only for those 4 causes) threaded through a
new `Hud::Draw()` parameter, selecting the already-loaded `element.png` batch instead of
`blupi.png` for just this one HUD element when it applies — `Hud` already draws other element.png
icons elsewhere (keys/bullets/dynamite/Voyage), so no new asset load or render path was needed.
8 new `VerifyBlupiMovement` assertions (a Stop-state baseline plus the sheet flag for all 6
`DeathCause` values, including 2 that had no dedicated test before, `Clear3`/`Glu`). Live-verified
under `xvfb-run` with temporary instrumentation (forced a `Clear1` death-lock, screenshot, fully
reverted before commit): the icon now shows a coherent Blupi silhouette instead of a mismatched
crop. Full regression clean (only the pre-existing unrelated `easy-gl-resource-smoke-tests`
failure). See `plan.md` `064`'s own entry for the full writeup.

### World editor: single-column green palette layout + BoxFill button (2026-07-19)

Follow-up to the per-type-icons redesign below, same session: the user clarified (with the same
`freeeggbert_editor.jpg` reference) that the *layout* also needed to match free-eggbert, not just
the icon style — one vertical column of solid-green buttons at the left edge, no separate grid
elsewhere on screen, plus a real toolbar button for box-fill ("beyond free-eggbert scope").
`EditorPalette` was rewritten from left-toolbar-plus-right-8x4-grid to a single left column:
- 8 fixed action buttons, paired 2-per-row (`ToolbarButtonRect(index)`: `index/2` = row, `index%2`
  = left/right half) — Undo/Redo, Save/Back, Play-Test/mode-toggle, BoxFill/Confirmed-All-tab —
  then a paging row, then content cells one per row. Pairing was necessary, not cosmetic: 9
  full-width rows (the original one-row-per-action draft) need 514px of vertical space alone, more
  than this engine's actual default 800x480 window has room for at all; 5 paired rows leaves real
  space for content.
- `PageCount()`/a new `ItemsPerPage()` now take the real `viewportHeight` and compute how many
  content rows fit below the fixed header, replacing the old compile-time `kIconsPerPage=32`
  constant — a single column has much less room per page than an 8x4 grid did.
- New `ToolbarAction::BoxFill`, wired into `WorldEditor::Update()`'s existing box-fill state
  machine via a simple `||` alongside the F-key condition — clicking the button behaves exactly
  like pressing F (first click marks corner A, second click fills), no new state needed.
- Solid green backing (not translucent) for every button, matching the reference's opaque style;
  the mode-toggle and Confirmed/All-tab buttons brighten to a lighter green when "on" (position
  alone can't convey that state); the selected content icon gets a gold highlight instead of the
  old enlarged-white-square treatment, so it stands out clearly against the now-green background.
5 new `VerifyGEWorldEditor` assertions (the BoxFill button drives the identical fill/undo behavior
the F key already had, verified against the same expected-region math). All existing palette-click
tests' hardcoded pixel coordinates were recomputed for the new geometry (the old ones silently
clicked the wrong control after the layout changed, until a full rebuild+rerun caught it as a wall
of new failures — fixed by walking every hardcoded coordinate in the test file, not by guessing).
Live-verified under `xvfb-run` with temporary env-var-gated instrumentation (fully reverted before
commit): both Blocks and Objects mode screenshots confirmed the single green column, real icons,
and the mode-toggle's brighter-green "on" state.

### World editor: real per-type object icons, replacing flat category-colored cells (2026-07-19)

User-requested redesign (unprompted user feedback, screenshot-driven, referencing free-eggbert's
own editor palette as the target look): the Objects-mode palette previously drew a flat
category-colored square per cell (EDITOR-106/109's own documented simplification, judged
"disproportionate" to build the real multi-atlas icon plumbing at the time). Now every cell draws
the real per-type sprite via `ObjectIcons::GetObjIcon()` plus its atlas-selection predicates
(`IsUniformCubeObject`/`IsObjectMPngSourced`/`IsExploPngSourced`/`IsBlupiPngSourcedAtPhase`/
`UsesBlupi1Texture`) — the SAME lookup already used to render real `MoveObject` billboards during
gameplay, so no new icon data or mapping was invented. Icons are batched per atlas (object-m.png/
element.png/explo.png/blupi.png/blupi1.png) so a page still costs at most 5 draw calls, same shape
as the pre-existing single-atlas Blocks-mode batch. `EditorPalette::Draw()` now takes 4 more
`Texture2D&` parameters (the game's own already-loaded `objectTexture_`/`exploTexture_`/
`blupiObjectTexture_`/`blupi1ObjectTexture_`, no new asset loads) threaded through
`WorldEditor::Draw()` from `GalaxyEggbertGame`; `CMakeLists.txt`'s `VerifyGEWorldEditor` target
gained `ObjectIcons.cpp` (a link-time dependency, engine-agnostic, same precedent as
`GenerateSampleWorld3D` already linking it). `EditorPalette::CategoryColor()` and its
`objectCategoryIndex_` map were removed (no longer used, not referenced by any test).

Every Objects-mode cell also gained a translucent white backing square (same technique/color as the
toolbar buttons, which the user explicitly asked to keep) so a real sprite's transparent margins
still read clearly against the animated 3D scene behind the palette — this was the SAME flat-quad
mechanism already used for the toolbar/selection-highlight, just now applied to every cell in
Objects mode (Blocks mode is untouched, its icons are already opaque). **Superseded within the same
session** by the layout entry above: the backing is now solid green (matching the reference's
opaque button style, not translucent), applied uniformly to Blocks mode too, once the user
clarified the reference meant the whole layout, not just per-cell icons.

No `VerifyGEWorldEditor` test changes were needed (`Draw()` is not exercised by that suite — it's
the one method needing a real `GraphicsDevice`, per this class's own established convention).
Live-verified under `xvfb-run` with temporary env-var-gated (`GE_EDITOR_AUTOTEST_PALETTE`)
instrumentation (jump straight to a fresh Editor world, synthetic click on the mode-toggle button,
screenshot, fully reverted before commit): confirmed real, distinct per-type icons (platform lift,
several enemies, explosions, a bee, key, egg, lollipop, balloons, a bomb, etc.) each on a legible
translucent backing, replacing the flat-colored placeholder.

### World editor: MoveObject select/edit/remove tool, EDITOR-110 (2026-07-19)

Adds a select-and-edit tool for already-placed `MoveObjectRecord`s, keyboard-only (no new toolbar
button, matching the box-fill tool's own keyboard-only precedent):
- `G` selects whichever placed object's billboard projects nearest screen center — nearest-billboard
  picking reuses `Hud::ProjectWorldToHudSpace()` (no new projection code was written), so this
  section links `src/GalaxyEggbert/Game/Hud.cpp` into `VerifyGEWorldEditor` for the first time
  (`CMakeLists.txt`). Shown with a new magenta selection-highlight cube — `EditorHighlightRenderer`
  gained a second public method, `ShowSelectedObject()`, and `WorldEditor` owns a *second* instance
  of that class so the selection highlight can stay visible alongside the existing aim-crosshair/
  box-fill one (each instance only tracks one set of bounds).
- `T` sets the selected object's `posEnd` to the current raycast aim cell, giving it a real patrol
  path (`posEnd != posStart`).
- `Tab` cycles which of the object's 5 numeric fields (`speed`, then the 4 real patrol-timing fields)
  `OemPlus`/`OemMinus` nudge by a fixed editor-UX step (not a transcribed real constant).
- `Delete` removes the selected object entirely.

Each action is a real `EditCommand::Kind::MoveObjectEdit` pushed onto the existing undo stack
(`U`/`R` already undo/redo these correctly — no undo/redo code changes were needed, confirming the
EDITOR-109-era design note that anticipated this). New `WorldEditor::RefreshSelectedObjectAfterHistoryChange()`
re-syncs the cached `selectedObject_` from the world after every Undo/Redo, so a later edit's "before"
state can't go stale relative to what Undo/Redo just wrote (a real bug caught and fixed during this
session's own testing, before it ever reached the live build — see below).

**A real test bug found and fixed while writing `VerifyGEWorldEditor`'s new section**: the first
draft pressed the same edge-triggered key (`U`) four times in a row across separate `Update()` calls
expecting each to undo one more step — but `WorldEditor::Update()`'s edge-triggering (matching
every other tool key in this class) only fires on a **press following a release**, so only the FIRST
of the four consecutive presses actually did anything; the other three were silent no-ops. Fixed by
adding a `pressKey()` test helper that presses then releases within two `Update()` calls, same
requirement a real keyboard has. Not a bug in `WorldEditor` itself — an easy trap to fall into
when scripting multiple presses of the same key in a test, worth remembering for any future section
that does this.

24 new `VerifyGEWorldEditor` assertions (189 total): place → G-select → T (patrol path) → OemPlus/
OemMinus (speed, then Tab + stepAdvanceTicks) → 4×undo → 4×redo → Delete → undo-the-delete, all
checked against `CollectMoveObjects`, plus a no-op sanity check (G with no objects anywhere in the
world selects nothing, and a Delete right after is a safe no-op).

Live-verified end-to-end under `xvfb-run` with temporary env-var-gated (`GE_EDITOR_AUTOTEST_110`)
instrumentation (fully reverted before commit, confirmed via `git diff`): a scripted place → G-select
→ fly → T → Play-Test sequence, driven through synthetic `KeyboardState`/`MouseState` fed into the
SAME real `WorldEditor::Update()` call site real player input uses. Confirmed the play-tested
world's `MoveObjectRecord` carried the real edited patrol path (`posStart=(50,4,33)
posEnd=(50,3,17)`), and — since the two screenshots taken during play-test looked visually
near-identical at that camera angle/distance — added a temporary numeric trace of the live
`MobileObjSpec::current{X,Y,Z}` during `Play` phase to settle it conclusively: `currentZ` advanced
continuously frame-by-frame from render-space -17 toward -33 (raw grid 33→17), proving the lift
genuinely patrols at runtime; the screenshots just weren't a legible way to see a mostly-depthwise
motion at that angle. No code bugs found in the live pass — instrumentation fully reverted afterward.

### In-game 3D world editor, EDITOR-100..109 (2026-07-18, 10 commits `a9140c2`..`a6ba199`)

New tree `src/GalaxyEggbert/Editor/`:
- `WorldEditor.hpp/.cpp` — owns camera/tool state/undo stack, drives everything else.
- `VoxelRaycast.hpp/.cpp` — Amanatides–Woo DDA against `Worlds::World`.
- `BoxRegion.hpp/.cpp` — corner pair → normalized, world-clamped box.
- `EditCommandStack.hpp/.cpp` — tagged `EditCommand` undo/redo stack.
- `PaletteCategories.hpp/.cpp` — curated + exhaustive numeric palette data (blocks and objects).
- `EditorPalette.hpp/.cpp` — toolbar + paged icon grid, Blocks/Objects modes.
- `EditorHighlightRenderer.hpp/.cpp` — cell/box selection overlay.
- `EditorBrowserScreen.hpp/.cpp` — per-gamer world list/create/open/delete UI.
- `CustomWorldStorage.hpp/.cpp` — `customworlds/gamer<N>/custom_NNN.vwr` path helpers.

Also added: `src/GalaxyEggbert/Game/QuadBatch.hpp/.cpp`, extracted from `InputPad` so the
editor UI reuses the exact same 2D quad primitives (`InputPad`'s public API unchanged;
`VerifyGEInputPad` passes unmodified as the regression guard), and `tools/VerifyGEWorldEditor.cpp`
(165 assertions).

Modified: `GamePhase.hpp` (new `Editor` value); `Worlds/World.hpp/.cpp`
(`removeBlockExtraMetadata`); `MoveObjectRecord.hpp/.cpp` (`RemoveMoveObject`);
`WorldRuntime.hpp/.cpp` (new public `ResyncFromWorld()`, refactored out of `LoadFromVwrFile()`'s
tail); `InputPad.hpp/.cpp` (Init-screen Editor button + `QuadBatch` extraction);
`GalaxyEggbertGame.hpp/.cpp` (phase dispatch, `worldEditor_`, `LoadCustomWorldForEditing`/
`ForPlayTest`, play-test routing, `ResyncFromWorld()` call in `RebuildWorldPresentation()`);
`CMakeLists.txt`; `tests/GalaxyEggbert/MoveObjectRecordTests.cpp` (+2 gtest cases, 66 total).

Bugs fixed: the empty-world `TerrainRenderer` crash and the palette press/release click bug —
both described in §2's "Recently implemented".

Verification method used throughout, worth reusing: pure logic (raycast math, box math, undo/redo,
palette hit-testing, storage, placement) goes into `VerifyGEWorldEditor` with synthetic
`KeyboardState`/`MouseState` values and no GraphicsDevice; anything visual is checked live with
temporary env-var-gated (`GE_EDITOR_AUTOTEST_*`) instrumentation under Xvfb with screenshot capture,
**always fully reverted before commit** (confirm with `git diff` — a whole-file `git checkout` is
risky here, it can also revert real uncommitted work; prefer targeted edits).

- **fix: Balloon's last 2 documented gaps closed — horizontal drift + ceiling stop (plan.md
  `E3D-MIG-135`).** User asked to close both limitations left after the rise fix below. (1)
  Horizontal drift: the SAME real `Decor.cpp:4059-4106` block gives Blupi momentum-based
  left/right drift while ballooned (ramps toward a real terminal speed, decelerates to exactly 0
  on release) instead of ordinary walking speed — ported as `m_balloonHorizontalSpeed`
  (3.125 units/s terminal). `TriggerBalloon()` now also force-exits any mounted vehicle (real
  `ByeByeHelico()`), required so this takes priority immediately. Real wall-stop while drifting
  NOT ported — this engine has no horizontal-wall-collision for ANY airborne movement today, a
  shared limitation beyond scope here. (2) Ceiling stop: corrected an earlier WRONG claim that the
  rise clipped through ceilings like Helicopter/Overcraft — real source's general swept collision
  (`Decor::TestPath()`) stops it same as walking into a wall. New `CeilingHeightAt()` implements
  this. **Found and fixed a real integration bug while verifying**: the pre-existing
  `GroundHeightAt()` misidentified the same ceiling block as ground to land ON TOP of once a rising
  Blupi got close enough — confirmed live via a standalone debug harness (Blupi teleported from
  y≈2.0 straight to y≈4.0, well before the real 2.5 contact height). Fixed by suppressing the
  ground-check for every frame a ceiling is within reach, not just the frame the clamp fires.
  Re-verified: Blupi now rises smoothly and holds exactly at the real ceiling height indefinitely.
  4 new test assertions, full regression clean.
- **fix: ballooned Blupi now actually RISES — the real behaviour, found at last (plan.md
  `E3D-MIG-135`).** Reported ~15 times. Every previous attempt asked only "does gravity still pull
  him down?" and stopped there — first modelling a 20%-gravity slow fall, then a fixed-height
  freeze, then papering over the invisibility with an invented cosmetic bob. All wrong. Gravity is
  not merely switched off: Blupi **actively drifts upward** for the full 10s. The real code is a
  dedicated block at `Decor.cpp:4039-4058` that no earlier pass ever opened — they reasoned only
  from the `!m_blupiBalloon`-gated fall trigger at `Decor.cpp:2823`. It accelerates him upward
  every `ScaleTime(6)` ticks to a `-3.0` px/tick terminal rise, `-5.0` with Jump/Up held, or
  decelerating to a hover with Down held (never a descent). Ported with real unit conversion
  (px/tick @20fps, 64px/block → 0.9375 / 1.5625 units/s, 1.0417 units/s²) and the invented bob
  deleted. Verified three ways: 6 new assertions pinning each real terminal speed to within
  0.02 units/s, a live realistic-input run measuring exactly 0.9375 units/s of climb, and
  third-person screenshots showing the level drop away beneath him. Documented limitations: the
  real floaty horizontal drift isn't modelled, and the rise isn't ceiling-clamped (matching the
  existing Helicopter/Overcraft modes).
- **fix: Balloon float effect was too subtle to notice during real gameplay (plan.md
  `E3D-MIG-135`).** User re-reported "still no change" after directly confirming (via targeted
  follow-up questions) they'd done a genuinely fresh CLion rebuild and watched the wasp sting in
  third-person view. Time-spaced screenshots (correcting two methodology gaps from the first
  verification pass: consecutive-frame shots can't show sine motion, and chase-camera damping was
  confounding an early retry) proved the original +/-0.08-unit bob WAS rendering exactly per the
  math — just too subtle at normal camera distance to read as "floating" rather than noise, since
  half the cycle dipped below normal standing height. Redesigned as a sustained upward hover
  (~0.12-0.32 units above normal stance the entire time, never dropping back down) with a gentle
  bob on top — confirmed via fresh screenshots to be an unmistakable, continuous gap between
  Blupi's feet and the ground. Physics untouched (still a real, verified zero-gravity freeze).
- **feat: trivially-reachable wasp for manual Balloon testing + conclusive realistic-input
  verification (plan.md `E3D-MIG-135`).** User reported "wasp sting still doesn't make Blupi float,
  no change at all" for the ~7th time despite two independent prior fixes (the physics freeze and
  the visual bob) both already re-verified live and correct. Root suspicion: the ONLY existing wasp
  sits on a north-hill plateau requiring a staircase + terraced-ascent platforming section to
  reach — real friction for manual testing that may explain why the fix was never actually being
  exercised. Added a second wasp directly on the flat spawn corridor of `worlds3d/world999.vwr`
  (5 tiles from spawn, zero platforming), and — critically — re-verified the ENTIRE chain using
  fully realistic simulated player input (held-forward `moveInput`, driven through the real
  `Update()` pipeline) instead of a position teleport like every earlier check this session:
  contact triggers `IsBallooned()` naturally at frame 46 of ordinary walking, with height
  correctly frozen while horizontal movement continues. This is the most rigorous verification of
  this mechanic yet — reachable via `worlds3d/world001.vwr`'s existing `DemoPortal` a few tiles
  from the global hub's own spawn point.
- **fix: main-menu pillarbox color + panel transparency (plan.md `MENU-006`/`HUD-004`).** Two
  user-reported main-menu visual bugs, both real. (1) The pillarbox margins on the sides of the
  Init/Wait screens were the generic default XNA/MonoGame "CornflowerBlue" template clear color,
  never customized to match the actual dark-navy menu background art — fixed by sampling
  `Content/backgrounds/init.png`'s own corner pixel (RGB(0,35,98)) and using that exact color for
  `device.Clear()`, eliminating the visible seam. (2) Every button/panel on the main menu (gamer
  slots, Play, Settings gear) rendered as fully opaque white instead of translucent — root cause:
  both `Hud`'s `kPanelOpacity` and `InputPad::DrawInit()`'s equivalent had been forced to 1.0
  since 2026-07-10 specifically to work around a genuine CNA/Vulkan bug (`BasicEffect` with
  Alpha<1 doesn't render at all under Vulkan) -- since Vulkan was the default backend at the time.
  Restored both to the real 0.6 value now that EasyGL (which renders this correctly) is the
  default. Building with `-DCNA_GRAPHICS_BACKEND=VULKAN` will still lose these panels -- that CNA
  bug itself remains unfixed, out of scope for today. Also investigated a third reported issue (a
  "different shade" outline around the SPEEDY BLUPI logo/Blupi image, suspected `cna` bug) and
  confirmed via direct pixel math it's the correct, expected alpha-blend result of the real asset's
  own soft edge over the (now-corrected) dark background -- not a bug. Full regression clean.
- **fix: Balloon visual legibility (no visible change on flat ground) + default graphics backend
  switched to EasyGL (plan.md `E3D-MIG-069`/`135`).** User re-reported "Blupi still doesn't float
  when the wasp stings him" after the physics fix below, ~5th time reporting it — frustrated,
  rightly so. Rigorous live re-investigation (temporary debug instrumentation: teleport Blupi onto
  the real wasp in `worlds3d/world999.vwr`, log every frame, then force him over a genuinely
  floorless column while already ballooned) proved the physics are 100% correct — `GetY()` holds
  EXACTLY constant for 500+ frames over open air. The real gap: the placeholder model's Balloon
  animation state maps to the SAME idle pose as normal standing, so on flat ground (where the wasp
  is placed) "frozen height" looks identical to just standing there — no visible cue at all. Fixed
  with a small cosmetic vertical sine bob applied only to the 3D model's render position while
  ballooned (does not touch physics). Also switched `CMakeLists.txt`'s default `CNA_GRAPHICS_BACKEND`
  from `VULKAN` back to `EASYGL` (user request, given the Vulkan shader bug found the same day) —
  still fully overridable, other backends remain buildable. `cmake-build-debug` explicitly
  reconfigured to pick up the new default (an existing cache doesn't change on its own). Full
  regression clean.
- **fix: Vulkan-only SkinnedEffect Y-flip bug in `../cna` (sibling repo) — the placeholder Fox was
  STILL floating on the user's real desktop builds (plan.md `E3D-MIG-069`).** User retested the
  fix below with a live screenshot showing the Fox still floating, well beyond the earlier 0.5-unit
  fix's scope. Found the user's actual desktop builds (`cmake-build-debug`, `build-cna-vulkan`) use
  the **Vulkan** backend, never tested in the earlier fix (only EasyGL). Side-by-side screenshots
  of the identical committed code, differing only in backend, proved a second, separate, Vulkan-
  only bug: EasyGL renders the Fox correctly grounded, Vulkan renders it hovering high in the sky.
  Root cause is in `../cna` (not galaxy-eggbert): every OTHER Vulkan 3D vertex shader applies a
  manual `pos.y = -pos.y` to compensate for Vulkan's inverted NDC Y axis vs. OpenGL, but all 4
  `SkinnedEffect` Vulkan shaders (used by `AvatarRenderer`, i.e. the third-person model) were
  missing this exact line — no Vulkan golden-image test exists for `SkinnedEffect` in `cna`'s own
  suite (only EasyGL has golden PNGs), so this was a real, previously-undetected upstream gap, not
  something wrong in galaxy-eggbert's own code. Fixed by adding the missing line to all 4 shaders
  and recompiling via `cna`'s own `compile_shaders.py`; re-verified via screenshot that Vulkan now
  matches EasyGL. **This fix currently sits uncommitted in `../cna`** — a genuine upstream engine
  fix with no possible galaxy-eggbert-side workaround, needs the user's own call on committing it
  there. All 3 native CNA build dirs (`build-cna`/`cmake-build-debug`/`build-cna-vulkan`) rebuilt
  and reconfirmed working. Full galaxy-eggbert regression clean (only the 1 known pre-existing
  failure). **Update (2026-07-21, `INFRA-009`): confirmed committed** in `../cna` — see §2's new
  "Sibling-repo commit pins" subsection.
- **fix: placeholder Fox model floating + wrong scale, real zero-gravity Balloon freeze (plan.md
  `E3D-MIG-069`/`135`).** Three user-reported bugs. (1) The third-person placeholder Fox model
  visibly hovered above the terrain. Root cause independently confirmed via 3 cross-checked
  sources (`TerrainRenderer.cpp`'s block-center formula, `easy-3d/CubeMesh.cpp`'s half-extent
  formula, and the existing first-person `kEyeHeight` usage): `BlupiController::GetY()` sits a
  constant 0.5 world units above the terrain's own rendered surface (an existing, previously-
  invisible convention — nothing rendered a body against the terrain to reveal it before now).
  Fixed with a render-only `kPlaceholderModelYOffset = -0.5f` local to this specific placeholder
  mesh's translation, deliberately NOT by changing `GetY()`/`GroundHeightAt()` itself (would ripple
  into jump physics, `kFallDeathY`, teleporter/water thresholds, and every already-tuned camera
  constant assuming the existing convention). (2) The model was ~160% of block height instead of
  the user-reported real 71.875%. The Fox mesh's local Y-span was measured directly from
  `fox1.verts.bin` (~79.03 units tall) and rescaled against that 71.875% figure (bracketed by real
  `Decor::BlupiRect()`'s default collision box, `Decor.cpp:2519-2520`), giving
  `kPlaceholderModelScale ≈ 0.009095` (was an unvalidated `0.02`). (3) Wasp-sting "Balloon" status
  used a 20%-reduced-gravity slow fall, an approximation its own comment admitted was unverified.
  Direct re-check of `Decor.cpp:2823`/`5834-5849` confirms real Blupi genuinely freezes at a fixed
  height (the real fall-trigger check is itself gated on `!m_blupiBalloon`) — fixed by skipping
  gravity integration entirely while ballooned; `kBalloonGravityMultiplier` removed as dead code.
  All three fixes live-verified via temporary env-var-gated debug instrumentation (forced
  third-person camera, menu-skip, and a delayed screenshot capture showing the model correctly
  grounded/scaled before-vs-after) and a tightened `VerifyBlupiMovement.cpp` assertion (ballooned
  height provably unchanged over time, not just "slower"), all reverted/kept minimal before commit.
  Full regression clean (78/78 minus the 1 known pre-existing unrelated failure).
- **feat: real Mockery (Blupi taunts nearby enemies) + real Stop idle-fidget cycle (plan.md
  `BLUPI-067`/`BLUPI-023`).** User asked two pointed questions: does Blupi stick his tongue out at
  a nearby enemy, and does he get bored and tap his foot after standing still a while? Both
  verified directly against `Decor.cpp` and both were real, unmodeled gaps — plus one
  documentation error found and fixed (an earlier note this session had Mockery's direction
  backwards: it's Blupi taunting a nearby enemy, not the reverse). Ported `Decor::MockeryDetect()`
  (`Decor.cpp:9518-9601`) as a new proximity scan in `GalaxyEggbertGame.cpp` (a bounding-box
  check, NOT contact/collision, against 11 real enemy ObjectTypes already placed in this engine),
  gated on Blupi being idle and a new `BlupiController::TriggerMockery()`/15s-cooldown mechanism
  — deliberately NOT a freeze (real source never drops focus for this, so movement cancels it
  immediately, no explicit cancel needed). `ObjectType54` always gets the distinct Mockeryp variant;
  every other qualifying type picks Mockery/Mockeryi by facing direction, with `ObjectType2`'s own
  real asymmetry (never the "ahead" variant) ported as-is. Also replaced `kStopFrames[]={0}` (a
  Simple3D-era single-frame placeholder) with the REAL 330-frame Stop table — confirmed this isn't
  a separate "boredom timer", just a long, naturally-cycling idle animation with fidget frames
  baked directly into it. Live-verified via a temporary debug harness (reverted before commit):
  standing near world999.vwr's own wasp correctly triggers `Mockery` with the exact real icon
  cycle, and standing still long enough genuinely reaches the real "twitch" frame. Full regression
  clean.
- **feat: extract mobile-eggbert's full real Blupi animation table, wire ~24 new states into the
  HUD icon (plan.md `BLUPI-036/037/038/040/041/043/044/047/051/053/058/069/073/076`).** User-
  reported: the HUD animation icon only shows a limited set of poses, unlike real mobile-eggbert.
  Wrote a small parser mirroring `Decor.cpp:2393-2400`'s own `{actionId, frameCount, holdFrame,
  icon0..iconN}` record-scan exactly, ran it against the REAL `Tables::table_blupi[2911]` literal
  (not hand-transcribed), cross-validated by reproducing the already-approved `kChargeFrames`/
  `kTeleportingFrames` byte-for-byte before trusting it for anything new — recovered all 84 named
  `BlupiAction` records that actually exist (3 of the real 87 — Set/Recedeq/Advanceq — have no
  table_blupi record at all, genuinely dead in the original game too). Wired real Stop/March icon
  pairs for all 5 vehicle modes (Helicopter/Jeep/Tank/Skateboard/Overcraft), Swim/Surf, Hide, Push
  (crate-pushing), and 2 one-shot actions (Switch, PutDynamite via a new
  `TriggerOneShotAnim()`/`IsOneShotAnimPlaying()` mechanism, same freeze shape as the existing
  Bye/pickup-freeze triggers) — selected by the SAME flags that already drive these already-
  functional mechanics (`m_vehicleMode`/`m_nage`/`m_surf`/`SecretPower::Hide`/a new `pushingCrate`
  `Step()` parameter). Real Turn variants (for every vehicle + base humanoid) and Skateboard's own
  airborne icons are NOT modeled — precise turn-trigger detection needs its own research pass, same
  gap as the never-implemented base `Turn` action. ~30 more real states have their data extracted
  and ready but no live trigger yet (no matching mechanic/edge-event exists for them currently) --
  see `plan.md`'s `BLUPI-047`/`062-067`/`070`/`074`/`075` entries for exactly why each one is
  deferred. Live-verified via a temporary debug harness (reverted before commit): mounting a Jeep
  correctly selects `StopJeep` and cycles the real `111,110,111,112` icon sequence. Full regression
  clean (only the pre-existing unrelated `easy-gl-resource-smoke-tests` failure).
- **feat: real "Bye" farewell freeze for hub world-select portals (plan.md `BLUPI-049`/`SOUND-042`).**
  User-reported: stepping onto a hub portal did nothing visually, unlike real mobile-eggbert's
  ~1.5s "turn and wave". Verified directly against `Decor.cpp:6436` (`Config::ScaleTime(30)` = 1.5s
  exactly) and `Decor.cpp:5476-5488` (real trigger is narrow — world-select contact only, never
  exit-reached/PauseBack/PauseRestart, which stay instant in real source too). New
  `BlupiController::TriggerBye()`/`IsBye()` freezes Blupi the same way `TriggerTeleport()` does; a
  new `AnimState::Bye` falls through to the existing Stop pose (no dedicated wave sprite exists in
  real source). The world-select touch handler now calls `SetYaw()` once to face the camera and
  plays the real entry sound (ch32); `LoadMission()` itself moved to a `wasBye`/`IsBye()`
  before/after-`Step()` completion check, same shape as teleport-transit completion. Live-verified
  via a temporary debug harness (reverted before commit): freeze holds ~1.5s, yaw updates correctly,
  `LoadMission()` fires exactly once with the right target. Full regression clean.
- **fix: north-hill platform lift visibly "clipped through" the crow's-nest floor near the wasp
  (plan.md `PICKUP-020`).** User-reported with a screenshot. The lift's patrol math was actually
  correct (verified twice against the real cube-rendering convention — flush at both ends, zero
  overlap) — the real problem was the crow's-nest floor's single 1-cell carved shaft opening being
  surrounded on all sides by solid rock, reading as "vanishes into stone" from nearly any angle.
  Widening the hole into a full trench still looked wrong once live-tested (a 1-cell-deep gap gets
  visually filled back in by neighbouring rows off-axis). Real fix, matching the exact conclusion
  `liftA`/`liftB` already reached for this same problem: removed the enclosing floor slab entirely —
  the shaft is now open air all the way up, with the chest/exit-goal perched on two small, separate
  stepping-stones instead of one enclosing floor. Live-verified: the platform now floats clearly in
  open space at every patrol height. Full regression clean.
- **fix: web build showed only a black screen on the real, unpatched shell (plan.md `BUILD-003`).**
  The prior entry's headless verification had patched `Module.noInitialRun` directly in a throwaway
  test copy, bypassing `shell-cna.html`'s own "Click or tap to begin." gate — masking a real bug the
  user then hit immediately. Root cause (found by reading the compiled `.js` glue): modern
  Emscripten's `run()` snapshots `Module["noInitialRun"]` into a local variable once, synchronously,
  before any click can occur, and never re-reads the `Module` property again — so the shell's old
  (U3D-era) "flip the flag from the click handler" recovery path was dead code; clicking never
  actually called `callMain()`. The canvas still went visible regardless (a separate, unconditional
  `setStatus("Running...")` call fires either way), which is exactly why it looked like a working
  page that simply never draws anything, not an obvious error. Fix: removed the click-to-begin gate
  entirely and set `noInitialRun: false` so the game starts immediately — re-verified with a fresh,
  completely unpatched Chrome run (no test patches, no simulated click) showing the real Init/
  gamer-select screen rendering right away. Also added `LINK_DEPENDS` on `shell-cna.html`/`pre.js` to
  the `GalaxyEggbertCNA` target — CMake doesn't treat `--shell-file`/`--pre-js` linker-flag strings
  as tracked dependencies, so editing either file alone previously left `cmake --build` silently
  serving the stale old `.html`/`.js` (bit us mid-fix here: the first "fix" build didn't actually
  relink until a source file was touched to force it).
- **feat: Emscripten/WebAssembly web build for `GalaxyEggbertCNA` (plan.md `BUILD-003`).**
  User-requested prototype build to publish on their own website. Turned out to be almost entirely
  CMake wiring — the CNA/easy-gl/meta-gl/SDL3 stack was already Emscripten-ready (vendored SDL3 has
  a `.sdl-prebuilt-emscripten` path, `EasyGLGraphicsBackend` already requests a GLES context
  unconditionally which maps straight to WebGL2, `Game.cpp` already had an
  `emscripten_set_main_loop` path). Made `CNA_GRAPHICS_BACKEND` default to `EASYGL` under
  `EMSCRIPTEN` (Vulkan has no web bridge), added the `.html`/`--preload-file`/WebGL2 target
  properties, a new `cmake/web/shell-cna.html`, and IDBFS-backed save persistence
  (`SaveData::kSavePath` now `#if defined(__EMSCRIPTEN__)`, paired with a `--pre-js`). Only
  preloads `Content/icons`/`backgrounds`/`sounds` (~24MB total), not mobile-eggbert's full
  `Content/` (~460MB) — confirmed via grep that `icons4x`/`backgrounds4x` (~437MB) are never read
  by this engine. Verified via a real headless-Chrome/WebGL2 run (DevTools-protocol-driven so the
  async asset load could be awaited): confirmed a real `WebGL 2.0 (OpenGL ES 3.0 Chromium)` context,
  terrain/sound/background loading, and a screenshot of the real Init/gamer-select screen rendering
  correctly. Native `build-cna` rebuilt + full regression re-run after these changes, clean (only
  the pre-existing unrelated `easy-gl-resource-smoke-tests` failure).
- **feat: split `world001.vwr`'s demo content into a genuine 79th world, `world999.vwr`; enlarge
  world-hub plazas to 18x18 (plan.md SCORE-013).** Explicit user follow-up request: galaxy-eggbert's
  main global hub should contain ONLY the real teleports (matching real mobile-eggbert's own global
  hub scope exactly), each sub-hub world should have proper teleports to its own sublevel worlds
  (already true, see the SCORE-013 full-scope expansion above), and hub-world floors should be
  enlarged to at least 10x10 so the teleport layout fits comfortably. `world001.vwr`'s previous rich
  demo/mechanics-showcase content (dynamite/crates/vehicles/water/doors/every tile-icon exhibition/
  etc.) is unaffected in substance — it moved wholesale to a new, engine-specific 79th world,
  `world999.vwr` (beyond mobile-eggbert's real 78, purely for this engine's own dev/test use).
  `world001.vwr` itself is now the lean real global hub: an 18x18 plaza holding exactly the 12
  `WorldSelect` portals + the real `ObjectType7` exit (→ mission 199) and nothing else. A new
  `BlockTypes::DemoPortal` (icon 177, engine-specific) sits in the global-hub plaza and leads
  directly to mission 999 — the only in-game way to reach the demo/test world, reusing the exact
  same contact-trigger portal-touch handling as `WorldSelect`. `GenerateWorldHub()`'s own spawn
  plaza grew from 11x11 to 18x18 (matching the global hub's size) for the same "fits comfortably"
  reason; the existing tested wall+`ProgressDoorN`-gate corridor mechanism for markers 2+ is
  unchanged, just now extends from a bigger plaza. `VerifyBlupiMovement`/`VerifyBigDecorParsingCna`/
  `VerifyInteractionSystem` default world paths updated `world001.vwr`→`world999.vwr` accordingly.
  Live headless verification (temporary debug instrumentation, reverted before commit): confirmed
  `world001.vwr` has exactly 12 `WorldSelect` + 1 `DemoPortal`, `LoadMission(999)` loads the demo
  world correctly, the enlarged `world010.vwr` shows its correct 4 markers, and reloading mission 1
  afterward round-trips cleanly. Full regression clean (only the pre-existing unrelated
  `easy-gl-resource-smoke-tests` failure).
- **feat: implement third-person camera wall collision (plan.md CAM-005).** User-prompted after
  discussing chunk streaming for future denser worlds — the real risk identified wasn't chunking,
  it was the chase camera having no wall check at all, letting it clip through geometry to end up
  outside a tunnel looking back in. New `RaymarchWallDistance()` (a small fixed-step raymarch from
  Blupi's look-at point toward the desired chase position, stopping at the first solid block —
  simplified vs. the task's own "DDA" wording, since the real max distance is tiny and a small
  step is plenty precise there) pulls the eye in along the same direction whenever a wall sits
  between Blupi and the full chase distance. First-person is unaffected. Live headless
  verification: Blupi placed mid-tunnel facing a nearby wall, confirmed the naive 4-unit chase
  distance gets clamped to the real available ~1.5 units. Full regression clean.
- **feat: expand the hub/mission-progression system to full real scope + real progress-gated
  doors (plan.md SCORE-013/014, SAVE-006).** Explicit user follow-up request: galaxy-eggbert
  should contain the SAME hub/world scope as real mobile-eggbert (all 78 world files), not just
  the earlier 3-world proof, plus the real "doors that only open under certain conditions"
  mechanic gating sublevel access. Researched directly against `Decor.cpp`: the real win-exit
  formula (`Decor.cpp:6411-6434`) is distinct from the `PauseBack` formula — mission 1's own exit
  goes to a real final bonus world (199), not `mission/10*10` — new `WorldRuntime::
  ComputeWinExitTarget()` fixes a latent bug where `world001.vwr`'s own pre-existing exit would
  have computed an invalid mission 0. Confirmed (by tracing `Decor::IsWorld()` directly) that real
  global-hub world-select markers are NEVER access-gated — the locked/unlocked icon swap is purely
  cosmetic — so no gating was needed there; but confirmed real per-world sublevel access IS
  gated by real physical door tiles (`Decor::AdaptDoors()`'s `m_mission%10==0` branch,
  `SearchDoor()`/icon 182, cross-checked against `worlds/world010.txt`'s own real sign+door
  layout), opened once the preceding sublevel is won (`Decor::OpenDoorsWin()`). Ported as new
  `BlockTypes::ProgressDoor2..8` + `SaveData::IsMissionDoorUnlocked()`/`UnlockMissionDoor()`
  (a per-gamer persisted flag array keyed directly by mission number), applied at
  `LoadMission()`-time (matching real `AdaptDoors()` running before the level is ever shown).
  Extended `BlockTypes::WorldSelect1-8` to `1-12` (all 12 real world hubs). Generated all 78 real
  mobile-eggbert world files with byte-for-byte identical filenames/mission numbers via a
  data-driven table in `GenerateSampleWorld3D.cpp` (1 global hub unchanged in content, 12 world
  hubs with real per-world sublevel counts 4/5/4/6/8/6/5/4/5/7/5/5, 64 minimal placeholder
  sublevels, 1 final bonus world) — every world beyond `world001` stays a near-empty placeholder
  per explicit user instruction, until a real 3D world editor exists. `world001.vwr`'s own rich
  demo content is completely unaffected (confirmed via the full existing test suite still passing
  unmodified). New tests (`VerifyInteractionSystem`/`VerifyGESaveData`) + 2 separate live headless
  verifications (the full navigation chain, and the door-gate mechanism itself: a fresh world's
  door starts closed, unlocking + reloading opens exactly that one door and no others) + full
  regression clean.
- **feat: implement the hub/mission-progression system (plan.md SCORE-013..019, MENU-035/036,
  TILE-006).** The single biggest new subsystem this session — real mobile-eggbert's actual
  structure (global hub, mission 1 → world hubs, mission X0 → sublevels, mission X1-X5 → back via
  exit/PauseBack/PauseRestart) ported faithfully via new `WorldRuntime::
  ComputeWorldSelectTarget()`/`ComputeMissionBack()` (pure, verified directly against
  `Decor.cpp`'s real mission-handler formulas and `Game1::MissionBack()`) and
  `GalaxyEggbertGame::LoadMission(int)` (loads `worlds3d/world{N:03d}.vwr`, rebuilds terrain/
  background, resets Blupi/interaction state to fresh per-level defaults preserving only lives —
  real `PlayPrepare()`'s exact scope). Renamed 8 long-unused `BlockTypes` constants (`Sp0`-`Sp7` →
  `WorldSelect1`-`8`) — these were ALREADY the correctly-identified real hub-screen world-select
  markers (icons 158-165, from `170`'s earlier research), just never wired to anything.
  **User-authorized exception to this session's normal "no 3D world editor work" rule**: 3 new
  minimal placeholder worlds (`worlds3d/world010/011/012.vwr` — a world-1 hub + 2 nearly-empty
  sublevels, just a small stone-cube floor each) plus one new portal marker added to the existing
  `world001.vwr`'s icon-exhibition floor (which now doubles as the real global hub) — its own rich
  demo content (dynamite/crates/vehicles/water/doors/etc.) is completely unchanged, confirmed via
  the full existing test suite still passing unmodified. Wired every real mission-transition
  trigger: world-select portal contact, exit-reached (`WinLostReturn`), `PauseBack` (previously
  fully inert — its own button-press tracking already existed, just was never surfaced), and
  `PauseRestart` (upgraded from a position-only reset to a genuine level reload, matching real
  `Game1.cpp:357-358`). Resume `CONTINUE` now actually reads back the mission number `SaveData`
  was already (write-only) persisting at every Win/Lost checkpoint. **Known, deliberate
  divergence**: every level completion still shows this engine's own Win screen before advancing
  (real source only does that for the true final mission 199, silently reloading otherwise) — kept
  as an intentional, already-built UX choice, not a new gap. 14 new `VerifyInteractionSystem`
  assertions + a live headless run proving the full chain end-to-end (mission 1 → portal → mission
  10 → marker → mission 11, temporary debug instrumentation reverted before commit) + full
  regression clean (only the pre-existing unrelated `easy-gl-resource-smoke-tests` failure).
- **docs only**: closed/corrected 15 more `BLUPI-1xx` sound-channel duplicates (`BLUPI-132`-`153`
  range) — most were exact duplicates of already-fixed `SOUND-0xx`/`PICKUP-0xx` entries just never
  checked off; several additionally had wrong channel claims (ch41 isn't "glide", ch50 isn't
  shield-on, ch44 isn't shield-off, ch53 isn't Tank-fire, ch58 isn't Drink's, ch62 isn't Sucette's,
  ch51 isn't glue, ch21 isn't secret-exit, ch7 isn't door-open), each corrected to point at the
  real already-wired channel. `BLUPI-149`'s "Electro sounds ch38/ch90" had both claims
  independently disproven (ch38 is crate-push, ch90 is a footstep-remap channel) and is left open
  rather than guessed. Also clarified `TILE-024` (water drip animation) is deliberately blocked —
  the real 48-frame table is a known transcription target, but the real visual is Billboard-
  rendered (not a terrain-cube atlas swap), so animating it needs the same render-mode decision
  as the already-deferred Saw-blade/`ThinMechanical` items. No code changes.
- **feat: implement the real vehicle motor sound crossfade (plan.md SOUND-007/008, ch15-18/28-31).**
  Real `Decor::AdaptMotorVehicleSound()` gives Helicopter its own start/loop-high/loop-low/stop
  sound set (ch15/16/18/17) and Jeep/Tank/Overcraft share a second set (ch28/29/31/30) —
  Skateboard has no motor sound at all. New `BlupiController::HasVehicleMotor()`/
  `IsVehicleMotorHigh()` (the real per-mode "moving vs idle" pitch-select flag, translated as
  nonzero horizontal speed for ground vehicles / nonzero vertical velocity for flight modes) +
  `GalaxyEggbertGame::UpdateVehicleMotorSound()`, a direct port of the real crossfade state
  machine (one-shot start/stop sounds bracket the looped motor sound, exactly mirroring real
  `m_blupiMotorSound`'s sentinel-based transition logic). Closed 12 more stale/mislabeled
  `BLUPI-0xx` checkboxes along the way (the whole "Vehicle Modes" subsection was in the same
  not-yet-cross-referenced state as the earlier `BLUPI-104`-`117` batch) — found and corrected a
  wrong channel claim (`BLUPI-093`'s "ch53 = Tank fire sound" is actually the real out-of-ammo
  click; fire itself is ch52) and a repeated "Balloon" naming-trap mislabel (`BLUPI-096`/`148`:
  `ObjectType46` grants Overcraft, not a separate Balloon vehicle). Left genuinely open/blocked
  items alone (`BLUPI-100` Vent/fan propulsion — confirmed still entirely unmodeled; `BLUPI-101`
  Suspend — blocked on the deferred render-geometry decision; `BLUPI-087` helicopter debris —
  needs a particle system that doesn't exist). 4 new `VerifyBlupiMovement` assertions. Full
  regression clean (only the pre-existing unrelated `easy-gl-resource-smoke-tests` failure); live
  headless launch smoke check clean.
- **docs only**: closed 14 stale `BLUPI-1xx` checkboxes (Shield/Hide/SuperBlupi/Invert/Sucette/
  Ecrase/Dynamite/Perso/death-cause/footstep) — all already implemented under Phase 14/15/17 or
  `HUD`/`CAM` items, just never cross-referenced back. Corrected `BLUPI-113`'s "walk up walls"
  premise (real Sucette/Power effect is a jump-velocity boost only). Also fully ruled out
  `HUD-020`'s "EXIT OPEN!" popup (grepped the real localized-string resource file too — zero
  hits, same likely-invented status as its siblings). No code changes.
- **feat: implement the real Blitz-emitter zap ambient sound (plan.md SOUND-079/VISUAL-024,
  ch69).** Real `Decor::BlitzActif()` plays ch69 per visible Blitz(305)-floor tile that has a
  BlitzEmitter(304) tile directly above it, on a fixed 6-tick-per-100-tick pattern. Since this
  engine's `Sound::Play()` has no positional audio (just a channel), a one-time lazy world scan
  for "does any qualifying Blitz/BlitzEmitter pair exist anywhere" (new
  `InteractionSystem::HasBlitzEmitterPair()`, same "-1 = not yet scanned" idiom as the existing
  `totalTreasures_` lazy scan) is behaviorally equivalent to a real per-visible-tile check, at a
  tiny fraction of the cost — avoids a per-frame whole-world grid scan entirely. New
  `BlockTypes::BlitzEmitter=304` constant (icon 304 is `kPassable[304]==false`, so this engine's
  world loader already preserved it verbatim; just needed a named constant). The visual half of
  the same checklist item ("tiles 66-68 draw 13px higher") is a separate, unrelated `m_bigDecor`
  sprite-corner-anchor pixel nudge — confirmed non-portable, same category as other already-
  dismissed 2D-anchoring artifacts, left un-implemented deliberately. 4 new
  `VerifyInteractionSystem` assertions (lazy-scan correctness with/without a qualifying pair, full
  100-tick-cycle smoke run both ways). Full regression clean (only the pre-existing unrelated
  `easy-gl-resource-smoke-tests` failure); live headless launch smoke check clean.
- **feat: implement real water splash/bubble effects (plan.md PICKUP-078/079/080, SOUND-032/033/
  034/074, BLUPI-151).** Found while auditing the Pickups & Objects checklist: real channel 22 was
  wired backwards (playing on water ENTRY; all 4 real `PlaySound(ch22,...)` call sites are EXIT
  paths, `Decor.cpp:5356/5378/5392/5405`) and the real entry splash (ch23, "Plouf",
  `ObjectType14`) wasn't wired to anything. Fixed ch22 to fire on exit only; added ch23 + a real
  `InteractionSystem::SpawnWaterSplash()` spawn on the dry->Surf/dry->Nage transition
  (single-instance-gated via new `HasActiveObjectOfType()`). Added the jump-exit-specific
  "Tiplouf" splash (ch64, `ObjectType35`) — this engine approximates "deliberate jump exit" as
  `jumpPressed` at the exact exit frame (documented simplification, single-point collision has no
  equivalent to the real 3-way exit-cause distinction). Added the ambient rising bubble while
  swimming (ch24, `ObjectType15`, new `InteractionSystem::SpawnWaterBubble()` — scans the real
  water column above Blupi via `BlockTypes::isWater()`, spawns a bubble rising exactly that many
  tiles, self-deleting on arrival via the existing `AdvancePatrolStep()` ObjectType23-arrival
  branch, matching real source's identical treatment of both types there), triggered twice per
  ~3.5s cycle off `WorldRuntime::GetAnimPhase() % 70`. Also fixed 3 more `ObjectIcons.cpp`
  wrong-approximation-formula bugs found along the way (same bug class as the `explo5/6/8` fixes
  the day before): `ObjectType14`/`15`/`35` were using monotonic-range formulas instead of the
  real non-monotonic/shuffled `table_plouf`/`table_blup`/`table_tiplouf` transcriptions. 14 new
  `VerifyInteractionSystem` assertions (spawn/self-delete timing for both splash types, corrected
  icon values, bubble column-scan height + self-delete-on-arrival + no-op-with-no-water-column
  guard). Full regression: 78/78 minus the pre-existing unrelated
  `easy-gl-resource-smoke-tests` failure on `build-cna`; live headless launch smoke check clean
  on both backends.
- **docs only**: resolved both remaining `plan.md` §3 Open Questions on user request ("hlubší
  research na icon 95/440"). **Icon 95**: not ambiguous — `Tables.cpp:1872`'s
  `table_decor_eau1[6] = {92,93,94,95,94,93}` (triggered only when the placed grid icon is 92,
  `Decor.cpp:1068-1072`) makes 95 a pure animation-derived frame, never placed directly; already
  correctly implemented (`TerrainRenderer.cpp:298`'s `kAnimWater1`, TILE-018). **Icon 440**:
  `Decor::OpenDoorsTresor()` (`Decor.cpp:11636-11658`) confirms treasure-gated doors are
  `421 + (N-1)` for N treasures required, so 440 = a legitimate 20-treasure-door id — but a sweep
  of all 78 real `worlds/*.txt` files found 421-435/437 each placed at least once while 436/438/
  439/440 are never placed (highest real usage: 437 = 17 treasures); 436-439 still have valid
  `object-m.png` art despite being unused, so 440's specific out-of-bounds atlas gap is best
  explained as the atlas simply being sized for exactly 440 slots (0-439) — one short of the
  numbering scheme's theoretical max — a slot nobody ever needed art for, not a bug ever hit in
  the shipped game. No code changes; `BlockTypes.hpp` bounds deliberately left untouched as before.
- `3c65870` **docs only**: fixed 6 stale `TILE-0XX` entries — Fan (all 4 directions), Temperature,
  and Marine tile animations are already implemented in `TerrainRenderer.cpp` but were still
  marked not done. Flagged an honest, unresolved compass-direction-label mismatch for the 4 Fan
  entries (no gameplay consequence either way, this engine doesn't model wind-push physics).
- `8a18d21` **docs only**: fixed `ENEMY-005` (likely hallucinated "5s enemy respawn timer" —
  verified `Decor::ObjectDelete()` has no such timer, no "respawn"/"revive" found anywhere for
  enemies) and `ENEMY-027` (stale cross-reference to the already-corrected `136` premise).
- `f1a96b9` **docs only**: clarified `BLUPI-012`/`013`/`014` are architecturally superseded by
  this engine's own float-native position/3D collision, not gaps to port literally.
- `fa13247` **docs only**: fixed 5 stale/wrong `BLUPI-0XX` entries — crouch, respawn, and the
  position-history FIFO were already implemented but marked not done; `BLUPI-005`'s "look-up
  glide" premise looks hallucinated (no such mechanic found anywhere in `Decor.cpp`); `BLUPI-016`
  labeled the water-breath gauge as a generic "vehicle charge gauge".
- `f570da4` **docs only**: finished the `SOUND-0XX` checklist audit (3rd/final pass) —
  identified real meanings for the remaining "unknown" channels (2/5/6/32/34/35/39/65/68), fixing
  more wrong labels along the way (ch39 is crate-"Pop", not "key sparkle"; ch65 is the Mockery
  idle-taunt sound, not "suspend detach") and catching a 3rd copy of the already-cancelled
  hallucinated "stomp kill" claim (ch5, `SOUND-015`) that the first 2 cancellations missed. Every
  channel 1-92 now has a verified real meaning on record. No code changes.
- `ff328f7` **feat: implement the real crate-push loop sound (ch38).** Found during the checklist
  audit — real ch38 is the crate-push loop (not "electric arc"), a genuine missing sound for an
  already-working mechanic. New `Sound::Stop(channel)` (per-channel, since `StopAll()` would
  kill every other sound) + `InteractionSystem::CrateBeingPushedThisFrame()` signal +
  `GalaxyEggbertGame::wasPushingCrate_` turning it into a real start/stop loop.
- `a1c8672`/`9efe096` **docs only**: 2 earlier passes over `plan.md`'s `SOUND-0XX` channel
  checklist, cross-referencing which channels this engine already plays against the checklist's
  `[ ]` marks — 23 entries corrected total (many already wired but marked "not done"/"unknown";
  several with outright wrong real-channel labels, e.g. ch54/57/72/73/75).

- `2bab5b4` **docs only**: clarified that PICKUP-027 (`m_blupiTimeNoAsc`) and PICKUP-035
  (crate-land shake) aren't independent gaps — the first is genuinely part of `AscenseurVertigo`
  (same render-geometry block as `PICKUP-024`), the second needs crate fall-physics that doesn't
  exist (`151`), not "no shake system" as previously stated (stale since 2026-07-14).
- `b3cbe64` **feat: implement secret exit (`ObjectType21`) contact (`PICKUP-009`/`083`).** Verified
  against `Decor.cpp:6158-6184` — shares the exact same real exit-gate logic as the regular exit;
  this engine previously only recognized `ObjectType7`, so a secret exit did nothing on contact.
- `7d2e612` **docs only**: cancelled PICKUP-086/087 (hallucinated "shield trail sound" — real
  channels 48/49 are unrelated Ouf3/Ouf4 idle-fidget sounds; the real trail-spawn code has no
  `PlaySound()` call at all).
- `9505fd5` **fix: exact `table_explo5/6/8` transcriptions + resolve the explo-mapping open
  question.** Verified against `Decor.cpp:8395-8489`: the real `explo1-8`→`ObjectType` mapping is
  a plain 1:1 correspondence, not "many-to-one or context-dependent" as the open question in
  `plan.md` claimed. explo1/2/3/4/7 were already exact (2026-07-14); explo5/6/8 were the 3
  remaining cases still using an approximation formula, now exact.
- `3f577c8` **feat: implement the Perso-decoy/lethal-decor enemy trap mechanic.** Resolves a
  mystery left open since 2026-07-13 ("what does placing a Perso decoy actually DO gameplay-wise" —
  the original search targeted enemy-AI code and missed the real site). Verified against
  `Decor.cpp:7957-7975` + `Decor::MovePersoDetect()`: small enemies (4/32/33) that patrol into
  contact with ANY real 200-203 object (the placed decoy, OR one of the 201-203 lethal decorations
  just below) mutually destroy each other.
- `4fecb1e` **feat: implement `ObjectType201-203` lethal contact (`PICKUP-069`).** A real,
  previously-undiscovered gameplay gap (not a vehicle/immunity retrofit like the others above) —
  found via direct `Decor.cpp:6088-6115` read: types 201-203 share `ObjectType200` Perso's real
  range but damage Blupi on contact (`BlupiDead(Clear1, Clear2)` coinflip, Shield/Hide immune,
  always channel 10 + SmallShake + an `ObjectType10` pop effect). `plan.md`'s own note that "no
  contact-damage logic found" was wrong — easy to miss reading only around the Perso branch. New
  block in `InteractionSystem::Update()`, 6 new `VerifyInteractionSystem` assertions.
- `2284775` **docs only**: fixed 3 more stale Shield/Hide-immunity notes (fired projectiles,
  Spikes, Fan) that claimed immunity "isn't modeled" — all 3 were already fixed when `170` (secret
  powers) landed 2026-07-12, just never updated afterward. Also corrected `141`'s stale "Drip NOT
  done" claim (Drip was implemented 2026-07-14).
- `a7622a0` **fix: Shield/Hide now grant real immunity to the water breath gauge.** Verified
  against `Decor.cpp:4615-4620` — the gauge only decrements at all while NOT Shield/Hide (this
  engine decremented unconditionally). An already-documented "not modeled" gap, now closed.
- `6341319` **docs only**: fixed 2 more stale "vehicles aren't modeled" comments found during a
  final sweep (TriggerTeleport's caller-side comment, the secret-power Trigger*() header note).
- `9027b84` **fix: vehicles/Balloon/Ecrase now skip water Surf/Nage detection.** Verified against
  `Decor.cpp:5284-5285` — not a "forced dismount" as a stale comment claimed, water tiles just
  never register as Surf/Nage while riding/ballooned/squashed at all.
- `51e4d6f` **fix: `TriggerDeathLock()` now clears vehicle/Balloon/Ecrase/secret-power/Invert
  state.** The most significant fix this session — found while researching the large-creature
  contact: real `BlupiDead()` unconditionally clears ALL of this on every death, not a per-hazard
  special case. This engine's death-lock left it completely untouched across death/respawn.
- `8fdc23b` **fix: springs now forcibly dismount vehicles before bouncing.** Verified against
  `Decor.cpp:2837-2893`. New shared `DismountAndDepositVehicle()` helper (factored out of the
  existing voluntary dismount, which had the same logic inline).
- `2783a9b` **docs only**: confirmed HUD-010/011/013/021 are non-features by reading the complete
  real `DrawInfo()` function end to end.
- `ee91d1f` **feat: real per-cause DeathLocked/PickupBusy animation frames (Blupi-model
  prep).** Parsed `Tables::table_blupi` directly via a small script (validated by first
  reproducing the already-approved `kTeleportingFrames` byte-for-byte before trusting new output)
  to transcribe the 6 real `DeathCause` hurt-sprite frame arrays (Clear1-4/Glu/Drown) and 3 real
  `PickupFreezeKind` busy-animation frame arrays (Sucette/Drink/Charge) — replaces the previous
  static "Stop pose" stand-in. New `BlupiController::m_deathCause` (stored so `GetAnimIcon()` can
  select the right array; `m_pickupFreezeKind` already existed). Format-agnostic icon-index data
  only — no 3D model exists yet to actually display it, becomes visible once `069` ships. New
  `VerifyBlupiMovement` assertions (Clear1/Clear2/Sucette exact icon values). Full suite green both
  backends.
- `9ca723f` **fix: real vehicle-mode gate for crate push.** Verified against `Decor.cpp:6130-6132`
  — excludes every vehicle mode + Balloon/Ecrase. New `blupiCanPushCrate` parameter on
  `InteractionSystem::Update()`.
- `34a1ccf` **fix: `TriggerMount()` now excludes Balloon/Ecrase.** Verified against
  `Decor.cpp:5649/5669/5687` — these were missing from the mount gate entirely; its own header
  comment wrongly claimed Ecrase was "already handled elsewhere."
- `f575064` **fix: real vehicle-mode gate for Dynamite/Perso placement.** Verified directly
  against `Decor.cpp:4792-4794`: placement (Dynamite AND Perso, same `if`/`else if` gate) excludes
  EVERY vehicle mode (Helicopter NOT exempt here, unlike switches) + Balloon/Ecrase. New
  `blupiCanUseHands` parameter on `PlaceDynamite()`/`TryPerso()` — deliberately does NOT gate
  `TryPerso()`'s separate pickup-an-already-placed-decoy branch (real source has no vehicle clause
  there).
- `59db4ad` **fix: real Over/Jeep/Tank/Skateboard/Balloon gate for switch activation.** Verified
  against `Decor.cpp:5529-5531` — Helicopter IS exempt here (unlike every other vehicle gate this
  session), a hovering Helicopter can still reach down and press a switch. Caught and fixed a
  drafting mistake before committing: an earlier version wrongly gated the WHOLE shared
  action-button block, which would have broken vehicle dismounting.
- `2247c38` **fix: real Over/Jeep/Tank hazard immunity for Spike/Drip/Saw.** Verified against
  `Decor.cpp:5497-5528` — these 3 hazards specifically exempt Overcraft/Jeep/Tank (not Helicopter/
  Skateboard); Lava/Blitz/Crusher deliberately have no vehicle clause at all. New
  `BlupiController::HasVehicleHazardImmunity()`.
- `5b105c0` **fix: real vehicle-mode gate + Skateboard velocity for ground jump.** Verified
  against `Decor.cpp:2913-2947` — Jeep/Tank/Helicopter/Overcraft/Balloon can't ground-jump at all;
  Skateboard gets its own real distinct velocity (-17 Power/-13 noPower) instead of the
  headroom-modulated ordinary values. New `kSkateboardJumpSpeed`/`kSkateboardJumpSpeedPowered`.
- `4c85517`/`8dcc986` **docs only**: corrected 2 stale `plan.md` notes (`066`'s turning-duration
  table turned out to be a discrete 2D facing-flip mechanic, not a portable continuous turn-rate
  table; `067`'s "still missing hazard table" was already done by the death-lock system), and
  root-caused `easy-gl-resource-smoke-tests` as entirely an `../easy-gl`-side bug (its test expects
  `glActiveTexture(GL_TEXTURE0)` from `Texture::set_image_2d()`/`bind()`, but neither calls it) —
  confirmed pre-existing/unrelated (that repo's last commit predates this session by 11 days), not
  fixed here since it's a separate independently-developed repo.
- `1d99487` **fix: real vehicle-mode/Balloon/Ecrase pickup+teleport gates.**
  Verified directly against `Decor.cpp:6025-6087` and `:5593-5594`: Sucette(26)/Drink(30)/
  Charge(31) pickups and the teleporter both really exclude every vehicle mount plus Balloon/
  Ecrase (`!m_blupiHelico/Over/Balloon/Ecrase/Jeep/Tank/Skate`) — this engine had none of that for
  pickups, and `TriggerTeleport()` only checked Balloon/Ecrase (its own comment wrongly claimed
  "vehicles aren't modeled", stale since `VehicleMode` was added). Shield(25)/Invert(40) genuinely
  have no such clause in real source — confirmed, not just assumed; this closes §8 item 1 from the
  previous update (which had mistakenly also listed Shield). New `blupiVehicleOrSquashed` gate in
  `GalaxyEggbertGame.cpp`'s `canGrantPower/Cloud/Hide` computation; `BlupiController::
  TriggerTeleport()` now also checks `m_vehicleMode`. New tests in `VerifyBlupiMovement.cpp`
  (vehicle-mounted teleport no-op) and `VerifyInteractionSystem.cpp` (gated Sucette grant).
- `f6753ca` **fix: require the action button for Sucette/Drink pickups.** Real mobile-eggbert
  gates Sucette(26)/Drink(30) on the action button held at contact
  (`getButtonPressedProperty()==PlayAction`); this engine previously granted both automatically.
  Added `blupiActionPressedEdge` to `InteractionSystem::Update()`. Charge(31) deliberately has no
  such gate (confirmed via direct source read). New positive/negative tests added.
- `de6ef8b` **feat: implement real Sucette/Drink/Charge 2-stage pickup delay.** Real exact
  durations: Sucette=32 ticks(1.6s), Drink=36(1.8s), Charge=64(3.2s) — all three genuinely freeze
  Blupi, a third application of the freeze-timer pattern built for `TriggerTeleport()`/the death
  lock. Sucette/Drink defer their buff grant to completion; Charge's buff already grants at
  contact in real source (only its freeze/sounds were missing). New
  `BlupiController::TriggerPickupFreeze()`/`ConsumePickupFreezeResolved()`, new
  `InteractionSystem::RespawnPickupItem()`.
- `6a34aaf` **docs: confirm respawn invincibility window is a non-feature.** Researched
  `plan.md` item `103` — no invincibility flag/timer exists anywhere in real mobile-eggbert source;
  post-respawn safety is purely spatial (the FIFO safe-position system). Closed, not implemented.
- `b03b827` **docs: record live-visual verification of the death-lock system.** Temporary,
  fully-reverted debug scaffold confirmed the death→lock→life-loss-Voyage→respawn sequence live in
  the running game.
- `3261417` **feat: implement real death-lock + life-loss Voyage system.** Every real hazard death
  (Clear1-4/Glu/Drown) now locks Blupi for a real fixed duration
  (`Decor.cpp:6374-6392`: 70/100/70/110/100/90 ticks), then plays the real life-loss Voyage (icon
  48/Blupi channel, decrements lives at Voyage **start**, not completion) before respawning.
  New `BlupiController::TriggerDeathLock()`/`ConsumeDeathLockResolved()`/`IsDeathHidden()`. This
  also completes the real `Glu` "stuck" mechanic (it shares this same pipeline, no unique VFX of
  its own). Found and fixed a real nuance: 3 causes (not the 2 first suspected) skip the
  safe-position respawn — Fan, generic-hazard-contact, and dynamite blast.
- `fde1c54` **docs: document Glu death mechanic research.** No code changes — established that
  `Glu` requires the death-lock system above.
- `b8e5a77` **feat: implement Clear2/Clear3/Clear4 death VFX.** Of Blupi's 8 real
  `BlupiAction::Clear1`-`Clear8` types, only 3 have real VFX: Clear2/Clear3 fire a "soul ascends"
  HUD Voyage (icons 230/40), Clear4 (Saw) fires a 3-direction particle burst. Clear1 has none;
  Clear5-8 are confirmed dead code (never assigned anywhere in real source).
- `4ff297a` **fix: restore real immediate Dynamite/BulletPack Voyage sounds.** A bug found via a
  fresh source re-read: both had been wrongly ported as silent.
- `5a424e3` **feat: implement faithful Voyage pickup-reward system.** Real mobile-eggbert defers
  pickup rewards (treasure/keys/egg/dynamite/Perso/bullet pack/door unlock) behind a 2D
  "fly to HUD icon" animation instead of applying them instantly on contact. New
  `InteractionSystem::VoyageKind`/`BeginVoyage()`/`TickVoyage()`, new
  `Hud::ProjectWorldToHudSpace()` (world→screen projection, didn't exist before).

## 4. Current blocker / main problem

**There is no active build or test failure blocking progress.** `build-cna` builds and passes
84 tests with the pre-existing `easy-gl` dependency test excluded (see §5). The original 3D world
editor plan is fully complete, and user-directed follow-ups through `EDITOR-130` are implemented.

**Resolved 2026-07-19, re-verified 2026-07-23**: `build-cna-vulkan` builds and passes `ctest`
cleanly through the entire editor line of work (EDITOR-100..112) and every INFRA-*/BUILD-*
addition since, confirming everything is genuinely backend-agnostic.

The remaining blockers are items that genuinely need a **human decision**, not more engineering:
- `AscenseurVertigo` (wide/shiftable lift platforms, icons 311-316) — needs the user to choose
  which of 3 existing render approaches to use.

(Saw blade (icon 378) render orientation — resolved 2026-07-20 after a 6-round live-feedback
session; see §3's own writeup.)

If picking up this project without that answer, the honest "next thing to do" is one of the
concrete, non-blocked tasks in §8 below, not a bug fix.

## 5. Known bugs and limitations

- **Resolved 2026-07-20** (was here as a confirmed bug for over a week): Saw blade (icon 378)
  render orientation. See §3 for the full 6-round live-feedback history.
- **Resolved 2026-07-21** (INFRA-005, plan.md §7): the airborne-wall-clip gap found 2026-07-18 while
  implementing Balloon's horizontal drift (`BlupiController::TryMoveAxis()`'s step-up gate only
  applying while `m_onGround`) is fixed — `ResolveMove()` now applies the same real wall check
  unconditionally for every mode (plain jumping, every vehicle, Balloon's drift), matching real
  `Decor::BlupiStep()`'s own unconditional `TestPath()` call. See §3's own writeup for the full
  resolver rewrite this came from.
- **Incomplete:** No visible 3D Blupi model (blocked on the user providing one).
- ~~Incomplete (world editor, by design — the remaining plan milestone): no unsaved-changes guard
  (EDITOR-112)~~ — **done 2026-07-23**, see §3/plan.md §6's own `EDITOR-112` entry: Back is now a
  real 2-tap confirm when there are unsaved changes (dirty flag + 2nd-press-to-discard), unchanged
  single-tap otherwise.
- **Deliberately limited editor text:** functional text now covers XYZ coordinates, numbered
  background thumbnails, browser labels, and the two-second not-implemented notice. The main tool
  palette remains icon-driven rather than becoming a general text UI.
- **Resolved 2026-07-19:** `build-cna-vulkan` rebuilt/re-tested (see §4) — 76/76 (100%).
- **Risky assumption (world editor):** `ObjectType` category names/membership in
  `PaletteCategories.cpp` were taken **only** from `ObjectType.hpp`'s own documented comment
  groups. The "Confirmed" tab deliberately excludes types the game spawns itself (explosions,
  splashes, projectiles, door/bridge animations) — they remain reachable via "All Types". If a type
  turns out to be placeable/non-placeable contrary to that grouping, fix the grouping rather than
  inventing new semantics for it.
- **Fixed 2026-07-16:** Sucette(26)/Drink(30)/Charge(31) pickups and `TriggerTeleport()` now check
  vehicle mode + Balloon/Ecrase (`Decor.cpp:6025-6087`/`:5593-5594`). Shield(25)/Invert(40) confirmed
  to have no such clause in real source (the previous entry here mistakenly listed Shield). See §3.
- **Fixed 2026-07-16** (Blupi-model prep, format-agnostic — see §7.5): the real per-cause
  hurt-sprite animation frame table (`Tables::table_blupi`) is now transcribed for both the
  death-lock (`AnimState::DeathLocked`, 6 causes) and pickup-freeze (`AnimState::PickupBusy`, 3
  kinds) states, replacing the static "Stop" pose stand-in — see §3. This is icon-index data only
  (no 3D model exists to actually display it yet); it becomes visible once `069` ships.
- **Needs verification:** `easy-gl-resource-smoke-tests`'s single failing assertion
  (`test_texture_upload_sets_unpack_alignment_wrap_and_unit0_binding`,
  `../easy-gl/tests/smoke/SmokeResourceTests.cpp:336`) — **root-caused 2026-07-16, confirmed
  entirely in `../easy-gl`, not galaxy-eggbert.** The test does `texture.set_image_2d(...)` then
  `texture.bind(Texture2D)` and asserts `g_state.last_active_texture == 0x84C0` (`GL_TEXTURE0`),
  but neither `Texture::set_image_2d()` nor `Texture::bind()` (`../easy-gl/src/Texture.cpp:52,118`)
  ever call `glActiveTexture` — only the separate `Texture::active_bind(unit, target)` does (line
  57). `g_state.last_active_texture` is never written, so it stays at its `reset_state()` default
  (0), not `0x84C0`, and the `assert()` aborts. Either the test should call `active_bind()`/not
  assert this, or `set_image_2d()`/`bind()` should call `glActiveTexture(GL_TEXTURE0)` first (both
  plausible fixes) — that decision belongs to `../easy-gl`'s own maintainers/plan (its last commit,
  `5a50c69`, predates this session by 11 days, confirming it's genuinely pre-existing, not
  introduced or affected by anything done here). Not fixed in this session — it's a different git
  repository under its own independent development (same caveat as `../sharp-runtime`, see §1), out
  of scope to modify without the user's direction.
- **Risky assumption to keep in mind:** `InteractionSystem` is deliberately decoupled from both
  `BlupiController` and any camera/graphics type (see §6). Every new feature that needs either
  has had to route through a same-frame "pending signal" (`*ThisFrame()` flags) consumed by
  `GalaxyEggbertGame`. It is tempting to "simplify" this by just passing a `BlupiController&`
  into `InteractionSystem::Update()` — don't; this has been a deliberate, repeated architectural
  choice across at least 3 features this session (Voyage, death-lock, pickup-freeze), not an
  oversight.

## 6. Architecture notes

**Core classes** (all in `src/GalaxyEggbert/Game/` unless noted):
- `BlupiController` — Blupi's own movement/physics/state machine. Owns position, velocity,
  secret-power state, vehicle mode, and now the death-lock and pickup-freeze timers. Has **no**
  dependency on `InteractionSystem`, `Sound`, or world/graphics types. Its freeze-timer shape
  (`TriggerTeleport()`/`m_teleporting`, `TriggerDeathLock()`, `TriggerPickupFreeze()`) is a
  reusable template: set a flag + a duration, `Step()` early-returns while the flag is set,
  auto-clears when the timer elapses. Reuse this exact shape for any future "freeze Blupi for N
  seconds" mechanic rather than inventing a new one.
- `InteractionSystem` — pickups, hazards, enemies, doors, lifts, crates, secret-power grants,
  cheats. Deliberately has **no** access to `BlupiController`, `Easy3D::Camera3D`, or the
  `GraphicsDevice` — every real mechanic that needs Blupi's controller state or screen-space
  projection communicates via same-frame `*ThisFrame()` boolean signals (and, where multi-frame
  state is needed, a small pending-request struct) that `GalaxyEggbertGame` reads and acts on
  right after `Update()` returns. Do not "fix" this by adding a direct dependency — see §5's own
  note.
- `GalaxyEggbertGame` (`src/GalaxyEggbertCNA/GalaxyEggbertGame.hpp`/`.cpp`) — owns `blupi_`,
  `interaction_`, `worldRuntime_`, `camera_`, `sound_`, `hud_`. This is where cross-class
  orchestration lives: `ResolvePendingVoyage()`, `ResolveDeathLock()`, `ResolvePickupFreeze()` are
  all called once per frame, right after `interaction_.Update()` returns, to consume pending
  signals from `InteractionSystem` and drive `BlupiController`/`Hud` accordingly.
- `WorldRuntime` — the live, mutable per-frame world state (terrain block queries plus the
  `MobileObjSpec` list of every pickup/enemy/effect object). `InteractionSystem` and
  `GalaxyEggbertGame` both operate on the same `WorldRuntime&` instance each frame.
- `BlockDefinitionRegistry` / `ObjectDefinitionRegistry` — the authoritative, backend-independent
  block and MoveObject type metadata. Rendering, collision, editor validation, imported-world
  support, texture-sheet selection, and shared declarative MoveObject capabilities consume these
  definitions; do not reintroduce parallel
  icon/type classification lists in consumers.
- `Hud` — 2D HUD rendering in mobile-eggbert's own 640x480 reference space, plus the
  `ProjectWorldToHudSpace()` static utility (world position → that reference space, via a real
  clip-space `Vector4::Transform` then inverting `Hud`'s own ref↔viewport scale/offset math).
- `include/GalaxyEggbert/Worlds/`, `Def/*.hpp`, `BlockTypes.hpp`,
  `BlockDefinitionRegistry.hpp` — engine-agnostic data model
  (`World`/`Chunk`/`Block`, real enum IDs). Shared by any future engine target; keep it that way
  (no CNA/Easy3D-specific dependencies here).

**World editor** (`src/GalaxyEggbert/Editor/`, new 2026-07-18):
- `WorldEditor` is the only class `GalaxyEggbertGame` talks to. It owns the free-fly camera,
  raycast/highlight state, undo stack, palette, box-fill state, and browsing/editing mode.
- It follows the **same "pending signal consumed by the owner" idiom** as `InteractionSystem`:
  it has no `WorldRuntime`, phase, or file-loading access, so it reports
  `ConsumeNeedsPresentationRebuild()` / `ConsumePlayTestRequested()` / a `BrowserRequest`, and
  `GalaxyEggbertGame` performs the actual rebuild/phase-switch/load. Keep it that way.
- The lower helpers (`VoxelRaycast`, `BoxRegion`, `EditCommandStack`, `PaletteCategories`,
  `CustomWorldStorage`) are **pure logic with no CNA/Easy3D dependency**, which is what lets
  `VerifyGEWorldEditor` link and test them headlessly. Do not introduce graphics dependencies into
  these five — it would silently cost the whole test suite.

**Coordinate invariants (the single easiest thing to get wrong here):**
- **Block centering:** block index `N` spans `[N-0.5, N+0.5)` — *not* `[N, N+1)`. Position→index
  conversion uses `std::lround`, not `floor` (see `WorldRuntime`). `VoxelRaycast` bakes the
  corresponding `+0.5` shift into its DDA.
- **Render space vs raw grid space:** render space = raw grid space shifted by
  `-WorldRuntime::kWorldCenterX/Z` (= 50) on **X and Z only**; Y is identical in both.
  `Worlds::World` and `MoveObjectRecord` are always raw grid space; the camera is always render
  space. `WorldEditor::Update()` converts internally so callers never apply the shift themselves.
- **`MoveObjectRecord` anchoring:** a record is stored in the block-extra-metadata of
  `floor(posStart)`, and at most one record may occupy a given anchor cell — placing a second
  silently replaces the first. Undo of an overwrite must therefore restore the previous record, not
  just clear the cell (`EditCommandStack::ApplyMoveObjectState` does exactly this).
- **`RebuildWorldPresentation()` now calls `WorldRuntime::ResyncFromWorld()` first.** The editor
  mutates the live `World` in place with no disk round-trip, so `LoadFromVwrFile()` — previously the
  only thing that populated `mobileObjects_`/`skyRegion_`/`missionNumber_` — never runs for an
  in-editor edit. Removing this call makes placed objects invisible until save+reload.
- **Palette vs world clicks:** `EditorPalette` claims the mouse from the **press** frame onward
  (per press, not per cursor position), because world edits are press-triggered while the palette
  acts on release. Don't "simplify" this back to a release-only check — see §2's bug note.

**Invariants / boundaries that must not be broken:**
- `../mobile-eggbert` is never modified, not even temporarily, not even for "just looking."
- `../simple-3d` is read-only.
- No `#ifdef` guards distinguishing engine backends anywhere in `GalaxyEggbertCNA`.
- `InteractionSystem` stays free of `BlupiController`/camera/graphics dependencies (see above).
- Real numeric IDs (`ObjectType`, `BlockTypes`, sound channels) must never be renumbered — they
  encode the real mobile-eggbert data format.
- `SaveData`/`InputPad`/`InteractionSystem` byte layouts are intentionally not compatible
  with real mobile-eggbert's own save/format — don't "fix" this without a deliberate, separate
  decision (see `CLAUDE.md`'s reuse table).

## 7. Useful commands

Configure + build (EasyGL, the default backend):
```
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON
cmake --build build-cna --target GalaxyEggbertCNA -j2
```

Configure + build (Vulkan backend):
```
cmake -S . -B build-cna-vulkan -DGALAXY_EGGBERT_BUILD_CNA=ON -DCNA_GRAPHICS_BACKEND=VULKAN
cmake --build build-cna-vulkan --target GalaxyEggbertCNA -j2
```
(Use `-j2` maximum — this environment has crashed under more parallel jobs.)

Run the game (must run from its own build dir — relative asset paths):
```
cd build-cna && ./GalaxyEggbertCNA
```

Run the full test suite:
```
cd build-cna && ctest
cd build-cna-vulkan && ctest
```

Build and run one specific verification tool directly (bypasses ctest's summary, shows every
`PASS:`/`FAIL:` line):
```
cmake --build build-cna --target VerifyInteractionSystem -j2
cd build-cna && ./VerifyInteractionSystem
```

Build + run the world-editor verification suite (the relevant one for all EDITOR-* work — prints
every `PASS:`/`FAIL:` line; expect 165 checks and `ALL CHECKS PASSED`):
```
cmake --build build-cna --target VerifyGEWorldEditor -j2
cd build-cna && ./VerifyGEWorldEditor
```

Run the engine-agnostic world-model gtest suite directly (expect 66/66):
```
cmake --build build-cna --target GalaxyEggbertWorldsTests -j2
cd build-cna && ./GalaxyEggbertWorldsTests
```

Reproduce the one known ctest failure (pre-existing, in the `easy-gl` dependency — **not** a
galaxy-eggbert bug, see §5):
```
cd build-cna && ctest -R easy-gl-resource-smoke-tests --output-on-failure
```

Headless live run (for visual verification of editor/gameplay changes; needs `xvfb-run`):
```
cd build-cna && timeout 80 xvfb-run -a ./GalaxyEggbertCNA
```

`INFRA-001`'s deterministic golden-screenshot capture (plan.md §7): loads the fixed demo world,
runs a fixed 180 ticks, writes `golden_frame_0060/0120/0180.png`, then exits on its own (needs
`xvfb-run`, no `timeout` required since it self-terminates — confirmed via 2 independent runs
producing byte-identical PNGs). **Not wired into the default `ctest` run** — like the live headless
check above, it needs a real display/GL context, which isn't guaranteed in every environment that
runs `ctest`; run it explicitly:
```
cd build-cna && xvfb-run -a ./GalaxyEggbertCNA --golden-capture
```

`INFRA-002`'s golden-image diffing (plan.md §7): runs the capture above and byte-compares each
frame against the approved reference under `tests/golden/`, printing `PASS`/`FAIL` per frame
(same precondition as `INFRA-001` — needs a real display, not wired into default `ctest`):
```
xvfb-run -a tools/verify_golden_frames.sh build-cna
```

`INFRA-002`'s "behavioral trace" half (plan.md §7, closed 2026-07-22): a SEPARATE mode,
`--golden-capture-trace`, that ALSO drives Blupi through a small scripted walk+jump down the
tested corridor (unlike `--golden-capture` above, which never feeds him any input at all) and
diffs the resulting per-tick `golden_trace.txt` against `tests/golden/golden_trace.txt`:
```
xvfb-run -a tools/verify_golden_trace.sh build-cna
```

Regenerate the sample world (only needed after editing `tools/GenerateSampleWorld3D.cpp`):
```
cmake --build build-cna --target GenerateSampleWorld3D -j2
cd build-cna && ./GenerateSampleWorld3D
```

No lint/format tooling is configured in this repository at present.

## 7.5. Standing directives for the current autonomous session (2026-07-16)

The user authorized an extended unattended session and pre-answered the questions that would
otherwise block it. These answers stand for the remainder of THIS session (re-confirm with the
user before treating them as permanent beyond it):

- **Skip every task that needs visual judgment on a render/screenshot entirely** — ~~Saw blade
  orientation~~ (**resolved 2026-07-20**, see §3), `ThinMechanical` geometry (`510`), water surface
  treatment (`512`), architectural-kit assembly (`514`), enemy billboard walk-cycle angle (`179`),
  `AscenseurVertigo` (`153`/`PICKUP-024`). Do not touch their rendering code or take "best guesses"
  at geometry — leave them exactly as flagged until the user can look at a screenshot themselves.
- **`SaveData` stays an independent format, not byte-compatible with real mobile-eggbert saves**
  (closes `E3D-MIG-106`/the open question in §3 of the main doc) — don't expand `SAVE-*` scope
  toward byte-layout matching.
- ~~**No 3D world editor work this session** (`EDITOR-000..010`) — a dedicated future effort, not
  part of this one.~~ **SUPERSEDED 2026-07-18**: the user explicitly commissioned the editor as its
  own effort and approved a 13-milestone plan (EDITOR-100..112), answering three design questions
  up front: it is an **in-game mode** inside `GalaxyEggbertCNA` (not a separate executable — this
  also overrides `plan.md`'s own `EDITOR-000` placeholder recommendation), custom worlds belong to
  the **existing 3-gamer-slot** save system, and the **full feature set** was requested rather than
  an MVP. A further standing instruction from that session: **any investigation of free-eggbert's
  code must be delegated to a subagent** that returns only what's needed, to keep it out of the
  main context.
- **Blupi-model prep IS authorized, format-agnostic only**: e.g. animation-state timing/signal
  plumbing, third-person placeholder improvements, asset-loading scaffolding that would work
  regardless of the eventual real model's exact format/rig. Do NOT commit to a specific model
  format or file layout without the user's input.
- Similarly discovered *this session*: rescaling `kGravity`/`kJumpSpeed`'s own absolute magnitude
  to real tick-domain values (part of `065`) needs the user's live-feel judgment, same as the
  visual-judgment items above (see `065`'s own plan.md entry for why) — left open, not a green
  light to guess.

## 8. Next smallest tasks

**The original editor plan is complete** (EDITOR-100 through EDITOR-112, see §3/plan.md §6), and
the user-directed follow-ups through `EDITOR-130` are implemented. Further editor work is selected
as a new follow-up rather than silently extending the original plan.

1. ~~EDITOR-111 — Sky-region picker.~~ — **done 2026-07-23**, see §3's own writeup and `plan.md`
   §6's `EDITOR-111` entry for the full history (including the live Xvfb+`xdotool` screenshot
   verification and a deliberate bug-injection teeth-check).

2. ~~EDITOR-112 — Hardening pass + full regression.~~ — **done 2026-07-23**, see §3's own writeup
   and `plan.md` §6's `EDITOR-112` entry for the full history: unsaved-changes guard (dirty flag +
   2-tap confirm on Back — scoped to that one call site after confirming it's the only real path
   out of an active editing session in this codebase), a box-fill test straddling the world's own
   Z=0/Z=99 bounds, and a completed top-of-file section index (found the file's section-naming
   already consistent throughout, so a wholesale physical reorder wasn't justified — see the plan.md
   entry for the full reasoning). `build-cna-vulkan` re-verified clean throughout.

3. ~~EDITOR-113 through EDITOR-122 — menu/layout maintainability and user-directed usability
   follow-ups.~~ — **done 2026-07-25**, see `plan.md` §6. The latest items add a Galaxy-only
   background group with 32 live thumbnails, make the top-left red-X control remove its current
   target through the undo/redo command stack, and replace three false temporary notices with
   verified Inverter, Wooden case, and Hovercraft mappings.

4. ~~EDITOR-123 through EDITOR-127 — complete all remaining source-menu entries.~~ —
   **done 2026-07-25.** All 96 cells have one exact implementation; exhaustive real-pointer tests,
   the complete native suite, and the final 800×480 visual inspection pass. See `plan.md` §6.

Non-editor tasks:

1. ~~Take the live screenshot verification of the death-lock/life-loss Voyage one step further~~
   — **done 2026-07-20.** Temporary, fully-reverted debug scaffold (env-var-gated: skip to Play,
   overwrite the ground block directly under Blupi's spawn with Lava via
   `WorldRuntime::GetWorldMutable().setBlock()`, periodic stderr state log + timed screenshots
   captured post-HUD-draw, gated on `drawFrameIndex_ > terrainPixelPrintedFrame_` same as
   `screenshot_hud.png`'s own established fix) confirmed the FULL real sequence live: Lava contact
   → real Clear3 lock (70 ticks=3.5s, with a concurrent Clear3Ascend cosmetic VFX voyage, icon 40,
   fixed 50 ticks=2.5s, completing partway through) → LifeLoss Voyage begins (icon 48, fixed 40
   ticks=2.0s) with lives visibly dropping 3→2 in the SAME frame the icon starts (matches the real
   "decrements at Voyage START, not completion" citation exactly) → icon 48 now VISIBLY CAUGHT
   mid-flight in a screenshot (a small icon near the HUD's treasure-counter row, present partway
   through the window, gone once the Voyage completes) — the specific gap the previous pass
   (`b03b827`) missed. No production code changes survive (confirmed via `git diff` showing zero
   diff after revert).
2. ~~Get the user's visual judgment on the Saw blade (icon 378) orientation, then fix it.~~ —
   **done 2026-07-20**, see §3's own 6-round writeup.

3. **Get the user's decision on `AscenseurVertigo` render geometry** (icons 311-316, which of the
   3 existing render approaches to reuse), then implement it.
   Files: `src/GalaxyEggbert/Game/TerrainRenderer.cpp`.
   Verify: `cmake --build build-cna --target VerifyTileUvBounds -j2` plus a live screenshot.

4. **`plan.md` §7's correctness-infrastructure task breakdown** (2026-07-21, `INFRA-001`
   through `INFRA-010`) — this whole paragraph is now stale (INFRA-005/006/BUILD-011 landed since
   it was written) and had 4 outright "done" overclaims (external audit, 2026-07-22, independently
   re-verified against `REMAKE-ANALYSIS.md`'s own wording before accepting — see `plan.md`'s own
   `INFRA-002`/`003`/`004`/`005` entries for the specific corrections). Read those entries directly
   rather than trust a status summary here — in short: `INFRA-001`/`007`/`009` hold up as described;
   `INFRA-002` had a real script bug (fixed) and was missing P0-1's "behavioral trace" half —
   **closed 2026-07-22** (`--golden-capture-trace`/`verify_golden_trace.sh`, see plan.md's own
   `INFRA-002` entry); `INFRA-003` was a regression lock only, missing the programmatic
   reference-doc cross-check P0-2 asked for — **closed 2026-07-22**
   (`tools/VerifyObjIconAgainstReferenceDoc.cpp`, see plan.md's own `INFRA-003` entry);
   `INFRA-004`'s table is unused by the actual renderer (a
   documented reference list, not automatic enforcement); `INFRA-005`'s 3-sequential-per-axis
   deviation from P1-1's literal single merged-position resolve — **closed 2026-07-23**, true merged
   X+Z+Y `ResolveMove()` now landed after a scoping session picked the most ambitious of 3 offered
   options, see §3's own writeup and `plan.md`'s `INFRA-005` entry for the full history (including a
   real ~19x walking-speed regression found and fixed during implementation, not just the merge
   itself). `INFRA-006` is an honest partial (7 focused families/extractions migrated).
   `INFRA-010` is now complete: `CURRENT.md` is the compact current-status index; use its own
   document-ownership table rather than adding another summary here.

## 9. Do not do yet

- **Do not resurrect the retired pre-CNA engine path.** Its last source state is already preserved
  in git history at `4afd53e`.
- **No modification of `../mobile-eggbert`**, not even temporarily, not even to "just check
  something" — copy a file out first if a working copy is genuinely needed.
- **No copying mobile-eggbert code/data** (tables, enums, byte layouts) into this repository
  without explicit user approval, even when it looks like "just data."
- **No new invented gameplay mechanics.** Every feature must trace to something confirmed in real
  mobile-eggbert source or `mobile-eggbert-reference/` — verify before implementing, don't guess
  from plausibility. **The world editor is the one approved exception, and only for editor UX**
  (tools, palettes, camera, undo): the editor must not introduce gameplay behavior that
  mobile-eggbert doesn't have.
- **No expanding editor scope beyond EDITOR-110/111/112** until those three are done — no text
  rendering, no real per-type object icons, no copy/paste or brush tools, no terrain generation.
  Each is defensible later; none is in the approved plan.
- **No graphics dependencies in the five pure-logic editor helpers** (`VoxelRaycast`,
  `BoxRegion`, `EditCommandStack`, `PaletteCategories`, `CustomWorldStorage`) — it would
  break `VerifyGEWorldEditor`'s headless linkage (see §6).
- **No whole-file `git checkout` to revert live-test instrumentation.** It can silently discard
  real uncommitted work in the same file (this happened once during EDITOR-109 and had to be caught
  and restored). Revert with targeted edits, then confirm with `git diff`.
- **No refactor of the `InteractionSystem`/`BlupiController` decoupling** (see §5/§6) — it is a
  deliberate, repeatedly-reaffirmed design choice, not technical debt.
- ~~No 3rd guess at the Saw blade orientation without the user's own visual input~~ — **resolved
  2026-07-20**, see §3. Still: don't second-guess it again without a fresh, explicit user report.
- **No broad refactor or unrelated cleanup** while any of the §8 tasks are in flight — each is
  meant to be a single, small, independently-verifiable session.
- **No Lua, MeshCraft, Mesh World, Nova3D, or alternate engine path** — none is part of the locked
  Direct-CNA-+-Easy3D direction.
- **No starting `INFRA-005`/`INFRA-006` (`plan.md` §7's shared collision resolver / `ObjectType`
  handler-table task entries), or any dual-renderer work beyond the already-merged, unwired
  `SceneFrame.hpp`,** without the user's own explicit go-ahead on that specific item — see §7's
  own "do not bundle into one modernization effort" framing. `INFRA-001`/`003`/`004` (§8 task 4)
  are fine to pick up on their own.

## 10. Resume prompt

**Status as of the end of the 2026-07-23 autonomous session**: repository is clean, builds and
passes on all 3 native backends (`build-cna`/`build-cna-vulkan`/`cmake-build-debug`, 81/82 or 79/79
— the one failure is the pre-existing unrelated `easy-gl-resource-smoke-tests`), everything is
committed and pushed to `origin/develop`. This session (in order): fixed a real ~19x horizontal-
walking-speed regression in the just-approved `ResolveMove()` merge (INFRA-005 follow-up); migrated
2 more `INFRA-006` families (vehicle mapping, patrol-enemy icon dispatch), one predicate
extraction, and one terminal-patrol-arrival table; **completed the entire 3D world editor plan** (EDITOR-111 sky-region picker, EDITOR-112
hardening pass — all 13 milestones, EDITOR-100..112, now done); re-investigated and closed a stale
"grass-topped cubes walkable-through" bug report (does not reproduce — was step-up behavior, not a
bug); swept for and fixed a compiler warning; closed a 3-pickup-wide test-coverage gap (Dynamite/
Egg/BulletPack "second instance at the cap"); a further broader audit then found and fixed a real
out-of-bounds **crash** in `SaveData::Load()` (an unvalidated `selectedGamer` from a corrupted
save file segfaulted on the very next accessor call — confirmed via deliberate bug injection) plus
unified a duplicated `kMaxEggCount` constant; a final systematic public-API-vs-test-coverage sweep
(every public method in `BlupiController`/`ObjectIcons`/`InputPad`/`WorldRuntime` grepped
against its own test file) closed 3 more zero-coverage gaps (5 texture-atlas-selection predicates,
`TriggerBye()`, `TriggerMockery()`); one more follow-up audit with yet another fresh angle (file-I/O
trust boundaries — external/parsed data flowing unvalidated into array indices or numeric
conversions) swept every disk-reading path in `src/GalaxyEggbertCNA/`/`src/GalaxyEggbert/` and found
one more real bug: an uncaught `std::stoi` crash in `LoadFromMobileEggbertFile()`'s `Decor:`/
`BigDecor:` cell parser (confirmed via bug injection, fixed via a `SafeStoi()` wrapper), plus 2 minor
hardening gaps consciously left alone (see plan.md's TEST-005 follow-up note for the reasoning); one
final targeted audit (editor undo/redo/command-stack invariants — a fresh, specific angle, not a
repeat of the generic sweeps) came back **clean, nothing found**, after genuinely careful direct
code reading (not a rushed pass) — see §3's own entry for exactly what was checked. See §3 for each
entry's full writeup.

**Genuinely next, in order of what's actually startable**:

1. **The code-quality/edge-case audit category is now confirmed exhausted for this session, not
   just narrowing.** 8 rounds (TODO/FIXME sweep, stale-doc sweep, compiler warnings, a broad
   edge-case pass that found the `SaveData` crash, a systematic public-API sweep, a file-I/O
   trust-boundary sweep, and a targeted editor-invariant audit) found real work every time except
   the last, which came back genuinely clean after a careful, specific-angle look. Don't reach for
   "audit round 9" with another generic sweep as the default next move — the remaining items below
   are the actual backlog, and most need the user's input before proceeding (visual/design judgment,
   or scope confirmation for large content work). If truly nothing else is startable, that is the
   correct point to report back and ask, not to manufacture another audit.
2. **`AscenseurVertigo` render geometry** (icons 311-316) and **Suspended/hanging-bar mode**
   (blocked on a NEW "thin-bar" render geometry for icon 202) both need the user to look at a
   screenshot/mockup and choose an approach — see plan.md §0's "Known open bugs" and the
   `Suspended` writeup in plan.md's `171`/`178` area. Do not guess at geometry; ask first.
3. **Residual seam-line transparency** (plan.md's pre-`## 1` preamble list) has 2 untested
   hypotheses (MSAA edge AA; UV inset too small at extreme viewing angles) but assessing either
   fix ultimately means judging "does this look better," the same category of task explicitly
   deferred this session for the 2 items above — don't start this without checking whether the
   user wants to weigh in live first.
4. **`TILE-005`** (`plan.md`) — 4 more hand-authored 3D worlds (Grassland, Forest, Ice Caves, Lava
   Fields, Space Station; only one small sample world exists so far) is real, substantial, valid
   work, but large-scope content creation, not a quick task — confirm scope/priority with the user
   before starting rather than assuming it's wanted next.
5. `INFRA-006`'s remaining `ObjectType` families: already surveyed twice this session with
   genuinely diminishing returns (most of what's left is either already covered by existing
   predicates or has real per-type behavioral divergence unsuited to a shared table — see plan.md's
   own `INFRA-006` entry for the specific candidates already ruled out). A fresh survey could still
   be tried, but temper expectations.
6. ~~`INFRA-010`~~ — **done 2026-07-23**: [CURRENT.md](CURRENT.md) is the compact current-truth
   index; keep its status fields current and leave detailed history in this file and `plan.md`.

If picking this up cold: read this whole file first, then `plan.md`'s most recent (top-dated)
entries under whichever section a task references, not just this prompt.
