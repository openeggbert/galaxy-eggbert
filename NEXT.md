# NEXT.md — Galaxy Eggbert

_Last updated: 2026-07-19 (see §7.5 for standing directives — note the "no editor work" directive
there is **superseded**, see §7.5's own header. The in-game 3D world editor: milestones EDITOR-100
through EDITOR-110 are implemented, verified, and pushed; EDITOR-111/112 remain, see §8 — but
**editor work is now PAUSED, by explicit 2026-07-19 user request** (see plan.md §6's own status
note for the full write-up, including an unresolved keyboard-input concern to check first if this
is picked back up). The user wants to work on other areas of `galaxy-eggbert` next — do not resume
EDITOR-111/112 without the user explicitly asking to.)._

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

**In-game 3D world editor (paused 2026-07-19, see plan.md §6):** `GamePhase::Editor`,
`src/GalaxyEggbertCNA/Editor/`, a user-requested feature letting each player create, edit, save
and play-test their own `.vwr` worlds — inspired by free-eggbert's "Own mission" editor. The user
approved a 13-milestone plan (EDITOR-100..112); **100-110 are done and pushed**, 111/112 remain
but development is paused here by explicit user request while other areas of the project get
attention instead — see plan.md §6 for the full current-state write-up (what's built, what
remains, and a real open concern about keyboard input possibly being a window-focus issue, not
yet confirmed resolved).
Note this editor is an explicit, user-approved **exception** to the faithful-remake rule (see
`plan.md` §6): it is content-creation tooling, not a mobile-eggbert feature, so inventing editor
UX is fine — inventing new *gameplay* mechanics is still not.

**Important architectural decisions:**
- **Direct CNA + Easy3D is the locked, sole long-term target** (decided 2026-07-05). The older
  `GalaxyEggbertSimple3D` (built on `simple-3d` → U3D/Urho3D) is **historical reference only as of
  2026-07-08** — do not build, fix, or troubleshoot it. Its code stays in the tree but is not
  maintained.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching) — it must not
  hide CNA; game code calls CNA directly.
- `../mobile-eggbert` is **read-only, never modified, even temporarily**. Assets (PNGs, sounds,
  world files) are freely reused by direct path; code/data (tables, enum values, byte layouts) are
  reference-only and require explicit user approval to transcribe.
- `../simple-3d` may be read for reference only; never modified.
- No `#ifdef` guards for engine differences — each target speaks its own API directly.
- Worlds are hand-authored via `tools/GenerateSampleWorld3D.cpp` (writes all 79 `worlds3d/*.vwr`
  files, including `world999.vwr`, this engine's own quarantined mechanics-showcase/test world);
  there is no automatic 2D→3D world converter and none is planned.
- `GESaveData`/`GEInputPad`/`GEInteractionSystem` are deliberately **not** byte-compatible with
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
  engine bug: `BasicEffect` draws with `Alpha < 1` don't render at all under this backend (see §5),
  and had a second real bug (missing NDC Y-flip in `SkinnedEffect` shaders, making the third-person
  placeholder model float) found and fixed **in `../cna` itself** 2026-07-18 — that fix currently
  sits uncommitted there, needs the user's own call on committing it (a different git repository).
- `GalaxyEggbertSimple3D` is **not built** (per direction lock above) — its build is known broken
  in this environment (missing/incompatible U3D prebuilt) and this is intentionally left unfixed.

### Web build (Emscripten/WebAssembly, 2026-07-17)
`GalaxyEggbertCNA` also builds and runs in-browser via Emscripten — see `plan.md` `BUILD-003` for
the full writeup. Configure/build:
```bash
source <path-to-emsdk>/emsdk_env.sh
cmake -S . -B build-web \
  -DCMAKE_TOOLCHAIN_FILE=<path-to-emsdk>/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake \
  -DCMAKE_BUILD_TYPE=Release -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF -DGALAXY_EGGBERT_BUILD_CNA=ON
cmake --build build-web --target GalaxyEggbertCNA -j2
```
Produces `build-web/GalaxyEggbertCNA.{html,js,wasm,data}` — serve that directory over plain HTTP
(not `file://`, browsers block `.wasm`/`.data` fetches from local files) and open the `.html`.
`CNA_ENABLE_NET` must stay `ON` (the default) even for the web build — `GalaxyEggbertCnaGame.cpp`
unconditionally references `AvatarRenderer` symbols from `CNA_GamerServices`, which is gated behind
it. Verified working via a real headless-Chrome/WebGL2 run (see `BUILD-003`); not yet wired into
CI or a hosting pipeline (`BUILD-009`/`BUILD-010`, still open) — publishing the built files is a
manual step for now.

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
Last full run (2026-07-19, `build-cna` only, after EDITOR-110):
- `build-cna`: **79 tests, 78 pass (99%)** — the only failure is `easy-gl-resource-smoke-tests`, a
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
  and `VerifyGEWorldEditor` (new — the whole editor's pure logic: camera math, voxel raycast, box
  region math, undo/redo, palette data + click hit-testing, custom-world storage, browser screen,
  object placement).
- `GalaxyEggbertSimple3D`, `VerifyBigDecorParsing`, `VerifyMoveObjectTypes` — Simple3D-only targets,
  not built per the direction lock.

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
  `GEObjectIcons` says the type actually lives on) on a solid green backing — see §3.
- Objects mode places real `MoveObjectRecord`s (enemies, pickups, lifts) that appear immediately as
  billboards — no save/reload round-trip.
- `G` selects whichever already-placed `MoveObjectRecord`'s billboard projects nearest screen center
  (reuses `GEHud::ProjectWorldToHudSpace()`, no new projection code), shown with a magenta highlight
  cube. `T` sets its `posEnd` to the current aim point, giving it a real patrol path; `Tab` cycles
  which of its 5 fields (speed + the 4 patrol-timing fields) `OemPlus`/`OemMinus` nudge; `Delete`
  removes it. All keyboard-only (no new toolbar button), matching the box-fill tool's own
  keyboard-only precedent; each edit is a real `MoveObjectEdit` undo command (`U`/`R` undo/redo them
  like any other edit).
- Save (`Enter` or toolbar) and a full Play-Test loop: saves, launches a real gameplay session
  against the custom world, and returns to the editor on win/loss/pause-back without touching real
  save progress.

**Two real bugs found and fixed via this work** (both pre-existing, neither editor-specific):
- `GETerrainRenderer` built and drew an empty static mesh for an all-air world, throwing
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
north-hill plateau reachable only via a staircase + terraced ascent.

### What does not work yet
- **No visible 3D Blupi model** — invisible collision point in first-person; a placeholder model
  in third-person. Blocked on the user providing a real model/rig.
- **Saw blade (icon 378) render orientation is wrong** — 3 prior fix attempts were wrong; needs the
  user's own visual judgment (a screenshot review), not another guess.
- **World editor: sky-region picking (EDITOR-111) and the hardening pass (EDITOR-112, incl. an
  unsaved-changes guard) are still open** — see §5. Object editing (EDITOR-110: select, patrol path,
  speed/timing, remove) is now done.
- **The editor has no text rendering** — toolbar buttons are distinguished by position and color
  only (object palette cells now draw real per-type sprites, not flat colors, since the 2026-07-19
  redesign — see §3). A documented, deliberate simplification, not an oversight (see §5).
- Several real HUD buttons render but are intentionally inert (no desktop equivalent exists yet):
  `SetupJump`/`SetupZoom`/`SetupAccel`, `PauseBack`, `InitRanking`/`InitBuy`.
- Idle "fidget" periodic sounds — blocked on `AnimState` values this engine doesn't have (same
  prerequisite as the 3D Blupi model).
- `GalaxyEggbertSimple3D` — historical reference only, not built/maintained.

## 3. Recent changes

Most recent first. Full history: `git log`.

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
  mount/dismount hook points in `GalaxyEggbertCnaGame.cpp` (confirmed the only vehicle with a
  dedicated mount/dismount pose).
- **FireTank** (53) — new `GEInteractionSystem::TankFiredThisFrame()` per-frame signal (mirrors
  `CrateBeingPushedThisFrame()`, since that class has no `GEBlupiController` access), true only the
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
animated" (the interim debug indicator, `GEHud.hpp`'s own class comment, not part of the real
mobile-eggbert HUD). Cross-checked against `mobile-eggbert-reference/08-animations.md` §2's own
"Channel selection" note (verified there directly against `Decor.cpp`'s real `BlupiSearchIcon()`):
only `Clear1`/`Clear2`/`Clear3`/`Glu`/`Electro` use `element.png`, every other real `BlupiAction`
uses `blupi.png`. `GEHud`'s indicator always sampled `blupi.png` regardless — since `Clear1`/
`Clear2`/`Clear3`/`Glu` are 4 of this engine's real, already-wired `DeathLocked` causes, all 4 were
showing whatever unrelated `blupi.png` pixels happened to sit at that numeric icon index (`Electro`
has no modeled mechanic here, so it never came up). Fixed with new
`GEBlupiController::AnimIconUsesElementSheet()` (true only for those 4 causes) threaded through a
new `GEHud::Draw()` parameter, selecting the already-loaded `element.png` batch instead of
`blupi.png` for just this one HUD element when it applies — `GEHud` already draws other element.png
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
`GEEditorPalette` was rewritten from left-toolbar-plus-right-8x4-grid to a single left column:
- 8 fixed action buttons, paired 2-per-row (`ToolbarButtonRect(index)`: `index/2` = row, `index%2`
  = left/right half) — Undo/Redo, Save/Back, Play-Test/mode-toggle, BoxFill/Confirmed-All-tab —
  then a paging row, then content cells one per row. Pairing was necessary, not cosmetic: 9
  full-width rows (the original one-row-per-action draft) need 514px of vertical space alone, more
  than this engine's actual default 800x480 window has room for at all; 5 paired rows leaves real
  space for content.
- `PageCount()`/a new `ItemsPerPage()` now take the real `viewportHeight` and compute how many
  content rows fit below the fixed header, replacing the old compile-time `kIconsPerPage=32`
  constant — a single column has much less room per page than an 8x4 grid did.
- New `ToolbarAction::BoxFill`, wired into `GEWorldEditor::Update()`'s existing box-fill state
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
the real per-type sprite via `GEObjectIcons::GetObjIcon()` plus its atlas-selection predicates
(`IsUniformCubeObject`/`IsObjectMPngSourced`/`IsExploPngSourced`/`IsBlupiPngSourcedAtPhase`/
`UsesBlupi1Texture`) — the SAME lookup already used to render real `MoveObject` billboards during
gameplay, so no new icon data or mapping was invented. Icons are batched per atlas (object-m.png/
element.png/explo.png/blupi.png/blupi1.png) so a page still costs at most 5 draw calls, same shape
as the pre-existing single-atlas Blocks-mode batch. `GEEditorPalette::Draw()` now takes 4 more
`Texture2D&` parameters (the game's own already-loaded `objectTexture_`/`exploTexture_`/
`blupiObjectTexture_`/`blupi1ObjectTexture_`, no new asset loads) threaded through
`GEWorldEditor::Draw()` from `GalaxyEggbertCnaGame`; `CMakeLists.txt`'s `VerifyGEWorldEditor` target
gained `GEObjectIcons.cpp` (a link-time dependency, engine-agnostic, same precedent as
`GenerateSampleWorld3D` already linking it). `GEEditorPalette::CategoryColor()` and its
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
  picking reuses `GEHud::ProjectWorldToHudSpace()` (no new projection code was written), so this
  section links `src/GalaxyEggbertCNA/Game/GEHud.cpp` into `VerifyGEWorldEditor` for the first time
  (`CMakeLists.txt`). Shown with a new magenta selection-highlight cube — `GEEditorHighlightRenderer`
  gained a second public method, `ShowSelectedObject()`, and `GEWorldEditor` owns a *second* instance
  of that class so the selection highlight can stay visible alongside the existing aim-crosshair/
  box-fill one (each instance only tracks one set of bounds).
- `T` sets the selected object's `posEnd` to the current raycast aim cell, giving it a real patrol
  path (`posEnd != posStart`).
- `Tab` cycles which of the object's 5 numeric fields (`speed`, then the 4 real patrol-timing fields)
  `OemPlus`/`OemMinus` nudge by a fixed editor-UX step (not a transcribed real constant).
- `Delete` removes the selected object entirely.

Each action is a real `GEEditCommand::Kind::MoveObjectEdit` pushed onto the existing undo stack
(`U`/`R` already undo/redo these correctly — no undo/redo code changes were needed, confirming the
EDITOR-109-era design note that anticipated this). New `GEWorldEditor::RefreshSelectedObjectAfterHistoryChange()`
re-syncs the cached `selectedObject_` from the world after every Undo/Redo, so a later edit's "before"
state can't go stale relative to what Undo/Redo just wrote (a real bug caught and fixed during this
session's own testing, before it ever reached the live build — see below).

**A real test bug found and fixed while writing `VerifyGEWorldEditor`'s new section**: the first
draft pressed the same edge-triggered key (`U`) four times in a row across separate `Update()` calls
expecting each to undo one more step — but `GEWorldEditor::Update()`'s edge-triggering (matching
every other tool key in this class) only fires on a **press following a release**, so only the FIRST
of the four consecutive presses actually did anything; the other three were silent no-ops. Fixed by
adding a `pressKey()` test helper that presses then releases within two `Update()` calls, same
requirement a real keyboard has. Not a bug in `GEWorldEditor` itself — an easy trap to fall into
when scripting multiple presses of the same key in a test, worth remembering for any future section
that does this.

24 new `VerifyGEWorldEditor` assertions (189 total): place → G-select → T (patrol path) → OemPlus/
OemMinus (speed, then Tab + stepAdvanceTicks) → 4×undo → 4×redo → Delete → undo-the-delete, all
checked against `CollectMoveObjects`, plus a no-op sanity check (G with no objects anywhere in the
world selects nothing, and a Delete right after is a safe no-op).

Live-verified end-to-end under `xvfb-run` with temporary env-var-gated (`GE_EDITOR_AUTOTEST_110`)
instrumentation (fully reverted before commit, confirmed via `git diff`): a scripted place → G-select
→ fly → T → Play-Test sequence, driven through synthetic `KeyboardState`/`MouseState` fed into the
SAME real `GEWorldEditor::Update()` call site real player input uses. Confirmed the play-tested
world's `MoveObjectRecord` carried the real edited patrol path (`posStart=(50,4,33)
posEnd=(50,3,17)`), and — since the two screenshots taken during play-test looked visually
near-identical at that camera angle/distance — added a temporary numeric trace of the live
`MobileObjSpec::current{X,Y,Z}` during `Play` phase to settle it conclusively: `currentZ` advanced
continuously frame-by-frame from render-space -17 toward -33 (raw grid 33→17), proving the lift
genuinely patrols at runtime; the screenshots just weren't a legible way to see a mostly-depthwise
motion at that angle. No code bugs found in the live pass — instrumentation fully reverted afterward.

### In-game 3D world editor, EDITOR-100..109 (2026-07-18, 10 commits `a9140c2`..`a6ba199`)

New tree `src/GalaxyEggbertCNA/Editor/`:
- `GEWorldEditor.hpp/.cpp` — owns camera/tool state/undo stack, drives everything else.
- `GEVoxelRaycast.hpp/.cpp` — Amanatides–Woo DDA against `Worlds::World`.
- `GEBoxRegion.hpp/.cpp` — corner pair → normalized, world-clamped box.
- `GEEditCommandStack.hpp/.cpp` — tagged `GEEditCommand` undo/redo stack.
- `GEPaletteCategories.hpp/.cpp` — curated + exhaustive numeric palette data (blocks and objects).
- `GEEditorPalette.hpp/.cpp` — toolbar + paged icon grid, Blocks/Objects modes.
- `GEEditorHighlightRenderer.hpp/.cpp` — cell/box selection overlay.
- `GEEditorBrowserScreen.hpp/.cpp` — per-gamer world list/create/open/delete UI.
- `GECustomWorldStorage.hpp/.cpp` — `customworlds/gamer<N>/custom_NNN.vwr` path helpers.

Also added: `src/GalaxyEggbertCNA/Game/GEQuadBatch.hpp/.cpp`, extracted from `GEInputPad` so the
editor UI reuses the exact same 2D quad primitives (`GEInputPad`'s public API unchanged;
`VerifyGEInputPad` passes unmodified as the regression guard), and `tools/VerifyGEWorldEditor.cpp`
(165 assertions).

Modified: `GamePhase.hpp` (new `Editor` value); `Worlds/World.hpp/.cpp`
(`removeBlockExtraMetadata`); `MoveObjectRecord.hpp/.cpp` (`RemoveMoveObject`);
`GEWorldRuntime.hpp/.cpp` (new public `ResyncFromWorld()`, refactored out of `LoadFromVwrFile()`'s
tail); `GEInputPad.hpp/.cpp` (Init-screen Editor button + `GEQuadBatch` extraction);
`GalaxyEggbertCnaGame.hpp/.cpp` (phase dispatch, `worldEditor_`, `LoadCustomWorldForEditing`/
`ForPlayTest`, play-test routing, `ResyncFromWorld()` call in `RebuildWorldPresentation()`);
`CMakeLists.txt`; `tests/GalaxyEggbert/MoveObjectRecordTests.cpp` (+2 gtest cases, 66 total).

Bugs fixed: the empty-world `GETerrainRenderer` crash and the palette press/release click bug —
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
  both `GEHud`'s `kPanelOpacity` and `GEInputPad::DrawInit()`'s equivalent had been forced to 1.0
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
  failure).
- **fix: placeholder Fox model floating + wrong scale, real zero-gravity Balloon freeze (plan.md
  `E3D-MIG-069`/`135`).** Three user-reported bugs. (1) The third-person placeholder Fox model
  visibly hovered above the terrain. Root cause independently confirmed via 3 cross-checked
  sources (`GETerrainRenderer.cpp`'s block-center formula, `easy-3d/CubeMesh.cpp`'s half-extent
  formula, and the existing first-person `kEyeHeight` usage): `GEBlupiController::GetY()` sits a
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
  (`Decor.cpp:9518-9601`) as a new proximity scan in `GalaxyEggbertCnaGame.cpp` (a bounding-box
  check, NOT contact/collision, against 11 real enemy ObjectTypes already placed in this engine),
  gated on Blupi being idle and a new `GEBlupiController::TriggerMockery()`/15s-cooldown mechanism
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
  `GEBlupiController::TriggerBye()`/`IsBye()` freezes Blupi the same way `TriggerTeleport()` does; a
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
  (`GESaveData::kSavePath` now `#if defined(__EMSCRIPTEN__)`, paired with a `--pre-js`). Only
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
  goes to a real final bonus world (199), not `mission/10*10` — new `GEWorldRuntime::
  ComputeWinExitTarget()` fixes a latent bug where `world001.vwr`'s own pre-existing exit would
  have computed an invalid mission 0. Confirmed (by tracing `Decor::IsWorld()` directly) that real
  global-hub world-select markers are NEVER access-gated — the locked/unlocked icon swap is purely
  cosmetic — so no gating was needed there; but confirmed real per-world sublevel access IS
  gated by real physical door tiles (`Decor::AdaptDoors()`'s `m_mission%10==0` branch,
  `SearchDoor()`/icon 182, cross-checked against `worlds/world010.txt`'s own real sign+door
  layout), opened once the preceding sublevel is won (`Decor::OpenDoorsWin()`). Ported as new
  `BlockTypes::ProgressDoor2..8` + `GESaveData::IsMissionDoorUnlocked()`/`UnlockMissionDoor()`
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
  exit/PauseBack/PauseRestart) ported faithfully via new `GEWorldRuntime::
  ComputeWorldSelectTarget()`/`ComputeMissionBack()` (pure, verified directly against
  `Decor.cpp`'s real mission-handler formulas and `Game1::MissionBack()`) and
  `GalaxyEggbertCnaGame::LoadMission(int)` (loads `worlds3d/world{N:03d}.vwr`, rebuilds terrain/
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
  `Game1.cpp:357-358`). Resume `CONTINUE` now actually reads back the mission number `GESaveData`
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
  Skateboard has no motor sound at all. New `GEBlupiController::HasVehicleMotor()`/
  `IsVehicleMotorHigh()` (the real per-mode "moving vs idle" pitch-select flag, translated as
  nonzero horizontal speed for ground vehicles / nonzero vertical velocity for flight modes) +
  `GalaxyEggbertCnaGame::UpdateVehicleMotorSound()`, a direct port of the real crossfade state
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
  engine's `GESound::Play()` has no positional audio (just a channel), a one-time lazy world scan
  for "does any qualifying Blitz/BlitzEmitter pair exist anywhere" (new
  `GEInteractionSystem::HasBlitzEmitterPair()`, same "-1 = not yet scanned" idiom as the existing
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
  `GEInteractionSystem::SpawnWaterSplash()` spawn on the dry->Surf/dry->Nage transition
  (single-instance-gated via new `HasActiveObjectOfType()`). Added the jump-exit-specific
  "Tiplouf" splash (ch64, `ObjectType35`) — this engine approximates "deliberate jump exit" as
  `jumpPressed` at the exact exit frame (documented simplification, single-point collision has no
  equivalent to the real 3-way exit-cause distinction). Added the ambient rising bubble while
  swimming (ch24, `ObjectType15`, new `GEInteractionSystem::SpawnWaterBubble()` — scans the real
  water column above Blupi via `BlockTypes::isWater()`, spawns a bubble rising exactly that many
  tiles, self-deleting on arrival via the existing `AdvancePatrolStep()` ObjectType23-arrival
  branch, matching real source's identical treatment of both types there), triggered twice per
  ~3.5s cycle off `GEWorldRuntime::GetAnimPhase() % 70`. Also fixed 3 more `GEObjectIcons.cpp`
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
  correctly implemented (`GETerrainRenderer.cpp:298`'s `kAnimWater1`, TILE-018). **Icon 440**:
  `Decor::OpenDoorsTresor()` (`Decor.cpp:11636-11658`) confirms treasure-gated doors are
  `421 + (N-1)` for N treasures required, so 440 = a legitimate 20-treasure-door id — but a sweep
  of all 78 real `worlds/*.txt` files found 421-435/437 each placed at least once while 436/438/
  439/440 are never placed (highest real usage: 437 = 17 treasures); 436-439 still have valid
  `object-m.png` art despite being unused, so 440's specific out-of-bounds atlas gap is best
  explained as the atlas simply being sized for exactly 440 slots (0-439) — one short of the
  numbering scheme's theoretical max — a slot nobody ever needed art for, not a bug ever hit in
  the shipped game. No code changes; `BlockTypes.hpp` bounds deliberately left untouched as before.
- `3c65870` **docs only**: fixed 6 stale `TILE-0XX` entries — Fan (all 4 directions), Temperature,
  and Marine tile animations are already implemented in `GETerrainRenderer.cpp` but were still
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
  already-working mechanic. New `GESound::Stop(channel)` (per-channel, since `StopAll()` would
  kill every other sound) + `GEInteractionSystem::CrateBeingPushedThisFrame()` signal +
  `GalaxyEggbertCnaGame::wasPushingCrate_` turning it into a real start/stop loop.
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
  block in `GEInteractionSystem::Update()`, 6 new `VerifyInteractionSystem` assertions.
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
  static "Stop pose" stand-in. New `GEBlupiController::m_deathCause` (stored so `GetAnimIcon()` can
  select the right array; `m_pickupFreezeKind` already existed). Format-agnostic icon-index data
  only — no 3D model exists yet to actually display it, becomes visible once `069` ships. New
  `VerifyBlupiMovement` assertions (Clear1/Clear2/Sucette exact icon values). Full suite green both
  backends.
- `9ca723f` **fix: real vehicle-mode gate for crate push.** Verified against `Decor.cpp:6130-6132`
  — excludes every vehicle mode + Balloon/Ecrase. New `blupiCanPushCrate` parameter on
  `GEInteractionSystem::Update()`.
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
  `GEBlupiController::HasVehicleHazardImmunity()`.
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
  `GalaxyEggbertCnaGame.cpp`'s `canGrantPower/Cloud/Hide` computation; `GEBlupiController::
  TriggerTeleport()` now also checks `m_vehicleMode`. New tests in `VerifyBlupiMovement.cpp`
  (vehicle-mounted teleport no-op) and `VerifyInteractionSystem.cpp` (gated Sucette grant).
- `f6753ca` **fix: require the action button for Sucette/Drink pickups.** Real mobile-eggbert
  gates Sucette(26)/Drink(30) on the action button held at contact
  (`getButtonPressedProperty()==PlayAction`); this engine previously granted both automatically.
  Added `blupiActionPressedEdge` to `GEInteractionSystem::Update()`. Charge(31) deliberately has no
  such gate (confirmed via direct source read). New positive/negative tests added.
- `de6ef8b` **feat: implement real Sucette/Drink/Charge 2-stage pickup delay.** Real exact
  durations: Sucette=32 ticks(1.6s), Drink=36(1.8s), Charge=64(3.2s) — all three genuinely freeze
  Blupi, a third application of the freeze-timer pattern built for `TriggerTeleport()`/the death
  lock. Sucette/Drink defer their buff grant to completion; Charge's buff already grants at
  contact in real source (only its freeze/sounds were missing). New
  `GEBlupiController::TriggerPickupFreeze()`/`ConsumePickupFreezeResolved()`, new
  `GEInteractionSystem::RespawnPickupItem()`.
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
  New `GEBlupiController::TriggerDeathLock()`/`ConsumeDeathLockResolved()`/`IsDeathHidden()`. This
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
  `GEInteractionSystem::VoyageKind`/`BeginVoyage()`/`TickVoyage()`, new
  `GEHud::ProjectWorldToHudSpace()` (world→screen projection, didn't exist before).

## 4. Current blocker / main problem

**There is no active build or test failure blocking progress.** `build-cna` builds and passes
78/79 (the one failure is the pre-existing `easy-gl` dependency test, see §5). The current line of
work — the 3D world editor — is mid-plan but not stuck: EDITOR-111 is simply the next unstarted
milestone, fully specified and unblocked.

**Resolved 2026-07-19**: `build-cna-vulkan` was rebuilt and re-tested through the editor work plus
this session's animation wiring — 76/76 (100%), confirming the editor code (and everything else
changed since) really is backend-agnostic. EDITOR-112's own copy of this task is now done; only its
unsaved-changes-guard part remains.

The remaining blockers are items that genuinely need a **human decision**, not more engineering:
- Saw blade (icon 378) render orientation — needs the user to look at a screenshot/crop and state
  the correct orientation; 3 prior autonomous guesses were wrong.
- `AscenseurVertigo` (wide/shiftable lift platforms, icons 311-316) — needs the user to choose
  which of 3 existing render approaches to use.

If picking up this project without either of those answers, the honest "next thing to do" is one
of the concrete, non-blocked tasks in §8 below, not a bug fix.

## 5. Known bugs and limitations

- **Confirmed bug:** Saw blade (icon 378) render orientation is wrong. Location:
  `src/GalaxyEggbertCNA/Game/GETerrainRenderer.cpp` (its `InnerFlatPlate` handling) and
  `../easy-3d`'s `CubeMesh.cpp` (`AppendPlateMesh`). Needs user visual input before another fix
  attempt.
- **Incomplete (found 2026-07-18, while implementing Balloon's horizontal drift):** No
  horizontal-wall-collision at all for ANY airborne movement — `GEBlupiController::TryMoveAxis()`'s
  step-up gate only applies while `m_onGround`; otherwise a move always applies unconditionally.
  Affects plain jumping, every vehicle mode, and now Balloon's own real sideways drift too — a
  floating/jumping/riding Blupi drifts straight through a side wall he'd stop against while walking.
  A real fix needs a general airborne-collision primitive shared by all of these, not a per-status
  patch — out of scope for any single-mechanic task; flag if picked up.
- **Incomplete:** No visible 3D Blupi model (blocked on the user providing one).
- **Incomplete (world editor, by design — the remaining plan milestones):**
  - No sky-region picker (EDITOR-111): `world.skyRegion()` can't be changed from the editor.
    `GEEditCommand::Kind::SkyRegionEdit` is declared but has no fields or stack handling yet.
  - No unsaved-changes guard (EDITOR-112): Back/Open/Quit discard silently. Save is explicit
    (`Enter` or the toolbar button); Play-Test auto-saves first.
- **Incomplete (world editor, deliberate simplification, documented in the code):** no text
  rendering anywhere in the editor — toolbar buttons are distinguished by position, and the
  mode-toggle by color. (Object palette cells now draw real per-type sprites, not flat colors —
  the multi-atlas `GEObjectIcons::GetObjIcon` plumbing this needed was implemented 2026-07-19, see
  §3; this is no longer a gap.) Refinable later; not a functional gap (every tool has a working
  binding).
- **Resolved 2026-07-19:** `build-cna-vulkan` rebuilt/re-tested (see §4) — 76/76 (100%).
- **Risky assumption (world editor):** `ObjectType` category names/membership in
  `GEPaletteCategories.cpp` were taken **only** from `ObjectType.hpp`'s own documented comment
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
- **Risky assumption to keep in mind:** `GEInteractionSystem` is deliberately decoupled from both
  `GEBlupiController` and any camera/graphics type (see §6). Every new feature that needs either
  has had to route through a same-frame "pending signal" (`*ThisFrame()` flags) consumed by
  `GalaxyEggbertCnaGame`. It is tempting to "simplify" this by just passing a `GEBlupiController&`
  into `GEInteractionSystem::Update()` — don't; this has been a deliberate, repeated architectural
  choice across at least 3 features this session (Voyage, death-lock, pickup-freeze), not an
  oversight.

## 6. Architecture notes

**Core classes** (all in `src/GalaxyEggbertCNA/Game/` unless noted):
- `GEBlupiController` — Blupi's own movement/physics/state machine. Owns position, velocity,
  secret-power state, vehicle mode, and now the death-lock and pickup-freeze timers. Has **no**
  dependency on `GEInteractionSystem`, `GESound`, or world/graphics types. Its freeze-timer shape
  (`TriggerTeleport()`/`m_teleporting`, `TriggerDeathLock()`, `TriggerPickupFreeze()`) is a
  reusable template: set a flag + a duration, `Step()` early-returns while the flag is set,
  auto-clears when the timer elapses. Reuse this exact shape for any future "freeze Blupi for N
  seconds" mechanic rather than inventing a new one.
- `GEInteractionSystem` — pickups, hazards, enemies, doors, lifts, crates, secret-power grants,
  cheats. Deliberately has **no** access to `GEBlupiController`, `Easy3D::Camera3D`, or the
  `GraphicsDevice` — every real mechanic that needs Blupi's controller state or screen-space
  projection communicates via same-frame `*ThisFrame()` boolean signals (and, where multi-frame
  state is needed, a small pending-request struct) that `GalaxyEggbertCnaGame` reads and acts on
  right after `Update()` returns. Do not "fix" this by adding a direct dependency — see §5's own
  note.
- `GalaxyEggbertCnaGame` (`src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.hpp`/`.cpp`) — owns `blupi_`,
  `interaction_`, `worldRuntime_`, `camera_`, `sound_`, `hud_`. This is where cross-class
  orchestration lives: `ResolvePendingVoyage()`, `ResolveDeathLock()`, `ResolvePickupFreeze()` are
  all called once per frame, right after `interaction_.Update()` returns, to consume pending
  signals from `GEInteractionSystem` and drive `GEBlupiController`/`GEHud` accordingly.
- `GEWorldRuntime` — the live, mutable per-frame world state (terrain block queries plus the
  `MobileObjSpec` list of every pickup/enemy/effect object). `GEInteractionSystem` and
  `GalaxyEggbertCnaGame` both operate on the same `GEWorldRuntime&` instance each frame.
- `GEHud` — 2D HUD rendering in mobile-eggbert's own 640x480 reference space, plus the
  `ProjectWorldToHudSpace()` static utility (world position → that reference space, via a real
  clip-space `Vector4::Transform` then inverting `GEHud`'s own ref↔viewport scale/offset math).
- `include/GalaxyEggbert/Worlds/`, `def/*.hpp`, `BlockTypes.hpp` — engine-agnostic data model
  (`World`/`Chunk`/`Block`, real enum IDs). Shared by any future engine target; keep it that way
  (no CNA/Easy3D-specific dependencies here).

**World editor** (`src/GalaxyEggbertCNA/Editor/`, new 2026-07-18):
- `GEWorldEditor` is the only class `GalaxyEggbertCnaGame` talks to. It owns the free-fly camera,
  raycast/highlight state, undo stack, palette, box-fill state, and browsing/editing mode.
- It follows the **same "pending signal consumed by the owner" idiom** as `GEInteractionSystem`:
  it has no `GEWorldRuntime`, phase, or file-loading access, so it reports
  `ConsumeNeedsPresentationRebuild()` / `ConsumePlayTestRequested()` / a `BrowserRequest`, and
  `GalaxyEggbertCnaGame` performs the actual rebuild/phase-switch/load. Keep it that way.
- The lower helpers (`GEVoxelRaycast`, `GEBoxRegion`, `GEEditCommandStack`, `GEPaletteCategories`,
  `GECustomWorldStorage`) are **pure logic with no CNA/Easy3D dependency**, which is what lets
  `VerifyGEWorldEditor` link and test them headlessly. Do not introduce graphics dependencies into
  these five — it would silently cost the whole test suite.

**Coordinate invariants (the single easiest thing to get wrong here):**
- **Block centering:** block index `N` spans `[N-0.5, N+0.5)` — *not* `[N, N+1)`. Position→index
  conversion uses `std::lround`, not `floor` (see `GEWorldRuntime`). `GEVoxelRaycast` bakes the
  corresponding `+0.5` shift into its DDA.
- **Render space vs raw grid space:** render space = raw grid space shifted by
  `-GEWorldRuntime::kWorldCenterX/Z` (= 50) on **X and Z only**; Y is identical in both.
  `Worlds::World` and `MoveObjectRecord` are always raw grid space; the camera is always render
  space. `GEWorldEditor::Update()` converts internally so callers never apply the shift themselves.
- **`MoveObjectRecord` anchoring:** a record is stored in the block-extra-metadata of
  `floor(posStart)`, and at most one record may occupy a given anchor cell — placing a second
  silently replaces the first. Undo of an overwrite must therefore restore the previous record, not
  just clear the cell (`GEEditCommandStack::ApplyMoveObjectState` does exactly this).
- **`RebuildWorldPresentation()` now calls `GEWorldRuntime::ResyncFromWorld()` first.** The editor
  mutates the live `World` in place with no disk round-trip, so `LoadFromVwrFile()` — previously the
  only thing that populated `mobileObjects_`/`skyRegion_`/`missionNumber_` — never runs for an
  in-editor edit. Removing this call makes placed objects invisible until save+reload.
- **Palette vs world clicks:** `GEEditorPalette` claims the mouse from the **press** frame onward
  (per press, not per cursor position), because world edits are press-triggered while the palette
  acts on release. Don't "simplify" this back to a release-only check — see §2's bug note.

**Invariants / boundaries that must not be broken:**
- `../mobile-eggbert` is never modified, not even temporarily, not even for "just looking."
- `../simple-3d` is read-only.
- No `#ifdef` guards distinguishing engine backends anywhere in `GalaxyEggbertCNA`.
- `GEInteractionSystem` stays free of `GEBlupiController`/camera/graphics dependencies (see above).
- Real numeric IDs (`ObjectType`, `BlockTypes`, sound channels) must never be renumbered — they
  encode the real mobile-eggbert data format.
- `GESaveData`/`GEInputPad`/`GEInteractionSystem` byte layouts are intentionally not compatible
  with real mobile-eggbert's own save/format — don't "fix" this without a deliberate, separate
  decision (see `CLAUDE.md`'s reuse table).

## 7. Useful commands

Configure + build (EasyGL, the default backend):
```
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
```

Configure + build (Vulkan backend):
```
cmake -S . -B build-cna-vulkan -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF -DCNA_GRAPHICS_BACKEND=VULKAN
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

- **Skip every task that needs visual judgment on a render/screenshot entirely** — Saw blade
  orientation, `ThinMechanical` geometry (`510`), water surface treatment (`512`),
  architectural-kit assembly (`514`), enemy billboard walk-cycle angle (`179`), `AscenseurVertigo`
  (`153`/`PICKUP-024`). Do not touch their rendering code or take "best guesses" at geometry —
  leave them exactly as flagged until the user can look at a screenshot themselves.
- **`GESaveData` stays an independent format, not byte-compatible with real mobile-eggbert saves**
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

The editor plan (EDITOR-111/112) is the active line of work; do these in order first. Each is one
focused session, and each ends with: build → `VerifyGEWorldEditor` → full `ctest` → live headless
check of the new behavior → revert instrumentation → commit → push.

1. **EDITOR-111 — Sky-region picker.** A stepper cycling `world.skyRegion()` 0–31, flagging which
   ids have real `Content/backgrounds/decorNNN.png` art (only 28 of 32 do — it must degrade
   gracefully, see `RebuildWorldPresentation()`'s own comment). Implement
   `GEEditCommand::Kind::SkyRegionEdit` in the stack (currently declared but unhandled).
   Files: `GEWorldEditor.{hpp,cpp}`, `GEEditCommandStack.{hpp,cpp}`, `GEEditorPalette.{hpp,cpp}`,
   `tools/VerifyGEWorldEditor.cpp`.
   Verify: `VerifyGEWorldEditor` (wrap-around at both ends + undo/redo) plus a live screenshot
   showing the background actually swap.

2. **EDITOR-112 — Hardening pass + full regression.** Unsaved-changes guard (dirty flag + one-tap
   confirm on Back/Open/Quit), a box-fill test that straddles world bounds, consolidate
   `VerifyGEWorldEditor` into clearly-named sections. (**The `build-cna-vulkan` re-run this item
   also called for is done — see §4, done 2026-07-19 independent of the rest of this task, since
   it needed no editor-specific judgment call.**)
   Files: `src/GalaxyEggbertCNA/Editor/*`, `tools/VerifyGEWorldEditor.cpp`.
   Verify: `cd build-cna && ctest` and `cd build-cna-vulkan && ctest`, plus a final live
   walkthrough of the whole editor → play-test → editor loop.

Non-editor tasks, available if the editor line is paused:

1. ~~Take the live screenshot verification of the death-lock/life-loss Voyage one step further~~
   — **done 2026-07-20.** Temporary, fully-reverted debug scaffold (env-var-gated: skip to Play,
   overwrite the ground block directly under Blupi's spawn with Lava via
   `GEWorldRuntime::GetWorldMutable().setBlock()`, periodic stderr state log + timed screenshots
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
2. **Get the user's visual judgment on the Saw blade (icon 378) orientation**, then fix it.
   Files: `src/GalaxyEggbertCNA/Game/GETerrainRenderer.cpp`, `../easy-3d`'s `CubeMesh.cpp`
   (`AppendPlateMesh`).
   Verify: live headless screenshot at the Saw demo block in `worlds3d/world001.vwr` +
   `VerifyBlupiMovement`.

3. **Get the user's decision on `AscenseurVertigo` render geometry** (icons 311-316, which of the
   3 existing render approaches to reuse), then implement it.
   Files: `src/GalaxyEggbertCNA/Game/GETerrainRenderer.cpp`.
   Verify: `cmake --build build-cna --target VerifyTileUvBounds -j2` plus a live screenshot.

## 9. Do not do yet

- **No work on `GalaxyEggbertSimple3D`** — historical reference only, per explicit user directive.
  Do not build, fix, or troubleshoot it.
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
- **No graphics dependencies in the five pure-logic editor helpers** (`GEVoxelRaycast`,
  `GEBoxRegion`, `GEEditCommandStack`, `GEPaletteCategories`, `GECustomWorldStorage`) — it would
  break `VerifyGEWorldEditor`'s headless linkage (see §6).
- **No whole-file `git checkout` to revert live-test instrumentation.** It can silently discard
  real uncommitted work in the same file (this happened once during EDITOR-109 and had to be caught
  and restored). Revert with targeted edits, then confirm with `git diff`.
- **No refactor of the `GEInteractionSystem`/`GEBlupiController` decoupling** (see §5/§6) — it is a
  deliberate, repeatedly-reaffirmed design choice, not technical debt.
- **No 3rd guess at the Saw blade orientation** without the user's own visual input — 3 prior
  autonomous attempts were wrong.
- **No broad refactor or unrelated cleanup** while any of the §8 tasks are in flight — each is
  meant to be a single, small, independently-verifiable session.
- **No Lua**, no MeshCraft/Mesh World/Nova3D/further-Simple3D features — none of these are part of
  the locked Direct-CNA-+-Easy3D direction.

## 10. Resume prompt

**PAUSED 2026-07-19 — do not use the prompt below until the user explicitly asks to resume editor
work.** The user asked to pause the world editor line of work (EDITOR-111/112 remain undone) and
move on to other areas of `galaxy-eggbert` instead — see plan.md §6 for the full status write-up
(what's built, what's left, and an unresolved keyboard-input concern to check first if this does
get picked back up). If you're starting a session cold and this note is still here, ask the user
what they want worked on next rather than defaulting to the prompt below.

```
Read NEXT.md first, in full, before doing anything else.

Then work on exactly ONE task from its "Next smallest tasks" section (§8) — start
with task 1 (EDITOR-111, sky-region picker) unless it is already done. Read only
the files that task names; do not open or refactor unrelated files.

Before writing editor code, read §6's "Coordinate invariants" — block centering
(index N spans [N-0.5, N+0.5), lround not floor), the render-space vs raw-grid-space
+50 X/Z shift, and MoveObjectRecord anchoring. These are the easiest things here to
get subtly wrong.

Do not refactor unrelated code. Do not touch ../mobile-eggbert or
GalaxyEggbertSimple3D. Do not invent gameplay mechanics not confirmed in real
mobile-eggbert source (editor UX is the one approved exception — see §9). If you
need to look at free-eggbert's code, delegate that to a subagent that returns only
what's needed.

Make one small, verified improvement. Run:
  cmake --build build-cna --target VerifyGEWorldEditor -j2 && cd build-cna && ./VerifyGEWorldEditor
then the full suite: cd build-cna && ctest
For anything visual, verify live under xvfb-run with temporary env-var-gated
instrumentation, and REVERT it with targeted edits before committing — confirm with
git diff, never a whole-file git checkout (§9 explains why).

Commit after the task is done and verified, then push to origin/develop.

When done, update NEXT.md: move the completed task out of §8 into §3 (Recent
changes) with a short factual summary, and adjust §2/§4/§5 if the change affects
them. Keep the whole file honest and concise — do not describe anything as done
that you have not actually verified this session.
```
