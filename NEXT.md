# NEXT.md — Galaxy Eggbert

_Last updated: 2026-07-14._

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
- Worlds are hand-authored via `tools/GenerateSampleWorld3D.cpp` (writes `worlds3d/world001.vwr`);
  there is no automatic 2D→3D world converter and none is planned.
- `GESaveData`/`GEInputPad`/`GEInteractionSystem` are deliberately **not** byte-compatible with
  real mobile-eggbert's own formats — real *behavior* is ported faithfully, byte layout is not.

## 2. Current status

### Build status
Both build trees configure and build cleanly as of the last verification this session
(2026-07-14):
- `build-cna/` — EasyGL backend (`CNA_GRAPHICS_BACKEND=EASYGL`, the default).
- `build-cna-vulkan/` — Vulkan backend (`CNA_GRAPHICS_BACKEND=VULKAN`).
- `GalaxyEggbertSimple3D` is **not built** (per direction lock above) — its build is known broken
  in this environment (missing/incompatible U3D prebuilt) and this is intentionally left unfixed.

Note observed this session: a `sharp-runtime` (external sibling dependency, `../sharp-runtime`)
build once failed with a duplicate `Environment::SetEnvironmentVariable` declaration/definition
conflict, then succeeded on an immediate retry with no changes on this side — that repository
appears to be under active, independent development and the failure was transient. If it recurs,
it is not caused by anything in this repository; check `../sharp-runtime`'s own git log first.

### Test status
Last full run (2026-07-14, both backends):
- `build-cna`: **78 tests, 99% pass** — the only failure is `easy-gl-resource-smoke-tests`, a
  **pre-existing, unrelated** failure in the `easy-gl` dependency, not caused by this repository's
  own code. It has been the same single failure across many verification passes this session.
- `build-cna-vulkan`: **73/73 tests, 100% pass.**
- `GalaxyEggbertWorldsTests` (gtest, engine-agnostic `World`/`Chunk`/`MoveObjectRecord` model) is
  included in both totals above.

### Tools/binaries available (see `CMakeLists.txt` for exact target names)
- `GalaxyEggbertCNA` — the main game executable.
- `GenerateSampleWorld3D` — writes `worlds3d/world001.vwr`, the hand-authored demo world.
- Scripted verification tools (each an `add_test()`-registered ctest case): `VerifyBlupiMovement`,
  `VerifyInteractionSystem` (largest suite — pickups, hazards, cheats, secret powers, death/respawn
  timing), `VerifyGEInputPad`, `VerifyGESaveData`, `VerifyMoveObjectTypesCna`,
  `VerifyBigDecorParsingCna`, `VerifyTerrainAnimDivisor`, `VerifyTileUvBounds`, `VerifyCameraShake`.
- `GalaxyEggbertSimple3D`, `VerifyBigDecorParsing`, `VerifyMoveObjectTypes` — Simple3D-only targets,
  not built per the direction lock.

### Recently implemented (this session, 2026-07-13/14 — see §3 for detail)
- Real deferred "Voyage" pickup-reward timing (treasure/keys/egg/dynamite/Perso/bullet pack/door
  unlock).
- Clear2/Clear3/Clear4 death VFX (soul-ascend HUD animations, Saw's particle burst).
- A full death-lock + life-loss-Voyage system: every real hazard death now locks Blupi for a real
  fixed duration, plays the real life-loss Voyage, then respawns him — replacing the previous
  instant death/respawn. Verified live in the running game (screenshots), not just in unit tests.
- Sucette(26)/Drink(30)/Charge(31) real 2-stage pickup delay (grab → freeze → complete), including
  the real action-button gate for Sucette/Drink.

### Known working demo
`worlds3d/world001.vwr`, loaded automatically by `GalaxyEggbertCNA` on startup. Playable with
first-/third-person camera toggle (`C` key), tank-control movement, and the full interactive-object
system (pickups, hazards, enemies, doors, lifts, crates).

### What does not work yet
- **No visible 3D Blupi model** — invisible collision point in first-person; a placeholder model
  in third-person. Blocked on the user providing a real model/rig.
- **Saw blade (icon 378) render orientation is wrong** — 3 prior fix attempts were wrong; needs the
  user's own visual judgment (a screenshot review), not another guess.
- **No 3D world editor** — worlds are hand-edited directly in `tools/GenerateSampleWorld3D.cpp`.
- Several real HUD buttons render but are intentionally inert (no desktop equivalent exists yet):
  `SetupJump`/`SetupZoom`/`SetupAccel`, `PauseBack`, `InitRanking`/`InitBuy`.
- Idle "fidget" periodic sounds — blocked on `AnimState` values this engine doesn't have (same
  prerequisite as the 3D Blupi model).
- `GalaxyEggbertSimple3D` — historical reference only, not built/maintained.

## 3. Recent changes

Most recent first. Full history: `git log`.

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

**There is no active build or test failure blocking progress.** Both backends build and pass their
full test suites (see §2). The practical blockers right now are a small number of items that
genuinely need a **human decision**, not more engineering:
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
- **Incomplete:** No visible 3D Blupi model (blocked on the user providing one).
- **Incomplete:** No 3D world editor.
- **Incomplete/suspected gap — needs verification:** Real Sucette(26)/Drink(30)/Charge(31)/Shield(25)
  pickup gates in real mobile-eggbert also exclude several vehicle modes
  (`!m_blupiHelico && !m_blupiOver && !m_blupiBalloon && !m_blupiEcrase && !m_blupiJeep &&
  !m_blupiTank && !m_blupiSkate`). This engine's `GEInteractionSystem.cpp` pickup switch does not
  currently check vehicle mode for these 4 pickups at all — since vehicles ARE implemented in this
  engine (5 vehicle mounts, per §2), this may be a real, live gap now, not just a historical
  "vehicles don't exist yet" simplification. Not verified or fixed this session; verify against
  `Decor.cpp:6014-6087` directly before assuming it's a real gap or a non-issue.
- **Known simplification, not a bug:** The real per-cause hurt-sprite animation frame table
  (`Tables::table_blupi`) has not been transcribed for the death-lock (`AnimState::DeathLocked`) or
  pickup-freeze (`AnimState::PickupBusy`) states — both render a static "Stop" pose instead. The
  freeze *timing* is faithful; the animation artwork is not.
  (`src/GalaxyEggbertCNA/Game/GEBlupiController.cpp`/`.hpp`).
  Only the state transition was confirmed live; a screenshot of the flying icon itself was not
  isolated.
- **Needs verification:** `easy-gl-resource-smoke-tests`'s single failing assertion
  (`test_texture_upload_sets_unpack_alignment_wrap_and_unit0_binding`,
  `../easy-gl/tests/smoke/SmokeResourceTests.cpp:336`) — treated as pre-existing/unrelated
  throughout this session (present before and after every change made), but has not been
  root-caused. If it ever becomes the *only* thing standing between a clean and a failing suite,
  investigate it properly rather than continuing to assume it's benign.
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

Regenerate the sample world (only needed after editing `tools/GenerateSampleWorld3D.cpp`):
```
cmake --build build-cna --target GenerateSampleWorld3D -j2
cd build-cna && ./GenerateSampleWorld3D
```

No lint/format tooling is configured in this repository at present.

## 8. Next smallest tasks

1. **Verify (and if needed, fix) the vehicle-mode gate gap for Sucette/Drink/Charge/Shield
   pickups.**
   Goal: confirm whether real vehicle-mode exclusions
   (`!m_blupiHelico/Over/Balloon/Ecrase/Jeep/Tank/Skate`) apply to these 4 pickups in this engine
   today, and if not, add them.
   Files: `src/GalaxyEggbertCNA/Game/GEInteractionSystem.cpp` (the `ObjectType25/26/30/31` switch
   cases), `GalaxyEggbertCnaGame.cpp` (wherever `canGrantShield`/`canGrantPower`/etc. are computed
   from `blupi_`'s vehicle state).
   Verify: `cmake --build build-cna --target VerifyInteractionSystem -j2 && cd build-cna &&
   ./VerifyInteractionSystem` (add a new test case mounting a vehicle then touching one of these
   4 pickups).

2. **Root-cause `easy-gl-resource-smoke-tests`'s failing assertion**, even though it's currently
   treated as pre-existing/unrelated (see §5). At minimum, confirm it fails identically on a fresh
   clone with no galaxy-eggbert-side changes, so the "unrelated" assumption is verified rather than
   inherited.
   Files: `../easy-gl/tests/smoke/SmokeResourceTests.cpp:336` and whatever
   `g_state.last_active_texture`/`0x84C0` (`GL_TEXTURE0`) tracking it depends on.
   Verify: `cd build-cna && ctest -R easy-gl-resource-smoke-tests --output-on-failure`.

3. **Take the live screenshot verification of the death-lock/life-loss Voyage one step further**:
   isolate a frame showing the flying icon-48 HUD animation itself (the earlier live check
   confirmed the life-total/position state transition but didn't catch the icon mid-flight at the
   screenshot intervals used).
   Files: temporary debug scaffold only (see `plan.md`'s "death-lock" writeup, Phase 15, for the
   exact technique used last time — spawn Blupi on a hazard tile, skip to `Play` phase, capture
   timed screenshots, fully revert before committing).
   Verify: visual inspection of the captured `.png`, no code changes should survive.

4. **Get the user's visual judgment on the Saw blade (icon 378) orientation**, then fix it.
   Files: `src/GalaxyEggbertCNA/Game/GETerrainRenderer.cpp`, `../easy-3d`'s `CubeMesh.cpp`
   (`AppendPlateMesh`).
   Verify: live headless screenshot at the Saw demo block in `worlds3d/world001.vwr` +
   `VerifyBlupiMovement`.

5. **Get the user's decision on `AscenseurVertigo` render geometry** (icons 311-316, which of the
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
  from plausibility.
- **No refactor of the `GEInteractionSystem`/`GEBlupiController` decoupling** (see §5/§6) — it is a
  deliberate, repeatedly-reaffirmed design choice, not technical debt.
- **No 3rd guess at the Saw blade orientation** without the user's own visual input — 3 prior
  autonomous attempts were wrong.
- **No broad refactor or unrelated cleanup** while any of the §8 tasks are in flight — each is
  meant to be a single, small, independently-verifiable session.
- **No Lua**, no MeshCraft/Mesh World/Nova3D/further-Simple3D features — none of these are part of
  the locked Direct-CNA-+-Easy3D direction.

## 10. Resume prompt

```
Read NEXT.md first, in full, before doing anything else.

Then work on exactly ONE task from its "Next smallest tasks" section (§8) — pick the
first one that isn't blocked on a decision only the user can make. Read only the
files that task names; do not open or refactor unrelated files.

Do not refactor unrelated code. Do not touch ../mobile-eggbert or
GalaxyEggbertSimple3D. Do not invent gameplay mechanics not confirmed in real
mobile-eggbert source.

Make one small, verified improvement. Run the exact verification command that task
lists, on both build-cna and build-cna-vulkan if the change touches shared game code.

When done, update NEXT.md: move the completed task out of §8 into §3 (Recent
changes) with a one-line factual summary, and adjust §2/§4/§5 if the change affects
them. Keep the whole file honest and concise — do not describe anything as done
that you have not actually verified this session.
```
