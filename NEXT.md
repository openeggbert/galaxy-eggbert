# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a
Windows Phone XNA game from 2013). Main goal: reimplement the same game logic, levels, and assets
in 3D — perspective camera, billboard sprites, 3D-rendered tiles — without inventing new mechanics.

**Current development phase:** mid-migration between two build targets:

- `GalaxyEggbertSimple3D` (built on `simple-3d` → U3D/Urho3D) — the **only playable** target,
  feature-complete enough to play through core mechanics end-to-end (see §2).
- `GalaxyEggbertCNA` (built directly on **CNA** + **Easy3D** helper library) — the **new
  long-term target**. Currently: opens a window, loads a genuinely 3D, hand-authored `.vwr` world
  (`worlds3d/world001.vwr` — real Y variation, not a flat mobile-eggbert layout), and renders real,
  textured terrain across all Y layers (one cube per non-air world cell, using `object-m.png`) with
  working animated tiles (lava/crusher/saw/spike/water/fan/marine/temp), and moves an invisible,
  collision-only Blupi placeholder around it (arrow keys + Space, grid collision with step-up
  traversal and gravity — no sprite yet). No object/pickup rendering, no HUD, no sound, no real
  gameplay yet.

**Important architectural decisions** (recorded in `plan.md`/`easy3d.md`/`CLAUDE.md`):

- Direct CNA + Easy3D supersedes the old Simple3D → U3D → Nova3D direction as the long-term
  target. `GalaxyEggbertSimple3D` is **not being deleted** — it stays as the working reference
  until the CNA/Easy3D path reaches feature parity with it.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching, mesh building,
  renderer adapters) — it must not hide CNA; game code may call CNA directly.
- `../mobile-eggbert` is **read-only**. Its assets (PNGs, sounds, world files) are freely reused
  by direct file path / build-time copy. Its code (`Pixmap`, `Sound`, `Decor.cpp`, tables, enum
  values) is reference-only — copying or linking requires explicit user approval first.
  mobile-eggbert currently has **no CMake library target** (only `add_executable`), so it cannot
  be linked as a dependency today regardless of approval.
- Mobile-eggbert's world files are flat 2D data (Y=0 everywhere) and are reference/inspiration
  only for world design — they are **not** the intended long-term data source for
  `GalaxyEggbertCNA`. There is **no** and will be **no** automated `.txt → .vwr` converter that
  "promotes" a 2D level into a 3D one (rejected direction, unchanged). `GalaxyEggbertCNA` now
  defaults to loading a genuinely 3D, hand-authored `.vwr` world (`worlds3d/world001.vwr`,
  generated via `tools/GenerateSampleWorld3D.cpp`); `GEWorldRuntime::LoadFromMobileEggbertFile`
  stays in the code only as a secondary/reference path (e.g. for later faithful-remake level
  porting), not the default anymore.
- New CPU-side mesh builders and CNA renderer adapters (e.g. `Easy3D::CubeMesh`,
  `Easy3D::CubeMeshRenderer`) live inside `../easy-3d` itself, not as galaxy-eggbert-local
  adapters — decided because that work is generic 3D-batching plumbing with zero Eggbert-specific
  knowledge, matching Easy3D's own stated role. Modifying `../easy-3d` still requires explicit,
  per-change user approval; it is not blanket-authorized.
- **Open design question, not decided yet (2026-07-03):** how should mobile-eggbert's objects/
  elements (all `ObjectType`s) actually be rendered in 3D — billboard, or a textured cube (with
  untextured faces filled by a per-texture fallback color instead of left blank), or a mix depending
  on the object? Whichever objects use the cube approach will need per-block metadata for render
  mode, fallback color, and likely a 4-direction facing/rotation for directional textures. Full
  writeup: `mobile-eggbert-reference/09-open-questions.md` (first bullet) — this is a generalization
  of the already-noted doors-as-billboards question, not a separate decision to make independently.

## 2. Current status

### Build status
- `GalaxyEggbertSimple3D` — **builds clean** (`cmake -S . -B build && cmake --build build -j2`),
  confirmed earlier this session (`S3D-2`'s `tileUV` fix build). **Environment note (2026-07-04):**
  partway through this same session `../simple-3d` was found moved into
  `/rv/data/archive/trash/2026/` — restored back to `../simple-3d` with explicit user approval.
  Once restored, discovered U3D's own prebuilt `cmake-build-debug` directory
  (`/rv/data/library/github.com/u3d-community/U3D/`, a separate pre-existing dependency, not part
  of this repo) is missing entirely — its source is present but never built in this environment.
  Rebuilding all of U3D/Urho3D from scratch is a long operation the user explicitly chose to skip
  for now (`S3D-4`'s Chenille fix below is therefore verified by static review only, not a real
  compile). **Before assuming `GalaxyEggbertSimple3D` currently builds, check whether U3D's
  `cmake-build-debug` has been rebuilt since.**
- `GalaxyEggbertWorldsTests` — **54/54 tests pass** (`ctest --test-dir build`).
- `GalaxyEggbertCNA` — **builds clean** (opt-in: `-DGALAXY_EGGBERT_BUILD_CNA=ON`), **runs and
  renders real, textured terrain with working animated tiles**.
- `../easy-3d` — default (headers-only) build and CNA-linked build both green; 2/2 and 5/5 tests
  respectively (the newest class, `CubeMeshRenderer`, is compile-check-only — see §6).

### What works
- **`GalaxyEggbertSimple3D`**: world loading from mobile-eggbert `.txt`, textured/animated 3D
  terrain, Blupi billboard with state machine, mobile-object billboards, crate push, platform
  patrol, hazard detection, enemy stomp, respawn invincibility, pickups (treasure/keys/shield/
  egg/drink), exit-gate logic, HUD, save/load (3 slots), 93-channel sound, 3rd-person orbit
  camera, 5 sky colors per region. **Changed this migration (commit `4cda53d`, 2026-07-03):**
  `GEWorldRuntime::LoadFromMobileEggbertFile` now spawns 12 `ObjectType`s that were previously
  silently dropped by an incomplete allowlist (jeep=19, secret-exit=21, skateboard=24,
  suction-cup=26, blupih=32, mirror=40, wasp/bee=44, balloon=46, platform-lift=47,
  large-creature=54, dynamite=55, follower=96) — `GEDecorSystem.cpp` now classifies and draws
  real icons for all of them (pickup/enemy/platform categories), verified against real level
  files with `tools/VerifyMoveObjectTypes.cpp`. Most of these still have **no gameplay behavior**
  (they render but don't do anything on pickup/contact) — see the "7 partial-support
  `ObjectType`s" note in `mobile-eggbert-reference/09-open-questions.md`. Also fixed the
  `S3D-4` Chenille (`ObjectType47`) texture bug (see §5) — **build-unverified**, static review
  only (U3D prebuilt dir missing in this environment).
- **`GalaxyEggbertCNA`**: CMake option `GALAXY_EGGBERT_BUILD_CNA` (default OFF) builds it
  alongside/instead of Simple3D; links `CNA` + `easy3d::easy3d`; copies
  `../mobile-eggbert/Content/` + `worlds/` and this repo's own `worlds3d/` next to its binary at
  build time. By default loads `worlds3d/world001.vwr` — a genuinely 3D, hand-authored world
  (`GEWorldRuntime::LoadFromVwrFile`, real Y variation) — into the shared
  `GalaxyEggbert::Worlds::World` (`LoadFromMobileEggbertFile` for mobile-eggbert `.txt` files still
  exists as a secondary/reference path). Also parses (but does not yet render) the `BigDecor:`
  second tile layer, fixed under `E3D-MIG-059` — see §5. Maps block types to `object-m.png` UV
  rects (`GETileAtlas`); walks every Y layer of the loaded world and renders one textured cube
  per non-air cell (`GETerrainRenderer` + `Easy3D::CubeMeshRenderer`), textured with a real CNA
  `Texture2D` of `object-m.png`, animated tiles included (see below). **Block count note
  (2026-07-04): `worlds3d/world001.vwr` was regenerated to 2729 non-air blocks (was 2749) after
  `E3D-MIG-060` fixed an unclimbable-cliff bug in `tools/GenerateSampleWorld3D.cpp`.** The
  vertex/triangle/on-screen-sample numbers below were measured against the older 2749-block
  version and have **not been re-run** against the current 2729-block file — treat them as
  indicative, not current, until re-verified with a real run. Last verified run (pre-regen):
  `2749 blocks, 65976 vertices, 32988 triangles` uploaded, Y range [0, 13]; a 5x5 on-screen pixel
  sample found 21/25 sampled points showing terrain color with 10 distinct colors among them
  (confirms real texture sampling, not a placeholder).
- **`Easy3D::CubeMesh`** (`AppendCubeMesh`/`BuildCubeMesh`): turns `CubeBatch` items into
  vertex/index arrays (24 vertices + 36 indices per cube).
- **`Easy3D::CubeMeshRenderer`**: uploads that data to CNA `VertexBuffer`/`IndexBuffer` once and
  issues `DrawIndexedPrimitives` via a caller-configured `BasicEffect` — the real CNA draw path,
  matching CNA's own `examples/house3d_demo.cpp` pattern.
- **`GalaxyEggbertCNA` animated tiles**: `GETerrainRenderer` splits blocks into a static mesh
  (built once) and an animated subset (lava/crusher/saw/spike/water1/water2/fan×4/marine/temp)
  rebuilt into a second `CubeMeshRenderer` whenever the anim phase changes, driven by
  `GEWorldRuntime::Update(dt)`/`GetAnimPhase()` (6 fps, matches Simple3D/mobile-eggbert) called
  from `GalaxyEggbertCnaGame::Update(GameTime&)`. Verified with mobile-eggbert's `world024.txt`
  (137 animated blocks, loaded via the secondary `.txt` path as a build-artifact-only test, not
  committed): ran 8s / ~48 rebuild cycles with no crash. The default `worlds3d/world001.vwr` has
  0 animated tiles (none of its block types are hazard/animated ones).
- **First hand-authored 3D world**: `worlds3d/world001.vwr`, generated by
  `tools/GenerateSampleWorld3D.cpp` (new CMake tool target, engine-agnostic) — ground floor,
  10-step ascending staircase, raised platform, walled room with doorway, two pillars; 2729
  non-air blocks (as of the `E3D-MIG-060` cliff-bug fix, see §3), Y range [0, 13];
  round-trip-verified via `World::loadFromFile()`.

### What does not work yet
- `GalaxyEggbertCNA`: no Blupi *rendering* (it's an invisible collision point — see §3), no
  object/pickup rendering, no HUD, no sound, no real gameplay. `Easy3D::BillboardBatch`/`DebugDraw`
  have no vertex builder or renderer adapter at all yet (needed for Blupi's sprite and objects).
- `GalaxyEggbertSimple3D`: no per-zone fog; Android/Web builds untested since the last engine
  change. (Camera shake is real and wired up — see §5, corrected 2026-07-05.)

## 3. Recent changes

Most recent first. `galaxy-eggbert` `develop` branch is 165+ commits ahead of `origin/develop` as
of 2026-07-05 — this whole batch (engine work + doc rework + review pass + CLAUDE.md fix + wider
staleness pass, below) is committed locally; whether it has been pushed depends on when you're
reading this (see §9 for the push policy: push only on explicit request, never assume standing
authorization).

**Stale camera-shake bug entry corrected (2026-07-05), found during a wider repo-doc staleness
sweep.** `NEXT.md` itself claimed Simple3D's camera shake was a no-op needing a new `../simple-3d`
API. False: `Camera::Shake(intensity, duration)`/`IsShaking()` already exist in `../simple-3d`
(`Camera.cpp:511`), `GECameraRig::StartShake()` already calls it, and it's wired up at 5 real
death/hazard sites in `GalaxyEggbertSimpleGame.cpp`. No code changed — only this file's own stale
claim, in §2/§5/§8 (see those sections).

**`CLAUDE.md` staleness fix, `DOC-278` (2026-07-05) — COMPLETE.** Fixed the loose end the review
pass below surfaced: `CLAUDE.md`'s "Current Direction Lock" section, build-target selection,
source layout, and Build section all still said `GalaxyEggbertCNA` "does not exist yet" / "has no
build instructions ... because it does not exist." Updated all of them to reflect that
`src/GalaxyEggbertCNA/` is a real, working, opt-in tree (builds via
`-DGALAXY_EGGBERT_BUILD_CNA=ON`, renders real textured/animated terrain) that just isn't at
feature parity with Simple3D yet — no Current Direction Lock rule was loosened or changed.

**Independent review pass over `DOC-100`–`DOC-267`, `DOC-268`–`DOC-276` (6 commits, 2026-07-05) —
COMPLETE.** Six parallel agents independently re-checked all 10 `mobile-eggbert-reference/*.md`
files against `../mobile-eggbert` source, galaxy-eggbert's own code, and the referenced
image/GIF assets on disk — this was the review pass requested 2026-07-04 (§8's former task 1).
Found and fixed 13 real errors across 5 files (00-overview.md, 03-objects.md, 04-enemy-behavior.md,
06-doors.md, 09-open-questions.md came back clean):
- `05-backgrounds.md` (`DOC-268`): `gear.png`'s draw site/rotation and `blupiyoupie.png`'s
  "win-only" framing were both wrong against `Game1.cpp`.
- `07-sounds.md` (`DOC-269`): sound channels 60/61/74 had incomplete or wrong trigger claims
  (channel 61 also fires on dynamite placement, not just persona swap; channel 74's "angel ascent"
  animation claim is false for the lava-death path).
- `01-world-file-format.md` (`DOC-270`): a quoted `world001.txt` excerpt was mislabeled "row 2"
  (it's the first `Decor:` data row).
- `02-tiles.md` (`DOC-271`/`272`/`273`, the largest file): Crusher's real-level usage count was
  copy-pasted from Saw's ("15 files" vs. actual 3); icon 91 was miscategorized as `Water1` when it
  actually belongs to `Water2`'s frame table (frame ranges fixed accordingly), and the file's
  "only base icons are authored" finding was stated as universal when `Water2` inverts it; icon 379
  (`SawStopped`, a real, actively-used `BlockTypes.hpp` constant) was misfiled in the unused/unnamed
  table instead of the named table — moved, crop regenerated, counts corrected 313/128 → 314/127.
- `08-animations.md` (`DOC-274`/`275`/`276`): the "155 animated sequences" summary didn't match its
  own breakdown (131); the §6 completeness sweep had the two fan-related table families swapped
  (`ventillo*` are the real fan tiles, `ventg/d/h/b` are a distinct unimplemented tile); the "6 fps,
  confirmed" tile-tick claim was only checked against galaxy-eggbert's own simplified constant, not
  mobile-eggbert's real per-tile rates, which vary (Saw is actually 4x faster).

Separately flagged (outside `mobile-eggbert-reference/` scope, found independently by 3 of the 6
review agents): `CLAUDE.md`'s "Current Direction Lock" section said `GalaxyEggbertCNA` "does not
exist yet" — stale. Fixed the same day, see `DOC-278` above.

**Documentation rework, `DOC-100`–`DOC-267` (~168 commits, 2026-07-03/04) — COMPLETE.** Full
detail in `plan.md` §16 and summarized in §4 below; not repeated here. Headline outcomes: 129
animated GIFs regenerated after a ghosting bug; 320 sprite crops regenerated after a real
leading-margin grid bug (`S3D-2`); a second real engine bug found and fixed (`S3D-4`, Chenille's
wrong sprite sheet); full 93-channel sound catalog and full background catalog documented; a
final read-through pass fixed stale claims in 5 of 10 reference files.

**Engine/CNA work, commit `4cda53d` (2026-07-03), predates the doc rework above:**
- **`GEBlupiController`** (`E3D-MIG-060`) — invisible, collision-only Blupi movement in
  `GalaxyEggbertCNA`: arrow keys move, Space jumps, grid collision with step-up traversal (climbs
  1 block) and gravity. Engine-agnostic (only depends on `Worlds::World`); verified with
  `tools/VerifyBlupiMovement.cpp` against `worlds3d/world001.vwr` (grounded spawn, staircase climb,
  wall block, gravity fall — all passed). Caught and fixed a real bug in
  `tools/GenerateSampleWorld3D.cpp` (an unclimbable double-stacked cliff); regenerated
  `worlds3d/world001.vwr` to 2729 blocks (was 2749) — **see §2's CNA bullet: the vertex/triangle/
  visibility numbers elsewhere in this file predate that regen and need re-verification.**
- **`BigDecor:` parsing fix** (`E3D-MIG-059`) — both `GEWorldRuntime` implementations
  (Simple3D and CNA) were silently dropping every level file's `BigDecor:` second-tile-layer
  section (row counter bug, shared with the main `Decor:` grid). Now parsed and stored
  (`GetBigDecor()`) but **not rendered anywhere** — how to represent it in 3D is still an open
  question (`09-open-questions.md`). Verified with `tools/VerifyBigDecorParsing.cpp`
  (`world013.txt`: 0→14 cells now parsed; `world001.txt`: 2, unchanged main-grid count 594).
- **MoveObject allowlist expansion (Simple3D)** — `GEWorldRuntime::LoadFromMobileEggbertFile` now
  spawns 12 `ObjectType`s previously silently dropped by an incomplete allowlist (see §2's
  Simple3D bullet for the full list and IDs); `GEDecorSystem.cpp` now classifies/draws real icons
  for each. Verified against real level files with `tools/VerifyMoveObjectTypes.cpp`.
- **Hand-authored 3D world pipeline** (`E3D-MIG-058`) — `tools/GenerateSampleWorld3D.cpp` (new
  engine-agnostic CMake tool) generates `worlds3d/world001.vwr`; `GalaxyEggbertCnaGame` now
  defaults to loading it (`GEWorldRuntime::LoadFromVwrFile`) instead of a mobile-eggbert `.txt`
  file (kept as a secondary/reference path). This directly addresses the user's flag that
  mobile-eggbert's flat 2D levels should not be galaxy-eggbert's real 3D world-data source (not a
  reversal of the "no auto `.txt`→`.vwr` converter" rule in §1 — this world is hand-authored).
  Surfaced and fixed a real gap: `GETerrainRenderer` previously only read Y=0; now walks every Y
  layer.
- **Animated tiles in `GalaxyEggbertCNA`** (`E3D-MIG-055`) — `GETerrainRenderer` splits blocks into
  a static mesh and an animated subset (lava/crusher/saw/spike/water/fan/marine/temp), rebuilt on
  phase change; frame tables ported 1:1 from Simple3D's already-shipped `GETerrainRenderer.cpp`
  (same repo, not a fresh mobile-eggbert transcription). Chunk-radius streaming (`E3D-MIG-057`),
  originally deferred as unneeded, was **re-scheduled** per user override — real hand-authored 3D
  worlds are intended to be denser than the current sample, so this will be needed, not deferred
  until it becomes a problem.

**Earlier, already committed and pushed:** `galaxy-eggbert` `a636649` (render real, textured CNA
terrain from world data) + `../easy-3d` `b4c52c0` (`CubeMesh`/`CubeMeshRenderer`) — these added
`GETerrainRenderer`, `GETileAtlas`, `Easy3D::CubeMesh`/`CubeMeshRenderer`, and CNA texture loading
via `Texture2D` (not mobile-eggbert's `Pixmap`, which is 2D-`SpriteBatch`-coupled). Before that:
`3293a3d` (reject 2D→3D world auto-converter), `0f01490` (mobile-eggbert asset build-time copy +
world-file parsing, `ObjectType`/`SoundChannel` ID parity confirmed), `c6b6456` (`GalaxyEggbertCNA`
skeleton target), `f579824`/`ebea834` (direction-lock docs), `ac1d8d1` (Simple3D crate push +
platform patrol fix). `../cna` shipped an external fix (`e1939bc`, unrelated `StorageDevice`/
`IAsyncResult` mismatch) that briefly broke the `GalaxyEggbertCNA` build — confirmed resolved.

**Next planned activity:** see §8 — the review pass and its `CLAUDE.md` follow-up are both done, so
the natural next step is confirming CNA's terrain stats post-regeneration (§8 task 1), then the
actual 3D-mapping design task (§8 task 2).

## 4. Current blocker / main problem

**No hard blocker right now.** Both active tracks are in a "done, next thing is queued" state:

**Documentation track: no blocker — the `mobile-eggbert-reference/` rework, its requested
independent review pass, and the `CLAUDE.md` staleness fix that review surfaced are all complete
as of 2026-07-05.** The rework (`DOC-100`–`DOC-267`) shipped 2026-07-03/04; the
independently-requested second review pass (`DOC-268`–`DOC-276`, see §3 above) ran 2026-07-05 via
6 parallel review agents and found + fixed 13 real errors across 5 files; `DOC-278` then fixed
`CLAUDE.md`'s own staleness that the review turned up. The reference material can now be treated
as ground truth for the upcoming 3D-mapping design work (§8, task 2) with meaningfully higher
confidence than before the review.

**Engine/code track: no blocker.** Real, textured terrain with working animated tiles renders
end-to-end from the actual loaded world file — `GEWorldRuntime` → `GETerrainRenderer` →
`Easy3D::CubeMesh`/`CubeMeshRenderer` + `Texture2D` → visible, textured pixels on screen. Recent
additions (§3): an invisible collision-only Blupi placeholder, a hand-authored 3D sample world as
the CNA default, a `BigDecor:`-parsing fix (Simple3D + CNA), and a Simple3D `MoveObject` allowlist
expansion (12 previously-dropped `ObjectType`s now spawn). The remaining CNA gaps (Blupi/object
rendering, HUD, sound) are new capabilities to build, not bugs to fix. Known follow-up, not
blocking: no face-culling/occlusion in `GETerrainRenderer` — fine at ~2700 blocks, will matter for
denser/taller hand-authored worlds later (co-requisite with the now-scheduled chunk-radius
streaming, `E3D-MIG-057`).

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| fixed (stale entry, corrected 2026-07-05) | ~~Simple3D: `GECameraRig::StartShake()` is a no-op (Simple3D has no camera-offset API)~~. Wrong: `../simple-3d` has a real `Camera::Shake(intensity, duration)`/`IsShaking()` API (`Camera.cpp:511`, random local-space XY offset applied per-frame), `GECameraRig::StartShake()` calls it directly, and it's wired up at 5 real death/hazard call sites in `GalaxyEggbertSimpleGame.cpp`. No code change was needed — this entry was simply out of date. |
| confirmed, environment-specific | `ctest` does not discover `GalaxyEggbertWorldsTests` when configured in the pre-existing `cmake-build-debug` CLion profile (binary runs fine manually). Not reproduced in a fresh `build/` directory — `ctest --test-dir build` correctly finds and runs all 54 tests there. Likely a stale/IDE-specific config issue in `cmake-build-debug`, not a general CMake problem. |
| incomplete | Simple3D: no per-zone fog, only `SetClearColor` per sky region |
| incomplete | `GalaxyEggbertCNA`: no Blupi/object rendering, no HUD, no sound, no gameplay (expected at this phase, not a bug) |
| unknown | Simple3D Android/Web builds untested since the last engine change |
| unknown | `GalaxyEggbertCNA`'s clean-exit-on-window-close path was not separately exercised — only a forced external `timeout`/kill was tested during verification |
| needs verification | Simple3D: stomp bounce height (`kJumpSpeed * 0.65f`) — does it match mobile-eggbert's feel? |
| needs verification | Simple3D: crate push floor-support check only tested at y=0; stacked crates (y=1) untested |
| risky assumption | `GalaxyEggbertCNA`'s world loader uses a relative path (`"worlds3d/world001.vwr"`, `"Content/icons/object-m.png"`) — only works if the binary is run from its own build directory; fails silently (world) or presumably throws (texture) otherwise |
| incomplete | `GETerrainRenderer` (CNA) has no face-culling/occlusion — draws one full cube per non-air block regardless of neighbors. Fine at the current sample world's size (2729 blocks); will need revisiting for denser/taller hand-authored worlds |
| incomplete | `BigDecor:` (second tile layer) is parsed and stored (`GEWorldRuntime::GetBigDecor()`, both targets) but not rendered anywhere — representation in 3D is an open design question, not a bug |
| incomplete | 7 `ObjectType`s (jeep/secret-exit/skateboard/suction-cup/mirror/balloon/dynamite) now spawn with correct icons in Simple3D but have no real gameplay behavior on pickup/contact — see `mobile-eggbert-reference/09-open-questions.md` |
| needs verification | `GalaxyEggbertCNA`'s reported terrain vertex/triangle counts and on-screen visibility sample (§2) predate the `worlds3d/world001.vwr` 2749→2729 block regeneration and have not been re-run since |
| fixed (2026-07-04) | ~~GIF ghosting bug across `mobile-eggbert-reference/images/`~~. All 129 animated GIFs (`DOC-101`-`DOC-229`) regenerated and verified with the fixed `make-gif.sh` — see §4. |
| fixed, build-unverified (2026-07-04) | `ObjectType47` (Chenille lift)'s `element.png`-vs-`object-m.png` texture bug in `GalaxyEggbertSimple3D` — see §3's top entry and `plan.md`'s `S3D-4`. Fixed by static review; U3D's prebuilt `cmake-build-debug` is missing in this environment so the fix could not be confirmed by an actual compile yet. |
| fixed (2026-07-04) | ~~`BlockTypes::tileUV()` assumed a flat 64px grid in `object-m.png`, missing the sheet's real 1px inter-tile gap (65px pitch) — bled neighboring icons in by later rows/columns~~. Fixed in both `GalaxyEggbertSimple3D` and `GalaxyEggbertCNA` — see §3's top entry and `plan.md`'s `S3D-2`. |

## 6. Architecture notes

### Main modules
```
include/GalaxyEggbert/Worlds/, src/GalaxyEggbert/Worlds/   — engine-agnostic voxel World (100×100
                                                               grid), Block/Chunk, .vwr save format.
                                                               Shared by BOTH targets below
                                                               (GE_SHARED_SOURCES in CMakeLists.txt).
include/GalaxyEggbert/BlockTypes.hpp                        — tile type constants; block type =
                                                               icon index in object-m.png;
                                                               fromMobileIconId() is the single
                                                               shared mobile-eggbert-icon→block
                                                               translation used by BOTH targets.

src/GalaxyEggbertSimple3D/   — full playable game (Simple3D/U3D). GalaxyEggbertSimpleGame owns
                                GEWorldRuntime, GETerrainRenderer, GEBlupiController,
                                GEDecorSystem, GEHud, GECameraRig, GESound, GEExploSystem,
                                GEBridgeSystem. Not touched by the CNA migration.

src/GalaxyEggbertCNA/        — GalaxyEggbertCnaGame (Microsoft::Xna::Framework::Game subclass)
                                owns Game/GEWorldRuntime (LoadFromVwrFile() default,
                                LoadFromMobileEggbertFile() secondary/reference path),
                                Game/GETileAtlas (block type → UV rect), Game/GETerrainRenderer
                                (World, all Y layers → static + animated CubeMeshRenderer-backed
                                meshes), an Easy3D::Camera3D (aimed at the terrain's 3D block
                                centroid), a CNA Texture2D of object-m.png, and a BasicEffect
                                bound to it.

tools/GenerateSampleWorld3D.cpp — engine-agnostic CLI tool (CMake target
                                GenerateSampleWorld3D): builds a hand-authored 3D World in memory
                                and saves it via World::saveToFile(). Only depends on
                                include/GalaxyEggbert/Worlds/ — no engine deps, runs standalone.

worlds3d/                    — galaxy-eggbert's own hand-authored .vwr worlds (committed to this
                                repo, unlike mobile-eggbert's worlds/ which is copied at build
                                time). world001.vwr is the first one (E3D-MIG-058).

../easy-3d/                  — companion library beside CNA (not touched by default; two
                                approved edits this migration). Camera3D/OrbitCamera/FollowCamera,
                                TextureAtlas, BillboardBatch/CubeBatch/DebugDraw (CPU-side item
                                queues), CubeMesh (vertex/index builder), CubeMeshRenderer (CNA
                                draw-call adapter). Billboard/DebugDraw builders+adapters: not
                                started.
```

### Data flow
```
Hand-authored 3D worlds (worlds3d/*.vwr, engine-agnostic binary format):
  CNA (default): → GEWorldRuntime::LoadFromVwrFile() → World::loadFromFile() → World (all Y
                   layers) → GETerrainRenderer → Easy3D::CubeBatch/CubeMesh/CubeMeshRenderer +
                   Texture2D(object-m.png) (renders real 3D, textured, animated terrain; no
                   Blupi/object rendering yet).

Mobile-eggbert worlds (worlds/worldXXX.txt, header + Decor: grid [+ MoveObject: lines], flat Y=0):
  Simple3D:      → GEWorldRuntime::LoadFromMobileEggbertFile() → World + MobileObjSpec list +
                   blupiSpawn → GETerrainRenderer / GEDecorSystem / GEBlupiController (renders
                   everything; this is Simple3D's only/default world source).
  CNA (secondary/reference path only, not the default anymore):
                 → GEWorldRuntime::LoadFromMobileEggbertFile() → World (MoveObject: lines still
                   ignored — that's a later phase) → same GETerrainRenderer path as above.
```

### Important invariants
- Block type = icon index in `object-m.png` (20 cols, 64×64 px tiles) — no separate mapping table.
- World grid is 100×100; both `GEWorldRuntime` implementations independently define a
  `kWCX`/`kWCZ` (Simple3D) or `kWorldCenterX`/`kWorldCenterZ` (CNA) = 50 offset to center the grid.
- `ObjectType` (204 IDs) and `SoundChannel` (93 IDs) in `include/GalaxyEggbert/def/` are confirmed
  numerically identical to mobile-eggbert's versions — do not renumber.
- No `#ifdef GE_ENGINE_*` anywhere — engine differences belong inside Simple3D (for that target)
  or are simply separate code in `src/GalaxyEggbertCNA/` (for the new target).
- `CubeMesh`'s frame index / UV assignment convention (`AddGrid`'s row-major numbering ==
  `BlockTypes`' `icon = row*kSheetCols+col`) is load-bearing for `GETileAtlas` — do not change one
  side without the other.
- Build with `-j2` maximum (32 GB RAM constraint; crashes observed with more parallel jobs).

### Boundaries that must remain stable
- `BlockTypes::fromMobileIconId()` — must match the mobile-eggbert world file format exactly.
- `ObjectType`/`SoundChannel` numeric values — stored in level files, must never be renumbered.
- `GEDecorSystem::GetObjIcon(ObjectType, phase)` (Simple3D) — do not change without
  cross-referencing mobile-eggbert's `Decor.cpp`.
- `src/GalaxyEggbertSimple3D/` must not be mutated into the CNA implementation — new CNA/Easy3D
  code goes in `src/GalaxyEggbertCNA/` only.
- mobile-eggbert stays read-only; no code/data copied from it without explicit user approval.
- `../easy-3d`, `../cna`, and `../simple-3d` are sibling repos: read freely when needed, modify
  only with explicit, per-change user approval.
- Easy3D must not hide CNA (its APIs use CNA/XNA types directly) and must not grow into a scene
  graph / ECS / engine — new helpers stay small and generic.

## 7. Useful commands

```bash
# Configure + build Simple3D (default target):
cmake -S . -B build
cmake --build build --target GalaxyEggbertSimple3D -j2
./build/GalaxyEggbertSimple3D

# Build + run world-model unit tests:
cmake --build build --target GalaxyEggbertWorldsTests -j2
ctest --test-dir build --output-on-failure

# Configure + build the new CNA target (opt-in, off by default):
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2

# Run CNA target — must run from its own build directory (relative asset paths):
cd build-cna && ./GalaxyEggbertCNA
# Expect on stdout, in order. Vertex/triangle/visibility numbers below are NOT yet re-verified
# against the current 2729-block worlds3d/world001.vwr (last real run was pre-regeneration,
# against the 2749-block version — see §2/§5, and update this comment once re-run):
#   "GalaxyEggbertCNA: loaded worlds3d/world001.vwr — 2729 non-air blocks, Y range [0, 13]."
#   GETileAtlas UV diagnostics (Ground/Lava/Wall)
#   "GalaxyEggbertCNA: terrain texture loaded — 1301x1431 px."
#   "GalaxyEggbertCNA: terrain mesh uploaded — N blocks (0 animated), ... vertices, ... triangles."
#   (this sample world has no animated tiles; run GalaxyEggbertCNA on a mobile-eggbert-derived
#   .txt world via GEWorldRuntime::LoadFromMobileEggbertFile for a non-zero animated count, e.g.
#   mobile-eggbert's world024.txt has 137)
#   "GalaxyEggbertCNA: terrain visibility check — N/25 sampled screen points show non-background (terrain) color, M distinct color(s) among them..."
# A window opens showing textured cube terrain (object-m.png tiles) from an angled overhead view.

# Regenerate the hand-authored 3D sample world (if tools/GenerateSampleWorld3D.cpp changes):
cmake --build build-cna --target GenerateSampleWorld3D -j2
./build-cna/GenerateSampleWorld3D worlds3d/world001.vwr

# Scripted verification tools (engine-agnostic, no CNA link needed; build under either target):
cmake --build build-cna --target VerifyBlupiMovement VerifyBigDecorParsing VerifyMoveObjectTypes -j2
./build-cna/VerifyBlupiMovement      # Blupi collision/step-up/gravity against worlds3d/world001.vwr
./build-cna/VerifyBigDecorParsing    # BigDecor: section parsing against real mobile-eggbert worlds
./build-cna/VerifyMoveObjectTypes    # the 12 previously-dropped ObjectTypes now spawn correctly

# easy-3d: default (headers-only) build + tests
cmake -S ../easy-3d -B /tmp/e3d-build -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build -j2
ctest --test-dir /tmp/e3d-build --output-on-failure

# easy-3d: CNA-linked build + tests (heavier — builds CNA/SHARP_RUNTIME/EasyGL too)
cmake -S ../easy-3d -B /tmp/e3d-build-cna -DEASY3D_LINK_CNA=ON -DEASY3D_CNA_BACKEND=EASY_GL -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build-cna -j2
ctest --test-dir /tmp/e3d-build-cna --output-on-failure

# Reference: mobile-eggbert animation tables / gameplay logic (read-only)
grep -n "table_decor\|table_blupi" ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp
less ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp
```

No `.clang-format`/`.clang-tidy` config exists in this repo — no lint/format tooling to run.

## 8. Next smallest tasks

1. **Confirm `GalaxyEggbertCNA`'s reported terrain stats after the `worlds3d/world001.vwr`
   2749→2729 block regeneration** (see §5) — run it, capture the real vertex/triangle counts and
   visibility-sample result, update this file. **Files:** none (verification only).
   **Verification:** `cd build-cna && ./GalaxyEggbertCNA`, compare stdout against §7's expected
   output.
2. **The actual 3D-mapping design task** (billboard vs. textured-cube for objects, `BigDecor`
   layer treatment, door rendering, etc. — see `09-open-questions.md`) — the reference material's
   independent review pass (§3, `DOC-268`-`276`) is now done, so this is unblocked; not yet scoped
   as its own `DOC-*`/`E3D-MIG-*` id.
3. **Chunk-radius world streaming (E3D-MIG-057, now scheduled)** — implement loading/rendering
   only the current + neighboring chunks, once real (denser, more 3D) hand-authored worlds exist.
   Natural co-requisite with face-culling below.
4. **Expand `worlds3d/world001.vwr`, or author more `.vwr` worlds** — the current sample is a
   proof-of-concept (staircase + one room). A natural next step is a more level-like, denser,
   genuinely 3D design (multiple rooms/levels, hazard tiles at various Y) — this should follow the
   mapping-design decisions (task 2), not precede them.
5. **Add face-culling/occlusion to `GETerrainRenderer`** — needed once worlds get denser (see
   task 3/4); not needed at the current ~2700-block scale.
6. **Render Blupi as a billboard (E3D-MIG-061..063)** — CPU-side vertex builder + CNA renderer
   adapter for `Easy3D::BillboardBatch`, then draw `blupi.png` at `GEBlupiController`'s position.
7. **Fix `ctest` discovery in the `cmake-build-debug` profile** — investigate why
   `gtest_discover_tests` doesn't find `GalaxyEggbertWorldsTests` there (works fine in a fresh
   `build/` dir). **Files:** `CMakeLists.txt`, `cmake-build-debug/` config.
   **Verification:** `ctest --test-dir cmake-build-debug -R GalaxyEggbert` reports 54 passed.
8. **Verify `GalaxyEggbertCNA`'s clean-exit path** — close the window via the window manager
   (not a forced kill) and confirm the process exits 0 with no leaked resources.
   **Files:** none expected — diagnostic verification only, possibly add an `OnExiting` log line
   to `GalaxyEggbertCnaGame` if useful. **Verification:** manual run + exit code check.
9. ~~Simple3D camera shake~~ — **already done**, this task was based on a stale bug entry (see
   §5). `GECameraRig::StartShake()` already calls a real `../simple-3d` `Camera::Shake()` API and
   is wired up at 5 death/hazard call sites. If it still doesn't look right in-game, the next step
   would be tuning intensity/duration to match mobile-eggbert's `DecorAction::SmallShake` feel, not
   implementing it from scratch.

## 9. Do not do yet

- No further investment in Simple3D/U3D/Nova3D beyond bug fixes on the existing reference target
  — that direction is superseded.
- No modifications to `../mobile-eggbert`, `../cna`, `../easy-3d`, or `../simple-3d` without
  explicit user approval for that specific change — none of the approvals granted so far are
  blanket authorization for further edits.
- No `.txt → .vwr` (or any) automated 2D-to-3D world converter — rejected, see §1.
- No Easy3D scope creep — no ECS, scene graph, physics, resource cache, editor, or Lua.
- No mass refactor of `include/GalaxyEggbert/Worlds/` unless a failing unit test justifies it.
- No `#ifdef GE_ENGINE_*` anywhere.
- No new gameplay mechanics not present in mobile-eggbert (no coins, coyote time, combo
  multipliers, star ratings, time bonuses).
- Committing after each finished task is now standing user instruction (2026-07-04, see §3) — one
  commit per `DOC-1xx`/task, not batched. **Pushing** to `origin/develop` is still NOT standing
  authorization — only push on explicit request each time.
- (Resolved 2026-07-05: the review pass ran, see §3/§4 — this no longer blocks task 3.)

## 10. Resume prompt

```
Read NEXT.md first. Then inspect only the files needed for the first task in section 8.
Do not refactor unrelated code and do not expand scope beyond that one task.
Make one small, verified improvement — implement the task goal as described, nothing more.
Cross-reference mobile-eggbert source at ../mobile-eggbert (read-only) before implementing any
gameplay or world-format behavior. If the task needs a design decision (e.g. how to handle
per-frame tile animation), make the smallest reasonable choice and record it, rather than
blocking on it.
After the change, run the verification command listed for that task.
Update NEXT.md when done: move the completed task into section 3 (Recent changes), remove it
from section 8, and add whatever new next-smallest task naturally follows.
```
