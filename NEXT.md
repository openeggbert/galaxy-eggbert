# NEXT.md — Galaxy Eggbert

_Last updated: 2026-07-16 (autonomous session, see §7.5 for standing directives)._

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
Last full run (2026-07-16, both backends, re-verified after this session's 5 vehicle-gate fixes):
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

### Recently implemented (this session, 2026-07-16 — see §3 for detail)
A systemic class of bug found and fixed **7 times** this session: real vehicle-mode (Helicopter/
Overcraft/Jeep/Tank/Skateboard) exclusion/immunity clauses that predate Phase 17's vehicle
implementation and were never retrofitted once vehicles actually shipped. Each verified directly
against `Decor.cpp`, not guessed:
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

(Prior session, 2026-07-13/14): real deferred "Voyage" pickup-reward timing, Clear2/3/4 death VFX,
the death-lock + life-loss-Voyage system, Sucette/Drink/Charge's 2-stage pickup delay.

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
- **No 3D world editor work this session** (`EDITOR-000..010`) — a dedicated future effort, not
  part of this one.
- **Blupi-model prep IS authorized, format-agnostic only**: e.g. animation-state timing/signal
  plumbing, third-person placeholder improvements, asset-loading scaffolding that would work
  regardless of the eventual real model's exact format/rig. Do NOT commit to a specific model
  format or file layout without the user's input.
- Similarly discovered *this session*: rescaling `kGravity`/`kJumpSpeed`'s own absolute magnitude
  to real tick-domain values (part of `065`) needs the user's live-feel judgment, same as the
  visual-judgment items above (see `065`'s own plan.md entry for why) — left open, not a green
  light to guess.

## 8. Next smallest tasks

1. **Take the live screenshot verification of the death-lock/life-loss Voyage one step further**:
   isolate a frame showing the flying icon-48 HUD animation itself (the earlier live check
   confirmed the life-total/position state transition but didn't catch the icon mid-flight at the
   screenshot intervals used).
   Files: temporary debug scaffold only (see `plan.md`'s "death-lock" writeup, Phase 15, for the
   exact technique used last time — spawn Blupi on a hazard tile, skip to `Play` phase, capture
   timed screenshots, fully revert before committing).
   Verify: visual inspection of the captured `.png`, no code changes should survive.

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
