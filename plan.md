# Galaxy Eggbert — Plan

**Galaxy Eggbert is a faithful 3D remake of mobile-eggbert. Nothing more.** Before adding any
task to this plan, verify the feature exists in mobile-eggbert. Do not add invented mechanics
(coins, time bonuses, coyote time, combo multipliers, etc.) — see `CLAUDE.md` for the full rule
and current examples.

Mission numbering (mobile-eggbert data, still accurate): 1 = intro hub, 10 = world1 hub,
11-19 = world1 levels, 20 = world2 hub, 21-29 = world2 levels, ... 78 worlds total.

Legend: `[x]` done · `[ ]` todo · `[~]` partial · `[?]` requires user decision.

This document was substantially rewritten 2026-07-10. The previous version planned around the
old Simple3D → U3D/Urho3D/Nova3D direction, which is now dead (superseded 2026-07-05, hardened
2026-07-08 — see `CLAUDE.md` "Current Direction Lock"). All Simple3D/U3D/Nova3D-specific content
(the old `S3D-*` milestone section, Android/Nova3D build tasks, and ~940 lines of a
documentation-regeneration bug-fix log) has been dropped from this file; it remains available in
git history (any commit before this rewrite) if ever needed. The mobile-eggbert feature-parity
checklist (menu/HUD/Blupi/tiles/enemies/pickups/score/sound/camera/save/visual — formerly scored
against Simple3D's implementation) has been carried forward but had every status mark **reset**
against `GalaxyEggbertCNA` specifically, since Simple3D's status has no bearing on CNA's — see
`## 2` below.

---

## 0. Current Status Snapshot (2026-07-10)

`GalaxyEggbertCNA` is the sole actively-built/maintained target. Full detail lives in `NEXT.md`
(updated continuously); this is a compact summary for planning purposes.

### Working

- **Terrain**: all 4 confirmed tile render modes (`DirectionalCube`, `InnerPillarBox`,
  `InnerFlatPlate`, `TripleCrossBillboard`) + water (alpha-blended `UniformCube`), ~175/175
  confirmed icons wired, real per-type animation timing (`Decor.cpp Config::ScaleDiv()`
  divisors — NOT a uniform rate), face culling on the static-opaque path (66% vertex reduction
  on the sample world).
- **Objects**: `MoveObject`/`BigDecor` render as real textured billboards, animated via real
  per-instance phase timers; platform lifts and crates (`ObjectType1/12/47/48`) render as solid
  `UniformCube`s instead, per the confirmed two exceptions to the billboard default. Embeddable
  directly in the 3D `.vwr` format, not just parsed from mobile-eggbert `.txt`.
- **Interactive objects** (`GEInteractionSystem`, 2026-07-10): platform lift ping-pong patrol;
  crate push (X-axis only, single-crate, real adjacency/lane/floor-support/occupancy checks);
  treasure/egg/key/level-exit pickup collection with real removal-on-contact semantics, real
  sound channels, `MAX_EGG_COUNT=10` cap, exit gated on treasures-collected.
- **Lives + fall death + lava + spikes + Blitz + saw + generic enemy contact**
  (`E3D-MIG-130`/`067`/`140`/`141`/`144`/`142`/`132`, 2026-07-11): `lives_` starts at 3, eggs
  grant +1 up to the cap, `LoseLife()` resets to 3 on game-over (real `DoorsLost()` behavior, not
  a permanent depletion). Wired to fall-off-world death, lava, Blitz (all deterministic, real
  channel 8), spikes (real channel 51), saw (real channel 75, real switch-linking via
  `GEWorldRuntime::TryActivateSwitch()`), and generic `ObjectType2`/`3` patrol-hazard contact
  (real channel 74, the 50/50 death-sound coinflip simplified to always-play). Vehicle immunity/
  sub-tile x-band restrictions and the real 10-slot last-safe-position FIFO respawn are known
  simplifications. All 5 real terrain hazard tiles are now covered (only Crusher, below, is
  non-lethal) — only named-enemy contact (Phase 13) still calls nothing.
- **Crusher squash state** (`E3D-MIG-143`, 2026-07-11): non-lethal, unlike every hazard above —
  `GEBlupiController::TriggerCrush()`/`IsEcrased()` (reduced move speed, jump blocked, ~10s
  auto-recovery, real entry/recovery sound channels 70/41), gated on `GEWorldRuntime::
  IsCrusherActiveAtPhase()` (approximates the real 3-out-of-10 danger window).
- **Basic HUD** (2026-07-11): icon-based only (no text rendering exists) — life icons
  (bottom-left, one per life) and key icons (top-left, shown only while held).
- **Sound** (`GESound`, 2026-07-10): all 93 real channels load via CNA's own `SoundEffect` API,
  real per-channel volume/conflict table, wired to jump/land/footstep.
- **Camera**: first-person default + third-person (placeholder GPU-skinned model) toggle,
  framerate-independent damping.
- **World format**: real `.vwr` v2 format (multi-Y-layer, embeddable `MoveObject`s, `skyRegion`
  header field selecting a real background), plus a secondary `.txt`-parsing path for reference.
- **Verification**: `GalaxyEggbertWorldsTests` (63/63), `VerifyBlupiMovement`,
  `VerifyMoveObjectTypesCna`, `VerifyBigDecorParsingCna`, `VerifyInteractionSystem` — all
  scripted/non-interactive, all passing.

### Not yet working

- **No visible Blupi** — collision-only point in first-person (a temporary 2D sprite HUD
  indicator stands in); third-person has only a placeholder Fox model. Billboard rendering was
  explicitly rejected 2026-07-10 (a billboard always faces the camera, which would look wrong for
  the fixed-angle `blupi.png` sprite under a free third-person orbit); a real 3D model
  (`E3D-MIG-069`) is now the only planned path to a visible Blupi, third-person only.
- **HUD is minimal** — icon-based only (life icons bottom-left, key icons top-left when held),
  no text rendering exists (no `text.png` glyph layout identified yet), so no numeric treasure
  counter/score/timer.
- **Enemy combat covers the real shared kill list, the wasp's balloon status, blupih/blupit's
  projectile attacks, and the large creature's turn-dwell-gated grab** — contact with
  `ObjectType` 2/3/4/16/17/20/96/97 kills Blupi (`E3D-MIG-132`, widened 2026-07-11 from just 2/3
  after finding they're all one real shared check in `Decor.cpp`); wasp (44,
  `E3D-MIG-135`, 2026-07-11) transforms him into a non-lethal "balloon" status instead
  (`GEBlupiController::TriggerBalloon()`/`IsBallooned()`/`PopBalloon()`), which in turn changes
  how exactly 4 of those 8 shared-kill types (`3`/`16`/`96`/`97`) behave — they pop the balloon
  instead of killing while it's active. Every `MoveObject` (except lifts/crates) now genuinely
  patrols using the real shared 4-phase dwell/advance/dwell/recede cycle (`E3D-MIG-131`,
  2026-07-11, `AdvancePatrolStep()`) instead of sitting frozen at `posStart` — the real
  prerequisite blupih/blupit (`E3D-MIG-134`, 2026-07-11) and the large creature (`E3D-MIG-136`,
  2026-07-11) needed for their own dwell-frame-timed attacks/lethality windows, both now done:
  blupih drops a projectile straight down, blupit fires two horizontal shots, both only during
  turn-dwell; the large creature grabs Blupi (fatal, real balloon immunity modeled, never
  destroyed itself) only during its own turn-dwell, safe to touch mid-walk. Follower 96/97's real
  dormant-until-a-padded-wake-box, then 1px/tick homing-toward-Blupi movement (`E3D-MIG-137`,
  2026-07-11) is also done, completing Phase 13 — contact-kill/pop already worked, and a dormant
  follower now genuinely wakes and chases, self-destructing if its path is ever blocked rather
  than passing through solid terrain. All
  5 real terrain hazard tiles (`BlockTypes::isHazard()`'s own bucket) are implemented
  (`E3D-MIG-140`-`144`, 2026-07-11) — 4 lethal (lava, spikes, Blitz, saw) via the lives
  foundation (`E3D-MIG-130`, alongside separately-implemented fall-off-world death),
  Crusher non-lethal (a squash state). **The spring/bounce tile is also done** (`E3D-MIG-145`,
  2026-07-11) — the first Phase 14 mechanic that isn't a hazard: `GEBlupiController::
  TriggerSpringBounce()` launches Blupi upward (one of two real magnitudes depending on whether
  Jump is held on contact) instead of costing a life. **The Temp/vanishing tile is also done**
  (`E3D-MIG-146`, 2026-07-11) — solid 90% of the time, passable (Blupi falls through) the other
  10% on a real 20-value phase cycle; unlike every other mechanic here, it's threaded directly
  into `GEBlupiController::Step()`'s own collision scan (a new `tempPassable` parameter) rather
  than checked post-hoc, since it changes whether the tile IS the ground at all. **The teleporter
  is also done** (`E3D-MIG-147`, 2026-07-11, redesigned same day per live-playtest user feedback)
  — `TriggerTeleport()` freezes Blupi for a real 6.4s, then `GEWorldRuntime::
  FindTeleportDestination()` relocates him to the paired pillar elsewhere in the grid (or leaves
  him in place if no partner exists, matching real behavior). Detection matches the real "one
  tile above Blupi" geometry exactly (`GetBlockTypeAbove()`) — teleporter icons are always
  non-solid for collision (`GroundHeightAt`'s own exclusion), so Blupi genuinely walks into the
  open space beneath a floating pillar, reproducing real mobile-eggbert's per-tile-independent 2D
  collision for this one purpose. (An earlier same-day attempt made the pillar solid and detected
  the cell Blupi was FACING instead, worked around rather than solved the real collision mismatch,
  and shipped a real bug — Blupi froze permanently and was never relocated, caught only by live
  playtesting, not the unit tests written for that design — since fixed and re-verified live.)
  **Its render geometry is also done** (same task, 2026-07-11): the pillar itself is a
  `DirectionalCube` (`GEDirectionalCubeTiles.cpp`, textured on all 4 sides, flat top/bottom —
  supersedes an earlier questionnaire pass' "Billboard" call, revised live per direct user
  description of the real crop: `mobile-eggbert-reference/02-tiles.md`/
  `questionnaire-all-remaining-tiles.md` both updated), plus a "tip" attachment hanging below the
  pillar, textured with the tile's own lower ~2/3 (`GETerrainRenderer.cpp`'s
  `IsTeleporterTipIcon()`/`TeleporterTipUv()`), wired into `AppendSpecialGeometry()` right after
  the `DirectionalCube` append. **Revised twice more the same day after live user re-checks**:
  (1) the tip was first built as a tapering pyramid (`Easy3D::PyramidTipItem`/
  `AppendPyramidTipMesh()`) — a live screenshot showed solid black filling each triangle's wide
  base, and the user clarified the real shape is a "čtyřhranol" (four-sided/rectangular prism), so
  it was rebuilt as a second, narrower `DirectionalCubeItem` box (4 straight side faces, no
  tapering) instead — `PyramidTipItem`/`AppendPyramidTipMesh()` removed entirely from `../easy-3d`
  (hpp/cpp/test), nothing else used it; (2) a direct pixel/alpha-channel inspection of
  `object-m.png` (small script sampling specific pixels) found the "black" was actually genuine
  alpha=0 transparency around the real teal cone graphic, not painted black — the box shape alone
  didn't fix this, since the CUBE'S OWN side faces use the same texture and were equally affected;
  fixed by adding Teleport1-4 to `GETerrainRenderer.cpp`'s `NeedsAlphaBlend()` (previously only
  icons 30/31), routing the whole icon through the existing alpha-blended transparent-static
  render path instead of the opaque one. Verified live via headless EasyGL screenshots at multiple
  angles/distances — renders correctly: blue cube sides with the real dots/emblem-letter texture,
  a clean genuinely-transparent teal spike hanging below with no black anywhere.
- **No riding a moving platform** — `GEBlupiController`'s collision only tests the static
  terrain grid, not `MobileObjSpec` objects.
- **No linked-crate stacks** — crate push is single-crate only.
- **No save/progression system** at all.
- Every pickup/vehicle/buff type beyond treasure/egg/keys/exit (helicopter, jeep, skateboard,
  tank, overcraft, balloon, shield, suction-cup, drink, charge/cloud, mirror, dynamite, bullets).
- `ThinMechanical` tile geometry (saws, springs, switches, fans, bridge, pipes, grates — ~25
  icons) — geometry not decided. Distinct water/liquid surface treatment. New "thin-bar"
  geometry (icon 202). Architectural kit modular assembly (~6 icons).
- Secret powers (`Sp0`-`Sp7`, icons 158-165) — not rendered, and behavior is undocumented
  anywhere yet (open research question, see `## 4`).

### Known open bugs

- **Texture distance washout** — real texture degrades to flat gray at ~12 units distance.
  Winding/mip-bleed/fog ruled out. Root cause not found; trail leads into `../cna`'s EasyGL
  backend, deliberately left there (another agent's active work area). Written up in
  `texture-distance-washout-bug.md`.
- **Exit code 1 on real window-manager close** — 100% reproducible `X Error: BadWindow` from
  SDL's own X11 teardown, root cause inside `../cna`/SDL. User explicitly declined a fix
  (2026-07-09); accepted as a known limitation, do not attempt without new approval.
- **Residual seam-line transparency** — 60%-mitigated (UV atlas half-texel inset), not fully
  eliminated. Two untested hypotheses (MSAA edge AA, inset too small at extreme angles).
- **Grass-topped cubes reported walkable-through** (no collision) — reported live, not yet
  reproduced (no specific coordinates given).

---

## 1. Architecture — Direct CNA + Easy3D

```text
Galaxy Eggbert
  -> CNA directly
  -> Easy3D beside CNA (small helpers only — cameras, texture atlas, billboard/cube batching)
  -> mobile-eggbert used read-only as reference / asset / data source
```

Easy3D does not hide CNA and must not grow into a scene graph/ECS/engine (see `easy3d.md` §7).
mobile-eggbert is read-only; `GalaxyEggbertSimple3D` stays intact but historical
(never build/fix it as of 2026-07-08). Full rules: `CLAUDE.md` "Current Direction Lock".

### Phases 0–5B: done (2026-07-01 through 2026-07-09)

All of documentation/direction-lock (`E3D-MIG-000`-`005`), CNA target skeleton
(`E3D-MIG-NEXT-001`-`006`), repository integration investigation (`E3D-MIG-010`-`014`; `015`
`[?]` still open — asking mobile-eggbert maintainers for a future shared library target),
target skeleton (`E3D-MIG-020`-`026`), asset reuse via build-time copy
(`E3D-MIG-030`-`039`), world loading (`E3D-MIG-040`-`043`), the Easy3D terrain path
(`E3D-MIG-050`-`055`, `058`, `059`), and tile-render-mode completion / object rendering / face
culling (`E3D-MIG-500`-`509`) are complete. Full task-by-task detail: git history (this file,
pre-2026-07-10 rewrite) or `NEXT.md`'s dated log.

Standing rules from this era, still in force: no MeshCraft/mesh-import path
(`E3D-MIG-056`); chunk-radius streaming still not implemented, scheduled not deferred
(`E3D-MIG-057`, face culling is a partial substitute).

### Phase 5C — Remaining tile geometry decisions (`E3D-MIG-510`-`516`)

- [ ] `510` `[?]` Decide `ThinMechanical` render geometry for the ~25 icons that are neither
      bulk cubes nor simple planes: saws, springs, switches, fans (bodies, not the already-solved
      head icons), bridge segments, pipes, grates. See `mobile-eggbert-reference/15-3d-render-mapping-design.md`
      §10.3 for the itemized list.
- [ ] `511` Implement `ThinMechanical` once `510` is decided.
- [ ] `512` `[?]` Decide water/liquid surface treatment distinct from the current alpha-blended
      cube (real behavior: flat surface with wavy edge; water is the single most common tile
      family, 25-38/78 real files).
- [ ] `513` Implement the "thin-bar" geometry for icon 202 (thin rectangular prism, walkable top,
      4 long faces textured, blue end-caps) — a genuinely new shape, not a variant of an existing
      mode.
- [ ] `514` `[?]` Decide architectural-kit assembly approach for icons 391-395/397/400
      (arch/window/door-jamb fragments) — likely needs coordinated multi-block placement logic,
      not a single per-icon render mode.
- [ ] `515` Render secret powers (`Sp0`-`Sp7`, icons 158-165) — gold-pedestal `Billboard`, per
      the existing render-mode default. Blocked on `E3D-MIG-190` (behavior research) for
      anything beyond static rendering.
- [ ] `516` Render closed doors (icons 334-336) as `Billboard` (red pillar/bollard shape) — NOT
      `UniformCube`, corrects an earlier wrong assumption in `15-3d-render-mapping-design.md`
      §9.1. Full door behavior is `E3D-MIG-160`.

### Phase 6 — Blupi, visible and complete (`E3D-MIG-060`-`069`)

- [x] `060` Collision-only movement, step-up traversal, gravity (`GEBlupiController`) — an
      engine-appropriate 3D grid system, explicitly NOT a transcription of the 2D
      `BlupiRect`/`BlupiAdjust`/`BlupiBloque` system. Verified via `VerifyBlupiMovement`.
- [x] `061`/`062` `Easy3D::BillboardBatch`/`BillboardMeshRenderer` exist, used for
      `MoveObject`/`BigDecor` billboards.
- [x] `063` **Rejected 2026-07-10, decided by user**: do NOT render Blupi himself as a billboard.
      `blupi.png`/`blupi1.png` only have left/right side-view frames drawn for one fixed viewing
      angle; a billboard always rotates to face the camera, so under a free-orbiting third-person
      camera Blupi would visibly always "face the player" regardless of his real movement
      direction or the camera's actual angle — the same defect already flagged for enemy
      billboards at `E3D-MIG-179`, just worse here because the player directly controls the
      camera around Blupi. First-person (the default camera) never sees Blupi's own model at all,
      so no billboard is needed there either. Superseded by `069` (real 3D model, third-person
      only) — until that exists, third-person keeps its placeholder Fox model, and first-person
      stays invisible-collision-point-only. Do not revisit without a new user decision.
- [ ] `064` Blupi animation-state machine (`AnimState` → real state timing) via `table_blupi` —
      re-scoped 2026-07-10: no longer about selecting 2D sprite frames (billboard rejected, see
      `063`), instead about driving a real 3D model's animation clips once `069` exists. Still
      needs explicit user approval to transcribe `table_blupi`'s timing/state data per
      `easy3d.md` §5.4/§5.7 (load-bearing data, not casual "just data"). Blocked on `069`.
      **Partial, narrower progress (2026-07-11)**: the separate bottom-right 2D debug animation
      indicator (not this task's real 3D model target, but the same `AnimState` enum/frame-table
      mechanism) gained a `Jump`/`Air` split (real `BlupiAction` IDs 4/5) — `GEBlupiController`
      previously collapsed all airborne time into `Jump`. Frame data for `Air` (`{169, 26, 170,
      170, 27}`) was NOT a fresh `table_blupi` transcription — it was ported from
      `GalaxyEggbertSimple3D::GEBlupiController.cpp`'s own `kAirFrames`, already shipped/approved
      in this same repo, so this one addition didn't need new approval. The state split itself
      uses velocity sign (ascending vs falling) rather than Simple3D's fixed 3-frame trigger
      window, a natural adaptation to this class's continuous-velocity physics. Verified via new
      `tools/VerifyBlupiMovement.cpp` assertions (Jump→Air transition, exact icon values 17/169).
      **User approved the further transcription (2026-07-11)**: `StopEcrase`(72)/`MarchEcrase`(73)/
      `Balloon`(66)/`Teleporte`(74)'s real `table_blupi` icon-frame records were parsed directly
      out of `../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Tables.cpp` (a small script, not by
      hand) and added as `kStopEcraseFrames`/`kMarchEcraseFrames`/`kBalloonFrames`/
      `kTeleportingFrames` in `GEBlupiController.cpp` — frame counts (1/24/16/128, 67 of the 128
      Teleporte frames being the real `-1` "invisible" sentinel) cross-checked exactly against
      `mobile-eggbert-reference/08-animations.md` §2's already-documented counts. `UpdateAnim()`'s
      precedence now checks `m_teleporting`/`m_balloon`/`m_ecrase` BEFORE the ground/air cascade
      (real `BlupiAction` has only one animation per status regardless of grounded/airborne, no
      "AirEcrase" etc.); `StopEcrase` vs `MarchEcrase` still splits on the same `moving` bool as
      `Stop`/`March`. **Found and fixed a real bug while wiring `Teleporting` in**: `Step()`'s
      early-return for `m_teleporting` skipped `UpdateAnim()` entirely, so the new state would
      never actually have displayed during a real teleport — now calls `UpdateAnim()` before that
      return. `GetAnimIcon()`'s `-1`→icon-0 substitution for Teleporte's invisible frames is a
      documented simplification (this debug HUD slot has no "draw nothing" mechanism), not a
      fidelity claim. New `VerifyBlupiMovement.cpp` assertions cover all 3 states' exact icon
      values; full suite + both backends re-verified.
- [ ] `065` Real jump/gravity constants matching mobile-eggbert's tick-domain values (gravity
      +2.0/tick to terminal 20.0, displacement = 2×velocity; jump launch values by
      Jump-held×Power combo; ledge-walk-off has no boost) — rescale from 20Hz tick-domain to
      CNA's real framerate. Current `GEBlupiController` constants are an independent
      engine-appropriate approximation, not yet cross-checked against these real values.
- [ ] `066` Turning-duration-per-mode table (Normal/Air 6 ticks, Overcraft/Jeep 7, Helicopter/
      Swim/Surf/Suspended 10, Tank 12, Skateboard 14).
- [~] `067` Death/respawn: **fall-off-world case done** (2026-07-11, real Clear2 case — no
      per-tile-type work needed, checked before hazard tiles per the real source too) —
      `GalaxyEggbertCnaGame::Update()` triggers `LoseLife()` + channel 8 sound + fixed-point
      respawn when Y drops below a threshold. **Real bug found and fixed (2026-07-11, user-
      reported via live playtest, same feedback batch as the teleporter fix)**: this check was
      unreachable via normal walking. Root cause: `GEBlupiController::GroundHeightAt()`'s "no
      solid block anywhere in this column" fallback returned `0`, silently treated as solid
      ground at Y=0 by both its callers (the main landing check and `TryMoveAxis`'s step-up
      gate) — confirmed live (temporary debug instrumentation, reverted before committing):
      dropped from Y=20 over a genuinely floorless column, Blupi fell to exactly Y=0.000 and
      stopped (`onGround=true`) instead of continuing to fall. Fixed by introducing an explicit
      `kNoGround` sentinel (-1, unambiguously distinct from every real returned height, which is
      always >= 1) that the landing check now explicitly excludes from the clamp condition —
      `TryMoveAxis`'s step-up gate needed no change, since `-1 <= m_y + kStepLimit` and
      `-1 > m_y` already evaluate correctly as "allow the move, don't snap up" without special-
      casing. Re-verified live after the fix: Blupi now falls straight through (Y went negative,
      observed down to -8 during a 3s drop) and the existing fall-death channel 8/respawn fires
      correctly once he crosses the threshold. `tools/VerifyBlupiMovement.cpp`'s own "faller"
      test (added for `E3D-MIG-060`) had been drop-testing over a location with NO real floor the
      whole time, meaning it was unknowingly asserting on the buggy fallback behavior — moved to
      a location with a real floor to test genuine gravity/landing, and added a new, separate
      test (2 new assertions) that drops over a genuinely floorless column and asserts Blupi
      never lands and his Y goes negative — a direct regression test for this exact bug.
      **The real 10-slot "safe position" FIFO respawn is also done** (2026-07-11, same
      user-reported feedback batch): verified directly against `Decor.cpp:6467-6478`
      (`m_blupiValidPos` update gate) and `6654-6673` (`BlupiAddFifo`). New
      `GEBlupiController::UpdateSafePosition(bool externallySafe)`/`GetValidX/Y/Z()` — a real
      10-slot FIFO of recent positions, updated once per frame while Blupi is grounded, not
      ballooned/squashed, and the caller reports `externallySafe` (this class only knows its own
      grounded/balloon/ecrase state, not terrain hazard tiles or teleporter-trigger occupancy, so
      `GalaxyEggbertCnaGame::Update()` computes that from the same `GetGroundBlockType()`/
      `GetBlockTypeAbove()` checks already used for the hazard/teleporter logic — real vehicle/
      shield/ledge-teeter/transport-riding/projectile-path checks aren't modeled, same
      simplification as everywhere else). Sets the tracked valid position to the FIFO's OLDEST
      entry BEFORE pushing the current one (the real order, giving the "don't respawn exactly
      where you died" buffer), with the same dedup-consecutive-duplicates shape as the real FIFO
      but extended to all 3 axes (real mobile-eggbert's 2-axis dedup is a direct consequence of
      having no Z axis at all, not a deliberate 2-of-3 choice for a 3D engine). Every respawn call
      site (the shared `triggerDeath` lambda, and `GEInteractionSystem::DiedThisFrame()`'s
      handler) now uses `GetValidX/Y/Z()` instead of the fixed spawn point. Verified live (forced
      continuous forward movement + temporary debug logging, reverted before committing): the
      tracked valid position correctly lagged behind Blupi's live position while walking, and
      correctly stopped updating once he walked into a wall and stayed still (dedup working).
      Added 6 new `VerifyBlupiMovement` assertions (default-before-any-safe-frame, no-op while
      airborne, no-op when the caller reports unsafe, and the FIFO's lag/buffer behavior across
      13 distinct positions exceeding the 10-slot capacity). Still missing: the full
      hazard→action table (Lava→Clear3, Saw→Clear4, Blitz→Clear1, Fan→Clear1/Clear2 coinflip,
      Trap/Drip→Glu, each depends on its own hazard from Phase 14), fixed per-action animation
      durations, fall->1000px = instant fatal bypassing lives. **Fall-death TIMING is also now
      fixed (2026-07-11, same feedback batch)**: the old `kFallDeathY=-5.0f` gave a sub-1-second
      death, which the user correctly recalled as unfaithful (real mobile-eggbert gives a real,
      noticeable multi-second fall). Root-caused by directly inspecting a real level
      (`../mobile-eggbert/worlds/world001.txt`): its real terrain (`Decor:` grid) occupies only
      rows 0-21, leaving a deliberate ~77-row (~4928px) empty margin before the real row-99 death
      check (`(end.Y+30)/64 >= 99`, `Decor.cpp:2754`) ever fires — not a tight threshold. Also
      re-verified the real gravity constants directly (`Decor.cpp:2966-2968`): terminal fall
      velocity is actually 21, not literally 20 (the un-clamped `+=2.0` increment overshoots the
      `<20` gate by one step), i.e. 840 real px/sec (13.125 tiles/sec) at the 20Hz reference rate.
      Falling that observed real margin at these real constants takes ~6.1s. `kFallDeathY` moved
      to `-60.0f` (from `-5.0f`), reproducing a comparable ~6.2s fall using this engine's own
      already-tuned `kGravity=25`/`kFallLimit=-10` (not the real tick-domain values, which aren't
      cross-checked against this engine's own constants per `065`) — matched as an equivalent
      NUMBER OF SECONDS, not the same literal unit distance, since this world's own terrain
      (Y 0-13) is far shorter than a real level's. Verified live (temporary debug instrumentation,
      reverted before committing): death fired at 6.72s over a genuinely floorless column,
      matching the researched estimate closely.
- [ ] `068` Electric aura (`BlupiElectro`, Blupi's own offensive Power-Charge buff, destroys
      small enemies within 40px) — depends on secret-power research (`E3D-MIG-190`).
- [ ] `069` Real 3D Blupi model (third-person only) — now the sole path to a visible, faithful
      Blupi per the `063` decision, not merely optional polish. First-person intentionally stays
      without a visible model (matches the existing default camera; the player never sees it).
      No timeline set; the current placeholder Fox model remains the third-person stand-in until
      this is scoped.

### Phase 7 — Objects & decor rendering (`E3D-MIG-070`-`074`)

- [x] `070` Pickups/enemies render as billboards from real sprite sheets (`element.png`/
      `object-m.png`/`blupi1.png` per `ObjectType`, not a single shared sheet); platform-lift/
      crate `UniformCube` exception (`506`) folded in.
- [x] `071` `ObjectType` IDs used directly throughout, confirmed exact ID parity with
      mobile-eggbert (204 IDs).
- [ ] `072`/`073`/`074` Standing rules, not one-shot tasks: `Decor.cpp` is reference only (never
      link/copy); no invented mechanics, cross-check every behavior against
      `mobile-eggbert-reference/`; stop and ask before exposing internal `Decor` state.

### Phase 8 — Sound (`E3D-MIG-080`-`089`)

- [x] `080`/`081` `GESound` class, real `SoundEffect`/`SoundEffectInstance` CNA API, all 93 real
      `.wav` files load, real per-channel volume/conflict table (channel 10 exempted from the
      no-overlap rule, matching real behavior). Wired to jump/land/footstep.
- [~] `082` Channel-index parity confirmed exact; pitch intentionally NOT applied yet
      (unverified encoding/units).
- [ ] `083` `[?]` `Config::ScaleTime()`'s real scale factor is unresolved anywhere in the
      research — needed to correctly pace any tick-domain timing (this blocks precise footstep
      interval, buff-warning timing, etc. beyond current approximations).
- [ ] `084` `SoundEnviron()` terrain-specific footstep/head-bump remapping — 7 terrain-specific
      channel pairs (78-91) keyed by tile-icon range underfoot, replacing today's generic
      channels.
- [ ] `085` Idle "fidget" periodic sounds — channels 36/37/46-49/65 trigger on
      `m_blupiPhase % N`, not simple one-shot events; easy to miss in a naive port.
- [ ] `086` Buff activate/expire-warning channel pairs: Shield 42/43(warn@10), Power 44/45(w@20),
      Cloud 55/56(w@25)+58(pickup-start), Hide 57/62/63(w@20), Mirror 66/67(none). Depends on
      the buffs themselves (`E3D-MIG-190`).
- [ ] `087` Corrected pickup sound channels are already applied in `GEInteractionSystem`
      (treasure/key 11 or 19, egg 3) — extend the same rigor to every future pickup/enemy/hazard
      sound trigger rather than reusing `GESound`'s convenience shortcuts (`PlayCollect`/
      `PlayLife`) which were found to be imprecise.

### Phase 9 — HUD (`E3D-MIG-090`-`093`)

- [~] `090`-`093` Minimal lives/world/treasure HUD using CNA `SpriteBatch`. **Started 2026-07-11**:
      life icons + key icons (see `## 2.3 HUD-001/005/006/007`), reusing existing textures already
      loaded elsewhere (`blupi.png`, `element.png`), not `pad.png`/`jauge.png` yet. Avoid building
      a UI framework — a HUD is a handful of sprite draws keyed to game state, not a system.
      Treasure/world/score text still needs `pad.png`/`jauge.png` plus text rendering, not started.

### Phase 10 — Gameplay parity (`E3D-MIG-100`-`107`)

- [~] `100` Pickups: treasure/egg/exit/keys done (`GEInteractionSystem`, 2026-07-10); every
      other pickup type not started (see `E3D-MIG-170`s).
- [ ] `101` Hazard/kill detection — not started, see `E3D-MIG-140`s.
- [ ] `102` Enemy stomp + any associated score/counter — not started, see `E3D-MIG-130`s.
- [ ] `103` Respawn invincibility window after death — not started, depends on `E3D-MIG-067`.
- [x] `104` Exit-gate logic: gated on `treasuresCollected_ >= totalTreasures_`, win/reject sound
      channels, debounced to fire once per contact.
- [~] `105` Crate-push + platform-patrol: basic single-crate push and lift ping-pong patrol done;
      linked-crate stacks, boarding/riding, vertigo/shift-off, conveyor nudge not started — see
      `E3D-MIG-150`s.
- [ ] `106` `[?]` Save/load — scope still TBD (`easy3d.md` §12 Q7: byte-level format
      compatibility with mobile-eggbert saves is an open question, not a default). See
      `## 2 §11 SAVE-*` for the full conceptual checklist.
- [ ] `107` Standing rule: no new mechanics — every gameplay task in this plan must trace to a
      documented mobile-eggbert behavior.

### Phase 13 — Enemy AI & combat (`E3D-MIG-130`-`137`)

Nothing here is started. mobile-eggbert-reference/04-enemy-behavior.md and
/12-hazards-and-interactables.md are the source of truth; do not invent stomp/hit feel not
documented there.

- [x] `130` Lives/gauge/respawn **foundation** done (2026-07-11): `GEInteractionSystem::Lives()`/
      `LoseLife()`, default 3, +1 per egg up to `MAX_EGG_COUNT=10`, real reset-to-3-on-zero
      (`DoorsLost()`) behavior — verified via `VerifyInteractionSystem`. This only unblocks per-type
      enemy work (`131`-`137`) and hazards (Phase 14) to actually call `LoseLife()` — none of them
      do yet (still `[ ]` below), so enemy contact still does nothing.
- [~] `131` **Shared patrol-turn cycle done 2026-07-11** — verified directly against
      `Decor.cpp:8005-8141` (`Decor::MoveObjectStepLine`), not just the reference doc. New
      `AdvancePatrolStep()` in `GEInteractionSystem.cpp` implements the real 4-phase state
      machine exactly (dwell@`posStart` for `timeStopStartTicks` → advance to `posEnd` over
      `stepAdvanceTicks` → dwell@`posEnd` for `timeStopEndTicks` → recede over
      `stepRecedeTicks` → loop), using normalized linear interpolation (equivalent to the real
      integer-pixel math, can't drift) at the real 20Hz reference tick rate. Applies to every
      `MoveObject` except platform lifts/crates, which keep their existing separate speed-based
      ping-pong patrol untouched (a deliberate scope choice, not an oversight — lifts are
      already shipped/tested and the visual difference for a symmetric-timing ping-pong is
      likely small; revisit if that assumption turns out wrong). This is the real prerequisite
      `E3D-MIG-134`/`136` (blupih/blupit's dwell-frame-timed attacks, the large creature's
      turn-dwell-gated lethality) both need — neither was attempted in this same pass, but both
      are now unblocked. **Real breaking format change**: `MoveObjectRecord`'s binary payload
      grew from 29 to 45 bytes to carry the 4 new timing fields (defaults: 2s dwell/3s traversal,
      a placeholder for hand-authored worlds, not a transcribed real per-type constant — real
      values are level-authored per instance); `worlds3d/world001.vwr` regenerated. Direction-
      mirrored animation-table selection (which visual table is picked by `posStart.X >
      posEnd.X`) is NOT modeled — no directional walk/turn sprite tables exist for these types
      yet, only simple icon-cycling. Verified via a new `VerifyInteractionSystem` test (a
      synthetic patrol object with a symmetric ~4s cycle, sampled mid-dwell at each end for
      timing-forgiving checkpoints) and an extended `MoveObjectRecordTests` round-trip
      assertion.
- [~] `132` **Shared kill-list contact-kill done 2026-07-11** in `GEInteractionSystem`, **widened
      2026-07-11** beyond just types 2/3 (verified against `Decor.cpp:5782-5816` directly, not
      just the reference doc — that source block IS the real shared contact check for exactly 8
      types: `ObjectType2`/`3`/`4`(bulldozer)/`16`(spider)/`17`(fish)/`20`(bird)/`96`/`97`
      (follower, both dormant and awake) — the only real difference between them is a purely
      cosmetic bigger-screen-shake for 17/20, not a behavioral one, so `IsGenericHazard()`
      covers all 8 with one check). Touching any of them kills Blupi (`LoseLife()`) and destroys
      the hazard; real duck-immunity for type3 specifically modeled via a `blupiCrouching`
      parameter. Real death sound is a 50/50 coinflip between channel 74 and silence
      (`BlupiDead`'s own `Clear2`-branch-only `PlaySound`) — simplified to always channel 74, not
      modeled as a coinflip. NOT done: type3's top-half-only hitbox, type2's "thrown object"
      wider anticipation box/taunt-suppression, type17/20's bigger explosion effect (all
      cosmetic — contact radius is a plain sphere, same simplification as every pickup type
      above), and follower 96/97's real homing-toward-Blupi movement (a genuinely separate
      feature — an un-homing follower still correctly kills on contact). New
      `GEInteractionSystem::DiedThisFrame()` lets the caller apply respawn (the system itself has
      no access to `GEBlupiController`). Icon-cycling animation for all these types was already
      implemented earlier (billboard rendering, NEXT.md §3) — this task was only ever about the
      contact/kill behavior. A spider (`ObjectType16`) is now placed in `worlds3d/world001.vwr`'s
      south tunnel so this is genuinely playable, not just tested via a synthetic injection.
- [~] `133` Crawler/flyer types 16 (spider), 17 (fish), 20 (bird) — **contact-kill done**, folded
      into `E3D-MIG-132`'s widened shared kill list (2026-07-11). NOT done: spider's real 9-frame
      crawl (may already fall out of existing generic animation handling, not verified per-type),
      fish/bird's narrowed hitbox and bigger-explosion effect, bird's taunt-capability (spider/
      fish are not taunt-capable, per the reference doc — no taunt system exists at all yet, so
      this distinction is currently moot either way).
- [x] `134` **Stationary shooters done 2026-07-11** — verified directly against
      `Decor.cpp:8878-8969` (attack timing) and `7794-7869`/`8095-8098` (the real `ObjectStart`
      raycast/travel-distance encoding and a fired projectile's self-destruct-on-arrival special
      case). Blupih (32) drops one `ObjectType23` straight down at dwell-frame 21; blupit (33)
      fires two horizontal shots per turn-dwell, frame 3 away from the upcoming walk direction and
      frame 21 toward it (**correction vs. this file's own earlier prose summary**, which had the
      two frames backwards — re-verified twice against the exact source condition/speed-sign
      pair). New `SearchAirDistance()` in `GEInteractionSystem.cpp` is a real grid-cell raycast
      (one cell == one real 64px tile) reproducing `SearchDistRight`'s "count clear cells to the
      next wall" behavior, including the real "0 distance = cancelled, but the attack sound still
      plays anyway" nuance (`ObjectStart` returns a valid slot even on that path — its `!= -1`
      sound gate only ever checks for a free object-pool slot). `stepAdvanceTicks = 5 * dist` is a
      direct, non-approximated transcription of the real `ScaleTime(abs(speed*dist/64))` formula,
      not an invented pacing constant. Body contact is deliberately NOT lethal (not in
      `IsGenericHazard()`) — only the fired projectile is, always fatal on contact (real
      shield/hide/superblupi immunity gates NOT modeled, same simplification as every other hazard
      — none of those concepts exist in this engine yet). New `GEWorldRuntime::GetWorldMutable()`
      accessor added for test tooling (hand-carving a guaranteed-shape ledge-over-a-pit/walled
      corridor rather than depending on incidental terrain shape elsewhere). A real, playable
      blupih ("turret perch", 3x3 ledge with a notch over a 3-cell drop) and blupit ("sentry
      corridor", walled passage) were added to `worlds3d/world001.vwr` south of the tunnel — the
      blupih's `posEnd`-side dwell sits over solid ledge (a real "no room" cancellation, not a
      bug), so a full cycle shows both a live shot and a cancelled one. Verified via 10 new
      `VerifyInteractionSystem` assertions (vertical raycast distance, contact-kill, "no room"
      cancellation, both horizontal shots' distances and directions).
- [x] `135` **Type 44 (wasp) done 2026-07-11** — verified directly against `Decor.cpp:5826-5863`
      (trigger) and `5766-5781` (hazard-pop interaction), not just the reference doc. New
      `GEBlupiController::TriggerBalloon()`/`IsBallooned()`/`PopBalloon()` (real
      `!m_blupiBalloon` re-trigger guard, real ~10s duration — same `m_blupiTimeShield=100`/
      decrement-every-`ScaleTime(2)`-ticks pattern as Crusher, NOT literally "100 ticks" —
      reduced gravity while active, an approximation of "floats rather than dying" since the
      real source doesn't cleanly transcribe to a specific fall-speed constant). Contact does
      NOT kill Blupi or destroy the wasp (`GEInteractionSystem::BalloonTouchedThisFrame()`).
      **Real hazard-pop interaction implemented**: while ballooned, touching exactly 4 of the 8
      shared-kill-list types (`3`/`16`/`96`/`97` — confirmed via the real source's if/else-if
      chain, NOT all 8) pops the balloon instead of killing, and does NOT destroy the popping
      hazard either (the real pop branch has no `ObjectDelete` call) — `2`/`4`/`17`/`20` still
      kill through the balloon. Real entry/recovery sound channels 40/41 (channel 41 shared with
      Crusher's own recovery, confirming it's a generic "status expired" cue, not
      hazard-specific). Already playable via the wasp already placed on the north-hill plateau
      in `worlds3d/world001.vwr`. Verified via 9 new `VerifyBlupiMovement` state-machine
      assertions and 9 new `VerifyInteractionSystem` assertions (wasp/follower/bulldozer
      interaction, injected synthetically since none but the wasp itself is placed in the real
      world).
- [x] `136` Type 54 (large creature) — lethal only while paused mid-turn, destroys current
      vehicle or fatally grabs Blupi, never destroyed itself, unconditional taunt icon. Done
      2026-07-11: verified directly against `Decor.cpp:5867-5913`. Contact is lethal ONLY while
      `patrolStep` is 1 or 3 (the real `step != 2 && step != 4` gate, now implementable since `131`
      landed) — safe to touch while it's actually mid-walk. The creature is never destroyed by the
      contact (no real `ObjectDelete` in that branch). Real balloon immunity IS modeled
      (`blupiBallooned` blocks the whole branch, matching the real `!m_blupiBalloon` gate, no
      separate pop path for this type unlike the 4 balloon-poppable hazards). Real shield/hide/
      superBlupi/focus immunity and the real "destroys Blupi's vehicle instead of killing him"
      branch are NOT modeled (no such concepts exist in this engine yet), so contact always takes
      the real no-vehicle death branch (channel 51). The real unconditional taunt icon is also NOT
      modeled — no idle-taunt animation system exists at all. The sample world's placement
      (`tools/GenerateSampleWorld3D.cpp`'s walled-room guardian) was converted from a zero-range
      `place()` call to a real `posStart != posEnd` patrol path (same real guard the platform lift
      needed) so both the safe-mid-walk and lethal-turn-dwell windows are genuinely playable, not
      just proven synthetically. Verified via 12 new `VerifyInteractionSystem` assertions (safe at
      patrolStep 2/4, lethal at 1/3, creature survives, balloon immunity, real placement has a
      patrol range) + full suite (63/63 unit tests, all verify tools) + live headless runs on both
      EasyGL and Vulkan backends.
- [x] `137` Types 96/97 (follower) — **contact-kill done**, folded into `E3D-MIG-132`'s widened
      shared kill list (2026-07-11), covering both the dormant (96) and awake (97) state
      identically, matching the real shared kill-list check. **Wake + homing done 2026-07-11**,
      completing Phase 13: verified directly against `Decor.cpp:9646-9678` (the wake box) and
      `8025-8064` (the homing step). A dormant `96` wakes into the homing `97` once Blupi is
      within its padded detection box (real 100px-padded rect approximated as a circular distance
      check, same simplification as every other proximity test in `GEInteractionSystem`; real
      channel 92 wake sound). Once awake it steps X and Y independently (Chebyshev-style, not a
      normalized diagonal) toward Blupi's live position at the real 1px/tick speed (≈0.3125 grid-
      units/sec); Z is left untouched (real mobile-eggbert has no Z axis, matching blupih/blupit's
      own shots). Self-destructs (real channel 10, no debris object spawned — no one-shot decorative-
      effect system exists yet) if its next step would land in a solid cell, rather than continuing
      to home, approximated as a single-point solid check at the destination cell (real `TestPath`
      sweeps a rectangle). The object exhibition's existing static `ObjectType96` specimen already
      makes this genuinely playable (open ground, no dedicated placement needed the way blupih/
      blupit/the large creature needed hand-carved terrain). Verified via 3 new
      `VerifyInteractionSystem` assertions (wake transition, gradual 1s homing progress in open
      air, blocked-path self-destruct) + full suite (63/63 unit tests, all verify tools) + live
      headless runs on both EasyGL and Vulkan backends.

**Phase 13 (Enemy AI & combat) is now complete.**

### Phase 14 — Hazards (`E3D-MIG-140`-`149`)

Nothing here is started. Full spec: `mobile-eggbert-reference/12-hazards-and-interactables.md`.
Note vehicle-immunity is NOT uniform — spikes/drip/saw/crusher have it, lava/blitz/fan do not.

- [x] `140` Lava (icon 68) — deterministic death, no immunity of any kind. Done 2026-07-11:
      `GEBlupiController::GetGroundBlockType()` (new, testable helper) detects standing on a lava
      block, `GalaxyEggbertCnaGame::Update()` triggers the same death consequence as
      `E3D-MIG-067`'s fall-off-world case (shared `triggerDeath()` lambda, channel 8). Verified via
      a new synthetic-world test in `VerifyBlupiMovement`. Deliberately does NOT reuse
      `BlockTypes::isHazard()` (a Simple3D-only uniform "any hazard kills" shortcut) — Spike/
      Crusher/Saw/Blitz each have real gating/timing conditions that helper ignores, so each stays
      its own dedicated task (`141`-`144`), not folded into one generic hazard check.
- [~] `141` **Spikes (373) done 2026-07-11**, same shape as `140` (`GetGroundBlockType() ==
      Spike` → `triggerDeath()`, real channel 51 not channel 8 — the Glu-death sound). Real
      vehicle+focus-gated immunity NOT modeled — neither vehicles nor a focus concept exist in
      CNA yet (Phase 17); currently unconditional, same simplification as lava, revisit once
      vehicles exist. Real narrow central x-band restriction within the tile also NOT modeled —
      no sub-tile position exists in the current single-point 3D collision. Verified via a
      synthetic-world test in `VerifyBlupiMovement`. **Drip (404/410) NOT done** — blocked on the
      `ThinMechanical` render-geometry decision (`E3D-MIG-510`, still `[?]`), since those icons
      aren't a placeable/renderable `BlockTypes` constant yet.
- [x] `142` **Saw (378/379) + switches done 2026-07-11** — verified directly against
      `Decor.cpp:7131-7148` (not just the reference doc). New `GEWorldRuntime::TryActivateSwitch()`:
      call on an edge-detected action-button press while grounded; a no-op unless standing on a
      `Switch`/`SwitchOff` tile; toggles the switch tile to the opposite state, then scans the real
      41-cell window (this switch's X ±20, same Y and Z — `BlockTypes.hpp`'s own pre-existing
      `isSwitch()` comment already documented this exact mapping before this task started)
      toggling every matching `Saw`/`SawStopped` block to match. Real entry/toggle sounds
      (channel 77 "on"/76 "off") and saw-contact death sound (channel 75, the "cut apart" cue,
      via the existing `triggerDeath()` pattern — only the active `Saw` type is lethal,
      `SawStopped` is a separate `BlockTypes` value so no extra gating is needed). Real
      "requires focus"/vehicle-immunity conditions not modeled (same reason as spikes/Crusher).
      Added a real switch+saw pair to `worlds3d/world001.vwr`'s south tunnel (grid (65,0,67) /
      (70,0,67), 5 cells apart) so the mechanic is genuinely playable, not just unit-tested — the
      saw starts safe (`SawStopped`) until the switch is pressed. Verified via 9 new
      `VerifyInteractionSystem` assertions against that real placement (no-op off-switch, no-op
      airborne, on-toggle, off-toggle, and the linked saw's state each time).
- [x] `143` **Crusher (317) done 2026-07-11** — verified directly against `Decor.cpp:5549-5597`/
      `5180-5197`/`7277-7288` (not just the reference doc). New `GEBlupiController::TriggerCrush()`/
      `IsEcrased()`: real `!m_blupiEcrase` re-trigger guard (idempotent, returns false if already
      squashed), reduced move speed (`kEcraseSpeedMultiplier=0.5`, an approximation — the real
      table's "×4 of m_blupiSpeedX" figure doesn't cleanly convert to a fraction of `kMoveSpeed`),
      jump blocked entirely, ~10s auto-recovery (`kEcraseDuration`, matches the real
      100-ticks-at-every-2nd-tick duration exactly, just as a plain countdown instead of a
      tick-based decrement). Real entry sound (channel 70) and recovery sound (channel 41,
      confirmed shared with other buff-expiry code, not crusher-specific) both wired. New
      `GEWorldRuntime::IsCrusherActiveAtPhase()` approximates the real 3-out-of-10 danger window
      (`m_time/3%10<=2`) — the real check runs on an explicitly non-FPS-normalized raw frame
      counter (a documented real-source quirk), which has no exact equivalent at this class's
      fixed 20-ticks/sec reference rate, so the same divisor shape is reused against that instead.
      No vehicle/focus gating modeled (same reason as spikes — neither concept exists yet).
      Verified via a `VerifyBlupiMovement` state-machine test (trigger/idempotency/speed/jump-
      block/recovery) and 5 `VerifyInteractionSystem` cycle-phase assertions.
- [x] `144` **Blitz (305) done 2026-07-11** — new `GEWorldRuntime::IsBlitzActiveAtPhase(int)`
      (static/pure, tested directly) replicates the real 100-tick flicker cycle exactly (lethal
      only on even ticks of the first half, 25% duty), using the same 20-ticks/sec `animPhase_`
      the per-tile animation-divisor system already advances at — no simplification needed here,
      real channel 8, no immunity of any kind (matches lava, unlike spikes). Emitter tile (304,
      cosmetic zap-sound timing only) NOT implemented — audio polish, not the hazard itself.
      Verified via 6 phase-value assertions in `VerifyInteractionSystem`.
- [x] `145` Spring (211) — dismounts vehicles first, then launches per Jump/Power combo (shares
      values with `E3D-MIG-065`'s direct-jump table). Done 2026-07-11: verified directly against
      `Decor.cpp:2835-2911`/`7312-7320`. Not a hazard — launches Blupi upward instead of costing a
      life, gated on grounded + not already airborne (real
      `!m_blupiNage && !m_blupiSurf && !m_blupiSuspend && !m_blupiAir`, only the last clause is
      relevant here since none of the other 3 states exist yet). New
      `GEBlupiController::TriggerSpringBounce(bool jumpHeld)` (idempotent, same
      no-op-while-already-triggered shape as `TriggerCrush()`/`TriggerBalloon()`) sets the real two
      noPower magnitudes (held=-19, not-held=-10 in the real source's own down-positive
      convention) — Power/SecretPower isn't modeled (Phase 17), so the two Power magnitudes
      (-25/-16) don't apply. `kJumpSpeed` itself has no documented real-value derivation (tuned by
      feel, not transcribed), so the two new constants (`kSpringBounceHeld`/
      `kSpringBounceNotHeld`) preserve the REAL PROPORTION between the spring's magnitudes and the
      real baseline ground-jump velocity (`IsNormalJump`'s held+noPower=-16) applied on top of
      this engine's own already-tuned `kJumpSpeed` — the same technique
      `GalaxyEggbertSimple3D`'s own stomp bounce already used (`kJumpSpeed * 0.65f`). Real vehicle-
      dismount-first branches (Helico/Over/Jeep/Tank/Skate) are NOT modeled (no vehicle concept
      exists yet, same simplification as every hazard so far). Also ported: the real generic
      landing-thud suppression when landing specifically on a spring (`Decor.cpp` ~2984,
      `if (!IsRessort(end))`), since the bounce sound (real channel 41) already covers it. Already
      playable — icon 211 is part of the tile exhibition's full 1..440 icon range, no dedicated
      placement needed. Verified via 6 new `VerifyBlupiMovement` assertions (ground detection,
      trigger/idempotency, held-vs-not-held magnitude comparison) + full suite (63/63 unit tests,
      all verify tools) + live headless runs on both EasyGL and Vulkan backends.
- [x] `146` Temp/vanishing tile (324) — NOT a kill-check, a 90%-solid/10%-passable oscillation
      Blupi can fall through during 2 transparent frames per cycle. Done 2026-07-11: verified
      directly against `Decor.cpp:7503-7538` (`IsPassIcon`/`IsBlocIcon`'s icon-324 special case).
      Solid for cycle buckets 0-17, passable (Blupi falls through) only for buckets 18-19 of a
      raw, un-scaled `m_time`-driven 20-value cycle (`m_time / 4 % 20 >= 18`) — NOT
      `Config::ScaleDiv`-normalized like almost every other timer in the source, an explicit
      exception the reference doc calls out (same category as Crusher's own raw `m_time`). At
      this project's Fps20 reference rate `Config::ScaleDiv(N) == N` exactly, so raw `m_time` and
      `GEWorldRuntime`'s own 20-ticks/sec `animPhase_` are numerically identical — unlike
      Crusher, this is an EXACT reuse, not an approximation. New static/pure
      `GEWorldRuntime::IsTempPassableAtPhase(int)`, same shape as `IsBlitzActiveAtPhase`/
      `IsCrusherActiveAtPhase`. No per-cell phase offset in the real source, so every Temp tile
      in a level blinks in perfect lockstep.
      Unlike every hazard/mechanic so far (all gated via a post-`Step()` `GetGroundBlockType()`
      check), this changes whether the tile IS solid ground at all — a collision-shape question,
      not a "what am I standing on" query — so it's threaded directly into
      `GEBlupiController::Step()`'s new `tempPassable` parameter (default `false`, so every
      existing call site is unaffected) down into `GroundHeightAt()`'s own solid-block scan
      (skips a `Temp` cell and keeps scanning downward when passable, so Blupi genuinely falls
      through to whatever's beneath, both for the main landing check and `TryMoveAxis`'s step-up
      gate). `GEBlupiController` itself stays fully engine-agnostic/decoupled from
      `GEWorldRuntime` (per its own class-comment design goal) — the caller
      (`GalaxyEggbertCnaGame::Update()`) pre-computes the bool from
      `GEWorldRuntime::IsTempPassableAtPhase(worldRuntime_.GetAnimPhase())` once per frame and
      passes it in, same pattern as `blupiCrouching`/`blupiBallooned` in
      `GEInteractionSystem::Update()`. Already playable — icon 324 is part of the tile
      exhibition's full 1..440 icon range, no dedicated placement needed. Verified via 4 new
      `VerifyBlupiMovement` assertions (solid-window ground detection + a genuine fall-through
      onto a real floor beneath once passable) and 5 new `VerifyInteractionSystem` phase-boundary
      assertions, + full suite (63/63 unit tests, all verify tools) + live headless runs on both
      EasyGL and Vulkan backends.
- [x] `147` Teleporter (330-333) — narrow trigger band, 128-tick delay, implicit pairing by
      shared icon value (first-match scan, requires exactly 2 instances per value). Done
      2026-07-11, **redesigned the same day after a live-playtest user bug report** (froze Blupi
      permanently, never transported him): verified directly against `Decor.cpp:7378-7394`
      (`IsTeleporte`), `5593-5606` (trigger), `6349-6358` (completion), and `7406-7429`
      (`SearchTeleporte`, pairing).
      - **Real architectural mismatch found, worked around, then genuinely solved.** The real
        check tests the tile one row ABOVE Blupi's feet (`pos.Y-60`), relying on real mobile-
        eggbert's genuinely per-tile-independent 2D collision (a tile's solidity has no bearing on
        the tile below it in the same column). This engine's simplified 3D collision instead
        treats the TOPMOST solid block in a column as that whole column's floor
        (`GroundHeightAt`), which makes "a solid tile floating above Blupi's own open, walkable
        column" physically unreachable — confirmed empirically (placing a solid pillar one cell
        above a floor in the same column made Blupi land ON the pillar, not beneath it). The
        FIRST attempt worked around this by detecting the cell Blupi FACES instead
        (`GetBlockTypeInFront()`, a wall-mounted-archway design) — this shipped, passed its own
        unit tests, but had a real, user-reported bug live: Blupi froze permanently and was never
        relocated. Root cause wasn't fully diagnosed (the redesign superseded it directly, see
        below) — the working theory is a live-game-specific interaction the synthetic unit tests
        (built around the same design) didn't exercise. The SECOND, final attempt instead solves
        the real collision mismatch directly: teleporter icons (330-333) are now ALWAYS non-solid
        for collision (new `IsTeleporterIcon()` check inside `GroundHeightAt`'s solid-block scan),
        so Blupi genuinely walks into the open space beneath a floating pillar — reproducing real
        mobile-eggbert's per-tile-independent collision for this one purpose, without a general
        per-cell-occupancy collision rewrite. Detection reverted to `GetBlockTypeAbove()` (one
        cell above Blupi's own position), matching the real geometry exactly, not just adapting
        around it.
      - New `GEBlupiController::TriggerTeleport(icon)`/`IsTeleporting()`/`GetTeleportIcon()`:
        idempotent (same shape as `TriggerCrush`/`TriggerBalloon`/`TriggerSpringBounce`), gated on
        grounded + not ballooned/squashed (real `!m_blupiAir && !m_blupiBalloon && !m_blupiEcrase`
        — vehicles/focus aren't modeled). Real `kTeleportDuration=6.4s` (`Config::ScaleTime(128)`
        at the 20Hz reference rate, a direct transcription). While teleporting, `Step()` fully
        freezes Blupi (no turning/movement/jump/gravity at all) via an early return — matches the
        real source's own effective behavior (`m_blupiFocus=false` gates essentially every other
        per-frame block, and nothing sets `m_blupiAir` during the transit).
      - New `GEWorldRuntime::FindTeleportDestination()` (real `SearchTeleporte`): a full-grid scan
        for another cell of the same icon, excluding candidates within a fixed radius of Blupi's
        own position (replacing the real source's exact entry-tile-equality skip, since this
        engine's "one cell below, not exactly at" entry convention doesn't map to an exact tile
        match). Lands Blupi one cell BELOW the matched pillar (Y), same real relationship as
        entry, offset one cell in +Z from directly beneath it (NOT the pillar's exact X/Z) —
        **a second real bug found and fixed live during this same task**: landing exactly beneath
        the matched pillar immediately re-satisfied `GetBlockTypeAbove()`'s own trigger condition
        again, producing an infinite teleport-back-and-forth ping-pong the instant Blupi arrived
        (confirmed via temporary debug instrumentation — reverted before committing — showing
        `teleporting=1` continuously across both the original transit AND an immediate second one
        with no gap). Real cosmetic entry/arrival particle effects (`ObjectType92`/`27`) are NOT
        modeled — same simplification as every other hazard/enemy's own real particle effects
        throughout this session; real channel 71 is reused for both entry and arrival.
      - Real playable placement: two small OPEN rooms south of the tunnel (previously-empty
        space), each with exactly one `Teleport1` pillar FLOATING one cell above the walkable
        floor (no walls at all — the pillar's own non-solid-for-collision status is what makes
        this reachable). Not a wide multi-cell teleporter structure — `FindTeleportDestination()`
        matches by exact block type, so more than one cell per room would risk self-matching
        within the same room. Icon 330 is deliberately excluded from the tile exhibition's
        generic per-icon loop to guarantee exactly 2 total occurrences; icons 331-333 remain in
        the exhibition as genuine lone specimens, faithfully demonstrating the real "no partner
        found" no-op path.
      - **Verified live, not just via unit tests** — the specific gap that let the first design's
        bug ship undetected. Temporary debug instrumentation (spawn override + periodic stdout
        position/state log, reverted before committing) confirmed: (1) the original broken
        design's exact failure mode is no longer reproducible, (2) the fixed design genuinely
        transports Blupi at the correct 6.4s mark to the exact expected destination coordinates,
        and (3) the ping-pong bug found along the way is also fixed (`teleporting=0` and a stable
        position after arrival, no immediate re-trigger). Also verified via 10 updated
        `VerifyBlupiMovement` assertions (a real regression test for the collision-solidity bug:
        Blupi must land on the real floor beneath a floating pillar, not on/blocked by it) and 3
        updated `VerifyInteractionSystem` assertions (matched-pair destination lookup accounting
        for the new landing offset, using non-real test icon values to avoid interference from the
        sample world's own real pair, and the no-partner-found path) + full suite (63/63 unit
        tests, all verify tools) + live headless runs on both EasyGL and Vulkan backends
        (6245-block/84-MoveObject world load).
- [ ] `148` Water breath gauge (91/92) — 3-state machine (Surf/Nage/dry), ~25s gauge, vehicles
      forcibly dismounted on entry.
- [x] `149` Fans (126-137, only the 4 head icons already rendered are lethal) — consumes itself
      (permanently clears the air column it blows through), kills only if unshielded+focused. Done
      2026-07-11, verified directly against `Decor::IsVentillo` (`Decor.cpp:7667-7752`, not just
      `12-hazards-and-interactables.md`'s own prior summary, which mischaracterized icons
      127/128/130/131/133/134/136/137 as the fan's "air-column/trail tiles" -- those are actually
      just the 4 head icons' own idle animation frames; the REAL trail-continuation icons
      (110/114/118/122) belong to a separate, not-yet-render-decided tile family never placed in
      any inspected level, so the trail-walk beyond the head tile itself is NOT ported -- doc
      corrected).
      - New `GEWorldRuntime::TryConsumeFan(blupiX, blupiY, blupiZ)`: checked one cell ABOVE Blupi
        (matching `GetBlockTypeAbove()`'s convention, same placement pattern the teleporter
        established), a no-op unless that cell is a real fan head icon
        (`BlockTypes::isFan()`); on a match, immediately clears it to `Air` (real
        `ModifDecor(pos, -1)` on the head tile) and returns the icon. Fan head icons are ALWAYS
        non-solid for collision (`GEBlupiController::GroundHeightAt`'s own new `isFan()` skip,
        same architecture as the teleporter pillar). Real sub-tile-band gating and
        focus/shield/hide/SuperBlupi immunity are NOT modeled (no sub-tile position or those buff
        concepts exist yet) -- contact anywhere in the cell is unconditionally lethal, same
        simplification as every other hazard this session. Real channel 10 plays via the existing
        `triggerDeath()` lambda; the real particle/screen-shake effects are NOT modeled (no
        particle system exists).
      - **Found a real, deeper architectural collision limitation while verifying this live** (not
        fixed, tracked in `NEXT.md` §5): `GEBlupiController::GroundHeightAt()` always resolves a
        column's floor as the single TOPMOST solid block in the ENTIRE column, with no concept of
        "nearest solid surface at or below Blupi's own current height." The sample world's south
        tunnel has a solid `BrickWall` ceiling (`y=3`) over its own open, walkable `y=1` interior
        -- meaning that interior's real floor is completely unreachable via normal walking (a
        `SetPosition()` teleport there resolves `onGround` at the CEILING's height, `y=4`, not the
        real floor; `TryMoveAxis` blocks any lateral step into that column for the same reason).
        Same root cause as the original teleporter bug (a floating solid block anywhere above
        Blupi in a column poisons the whole column's ground-height query), just triggered by a
        ceiling instead of a pillar. Confirmed via a standalone scripted walk test -- notably, NOT
        caught by any existing test in this session, including the same tunnel's own already-
        shipped switch/saw pair (`142`), because every prior hazard test used single-position
        `Step()` calls or direct `GEWorldRuntime` queries, never a genuine multi-column walk into
        that specific enclosed interior.
      - **Consequently, the fan hazard is placed in 2 new small OPEN rooms** (no walls/ceiling at
        all, same pattern as the teleporter rooms), NOT the tunnel's own roofed interior (which
        still has 2 fans placed, but as pure visual/render confirmation only, unreachable as a
        hazard until the ceiling limitation above is fixed).
      - New tests: `VerifyBlupiMovement.cpp` (fan head is non-solid, real floor beneath it
        reachable, `GetBlockTypeAbove()` detection) and `VerifyInteractionSystem.cpp`
        (`TryConsumeFan()` no-op away from any fan, detects+consumes+returns the icon, no-op the
        second time). Verified live end-to-end via a headless EasyGL run with temporary debug
        instrumentation (spawn in the open fan room + forced forward movement + periodic
        lives/position/fan-block-type logging, reverted before committing): Blupi walked in at a
        real `onGround=true`/`y=1` the whole approach, the fan triggered at the correct position,
        lives dropped 3→2, the fan tile became `Air`, and the FIFO respawn correctly kicked in
        afterward.

### Phase 15 — Crates, lifts, bridges, effects: full fidelity (`E3D-MIG-150`-`158`)

Extends the basic patrol/push already shipped in `GEInteractionSystem`. Full spec:
`mobile-eggbert-reference/14-crates-lifts-bridges-effects.md`.

- [ ] `150` Linked-crate flood-fill (`SearchLinkCaisse` equivalent) — stacks push atomically as
      a group, not single-crate; reduced push speed scales with stack size.
- [ ] `151` Crate "pop" push variant (landing-into-crate, different base speed) and the real
      20-tick speed ramp-up (vs. today's simplified constant-speed push).
- [ ] `152` Platform boarding via swept-probe detection (avoid tunneling on fast falls) and
      continuous per-tick foot-strip re-test while riding (not just an initial catch) — this is
      the prerequisite for "riding a moving platform" at all, since `GEBlupiController`
      currently only tests the static terrain grid.
- [ ] `153` Vertigo edge-detection with auto-slide-off for wide/shiftable lift platforms
      (icons 311-316).
- [ ] `154` Conveyor nudge (±2px/tick) for caterpillar-track types 47/48, on top of their
      existing patrol.
- [ ] `155` Dynamite — 9 separate blast calls at fixed ticks with asymmetric per-blast
      (dx,dy) scatter, each destroying enemies/crates/objects in a 128×128px area.
- [ ] `156` Helicopter-destruction / debris pool (ballistic pop-then-drop physics), shared by
      crate destruction too.
- [ ] `157` Bridge live collision toggle — `table_bridge` overwrites the actual terrain grid
      cell every tick across a 157-tick build sequence (solid only ticks 0-15/152-156); the
      earlier "purely cosmetic" assumption was wrong.
- [ ] `158` "Voyage" pickup-reward pattern: several pickups (treasure/egg/3 keys/dynamite)
      already delete-on-contact (done, `E3D-MIG-100`/`158` supersedes the old assumption) but
      should defer the actual reward (count++, flag) until a HUD-fly animation completes — not
      yet implemented, current impl applies the reward immediately.

### Phase 16 — Doors & keys (`E3D-MIG-160`-`165`)

Not started. Full spec: `mobile-eggbert-reference/06-doors.md`.

- [ ] `160` Door open sequence: icon removed, transient sliding-up ObjectType22 over 50 ticks,
      real sound channel — a pure slide, not a fade/shatter/swing.
- [ ] `161` Key pickup deferred to voyage-completion (shares `E3D-MIG-158`'s pattern); keys are
      persistent (not consumed on pickup) but consumed one-per-door on use.
- [ ] `162` Treasure-gated doors (icon family 421+N = needs N treasures) — opens ALL qualifying
      doors level-wide simultaneously on any treasure pickup, not just the nearest.
- [ ] `163` Render closed doors as `Billboard` — tracked in `E3D-MIG-516`, cross-referenced here.
- [ ] `164` `AdaptDoors` hub-screen logic (gold-flag reveals, icon swaps) — depends on hub/menu
      screens existing (`## 2 §2 MENU-*`), lower priority.
- [ ] `165` World-entry-screen door logic (opens matching sublevel doors, snaps Blupi facing) —
      same menu dependency as `164`.

### Phase 17 — Secret powers, vehicles, buffs, remaining pickups (`E3D-MIG-170`-`179`)

Not started. Full spec: `mobile-eggbert-reference/13-object-pickups.md`,
`10-blupi-mechanics.md`.

- [ ] `170` `[?]` Research secret-power (`Sp0`-`Sp7`) behavior — currently unclassified in every
      reference doc; must be resolved before implementing (only the icon/render side,
      `E3D-MIG-515`, is currently plannable).
- [ ] `171` Vehicle mounts: Helicopter(13), Jeep(19), Skateboard(24), Tank(28), Overcraft(46,
      inverted accel — only accelerates over gaps), Balloon(buoyancy drift) — shared guard:
      blocked only while riding another vehicle or swim/surf/suspend.
- [ ] `172` Shared power-up timer (Shield/Power/Cloud/Hide all use `m_blupiTimeShield`, differing
      decrement rates and thus differing real durations despite the same start value) and their
      warning-sound thresholds.
- [ ] `173` Suction-cup(26) and Drink(30) — both two-stage pickups (grab sound, then a delayed
      buff-activate sound ~32 ticks later).
- [ ] `174` Charge/Cloud(31) — gated against ALL other buffs including itself (loosest-guard
      opposite is Mirror/Invert(40), gated only against Hide).
- [ ] `175` Bullet pack(29) — auto-pickup, cap 10, immediate (not voyage-deferred) unlike most
      other pickups.
- [ ] `176` Pickup sparkle-fx(39) — cosmetic only, spawned by treasure/key pickups.
- [ ] `177` Ecrase/pancake collision-box mode and Suspended (hanging, no accel ramp) movement
      mode — the two collision/movement modes not covered by `E3D-MIG-171`'s vehicle list.
- [ ] `178` Vehicle/mode-specific movement table (max speed/accel/vertical behavior per mode) —
      needed once any vehicle from `171` is implemented.
- [ ] `179` `[?]` Enemy billboard walk-cycle direction mismatch — enemy sprites only have
      left/right side-view frames; viewed at an oblique 3D angle this will look visibly wrong
      (already observed for Blupi, mooted there by the first-person camera, but NOT mooted for
      enemies since the player does see them from arbitrary angles). No resolution proposed yet.

### Phase 11 — Retire Simple3D path (later) (`E3D-MIG-110`-`112`)

- [ ] Don't delete Simple3D now; retire only once CNA reaches playable parity. Note:
      `CLAUDE.md` (2026-07-08) has already tightened this — Simple3D is "historical reference
      only," never built/fixed, tighter than this phase's original premise. Re-evaluate deletion
      with the user before acting, per CLAUDE.md's explicit-removal-task requirement.

### Phase 12 — Optional future (`E3D-MIG-120`-`124`)

- [ ] Optional 3D Blupi model, camera-mode-switching polish, Lua discussion (undecided, not
      requested), Easy3D renderer polish. None of these are required for first playable
      gameplay parity.

---

## 2. Feature Parity Checklist

Enumerates every mobile-eggbert feature that must eventually exist in some form in
`GalaxyEggbertCNA`. Status marks below reflect CNA specifically — they were reset
2026-07-10 (see the note at the top of this file); GalaxyEggbertSimple3D's historical
status has no bearing here. Where a task's assumption was found to be wrong by more
recent `mobile-eggbert-reference/` research, a correction note is inlined.

### 2.1 Engine & Build

CNA is the sole long-term target (see CLAUDE.md "Current Direction Lock"). Its CMake wiring is
already done and working: `-DGALAXY_EGGBERT_BUILD_CNA=ON` (default `ON`), target `GalaxyEggbertCNA`,
builds cleanly under `build-cna` and `build-cna-vulkan`. The tasks below are re-scoped to CNA;
anything that was specific to the dead Simple3D/U3D/Nova3D/Android direction is dropped.

- [x] BUILD-001 — CMake target `GalaxyEggbertCNA` builds on Linux (CNA backend) (CNA, 2026-07-10)
- [x] BUILD-002 — `GALAXY_EGGBERT_BUILD_CNA` option wired and defaults to `ON` (CNA, 2026-07-10)
- [ ] BUILD-003 — Web build (Emscripten / WebAssembly) for `GalaxyEggbertCNA` — not attempted yet
- [ ] BUILD-005 — Windows cross-compile (MinGW-w64) for `GalaxyEggbertCNA` — not attempted yet
- [ ] BUILD-007 — `GalaxyEggbertWorldsTests` unit tests build and all pass — re-verify current count under CNA-only build (was 54, see TEST-001 note in §13)
- [ ] BUILD-008 — `ctest --test-dir <build-dir>` discovers and runs the world tests
- [ ] BUILD-009 — CI: automated build on push (GitHub Actions), CNA target only (Linux; Web once BUILD-003 exists)
- [ ] BUILD-010 — Package installer / distributable (Linux AppImage or .tar.gz with bundled assets) for `GalaxyEggbertCNA`

Dropped (dead Simple3D/U3D/Nova3D/Android direction, do not carry forward): old BUILD-001..002 as
originally scoped to `GalaxyEggbertSimple3D`/U3D, old BUILD-004 (Android via U3D/Nova3D), old
BUILD-006 (Nova3D backend switch).

---

### 2.2 Menu & Screens (PRIORITY)

Menu screens use the same PNG backgrounds as mobile-eggbert (`Content/backgrounds/*.png`,
`Content/icons/*.png`). All statuses below reset to reflect `GalaxyEggbertCNA`, which has not
started on HUD/menu work — see CLAUDE.md and NEXT.md. Every item that was previously `[x]`
reflected `GalaxyEggbertSimple3D` only (historical reference now) and is reset to `[ ]` unless
otherwise noted.

#### 2.1 Phase: First / Wait (loading screen)

- [ ] MENU-001 — Render `wait.png` as full-screen image during boot loading phase
- [ ] MENU-002 — Display animated loading gauge (`jauge.png`, yellow fill) at bottom-centre, same position as mobile-eggbert (196, 426 in 640×480 space)
- [ ] MENU-003 — Gauge fills from 0→100% as resources load (replicate `DrawWaitProgress` logic)
- [ ] MENU-004 — Transition from Wait → Init after loading completes (≥1 s minimum)
- [ ] MENU-005 — Hide wait gauge if resuming a saved game (ContinueMission path)

#### 2.2 Phase: Init (main menu / gamer select)

- [ ] MENU-006 — Render `init.png` as full-screen background
- [ ] MENU-007 — Render `speedyblupi.png` (title logo) sliding in from top on enter, ease-out quadratic over 1 s
- [ ] MENU-008 — Render `blupiyoupie.png` (Blupi character art) scaling in from centre (0.5→1.0 with fade-in) over 1 s
- [ ] MENU-009 — Three gamer-slot buttons (A / B / C): render from `pad.png` (cell 140×140), correct screen positions, selected slot highlighted with alternate icon
- [ ] MENU-010 — Each gamer slot shows: name ("Gamer A/B/C"), lives count, main doors opened, secondary doors opened (text next to button, 0.7 scale)
- [ ] MENU-011 — "PLAY" button (`InitPlay` glyph) with label below
- [ ] MENU-012 — "SETUP" button (`InitSetup` glyph) with label to the right
- [ ] MENU-013 — "RANKING" button (`InitRanking` glyph) — shown only when ranking mode active
- [ ] MENU-014 — Semi-transparent panel behind left gamer slots (pad.png icon 15, opacity 0.3)
- [ ] MENU-015 — Semi-transparent panel behind right action buttons (pad.png icon 15, opacity 0.3)
- [ ] MENU-016 — Keyboard Back / Escape → exit game (from Init phase)
- [ ] MENU-017 — Animated fade-out when transitioning from Init → Play (speedyblupi.png slides out, blupiyoupie.png zooms out)
- [ ] MENU-018 — Animated fade-out when transitioning from Init → MainSetup (speedyblupi.png slides right, gear appears)
- [ ] MENU-019 — Gamer selection persisted (GameData byte 2) — SAVE system not started on CNA, see §11
- [ ] MENU-020 — Gamer slot info (lives, lastWorld, doors) read from GameData

#### 2.3 Phase: Play (active gameplay)

- [ ] MENU-021 — Hide all menu UI elements during Play phase
- [ ] MENU-022 — "PAUSE" button (`PlayPause` glyph) visible during Play — top-right corner icon from `button.png`
- [ ] MENU-023 — On-screen directional pad (`pad.png` icons 0, 1) for touch/gamepad emulation
- [ ] MENU-024 — On-screen "JUMP" button (`PlayJump`) visible during Play
- [ ] MENU-025 — On-screen "ACTION" button (`PlayAction`) visible during Play
- [ ] MENU-026 — On-screen "DOWN" button (`PlayDown`) visible during Play (when applicable)
- [ ] MENU-027 — Keyboard: Back/Escape during Play → Pause phase

#### 2.4 Phase: Pause

- [ ] MENU-028 — Render `pause.png` as full-screen background
- [ ] MENU-029 — Render `blupiyoupie.png` scaling/rotating in (same animation as Init but centred at 418,190)
- [ ] MENU-030 — "MENU" button (`PauseMenu`) with label below
- [ ] MENU-031 — "BACK" button (`PauseBack`) — shown only when mission ≠ 1
- [ ] MENU-032 — "SETUP" button (`PauseSetup`) with label below
- [ ] MENU-033 — "RESTART" button (`PauseRestart`) — shown only when mission ≠ 1 AND mission % 10 ≠ 0
- [ ] MENU-034 — "CONTINUE" button (`PauseContinue`) with label below
- [ ] MENU-035 — PauseBack goes to previous hub world (MissionBack logic: if mission%10==0 → Init, else mission/10*10)
- [ ] MENU-036 — PauseRestart restarts current mission
- [ ] MENU-037 — PauseContinue resumes play without reloading
- [ ] MENU-038 — Animated fade-out from Pause → Play (blupiyoupie.png zooms out)
- [ ] MENU-039 — Animated slide-out when Pause → PlaySetup (blupiyoupie.png slides right)

#### 2.5 Phase: Resume (saved game continue prompt)

- [ ] MENU-040 — Render `pause.png` background (same as Pause)
- [ ] MENU-041 — Render `blupiyoupie.png` with rotation spring animation
- [ ] MENU-042 — "MENU" button (`ResumeMenu`) → Init
- [ ] MENU-043 — "CONTINUE" button (`ResumeContinue`) → ContinueMission()
- [ ] MENU-044 — Resume phase triggers when app reactivates with a saved mid-game state
- [ ] MENU-045 — Keyboard Back during Resume → Init

#### 2.6 Phase: Win

- [ ] MENU-046 — Render `win.png` as full-screen background
- [ ] MENU-047 — Render `blupiyoupie.png` with pulsating scale (sin wave animation, amplitude 1.0±0.5)
- [ ] MENU-048 — "RETURN" button (`WinLostReturn`) → Init
- [ ] MENU-049 — Display mission elapsed time in text overlay
- [ ] MENU-050 — Display score in text overlay
- [ ] MENU-051 — Display "NEW RECORD!" text if score exceeds saved high score
- [ ] MENU-052 — Auto-advance to next level after N seconds (optional: like mobile-eggbert)

#### 2.7 Phase: Lost (game over)

- [ ] MENU-053 — Render `lost.png` as full-screen background
- [ ] MENU-054 — Render `blupiyoupie.png` with spin animation (6× rotation, quadratic ease-in, same as mobile-eggbert)
- [ ] MENU-055 — "RETURN" button (`WinLostReturn`) → Init
- [ ] MENU-056 — Display lives remaining and score
- [ ] MENU-057 — If 0 lives: "GAME OVER" text; if lives remain: "TRY AGAIN" hint

#### 2.8 Phase: MainSetup / PlaySetup (settings)

- [ ] MENU-058 — Render `setup.png` as full-screen background
- [ ] MENU-059 — Render `speedyblupi.png` sliding in from left (ease-out quadratic)
- [ ] MENU-060 — Render two rotating `gear.png` icons (one CW, one CCW, varying speeds)
- [ ] MENU-061 — "SOUNDS" toggle button (`SetupSounds`) — shows ON/OFF state
- [ ] MENU-062 — "JUMP" mode toggle (`SetupJump`) — left/right jump direction
- [ ] MENU-063 — "ZOOM" toggle (`SetupZoom`) — auto-zoom on/off
- [ ] MENU-064 — "ACCEL" toggle (`SetupAccel`) — accelerometer on/off
- [ ] MENU-065 — "RESET Gamer X" button (`SetupReset`) — with gamer letter in text
- [ ] MENU-066 — "RETURN" button (`SetupReturn`) → Init (from MainSetup) or Play (from PlaySetup)
- [ ] MENU-067 — All toggles persist to GameData immediately on press
- [ ] MENU-068 — Animated slide-in/out of settings panel (matching mobile-eggbert timing)
- [ ] MENU-069 — Keyboard Back during Setup → Init

#### 2.9 Phase: Ranking

- [ ] MENU-070 — Render `pause.png` background (same as Pause/Resume)
- [ ] MENU-071 — Display high-score table for all 3 gamer slots (name, score, doors opened)
- [ ] MENU-072 — "BACK" button (`RankingContinue`) → Init
- [ ] MENU-073 — Highlight current gamer row

#### 2.10 Phase: Trial (purchase prompt — low priority for open-source port)

- [ ] MENU-074 — Render `trial.png` background
- [ ] MENU-075 — Display trial text lines (TX_TRIAL1..6)
- [ ] MENU-076 — "BUY" button (`TrialBuy`) — no-op or skip in open-source build
- [ ] MENU-077 — "CANCEL" button (`TrialCancel`) → Init
- [ ] MENU-078 — Trial mode guard: if mission > 20 and mission % 10 > 1 → Trial (replicate mobile-eggbert trial logic)

#### 2.11 Level Intro / Mission Title

- [ ] MENU-079 — Level intro title card: world name text, 3 s duration (fade-in 0.5s, hold 2s, fade-out 0.5s)
- [ ] MENU-080 — Training level hint bar: show tutorial text from `table_training1..4` based on Blupi position
- [ ] MENU-081 — Training hint rendered as overlay bar (pad.png icon 15 background, text centred)
- [ ] MENU-082 — Training hint auto-scales down if text is too wide (min 0.5×)

#### 2.12 Button Font & Text Rendering

- [ ] MENU-083 — Render button labels using `text.png` font sheet (32×32 per glyph)
- [ ] MENU-084 — `Text::DrawText` equivalent: render text string using glyph atlas
- [ ] MENU-085 — `Text::DrawTextCenter` equivalent: centre-aligned text rendering
- [ ] MENU-086 — Text scaling (0.45×, 0.7×, 1.0×) used for different label sizes
- [ ] MENU-087 — Localised strings (MyResource strings): port key TX_ constants for button labels

#### 2.13 Phase Transitions & Animations

- [ ] MENU-088 — Fade-out animation between animated phases (20-frame linear fade, Config::ScaleTime(20))
- [ ] MENU-089 — `fadeOutPhase` deferred transition: start animation, complete transition after 20 frames
- [ ] MENU-090 — `missionToStart1/2` two-stage mission loading pipeline (background swap before Start)
- [ ] MENU-091 — Phase time counter reset on each phase entry

#### 2.14 Cheat Menu (hidden)

- [ ] MENU-092 — Cheat gesture recognition: sequence of 6 button glyphs (Cheat11..Cheat32) unlocks cheat menu
- [ ] MENU-093 — Cheat menu overlay: 9 cheat action buttons (Cheat1..Cheat9)
- [ ] MENU-094 — Cheat 1: OpenDoors (open all doors in current level)
- [ ] MENU-095 — Cheat 2: SuperBlupi (invincibility + all abilities)
- [ ] MENU-096 — Cheat 3: ShowSecret (reveal secret exits)
- [ ] MENU-097 — Cheat 4: LayEgg (spawn eggs)
- [ ] MENU-098 — Cheat 5: Reset gamer progress
- [ ] MENU-099 — Cheat 6: Simulate trial mode toggle
- [ ] MENU-100 — Cheat 7: CleanAll (remove all mobile objects)
- [ ] MENU-101 — Cheat 8: AllTreasure (collect all treasures)
- [ ] MENU-102 — Cheat 9: EndGoal (win current level immediately)

---

### 2.3 HUD (Heads-Up Display)

CNA has no real HUD yet — only a temporary 2D debug anim-state indicator. Every item below is
reset to `[ ]`; none of the old Simple3D `[x]` marks carry over.

- [x] HUD-001 — Life icons: Blupi head sprite (icon 48 from `blupi.png`) × nbVies, bottom-left row (CNA, 2026-07-11; since 2026-07-10 at the REAL `DrawInfo` position (210,417), X+=16, via `GEHud`)
- [ ] HUD-002 — Life icons cap at 5 visible; overflow shown as "+N" text — revision: verify exact mobile-eggbert layout; current CNA impl draws one icon per life uncapped (real `DrawInfo` is also uncapped — re-verify whether any cap exists at all before implementing one)
- [x] HUD-003 — Treasure counter "N/total" text, bottom-centre panel (CNA, 2026-07-10, `GEHud`): real position (460,450), glyphs from `text.png` whose sheet index IS the ASCII code (read off the asset, not `table_char`); fixed 17px advance approximates the real proportional widths
- [x] HUD-004 — Panel background behind treasure counter (pad.png icon 15) (CNA, 2026-07-10, `GEHud`) — at opacity 1.0 instead of the real 0.6 for now (CNA Vulkan drops `BasicEffect` draws with Alpha<1, see NEXT.md §5)
- [x] HUD-005 — Key icon — red key (element.png icon 215) shown when Key1 held (CNA, 2026-07-11; since 2026-07-10 at the REAL position (520,418) via `GEHud`)
- [x] HUD-006 — Key icon — green key (element.png icon 222) shown when Key2 held (CNA, 2026-07-11; real position (530,418))
- [x] HUD-007 — Key icon — blue key (element.png icon 229) shown when Key3 held (CNA, 2026-07-11; real position (540,418))
- [ ] HUD-008 — Shield timer gauge (jauge.png yellow fill) — visible when shield active
- [ ] HUD-009 — Score display (text label, top-right area)
- [ ] HUD-010 — World name + elapsed level timer
- [ ] HUD-011 — Game speed indicator label (SLOW / NORMAL / FAST)
- [ ] HUD-012 — Gauge sprite (jauge.png) bottom-left area
- [ ] HUD-013 — Hit flash: red full-screen overlay panel, 0.4 s fade on damage
- [ ] HUD-014 — Camera shake on hit
- [ ] HUD-015 — Bullet counter: element.png icon 176 × m_blupiBullet, small row near bottom-right
- [ ] HUD-016 — Dynamite count: element.png icon 252 shown when m_blupiDynamite > 0
- [ ] HUD-017 — Perso (persona) counter: button.png icon 108 + "= N" text when m_blupiPerso > 0
- [ ] HUD-018 — Second gauge (jauge.png red fill): used for charge level (m_blupiLevel in charge mode)
- [ ] HUD-019 — Yellow gauge: shield timer ticks down from 100 (shown while shield active, hidden when expired)
- [ ] HUD-020 — "EXIT OPEN!" popup text (3 s timed, big centred text) when all treasures collected
- [ ] HUD-021 — Controls hint bar fades after 8 s (re-show on new level)
- [ ] HUD-022 — Pause button icon visible during Play phase (top area)
- [ ] HUD-023 — HUD hidden during non-Play phases (Init, Pause, Win, Lost, Setup)
- [ ] HUD-024 — Training hint overlay at screen top (missions 11-14 only)
- [ ] HUD-025 — Score popup: "+N" floating text rises and fades over 1 s at collection position *(3D visual)*
- [ ] HUD-026 — "EXIT OPEN!" text: centre screen, large font, 3 s duration

---

### 2.4 Blupi Character

CNA's `GEBlupiController` currently has grid-based collision, step-up traversal, gravity, and a
minimal `AnimState` (Stop/March/Jump/Down/Up) — collision-only, not a transcription of the 2D
`BlupiRect`/`BlupiAdjust`/`BlupiBloque` system. Blupi has **no visible billboard/model in
first-person** yet (only a temporary 2D sprite HUD indicator plus an optional third-person
placeholder Fox model) — this is the single biggest open gap in the whole project. All items below
reset to `[ ]` except the small set with direct CNA evidence.

#### 4.1 Physics & Movement

- [x] BLUPI-001 — Gravity applied every frame (CNA, 2026-07-10)
- [x] BLUPI-002 — Jump: upward impulse on jump input; air flag set (CNA, 2026-07-10)
- [x] BLUPI-003 — Walk left/right: input sets horizontal speed (CNA, 2026-07-10)
- [ ] BLUPI-004 — Crouch: Left Shift sets Down state
- [ ] BLUPI-005 — Look up / glide: Right Shift in air → reduced gravity, capped fall speed
- [x] BLUPI-006 — Auto step-up: 1-tile ledges climbed automatically (CNA, 2026-07-10) *(3D adaptation)*
- [x] BLUPI-007 — Grid-based tile collision (CNA, 2026-07-10) — collision-only, not the 2D AABB/CharacterController transcription
- [ ] BLUPI-008 — Respawn at blupiStart on death
- [ ] BLUPI-009 — Respawn invincibility: 2 s grace period after death
- [ ] BLUPI-010 — Flash during invincibility (10 Hz sprite show/hide)
- [ ] BLUPI-011 — Blob shadow (scans downward, scales with height) *(3D adaptation)*
- [ ] BLUPI-012 — Sub-pixel accumulator (m_blupiSubPixelX/Y) — prevents drift at high FPS
- [ ] BLUPI-013 — BlupiBloque: directional collision query (can I move here?)
- [ ] BLUPI-014 — BlupiAdjust: push Blupi out of penetrated tiles after movement
- [ ] BLUPI-015 — SCROLL_SPEED = 8 px/tick camera scroll toward Blupi
- [ ] BLUPI-016 — m_blupiLevel: charge level gauge for vehicles/actions
- [ ] BLUPI-017 — m_blupiTimeNoAsc: timer preventing lift re-entry after a dismount
- [ ] BLUPI-018 — m_blupiTimeMockery: timer for enemy mockery animation
- [ ] BLUPI-019 — m_blupiTimeOuf: relief animation timer (Ouf variants)
- [ ] BLUPI-020 — m_blupiFifoPos[10]: history of last 10 positions (used for teleporter exit placement)
- [ ] BLUPI-021 — Blupi "front" flag (m_blupiFront): determines draw order vs objects

#### 4.2 BlupiAction State Machine (87 states)

- [x] BLUPI-022 — None (uninitialised) (CNA, 2026-07-10 — trivial default state)
- [x] BLUPI-023 — Stop (idle standing) (CNA, 2026-07-10)
- [x] BLUPI-024 — March (walking) (CNA, 2026-07-10)
- [ ] BLUPI-025 — Turn (turning around)
- [x] BLUPI-026 — Jump (jumping) (CNA, 2026-07-10)
- [ ] BLUPI-027 — Air (airborne / falling) — note: CNA AnimState list is Stop/March/Jump/Down/Up; confirm Air is distinct before marking
- [x] BLUPI-028 — Down (crouch) (CNA, 2026-07-10 — state exists in AnimState enum; verify crouch logic itself, see BLUPI-004)
- [x] BLUPI-029 — Up (look-up / glide) (CNA, 2026-07-10 — state exists in AnimState enum; verify glide logic itself, see BLUPI-005)
- [ ] BLUPI-030 — Vertigo (hanging on ledge in fear — ACTION_VERTIGO)
- [ ] BLUPI-031 — Recede (moving backward — ACTION_RECEDE)
- [ ] BLUPI-032 — Advance (moving forward — ACTION_ADVANCE)
- [ ] BLUPI-033 — Clear1..Clear8 (clearing animations — used for special level events)
- [ ] BLUPI-034 — Set (placing object — ACTION_SET)
- [ ] BLUPI-035 — Win (level-win celebration animation)
- [ ] BLUPI-036 — Push (pushing a crate — ACTION_PUSH)
- [ ] BLUPI-037 — StopHelico (helicopter hover)
- [ ] BLUPI-038 — MarchHelico (helicopter fly forward)
- [ ] BLUPI-039 — TurnHelico (helicopter turn)
- [ ] BLUPI-040 — StopNage (treading water)
- [ ] BLUPI-041 — MarchNage (swimming forward)
- [ ] BLUPI-042 — TurnNage (turning while swimming)
- [ ] BLUPI-043 — StopSurf (surfboard idle)
- [ ] BLUPI-044 — MarchSurf (surfing forward)
- [ ] BLUPI-045 — TurnSurf (turning on surfboard)
- [ ] BLUPI-046 — Drown (drowning in deep water)
- [ ] BLUPI-047 — StopJeep / MarchJeep / TurnJeep (jeep vehicle states)
- [ ] BLUPI-048 — StopPop / Pop (pop-star costume idle/dance)
- [ ] BLUPI-049 — Bye (farewell exit animation)
- [ ] BLUPI-050 — StopSuspend / MarchSuspend / TurnSuspend / JumpSuspend (rope hanging)
- [ ] BLUPI-051 — Hide (hiding in object)
- [ ] BLUPI-052 — JumpAie (hurt jump on hazard contact)
- [ ] BLUPI-053 — StopSkate / MarchSkate / TurnSkate / JumpSkate / AirSkate (skateboard)
- [ ] BLUPI-054 — TakeSkate (picking up skateboard)
- [ ] BLUPI-055 — DeposeSkate (putting down skateboard)
- [ ] BLUPI-056 — Ouf1a / Ouf1b / Ouf2 / Ouf3 / Ouf4 / Ouf5 (relief animations)
- [ ] BLUPI-057 — Sucette (collecting lollipop/suction-cup power-up)
- [ ] BLUPI-058 — StopTank / MarchTank / TurnTank / FireTank (tank vehicle)
- [ ] BLUPI-059 — Glu (stuck in glue)
- [ ] BLUPI-060 — Drink (drinking power-up animation)
- [ ] BLUPI-061 — Charge (being charged at by enemy)
- [ ] BLUPI-062 — Electro (electrocuted by electric field)
- [ ] BLUPI-063 — HelicoGlu (helicopter stuck in glue)
- [ ] BLUPI-064 — TurnAir (turning while airborne)
- [ ] BLUPI-065 — StopMarch (decelerating from walk to stop)
- [ ] BLUPI-066 — StopJump / StopJumph (jump landing, high-jump landing)
- [ ] BLUPI-067 — Mockery / Mockeryi / Mockeryp (enemy mocking Blupi)
- [ ] BLUPI-068 — Balloon (balloon flight mode)
- [ ] BLUPI-069 — StopOver / MarchOver / TurnOver (flat/squashed mode)
- [ ] BLUPI-070 — Recedeq / Advanceq (quick backward/forward movement)
- [ ] BLUPI-071 — StopEcrase / MarchEcrase (crushed under object)
- [ ] BLUPI-072 — Teleporte (teleporting animation)
- [ ] BLUPI-073 — Switch (activating a switch)
- [ ] BLUPI-074 — Non (refusing / head-shake animation)
- [ ] BLUPI-075 — SlowdownSkate (skateboard braking)
- [ ] BLUPI-076 — TakeDynamite / PutDynamite (dynamite pickup/place)

#### 4.3 Blupi Sprite Animation

- [ ] BLUPI-077 — Frame tables from `table_blupi` (2911 entries)
- [ ] BLUPI-078 — Billboard sprite from `blupi.png` (60×60 px cells) — CNA has no visible first-person Blupi yet; only a temporary 2D sprite HUD indicator / optional third-person placeholder model
- [ ] BLUPI-079 — Direction flipping: mirror sprite when moving right (table_mirror)
- [ ] BLUPI-080 — All 87 BlupiAction frames resolved from table_blupi via action+phase+dir lookup
- [ ] BLUPI-081 — `blupi1.png` alternate skin channel (Blupi1_11/12/13 variants for ObjectType200-203)
- [ ] BLUPI-082 — Shield tint: cyan/blue sprite overlay when m_blupiShield active
- [ ] BLUPI-083 — Shield blink at < 1.5 s remaining (blink 10 Hz)

#### 4.4 Vehicle Modes (each = new movement model + sprite sheet section)

- [ ] BLUPI-084 — Helicopter mode (m_blupiHelico): 8-direction flight, no gravity, propeller sound loop (ch16/ch18)
- [ ] BLUPI-085 — Helicopter boarding: touch ObjectType13 → sets m_blupiHelico, removes object
- [ ] BLUPI-086 — Helicopter dismount: press Down → drops Blupi, reverts to normal mode
- [ ] BLUPI-087 — Helicopter destroyed by creature (ObjectType54): ByeByeHelico debris effect
- [ ] BLUPI-088 — Jeep mode (m_blupiJeep): horizontal drive, jump, carry Blupi
- [ ] BLUPI-089 — Jeep boarding: touch ObjectType19 → sets m_blupiJeep
- [ ] BLUPI-090 — Jeep motor sound loop (ch29/ch31, high/low pitch via m_blupiMotorHigh)
- [ ] BLUPI-091 — Tank mode (m_blupiTank): drive + fire projectiles (ch FireTank)
- [ ] BLUPI-092 — Tank boarding: touch ObjectType28 → sets m_blupiTank
- [ ] BLUPI-093 — Tank fire sound (ch 53) on FireTank animation frames
- [ ] BLUPI-094 — Skateboard mode (m_blupiSkate): faster horizontal, higher jump
- [ ] BLUPI-095 — Skateboard pickup: touch ObjectType24 → TakeSkate animation → sets m_blupiSkate
- [ ] BLUPI-096 — Balloon mode (m_blupiOver/m_blupiBalloon): float up/down, limited horizontal
- [ ] BLUPI-097 — Balloon pickup: touch ObjectType46 → sets m_blupiOver
- [ ] BLUPI-098 — Swimming (m_blupiNage): entered when Blupi falls into water (`table_vitesse_nage`)
- [ ] BLUPI-099 — Surfing (m_blupiSurf): surfboard on water surface (`table_vitesse_surf`)
- [ ] BLUPI-100 — Vent (fan) propulsion (m_blupiVent): blown by ventilator tile
- [ ] BLUPI-101 — Suspend (rope hang): entered when Blupi grabs a rope tile
- [ ] BLUPI-102 — Motor sound crossfade: one-shot start/stop sounds + looped motor sound
- [ ] BLUPI-103 — m_blupiMotorHigh: pitch variant selection (fast vs slow motor)

#### 4.5 Blupi Special States & Power-ups

- [ ] BLUPI-104 — Shield (m_blupiShield): 5 s invincibility from ObjectType25; bypasses all hazards
- [ ] BLUPI-105 — Shield timer (m_blupiTimeShield): counts down 0→100 ticks; gauge shows progress
- [ ] BLUPI-106 — Shield trail sparkle (ObjectType57 spawned while shield active)
- [ ] BLUPI-107 — SuperBlupi (m_bSuperBlupi): cheat mode, full invincibility + all powers
- [ ] BLUPI-108 — Cloud mode (m_blupiCloud): from ObjectType31, floats through blocks for N ticks
- [ ] BLUPI-109 — Invert mode (m_blupiInvert): from ObjectType40, inverted controls for 100 ticks
- [ ] BLUPI-110 — Invert start/stop particle burst (ObjectType41/42 in 4 directions)
- [ ] BLUPI-111 — Ghost mode (m_blupiGhost): cheat, passes through walls, no interactions
- [ ] BLUPI-112 — Hide mode (m_blupiHide): concealed in object
- [ ] BLUPI-113 — Sucette/suction-cup (m_blupiPower): from ObjectType26, walk up walls
- [ ] BLUPI-114 — Dynamite (m_blupiDynamite): from ObjectType55; TakeDynamite / PutDynamite actions
- [ ] BLUPI-115 — Bullet count (m_blupiBullet): from ObjectType29; FireTank expends bullets
- [ ] BLUPI-116 — Ecrase mode (m_blupiEcrase): crushed flat under object (StopEcrase/MarchEcrase)
- [ ] BLUPI-117 — m_blupiPerso: persona counter (shown in HUD as button icon 108 + count)

#### 4.6 Blupi Death & Respawn

- [ ] BLUPI-118 — Death: BlupiDead() triggers explosion effect, resets lives-1, respawn
- [ ] BLUPI-119 — Death freeze: 1 s input lock before respawn
- [ ] BLUPI-120 — Drown death: different animation (ACTION_DROWN) in deep water
- [ ] BLUPI-121 — Electro death: ACTION_ELECTRO animation + electric shake
- [ ] BLUPI-122 — Glu death: Blupi stuck (ACTION_GLU) for several frames then die
- [ ] BLUPI-123 — Charge death: enemy charge-hit animation (ACTION_CHARGE)
- [ ] BLUPI-124 — Ouf recovery: after close call, play one of Ouf1a..Ouf5 animations
- [ ] BLUPI-125 — Mockery: enemies mock Blupi (ACTION_MOCKERY/i/p) for m_blupiTimeMockery ticks
- [ ] BLUPI-126 — Stomp kill: velY < -1.0 on contact with enemy → BounceUp() + kill enemy — blocked on ENEMY-* work (no enemy hit detection in CNA yet)
- [ ] BLUPI-127 — BounceUp: upward impulse kJumpSpeed × 0.65

#### 4.7 Blupi Sounds

CNA's `GESound` loads all 93 real .wav files with the real per-channel volume/conflict table and
is wired to jump/land/footstep events, so the base plumbing exists — but per-item wiring below is
re-verified individually since it is not a full port yet.

- [x] BLUPI-128 — Jump sound: ch1 on jump (CNA, 2026-07-10)
- [x] BLUPI-129 — Footstep sound: ch3 per march stride (CNA, 2026-07-10 — plain footstep wiring only; surface-dependent remap NOT done, see BLUPI-133)
- [x] BLUPI-130 — Landing sound: ch4 on ground contact (CNA, 2026-07-10)
- [ ] BLUPI-131 — Stomp kill sound: ch5 — blocked on enemy stomp detection, see BLUPI-126
- [ ] BLUPI-132 — Death sound: ch8
- [ ] BLUPI-133 — Surface-specific footstep: SoundEnviron maps ch3/ch4 to ch78-91 based on tile type — NOT done in CNA (7 terrain pairs, channels 78-91)
- [ ] BLUPI-134 — Walk in water sound: ch36 (shallow water ambient)
- [ ] BLUPI-135 — Swim bubble sound: ch37
- [ ] BLUPI-136 — Glide sound: ch41
- [ ] BLUPI-137 — Teleport in/out sounds: ch9 / ch12
- [ ] BLUPI-138 — Shield-on sound: ch50 when shield activated
- [ ] BLUPI-139 — Shield-off sound: ch44 when shield expires
- [ ] BLUPI-140 — Dynamite pickup/place sounds: ch52
- [ ] BLUPI-141 — Tank fire sound: ch53
- [ ] BLUPI-142 — Switch activate: ch76/ch77 (off/on)
- [ ] BLUPI-143 — Rope suspend sounds: ch47 (attach), ch65 (detach)
- [ ] BLUPI-144 — Drink sound: ch58 on Drink action
- [ ] BLUPI-145 — Key pickup sound: ch11 — see PICKUP-004/005/006, PICKUP-071 (CNA has this wired at the pickup level)
- [ ] BLUPI-146 — Life / egg pickup sound: ch42 — see PICKUP-002/072, egg pickup uses ch3 per corrected channel table below, not ch42; re-verify against real `Decor.cpp`
- [ ] BLUPI-147 — Sucette pickup sound: ch62
- [ ] BLUPI-148 — Balloon motor sounds: ch28/ch30 (start/stop), ch29/ch31 (loop low/high)
- [ ] BLUPI-149 — Electro sounds: ch38 (long arc) / ch90 (spark)
- [ ] BLUPI-150 — Glu splash sounds: ch51
- [ ] BLUPI-151 — Water splash sounds: ch23 (small plouf), ch64 (tiplouf), ch24 (blup bubble)
- [ ] BLUPI-152 — Secret exit found sound: ch21
- [ ] BLUPI-153 — Door open sound: ch7

---

### 2.5 Tile Types & Terrain

CNA's terrain system is comparatively advanced: all 4 confirmed render modes shipped
(`DirectionalCube`, `InnerPillarBox`, `InnerFlatPlate`, `TripleCrossBillboard`) plus water as
alpha-blended cubes, ~175/175 confirmed icons, animated-tile timing using real per-type divisors,
`BigDecor` billboards, and face culling on the static terrain mesh. `ThinMechanical` mode geometry
(saws/springs/switches/fans/bridge/pipes/grates, ~25 icons) is a not-yet-made decision, not an
in-progress item.

#### 5.1 World Loading

- [x] TILE-001 — Load world from mobile-eggbert `.txt` format (100×100 grid) (CNA, 2026-07-10)
- [x] TILE-002 — 100×100 decor grid rendered as 3D geometry per the 4 confirmed render modes (CNA, 2026-07-10) — note: not "1 cube per tile" as originally phrased; CNA uses per-type render modes, not a uniform cube
- [x] TILE-003 — Tile textures from `object-m.png`, correct 65px-pitch atlas (65px = 64px icon + 1px gap, 1px leading margin) (CNA, 2026-07-10)
- [x] TILE-004 — Correct tile passability distinguishing decorative vs. solid tiles (CNA, 2026-07-10)
- [ ] TILE-005 — 5 worlds (Grassland, Forest, Ice Caves, Lava Fields, Space Station) hand-authored as real 3D `.vwr` worlds — only a small sample world exists so far, not all 5
- [ ] TILE-006 — Level progression: win → next world, wraps at world 5
- [ ] TILE-007 — Terrain depth fill (cliff edges extrude dark fill blocks downward) *(3D)*
- [ ] TILE-008 — Sky dome per world (`backgrounds/decorNNN.png`)
- [ ] TILE-009 — Per-world sky colour (ambient + fog)
- [x] TILE-010 — Blupi spawn position parsed from world data (CNA, 2026-07-10 — collision point spawn only, no visible Blupi, see BLUPI-078)
- [ ] TILE-011 — `region=` header parsed → background texture selection
- [ ] TILE-012 — `music=` header parsed → ambient music track

#### 5.2 Animated Tiles

CNA uses real per-type divisors from `Decor.cpp Config::ScaleDiv()`, not a uniform tick rate:
Saw/Fan div 1 (50ms), Lava div 2 (100ms), Water1/Crusher/Water2/Marine div 3 (150ms), Spike/Temp
div 4 (200ms). Any task below that previously assumed a uniform "6 fps" animation rate is corrected
accordingly.

- [x] TILE-013 — Per-type animation phase timing using real `ScaleDiv()` divisors, not a uniform tick counter (CNA, 2026-07-10 — corrected from the old uniform-6fps assumption)
- [x] TILE-014 — Lava tiles (icon 68, 8-frame: {68,69,70,71,72,71,70,69}), div 2 / 100ms (CNA, 2026-07-10) — kill-on-contact behavior itself blocked on hazard/lives system, see §6/§8
- [x] TILE-015 — Crusher tiles (10-frame: {317..323...}), div 3 / 150ms (CNA, 2026-07-10 — animation only; kill-in-frames-5-9 hazard logic NOT done)
- [x] TILE-016 — Saw tiles (6-frame: {378..383}), div 1 / 50ms (CNA, 2026-07-10 — animation only; kill-on-contact NOT done)
- [x] TILE-017 — Spike tiles (16-frame: table_decor_piege1), div 4 / 200ms (CNA, 2026-07-10 — animation only; kill-on-contact NOT done)
- [x] TILE-018 — Water1 tiles (6-frame: {92..95,94,93}), div 3 / 150ms (CNA, 2026-07-10 — animated decoration, alpha-blended cube, not wavy-edge surface)
- [x] TILE-019 — Water2 tiles (6-frame: {91,96..98,97,96}), div 3 / 150ms (CNA, 2026-07-10 — same caveat as TILE-018)
- [ ] TILE-020 — Ventilator/fan Up tiles (icons 126-128, 3-frame: table_decor_ventillog)
- [ ] TILE-021 — Ventilator/fan Down tiles (icons 129-131, 3-frame: table_decor_ventillod)
- [ ] TILE-022 — Ventilator/fan Right tiles (icons 132-134, 3-frame: table_decor_ventilloh)
- [ ] TILE-023 — Ventilator/fan Left tiles (icons 135-137, 3-frame: table_decor_ventillob)
- [ ] TILE-024 — Water drip tiles (icons: table_decor_goutte, 48-frame)
- [ ] TILE-025 — Temperature tile animation (table_decor_temp, 20-frame), div 4 / 200ms
- [ ] TILE-026 — Marine tile (icon 203: table_marine, 11 frames, Object channel), div 3 / 150ms
- [x] TILE-027 — Terrain renderer's per-frame animation-phase update loop covers all animated tile types (CNA, 2026-07-10)

#### 5.3 Interactive / Hazard Tiles

None of these have hazard/gameplay logic wired in CNA yet — only static/animated rendering exists
where noted in §5.2. Blupi's collision does not test hazard tiles at all yet.

- [ ] TILE-028 — Lava (icon 68-72): kill Blupi on contact (IsLave)
- [ ] TILE-029 — Spike (icon 373/347): kill Blupi on contact (IsPiege)
- [ ] TILE-030 — Crusher (icon 317-323): kill only when fully extended (IsEcraseur, phase 5-9)
- [ ] TILE-031 — Saw (icon 378-383): kill Blupi on contact (IsScie)
- [ ] TILE-032 — Water drip (IsGoutte): triggers glu/slow effect when hit
- [ ] TILE-033 — Blitz/lightning tile (IsBlitz): electric instant death
- [ ] TILE-034 — Spring/ressort tile (IsRessort): launch Blupi upward
- [ ] TILE-035 — Temp tile (IsTemp): brief passability change (bridge-like)
- [ ] TILE-036 — Door tile (IsDoor): locked door, opened by matching key (DoorKeyFlags) — note: closed doors should render as `Billboard` (red pillar/bollard shape), NOT `UniformCube`
- [ ] TILE-037 — Teleporter tile (IsTeleporte / SearchTeleporte): pair of tiles, teleport Blupi
- [ ] TILE-038 — Switch tile (IsSwitch / ActiveSwitch): toggles state of linked door/bridge
- [ ] TILE-039 — Bridge tile (IsBridge): builds a bridge (ObjectType52 animation)
- [ ] TILE-040 — Ventilator tile (IsVentillo): blows Blupi in direction when standing in fan stream
- [ ] TILE-041 — Normal jump tile (IsNormalJump): forces a jump when stepped on
- [ ] TILE-042 — Water surface (IsSurfWater): enter surf mode
- [ ] TILE-043 — Deep water (IsDeepWater): enter swim/drown mode
- [ ] TILE-044 — Out-of-water exit (IsOutWater): exit swim mode when reaching dry tile
- [ ] TILE-045 — Barre / barrier tile (GetTypeBarre): blocks certain vehicle types

#### 5.4 Tile Adaptation (visual smoothing)

- [ ] TILE-046 — `table_adapt_decor` (144 entries): smooth corner blending based on neighbour mask
- [ ] TILE-047 — `table_adapt_fromage` (32 entries): cheese tile corner blending
- [ ] TILE-048 — `table_decor_quart` (7056 entries): full tile replacement lookup by neighbour mask

#### 5.5 Background & Sky

- [ ] TILE-049 — Background sky PNG per region (`decor000.png`..`decor031.png`, not all consecutive)
- [ ] TILE-050 — 5 sky colour palettes (ambient + fog per world region)
- [ ] TILE-051 — Per-zone fog colour changes mid-level (region changes between areas)
- [ ] TILE-052 — Lightning tile visual effect (icon 66-68 drawn 13 px higher, ch69 sound)

#### 5.6 Open Rendering-Mode Decisions (CNA-specific, new)

- [ ] TILE-053 — Decide and implement `ThinMechanical` render-mode geometry for saws/springs/switches/fans/bridge/pipes/grates (~25 icons) — decision not yet made
- [ ] TILE-054 — Distinct water/liquid surface treatment (wavy-edge surface) to replace the current alpha-blended-cube placeholder
- [ ] TILE-055 — "Thin-bar" new geometry for icon 202
- [ ] TILE-056 — Architectural kit modular assembly
- [ ] TILE-057 — Secret-power (Sp0-7) billboard rendering and behavior — behavior itself is undocumented anywhere, research needed first

---

### 2.6 Enemy AI

**Not started in CNA at all** — no enemy hit/stomp/hazard detection of any kind exists yet; this is
explicitly deferred pending a lives/gauge system (see §8). All items reset to `[ ]`.

#### 6.1 Common Enemy Behaviour

- [ ] ENEMY-001 — Patrol movement: oscillate between posStart and posEnd at constant speed
- [ ] ENEMY-002 — Stationary enemies get ±2 tile default patrol range
- [ ] ENEMY-003 — Directional sprites: flipX when moving right (`table_mirror`)
- [ ] ENEMY-004 — Stomp kills all enemy types on velY < -1.0 contact
- [ ] ENEMY-005 — Enemy respawns at posStart after 5 s (kill timer)
- [ ] ENEMY-006 — Blob shadow under enemies *(3D adaptation)*
- [ ] ENEMY-007 — Y-proximity check: aerial enemies don't hit ground-level Blupi
- [ ] ENEMY-008 — `MoveObjectStepLine`: advance/recede speed + end-dwell timer logic
- [ ] ENEMY-009 — `MoveObjectStepIcon`: per-type animation phase counter update — note: object animation rate is NOT confirmed to be throttled to 6fps as older assumptions had it; re-verify against real `Decor.cpp` before implementing

#### 6.2 Per-Type Enemy Implementation

- [ ] ENEMY-010 — ObjectType2: patrol enemy A (table_robot_left/right, icons 12-20 in element.png)
- [ ] ENEMY-011 — ObjectType3: patrol enemy B (icons 48-56 in element.png)
- [ ] ENEMY-012 — ObjectType4: bulldozer (table_bulldozer_left/right, turn2l/r)
- [ ] ENEMY-013 — ObjectType4: bulldozer charge behaviour on Blupi contact (distinct from simple patrol)
- [ ] ENEMY-014 — ObjectType16: spider (icons 69-77, vertical oscillation hang↔drop)
- [ ] ENEMY-015 — ObjectType17: fish (table_poisson_left/right, patrol in water)
- [ ] ENEMY-016 — ObjectType17: turn animation (table_poisson_turn2l/r, 48 frames each)
- [ ] ENEMY-017 — ObjectType20: bird (table_oiseau_left/right, aerial Y=3.0 patrol)
- [ ] ENEMY-018 — ObjectType20: turn animation (table_oiseau_turn2l/r, 10 frames each)
- [ ] ENEMY-019 — ObjectType33: blupit (table_blupit_left/right)
- [ ] ENEMY-020 — ObjectType33: blupit fires ObjectType23 projectile at phase 3 and phase 21 during turn
- [ ] ENEMY-021 — ObjectType32: blupih (table_blupih_left/right, turn2l/r)
- [ ] ENEMY-022 — ObjectType32: blupih fires ObjectType23 projectile during turn animation
- [ ] ENEMY-023 — ObjectType44: wasp/bee (table_guepe_left/right, 6-frame, fast patrol)
- [ ] ENEMY-024 — ObjectType44: turn animation (table_guepe_turn2l/r, 5 frames)
- [ ] ENEMY-025 — ObjectType54: creature (table_creature_left/right, 8-frame, slow patrol)
- [ ] ENEMY-026 — ObjectType54: long turn animation (table_creature_turn2, 152 frames)
- [ ] ENEMY-027 — ObjectType54: destroys Blupi's helicopter on contact (ByeByeHelico triggered)
- [ ] ENEMY-028 — ObjectType18: additional patrol enemy variant (sprite/behaviour TBD from Decor.cpp)
- [ ] ENEMY-029 — ObjectType96: follow enemy 1 (table_follow1, 26 frames) — chases Blupi
- [ ] ENEMY-030 — ObjectType97: follow enemy 2 (table_follow2, 5 frames) — tracks exact position
- [ ] ENEMY-031 — ObjectType96/97: MoveObjectFollow() logic — path toward Blupi

Note: `ObjectType32`/`33` (blupih/blupit) need `blupi1.png`, not `element.png` — if any prior
assumption used a single shared sprite sheet for all enemies, that assumption is wrong; see
ObjectType-to-sheet corrections repeated in §7.

#### 6.3 Projectiles

- [ ] ENEMY-032 — ObjectType23: fired projectile (icon 176 from element.png) — spawned by blupih/blupit
- [ ] ENEMY-033 — Projectile travels toward Blupi position, expires after 55 frames
- [ ] ENEMY-034 — Projectile hit detection: damage Blupi if shield inactive
- [ ] ENEMY-035 — Projectile sound (ch27 on fire?)

#### 6.4 Enemy Sounds

- [ ] ENEMY-036 — Stomp kill sound: ch5
- [ ] ENEMY-037 — Bulldozer turn sound (ch33)
- [ ] ENEMY-038 — Enemy destruction sound varies by type
- [ ] ENEMY-039 — Wasp/bee movement sound (ch72/ch73)
- [ ] ENEMY-040 — Creature movement sound

---

### 2.7 Pickups & Objects

CNA has a working first interactive-object system (`GEInteractionSystem`): treasure(5)/egg(6)/
exit(7)/keys(49-51) pickup collection works with real removal-on-contact semantics, real sound
channels (11/19 for treasure/key, 3 for egg — corrected below), `MAX_EGG_COUNT=10` cap, and exit
gated on treasures-collected. Crates/lifts also have real substance: `ObjectType1/12/47/48` render
as real cubes; platform lifts patrol (ping-pong between posStart/posEnd); crates can be pushed
(X-axis only, single-crate, with adjacency/floor-support/occupancy checks). Everything else in this
section is not yet done.

**Sound-channel correction, applies throughout this section:** treasure/key pickup = Channel 11 (or
19 if the pickup completes a set), egg = Channel 3 — not the older generic "ch42 collect" /
"ch10 always-restarts" assumptions used below in some line items; where a line item's channel
number below conflicts with this, the corrected value governs. **Sprite-sheet correction:**
`ObjectType1`/`12` need `object-m.png`, not `element.png`; `ObjectType32`/`33` need `blupi1.png`,
not `element.png` — any task assuming one shared sheet for all pickups/objects is wrong.

#### 7.1 Static Collectibles

- [x] PICKUP-001 — ObjectType5: treasure — collection/removal-on-contact works, required for exit gating (CNA, 2026-07-10) — score/HUD-fly-animation still TBD, see PICKUP-049/HUD-025
- [x] PICKUP-002 — ObjectType6: egg — collection works, MAX_EGG_COUNT=10 cap (CNA, 2026-07-10) — note: real channel is ch3, not ch42 as originally listed
- [x] PICKUP-003 — ObjectType7: exit goal — gated on treasures-collected (CNA, 2026-07-10)
- [x] PICKUP-004 — ObjectType49: red key — sets Key1 flag, real sound channel (11, or 19 if set-completing) (CNA, 2026-07-10)
- [x] PICKUP-005 — ObjectType50: green key — sets Key2 flag, same channel correction as PICKUP-004 (CNA, 2026-07-10)
- [x] PICKUP-006 — ObjectType51: blue key — sets Key3 flag, same channel correction as PICKUP-004 (CNA, 2026-07-10)
- [ ] PICKUP-007 — ObjectType25: shield orb (table_shield, 16 frames) — 5 s invincibility, ch50
- [ ] PICKUP-008 — ObjectType30: drink (icon 178) — +1 life (cap 9), ch42 sound (re-verify channel against real Decor.cpp, per correction note above)
- [ ] PICKUP-009 — ObjectType21: secret exit (table_cle, 12 frames) — sets m_bFoundCle, triggers Win
- [ ] PICKUP-010 — ObjectType31: cloud power-up (table_charge, 6 frames, Object channel) — m_blupiCloud 100 ticks
- [ ] PICKUP-011 — ObjectType40: invert power-up (table_invert, 20 frames) — m_blupiInvert 100 ticks + particle burst
- [ ] PICKUP-012 — ObjectType26: suction-cup (table_power, 8 frames) — ACTION_Sucette, wall climbing
- [ ] PICKUP-013 — ObjectType29: bullet ammo (icon 177) — +10 bullets to m_blupiBullet
- [ ] PICKUP-014 — ObjectType55: dynamite (icon 252) — ACTION_TakeDynamite
- [ ] PICKUP-015 — ObjectType13: helicopter (icon 68 in element.png) — sets m_blupiHelico
- [ ] PICKUP-016 — ObjectType19: jeep (icon 89) — sets m_blupiJeep
- [ ] PICKUP-017 — ObjectType28: tank (icon 167) — sets m_blupiTank
- [ ] PICKUP-018 — ObjectType24: skateboard (table_skate, 34 frames) — ACTION_TakeSkate
- [ ] PICKUP-019 — ObjectType46: balloon (icon 208) — sets m_blupiOver / m_blupiBalloon

#### 7.2 Platform Lifts

- [x] PICKUP-020 — ObjectType1: platform lift patrols posStart↔posEnd (CNA, 2026-07-10) — note: does NOT yet carry Blupi; Blupi's collision doesn't test moving objects at all, see PICKUP-023
- [ ] PICKUP-021 — ObjectType47: platform lift rightward carry (+2 px/frame horizontal to Blupi when riding) — blocked on PICKUP-023 (platform boarding/riding)
- [ ] PICKUP-022 — ObjectType48: platform lift leftward carry (-2 px/frame horizontal) — same blocker
- [ ] PICKUP-023 — Platform boarding/riding: Blupi's collision must test moving objects, not just static terrain — NOT done, root blocker for this whole subsection
- [ ] PICKUP-024 — AscenseurVertigo: Blupi hangs on edge of platform (Vertigo state)
- [ ] PICKUP-025 — AscenseurShift: shift Blupi with moving platform
- [ ] PICKUP-026 — AscenseurSynchro: synchronise multiple lifts
- [ ] PICKUP-027 — m_blupiTimeNoAsc: cooldown preventing immediate re-entry

#### 7.3 Crates (ObjectType12)

- [x] PICKUP-028 — ObjectType12: crate renders as a real cube (CNA, 2026-07-10) — was previously a static billboard placeholder in Simple3D; CNA's is a real `UniformCube`
- [x] PICKUP-029 — Crate push: walking into a crate pushes it 1 tile, X-axis only, single-crate (CNA, 2026-07-10) — Y-axis / non-X pushing NOT done
- [x] PICKUP-030 — Crate stops when hitting a wall or another crate (adjacency/floor-support/occupancy checks) (CNA, 2026-07-10)
- [ ] PICKUP-031 — Crates can be stacked and linked crates push as a group (flood-fill; UpdateCaisse / SearchLinkCaisse) — NOT done, current push is single-crate only
- [ ] PICKUP-032 — m_rankCaisse / m_nbRankCaisse: array of crate object indices
- [ ] PICKUP-033 — TestPushCaisse: check if push is valid (clear path)
- [ ] PICKUP-034 — CaisseInFront: detect crate directly in front of Blupi
- [ ] PICKUP-035 — SmallShake on crate land / impact
- [ ] PICKUP-035b — Conveyor nudge for ObjectType47/48 (new item, CNA-specific gap noted in project status: crates on conveyor-lift tiles should be nudged, not yet implemented)

#### 7.4 Doors & Keys

- [ ] PICKUP-036 — DoorKeyFlags: 3-bit flag (Key1 / Key2 / Key3)
- [ ] PICKUP-037 — Door tile (IsDoor): opens when Blupi touches and holds matching key — render mode correction: closed doors are `Billboard` (red pillar/bollard), NOT `UniformCube`
- [ ] PICKUP-038 — InitializeDoors: restore door states from GameData on level load
- [ ] PICKUP-039 — MemorizeDoors: save door states to GameData on level exit
- [ ] PICKUP-040 — Door open animation: ObjectType22 (3-phase animation, self-removes)
- [ ] PICKUP-041 — Door open sound: ch7

#### 7.5 Visual Effects (transient objects)

- [ ] PICKUP-042 — ObjectType8: primary explosion (table_explo1, explo.png Explosion channel)
- [ ] PICKUP-043 — ObjectType9: small explosion (table_explo2, 20 frames)
- [ ] PICKUP-044 — ObjectType10: tertiary explosion (table_explo3, 20 frames)
- [ ] PICKUP-045 — ObjectType11: fan shockwave (table_explo4, 9 frames) — triggers BigShake
- [ ] PICKUP-046 — ObjectType36: pollution puff (table_pollution, 8 frames, 16-tick lifetime)
- [ ] PICKUP-047 — ObjectType37: clear effect (table_clear, 70 frames)
- [ ] PICKUP-048 — ObjectType38: electric arc (table_electro, 90 frames, starts Blupi1_12 channel)
- [ ] PICKUP-049 — ObjectType39: treasure sparkle (table_tresortrack, 11 frames) — spawned on pickup; part of the not-yet-done voyage-deferred HUD-fly animation (current impl deletes/counts pickups immediately instead of deferring to a flight animation)
- [ ] PICKUP-050 — ObjectType41/42: invert start/stop particles (table_invertstart/stop, 8 frames × 4 dirs)
- [ ] PICKUP-051 — ObjectType53: tentacle hazard (table_tentacule, 45 frames, 90-tick lifetime)
- [ ] PICKUP-052 — ObjectType57: shield trail (table_shieldtrack, 20 frames)
- [ ] PICKUP-053 — ObjectType58: shield disappear (20 frames)
- [ ] PICKUP-054 — ObjectType27: magic track sparkle (table_magictrack, 24 frames)
- [ ] PICKUP-055 — ObjectType90: electric spark (table_explo5, 12 frames) — triggers ElectricShake
- [ ] PICKUP-056 — ObjectType91: small flash (table_explo6, 6 frames)
- [ ] PICKUP-057 — ObjectType92: long energy arc (table_explo7, 128 frames)
- [ ] PICKUP-058 — ObjectType93: tiny flash (table_explo8, 5 frames)
- [ ] PICKUP-059 — ObjectType98/99/100: water splashes (table_sploutch1/2/3, 10/13/18 frames)
- [ ] PICKUP-060 — ObjectType14: water plouf (table_plouf, 7 frames, Object channel)
- [ ] PICKUP-061 — ObjectType15: water bubble (table_blup, 20 frames) — rises to waypoint
- [ ] PICKUP-062 — ObjectType34: goo particle (table_glu, 25-frame looping element) — sticks to geometry
- [ ] PICKUP-063 — ObjectType35: small plouf (table_tiplouf, 3 frames)

#### 7.6 Special Level Objects

- [ ] PICKUP-064 — ObjectType52: bridge construction (table_bridge, 157 frames) — also updates static decor
- [ ] PICKUP-065 — ObjectType56: dynamite fuse (table_dynamitef, 100 frames) — triggers DynamiteStart() at phases 50-69
- [ ] PICKUP-066 — DynamiteStart: blast clears tiles in all 4 directions (radius-based)
- [ ] PICKUP-067 — ObjectType200-203: Blupi avatar skins (icons 257-262 on respective channels)
- [ ] PICKUP-068 — ObjectType200: costume select pickup → triggers player-select voyage when touched
- [ ] PICKUP-069 — ObjectType201-203: damage Blupi on contact if shield/hide/SuperBlupi inactive

#### 7.7 Pickup Sounds

Per the channel correction above (treasure/key = ch11 or ch19 if set-completing; egg = ch3), the
line items below are corrected in place rather than reset blindly to old (possibly wrong) channel
numbers.

- [x] PICKUP-070 — Treasure collect sound wired: ch11 (ch19 if set-completing) (CNA, 2026-07-10) — corrected from the old "ch10 always restarts" assumption
- [x] PICKUP-071 — Key pickup sound wired: ch11 (ch19 if set-completing) (CNA, 2026-07-10) — same channel as treasure, corrected from old ch11-only assumption (still ch11, but conflict/set-completing behavior added)
- [x] PICKUP-072 — Egg pickup sound wired: ch3 (CNA, 2026-07-10) — corrected from the old "ch42" assumption; drink pickup (PICKUP-008) channel still needs separate verification
- [ ] PICKUP-073 — Shield pickup: ch50
- [ ] PICKUP-074 — Win/exit sound: ch57
- [ ] PICKUP-075 — Door open: ch7
- [ ] PICKUP-076 — Switch activate (on): ch77; switch deactivate: ch76
- [ ] PICKUP-077 — Explosion sounds: ch40 (explosion); re-verify ch10/ch39 assignments against real Decor.cpp given the treasure/key channel correction above
- [ ] PICKUP-078 — Water plouf: ch23
- [ ] PICKUP-079 — Water bubble: ch24
- [ ] PICKUP-080 — Water small plouf: ch64
- [ ] PICKUP-081 — Glu/glue sound: ch51
- [ ] PICKUP-082 — Dynamite fuse sounds: ch52 (placement / explosions)
- [ ] PICKUP-083 — Secret exit pickup: ch21
- [ ] PICKUP-084 — Bridge construction sound: ch20
- [ ] PICKUP-085 — Balloon pickup sound: ch46
- [ ] PICKUP-086 — Shield trail sound: ch48
- [ ] PICKUP-087 — Shield loop sound: ch49 (looped while shield active)

---

### 2.8 Score & Progression

Not started in CNA beyond what's implied by the interactive-object system's counting (treasures
collected, egg cap). No score numbers, HUD display, win/lose screen, or save-linked progression
exist yet. All items reset to `[ ]`.

- [ ] SCORE-001 — +10 score per treasure collected
- [ ] SCORE-002 — +25 score per enemy stomped
- [ ] SCORE-003 — +50 score per key collected
- [ ] SCORE-004 — +50 score per egg collected (mobile-eggbert: ch3 + life + score — channel corrected, see §7 note)
- [ ] SCORE-005 — +50 score per drink collected
- [ ] SCORE-006 — +100 bonus when all treasures collected (all-treasures bonus)
- [ ] SCORE-007 — High score per gamer slot persisted
- [ ] SCORE-008 — Level elapsed timer displayed in HUD
- [ ] SCORE-009 — Game speed selector: G key cycles Slow(0.6×) → Normal(1.0×) → Fast(1.5×)
- [ ] SCORE-010 — GameSpeed::Faster and GameSpeed::Fastest modes (from mobile-eggbert enum)
- [ ] SCORE-011 — Slow game speed: alternate-frame skip (`slow_frame` toggle in game loop)
- [ ] SCORE-012 — Win screen: display total score, elapsed time, new-record indicator
- [ ] SCORE-013 — Mission numbering: world hub (X0) → levels (X1-X5) → next hub ((X+1)0)
- [ ] SCORE-014 — 78 world files (world001.txt … world055.txt + hubs) supported
- [ ] SCORE-015 — IsTerminated: -1=lost, -2=win, ≥1=advance to mission N
- [ ] SCORE-016 — Mission advance: m_term = m_mission/10*10 + next_level_in_world
- [ ] SCORE-017 — Hub mission (mission % 10 == 0): no treasure counter in HUD
- [ ] SCORE-018 — Training missions (11-14): show tutorial hint overlay
- [ ] SCORE-019 — MemorizeGamerProgress: save lives and doors after each win/loss
- [ ] SCORE-020 — LastWorld: updated when completing a hub (mission divisible by 10)

---

### 2.9 Sound System

CNA has real substance here: `GESound` loads all 93 real .wav files via CNA's own
`SoundEffect`/`SoundEffectInstance` API, reuses the real per-channel volume/conflict table, and is
wired to jump/land/footstep events. Not done: pitch application, `SoundEnviron()` terrain-specific
footstep/bump remapping (7 terrain pairs, channels 78-91), idle "fidget" periodic sounds (channels
36/37/46-49/65), buff activate/expire-warning channel pairs.

- [x] SOUND-001 — 93 WAV files (`sounds/sound000.wav`..`sound092.wav`) loaded (CNA, 2026-07-10)
- [x] SOUND-002 — Per-channel volume/conflict table reused from the real data (CNA, 2026-07-10)
- [ ] SOUND-003 — Sound on/off toggle (persisted in GameData byte 3) — blocked on §11 Save Data, not started
- [x] SOUND-004 — Sound loop support via `SoundEffectInstance` (CNA, 2026-07-10)
- [ ] SOUND-005 — Positional (panned) audio: volume/balance based on screen X position (SoundEnviron)
- [ ] SOUND-006 — SoundEnviron: maps ch3/ch4 footstep to tile-surface variant (ch78-91) — NOT done (7 terrain pairs)
- [ ] SOUND-007 — Vehicle motor loop: ch16/ch18 (helicopter high/low), ch29/ch31 (jeep/tank/over)
- [ ] SOUND-008 — Motor sound crossfade: start sound (ch15/ch28) + stop sound (ch17/ch30)
- [ ] SOUND-009 — PosSound: update panned position of active motor loop each frame
- [ ] SOUND-010 — Ambient sound: all 72 gameplay channels wired to correct game events (see full list below)
- [ ] SOUND-010b — Pitch application per tableVolumePitch (new item — noted explicitly as NOT done in CNA despite the volume/conflict table being reused)
- [ ] SOUND-010c — Idle "fidget" periodic sounds (channels 36/37/46-49/65) (new item — explicitly NOT done)
- [ ] SOUND-010d — Buff activate/expire-warning channel pairs (new item — explicitly NOT done)

#### Complete Sound Channel Wire-up (0=reserved, 1-92=game SFX)

Base loading/volume plumbing exists for all channels (SOUND-001/002); the marks below track
whether each channel is actually wired to a real game event yet. Corrected per §7: treasure/key =
ch11 (ch19 if set-completing), egg = ch3 (not ch42 as some entries below still assumed under the
old scheme — cross-check against §7 when wiring these).

- [x] SOUND-011 — ch1: jump (CNA, 2026-07-10)
- [ ] SOUND-012 — ch2: unknown (research needed)
- [x] SOUND-013 — ch3: footstep AND/OR egg pickup — note: ch3 is used for both footstep (BLUPI-129) and egg pickup (PICKUP-072) in the real data; confirm both wire-ups are distinct calls, not a collision (CNA, 2026-07-10 for footstep)
- [x] SOUND-014 — ch4: landing (CNA, 2026-07-10)
- [ ] SOUND-015 — ch5: stomp kill — blocked on enemy hit detection
- [ ] SOUND-016 — ch6: unknown
- [ ] SOUND-017 — ch7: door open
- [ ] SOUND-018 — ch8: death / hit
- [ ] SOUND-019 — ch9: teleport in
- [ ] SOUND-020 — ch10: collect (re-verify against corrected ch11/ch3 treasure/egg channels — may be unused/different purpose)
- [x] SOUND-021 — ch11: key pickup AND treasure pickup (CNA, 2026-07-10 — corrected: shared by both per real data)
- [ ] SOUND-022 — ch12: teleport out
- [ ] SOUND-023 — ch13: bridge build phase 1
- [ ] SOUND-024 — ch14: bridge build phase 2
- [ ] SOUND-025 — ch15: helicopter motor start
- [ ] SOUND-026 — ch16: helicopter motor high (loop)
- [ ] SOUND-027 — ch17: helicopter motor stop
- [ ] SOUND-028 — ch18: helicopter motor low (loop)
- [x] SOUND-029 — ch19: treasure/key pickup, set-completing variant (CNA, 2026-07-10 — new correct meaning, was previously listed as generic "teleport (alternate)")
- [ ] SOUND-030 — ch20: bridge completed
- [ ] SOUND-031 — ch21: secret exit found
- [ ] SOUND-032 — ch22: unknown
- [ ] SOUND-033 — ch23: water plouf
- [ ] SOUND-034 — ch24: water bubble rise
- [ ] SOUND-035 — ch25: unknown
- [ ] SOUND-036 — ch26: unknown
- [ ] SOUND-037 — ch27: projectile fired
- [ ] SOUND-038 — ch28: jeep/tank start
- [ ] SOUND-039 — ch29: jeep/tank motor high (loop)
- [ ] SOUND-040 — ch30: jeep/tank stop
- [ ] SOUND-041 — ch31: jeep/tank motor low (loop)
- [ ] SOUND-042 — ch32: unknown
- [ ] SOUND-043 — ch33: bulldozer turn
- [ ] SOUND-044 — ch34: unknown
- [ ] SOUND-045 — ch35: unknown
- [ ] SOUND-046 — ch36: water walk ambient (idle fidget channel, see SOUND-010c)
- [ ] SOUND-047 — ch37: swim bubble (idle fidget channel, see SOUND-010c)
- [ ] SOUND-048 — ch38: electric arc (long)
- [ ] SOUND-049 — ch39: key sparkle effect
- [ ] SOUND-050 — ch40: explosion
- [ ] SOUND-051 — ch41: glide
- [ ] SOUND-052 — ch42: life/drink pickup (egg corrected to ch3, see PICKUP-072 — ch42 role needs re-verification, may be drink-only)
- [ ] SOUND-053 — ch43: unknown
- [ ] SOUND-054 — ch44: shield off
- [ ] SOUND-055 — ch45: unknown
- [ ] SOUND-056 — ch46: balloon mode sound (idle fidget channel, see SOUND-010c)
- [ ] SOUND-057 — ch47: suspend attach (idle fidget channel, see SOUND-010c)
- [ ] SOUND-058 — ch48: shield sparkle (idle fidget channel, see SOUND-010c)
- [ ] SOUND-059 — ch49: shield loop (looped while active) (idle fidget channel, see SOUND-010c)
- [ ] SOUND-060 — ch50: shield pickup
- [ ] SOUND-061 — ch51: glu/glue splash
- [ ] SOUND-062 — ch52: dynamite / impact
- [ ] SOUND-063 — ch53: tank fire
- [ ] SOUND-064 — ch54: long explosion (creature death?)
- [ ] SOUND-065 — ch55: unknown
- [ ] SOUND-066 — ch56: unknown
- [ ] SOUND-067 — ch57: exit open / win
- [ ] SOUND-068 — ch58: drink pickup
- [ ] SOUND-069 — ch59: unknown
- [ ] SOUND-070 — ch60: pickup/collect (variant)
- [ ] SOUND-071 — ch61: unknown
- [ ] SOUND-072 — ch62: sucette / suction-cup
- [ ] SOUND-073 — ch63: unknown
- [ ] SOUND-074 — ch64: small water plouf
- [ ] SOUND-075 — ch65: suspend detach / rope release (idle fidget channel, see SOUND-010c)
- [ ] SOUND-076 — ch66: unknown
- [ ] SOUND-077 — ch67: unknown
- [ ] SOUND-078 — ch68: unknown
- [ ] SOUND-079 — ch69: lightning strike
- [ ] SOUND-080 — ch70: unknown
- [ ] SOUND-081 — ch71: unknown
- [ ] SOUND-082 — ch72: wasp approach
- [ ] SOUND-083 — ch73: wasp attack
- [ ] SOUND-084 — ch74: teleport in (Blupi arrival)
- [ ] SOUND-085 — ch75: teleport out (Blupi exit)
- [ ] SOUND-086 — ch76: switch deactivate
- [ ] SOUND-087 — ch77: switch activate
- [ ] SOUND-088 — ch78-91: surface-specific footstep/landing variants (7 terrain pairs, mapped by SoundEnviron) — see SOUND-006
- [ ] SOUND-089 — ch92: follow-enemy sound
- [ ] SOUND-090 — Sound enable/disable respects enabled_ flag (all channels silenced when off)

---

### 2.10 Camera *(3D-specific)*

Camera is relatively more advanced than other CNA systems: first-person default plus third-person
toggle (real GPU-skinned placeholder model, "C" key) with exponential framerate-independent
damping already work.

- [x] CAM-001 — First-person default view with third-person toggle ("C" key, GPU-skinned placeholder model) (CNA, 2026-07-10) — note: replaces the old Simple3D-era "3rd-person orbit is the only mode" framing; CNA defaults first-person
- [ ] CAM-002 — RMB pitch control
- [ ] CAM-003 — Scroll-wheel zoom (smooth lerp)
- [ ] CAM-004 — Pitch auto-reset to a default angle when RMB released
- [ ] CAM-005 — Wall collision (DDA ray march from Blupi to desired camera position)
- [x] CAM-006 — Exponential, framerate-independent damping on camera follow (CNA, 2026-07-10)
- [ ] CAM-007 — FOV tuned/finalized (verify current FOV value against original 65° reference)
- [ ] CAM-008 — Camera shake: SmallShake (minor impacts: crate land, small explosions)
- [ ] CAM-009 — Camera shake: BigShake (fan-blade hit, large explosion) — triggered by ObjectType11
- [ ] CAM-010 — Camera shake: ElectricShake (ObjectType90 electric spark contact)
- [ ] CAM-011 — Camera shake: table_decor_action per-frame (dx, dy) offsets × 3 multiplier
- [ ] CAM-012 — Camera shake: fixed N-frame duration, self-clears to None after last frame
- [ ] CAM-013 — Camera shake implementation ported into CNA/Easy3D (StartShake equivalent)
- [ ] CAM-014 — HotSpot zoom: MoveHotSpot() eases camera zoom toward target
- [ ] CAM-015 — HotSpot target: m_hotSpotFinalZoom/X/Y interpolated over N frames
- [ ] CAM-016 — HotSpot: triggered on special events (secret exit found, level end zoom)
- [ ] CAM-017 — SCROLL_MARGX = 80 px / SCROLL_MARGY = 40 px viewport scroll margins (2D-era concept — evaluate whether a 3D-equivalent framing margin applies at all before implementing)
- [ ] CAM-018 — Smooth scroll: camera eases toward Blupi at SCROLL_SPEED = 8 px/tick (2D-era concept — likely superseded by CAM-006's damping; evaluate before implementing separately)

---

### 2.11 Save Data

**Not started at all in CNA.** Every item resets to `[ ]`; the old Simple3D save-format work does
not carry over as evidence of anything CNA does today.

- [ ] SAVE-001 — GameData: 640-byte flat binary format, binary-compatible with mobile-eggbert
- [ ] SAVE-002 — Global header (10 bytes): version, selectedGamer, sounds, jumpRight, autoZoom, accelActive
- [ ] SAVE-003 — 3 gamer slots × 210 bytes: lives (byte 0), lastWorld (byte 1), doors[200] (bytes 10-209)
- [ ] SAVE-004 — Auto-save on win / lost / quit / reset / gamer-select
- [ ] SAVE-005 — Persistence mechanism chosen and wired for CNA (CNA has no equivalent of Simple3D's `Simple3D::SaveData` yet — this needs its own CNA-appropriate API, not a straight port)
- [ ] SAVE-006 — doors[0..179]: secondary door states (180 secondary doors)
- [ ] SAVE-007 — doors[180..199]: main door states (20 main doors / hub worlds)
- [ ] SAVE-008 — GetGamerInfo: return lives, mainDoors, secondaryDoors per gamer slot
- [ ] SAVE-009 — CurrentWrite / CurrentRead: mid-game save/load (on app deactivate/activate)
- [ ] SAVE-010 — CurrentDelete: remove mid-game save (on OnExiting or normal level exit)
- [ ] SAVE-011 — Accelerometer sensitivity setting (byte 7, 0-100 → 0.0-1.0)
- [ ] SAVE-012 — JumpRight setting (byte 4) — jump direction preference
- [ ] SAVE-013 — AutoZoom setting (byte 5)
- [ ] SAVE-014 — Ranking mode persisted when isRankingMode is active

Reuse of the byte-level layout for save compatibility with mobile-eggbert is still an open question
(see `easy3d.md` §12 Q7) — do not assume SAVE-001's binary-compatible framing until that's decided.

---

### 2.12 Visual Polish *(3D-specific and faithful to mobile-eggbert)*

Everything here depends on systems (Blupi visibility, enemies, pickups, camera shake) that are
themselves mostly not started in CNA yet. All items reset to `[ ]`.

- [ ] VISUAL-001 — Blob shadow under Blupi (scales with height, disabled in helicopter/balloon)
- [ ] VISUAL-002 — Blob shadow under enemies (disabled for birds)
- [ ] VISUAL-003 — Pickup bobbing: sine-wave Y offset on collectibles
- [ ] VISUAL-004 — Score popups: rising "+N" text, 1 s fade at collection position
- [ ] VISUAL-005 — Respawn flash: Blupi billboard blinks at 10 Hz for 2 s after respawn
- [ ] VISUAL-006 — Shield tint: cyan sprite when m_blupiShield active
- [ ] VISUAL-007 — Shield blink at < 1.5 s remaining
- [ ] VISUAL-008 — Explosion billboard effects: ObjectType8-11 from `explo.png` (128×128 px, Explosion channel)
- [ ] VISUAL-009 — Water splash billboard effects: ObjectType98-100 from `explo.png`
- [ ] VISUAL-010 — Electric arc: ObjectType92 long arc from `explo.png` (128 frames)
- [ ] VISUAL-011 — Shield sparkle loop: ObjectType57 trail behind Blupi while shielded
- [ ] VISUAL-012 — Treasure sparkle: ObjectType39 on each treasure pickup
- [ ] VISUAL-013 — Pollution puff: ObjectType36 on environmental triggers
- [ ] VISUAL-014 — Invert power-up particles: ObjectType41 (4-direction burst on pickup)
- [ ] VISUAL-015 — Invert expire particles: ObjectType42 (4-direction burst on expiry)
- [ ] VISUAL-016 — Goo particle: ObjectType34 sticks to geometry (element.png, 25 frames)
- [ ] VISUAL-017 — Magic track sparkle: ObjectType27 trail effect
- [ ] VISUAL-018 — Helicopter debris: ByeByeHelico float-based debris pool when helico destroyed
- [ ] VISUAL-019 — Bridge construction animation: ObjectType52 (157 frames) modifies static decor
- [ ] VISUAL-020 — Dynamite fuse animation: ObjectType56 (100 frames) with blast events at phases 50-69
- [ ] VISUAL-021 — Tentacle hazard animation: ObjectType53 (45 frames, explo.png)
- [ ] VISUAL-022 — Sky gradient per world region (zenith/horizon colours)
- [ ] VISUAL-023 — Per-world fog (fog color + fog range per region)
- [ ] VISUAL-024 — Lightning visual: tiles 66-68 draw 13 px higher; ch69 sound
- [ ] VISUAL-025 — "EXIT OPEN!" text pop-up with sparkle effect when exit unlocks

---

### 2.13 Tests & Quality

- [x] TEST-001 — `GalaxyEggbertWorldsTests`: engine-independent unit tests (BlockTests, BitPackingTests, ChunkTests, WorldTests, BlockMetadataTest) — re-verify current pass count in this build (documented elsewhere as 63/63; a raw `TEST(...)` grep in this pass counted 60 macros, likely a counting-method difference, not a regression — reconcile before quoting a number in docs)
- [ ] TEST-002 — ctest discovery in the CNA build dir (gtest_discover_tests fix)
- [ ] TEST-003 — Test: all mobile-eggbert world files parse without error
- [ ] TEST-004 — Test: `BlockTypes::tileUV` returns valid UV for all known icon IDs, including the corrected 65px-pitch/1px-gap math (see plan §1 history / NEXT.md for the tileUV pitch bug)
- [ ] TEST-005 — Test: `GEWorldRuntime::LoadFromMobileEggbertFile` round-trip
- [ ] TEST-006 — Test: GameData read/write round-trip (640-byte format) — blocked on §11 Save Data being started at all
- [ ] TEST-007 — Test: animation-phase timing matches the real per-type `ScaleDiv()` divisors (Saw/Fan div 1, Lava div 2, Water1/Crusher/Water2/Marine div 3, Spike/Temp div 4) — supersedes the old "matches mobile-eggbert table indices at known times" framing, which assumed a uniform rate
- [ ] TEST-008 — Test (new): `GEInteractionSystem` — treasure/egg/exit/key pickup collection, removal-on-contact, MAX_EGG_COUNT=10 cap, exit gating on treasures-collected (covers the same ground as the existing `VerifyInteractionSystem` tool, but as an automated/CI-checked test rather than a manual verification binary)
- [ ] TEST-009 — Test (new): crate push validity (adjacency/floor-support/occupancy checks) and platform-lift ping-pong patrol motion, independent of the manual `VerifyMoveObjectTypesCna`/`VerifyBlupiMovement` tools
- [ ] TEST-010 — Test (new): `BigDecor` billboard parsing round-trip, independent of the manual `VerifyBigDecorParsingCna` tool

## 3. Open Questions

Carried forward from prior research passes; none resolved yet, listed here so future work
doesn't silently re-open them or silently guess an answer:

- `[?]` **`Config::ScaleTime()`'s real scale factor** — unresolved anywhere in the research so
  far. Blocks precise conversion of tick-domain durations to real seconds across multiple
  systems: footstep interval (`E3D-MIG-083`), door-slide duration (`E3D-MIG-160`), teleporter
  delay (`E3D-MIG-147`), dynamite fuse (`E3D-MIG-155`).
- `[?]` **`explo1`-`explo8` → `ObjectType` trigger mapping** — which explosion animation fires
  for which object/hazard is many-to-one or context-dependent; not traced anywhere yet.
- `[?]` **Icon 95** — ambiguous, boundary-only reference in mobile-eggbert source, intentionally
  left unresolved by the reference documentation.
- `[?]` **`E3D-MIG-015`**: whether to ask mobile-eggbert maintainers for a future
  `add_library()` target covering `Tables`/`Def`/`GameData`/`ObjectType`/`SoundChannel` — still
  open, would need explicit user approval as a separate task even if pursued.
- `[?]` **Save-format byte compatibility** (`easy3d.md` §12 Q7) — whether galaxy-eggbert should
  ever read/write mobile-eggbert's real save byte layout, vs. a fresh format. Not a default
  either way; decide when `E3D-MIG-106` is actually scoped.
- `[?]` **Secret power (`Sp0`-`Sp7`) behavior** — unclassified in every reference document so
  far; needs dedicated research before `E3D-MIG-170` can be implemented (only the static
  render, `E3D-MIG-515`, is currently plannable without it).
- `[?]` **Enemy billboard walk-cycle direction mismatch** — enemy sprites only have left/right
  side-view frames; no resolution proposed for how they should look when viewed at an oblique
  angle in true 3D (`E3D-MIG-179`). The equivalent question for Blupi himself is **resolved**
  (2026-07-10): billboard rejected outright, real 3D model required instead (`E3D-MIG-069`) —
  but enemies don't have that option yet (no enemy 3D models exist or are planned), so this
  remains genuinely open for them.
- The **7 partial-support `ObjectType`s** (jeep/secret-exit/skateboard/suction-cup/mirror/
  balloon/dynamite) already have fully-documented behavior (see `13-object-pickups.md`) and are
  simply not yet prioritized — folded into `E3D-MIG-171`-`176`, not a research gap, just an
  ordering decision.

## 4. Documentation Status

- `mobile-eggbert-reference/00-overview.md` through `15-3d-render-mapping-design.md` — complete,
  approved 2026-07-05 as a prose behavioral specification (not pseudocode, not verbatim
  transcription). This is the primary source of truth for *behavior* going forward; this plan
  intentionally does not re-duplicate constants already documented there beyond what's needed
  for task-level scoping.
- Tile-identification questionnaires (`questionnaire-*.md`) — complete as of the round-3 pass
  (97 previously unused/unnamed icons resolved); no further identification rounds are planned
  unless a specific icon's behavior is later found to be wrong.
- The old `DOC-001`-`DOC-007` documentation-tracking checklist and the ~940-line `DOC-100`-
  `DOC-267` documentation-regeneration bug-fix log have been removed from this file as pure
  historical logs with no forward-looking content — available in git history (any commit before
  this 2026-07-10 rewrite) if ever needed.

## 5. Retired / Not Doing

Standing rules, not one-shot tasks — durable until explicitly revisited with the user:

- No Simple3D/U3D/Nova3D work of any kind — no bug fixes, no build-environment troubleshooting.
  `GalaxyEggbertSimple3D` is historical/behavioral reference only (2026-07-08).
- No modifying `../mobile-eggbert`, ever, even temporarily/for analysis — copy a file into
  galaxy-eggbert first if a working copy is genuinely needed.
- No copying mobile-eggbert code or data (tables, enums, save-format byte layout, sprite/frame
  logic) into Galaxy Eggbert without explicit per-instance user approval, even if it looks like
  "just data."
- No `.txt`→`.vwr` auto-converter tool — real 3D worlds must be hand-authored; a flat-Y
  auto-conversion would produce an unplayable curiosity, not a real level.
- No MeshCraft, Mesh World, or further Simple3D features re-entering the active target path.
- No Lua unless explicitly requested by the user.
- No Easy3D scope creep — no ECS, scene graph, or engine; small generic 3D-batching helpers only.
- No new gameplay mechanics — every task in `## 1`/`## 2` traces to a documented mobile-eggbert
  behavior; if it doesn't, it doesn't belong in this plan.
- Don't silently narrow "all X" / "complete" scoping — ask first if a task's true scope is
  ambiguous.
- Commit after each discrete task; push only when explicitly requested per-instance.

---

## 6. Development Tooling — 3D World Editor

Not a mobile-eggbert feature, so the faithful-remake rule (`## 0`/`CLAUDE.md`) doesn't govern
this section — it's a content-creation tool for building `.vwr` worlds, the same category as the
already-existing `tools/GenerateSampleWorld3D.cpp` (a fixed, hand-coded C++ generator) and
`tools/VerifyBlupiMovement.cpp`-style scripted verification tools. It does not conflict with the
standing "no `.txt`→`.vwr` auto-converter" rule (`## 5`) — that rule rejects *automatic*
conversion from flat 2D data; an interactive editor is exactly the "hand-authored" tool that rule
already assumes exists. Today, hand-authoring a `.vwr` world means writing/editing C++ calls in
`GenerateSampleWorld3D.cpp` and rebuilding — real but slow and error-prone (see plan.md's own
`E3D-MIG-058` history: a real data-loss bug, `Chunk::isEmpty()` ignoring `extraMetadata_`, and a
real zero-patrol-range lift bug were both introduced this way and only caught by scripted
verification after the fact, not while authoring). An interactive editor is aimed at that gap.

Much of the needed infrastructure already exists and should be reused, not rebuilt:
`GalaxyEggbert::Worlds::World::loadFromFile()`/`saveToFile()` (engine-agnostic, already tested,
`.vwr` format), `GETerrainRenderer`/`GETileAtlas` (already renders any loaded `World`),
`Easy3D::Camera3D`, and `GalaxyEggbert::MoveObjectRecord`'s embed-in-`.vwr` mechanism (already
used by `GenerateSampleWorld3D.cpp`). The editor is new UI/interaction code on top of these, not a
new rendering or file-format stack.

- [ ] `EDITOR-000` `[?]` Decide target shape: a separate executable (`GalaxyEggbertEditor3D`,
      own `main.cpp`, own CMake target under `tools/` or a new `src/GalaxyEggbertEditor/` tree —
      matches the existing `VerifyXxx`/`GenerateSampleWorld3D` precedent of dedicated tool
      targets) vs. an in-game mode toggle inside `GalaxyEggbertCNA` itself (shares the binary,
      but mixes editor UI/controls into the shipping game's control scheme and input handling).
      Recommend the separate-executable shape for the same reason the verification tools are
      separate: keeps `GalaxyEggbertCnaGame` free of editor-only state and input branches.
- [ ] `EDITOR-001` Free-fly camera (WASD + mouse look, detached from any Blupi controller) —
      reuses `Easy3D::Camera3D` directly, no new camera math needed beyond input-driven
      position/yaw/pitch (`GEBlupiController`'s tank-control scheme is Blupi-specific, not
      reusable here).
- [ ] `EDITOR-002` Load/save `.vwr` via the existing engine-agnostic `World::loadFromFile()`/
      `saveToFile()` — no new format or parser work; this is already a tested, working API
      (`GalaxyEggbertWorldsTests`).
- [ ] `EDITOR-003` Render the loaded world via the existing `GETerrainRenderer`/`GETileAtlas` —
      reuse `GalaxyEggbertCnaGame::LoadContent()`'s terrain-setup code as a template, don't
      reimplement it.
- [ ] `EDITOR-004` Block picking: raycast from the camera through the mouse cursor into the voxel
      grid to find which cell (and which face) the cursor is over — genuinely new work, no
      existing raycast helper in Easy3D or CNA to reuse.
- [ ] `EDITOR-005` Place/remove a block at the picked cell using a `BlockType` chosen from a
      palette (see `EDITOR-008`).
- [ ] `EDITOR-006` `MoveObject` placement/editing: add/move/remove `ObjectType` instances,
      including setting `posStart`/`posEnd`/`speed` for platform lifts — via
      `GalaxyEggbert::MoveObjectRecord`'s existing embed mechanism, not a new data model.
- [ ] `EDITOR-007` Sky region selection (sets the `.vwr` v2 `skyRegion` header field used by
      `GalaxyEggbertCnaGame`'s real background-image rendering, `E3D-MIG-05x`).
- [ ] `EDITOR-008` Minimal on-screen tile/object-type palette UI — reuses the icon-drawing
      `SpriteBatch` approach already used for the game's own HUD (`## 2.3`, 2026-07-11), not a
      new UI framework; a scrollable/paged icon grid, not text-driven.
- [ ] `EDITOR-009` Undo/redo — optional, later; not required for a usable first version.
- [ ] `EDITOR-010` Multi-block brush/fill tool, mirroring `GenerateSampleWorld3D.cpp`'s own
      `fill()` helper interactively (drag a box, apply one `BlockType` to the whole region) —
      optional, later.
