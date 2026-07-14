# NEXT.md — Galaxy Eggbert

_Last updated: 2026-07-13. Concise handoff document — full history lives in `git log` (502+
commits) and `plan.md`'s per-phase task lists. This file summarizes current state; it does not
replace either of those._

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (itself a C++ port of *Speedy
Blupi*, a 2013 Windows Phone XNA game). Main goal: reimplement the same game logic, levels, and
assets in 3D (perspective camera, billboard sprites, 3D-rendered tiles) **without inventing new
mechanics** — see `CLAUDE.md`'s "Faithful Remake" rule, which overrides default behavior.

**Current development phase:** `GalaxyEggbertCNA` is the sole actively-developed target and has
reached a genuinely playable state — real terrain/objects/enemies/hazards/HUD/sound/save-system/
menus, though still missing a visible 3D Blupi model.

- **`GalaxyEggbertCNA`** (built directly on **CNA** + the **Easy3D** helper library) — opens a
  window, loads a hand-authored `.vwr` world (`worlds3d/world001.vwr`), renders real
  textured/animated terrain (4 confirmed tile render modes, all ~175 confirmed icons wired,
  face-culled), moves an invisible collision-point Blupi (tank controls + vehicles), and has a
  complete real gameplay loop: pickups, all 6 confirmed terrain hazards, the full enemy-AI/combat set,
  platform lifts/crates/bridges, doors & keys, secret powers/vehicles, bullet firing, a real
  `Def::Phase` state machine (Wait→Init→Play→Pause/Win/Lost/Setup/Resume, with real fade-out
  transitions between phases), a cross-restart save system (3 gamer slots), and a hidden cheat
  menu. No visible Blupi model yet (an invisible collision point in first-person; third-person
  shows a temporary placeholder model), no 3D world editor, one known-wrong render detail (Saw
  blade orientation).
- **`GalaxyEggbertSimple3D`** (built on `simple-3d` → U3D/Urho3D) — **historical reference only as
  of 2026-07-08, per user directive. Do not build, fix, or troubleshoot it.** Its build is broken
  in this environment (missing/incompatible U3D prebuilt) and that is intentionally left unfixed.

**Important architectural decisions:**

- **Direct CNA + Easy3D is the sole long-term target (locked 2026-07-05).** `GalaxyEggbertSimple3D`
  is scheduled for gradual removal as CNA reaches parity — its code stays intact for now (no
  removal without an explicit task), but is not built/fixed/verified going forward.
- **No Blupi 3D model exists yet — the user will provide one later.** Current stopgap: first-person
  camera only (no visible character) plus a small 2D animation-state sprite indicator; third-person
  mode shows a temporary CC0/CC-BY placeholder model via CNA's `AvatarRenderer`. Not a design
  decision about the eventual real model's look.
- Easy3D is a small helper library beside CNA (cameras, texture atlas, batching, mesh building) —
  must not hide CNA; game code calls CNA directly. Standing permission to modify `../easy-3d`
  (small, generic 3D-batching helpers only — no scene graph/ECS/engine creep).
- `../mobile-eggbert` is **read-only, never modified even temporarily**. Assets (PNGs, sounds,
  world files) are freely reused by direct path/build-time copy; code/data (`Decor.cpp`, tables,
  enum values, byte layouts) is reference-only and requires explicit user approval to copy — see
  `CLAUDE.md`'s reuse table. It has no CMake library target, so it cannot be linked as a dependency.
- No automated `.txt → .vwr` 2D-to-3D world converter (rejected direction) — worlds are
  hand-authored via `tools/GenerateSampleWorld3D.cpp`.
- **Terrain-tile identification is complete** — all 441 `object-m.png` icons accounted for across
  4 confirmed render modes (`DirectionalCube` 99, `InnerPillarBox` 3, `InnerFlatPlate` 63,
  `TripleCrossBillboard` 10), all wired up. No further tile-identification work remains except the
  Saw blade's own internal texture-orientation bug (see §4).
- **GESaveData/GEInputPad/GEInteractionSystem are NOT byte/format-compatible with real
  mobile-eggbert equivalents** (`GameData`, `InputPad`, `Decor`) — deliberately so, confirmed via
  research each time: the real formats are shaped around a 100+-level/3-gamer-slot/touch-hardware
  structure this engine's single hand-authored world doesn't have. Real *behavior* (formulas,
  gates, timings) is ported faithfully; byte layout is not.

## 2. Current status

### Build status
- **`GalaxyEggbertCNA` — last confirmed clean build on both graphics backends: 2026-07-13**
  (commit `63e020d`). Two build trees exist in this repo, both currently configured and buildable:
  - `build-cna/` — EasyGL backend (`CNA_GRAPHICS_BACKEND=EASYGL`, the default).
  - `build-cna-vulkan/` — Vulkan backend (`CNA_GRAPHICS_BACKEND=VULKAN`).
  - Current CMakeLists.txt defaults: `GALAXY_EGGBERT_BUILD_CNA=ON`, `GALAXY_EGGBERT_BUILD_SIMPLE3D=OFF`
    (i.e. a plain `cmake -S . -B build` with no flags now configures CNA, not Simple3D).
- **`GalaxyEggbertSimple3D` — not built, per user directive (2026-07-08).** Known broken in this
  environment (`find_package(Urho3D)` fails, U3D prebuilt missing/incompatible) — intentionally
  left unfixed. Do not attempt to build or fix it.

### Test status (all last run and passing, 2026-07-14)
- `GalaxyEggbertWorldsTests` — 64/64 (gtest, engine-agnostic `World`/`Chunk`/`MoveObjectRecord`
  model, no CNA link needed).
- `VerifyBlupiMovement` — collision/step-up/gravity/vehicle-movement checks against
  `worlds3d/world001.vwr`.
- `VerifyInteractionSystem` — pickups, hazards, cheats, secret powers, bullet firing, Cloud aura
  (largest suite, links CNA).
- `VerifyGEInputPad` — on-screen control hit-testing (D-pad/Jump/Action/Pause/Setup/Init/cheat
  gesture), synthetic `MouseState`, no `GraphicsDevice` needed.
- `VerifyGESaveData` — `GESaveData` load/save round-trip, 3-gamer-slot isolation, `Reset()`.
- `VerifyMoveObjectTypesCna` — `MoveObject` parsing against real mobile-eggbert `.txt` level files
  (curated per-`ObjectType` examples, plus an exhaustive sweep of all 78 real world files, plan.md
  TEST-003, added 2026-07-14).
- `VerifyBigDecorParsingCna` — `BigDecor:` parsing against real mobile-eggbert `.txt` level files.
- `VerifyTerrainAnimDivisor` — terrain hazard-tile animation-phase divisor mapping (plan.md
  TEST-007, added 2026-07-14), no CNA/graphics link needed.
- `VerifyTileUvBounds` — `BlockTypes::tileUV()` atlas-rect validity for every icon (plan.md
  TEST-004, added 2026-07-14), no CNA/graphics link needed.

### What works (high level — see `plan.md` for the exhaustive per-item checklist)
- Real textured/animated 3D terrain (4 render modes, face-culled), real background image per world.
- Tank-control Blupi (invisible collision point) + first-/third-person camera toggle.
- Full interactive-object system: platform lifts (patrol + riding), crates (linked-stack push),
  bridges, doors (key- and treasure-gated), all pickups (treasure/egg/keys/dynamite/bullets/Perso/
  secret-power pickups), level exit.
- Full enemy AI/combat: shared 8-type hazard kill-list, wasp balloon status, blupih/blupit
  projectile attacks, large-creature turn-dwell grab, follower wake+homing, all real patrol timing.
- All 6 real terrain hazards (lava/spikes/Blitz/saw+switches/crusher/water-drip, the last added
  2026-07-14) + spring, Temp/vanishing tile, teleporter, water breath gauge.
- Secret powers (Shield/Power/Cloud/Hide) fully modeled including Cloud's offensive
  `BlupiElectro` aura (destroys small enemies within range) and all 5 vehicle mounts. Invert/Mirror
  (independent movement-reversal debuff/buff) also implemented (2026-07-13).
- Player-fired bullets while riding Tank (real cooldown/ammo gates).
- Real camera shake, all 3 types wired (Fan-death/fish-bird-hazard-kill BigShake, wasp-sting
  ElectricShake, generic-hazard-kill/dynamite-blast/CleanAll SmallShake) and Ghost mode (typed-word
  cheat: free flight, no gravity/collision/interactions), both added 2026-07-14.
- The first real particle effect (Invert start/stop 4-direction burst, added 2026-07-14) — logic
  and positions independently confirmed correct via unit tests; live visual confirmation was
  attempted but inconclusive (see §3's own note).
- Real mobile-eggbert-faithful HUD (`GEHud`): lives/keys/treasure/bullets/dynamite/Perso icons,
  water and secret-power gauges, training-hint overlay — every element the real `Decor::DrawInfo`
  draws is implemented.
- Real `Def::Phase` state machine: `Wait` (progress gauge) → `Init` (gamer-select menu, 3 save
  slots) → `Play` ↔ `Pause` → `PlaySetup`/`MainSetup` (settings) , `Win`/`Lost` screens, `Resume`
  (offered after a checkpointed run). Real fade-out transitions between the 5 phases that actually
  defer (`Init`/`MainSetup`/`PlaySetup`/`Pause`/`Resume`) — Play↔Pause and Win/Lost are real
  instant cuts, confirmed via research, not merely fast.
- Cross-restart save system (`GESaveData`, plain text, not byte-compatible with real `GameData`):
  sound on/off + 3 independent gamer slots (lives/mission/hasProgress) + selected-gamer index.
- Hidden cheat menu: 10-tap gesture unlocks a 9-button overlay (OpenDoors/SuperBlupi/LayEgg/Reset/
  CleanAll/AllTreasure/EndGoal implemented; ShowSecret and the Trial toggle are documented
  gaps/no-ops). Plus a real second cheat-entry method (typed word during Play) — only "ghost" is
  wired so far (Ghost mode: free flight, no gravity/collision/interactions).
- 93/93 real sound channels.

### What does not work / is not implemented
- **No visible 3D Blupi model** — invisible collision point in first-person, placeholder model in
  third-person. Blocked on the user providing a real model/rig; this is the single largest
  remaining gap (`plan.md E3D-MIG-069`).
- **Saw blade (icon 378) render orientation is still wrong** after 3 fix attempts — see §4.
- **No 3D world editor** — worlds are still hand-authored by editing
  `tools/GenerateSampleWorld3D.cpp` (`plan.md` §6, planned, not started).
- Several real buttons render at their correct position/icon/label but are intentionally inert —
  no meaningful desktop equivalent exists yet: `SetupJump`/`SetupZoom`/`SetupAccel` (touch-side/
  auto-zoom/accelerometer settings), `PauseBack` (hub-level navigation, no hub system exists),
  `InitRanking`/`InitBuy` (real gate resolves to "never shown by default" anyway), Init's own
  semi-transparent gamer-slot background panels (`MENU-014/015`, pure cosmetic).
- Cheat3 "ShowSecret" is a documented gap — its real citation conflicts with this engine's own
  already-confirmed `ObjectType12` (crate); deliberately not guessed through.
- Idle "fidget" periodic sounds (`plan.md #085`) — blocked on new `AnimState` values this engine
  doesn't have yet (same prerequisite as the 3D Blupi model work).
- `GalaxyEggbertSimple3D` — historical reference only, not buildable/maintained (see above).

## 3. Recent changes

Most recent first. Full history: `git log`. Everything below is from **2026-07-13/14** (one very
long continuous autonomous session); each item is its own commit.

- **Implemented the Invert start/stop particle burst — the FIRST real particle effect in this
  engine** (plan.md BLUPI-110/VISUAL-014/015), per the user's explicit direction to start on the
  particle-effects system now that data-table transcription is approved. Real spawn sites
  confirmed via direct `Decor.cpp` reads: grant places 4 instances (up/down/+X/-X) at exactly 500
  real-px from Blupi; expiry places them at 400 real-px (a real, deliberate, closer-in radius, not
  symmetric with grant). Real `SearchDistRight()` short-circuits to a flat 500px for this specific
  effect — no raycast/wall-collision logic needed. New `GEInteractionSystem::SpawnInvertBurst()`,
  called directly at the game class's existing grant/expiry sites. Found and fixed 2 real bugs in
  `GEObjectIcons.cpp`'s existing (previously unused) icon formulas along the way: wrong divisor (6
  instead of the real 2) and ObjectType42 ascending past the valid range instead of matching the
  real table's exact reverse order. 8 new `VerifyInteractionSystem` checks + full regression on
  both backends, all pass. **Live visual verification was attempted but inconclusive** — confirmed
  via a live debug print that the object spawns at the exact right position, but the real sprite
  (a small ~5%-coverage element.png icon) wasn't clearly distinguishable in headless screenshots
  within a reasonable time budget. Reuses the same already-proven billboard rendering path used
  by every other element.png object in this engine, so this is a lower-risk gap than a new
  rendering mode would be — see plan.md VISUAL-014's own note for the full reasoning. Flagging
  this openly rather than either fabricating a "confirmed working" claim or endlessly debugging.
- **Wired SmallShake's real trigger sites (plan.md CAM-008)** — the earlier camera-shake commit
  (below) left this unwired pending research; direct re-verification of every cited `Decor.cpp`
  site found the "7 near-identical dynamite-blast sites" description was wrong — it was actually a
  MIX of 3 different mechanics. (1) The generic-hazard contact-kill branch (already implemented as
  this engine's own `IsGenericHazard()` logic) plays SmallShake for 6 of its 8 types and BigShake
  specifically for fish/bird (`ObjectType17`/`20`) — a real exception found along the way. (2) The
  REAL dynamite blast plays SmallShake only ONCE, at the blast's own center tile, not
  per-destroyed-object — matches this engine's existing center-tile ch10-sound branch exactly. (3)
  The CleanAll cheat's enemy-destruction loop plays it once per real destroyed enemy, modeled here
  as once-per-invocation via a new `bool CheatCleanAll()` return value. 3 other cited sites turned
  out to be different, not-yet-implemented mechanics (Perso-decoy-kills-enemy, `ObjectType201-203`
  contact damage — plan.md's own already-flagged `PICKUP-069` gap — forced-vehicle-dismount) and
  were correctly left unwired. 6 new `VerifyInteractionSystem` checks + full regression on both
  backends, all pass.
- **Implemented the camera-shake system (plan.md CAM-008..013/HUD-014)** — previously blocked on
  `Tables::table_decor_action`'s data-table transcription approval, granted by the user the same
  day. Verbatim-transcribed the real 519-entry table into new `GECameraShake.hpp`/`.cpp`
  (engine-agnostic, no CNA/graphics dependency) and INDEPENDENTLY byte-verified all 519 values via
  a script comparing against the real source directly — not just visually proofread. Ported
  `DecorNextAction()`'s exact real per-frame logic (20Hz tick, real ×3 multiplier, self-clear on
  table exhaustion, unconditional restart-on-retrigger with no priority gating). Corrected 2 wrong
  trigger descriptions along the way: BigShake's real trigger is Fan/Ventillo contact killing
  Blupi (not "large explosion"/ObjectType11 — that's a cosmetic particle spawned alongside it, not
  the trigger condition), and ElectricShake's real trigger is specifically the wasp-sting/balloon
  event (not a generic "electric field tile" despite the enum's own doc comment). Wired both into
  `GalaxyEggbertCnaGame.cpp`'s existing Fan-hazard and wasp-balloon blocks; applied as a small 3D
  camera-space translation (eye+target shifted equally, preserving look direction) converted from
  real pixels via the same 64px-per-tile scale used throughout this engine's own atlas math.
  SmallShake's own real triggers (dynamite-blast destroyed objects, CleanAll cheat) are NOT wired
  yet — a real, scoped follow-up needing a new signal path from `GEInteractionSystem` to the
  camera. 18 new `VerifyCameraShake` checks + full regression on both backends, all pass.
- **Implemented Ghost mode (plan.md BLUPI-111)** — the user provided the missing real trigger
  directly (typed word "ghost", a real second cheat-entry method in mobile-eggbert distinct from
  the on-screen button dispatch), confirmed via a direct source read (`InputPad.cpp:686-753`): a
  rolling lowercase-letter buffer, Play-phase-gated, suffix-matched against a real ~26-entry cheat
  name table. Ported the buffer mechanism (`GEInputPad::UpdateTypedGhostCheat()`, only "ghost"
  wired for now) and the real free-flight movement (`GEBlupiController`'s top-priority `m_ghost`
  branch in `Step()` — real exact 4x speed, no gravity, no collision, world-bounds clamp) and "no
  interactions" (a sentinel far-away Blupi position passed into `GEInteractionSystem::Update()`
  while ghosting, making every existing proximity check fail shut with zero changes inside that
  class). Toggle-on clears any vehicle mount; toggle-off is rejected while standing inside solid
  geometry (both real behaviors). 12 new `VerifyBlupiMovement` checks + 4 new `VerifyGEInputPad`
  checks + full regression on both backends, all pass. Also, per explicit user direction the same
  day: data-table transcription from mobile-eggbert is now approved (unblocking camera shake and
  several particle-effects items for a future pass), and `BLUPI-108`/`BLUPI-126/127/131`/
  `SCORE-001..007`/`VISUAL-001..007` are confirmed HALLUCINATED and CANCELLED (no real
  mobile-eggbert source) rather than merely "unconfirmed."
- **Closed plan.md TEST-003** — `VerifyMoveObjectTypesCna` now also sweeps every real
  `../mobile-eggbert/worlds/*.txt` file (enumerated at runtime, not hardcoded) confirming
  `GEWorldRuntime::LoadFromMobileEggbertFile()` succeeds on all of them, closing the literal "all
  world files parse without error" ask (previously only a curated per-`ObjectType`-example subset
  was covered). All 78 real world files parse cleanly. Full regression on both backends passes.
- **Found and fixed a real icon-440 atlas-bounds bug in galaxy-eggbert's own exhibition demo**
  (plan.md TEST-004 investigation) — `BlockTypes::tileUV()`'s formula places icon 440 at atlas
  pixel rect `(1,1431)`-`(65,1495)`, entirely outside the real `object-m.png`'s actual 1301×1431
  bounds (confirmed via direct Python/PIL crop attempt — fails). Icons 0..439 all fit correctly;
  this is a 1-icon overshoot at the very last slot, and `GEObjectIcons.cpp` already independently
  uses bound 439 elsewhere with a comment calling this "object-m.png's 440-icon grid" — the
  440-vs-441 discrepancy has apparently been latent and unreconciled across files for a while.
  Confirmed zero real-world impact (icon 440 is never placed in any real mobile-eggbert level
  file) — the only place in this repo that ever rendered it was galaxy-eggbert's own synthetic
  "museum" exhibition demo, now fixed to exclude it. Deliberately did NOT touch the foundational
  `BlockTypes.hpp` `kPassable[441]`/`tileUV()` logic itself — see plan.md TEST-004 for why that
  needs deliberate follow-up, not a rushed fix. Also closed the underlying `TEST-004` ask itself:
  added `VerifyTileUvBounds` (ctest-registered), confirming every icon 0..439 fits the real atlas
  and explicitly locking in icon 440 as a known, tracked exception rather than a silent one.
- **Added `VerifyTerrainAnimDivisor` and closed plan.md TEST-007** — the real per-type terrain
  hazard-tile animation-phase divisor mapping (`AnimDivisor()`: Saw div 1, Lava div 2, Water1/
  Crusher/Water2/Marine/the 4 Fan icons div 3, Spike/Temp div 4) was already correctly implemented
  but untested. Extracted `AnimDivisor()` out of `GETerrainRenderer.cpp`'s anonymous namespace into
  its own `GETerrainAnimDivisor.hpp`/`.cpp` (behavior unchanged) so a new lightweight,
  engine-independent tool could link just that pure function with no CNA/graphics dependency, same
  precedent as `VerifyGESaveData`/`VerifyBlupiMovement`. 13 checks, now ctest-registered. Full
  regression on both backends passes (76 tests on EasyGL, 71/71 on Vulkan).
- **Ruled out several opportunistic "next task" candidates via direct source verification** (no
  code changes, documentation-only, all pushed): `BLUPI-108`'s "Cloud floats through blocks" line
  was stale (real effect already shipped as `BlupiElectro`); `BLUPI-111` Ghost-mode cheat is real
  but has no player-facing trigger in real source; `BLUPI-126/127/131` "stomp kill" is an invented
  Mario-style mechanic; a follow-up research fork's own `ENEMY-CONTACT-001` claim was itself wrong
  and retracted (contact detection is already comprehensive); the entire `SCORE-001..006` numeric
  score system and `VISUAL-001..007` (shadows/bobbing/tint/blink/respawn-flash) all lack any real
  source and look invented; the rest of the particle-effects backlog (§7.5) is real but blocked on
  data-table transcription approval. See each item's own plan.md correction for citations.
- **Implemented the Suspended/hanging bar-and-rope movement mode** (real `Decor::GetTypeBarre()`,
  plan.md TILE-045) — a genuinely new movement mode comparable in scope to the 5 already-
  implemented vehicles, following up the icon-ID research earlier this session (icons 138/202,
  cross-confirmed against `Decor.cpp` and the user's own prior questionnaire answer).
  `GEBlupiController::GetBarreCellType()` classifies a grid cell as `None`/`Hanging`/
  `LandingAvailable` (bar tile absent / open air below / solid ground below); grabbing is automatic
  on contact (no button), matching the real trigger exactly. While hanging: direct no-ramp
  horizontal movement (reusing the same shape this engine's normal walk already has), re-classifies
  at the new position every step (reaching a `LandingAvailable` cell releases gracefully onto solid
  ground; reaching `None` — walking off the structure — drops Blupi into free-fall, same as holding
  Down for the real ~0.25s threshold), and Jump immediately releases with a real proportionally-
  anchored upward launch (the real 10-tick wind-up animation isn't modeled — no visible Blupi model
  exists to show one). Real 5-tick no-regrab grace timer modeled as a direct transcription.
  `TriggerMount()` now also excludes hanging (the real gate already documented this exclusion).
  Demo bar added to `worlds3d/world001.vwr` (icon 138 and icon 202, each in its own row — icon
  202 now has its own new render geometry, `plan.md TILE-055`, see below). 11 new
  `VerifyBlupiMovement` checks (grab, climb, graceful landing, free-fall drop, jump-release,
  grace-timer block + expiry) + a live headless screenshot sanity check + full regression on both
  backends, all pass.
- **Implemented the "thin-bar" render geometry for icon 202** (plan.md TILE-055) — new
  `GEThinBarTiles.hpp`/`.cpp`, modeled on the existing `GEInnerPillarBoxTiles` precedent: a thin
  `DirectionalCubeItem` (full block width along X, `kThinBarThickness=0.3` in Y/Z) with the real
  texture on its 4 long sides and a flat blue `SwatchUv` fallback on the 2 end caps, matching the
  user's 2026-07-07 questionnaire description of icon 202. Live screenshots initially showed a
  solid white block instead of the intended thin rod; direct pixel inspection of `object-m.png`
  found icon 202's real crop is a thin stripe on an otherwise fully-transparent tile, and the
  static-mesh renderer's opaque pass doesn't respect alpha (same bug category as icons 30/31 and
  the teleporter pillars). Fixed by adding icon 202 to `GETerrainRenderer.cpp`'s
  `NeedsAlphaBlend()`, routing it through the existing alpha-respecting transparent pass. Confirmed
  fixed via live headless screenshots (close range and 3/4 angle): the tile now renders as a thin,
  mostly-transparent rod, not a solid block. Full regression on both backends passes.
- **Implemented the water-drip terrain hazard** (icon 404, plan.md TILE-032) — real
  `Decor::IsGoutte()`, corrected from a wrong "triggers a glu/slow effect" premise (same category
  of error as TILE-041/045 earlier this session): confirmed via a direct `Decor.cpp` read that real
  behavior is a deterministic kill, mechanically identical to Spike (same gate shape, same real
  channel 51 sound). Added `BlockTypes::Drip`, wired the hazard-kill check + safe-respawn-position
  exclusion in `GalaxyEggbertCnaGame.cpp` exactly mirroring Spike's own code. Now a real 6th
  confirmed instant-kill terrain hazard (previously 5: lava/spike/saw/crusher/Blitz). Real visual
  is a green vase/bulb, not a liquid graphic — falls back to the default `UniformCube` render, an
  already-correct resolution. `GetGroundBlockType()` recognition test added; full regression +
  both backends pass.
- **Implemented jump-height headroom modulation** (`ObjectType`-agnostic Blupi physics, plan.md
  TILE-041) — real `Decor::IsNormalJump()`, corrected from a wrong "forces a jump when stepped on"
  premise to its real behavior: a ceiling-clearance headroom check that reduces jump strength when
  Blupi's head would clip a nearby ceiling. `GEBlupiController::HasJumpHeadroom()` probes the 2 grid
  cells above his current standing height; 3 new proportionally-anchored speed constants
  (`kJumpSpeedPowered`/`kJumpSpeedReduced`/`kJumpSpeedReducedPowered`) give the real 4 magnitudes
  (clear/blocked × Power/no-Power). Verified against the real south tunnel's low BrickWall ceiling
  with an exact expected-velocity match (accounting for the same frame's own gravity subtraction),
  not just a pass/fail — full regression on both backends.
- **Wired all 7 test/verify tools into `ctest`** (plan.md TEST-002) — `GalaxyEggbertWorldsTests`
  turned out to already be ctest-discoverable; added `add_test()` for the 6 `VerifyXxx` binaries
  (with an explicit repo-root `WORKING_DIRECTORY` for the 3 that need it). `ctest --test-dir
  build-cna --output-on-failure` now runs everything in one command on both backends — see §5 for
  the one pre-existing, unrelated third-party test failure this surfaced (not a regression).

- **Implemented bridge construction** (`ObjectType52`, plan.md PICKUP-064) — the last of the
  genuinely-open gaps found by the `plan.md` reconciliation. While grounded on a Bridge (icon 364)
  tile, spawns an object that writes the construction sequence directly into the terrain grid every
  tick — a real, live ground-collision toggle (28 ticks ascending 365-372, 112 ticks holding the
  real `-1`/no-tile sentinel as `Air`, 17 ticks descending back to 364), not just a cosmetic
  overlay. Reuses the existing `BlockTypes::fromMobileIconId()` passable-icon conversion, so no
  `GroundHeightAt()` changes were needed. Real per-tick `table_bridge` array not transcribed (no
  pre-approved source) — reproduces the documented 28/112/17-tick shape with this engine's own
  uniform pacing. Demo gap+bridge added to `tools/GenerateSampleWorld3D.cpp` (careful to avoid
  grid (90,*,90), which `VerifyBlupiMovement`'s own fall-off-world test relies on staying empty —
  caught this collision via a real regression run and relocated the demo). 5 new
  `VerifyInteractionSystem` checks (spawn, mid-sequence hollow, an independent `GEBlupiController`
  actually falling through, restoration, self-delete) + full 7-tool suite + both backends pass.

- **Implemented Invert/Mirror secret power** (`ObjectType40`, plan.md PICKUP-011) — a real
  gameplay gap found while reconciling `plan.md` against source (see below). Independent of the 4
  existing SecretPower buffs (own gauge, real gate only `!Hide`), negates normal ground movement
  while active (`GEBlupiController::TriggerInvert()`/`IsInverted()`), real ~15s duration (no
  warning stage), ch66/67 pickup/expiry sounds. Demo pickup added to the secret-powers room in
  `worlds3d/world001.vwr`. New tests in both `VerifyBlupiMovement` (11 checks: gating, timing,
  expiry, the actual movement-reversal effect) and `VerifyInteractionSystem` (world-pickup contact
  wiring); full 7-tool suite + both backends pass.
- **Reconciled `plan.md`'s Feature Parity Checklist with actual source** — sections 2.6 (Enemy AI)
  and 2.7 (Pickups & Objects) had gone badly stale after the 2026-07-11/12/13 implementation push
  (2.6 still said "not started at all"); 55 items flipped `[ ]`→`[x]`, ~26 description/sound-channel
  corrections, after direct verification against `GEInteractionSystem.cpp`/`GEBlupiController`/
  `GEHud`/`GESound`. A follow-up pass caught and fixed 2 false negatives in the first audit itself
  (secret-power and switch pickup sounds were actually already wired, just in a different file than
  the audit checked) — a live example of why "trust but verify" matters even for careful audits.
  5 genuinely-open gaps remained after all corrections; Invert (above) closed one of them.
- **Fixed `GEBlupiController::GroundHeightAt()`'s roofed-interior limitation** — previously always
  scanned from the world's topmost Y down for a column's "floor", so a real ceiling anywhere above
  an open interior (e.g. a roofed tunnel) registered as that column's ground, misresolving Blupi
  onto TOP of the ceiling instead of the real floor beneath it (`SetPosition()`/walking into any
  enclosed/roofed space was broken). Fixed by bounding the scan with a `referenceY` parameter:
  `TryMoveAxis` passes `m_y + kStepLimit` (still detects legitimate step-up ground within reach),
  the vertical/gravity pass passes plain `m_y` (a ceiling above where Blupi already is is now
  irrelevant to where he lands). Verified two ways: (1) a new `VerifyBlupiMovement` check against
  the real south tunnel (floor at grid y=0, BrickWall ceiling at grid y=3, `tools/
  GenerateSampleWorld3D.cpp`) — confirmed via `git stash` that this check FAILS on the pre-fix code
  (lands at y=4, the ceiling) and PASSES after the fix (lands at y=1, the real floor); (2) full
  regression: `GalaxyEggbertWorldsTests` (64/64), all 6 `VerifyXxx` tools, both EasyGL and Vulkan
  builds. Files: `src/GalaxyEggbertCNA/Game/GEBlupiController.cpp`/`.hpp`,
  `tools/VerifyBlupiMovement.cpp`.
- **Fixed 3D-world render bleed-through during Wait/Init** — reported live by the user: at game
  start, the 3D terrain/background was visible in the pillarbox margins around (and, on
  non-4:3-aspect windows, alongside) the Wait loading-gauge screen and Init's gamer-select menu.
  Root cause: `GalaxyEggbertCnaGame::Draw()` always rendered the full 3D world (background/
  terrain/billboards) every frame regardless of phase; Wait/Init's own 2D UI only covers the
  centered 640×480 reference area, leaving the wider-aspect pillarbox bars showing whatever 3D
  content was drawn underneath. Fixed by skipping the entire 3D-world render block for exactly
  `Wait`/`Init` (the only two phases with no real game world to show — Wait is a fake progress
  screen since `LoadContent()` already loaded everything up front, and Init is the gamer-select
  menu entered before any level is being played); `Pause`/`Win`/`Lost`/`PlaySetup`/`MainSetup`/
  `Resume` are deliberately untouched since real mobile-eggbert legitimately freezes and shows the
  in-progress game world behind those overlays. Verified via live headless screenshots of both
  phases (temporary `kWaitDurationSeconds` debug override to reach Init immediately, reverted
  before commit) + full 7-tool regression suite. File:
  `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.cpp` (`Draw()`).
- **Cloud secret-power electric aura** (`plan.md #068`) — while `SecretPower::Cloud` is active,
  destroys small enemies (Bulldozer/blupih/blupit) within a real 40px aura, real sound channel 59.
- **Fall-death timing halved** — `kFallDeathY` -60.0f→-27.0f per user feedback (was ~6.7s, now
  ~3.4s); a game-feel tweak, not a faithfulness correction.
- **Real fade-out phase transitions** (`plan.md MENU-088/089`) — only `Init`/`MainSetup`/
  `PlaySetup`/`Pause`/`Resume` ever defer a transition (~1.0s exit animation); every other phase
  transition is a real instant cut. Also added Pause/Resume's own 0.75s entrance flourish and
  MainSetup/PlaySetup's speedyblupi slide-in + rotating gear decorations. A real bug (infinite
  re-defer loop from clearing `fadeOutPhase_` before recommitting) was caught and fixed during live
  verification.
- **Wait/Init boot flow + gamer-select menu** (`plan.md MENU-001..020`) — engine now starts in
  `Wait` (progress gauge) instead of directly in `Play`; `Init` renders a real 3-gamer-slot
  select screen. `GESaveData` extended from 1 implicit slot to 3 independent slots.
- **Hidden cheat menu** (`plan.md CHEAT-001..009`/`MENU-092..102`) — corrected several wrong
  draft descriptions against real source (e.g. "LayEgg" sets lives=9, doesn't spawn an egg;
  gesture is a 10-tap sequence, not 6).
- **Tank bullet firing** (`plan.md BULLET-001`) — confirmed real bullets are never a weapon
  against enemies (bullet-vs-Blupi contact only); dedicated "F" key, Tank-only, real
  cooldown/ammo gates.
- **Resume phase + save data extended to lives/mission** (`plan.md MENU-040..045`).
- **Minimal settings persistence** (`plan.md MENU-067`) — first version of `GESaveData`.
- **PlaySetup (settings) screen** (`plan.md MENU-058..069`).
- **Win/Lost screens + Pause button text labels** (`plan.md MENU-046..057`).
- **Pause screen + functional on-screen Play controls** (`plan.md MENU-021..027/028..039`) — new
  `GEInputPad` class, a mouse-driven port of the real `InputPad`.
- **Real `Def::Phase` state machine** (`plan.md HUD-023`) — the full real 13-value enum wired up,
  simulation gated behind `Phase::Play` exactly like the real `Decor::MoveStep()`.
- **Phase 9 (HUD) completed** — training-hint overlay (`HUD-024`), Perso decoy mechanic + counter
  (`HUD-017`), both `jauge.png` gauges (`HUD-008/012/018/019`), bullet/dynamite counters
  (`HUD-015/016`). A full read-through of the real `Decor::DrawInfo` confirmed every element it
  draws is now implemented.

**Earlier (before 2026-07-13, still true):** Phases 13–17 (enemy AI, hazards, crates/lifts/bridges,
doors & keys, secret powers/vehicles/buffs) all substantially/fully complete (2026-07-11/12) — see
`plan.md` for the itemized per-phase checklists. Terrain-tile identification, all 4 render modes,
face culling, real background images, third-person placeholder camera, `MoveObject`/`BigDecor`
billboard rendering, and the CNA target skeleton itself were built 2026-07-01 through 2026-07-10 —
see `git log` for the full commit-by-commit history of that earlier work.

## 4. Current blocker / main problem

**There is no build- or test-breaking blocker right now.** Both graphics backends build clean and
all 7 automated test/verify tools pass as of the last commit (2026-07-13, `63e020d`).

The one open, paused item is the **Saw blade's render orientation** (icon 378, `InnerFlatPlate`
render mode) — wrong after 3 fix attempts across live user feedback rounds. Per explicit user
direction (2026-07-12): **do not attempt a 4th blind fix** — this needs the user's own direct
visual judgment (a screenshot/crop review), not another autonomous guess. A new, not-yet-acted-on
finding exists: the real icon 378 texture's actual content occupies only the bottom half of its
64×64 tile, and there's an unverified theory that `Easy3D::AppendPlateMesh` vertically flips
whatever texture it's given. This is purely a visual/UV-mapping issue — it does not affect any
test or the build.

The previously-open `GEBlupiController::GroundHeightAt()` roofed-interior limitation (resolved a
column's floor as the single topmost solid block with no concept of "nearest solid surface at or
below Blupi's own height", making any roofed/enclosed interior unreachable) **is now fixed
(2026-07-13)** — see §3 and §5 for the full writeup. The fan-hazard/Saw-verification demo rooms
that previously worked around it by using open-sky placements were left as-is (no need to
retrofit), but new enclosed-space content no longer needs that workaround.

## 5. Known bugs and limitations

| Status | Issue |
|---|---|
| **open, needs_human — do not guess again** | Saw blade (icon 378) render orientation still wrong; see §4. |
| **pre-existing, third-party, unrelated to galaxy-eggbert** | `ctest --test-dir build-cna` (since the 2026-07-14 TEST-002 fix) bundles in a couple of `../easy-gl`/`../meta-gl` smoke tests alongside galaxy-eggbert's own 7 tools. `easy-gl-resource-smoke-tests` fails an assertion about GL texture-unit binding state (`SmokeResourceTests.cpp`, `g_state.last_active_texture`) — confirmed unrelated to any galaxy-eggbert change (a sibling dependency's own test, out of scope to fix per CLAUDE.md's `../easy-3d`/`../cna`-adjacent sibling-repo rules; `../easy-gl` isn't even in that explicit list, treat with the same caution). Not a regression; galaxy-eggbert's own 7 tools are unaffected and all pass. |
| **fixed 2026-07-13** | `GEBlupiController::GroundHeightAt()` used to misread a ceiling as the floor for any roofed/enclosed interior. Fixed by bounding the scan with a `referenceY` parameter (current Y, or current Y + step-up allowance) instead of always scanning from the world's topmost Y; verified against the real south tunnel (a `git stash`-confirmed before/after test) + full regression on both backends. See §3. |
| **CNA upstream bug, worked around — needs approval to fix upstream** | CNA's Vulkan backend records all `SpriteBatch` draws before all 3D draws each frame, so a sprite HUD drawn after the 3D scene gets painted over. `galaxy-eggbert` no longer uses `SpriteBatch` (`GEHud` draws real 3D quads instead), so it's unaffected — but any future `SpriteBatch` use would be. |
| **CNA quirk, worked around** | A `BasicEffect` draw with `Alpha < 1` renders on EasyGL but not at all on CNA's Vulkan backend. `GEHud`'s treasure panel uses opacity 1.0 instead of the real mobile-eggbert 0.6 until this is fixed upstream (`kPanelOpacity`). |
| **accepted limitation, user declined a fix (2026-07-09)** | `GalaxyEggbertCNA` exits with code 1 (not 0) when closed via a real window-manager close request — root cause is inside SDL's own X11 teardown (`../cna`/SDL), needs explicit approval to fix. Do not attempt without new approval. |
| **known, deliberately out of scope for now** | The pillarbox-margin 3D-world bleed-through fixed for Wait/Init (§3, 2026-07-13) still exists for `Pause`/`Win`/`Lost`/`PlaySetup`/`MainSetup`/`Resume` on non-4:3-aspect windows — NOT a bug for those phases (real mobile-eggbert legitimately shows the frozen game world behind them), but the pillarbox bars themselves (outside the centered 640×480 reference area) are unfilled letterboxing, a separate minor cosmetic gap from the one just fixed. Not reported by the user; only fix if asked. |
| **re-tested 2026-07-13, not reproducible — closed** | The "texture distance washout" investigation (`texture-distance-washout-bug.md`) was conducted while an unrelated cube-winding bug was active. Re-ran the same repro (5/12/25 units from a BrickWall face, temporary debug camera override, reverted): sharp, fully-detailed texture at all 3 distances, zero pixels matching the original flat-gray value. Very likely the `+Y`/`-Y` cube-winding fix (`../easy-3d` commit `8ab3854`) was the real cause all along; see the doc's own "Re-test conclusion" section. No further action unless reported again live. |
| **root-caused and fixed 2026-07-13 (galaxy-eggbert-side workaround)** | Under Vulkan specifically, `screenshot_hud.png` had shown a plain blue background with un-blended white boxes around billboards, even though the same frame's terrain-visibility check reported real terrain color. Root cause: calling `device.GetBackBufferData()` mid-frame (the terrain-visibility diagnostic's own readback) corrupts the REST of that same frame's rendering on CNA's Vulkan backend (confirmed by temporarily disabling the mid-frame call — the HUD screenshot then rendered perfectly). Not fixable in `../cna` without separate approval, so worked around entirely in `galaxy-eggbert`: `Draw()` now tracks `drawFrameIndex_`/`terrainPixelPrintedFrame_` so the one-shot HUD screenshot always waits for a frame strictly after whichever frame ran the mid-frame terrain readback, guaranteeing it captures an uncorrupted frame. Verified on both backends + full 7-tool suite. File: `src/GalaxyEggbertCNA/GalaxyEggbertCnaGame.cpp`/`.hpp` (`Draw()`). |
| **not reproduced via scripted collision test** | "Grass-topped cubes reported walkable-through" — a new `VerifyBlupiMovement` check (2026-07-13) drops Blupi onto all 9 icon-107/108/109 blocks in `worlds3d/world001.vwr`; all land correctly (`IsSolidAt()` is independent of which faces a render mode draws). Collision logic itself is not at fault for this world's blocks. Possible explanation: icon 107/108/109 intentionally leave their `PosY` face un-rendered (drawn instead by the separate `grass_top.png` overlay plate), which may visually read as an open hole from some camera angles even though the block is solid — an optical-illusion theory, not confirmed. Still needs a live repro with specific coordinates if the report recurs. |
| **risky assumption** | World/texture loading uses relative paths — `GalaxyEggbertCNA` only works when run from its own build directory. |
| **incomplete** | `GETerrainRenderer`'s face culling only covers the plain static `UniformCube` path — animated/water paths render every face unconditionally (fine while those are sparse decorative elements, not bulk fills). |
| **incomplete** | No per-zone fog (Simple3D only had `SetClearColor` per sky region; not evaluated for CNA). |
| **not to be fixed (per user, 2026-07-08)** | `GalaxyEggbertSimple3D` build broken (missing U3D prebuilt) — historical reference only, do not spend effort on it. |

## 6. Architecture notes

### Main modules
```
include/GalaxyEggbert/Worlds/, src/GalaxyEggbert/Worlds/
    Engine-agnostic voxel World (100×100 grid), Block/Chunk, .vwr save format. Shared by both
    targets. Chunk's sparse ChunkBlockMetadataRecord mechanism stores arbitrary per-block payloads
    (used by MoveObjectRecord below).

include/GalaxyEggbert/MoveObjectRecord.hpp, src/GalaxyEggbert/MoveObjectRecord.cpp
    Engine-agnostic: encodes/decodes a MoveObject (pickup/enemy/lift/crate) as a World
    block-extra-metadata payload, anchored at floor(posStart) in World's own raw grid space.
    PlaceMoveObject()/CollectMoveObjects().

include/GalaxyEggbert/BlockTypes.hpp
    Tile type constants; block type = icon index in object-m.png. Ground/StoneA/StoneB are
    CONFIRMED MISLABELED (machine-piece graphics, not terrain) — do not use for new bulk fills;
    use RockPile/BrickWall or another questionnaire-confirmed-genuine icon instead.

src/GalaxyEggbertSimple3D/
    Historical reference only as of 2026-07-08 — not built/maintained. Do not mutate into the CNA
    implementation.

src/GalaxyEggbertCNA/
    GalaxyEggbertCnaGame — owns GEWorldRuntime, GETileAtlas, GETerrainRenderer, GEBlupiController,
    GEInteractionSystem, GEObjectIcons, GEHud, GESound, GEInputPad, GESaveData, an Easy3D::Camera3D
    (first-person) + third-person AvatarRenderer toggle. Also owns bigDecorCells_/objectCubeEffect_
    for BigDecor/platform-lift/crate rendering.

    GETerrainRenderer: per block, one shared AppendSpecialGeometry() helper checks 4 special
    render-mode tables in order (DirectionalCube, InnerPillarBox, InnerFlatPlate,
    TripleCrossBillboard), falling back to plain UniformCube. 5 CubeMeshRenderer-backed meshes:
    static opaque (face-culled), static-but-transparent, grass-top overlay, animated opaque, water.

    GEInteractionSystem: platform lift/crate/bridge patrol+push, all pickups, all 5 terrain
    hazards, enemy AI/combat (shared kill-list, wasp balloon, blupih/blupit projectiles, large
    creature, follower), secret-power pickups/effects (including the Cloud aura), bullet firing,
    cheat effects (CheatOpenDoors/CheatCleanAll/CheatAllTreasure/CheatFindExit).

    GEBlupiController: movement/gravity/collision, vehicle modes, secret-power state, crouch/crush/
    balloon/teleport states.

    GEInputPad: mouse-driven port of the real InputPad — Play D-pad/Jump/Action/Pause,
    Pause/Setup/Resume/Init/WinLost screens, the hidden cheat gesture+menu overlay. Real
    drawBounds-relative button rects don't fit this engine's fixed 640×480 reference space — every
    rect here is a proportionally-adapted layout, not a literal port (except a few cases that
    coincidentally need zero adaptation at this engine's own 480 reference height).

    GESaveData: plain key=value text file, NOT byte-compatible with real GameData. soundEnabled
    (global) + 3 independent GamerSlot{lives, missionNumber, hasProgress} + selectedGamer index.

tools/GenerateSampleWorld3D.cpp
    Builds worlds3d/world001.vwr — the only hand-authored demo world. No 3D world editor exists;
    this is how new terrain/objects get added today.

mobile-eggbert-reference/
    Prose catalogs: 02-tiles.md (441 icons), ObjectType/sound/animation catalogs, a gameplay-
    behavior spec (10-blupi-mechanics.md etc.) — approved as documentation, not a license to copy
    code/data into galaxy-eggbert without separate approval each time.

../easy-3d/
    Companion library beside CNA. Standing permission to modify (small, generic 3D-batching
    helpers only). CubeMesh/CubeMeshRenderer, BillboardMesh/BillboardMeshRenderer.
```

### Important invariants
- Block type = icon index in `object-m.png` (20 cols, 64×64px tiles, 1px gap + 1px leading margin).
- World grid is 100×100; `GEWorldRuntime` defines a 50-offset world-center constant.
- `ObjectType` (204 IDs) and `SoundChannel` (93 IDs) are numerically identical to mobile-eggbert's
  — do not renumber.
- No `#ifdef GE_ENGINE_*` anywhere — each target speaks its own API directly.
- `BlockTypes::Ground`/`StoneA`/`StoneB` must not be used for new bulk-terrain fills (confirmed
  mislabeled). Only add a new icon to any `GE*Tiles.cpp` table with its exact confirmed answer from
  the questionnaire files in `mobile-eggbert-reference/` — never guess a render mode/face config.
- `PlateItem`/`TripleCrossItem`/`DirectionalCubeItem` are the only "special geometry" primitives —
  add a new one only for a genuinely new shape, not as a `GETerrainRenderer` workaround.
- The 5 real deferring phases (`Init`/`MainSetup`/`PlaySetup`/`Pause`/`Resume`) and their exact
  fade formulas are load-bearing for `GalaxyEggbertCnaGame::SetPhase()`/`Update()` — see that
  method's own class comment before changing phase-transition logic.
- Build with `-j2` maximum (32 GB RAM constraint; crashes observed with more parallel jobs).

### Boundaries that must remain stable
- `BlockTypes::fromMobileIconId()` — must match the mobile-eggbert world file format exactly.
- `ObjectType`/`SoundChannel` numeric values — stored in level files, must never be renumbered.
- `src/GalaxyEggbertSimple3D/` must not be mutated into the CNA implementation.
- mobile-eggbert stays read-only, never modified even temporarily; no code/data copied from it
  without explicit user approval per use.
- `../cna` and `../simple-3d` are sibling repos: read freely, modify only with explicit per-change
  approval. `../easy-3d` has standing permission (scoped to small, generic 3D-batching helpers).

## 7. Useful commands

```bash
# Configure + build GalaxyEggbertCNA, EasyGL backend (default):
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA        # must run from its own build dir (relative asset paths)

# Configure + build GalaxyEggbertCNA, Vulkan backend:
cmake -S . -B build-cna-vulkan -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF -DCNA_GRAPHICS_BACKEND=VULKAN
cmake --build build-cna-vulkan --target GalaxyEggbertCNA -j2

# Regenerate the hand-authored 3D sample world (after tools/GenerateSampleWorld3D.cpp changes):
cmake --build build-cna --target GenerateSampleWorld3D -j2
./build-cna/GenerateSampleWorld3D worlds3d/world001.vwr

# Build + run all tests/verify tools. Since 2026-07-14 (plan.md TEST-002/TEST-007/TEST-004) all 9
# are ctest-registered with the correct working directory baked in, so a single ctest invocation
# from ANYWHERE covers everything (no more need to cd to repo root or invoke each binary by hand):
cmake --build build-cna --target GalaxyEggbertWorldsTests VerifyBlupiMovement VerifyInteractionSystem \
    VerifyGEInputPad VerifyGESaveData VerifyMoveObjectTypesCna VerifyBigDecorParsingCna \
    VerifyTerrainAnimDivisor VerifyTileUvBounds -j2
ctest --test-dir build-cna --output-on-failure
# Note: this also runs a handful of third-party (../easy-gl, ../meta-gl) smoke tests bundled into
# the same ctest run -- 1 of those (easy-gl-resource-smoke-tests) has a known pre-existing,
# unrelated failure (see NEXT.md §5); it is not one of galaxy-eggbert's own 9 tools/suites and is
# not a regression if you see it fail.

# Equivalent manual invocation of just galaxy-eggbert's own 9 tools, if isolating from the
# third-party smoke tests above (must run from repo ROOT — several tools use
# ../mobile-eggbert-relative paths that only resolve correctly from there):
./build-cna/GalaxyEggbertWorldsTests
./build-cna/VerifyBlupiMovement
./build-cna/VerifyInteractionSystem
./build-cna/VerifyGEInputPad
./build-cna/VerifyGESaveData
./build-cna/VerifyMoveObjectTypesCna
./build-cna/VerifyBigDecorParsingCna
./build-cna/VerifyTerrainAnimDivisor
./build-cna/VerifyTileUvBounds

# easy-3d: CNA-linked build + tests
cmake -S ../easy-3d -B /tmp/e3d-build-cna -DEASY3D_LINK_CNA=ON -DEASY3D_CNA_BACKEND=EASY_GL -DEASY3D_CNA_DIR=../cna
cmake --build /tmp/e3d-build-cna -j2 && ctest --test-dir /tmp/e3d-build-cna --output-on-failure

# Reference: mobile-eggbert animation tables / gameplay logic (read-only, never modify)
grep -n "table_decor\|table_blupi" ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp
less ../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp

# GalaxyEggbertSimple3D — historical reference only, NOT to be built/fixed (shown for completeness):
# cmake -S . -B cmake-build-debug -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON
# cmake --build cmake-build-debug --target GalaxyEggbertSimple3D -j2
```

No `.clang-format`/`.clang-tidy` config exists in this repo — no lint/format tooling to run.

## 8. Next smallest tasks

1. **Resolve the Saw blade (icon 378) render orientation** — needs the user's own visual judgment
   first (a fresh screenshot/crop review), not another blind guess. Once the user confirms the
   correct orientation: adjust `GETerrainRenderer`'s `InnerFlatPlate` UV/rotation handling for this
   icon (check the "vertical flip in `AppendPlateMesh`" theory noted in §4 first).
   Files: `src/GalaxyEggbertCNA/Game/GETerrainRenderer.cpp`, `../easy-3d`'s `CubeMesh.cpp`
   (`AppendPlateMesh`). Verify: live headless screenshot at the Saw demo block + `VerifyBlupiMovement`.

2. ~~Re-run the texture-distance-washout repro on current code.~~ **DONE 2026-07-13, CLOSED** — not
   reproducible at 5/12/25 units; see §5 and the doc's own "Re-test conclusion" section.

3. ~~Investigate the Vulkan-only `screenshot_hud.png` blue-background anomaly~~ **DONE 2026-07-13**
   — root-caused (mid-frame `GetBackBufferData` corrupts the rest of that frame on Vulkan) and
   worked around (`drawFrameIndex_`/`terrainPixelPrintedFrame_`); see §5 for the full writeup.

4. ~~Try to reproduce "grass-topped cubes walkable through" with a scripted test.~~ **DONE
   2026-07-13** — added a `VerifyBlupiMovement` check that drops Blupi onto all 9 icon-107/108/109
   blocks in `worlds3d/world001.vwr`; all 9 land correctly, not reproduced via scripted collision.
   See §5 for the updated conclusion; still open if a live repro with coordinates ever recurs.

5. ~~Fix `GEBlupiController::GroundHeightAt()`'s roofed-interior limitation~~ **DONE 2026-07-13** —
   see §3/§5 for the full writeup and verification (before/after `git stash` test against the real
   south tunnel + full regression on both backends).

6. ~~Add Init's semi-transparent gamer-slot background panels~~ **DONE 2026-07-13**
   (`plan.md MENU-014/015`) — `GEInputPad::DrawInit()` now draws a `pad.png` icon-15 panel (same
   convention as `GEHud`'s `DrawInfo` panel) behind each gamer row and behind the Setup/Play
   buttons. Drawn at opacity 1.0, not true transparency, per the same CNA/Vulkan `BasicEffect`
   Alpha<1 workaround already used by `GEHud::kPanelOpacity` (§5). Verified via a live headless
   EasyGL screenshot (temporary `kWaitDurationSeconds` debug override to reach Init immediately,
   reverted before commit) + full 7-tool suite.

All of the above are now done except #1 (Saw blade), which is blocked on the user's own visual
judgment (§9).

### Next round (identified 2026-07-13, after reconciling `plan.md` against source)

7. ~~`ObjectType52` bridge construction~~ **DONE 2026-07-13** — see §3 for the full writeup.

8. **`ObjectType21` secret-level exit** (`plan.md PICKUP-009`) — only a render-icon lookup exists,
   no contact/trigger logic. Real behavior is simple (identical to the existing `ObjectType7`
   exit-goal contact logic, plus setting a `m_bFoundCle`-equivalent flag), but that flag currently
   has no consumer (door-open-on-win isn't modeled, `PICKUP-038`/`039`) — low priority until door
   persistence exists; implementing it now would add code with no observable effect. Files:
   `src/GalaxyEggbertCNA/Game/GEInteractionSystem.cpp` (mirror the existing `ObjectType7` case).

9. **`AscenseurVertigo`** (`plan.md PICKUP-024`) — edge-hang on wide/shiftable platforms (icons
   311-316); deliberately deferred pending a render/icon-selection decision, not a gameplay-logic
   gap. Needs that decision made first (see plan.md `153`), not a blind implementation attempt.

10. ~~Re-audit sections 2.1-2.5/2.8/2.10-2.13 of `plan.md`~~ **DONE 2026-07-14** — 2.4/2.8/2.10/2.12
    confirmed NOT stale (correctly all `[ ]`, matching reality); 2.1/2.5/2.11/2.13 WERE badly stale
    (24 items flipped `[ ]`→`[x]`, most notably §5.3's hazard-tile subsection, whose own intro
    claimed "none of these have hazard logic wired" when Phase 14 had implemented almost all of it,
    and §11 Save Data, which claimed "not started at all" when `GESaveData` is real and working).

### Genuinely open gaps (confirmed via direct source grep, not stale-doc negatives — from the
### 2026-07-14 audit above; ordered roughly by value/effort)

11. ~~`ctest`/CI integration~~ **DONE 2026-07-14** (`plan.md TEST-002`) — `GalaxyEggbertWorldsTests`
    turned out to already be `gtest_discover_tests()`-registered and ctest-runnable (a sibling
    dependency's own CMakeLists.txt already calls `enable_testing()` transitively); the real gap was
    the 6 `VerifyXxx` binaries having no `add_test()` at all. Added one for each in `CMakeLists.txt`
    (`WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` for the 3 that default to repo-root-relative paths).
    `ctest --test-dir build-cna` now runs all 7 tools in one command — confirmed on both backends
    (75 tests total including third-party deps; 1 pre-existing, unrelated `../easy-gl` smoke-test
    failure noted below, not a regression from this change).

12. ~~Jump-height headroom modulation~~ **DONE 2026-07-14** (`plan.md TILE-041`) — see §3 for the
    full writeup. `GEBlupiController::HasJumpHeadroom()` + 3 new proportionally-anchored jump-speed
    constants; verified with an exact expected-velocity match (not just pass/fail) against the real
    south tunnel's low ceiling, full regression on both backends.

13. ~~Suspended/hanging bar-and-rope movement mode~~ **DONE 2026-07-14** (`plan.md TILE-045`) — see
    §3 for the full writeup. Icon 202 (also a real trigger icon) now has its own new render
    geometry too (`plan.md TILE-055`, "thin-bar", done 2026-07-14) — the demo world now uses both
    icon 138 and icon 202, each in its own row.

14. ~~Water-drip tile~~ **DONE 2026-07-14** (`plan.md TILE-032`) — see §3 for the full writeup. Real
    behavior was NOT a glu/slow effect (a wrong premise, same category as TILE-041/045) — it's a
    deterministic kill, mechanically identical to Spike. Now a real 6th confirmed terrain hazard.

15. ~~Camera shake system~~ **DONE 2026-07-14** (`plan.md CAM-008..013`) — see §3 for the full
    writeup. Was blocked on `table_decor_action`'s data-table transcription approval; the user
    granted blanket approval the same day. SmallShake's own real trigger sites (dynamite-blast
    destroyed objects, CleanAll cheat) are a real, scoped follow-up, not wired yet — the core
    mechanism (real table, real ×3 multiplier, real self-clear timing) and 2 of 3 shake types
    (BigShake/Fan, ElectricShake/wasp) are done and camera-wired.

16. **Particle/transient-visual-effects system** (`plan.md` §2.7 §7.5 / §2.12, ~22 items) — the
    entire system (explosions, sparkles, splashes, bursts) is genuinely unbuilt; explicitly the
    single largest remaining checklist section by item count. A real feature, not a quick fix —
    scope as its own multi-task effort if picked up, not a "next smallest task." **Update
    2026-07-14: the user directed a start on this system and the first slice (Invert start/stop
    burst, `BLUPI-110`/`VISUAL-014/015`) is now done** — see §3's own writeup, including the
    real `SearchDistRight()` short-circuit finding that makes several OTHER effects in this family
    (types 36/39/93) similarly simple (no raycast needed), a reusable discovery for whichever
    effect is picked up next. ~20 items remain.

**Status as of 2026-07-14 (updated): #13 is now done** (see §3) — implemented the same session this
note was first written, after concluding the icon-ID research had actually de-risked it enough to
attempt directly rather than defer. **Everything else still needs either a human decision or its
own dedicated multi-task session:** #8 (blocked on door persistence, low value even if done), #9
(blocked on a render decision), #15 (blocked on a data-table transcription approval decision), #16
(explicitly its own multi-task effort, the single largest remaining checklist section). A future
session should either get one of these blockers resolved by the user, or deliberately scope and
commit to implementing #16 as its own focused effort rather than an opportunistic pass.

**Further update, 2026-07-14 (later the same day):** shipped `TILE-055` (icon-202 thin-bar render
geometry, the follow-up to #13/TILE-045) — see §3. Then did a thorough round of candidate-hunting
for the next small opportunistic task and closed out several dead ends with source-verified
corrections (all pushed): Cloud's `BLUPI-108` line was stale (real effect already shipped as
`BlupiElectro`); `BLUPI-111` Ghost-mode cheat is real but has no player-facing trigger anywhere in
real source; `BLUPI-126/127/131` "stomp kill" is an invented Mario-style mechanic with zero source
support; a follow-up research fork's own `ENEMY-CONTACT-001` claim ("no enemy contact detection
exists") was itself checked and found WRONG — retracted, since `GEInteractionSystem.cpp` already
implements comprehensive contact handling for every real enemy case; and the entire particle/
visual-effects backlog (§7.5, `VISUAL-001..007`) is either data-table-blocked or has no confirmed
real source at all (likely invented, same category as SCORE). **No further small, safe,
unblocked, real task was found this pass.** Every remaining checklist item needs either explicit
user approval (data-table transcription for particle effects/camera shake) or the user's own
decision (Ghost-mode trigger, AscenseurVertigo's render choice, Saw blade orientation, or
committing to #16 as its own dedicated multi-task effort). A future session should check with the
user on one of these before continuing, or re-run this same discovery process in case a fresh look
turns up something this pass missed.

**Further update, 2026-07-14 (same day, later): the user resolved most of the open blockers
directly.** (1) Data-table transcription is now approved — camera shake (#15) and the
data-table-backed particle-effects items in #16 are unblocked; still needs its own scoped
implementation pass (particle effects remain a genuinely large feature, camera shake is small).
(2) `BLUPI-108`/`BLUPI-126/127/131`/`SCORE-001..007`/`VISUAL-001..007` confirmed HALLUCINATED —
explicitly cancelled by the user, see `plan.md`'s per-item corrections and §9's "do not do yet"
note above. (3) Ghost-mode's real trigger was provided directly by the user: typed word "Ghost",
via a second (word-typing) cheat-entry method distinct from the on-screen button dispatch this
session found earlier — needs source research to find the exact mechanism before implementing,
in progress. Remaining open: #8 (door persistence, still low value), #9 (AscenseurVertigo render
decision), Saw blade orientation (needs visual input).

**Further update, 2026-07-14 (same day, later still): Ghost mode (#3 above) and camera shake (#15,
`plan.md CAM-008..013`) are both now DONE** — see §3 for both writeups. Camera shake's core
mechanism (real `table_decor_action`, verbatim-transcribed and byte-verified) and 2 of 3 shake
types (BigShake/Fan, ElectricShake/wasp) are wired; SmallShake's own dynamite-blast/CleanAll
trigger sites are a real, scoped follow-up (need a new signal path from `GEInteractionSystem` to
the camera, which that class has no concept of today). Remaining genuinely open: #8 (door
persistence), #9 (AscenseurVertigo render decision), Saw blade orientation (needs visual input),
and the particle-effects system (#16, real but large — a future session should scope it as its
own dedicated effort, same conclusion as before).

**Further update, 2026-07-14 (same day, even later): SmallShake's own trigger sites are now wired
too** (see §3's own writeup) — camera shake (#15) is now fully done, all 3 shake types. Remaining
genuinely open is unchanged from the note just above: #8, #9, Saw blade orientation, #16.

## 9. Do not do yet

- **Do not guess at the Saw blade orientation a 4th time without the user's own visual input** —
  3 prior autonomous attempts were wrong; this specifically needs human judgment.
- **Do not build, fix, or troubleshoot `GalaxyEggbertSimple3D`** — historical reference only, per
  explicit user directive (2026-07-08).
- **Do not modify `../mobile-eggbert`**, not even temporarily — copy files out first if a working
  copy is genuinely needed.
- **Do not copy mobile-eggbert code/data (tables, enums, byte layouts) into galaxy-eggbert**
  without explicit user approval, even when it looks like "just data."
- **Do not invent new gameplay mechanics.** Every feature must trace to something confirmed in
  mobile-eggbert — verify against real source or `mobile-eggbert-reference/` before implementing,
  not just plausibility.
- **Data-table transcription is now APPROVED (user, 2026-07-14)** — the user gave explicit
  blanket approval to copy small real mobile-eggbert data tables into galaxy-eggbert. This
  unblocked (and, for camera shake, has now shipped — see `plan.md CAM-008..013`/§3) the
  particle/visual-effects items in `plan.md` §7.5 that have a real confirmed mechanic (Invert
  start/stop burst, Goo, water plouf/bubble/small-plouf, pollution puff, etc. —
  `table_invertstart`/`table_invertstop`/`table_glu`/`table_plouf`/`table_blup`/`table_tiplouf`/
  `table_pollution`) and camera shake (`table_decor_action`, done). Still verify each mechanic
  against real source before implementing (approval covers copying real data, not inventing
  behavior) — this does NOT retroactively make the confirmed-hallucinated items below real.
- **`plan.md` BLUPI-108 (Cloud "floats through blocks"), BLUPI-126/127/131 ("stomp kill"),
  SCORE-001..007/the score parts of SCORE-012, and VISUAL-001/002/003/004/005/006/007 are
  HALLUCINATED and CANCELLED (confirmed by the user, 2026-07-14)** — none of these have any real
  mobile-eggbert source; do not implement any of them under any item number, and do not revisit
  unless the user explicitly asks again. See each item's own `plan.md` correction for the research
  citations.
- **`plan.md` BLUPI-111 (Ghost mode cheat) is DONE (2026-07-14)** — real trigger provided directly
  by the user (typed word "ghost", the second of two real cheat-entry methods in mobile-eggbert)
  and fully implemented: `GEInputPad::UpdateTypedGhostCheat()` + `GEBlupiController`'s
  `ToggleGhost()`/`IsGhost()`/free-flight `Step()` branch + `GEInteractionSystem` interaction
  suppression via a sentinel Blupi position. See §3 for the full writeup. The other ~25 real
  typed cheat names are a real, still-open port opportunity (each needs its own verification pass
  before wiring, same discipline as this one) — not implemented yet, not blocked either.
- **Do not modify `../cna` or `../simple-3d`** without explicit per-change approval (the exit-code-1
  and Vulkan SpriteBatch-ordering bugs in §5 both need this and were explicitly deferred).
- **Do not attempt the exit-code-1-on-window-close fix** — the user already explicitly declined it.
- **Do not design or build the real 3D Blupi model** — blocked on the user providing an asset;
  don't speculate about its look/rig in the meantime.
- **Do not broadly refactor `GEInputPad`/`GEInteractionSystem`/`GEBlupiController`** — they are
  large, working, and have many call sites; prefer small, targeted, well-tested additions over
  restructuring.
- **Do not re-inflate this file back into a multi-thousand-line narrative.** Keep §3 to the current
  session plus a one-paragraph "earlier, still true" summary; rely on `git log`/`plan.md` for full
  historical detail.

## 10. Resume prompt

```
Read NEXT.md first (this file). Pick the FIRST unstarted item in §8 "Next smallest tasks" (or the
one the user names) and inspect only the files it lists — do not read or refactor unrelated code.
Make one small, verified improvement: implement it, run the verification command the task
specifies, and confirm it actually passes (don't just assume). Do not touch `GalaxyEggbertSimple3D`,
`../mobile-eggbert`, `../cna`, or `../simple-3d` unless the task explicitly requires it and you have
explicit approval. When done, update NEXT.md: move the completed task out of §8, add one factual
bullet to the top of §3 (dated), and update §2 if the change affects build/test status. Keep the
update concise — do not restore removed historical detail.
```
