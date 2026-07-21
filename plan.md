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
  **Reverted back to a pyramid the SAME day (3rd live feedback round, with an actual attached
  screenshot this time — `Screenshot From 2026-07-11 16-35-24.png`, found locally under
  `/home/robertvokac/Pictures/Screenshots/` and inspected directly)**: the box's 4 flat side faces
  (each showing a triangle via an alpha cutout) don't share a common vertex the way a real
  pyramid's 4 triangular faces do, so adjacent faces' triangle graphics visibly failed to connect
  at the block's 4 vertical edges ("ty hroty 4 textury trojúhelníku nejsou dole svázané k sobě",
  confirmed directly in the screenshot). `PyramidTipItem`/`AppendPyramidTipMesh()` re-added to
  `../easy-3d` (hpp/cpp + its own test, same code as the original round-1 version) — a genuine
  pyramid's 4 faces share one apex vertex by construction, so it's structurally seamless; combined
  with the already-fixed alpha blending (round 2, unrelated to the box-vs-pyramid choice), this
  keeps the earlier "black background" fix too. The user's screenshot also reported "zadní
  teleporter renderuje dopředu, je to rozbité" (the rear teleporter renders forward, broken) —
  not independently reproduced after the pyramid revert (both real teleporter rooms and the exact
  exhibition row shown in the screenshot rendered cleanly on a fresh screenshot at the same
  location), treated as a symptom of the box design rather than a confirmed separate bug. Full
  5-tool suite + both backends + easy-3d's own test suite (including the re-added pyramid test)
  all re-verified.
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

**How this section relates to `## 2` below (clarified 2026-07-20, after a session found the two
looked like duplicate tracking and asked about cleanup):** the Phases below (`E3D-MIG-0xx`-`1xx`)
are a **chronological development narrative** — one porting effort per phase, told mostly in the
order it happened, with the reasoning/dead-ends/live-playtest revisions kept in. `## 2` (Feature
Parity Checklist) is a **flat, granular, per-feature status list** (`BLUPI-xxx`/`PICKUP-xxx`/
`ENEMY-xxx`/`SOUND-xxx`/etc. — a completely separate ID space from `E3D-MIG-xxx`, not aliases of
each other) that keeps growing as work continues; `## 2` cross-references INTO specific `E3D-MIG`
phase entries for their full technical story (30+ such cross-references already exist). These are
genuinely complementary, not redundant — the phase narrative below is largely a frozen record of
2026-07-01 through 2026-07-12 and is **not** kept in sync with later work, while `## 2` is the
document's actively-maintained, current source of truth. **When they disagree, or when checking
whether something is currently done, trust `## 2` first** — a Phase item below marked incomplete
or "blocked" may well have since been finished under its own separate `## 2` ID (the granular
per-`BlupiAction`/enemy/pickup breakdown didn't fully exist yet when most phases below were
written). Read a Phase entry below for the *why* and the war story behind a decision, not for
today's status.

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
- [ ] `515` **Re-scoped 2026-07-12 (`E3D-MIG-170`'s research)**: icons 158-165 are hub-screen
      world-select markers (`Decor::IsWorld()`), NOT secret-power pickups — the real 4
      `SecretPower` buffs are granted by separate `MoveObject` pickups (25/26/30/31, already
      rendered via the existing generic billboard/object-icon system, no new render decision
      needed, and now gameplay-complete). This task should become "render hub-screen world-select
      icons 158-165/166-173 as gold-pedestal `Billboard`" instead, which only matters once hub/
      menu screens exist (`## 2 §2 MENU-*` territory) — not touched this session, low priority
      until then.
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
      **Real bug found and fixed 2026-07-19** (user request: "make sure the bottom-right animation
      icon actually corresponds to what's being animated"): `GEHud`'s indicator always sampled
      `blupi.png` for `GetAnimIcon()`'s value, but real `BlupiSearchIcon()`
      (`mobile-eggbert-reference/08-animations.md` §2's own "Channel selection" note, verified
      directly against `Decor.cpp`) selects `element.png` instead for `Clear1`/`Clear2`/`Clear3`/
      `Glu`/`Electro` specifically — of those, `Clear1`/`Clear2`/`Clear3`/`Glu` are all real,
      already-wired `DeathLocked` causes in this engine (`Electro` has no modeled mechanic yet), so
      every one of those 4 death animations was showing the wrong sheet's pixels at the same icon
      index. Fixed with a new `GEBlupiController::AnimIconUsesElementSheet()` predicate (true only
      for those 4 `DeathLocked` causes) threaded through `GEHud::Draw()`'s new
      `animIconUsesElementSheet` parameter, selecting `elementQuads`/`elementSheetW/H` instead of
      `blupiQuads`/`blupiSheetW/H` for exactly this one HUD element when it applies -- `GEHud`
      already drew other element.png icons elsewhere (keys, bullets, dynamite, Voyage), so no new
      texture load or render path was needed. 8 new `VerifyBlupiMovement` assertions (one baseline
      "Stop stays on blupi.png" + the sheet flag for all 6 `DeathCause` values, including 2
      previously test-uncovered ones, `Clear3`/`Glu`, added along the way). Live-verified under
      `xvfb-run` with temporary instrumentation (forced a `Clear1` death-lock, screenshot,
      reverted before commit): the icon now shows a coherent Blupi silhouette instead of whatever
      unrelated `blupi.png` region the same numeric index happened to land on.
- [~] `065` Real jump/gravity constants matching mobile-eggbert's tick-domain values (gravity
      +2.0/tick to terminal 20.0, displacement = 2×velocity; jump launch values by
      Jump-held×Power combo; ledge-walk-off has no boost) — rescale from 20Hz tick-domain to
      CNA's real framerate. Current `GEBlupiController` constants are an independent
      engine-appropriate approximation, not yet cross-checked against these real values.
      **Sub-finding fixed 2026-07-16**: while researching the real Jump-held×Power combo values
      (`Decor.cpp:2913-2947`), found the real ground-jump trigger is ALSO gated on vehicle mode —
      excludes Helico/Over/Balloon/Ecrase/Jeep/Tank entirely (Nage/Surf/Suspend don't exist in
      this engine), and gives Skateboard its OWN distinct velocity (-17 Power/-13 noPower), not
      the headroom-modulated ordinary-Blupi values. This engine's jump gate had no vehicle-mode
      check at all (same "predates vehicle modeling" gap already found/fixed this session for
      Sucette/Drink/Charge pickups and `TriggerTeleport()`) — Jeep/Tank could incorrectly launch a
      normal jump; Skateboard used the wrong magnitude. Fixed via a `VehicleMode` check in the
      jump gate + new `kSkateboardJumpSpeed`/`kSkateboardJumpSpeedPowered` constants (same
      proportional-anchoring technique as `kJumpSpeedPowered`/`Reduced`). New
      `VerifyBlupiMovement` assertions (Jeep/Tank jump-press no-ops, Skateboard's distinct
      velocity). **The larger task (rescaling `kGravity`/`kJumpSpeed`'s own absolute magnitude to
      real tick-domain values) is deliberately NOT done** — those constants have been repeatedly,
      deliberately game-feel-tuned against live user playtesting (e.g. `kFallDeathY`'s two
      documented feel-driven revisions above) rather than literal real-value transcriptions;
      changing their absolute magnitude needs the user's own live-feel judgment, not a solo
      numeric rescale — same category of "needs the user physically present" as the render-
      geometry decisions, just for game feel instead of visuals. Left open, not attempted this
      session.
- [ ] `066` Turning-duration-per-mode table (Normal/Air 6 ticks, Overcraft/Jeep 7, Helicopter/
      Swim/Surf/Suspended 10, Tank 12, Skateboard 14). **Investigated 2026-07-16, NOT a simple
      constant port**: directly checked `Decor.cpp` — real `BlupiAction::Turn`/`TurnHelico`/
      `TurnJeep`/`TurnTank`/`TurnSkate`/`TurnNage`/`TurnSurf`/`TurnSuspend` is a DISCRETE 2D
      facing-flip mechanic (Blupi only ever faces `Direction::Left`/`Right`; reversing plays a
      fixed-duration "Turn" animation state, mode-dependent tick count, before the flip completes
      and movement resumes) — NOT a continuous angular turn-RATE modifier. This engine's
      `kTurnSpeed`/tank-control continuous free-yaw (`m_yaw += turnInput*kTurnSpeed*dt`) is
      already a natural 3D adaptation of that (per `CLAUDE.md`'s allowed adaptations), fully
      unlike the real discrete 2-state mechanic — there's no obvious 1:1 mapping from "N ticks to
      flip facing" onto "continuous yaw degrees/sec", so this isn't a safe solo numeric rescale
      the way `kSkateboardJumpSpeed` was (`065`'s fix above): naively multiplying `kTurnSpeed` by
      `6/N` per vehicle mode would be inventing a mechanic-mapping, not porting real data. Left
      open; would need either accepting an invented mapping (against the faithful-remake rule) or
      a design discussion on whether/how to model per-mode turn delay at all in a continuous-yaw
      engine.
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
      13 distinct positions exceeding the 10-slot capacity). **Corrected 2026-07-16: the "still
      missing" list below is stale** — the full hazard→action table (Lava→Clear3, Saw→Clear4,
      Blitz→Clear1, Fan→Clear1/Clear2 coinflip, Spike/Drip→Glu) and fixed per-action animation
      durations were both completed by Phase 14/the death-lock system (`GalaxyEggbertCnaGame.cpp`'s
      per-hazard `triggerDeath()` calls + `GEBlupiController::TriggerDeathLock()`'s
      `kDeathLockTicks[]`, confirmed directly in source) — this note just predates that work and
      was never updated after. **Still genuinely open**: fall→1000px-past-the-death-boundary =
      instant game over bypassing remaining lives (`Decor.cpp:6406-6410`, unconditional
      `m_term=-1; DoorsLost()`, distinct from the normal per-life Clear2 fall-death at the row-99
      boundary a bit above it) — investigated 2026-07-16, NOT implemented: this engine's fall
      death (`kFallDeathY=-27`) always goes through the normal `LoseLife()`/respawn path with no
      second, deeper "instant total game over" threshold at all. Left unimplemented rather than
      guessed at: real `TriggerDeathLock()`-equivalent state fully freezes Blupi (no further
      Y movement) once a fall-death fires, same as this engine already does — meaning in the REAL
      game this deeper threshold is very likely unreachable through ordinary continued falling
      too (a defensive catch-all, not a normally-triggerable mechanic), so confidence in exactly
      how/whether to port it is low; needs further source research (specifically: what could
      leave `m_blupiRestart` false or `m_blupiValidPos` itself already past this threshold) before
      attempting, not a straightforward table-fill. **Fall-death TIMING is also now
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
      **Follow-up (2026-07-13, user feedback): even this "feel"-matched ~6.2-6.7s read as too long
      in practice.** User asked for roughly half. `kFallDeathY` moved again, `-60.0f` → `-27.0f` —
      a pure game-feel tweak this time, not a further faithfulness correction (the real absolute
      margin was never being matched here anyway). Verified live: death now fires at 3.417s, almost
      exactly half of the previous 6.72s.
- [x] `068` Electric aura (`BlupiElectro`) — **done 2026-07-13**, per explicit user request. The
      stale `E3D-MIG-190` dependency this entry pointed at was already superseded by `170`/`172`/
      `174` (the actual secret-power buffs, confirmed already implemented — see `086`'s own note).
      Confirmed via `mobile-eggbert-reference/10-blupi-mechanics.md` §9 (`Decor::BlupiElectro`,
      `Decor.cpp:9610-9638`/`7976-7987`): while `m_blupiCloud` (this engine's `SecretPower::Cloud`,
      the real "Power-Charge" pickup) is active, instantly destroys small enemies
      (`ObjectType4`/`32`/`33`) within 40px of Blupi's own box — an offensive aura Blupi carries,
      unrelated to the `Blitz` lightning HAZARD despite the similarly-named real function. Ported
      as a new `GEInteractionSystem::Update()` parameter (`blupiCloudActive`, wired from the
      caller's existing `GetSecretPower()==Cloud` check) checked first in the per-object loop, so a
      `Type4` enemy (also in `IsGenericHazard()`'s own list) is destroyed by the aura rather than
      also killing Blupi via hazard contact the same frame. Real sound channel 59 on each kill.
      Real 40px expansion modeled as a circular radius combining that with a 32px half-tile
      (`(40+32)/64` grid units — same "half-tile + real px offset, /64" pattern already used for
      `kFollowerWakeRadius`), consistent with every other proximity test in this file being
      circular rather than the real AABB rect test. Verified via 4 new `VerifyInteractionSystem`
      checks (aura off leaves a blupih untouched, aura on destroys it, a distant blupit 50 units
      away is untouched even with the aura on) plus a live headless run confirming a real blupih
      is destroyed within one frame of standing on it with Cloud active. Full regression suite
      green on both backends.
- [ ] `069` Real 3D Blupi model (third-person only) — now the sole path to a visible, faithful
      Blupi per the `063` decision, not merely optional polish. First-person intentionally stays
      without a visible model (matches the existing default camera; the player never sees it).
      No timeline set; the current placeholder Fox model remains the third-person stand-in until
      this is scoped.
      **Placeholder scale/grounding fixed 2026-07-18** (user-reported: model floats in the air,
      and is ~160% of block height instead of the real 71.875%): two independent bugs in the
      placeholder's own draw block (`GalaxyEggbertCnaGame.cpp`, third-person model transform), not
      in the real Blupi model target itself.
      1. **Scale**: `kPlaceholderModelScale` was an unvalidated guess (`0.02`) admitted as such in
         its own comment. The Fox mesh's real local-space vertex Y span was measured directly by
         parsing `avatars3d/blupi_placeholder/fox1.verts.bin` (stride 52 bytes, position = first 3
         floats per `convert_avatar.py`'s own `struct.pack("<3f3f2f4f4B", ...)` layout): -0.122 to
         78.907, ~79.03 units tall, feet already at local Y~=0. Real mobile-eggbert's own standing
         collision box (`Decor::BlupiRect()`'s default case, `Decor.cpp:2519-2520`: `Top=pos.Y+11,
         Bottom=pos.Y+60-2`, 47px inside the 60px `DIMBLUPIY` cell, `Def.hpp:154`) brackets the
         user's own reported 71.875% figure (exact fraction not located as a literal source
         constant, likely the user's own sprite measurement). Since 1 block = 1.0 world unit here
         (`GEBlupiController::GroundHeightAt()` returns topmost-solid-block-Y + 1), target height =
         0.71875 world units → `kPlaceholderModelScale = 0.71875 / 79.029 ≈ 0.009095`.
      2. **Grounding**: `GEBlupiController::GetY()` is NOT flush with the terrain's own rendered
         surface. `GETerrainRenderer` places a solid block's cube `Center.Y` at the block's raw
         grid index directly (no `+0.5`), so a column's topmost solid block at grid `Y=g` has its
         visual top surface at world `Y=g+0.5` — but `GroundHeightAt()` returns `g+1` (matching the
         separate `MoveObject`/`BigDecor` cube convention of sitting a full unit above the floor
         block's own grid index instead). Net effect: `GetY()` sits a constant 0.5 world units
         above the true visual ground surface — invisible before now (no rendered body to compare
         against; the first-person `kEyeHeight`/chase-camera constants were already feel-tuned
         against this same convention), but glaringly visible once an actual body silhouette
         renders against the terrain mesh. Confirmed live via a forced third-person screenshot
         (`GE_DEBUG_FORCE_THIRDPERSON`/`GE_DEBUG_SKIP_TO_PLAY`/`GE_DEBUG_LATE_SCREENSHOT_FRAME`
         temporary env-var instrumentation, reverted before commit) showing the Fox's feet
         hovering above the floor tiles before the fix, flush after. Deliberately fixed as a
         render-only `kPlaceholderModelYOffset = -0.5f` local to this specific placeholder mesh's
         own translation — NOT by changing `GetY()`/`GroundHeightAt()` itself, which would ripple
         into jump/fall physics, `kFallDeathY`, teleporter/water Y thresholds, and every already
         screenshot-tuned camera constant that implicitly assumes the existing convention.
      **Second, deeper root cause found and fixed 2026-07-18** (user retested with a screenshot
      still showing the fox floating, well beyond the small 0.5-unit gap the fix above accounts
      for): the user's actual desktop builds (`cmake-build-debug`, `build-cna-vulkan`) use the
      **Vulkan** graphics backend, not EasyGL — the only backend this task's fix had been verified
      against. Side-by-side screenshots of the exact same committed code, differing only in
      backend, confirmed a genuine, separate Vulkan-only bug: EasyGL showed the Fox correctly
      grounded, Vulkan showed it hovering high in the sky. Root cause, found in the sibling `cna`
      engine repo (`../cna`, not galaxy-eggbert): every other Vulkan 3D vertex shader
      (`textured3d.vert.glsl`, `colored3d.vert.glsl`, `lit_textured3d.vert.glsl`, ...) applies a
      manual `pos.y = -pos.y` after the MVP transform to compensate for Vulkan's inverted NDC Y
      axis vs. OpenGL — but all 4 `SkinnedEffect` Vulkan vertex shaders (`skinned3d.vert.glsl`,
      `skinned3d_color.vert.glsl`, `skinned3d_vertexlit.vert.glsl`,
      `skinned3d_vertexlit_color.vert.glsl`, used by `AvatarRenderer`/the third-person placeholder
      model) were missing this exact line — a genuine oversight, not a deliberate difference.
      Confirmed via `../cna`'s own repo layout that this is a real, previously-undetected gap: no
      Vulkan golden-image regression test exists for `SkinnedEffect` at all (only EasyGL has golden
      PNGs under `cna/examples/golden/`), so nothing in `cna`'s own test suite could have caught it.
      Fixed by adding the identical `gl_Position.y = -gl_Position.y;` line to all 4 shaders
      (matching `textured3d.vert.glsl`'s exact convention) and recompiling via `cna`'s own
      `shaders/compile_shaders.py` (regenerates the checked-in `spirv_shaders.hpp`). Re-verified via
      the same screenshot method on `build-cna-vulkan`: Vulkan now renders the Fox identically to
      EasyGL, correctly grounded. **This fix lives in `../cna`, currently uncommitted there** — it
      is a genuine upstream engine bug with no possible workaround from galaxy-eggbert's side (no
      shim is possible for a missing shader instruction), unlike the earlier sibling-repo CMake gap
      this session worked around locally; needs the user's own decision on committing it in `cna`.
      All 3 native CNA-configured build directories (`build-cna` EasyGL, `cmake-build-debug` and
      `build-cna-vulkan` both Vulkan) rebuilt and reconfirmed working after this fix.
      **Default graphics backend switched to EasyGL 2026-07-18** (user request, given the Vulkan
      bug above): `CMakeLists.txt`'s native default changed from `VULKAN` to `EASYGL`
      unconditionally (Emscripten already defaulted to EasyGL). Still fully overridable via
      `-DCNA_GRAPHICS_BACKEND=VULKAN` etc. for the future — a default change, not a removal of
      other backends. `cmake-build-debug` (the directory with a pinned `VULKAN` cache from an
      earlier 2026-07-09 request) explicitly reconfigured with `-DCNA_GRAPHICS_BACKEND=EASYGL` to
      actually pick up the new default, since an existing cache entry doesn't change on its own.
      **Balloon visual legibility gap found and fixed, same day**: the user re-reported "Blupi
      still doesn't float when the wasp stings him" after the Balloon physics fix above. Rigorous
      live investigation (temporary debug instrumentation: teleport Blupi onto the real wasp in
      `worlds3d/world999.vwr`, log `IsBallooned()`/`GetY()` every frame, then forcibly move him
      over a genuinely floorless column while still ballooned) proved the underlying physics are
      100% correct — contact detection fires immediately, and `GetY()` holds EXACTLY constant for
      500+ frames over open air, matching real source's true zero-gravity freeze exactly. The real
      gap: `BlupiAnimStateToPlaceholderClipName()` maps `AnimState::Balloon` to the same idle
      "Survey" clip as normal standing — so when the wasp (placed on flat ground in
      `worlds3d/world999.vwr`) stings Blupi, "frozen height" looks IDENTICAL to normal standing,
      since there's nothing to fall out of. Real mobile-eggbert's own Balloon sprite is a visibly
      distinct round/ball shape (`kBalloonFrames`, icon 291+) that reads as "floating" regardless of
      ground contact; the placeholder Fox mesh's own 3-clip Survey/Walk/Run set has nothing
      resembling it. Two successive cosmetic render-only "bob" attempts were made here (a
      +/-0.08 symmetric sine, then a sustained ~0.22-unit lift) — **both have since been removed**,
      because the whole premise was wrong: see the entry immediately below.
      **THE ACTUAL BUG, found 2026-07-18 on the third investigation (user reported it ~15 times).**
      Every earlier pass answered the question "does gravity still pull Blupi down while ballooned?"
      and stopped there. The real answer is that gravity is not merely switched off — **Blupi
      actively RISES**, drifting upward like a balloon for the whole 10s. The real code for this
      lives in its own dedicated block, `Decor.cpp:4039-4058` (`if (m_blupiBalloon &&
      m_blupiFocus)`), which none of the earlier passes ever opened: they only ever found the
      `!m_blupiBalloon`-gated fall trigger at `Decor.cpp:2823` and reasoned (wrongly) from its
      absence. That block accelerates `m_blupiVitesseY` upward every `ScaleTime(6)` ticks:
      no input → terminal `-3.0` px/tick; `Jump`/Up held → faster `-5.0` px/tick; Down held →
      decelerate back toward `0` (hover), never into a descent. (Negative is up in the real
      2D screen-down Y space.) Ported into `GEBlupiController::Step()`'s own balloon branch with
      `kBalloonRiseSpeed`/`kBalloonRiseSpeedFast`/`kBalloonRiseAccel`, converting real px/tick at
      the pinned 20fps with 64px per block into this engine's 1-block-per-unit, +Y-up space
      (`px/tick * 20 / 64`): 0.9375 / 1.5625 units/s terminal, 1.0417 units/s^2 acceleration.
      Up/Down map to `lookUpHeld`/`crouchHeld`, reusing the pair this class's own
      Helicopter/Overcraft branch had already established rather than inventing a second mapping.
      The invented cosmetic bob was deleted at the same time — with real rising in place it is both
      unnecessary and a non-faithful addition (CLAUDE.md's faithful-remake rule). Verified three
      ways: 6 new `VerifyBlupiMovement` assertions (rises above the sting height, outruns a falling
      Blupi, is airborne, and settles at each of the three real terminal speeds within 0.02 units/s);
      a live realistic-input run showing Y climb 1.0 → 4.4+ at a measured 0.9375 units/s, exactly
      the real terminal rate; and third-person screenshots showing the entire level drop away
      beneath him.
      **Both remaining limitations closed same day, per explicit user request** (`kBalloonDuration`'s
      own comment has the full citations):
      1. **Horizontal drift** — `Decor.cpp:4059-4106`, the SAME real block, right after the Y-rise
         code: `m_blupiVitesseX` independently ramps toward a signed `speedX*10` px/tick target (1.0
         px/tick accel while held, 2.0 px/tick/tick decel back to exactly 0 when released — never
         overshooting past 0). Ported as `m_balloonHorizontalSpeed`
         (`kBalloonHorizontalSpeed`=3.125, `kBalloonHorizontalAccel`=6.25, `kBalloonHorizontalDecel`=12.5
         units/s, same px/tick*20/64 conversion as the rise constants), mirroring `m_vehicleSpeed`'s
         existing ramp shape but with the real distinct rates, written as the real 3-way branch on
         `moveInput`'s sign rather than force-fit into that single-rate shape. `TriggerBalloon()` now
         also force-exits any vehicle mode (real `Decor.cpp:5826-5849`'s `ByeByeHelico()` + every
         vehicle flag cleared) — required so this new branch takes priority over `IsInVehicle()`
         immediately on being stung, not merely a nice-to-have. **Real `BlupiBloque` wall-stop NOT
         ported**: `TryMoveAxis()`'s step-up gate only applies while grounded, so this engine has no
         horizontal-wall-collision at all for ANY airborne movement today (plain jumping/vehicles
         included) — adding one is a shared-collision change well beyond "the balloon gaps" and risks
         regressing already-verified airborne behavior; left as a known, narrower difference (a
         floating Blupi drifts through a wall he'd normally stop against).
      2. **Ceiling stop** — corrected from an earlier WRONG claim that the rise matched Helicopter/
         Overcraft's own free-clip gap: real source's general `Decor::TestPath()` swept collision
         (`Decor.cpp:6782`) is applied to the FINAL merged position every frame regardless of status,
         confirmed directly to stop Blupi against solid decor in ANY direction including straight up.
         New `CeilingHeightAt()` (mirrors `GroundHeightAt()`, scans upward instead of down) finds the
         first solid block above; the rise clamps to just beneath it and zeroes `m_velocityY`.
         **A real integration bug was found and fixed while verifying this**: `GroundHeightAt()`'s own
         scan window is keyed off the same pre-frame `m_y`, so once a rising Blupi gets within ~1 unit
         of that exact ceiling block, `GroundHeightAt()` finds it FIRST too and misreports it as
         ground to land ON TOP of — confirmed live via a standalone debug harness, Blupi teleported
         from y≈2.0 straight to y≈4.0 (the ceiling block's own "land on top" height) well before ever
         reaching the real 2.5 contact height, silently pre-empting the new clamp. Fixed by having the
         ceiling scan ALSO suppress the ground-check for every frame a ceiling is within its own reach
         (not merely the frame the clamp itself fires) — both scans share the identical trigger
         distance, so this reliably covers every contaminated frame. Re-verified via the same
         standalone harness: Blupi now rises smoothly and holds stable at exactly the real ceiling
         contact height indefinitely, no teleport. Deliberately NOT extended to the general jump apex
         or to Helicopter/Overcraft (no real per-status precedent read for those, and jump-ceiling
         collision isn't modelled anywhere in this class today) — scoped to exactly what was asked.
      Verified via 4 new `VerifyBlupiMovement` assertions (drift settles at the real terminal speed,
      decelerates to exactly 0 on release, a mounted vehicle is force-exited on sting, and the rise
      stops flush at a real solid ceiling instead of clipping through it) plus the standalone debug
      harnesses above; full regression clean (only the 1 known pre-existing unrelated failure).
      **A second wasp was also added directly on the flat spawn corridor** of `worlds3d/world999.vwr`
      (see `135`'s own entry below) specifically to make manual re-testing of this exact mechanic
      trivial going forward — the original wasp requires navigating a staircase + terraced ascent.

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
- [~] `084` `SoundEnviron()` terrain-specific footstep remapping — **footstep half done
      2026-07-12**, head-bump half not applicable yet. New
      `GESound::FootstepChannelFor(icon)`, a pure icon->channel lookup covering all 7 real ranges
      from `mobile-eggbert-reference/07-sounds.md` (78: 32-34/41-47/139-143, 80: 1-28/78-90/
      250-260/311-316/324-329, 82: 284-303/338, 84: 341-363, 86: 215-234, 88: 246-249, 90:
      107-109), falling back to the generic channel 3 outside all of them. Wired into both
      `PlayStep()`/`PlayLand()`, keyed off `GEBlupiController::GetGroundBlockType()`. **Found and
      fixed a real, pre-existing bug while researching this**: `PlayLand()` played channel 4, but
      the real source's channel 4 is head-bump/ceiling-hit — an entirely different event (hitting
      an obstacle above during a jump) — not landing at all; channel 3 covers BOTH footstep and
      landing in the real game. This wrong mapping was ported verbatim from
      `GalaxyEggbertSimple3D`'s own `GESound` (predating the later, independently-verified
      channel research in `07-sounds.md`) and silently carried into `GalaxyEggbertCNA`. Fixed
      `PlayLand()` to channel 3, matching `PlayStep()`. Head-bump itself (channel 4 and its own
      79/81/83/85/87/89/91 terrain remaps) is NOT wired — there is no ceiling-hit detection in
      `GEBlupiController` at all (Blupi's upward jump arc has no "hit an obstacle above" event to
      remap), a separate, not-yet-implemented mechanic; out of this task's scope. Verified: 10 new
      `VerifyInteractionSystem` assertions (one representative icon per range + the generic
      fallback) + full suite (63/63 unit tests, all verify tools) + both backends (live headless
      run confirms no crash/regression).
- [ ] `085` Idle "fidget" periodic sounds — channels 36/37/46-49/65 trigger on
      `m_blupiPhase % N`, not simple one-shot events; easy to miss in a naive port.
      **Checked 2026-07-12, genuinely blocked, not attempted**: per
      `mobile-eggbert-reference/07-sounds.md`, channels 46-49/65 each pair to a distinct idle-
      boredom animation VARIANT (`Ouf1a`/`Ouf1b`/`Ouf5`/`Mockeryp`/`Ouf3`/`Ouf4`/`Mockery`/
      `Mockeryi`) that doesn't exist as an `AnimState` in `GEBlupiController` at all yet — these
      are new animation states, not sound-only additions, and implementing them meaningfully
      needs the same real `table_blupi` timing/state transcription `E3D-MIG-064` already flags as
      blocked on `069` (the real 3D Blupi model) and requiring explicit user approval. Channel 37
      (idle tick while `Stop`) could theoretically be cherry-picked since `AnimState::Stop`
      already exists, but channel 36 (its `Suspend`-mode counterpart) is blocked the same way
      `E3D-MIG-177`'s Suspended mode is (needs icon 202's pending render-geometry call) — picking
      off only channel 37 alone would be an oddly partial slice of one task, not implemented here.
      Revisit this whole task together once `064`/`069`/`177` unblock.
- [~] `086` Buff activate/expire-warning channel pairs — **done 2026-07-12** as part of
      `E3D-MIG-170`/`172`/`174` (the actual secret-power buffs, implemented under those task IDs,
      not the stale `E3D-MIG-190` this entry used to point at): Shield 42/43(warn@10), Power
      44/45(w@20), Cloud 55/56(w@25), Hide 62/63(w@20) all confirmed wired in
      `GalaxyEggbertCnaGame.cpp` with the exact real channel numbers. Channel 58 (Cloud
      pickup-start) is part of the real 2-stage pickup delay/animation-lock, which `E3D-MIG-173`
      already documents as NOT modeled (buffs grant instantly instead) — not a gap in this task.
      Mirror/Invert(40)'s 66/67 pair remains unimplemented since Mirror/Invert itself isn't a
      modeled `SecretPower` value (see `174`'s own note) — the only real remaining piece of this
      task, blocked on that separate mechanic ever being scoped.
- [ ] `087` Corrected pickup sound channels are already applied in `GEInteractionSystem`
      (treasure/key 11 or 19, egg 3) — extend the same rigor to every future pickup/enemy/hazard
      sound trigger rather than reusing `GESound`'s convenience shortcuts (`PlayCollect`/
      `PlayLife`) which were found to be imprecise.

### Phase 9 — HUD (`E3D-MIG-090`-`093`) — `Decor::DrawInfo`'s real scope complete (2026-07-13)

- [x] `090`-`093` Minimal lives/world/treasure HUD — **the real `Decor::DrawInfo` scope is now
      complete** (2026-07-13), drawn via real 3D
      quads (`GEHud`, NOT `SpriteBatch` — see that class's own comment for why: CNA's Vulkan
      backend records every `SpriteBatch` batch before every 3D draw each frame, so a sprite HUD
      is always painted over by the 3D scene). Done: life icons (`blupi.png`), key icons
      (`element.png`), treasure counter text + `pad.png` panel (`text.png`, glyph-index-is-ASCII,
      fixed advance — see `## 2.3 HUD-001/003/004/005/006/007`), (2026-07-13) bullet/dynamite
      counters (`HUD-015`/`016`), (2026-07-13) both real `jauge.png` gauges — water/Nage
      breath (`HUD-012`/`018`) and the shared Shield/Power/Cloud/Hide countdown (`HUD-008`/`019`)
      — and (2026-07-13) the perso decoy counter + its underlying place/retrieve mechanic
      (`HUD-017`), (2026-07-13) the training-hint overlay + its real mission-number
      infrastructure (`HUD-024`), and (2026-07-13) the real `Def::Phase` state machine gating
      when the HUD is even shown at all (`HUD-023`). Avoid building a UI framework — a HUD is a handful of sprite
      draws keyed to game state, not a system. **The real `Decor::DrawInfo` function
      (`Decor.cpp:1185-1311` — the actual in-game HUD draw call, the authoritative source for what
      this phase should port) has now been read in full, end to end (2026-07-13), and every single
      element it draws is now implemented — `090`-`093`'s real scope is complete.** Every OTHER
      `HUD-0NN` item (`002`/`009`-`011`/`013`/
      `020`-`023`/`025`-`026`) is NOT present in the real `DrawInfo` at all — several turned out to
      be based on wrong premises entirely (`HUD-002`'s claimed life-icon cap doesn't exist;
      `HUD-009`/`025`'s "score" has no real backing anywhere found) and are now flagged `[?]` in
      `## 2.3` for independent re-verification against whichever other real function (if any)
      actually implements them, rather than assumed real. `HUD-014` (camera shake) IS confirmed
      real (`m_decorAction`/`DecorAction::SmallShake`/`BigShake`) but belongs with camera work, not
      `GEHud`.

### Phase 10 — Gameplay parity (`E3D-MIG-100`-`107`)

- [~] `100` Pickups: treasure/egg/exit/keys done (`GEInteractionSystem`, 2026-07-10); dynamite,
      doors/keys, secret powers, bullet pack also done since (`E3D-MIG-155`/`160`-`162`/`170`-
      `172`/`174`/`175`, all 2026-07-12) — remaining pickup types are the deliberately-deferred
      ones documented under those same task IDs (2-stage delay, sparkle-fx), not unstarted work.
- [x] `101` Hazard/kill detection — **done**, see `E3D-MIG-140`s (Phase 14, 10/10 complete).
- [x] `102` Enemy contact-kill — **done**, see `E3D-MIG-130`s (Phase 13, fully complete
      2026-07-11/12). **Title corrected 2026-07-16**: this entry's own title said "stomp +
      contact-kill", implying stomp is a separate real mechanic alongside contact-kill — it isn't
      (confirmed repeatedly this session, `BLUPI-126`/`131`, `ENEMY-004`/`036`: no velocity-gated
      "stomp" concept exists anywhere in real source, contact-kill is a plain unconditional touch
      check). No separate "score/counter" exists in real mobile-eggbert to port (see
      `107` below) — lives/egg-gauge is the only real counter, already covered by `130`.
- ~~`103` Respawn invincibility window after death~~ **CONFIRMED NON-FEATURE, 2026-07-14** — direct
      source research (grepped all of `Decor.cpp`/`Decor.hpp`, re-verified the exact life-loss
      Voyage completion site, `Decor.cpp:10256-10260`: `m_blupiAction=Stop; m_blupiPhase=0;
      m_blupiFocus=true;`, nothing else) found NO invincibility flag/timer/blink state tied to
      respawn anywhere in real source. `mobile-eggbert-reference/10-blupi-mechanics.md:260-268`
      independently confirms this — the real "safety" after respawn is purely SPATIAL (the FIFO
      safe-position system, `E3D-MIG-067`, already implemented, never respawns onto a hazard
      tile), not temporal immunity. Per CLAUDE.md's faithful-remake rule, not implemented — this
      item is closed as a non-feature, not a remaining gap.
- [x] `104` Exit-gate logic: gated on `treasuresCollected_ >= totalTreasures_`, win/reject sound
      channels, debounced to fire once per contact.
- [x] `105` Crate-push + platform-patrol — **done**: linked-crate stacks (`E3D-MIG-150`), lift
      riding/boarding (`152`/`154`), dynamite (`155`) all complete 2026-07-12 (Phase 15
      substantially done; see that phase's own entries for exactly what's deferred and why —
      `151`/`153`/`156`-`158`).
- [ ] `106` `[?]` Save/load — scope still TBD (`easy3d.md` §12 Q7: byte-level format
      compatibility with mobile-eggbert saves is an open question, not a default). See
      `## 2 §11 SAVE-*` for the full conceptual checklist.
- [ ] `107` Standing rule: no new mechanics — every gameplay task in this plan must trace to a
      documented mobile-eggbert behavior.

### Phase 13 — Enemy AI & combat (`E3D-MIG-130`-`137`) — fully complete (2026-07-11/12)

mobile-eggbert-reference/04-enemy-behavior.md and /12-hazards-and-interactables.md are the source
of truth; do not invent stomp/hit feel not documented there.

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
      shield/hide immunity gates **fixed 2026-07-12 as part of `170`** — this note was written
      before that landed and was never updated; superBlupi immunity remains NOT modeled, no such
      concept exists). New `GEWorldRuntime::GetWorldMutable()`
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
      decrement-every-`ScaleTime(2)`-ticks pattern as Crusher, NOT literally "100 ticks").
      **Vertical physics corrected 2026-07-18** (user-reported: Blupi should float/hover, not just
      cosmetically balloon): the original implementation applied 20%-reduced gravity (a slow
      fall), an approximation explicitly flagged in its own comment as unverified against a
      located constant. Direct re-verification against `Decor.cpp:2823` (the real "start falling"
      trigger, gated on `!m_blupiBalloon`) and the trigger itself (`Decor.cpp:5834-5849`, zeroes
      `m_blupiVitesseY`/`m_blupiAir`) confirms real Blupi genuinely FREEZES at a fixed height —
      true zero-gravity suspension, not a slow sink — since the only code path that would ever set
      `m_blupiAir=true`/resume gravity is itself gated off for the whole balloon duration. Fixed by
      skipping gravity integration entirely while `m_balloon` is active (`GEBlupiController::
      Step()`) instead of applying a reduced multiplier; `kBalloonGravityMultiplier` removed as
      dead code. `VerifyBlupiMovement.cpp`'s existing "falls slower" assertion was itself
      re-verified and tightened to assert the height is EXACTLY unchanged over a multi-step span.
      Contact does NOT kill Blupi or destroy the wasp (`GEInteractionSystem::BalloonTouchedThisFrame()`).
      **Real hazard-pop interaction implemented**: while ballooned, touching exactly 4 of the 8
      shared-kill-list types (`3`/`16`/`96`/`97` — confirmed via the real source's if/else-if
      chain, NOT all 8) pops the balloon instead of killing, and does NOT destroy the popping
      hazard either (the real pop branch has no `ObjectDelete` call) — `2`/`4`/`17`/`20` still
      kill through the balloon. Real entry/recovery sound channels 40/41 (channel 41 shared with
      Crusher's own recovery, confirming it's a generic "status expired" cue, not
      hazard-specific). Already playable via the wasp already placed on the north-hill plateau
      in `worlds3d/world999.vwr` (this engine's own quarantined mechanics-showcase world, reached
      from the real global hub `world001.vwr` via its `DemoPortal` marker at grid (50,1,54), a
      few tiles from spawn). Verified via 9 new `VerifyBlupiMovement` state-machine
      assertions and 9 new `VerifyInteractionSystem` assertions (wasp/follower/bulldozer
      interaction, injected synthetically since none but the wasp itself is placed in the real
      world).
      **Second wasp added right on the spawn corridor, 2026-07-18** (user reported "still doesn't
      work" ~7 times despite the physics/visual fixes above both being independently re-verified
      live and correct): the ONLY existing wasp sits on the north-hill plateau, which requires
      navigating a 10-step staircase + terraced ascent to reach — real friction for anyone manually
      testing this specific mechanic. Added `place(ObjectType::ObjectType44, 55.0f, 1.0f, 50.0f)`
      in `tools/GenerateSampleWorld3D.cpp`'s `GenerateDemoWorld()`, 5 tiles east of spawn on the
      same flat corridor floor, zero platforming required. **Conclusively re-verified end-to-end
      with fully realistic simulated player input** (not a position teleport like every earlier
      verification pass this session): temporary debug instrumentation forced `moveInput=1.0` every
      frame (the same "walk forward" signal a real held arrow key produces) through the actual
      `GalaxyEggbertCnaGame::Update()` pipeline from a fresh mission-999 load, logging position/
      `IsBallooned()` every frame. Contact with the new wasp triggers `IsBallooned()==true` at frame
      46 (~0.77s of walking), and Blupi's height stays frozen exactly while horizontal movement
      continues normally — proving the full chain (contact detection → `TriggerBalloon()` →
      physics freeze → visual bob) genuinely works via ordinary input, not just synthetic
      `SetPosition()` calls. All debug instrumentation reverted before commit, `worlds3d/world999.vwr`
      regenerated via the tool's own "manually run to regenerate" convention (only that one file
      changed — confirmed via `git status`, every other of the 78 world files stayed byte-identical).
- [x] `136` Type 54 (large creature) — lethal only while paused mid-turn, destroys current
      vehicle or fatally grabs Blupi, never destroyed itself, unconditional taunt icon. Done
      2026-07-11: verified directly against `Decor.cpp:5867-5913`. Contact is lethal ONLY while
      `patrolStep` is 1 or 3 (the real `step != 2 && step != 4` gate, now implementable since `131`
      landed) — safe to touch while it's actually mid-walk. The creature is never destroyed by the
      contact (no real `ObjectDelete` in that branch). Real balloon immunity IS modeled
      (`blupiBallooned` blocks the whole branch, matching the real `!m_blupiBalloon` gate, no
      separate pop path for this type unlike the 4 balloon-poppable hazards). Real shield/hide
      immunity (`blupiInvincible`) IS modeled too, superBlupi/focus are not (no such concepts
      exist). **Corrected 2026-07-16**: this entry's own title/body previously mis-stated the
      "destroys Blupi's vehicle instead of killing him" premise — verified directly against
      `Decor.cpp:5867-5913`: contact sets `BlupiAction::Glu` UNCONDITIONALLY either way (a real
      death via the shared per-action life-loss dispatch), it just ALSO plays a different sound/
      shake and clears vehicle/Balloon/Ecrase state when contact happens while riding/ballooned/
      squashed — mirroring what `BlupiDead()` already does unconditionally for every real death
      cause. That state-clearing is now faithful (`GEBlupiController::TriggerDeathLock()` fixed to
      match `BlupiDead()`, see `067`'s own entry) — the one remaining gap is purely cosmetic:
      this engine always plays the plain channel-51 sound regardless of vehicle/Balloon/Ecrase
      state at contact (`GEInteractionSystem` has no `GEBlupiController` access to know which
      applies), not worth new plumbing for an audio-only distinction. The real unconditional taunt
      icon is also NOT modeled — no idle-taunt animation system exists at all. The sample world's placement
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
- [x] `141` **Spikes (373) done 2026-07-11**, same shape as `140` (`GetGroundBlockType() ==
      Spike` → `triggerDeath()`, real channel 51 not channel 8 — the Glu-death sound). Real narrow
      central x-band restriction within the tile is NOT modeled — no sub-tile position exists in
      the current single-point 3D collision. Verified via a synthetic-world test in
      `VerifyBlupiMovement`.
      **Corrected 2026-07-16**: this entry was stale on 2 counts. (1) **Vehicle immunity fixed**
      2026-07-16 as part of that session's broader vehicle-gate audit — Overcraft/Jeep/Tank now
      correctly grant immunity (`GEBlupiController::HasVehicleHazardImmunity()`); focus still isn't
      modeled (no such concept exists). (2) **Drip (404) was already done 2026-07-14** (see
      `TILE-032`) — this entry's own "Drip NOT done, blocked on ThinMechanical" claim predates that
      and was never updated; Drip is a real, placeable `BlockTypes` constant and shares the same
      vehicle-immunity fix as Spike.
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
      **Vehicle-immunity gate fixed 2026-07-16** (the "vehicle-immunity... not modeled" note just
      above was correct when written but is now stale): verified directly against
      `Decor.cpp:5529-5531` — real switch activation excludes Overcraft/Jeep/Tank/Skateboard and
      Balloon (NOT Helicopter, which real source allows — a hovering Helicopter can still reach
      down and press a switch, unlike the other 4). Gated in the caller
      (`GalaxyEggbertCnaGame.cpp`, right before the `TryActivateSwitch()` call only — NOT the whole
      shared action-button block, since Dynamite/Perso/vehicle mount-dismount share the same press
      with their own independent real gates, and dismounting must still work while riding).
      `TryActivateSwitch()` itself is unchanged (still has no `GEBlupiController` access, matching
      the established decoupling). Not unit-testable at this layer (no test harness for
      `GalaxyEggbertCnaGame` itself); verified via full regression suite (only the pre-existing
      unrelated `easy-gl-resource-smoke-tests` failure) + a live headless launch/exit smoke check,
      both backends.
      **Render mode fixed (2026-07-11, same day, live user re-check)**: Saw/SawStopped
      (378/379) were rendering as plain `UniformCube`s (falling through every special-geometry
      table) — user reported "nema to byt na krychly" (shouldn't be on a cube). Added both icons
      to `GEInnerFlatPlateTiles.cpp`'s confirmed table (a genuinely thin double-sided plate
      through the block's middle, outer 6 faces never drawn), superseding an earlier
      questionnaire pass' undecided "ThinMechanical" placeholder categorization —
      `mobile-eggbert-reference/02-tiles.md`/`questionnaire-all-remaining-tiles.md` both updated.
      `GetInnerFlatPlateAxis()` initially special-cased Saw/SawStopped to `PlateAxis::X` (not the
      usual Z default) since the one real placement (this switch+saw pair) sits in a corridor
      Blupi walks along X — a Z-axis plate is invisible edge-on from that approach (confirmed
      live). Verified live via a headless EasyGL screenshot at the real placement (temporary
      camera-pose debug override, reverted before committing, needed because this exact spot also
      triggers the tunnel-ceiling `GroundHeightAt()` limitation tracked in `NEXT.md` §5 — Blupi's
      own Y resolves onto the roof there, so the debug camera had to be positioned independently
      of `blupi_`'s corrupted Y for this one screenshot) — confirms a genuine thin plate showing
      the real jagged circular-blade texture, with the tunnel's red wall visible around/through it
      (proving the cube's outer faces are truly not drawn).
      **Positioning + rotation revised again same day (second live user feedback round)**: the
      plate was centered mid-block, reading as "floating" rather than a floor-mounted blade —
      fixed with a Saw-specific `kSawPlateHeight=0.5` (shorter than the shared
      `kInnerFlatPlateHeight=0.9`) bottom-anchored at the block's own bottom face
      (`IsGroundAnchoredPlateIcon()`), leaving every other confirmed `InnerFlatPlate` icon
      unaffected. The hardcoded `PlateAxis::X` icon default was ALSO wrong in principle (correct
      only for this one placement's corridor direction, not for a future Saw in a Z-running
      corridor) — replaced with new `GEPlateRotationMetadata.hpp/.cpp`
      (`src/GalaxyEggbertCNA/Game/`, deliberately Easy3D/CNA-independent so world-authoring tools
      can set it without linking Easy3D, matching `GEBlupiController`/`GEWorldRuntime`'s own
      precedent), a 1-byte per-PLACEMENT "rotated 90°" flag stored via `Worlds::World`'s existing
      sparse block extra-metadata mechanism (`kPlateRotationMetadataType=2`, next after
      `MoveObjectRecord`'s own `kMoveObjectMetadataType=1`). Saw's own icon default reverted to
      the shared `Z` axis; `GetInnerFlatPlateAxis()` now takes a `rotated` bool and swaps X<->Z.
      `GETerrainRenderer` collects all rotated positions once per (re)build into a packed-key hash
      set for O(1) lookup across its 3 `AppendSpecialGeometry()` call sites.
      `tools/GenerateSampleWorld3D.cpp` now calls `SetPlateRotated(world, 70, 0, 67, true)`
      explicitly for the real switch+saw pair. Verified: metadata round-trips through `.vwr`
      save/load (standalone check), full 5-tool suite + both backends re-verified, and live via
      headless EasyGL screenshots at multiple distances — the blade now sits low near the floor
      with visible wall space above it, clearly different from the earlier centered look.
      **Anchor direction fixed again same day (3rd live user feedback round, with an actual
      screenshot this time)**: the round-2 fix bottom-anchored the plate to the block's own `-0.5`
      face, but neighboring floor tiles' own walkable surface sits at `+0.5` (their solid tops),
      not `-0.5` — the blade sat entirely BELOW the visible floor line, reading as buried/cutting
      into the ground ("řeže do země") instead of poking up where Blupi walks ("měla by řezat
      nahoru"). Re-anchored to the block's TOP face instead (`center.Y + 0.5f - kSawPlateHeight *
      0.5f`), extending downward from there. Confirmed live: the screenshot shows the blade now at
      the top of its recessed "pit," flush with the surrounding floor, not at the bottom.
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
      `GalaxyEggbertSimple3D`'s own stomp bounce already used (`kJumpSpeed * 0.65f`).
      **Vehicle-dismount-first branches fixed 2026-07-16** (this entry's own title already
      described the real behavior, written 2026-07-11 before vehicles existed -- the body above
      was accurate about the gap, just stale relative to the title): touching a spring while
      riding ANY vehicle now forcibly ejects Blupi first (small shake + channel 10, depositing the
      vehicle pickup back into the world), unless Shield/Hide is active (matching
      `Decor.cpp:2837-2893`'s real per-vehicle `!m_blupiShield && !m_blupiHide` guard) -- the
      bounce itself still applies on the same contact (real source doesn't treat these as mutually
      exclusive). New shared `GalaxyEggbertCnaGame::DismountAndDepositVehicle()` helper (factored
      out of the existing voluntary action-button dismount, which had the same logic inline).
      Verified via full regression suite (both backends, only the pre-existing unrelated
      `easy-gl-resource-smoke-tests` failure) + live headless launch smoke check -- not unit-
      tested directly (no test harness at the `GalaxyEggbertCnaGame` layer, same as every other
      fix at this layer this session). Also ported: the real generic
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
      - **Follow-up render fix, 2026-07-12** (user report, Czech, verbatim): "tvar hrotu je, zda se
        spravne, problem je ze ten hrot se renderuje az pod tu krychly a ty hroty tam jsou dvakrat
        ty hroty pod tou deskou zrus a nech jenom ty hroty pod tim, hroty podtim posun nahorou aby
        byly rovnou pod deskou" (the tip's shape is fine, but the tips render twice; remove the
        one under the panel, keep only the one below it, and move it up flush under the panel).
        Root-caused via a live headless screenshot (world isolated down to just this one pillar,
        background disabled, to rule out every other on-screen object first): the cube's own 4
        side faces reuse the WHOLE tile texture (the shared `kSymmetricEntries` convention every
        other icon in `GEDirectionalCubeTiles.cpp` uses) — which includes the SAME lower-two-
        thirds "post/spike" graphic `TeleporterTipUv()` already crops out for the real 3D
        `PyramidTipItem` hanging below. Alpha-cutout on that graphic made the cube's own flat side
        face look like a second, fake, flattened spike sitting immediately below the panel — right
        where the real 3D tip already hangs — reading as "the tip renders twice" exactly as
        reported. Fixed by adding a teleporter-specific override in
        `GETerrainRenderer.cpp::AppendSpecialGeometry()`: the 4 side faces now sample only the top
        third of the tile (new `TeleporterPanelUv()`, the mirror crop of `TeleporterTipUv()`) —
        the panel graphic stretched across the full face, no more baked-in spike shape. The real
        3D tip itself needed no repositioning (already flush with the cube's bottom face,
        confirmed via a vertex-level dump: cube Y range exactly [1.5,2.5], tip exactly [0.9,1.5],
        a single instance, matching the design). Verified live: isolated the world down to just
        this one pillar (temporary debug filter, reverted before committing) and screenshotted
        from 3 different angles/distances before and after — before showed 2 distinct stacked
        cone shapes with a visible gap; after shows one clean panel + one correctly-flush tip, no
        duplication, confirmed from a straight-on view and a 3/4 angle showing both textured side
        faces. Full suite re-run after the fix (63/63 unit tests, all 4 verify tools) + both
        backends.
- [x] `148` Water breath gauge (91/92) — 3-state machine (Surf/Nage/dry), ~25s gauge. Done
      2026-07-12, verified directly against `Decor.cpp` via
      `mobile-eggbert-reference/12-hazards-and-interactables.md`'s "Water depth state machine"
      section. **Real architectural fix required first**: water blocks were solid-for-collision
      (any non-air block counted as solid ground, so Blupi always rested ON TOP of the topmost
      water layer, exactly like land) — made water ALWAYS non-solid in
      `GEBlupiController::GroundHeightAt()` (same precedent as the teleporter pillar/fan head),
      so Blupi genuinely sinks through any depth of water to the real floor beneath it. The
      tile at his own resting cell vs. the tile one above it (new `GetBlockTypeAt()`, mirroring
      `GetGroundBlockType()`/`GetBlockTypeAbove()`) then reproduces the real `IsSurfWater`/
      `IsDeepWater` distinction: water-with-dry-above = Surf, water-with-water-above = Nage. New
      `GEBlupiController::IsSurf()`/`IsNage()`/`GetWaterGaugeLevel()`/`JustDrowned()`, set via two
      new `Step()` parameters (`inSurfWater`/`inDeepWater`, both default false, caller-computed
      exactly like `tempPassable`). Gauge: 100→0 over the real ~25s (`kWaterGaugeTickSeconds =
      5/20 = 0.25s/level`, a direct `Config::ScaleTime(5)`-at-20Hz transcription, same technique
      already used for `kTeleportDuration`), resets to full the instant Nage ends. Drowning plays
      the real dedicated channel 26 (distinct from every other death cause, confirmed via
      `07-sounds.md`) via the existing shared `triggerDeath()` lambda. Channel 25 (start-surfing)
      plays on Nage→Surf. **Correction 2026-07-17**: channel 22 was originally wired to play
      entering Surf/Nage from dry — backwards; real ch22 plays on water EXIT, not entry (see
      `SOUND-032`'s full correction and `PICKUP-078`/`079`/`080` for the real entry/ambient splash
      sounds, ch23/24/64, added the same day). Nage also gets reduced ("floaty") gravity
      (`kNageGravityMultiplier`, an
      approximation, same shape as the wasp balloon's own gravity multiplier) and a swim-up jump
      (`kSwimUpSpeed`, also an approximation) instead of the normal ground jump. **Not modeled**
      (documented simplifications, same pattern as every other Phase 14 mechanic): the real
      "Jump near the surface launches you clear of the water" fine-grained sub-tile-depth nuance
      (-16/-12, Power-gated); SuperBlupi drowning immunity (no such concept exists).
      **Shield/Hide drowning immunity fixed 2026-07-16**: verified directly against
      `Decor.cpp:4615-4620` — the water gauge only decrements at all while NOT Shield/Hide
      (`!m_blupiShield && !m_blupiHide` wraps the decrement itself, not a separate immunity check
      after the fact). This engine's gauge previously decremented unconditionally. New
      `VerifyBlupiMovement` assertion (gauge stays at `kWaterGaugeMax` after 15s submerged with
      Shield active, well past the point an unprotected Blupi would have lost over half of it).
      **Vehicle exclusion fixed 2026-07-16** (this entry previously mis-described it as "vehicle
      forced-dismount-on-entry" — verified directly against `Decor.cpp:5284-5285`: it's not a
      dismount at all, water simply never registers as Surf/Nage while riding ANY vehicle or
      Balloon/Ecrase — a vehicle just drives straight over/through water instead, no special
      handling needed beyond skipping the detection). New `canEnterWater` gate in
      `GalaxyEggbertCnaGame.cpp`, computed before `Step()`. A pre-existing
      content bug was found and fixed while implementing this: the sample world's tunnel water
      crossing sat directly on the world floor (y=0) with nothing beneath it (world Y can't go
      negative) — once water became non-solid this would have turned a shallow wade into a
      bottomless-pit death trap; moved the water to y=1, floor intact at y=0. A new genuinely
      2-layer-deep pool (open-sky room, x=61-67/z=71-77) was added so Nage/drowning is live-
      playable, not just unit-tested. Verified: 6 new `VerifyBlupiMovement` assertions (shallow
      pool = Surf, deep pool = Nage, full gauge-to-drowning cycle at the correct ~25s+fall-time
      mark, gauge-reset-on-resurface) + full suite (63/63 unit tests, all 5 verify tools) + a
      live headless run with temporary debug instrumentation (spawn override + periodic
      position/state log, reverted before committing) confirming the exact real-time sequence:
      falls in, Nage detected, gauge ticks down steadily, drowns and respawns via the existing
      FIFO safe-position system, gauge resets — both EasyGL and Vulkan backends. **Phase 14 is
      now 10/10 complete.**
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
        same architecture as the teleporter pillar). Real sub-tile-band gating is NOT modeled (no
        sub-tile position exists). Shield/Hide immunity IS modeled (`!blupi_.IsInvincible()` at the
        call site, `GalaxyEggbertCnaGame.cpp` -- this note previously claimed it wasn't, stale since
        `170` landed); focus/SuperBlupi still aren't (no such concepts exist) -- contact anywhere in
        the cell is unconditionally lethal otherwise, same
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

### Phase 15 — Crates, lifts, bridges, effects: full fidelity (`E3D-MIG-150`-`159`)

Extends the basic patrol/push already shipped in `GEInteractionSystem`. Full spec:
`mobile-eggbert-reference/14-crates-lifts-bridges-effects.md`.

- [x] `150` Linked-crate flood-fill (`SearchLinkCaisse` equivalent) — done 2026-07-12. Pushing a
      crate now flood-fills every other crate whose box touches an already-linked one (1 grid
      unit in X or Y, same Z), restricted to crates AT OR ABOVE the seed's own row (real
      "push the stack, not the floor it rests on" restriction) — all linked members are tested
      (floor support only for members at the seed's own row, matching the real source exactly)
      and moved atomically; any one member blocked cancels the whole push. Reduced-push-speed-
      scales-with-stack-size is NOT modeled — this engine's crate push is a discrete per-frame
      grid-cell snap (not the real continuous px/tick system), so there's no existing "speed" to
      scale; see `151`. New linked-crate demo (2 side-by-side + 1 stacked) added to the sample
      world for live verification, plus 3 new `VerifyInteractionSystem` assertions (seed pushed,
      neighbor moves the same net distance, stacked crate moves too). Full suite (63/63 unit
      tests, all verify tools) re-verified.
      **Vehicle-mode gate fixed 2026-07-16**: verified directly against `Decor.cpp:6130-6132` —
      real crate push also excludes every vehicle mode + Balloon/Ecrase. New
      `blupiCanPushCrate` parameter on `GEInteractionSystem::Update()` (vehicle+Ecrase, computed
      in `GalaxyEggbertCnaGame.cpp`; Balloon already covered by the existing `blupiBallooned`
      parameter). New `VerifyInteractionSystem` assertion (crate doesn't move with
      `blupiCanPushCrate=false`). Full suite green both backends.
- [ ] `151` Crate "pop" push variant (landing-into-crate, different base speed) and the real
      20-tick speed ramp-up (vs. today's simplified constant-speed push) — NOT started. The pop
      trigger (landing from a fall while moving horizontally into a crate) needs coordination
      with `GEBlupiController`'s air/fall state that doesn't exist yet; the speed ramp doesn't map
      cleanly onto this engine's discrete "snap by 1 grid cell per satisfied frame" push model
      (see `150`'s own note) without a deeper rework of crate movement to be continuous/timed
      rather than instant. Deferred, not attempted this session.
- [x] `152` Platform boarding — done 2026-07-12, verified against `Decor::AscenseurDetect`/
      `MoveObjectStepLine` (~9184/~8005-8174). `GEInteractionSystem` now detects (before its own
      per-frame lift-patrol step) whether Blupi's position matches an active lift's current
      surface (X/Z within its footprint, Y at its stand height), remembers the lift, then after
      the patrol step reports the lift's own displacement this tick (`IsRidingLift()`/
      `RideDeltaX()`/`RideDeltaZ()`/`RideStandY()`) for the caller to apply via a new
      `GEBlupiController::RideLift(x,y,z)` (snaps position, marks grounded, zeroes vertical
      velocity — NOT a full `SetPosition()` teleport, since this runs every single frame while
      riding). Unifies the real source's two separate functions (initial-catch-while-falling vs.
      continuous glue) into one per-frame check, since this engine's discrete position doesn't
      need that real pixel-level distinction. X/Z apply as a DELTA (preserving Blupi's own
      walking input that frame), Y snaps absolutely (matching the real source's own "correct
      drift every frame" approach). Real 30px swept multi-step tunnelling prevention is NOT
      modeled — this engine's gravity/dt doesn't cross a lift's height in one frame under normal
      conditions. Verified live (temporary debug instrumentation, reverted before committing):
      Blupi's Y tracked the north-hill lift's ping-pong patrol exactly, staying grounded the
      whole ride, both before and after refactoring the logic from the game loop into
      `GEInteractionSystem` for testability. New `VerifyInteractionSystem` assertions (riding
      true while positioned on the lift, false when far away, `RideStandY()` matches the lift's
      post-patrol-step height) + full suite (63/63 unit tests, all verify tools) + both backends.
- [ ] `153` Vertigo edge-detection with auto-slide-off for wide/shiftable lift platforms
      (icons 311-316) — NOT started. Needs a new render/icon-selection decision (which of
      types 1/47/48 render as the "wide" 311-316 frames) that the user asked to defer
      (2026-07-12, new visual-design decisions explicitly skipped this session).
- [x] `154` Conveyor nudge (±2px/tick) for caterpillar-track types 47/48 — done 2026-07-12,
      folded into `152`'s `RideDeltaX()` (a constant `kConveyorNudgeSpeed` added/subtracted for
      types 47/48 respectively). The exact real 2px/tick has no established unit-conversion for
      this engine's grid scale, so the magnitude is a documented approximation, not a
      transcription.
- [x] `155` Dynamite — done 2026-07-12, verified directly against `Decor.cpp` (~4792-4812 pickup/
      placement gate, ~8252-8296 fuse timing, ~9058-9175 per-blast effect — not just the
      reference doc's rounded summary). Pickup (`ObjectType55`) caps at exactly 1 carried (real
      `m_blupiDynamite`, a second does nothing until the first is placed). `GEInteractionSystem::
      PlaceDynamite()` (action-button, gated on carrying one + grounded) spawns a real
      `ObjectType56` fuse object. The fuse's `phase` (already advanced generically by
      `GEWorldRuntime::Update()`) drives the exact real 9-blast sequence — ticks 50/53/55/56/59/
      62/64/67/69 with their exact real per-blast `(dx,dy)` pixel offsets (read directly from
      `Decor.cpp`, not approximated), /64 to this engine's grid units, real X/Y-only 2D-source
      convention (Z always 0). Each blast: clears Saw/SawStopped hazard tiles in its 2×2-tile
      area (drip hazards 404/410 skipped — no placeable `BlockTypes` constant for them yet),
      destroys every active object of the real exact 28-type list (ported directly from
      `Decor.cpp` ~9102-9132, not approximated) overlapping that area — crates via the same
      linked-group flood-fill as `150` — and kills Blupi if caught (real Shield/Hide/SuperBlupi
      gating unconditional, Phase 17 dependency). Only the center blast plays the boom (channel
      10); no debris/particle visuals (no such system exists). Fuse self-destructs once its
      sequence completes. New dynamite pickup + linked-crate target added to the sample world
      (lift room B) for a genuinely playable pickup-then-blast scenario. Verified: 10 new
      `VerifyInteractionSystem` assertions (pickup cap, placement gate, full blast sequence
      destroying a synthetic target crate + costing Blupi a life + fuse self-destruct) + full
      suite (63/63 unit tests, all verify tools) + both backends.
      **Vehicle-mode gate fixed 2026-07-16**: verified directly against `Decor.cpp:4792-4794` —
      real placement (both Dynamite AND Perso, the same `if`/`else if` pair) excludes EVERY vehicle
      mode (unlike switches, Helicopter is NOT exempt here) plus Balloon/Ecrase. New
      `blupiCanUseHands` parameter on `PlaceDynamite()`/`TryPerso()` (default `true`), computed in
      `GalaxyEggbertCnaGame.cpp` from `!IsInVehicle() && !IsBallooned() && !IsEcrased()`.
      Deliberately does NOT gate `TryPerso()`'s pickup branch (real `Decor.cpp:6088-6101` has no
      vehicle clause on picking up an already-placed decoy) — verified by 2 new
      `VerifyInteractionSystem` assertions per function (blocked placement, still-working pickup).
      Full suite green both backends + live headless launch smoke check.
- [ ] `156` Helicopter-destruction / debris pool (ballistic pop-then-drop physics), shared by
      crate destruction too — NOT started. The helicopter trigger itself needs vehicles (Phase
      17, not implemented); the debris-pool VISUAL effect (also used for destroyed crates in a
      dynamite blast) needs a particle/fragment rendering system that doesn't exist yet. `155`'s
      own crate destruction works correctly without it (no debris visual, a documented
      simplification matching every other missing-particle-system gap this session).
- [x] `157` Bridge live collision toggle — done (`ed60898`, PICKUP-064; this checkbox was stale).
      `table_bridge` overwrites the actual terrain grid cell every tick across a 157-tick build
      sequence (solid only ticks 0-15/152-156) — real, not purely cosmetic. Verified by 7
      `VerifyInteractionSystem` assertions (bridge tile found, mid-construction cell genuinely
      non-solid, Blupi falls through, cell restored + construction object self-deletes at tick
      157).
- [x] `158` "Voyage" pickup-reward pattern — done 2026-07-14, verified directly against
      `Decor::VoyageInit`/`VoyageStep`/`VoyageDraw` (`Decor.cpp:10141-10348`). Real mechanic: a
      pickup deletes its world object on contact but does NOT apply the reward immediately — a
      2D "fly to HUD icon" animation runs first (start/end point in mobile-eggbert's 640x480
      reference screen space, `total = (|dx|+|dy|)/10` real integer-truncated-Manhattan ticks,
      `phase += dt*20` — the established `AdvancePatrolStep` tick convention); the reward applies
      exactly when `phase >= total`. Only one voyage runs at a time — starting a new one
      force-completes the previous one's reward immediately (real `if (m_voyageIcon != -1) {
      phase = total; Step(); }`).
      - In scope, all confirmed via direct `Decor.cpp` reads: Treasure (icon 6/Element, end
        (430,430), reward `treasuresCollected_++` + treasure-door rescan), Key1/2/3 (icons
        215/222/229/Element, ends (520,418)/(530,418)/(540,418), reward = set key bit), Egg
        (icon 21/Element, dynamic end `(210+16*(lifeEggCount_+1), 417)`, reward
        `lifeEggCount_++`/`lives_++` capped), Dynamite (icon 252/Element, end (505,414), reward
        `dynamiteCount_++`), Perso (icon 108/Button, end (0,438), reward `persoCount_++`),
        BulletPack (icon 177/Element, end (570,430) — real reward is NOT deferred, granted
        immediately at touch; only the completion sound is deferred), DoorUnlock (icon
        `214+(doorIcon-334)*7`, REVERSED direction — start = the fixed HUD position of whichever
        key was consumed, end = the door's own world position; no reward at completion since the
        key bit is cleared before the voyage starts, purely a visual flourish).
      - Found and fixed touch-time-sound mismatches while porting this (previously this engine
        either played the wrong channel, played it at the wrong time, or played nothing): Egg now
        plays `ch12` immediately (was incorrectly playing the deferred `ch3` early); Perso now
        plays `ch60` immediately (was previously silent — `TryPerso()` had zero `sound.Play`
        calls); DoorUnlock correctly plays no immediate sound (its dynamic icon never matches any
        of `VoyageInit`'s fixed-icon checks). Treasure/Key1/2/3's existing immediate sounds were
        already correct. All 6 reward-bearing kinds now also play the real deferred completion
        sound (`ch3`) exactly when `ApplyVoyageReward()` fires. **Correction (2026-07-14, found
        while re-reading `Decor.cpp` for the death-VFX follow-up task): Dynamite (icon 252) and
        BulletPack (icon 177) DO have real immediate sounds after all — `ch60` and `ch54`
        respectively (`Decor.cpp:10141-10226`, freshly re-read directly) — an earlier pass's
        research had wrongly concluded both were silent. Both are now fixed to play their real
        immediate sound; re-verified on both backends (78/78 minus the pre-existing unrelated
        `easy-gl-resource-smoke-tests` failure on `build-cna`, 73/73 on `build-cna-vulkan`).**
      - Architecture: `GEInteractionSystem` has zero camera/graphics dependency — pickup sites
        record a same-frame pending request (`RequestVoyage()`); after `Update()`/`TryPerso()`
        return, `GalaxyEggbertCnaGame::ResolvePendingVoyage()` (which owns `camera_`) projects the
        pickup's world position into 640x480 reference space via the new
        `GEHud::ProjectWorldToHudSpace()` (uses `Vector4::Transform` for the real clip-space W,
        then inverts `GEHud`'s own existing ref-space<->viewport mapping) and calls the public
        `BeginVoyage()`. This mirrors the already-established pattern used by
        `SpawnInvertBurst()`/`SpawnTeleportArc()`/`ResetMagicTrail()`, and avoided adding camera
        parameters to `Update()`'s already-large (~20-parameter) signature.
      - Corrected a wrong premise from an earlier session note: `BlupiAction::Clear1`-`Clear8` are
        8 distinct real Blupi DEATH-animation types (not related to a 3rd Invert-grant site as
        previously assumed) — Clear3/Lava and Clear2 fire their own "soul ascends" Voyage
        (icon 40/230), Clear4/Saw fires an unrelated `ObjectType41`-reusing particle burst. This
        means no death in this engine currently has ANY of these real VFX — a genuinely separate
        **"death VFX" system**, explicitly deferred as a future task, not part of this pass (along
        with the life-loss icon-48/Blupi-channel animation, which ties into respawn/death-lock
        control flow).
      - Verified: 18 existing `VerifyInteractionSystem` assertions updated for the new deferred
        timing (treasure/keys/egg/dynamite/Perso/both door-unlock kinds), plus new dedicated tests
        for the interpolation/reward-timing math, the force-complete-on-new-voyage interaction,
        and `GEHud::ProjectWorldToHudSpace()`'s math against a controlled `Easy3D::Camera3D`.
        Full suite: 78 tests on `build-cna` (99%, only the pre-existing unrelated
        `easy-gl-resource-smoke-tests` failure), 73/73 (100%) on `build-cna-vulkan`.
- [x] `159` "Death VFX" (Clear2/Clear3/Clear4 death-animation VFX) — done 2026-07-14, the
      follow-up flagged by `158` above. Verified directly against `Decor::BlupiDead`
      (`Decor.cpp:6547-6614`), every one of its real trigger call sites (fall-off-world
      `Decor.cpp:2754-2761`, Ventillo/fan `5458-5471`, Lava `5497-5499`, Piege/Goutte
      `5504-5519`, Saw `5521-5524`, Blitz `5541-5547`, the generic 8-type hazard-contact list
      `5788-5795`/`6103-6106`, dynamite blast `9172`), and `Decor::ObjectStart`'s real direction/
      magnitude decoding (`Decor.cpp:7805-7869`).
      - Of Blupi's 8 real `BlupiAction::Clear1`-`Clear8` death-animation types, only 3 turned out
        to have any actual VFX: **Clear1** has none at all (just whatever death sound the hazard's
        own contact site plays); **Clear5**-**Clear8** are confirmed DEAD CODE (defined in the
        enum, given a duration table entry, and excluded by every "is this a Clear-type action"
        guard, but never assigned anywhere in real source — grepped the whole real codebase to
        confirm). `BlupiAction::Glu` (spikes/drip/fired-projectile-contact/large-creature-grab
        deaths) is a wholly separate, un-researched "stuck" mechanic, explicitly out of scope
        here — this feature covers only Clear2/Clear3/Clear4.
      - **Clear2** ("soul ascends" 300px, real trigger: fall-off-world deterministically, Fan and
        the generic-hazard-contact list each a real 50/50 coinflip vs. Clear1) and **Clear3**
        ("soul ascends" 2000px, Lava, deterministic) both reuse the Voyage machinery `158` built
        (`VoyageKind::Clear2Ascend`/`Clear3Ascend`) — but unlike every pickup kind, BOTH endpoints
        derive from Blupi's OWN position (start = Blupi projected to HUD space via
        `GEHud::ProjectWorldToHudSpace()`, end = straight up from there by a fixed HUD-space
        offset), a natural technical adaptation of the real `pos`/`pos2` (both computed in 2D
        decor-pixel space before a shared `HotSpotToHud()` transform this engine has no equivalent
        2D scrolling space for). Real fixed, NON-distance-proportional durations (`Decor.cpp:
        10222-10226`, confirmed by direct re-read) — Clear2 total=100 ticks, Clear3 total=50 —
        override the generic `(|dx|+|dy|)/10` formula entirely. Clear2's icon animates (230→241,
        cycling every 2 ticks, `Decor.cpp:10241-10252`); Clear3's icon (40) is static but has a
        real 30-tick pre-move delay (position clamped to start, icon hidden,
        `Decor.cpp:10310-10318`) before it starts rising, plus a continuous `ObjectType93` puff-
        particle spawn every tick while active (`Decor.cpp:10331-10348`, real 7-value horizontal
        scatter table + wider vertical random range, both halved/quadrupled respectively during
        the pre-move delay) — ported as a small world-space jitter around Blupi's own death
        position (no 2D decor-pixel space to reproduce the real conversion against). Neither
        ascend has a reward at completion (confirmed: neither icon appears in `VoyageStep`'s own
        completion if-chain).
      - **Clear4** (Saw, deterministic) is NOT a Voyage — it is 3 `ObjectType41` particles (up/
        right/left, no "down") reusing the same real object type as the already-shipped Invert
        burst, but with a LARGER real magnitude (20 vs Invert's 10, decoded via `Decor::
        ObjectStart`'s own direction/magnitude logic from real speeds -70/20/-20) giving
        `stepAdvanceTicks=156` instead of Invert's 78, plus channel 75 (played once, matching real
        source's own single call site for that sound — the existing Saw death-site code already
        played channel 75 itself and had to be changed to NOT double-play it).
      - **First randomness anywhere in this engine's gameplay code** (`GEInteractionSystem`'s new
        `rng_`/`RollClear2Coinflip()`, seeded from `std::random_device`) — real `Decor::BlupiDead
        (action1, action2)`'s own `m_random.get()->Next() % 2 == 0` coinflip choice
        (`Decor.cpp:6551-6554`), genuinely non-deterministic in real source and cosmetic-only here
        (never affects `lives_`/reward state, only whether the ascend VFX/sound plays). New
        `VoyagePendingIsAscend()`/`RequestClear2Ascend()` extend the existing pending-request
        round-trip (`158`) for the ONE trigger site with no camera access (the generic-hazard-
        contact coinflip, inside `GEInteractionSystem::Update()`) — the other 4 trigger sites
        (fall/Lava/Saw/Fan) live directly in `GalaxyEggbertCnaGame.cpp`, which already has
        `camera_` in scope, so they call `BeginVoyage()`/`SpawnSawDeathBurst()` directly via a new
        `triggerDeathAscend` lambda alongside the existing `triggerDeath`.
      - Found and fixed a real pre-existing bug while wiring this: the Fan hazard's own
        `SpawnFanHitFlash()` call read `blupi_.GetX/Y/Z()` AFTER `triggerDeath()` had already
        respawned Blupi, spawning the flash at the RESPAWN position instead of the death position
        — fixed by capturing the death position before calling `triggerDeath()`, same fix applied
        to the 3 new ascend/burst call sites for the same reason.
      - Verified: new `VerifyInteractionSystem` tests for Clear2Ascend's fixed-duration override +
        icon-cycle animation + no-reward completion, Clear3Ascend's fixed duration + pre-move
        delay + puff-particle spawn (position and jitter bounds), Clear4's exact 3-direction burst
        (up/right/left, no down) with the real `stepAdvanceTicks=156`, and a statistical test
        confirming `RollClear2Coinflip()` produces both outcomes over 200 trials. Full suite: 78
        tests on `build-cna` (99%, only the pre-existing unrelated `easy-gl-resource-smoke-tests`
        failure), 73/73 (100%) on `build-cna-vulkan`.
      - **Still explicitly out of scope**: the real `Glu` "stuck" death mechanic (spikes/drip/
        fired-projectile-contact/large-creature-grab) and the life-loss icon-48/Blupi-channel
        Voyage (which ties into respawn/death-lock control flow — a separate behavior change, not
        touched here).

**Unnumbered follow-up "death-lock" (no `E3D-MIG` ID — would collide with Phase 16's own
`160`-`165` range): done 2026-07-14, user-approved after the research below.** Implementing `Glu`
faithfully turned out to require a shared death-lock + life-loss-Voyage system covering EVERY real
death type (Clear1-4/Glu/Drown), not just Glu — real per-cause fixed lock duration
(`Decor.cpp:6374-6392`, `m_blupiPhase == Config::ScaleTime(N)`: Clear1=70, Clear2=100, Clear3=70,
Clear4=110, Glu=100, Drown=90 ticks; Clear5-8/Electro unreachable/unmodeled, skipped), THEN a real
life-loss Voyage (icon 48/Blupi channel, fixed `ScaleTime(40)`=40 ticks, decrements `lives_`
**at Voyage START, not completion** — the opposite timing direction from every Voyage `158`/`159`
built — plays channel 9, start = the real lives-HUD-icon position using the PRE-decrement count,
end = Blupi's own post-respawn position). Only THEN does control return.
- **Real `m_blupiRestart` (respawn-at-safe-position) nuance, confirmed by directly re-reading
  every trigger site** (a correction found while implementing, not caught by the initial
  research): **3** of the real death causes do NOT respawn Blupi at the safe FIFO position
  (Fan, the generic 8-type hazard-contact list, AND dynamite blast — the last one missed by the
  initial research, which only flagged the first two) — confirmed via direct grep, none of
  these 3 sites' real code sets `m_blupiRestart=true` anywhere nearby, unlike the other 9 real
  sites which all do. Faithfully, dying to these 3 causes leaves Blupi exactly where he died
  (once un-Hidden) instead of teleporting him away.
- Architecture: `GEBlupiController` gained `TriggerDeathLock()`/`IsDeathLocked()`/
  `IsDeathHidden()`/`ConsumeDeathLockResolved()`, reusing the EXACT proven freeze-timer template
  already established by `TriggerTeleport()`/`m_teleporting` — two chained frozen sub-states
  (the lock, then the life-loss-Voyage window) instead of one. `GEInteractionSystem` stays fully
  decoupled from `GEBlupiController` (no shared type, matching `158`'s own precedent) — the one
  real trigger site living inside `Update()` itself with no camera/controller access (the
  generic-hazard-contact coinflip) uses a small LOCAL `PendingDeathKind{Clear1,Clear2,Glu}` enum
  + a `*ThisFrame()` pending signal, consumed by the game class's new `ResolveDeathLock()`
  (called alongside the existing `ResolvePendingVoyage()`). The other 8 real trigger sites
  (fall/lava/spike/drip/blitz/saw/fan/drown) already live directly in `GalaxyEggbertCnaGame.cpp`
  and call `TriggerDeathLock()` straight away. `VoyageKind::LifeLoss` reuses the existing Voyage
  machinery with a fixed 40-tick total override; `LoseLife()`'s own logic is UNCHANGED, only
  moved from every death-trigger call site to this one deferred resolution point. Game-over
  (`lives_<=1`) is predicted BEFORE starting the Voyage (matching real `else { DoorsLost() }`,
  no Voyage at all in that case). `GEBlupiController::AnimState::DeathLocked` covers both frozen
  sub-states. **Real per-cause hurt-sprite frames transcribed 2026-07-16** (Blupi-model
  format-agnostic prep): `Tables::table_blupi` parsed directly via a small script (validated by
  reproducing the already-approved `kTeleportingFrames` byte-for-byte first) for all 6
  `DeathCause` values — new `kClear1/2/3/4Frames`/`kGluFrames`/`kDrownFrames`, dispatched via a new
  stored `m_deathCause` member. Render-side: third-person model
  draw gets one added `!IsDeathHidden()` clause (first-person already renders no Blupi model).
- **Significant gap found and fixed 2026-07-16** (while researching the large-creature
  `ObjectType54` contact, `136`): real `BlupiDead()` (`Decor.cpp:6547-6614`) unconditionally clears
  vehicle mount/Balloon/Ecrase/every secret power (Shield/Power/Cloud/Hide)/Invert/Nage/Surf/
  Suspend/Ghost on EVERY real death, not just a per-hazard special case. `GEBlupiController::
  TriggerDeathLock()` previously left every one of these completely untouched across death and
  respawn — meaning dying while riding a vehicle, ballooned, squashed, or with an active secret
  power silently carried that state through respawn, unlike real mobile-eggbert. Fixed by adding
  the equivalent unconditional reset directly to `TriggerDeathLock()` (matches `BlupiDead()`'s own
  timing — immediately at the death trigger, not at the deferred respawn point). Real `BlupiDead()`
  does NOT redeposit a mounted vehicle's pickup back into the world (no such `ObjectStart` call in
  it, unlike voluntary/spring-forced dismount) — dying with one mounted just loses it, matching
  this fix (no world-facing action, `GEBlupiController` has none anyway). New `VerifyBlupiMovement`
  assertions (vehicle/Shield/Invert/Balloon/Ecrase all cleared by a death lock). This is a broader,
  more significant fix than the narrow single-mechanic vehicle-gate fixes elsewhere this session —
  it affects the shared death-lock path used by all 6 real death causes.
- Found and fixed a real pre-existing bug while wiring this: the Fan hazard's own
  `SpawnFanHitFlash()`/ascend-Voyage call previously read Blupi's position via `blupi_.GetX/Y/Z()`
  which, before this change, had ALREADY been moved by the old instant-respawn `triggerDeath()` —
  fixed by capturing the death position first (now largely moot since respawn itself is deferred,
  but the capture-first pattern was kept for clarity/consistency).
- Verified: new `GEBlupiController`-level tests (per-cause fixed durations including a spot-check
  of Clear4's distinct 110-tick duration and Drown's 90, full-frozen movement, `IsDeathHidden()`
  timing, `ConsumeDeathLockResolved()` firing exactly once and echoing the real `shouldRespawn`,
  auto-completion) in `VerifyBlupiMovement`, plus every existing `VerifyInteractionSystem` hazard-
  contact test (fall/lava/spike/drip/blitz/saw/fan/generic-hazard ×several enemy types/dynamite/
  drown) updated to advance through the real deferred timing before checking life/position —
  including 2 genuine false-positive fixes found along the way (a `*ThisFrame()` flag / transient
  particle-lifetime check that must run BEFORE the fast-forward, not after; a position-blind
  "last matching type wins" search that broke once the fast-forward let another periodic spawn
  reuse an already-destroyed test object's slot — both fixed by reordering/adding position filters,
  the same defensive patterns already established elsewhere in this file). Full suite: 78 tests on
  `build-cna` (99%, only the pre-existing unrelated `easy-gl-resource-smoke-tests` failure), 73/73
  (100%) on `build-cna-vulkan`.
  - **Live-visual verification, done 2026-07-14 (later, user-requested follow-up):** temporary
    debug scaffold (spawn Blupi directly on the sample world's first Lava tile, skip straight to
    the `Play` phase, capture timed screenshots — all reverted before commit, confirmed via a
    clean `git diff`) confirmed the real sequence live in the actual running game: lives visibly
    drop from 3 to 2 a few seconds after lava contact, and Blupi's camera/position visibly jumps to
    a different (safe-respawn) location — matching the real deferred lock → life-loss-Voyage →
    respawn timing, not an instant death. The exact icon-48 HUD-icon-flight animation itself is
    small at this screenshot resolution/interval spacing and wasn't specifically isolated in a
    frame, but the core state transition (deferred life loss + deferred respawn) is confirmed
    working end-to-end, not just in unit tests.
  - **Follow-up, done 2026-07-20 (NEXT.md §8 non-editor task 1):** isolated the icon-48 flight
    itself. New temporary debug scaffold placed Lava directly under Blupi's spawn (via
    `GEWorldRuntime::GetWorldMutable().setBlock()`, the same grid-coordinate formula
    `GetGroundBlockType()` uses: `round(x+kWorldCenterX)`, `round(z+kWorldCenterZ)`, `round(y)-1`)
    and captured screenshots post-HUD-draw at 0.15s intervals through the transition window
    (gated on `drawFrameIndex_ > terrainPixelPrintedFrame_`, the same Vulkan-mid-draw-readback
    fix `screenshot_hud.png` already established). Confirmed live: a real Clear3 lock (70
    ticks=3.5s) runs concurrently with a Clear3Ascend cosmetic VFX voyage (icon 40, fixed 50
    ticks=2.5s) that completes partway through the lock; at lock-end the LifeLoss Voyage begins
    (icon 48, fixed 40 ticks=2.0s) with `lives_` dropping 3→2 in the exact same frame the icon
    appears (matching the real "decrements at Voyage START" citation precisely) — the icon 48 is
    now visibly present in mid-window screenshots and gone once the Voyage resolves. All debug
    code fully reverted (confirmed via a zero-diff `git diff` after revert), no production changes.

### Phase 16 — Doors & keys (`E3D-MIG-160`-`165`)

Full spec: `mobile-eggbert-reference/06-doors.md`. `160`/`161`/`162` done 2026-07-12; `163`/`164`/
`165` deferred (see each entry).

- [x] `160` Door open sequence — done 2026-07-12, verified directly against `Decor::OpenDoor`
      (~11667). Opening a door (`GEInteractionSystem::OpenDoorAt()`, shared by the key-gated and
      treasure-gated families below) sets the tile to `Air` and spawns a transient `ObjectType22`
      that slides up by exactly 1 grid unit over the real `Config::ScaleTime(50)` = 50 ticks
      (2.5s at the 20Hz reference rate), then self-destructs — handled directly by its own branch
      in `GEInteractionSystem::Update()`'s main loop (a one-shot animation, not the generic
      dwell/advance/dwell/recede patrol, which loops and doesn't fit). Real channel 33. The
      slide object's own icon is NOT rendered accurately — `GEObjectIcons::GetObjIcon()` already
      has no confirmed icon data for type 22 regardless (returns 0), a pre-existing gap unrelated
      to this task.
- [x] `161` Key pickup/consumption — done 2026-07-12, verified directly against `Decor.cpp`
      ~7360-7394 (`IsDoor`, probes Blupi's own cell AND one cell further in his facing direction)
      and ~5619 (key cleared on use). Doors open AUTOMATICALLY on approach while holding the
      matching key (no action-button gate, unlike switches/dynamite) — new `blupiFacingDX`/`DZ`
      `Update()` parameters (caller derives them from `GEBlupiController::GetYaw()`) drive the
      2-cell probe. This engine's `keys1_`/`keys2_`/`keys3_` are plain pickup counters (not a
      persisted bitmask) — modeled as "count > 0 opens, cleared to 0 on use", behaviorally
      identical to the real boolean flag for the realistic case (real levels only ever grant one
      of each key before requiring a re-pickup). **Correction 2026-07-17**: the note below was
      stale as of `158` (done 2026-07-14, same day as this entry but written first) — real
      voyage-deferred key-flag-setting (pickup deleted from the world on contact, but
      `keys1_`/`keys2_`/`keys3_` only incremented once the HUD-fly voyage completes,
      `ApplyVoyageReward()` lines ~2990-2999) IS modeled after all, via `RequestVoyage(VoyageKind::
      Key1/2/3, ...)` at each key's contact site. Superseded, no longer a gap.
- [x] `162` Treasure-gated doors — done 2026-07-12, verified directly against
      `Decor::OpenDoorsTresor` (~11642). A door needing N treasures uses icon `420+N`; the instant
      a treasure pickup completes, the WHOLE terrain grid is scanned and every door in
      `[421, 420+treasuresCollected_]` opens at once (not just the nearest), matching the real
      "newly-qualifying treasure doors across the whole level open together" behavior exactly.
      New key-gated + treasure-gated door demo (2 short corridors, each with exactly one door-tile
      gap in a real wall) added to the sample world for a genuinely playable scenario. Verified:
      11 new `VerifyInteractionSystem` assertions (closed-without-key, opens-with-key,
      key-consumed-on-use, treasure-gate-stays-closed-until-met, opens-on-2nd-qualifying-pickup)
      + full suite (63/63 unit tests, all verify tools) + both backends.
- [ ] `163` Render closed doors as `Billboard` — tracked in `E3D-MIG-516`, cross-referenced here.
      Explicitly SKIPPED this session per the user's 2026-07-12 direction (no new visual/render-
      geometry decisions) — door tiles currently fall through to the terrain renderer's plain
      `UniformCube` default (same as any other unclassified icon) until opened, a pre-existing,
      not-newly-introduced gap.
- [ ] `164` `AdaptDoors` hub-screen logic (gold-flag reveals, icon swaps) — depends on hub/menu
      screens existing (`## 2 §2 MENU-*`), not started, lower priority.
- [ ] `165` World-entry-screen door logic (opens matching sublevel doors, snaps Blupi facing) —
      same menu dependency as `164`, not started.

### Phase 17 — Secret powers, vehicles, buffs, remaining pickups (`E3D-MIG-170`-`179`)

Not started. Full spec: `mobile-eggbert-reference/13-object-pickups.md`,
`10-blupi-mechanics.md`.

- [x] `170` `[?]` Research secret-power (`Sp0`-`Sp7`) behavior — **RESOLVED 2026-07-12, and the
      "Sp0-Sp7" premise itself was WRONG.** Direct `Decor.cpp` research (not just the reference
      doc) found: (1) the real `SecretPower` enum (`def/SecretPower.hpp`) only has 5 values —
      None/Shield/Power/Cloud/Hide — NOT 8; (2) `Decor::IsWorld()` (~7079-7095) proves tile icons
      158-165/166-173 are hub-screen WORLD-SELECT markers (locked/unlocked pairs, per
      `06-doors.md`'s own `AdaptDoors` section), wholly unrelated to Blupi's secret-power buffs —
      the reference doc's own "Sp0-Sp7... likely SecretPower value 0-7" label was a speculative,
      never-confirmed guess based on the decompiled name alone (explicitly marked "likely" in
      `02-tiles.md`, now corrected there); (3) the 4 REAL buffs are granted by `MoveObject`
      pickups instead, confirmed directly: `ObjectType25` (Shield, instant), `26` (Sucette->
      Power), `30` (Drink->Hide), `31` (Charge->Cloud) — `Decor.cpp` ~6014-6087 (pickup gates)
      and ~3048-3235 (the real 2-stage delay before Power/Hide/Cloud activate, NOT modeled here,
      see `172`'s own note). `E3D-MIG-515`'s "render Sp0-Sp7 as gold-pedestal Billboard" task is
      based on the same wrong premise and should be re-scoped as a HUB-SCREEN world-select icon
      (Phase `## 2 §2 MENU-*` territory), not a secret-power pickup — not touched this session.
      Implemented alongside this research (`GEBlupiController`'s new `SecretPower` enum/
      `TriggerShield/Power/Cloud/Hide()`/`IsInvincible()`): the real hazard-immunity gate
      (`!m_blupiShield && !m_blupiHide`, confirmed identical across ~15 separate `Decor.cpp` call
      sites — lava/spikes/saw/blitz/crusher/dynamite/fan/the shared kill-list/wasp/large-
      creature/projectiles) now genuinely protects Blupi, resolving the "Shield/Hide/SuperBlupi
      immunity NOT modeled" caveat left on essentially every hazard implemented earlier this
      session (superBlupi itself still isn't modeled, no such concept exists). New secret-powers
      demo (one of each of the 4 pickups) added to the sample world. Verified: 19 new
      `VerifyBlupiMovement` assertions (trigger gates, exact real per-power decrement rates,
      expiry, warning threshold) + 5 new `VerifyInteractionSystem` assertions (pickup grant
      gating, hazard-immunity integration) + full suite (63/63 unit tests, all verify tools) +
      live headless verification (temporary debug instrumentation, reverted before committing) +
      both backends.
- [x] `171` Vehicle mounts — done 2026-07-12, verified directly against `Decor.cpp` via
      `mobile-eggbert-reference/10-blupi-mechanics.md` §6/`13-object-pickups.md`'s own "Vehicle
      mounts" section. Confirmed real pickup->vehicle mapping: `ObjectType13`->Helicopter, `19`->
      Jeep, `28`->Tank, `24`->Skateboard, `46`->Overcraft (**not** "Balloon" despite
      `ObjectType.hpp`'s own misleading doc comment — `10-blupi-mechanics.md`'s own research
      already found this exact discrepancy: touching `46` sets `m_blupiOver`, not a separate
      Balloon ride; no confirmed pickup grants the real standalone Balloon vehicle, so it isn't
      modeled). New `GEBlupiController::VehicleMode` + `TriggerMount()`/`TriggerDismount()`: real
      gate (blocked while already riding ANY vehicle, or Nage/Surf; **not** gated on Shield/Power,
      confirmed real oddity) and mount silently cancelling Cloud/Hide but leaving Shield/Power
      untouched. Real per-mode horizontal max-speed/accel/decel have no established px-to-this-
      engine conversion factor (unlike vertical fall distance) — preserved the REAL RELATIVE
      proportions between vehicles instead (Jeep fastest, Tank/Overcraft slowest confirmed modes,
      Skateboard/Helicopter between), anchored to an arbitrary-but-reasonable "vehicles feel
      faster than walking" baseline, same technique as `kSpringBounceHeld`. Helicopter/Overcraft
      get real free vertical flight (ascend/descend via the same crouch/lookUp inputs that mean
      camera pitch on foot) instead of gravity; Jeep/Tank/Skateboard reuse the existing ground
      gravity/jump path unchanged (matching the real source's own "uses the shared ground
      gravity/Air path" note for Skateboard). Mount/dismount + the pickup-deposit-back-into-the-
      world logic live directly in `GalaxyEggbertCnaGame.cpp` (not `GEInteractionSystem`, which
      has no access to `GEBlupiController::VehicleMode`) — same action button as switches/
      dynamite. NOT modeled: Balloon vehicle (no confirmed trigger), tilt easing (Jeep/Tank, no
      visible 3D model to tilt anyway), Overcraft's real altitude-cap/inverted-accel-over-gaps
      nuance (simplified to the same ramp as every other mode), and the real Helicopter floating-
      object auto-mount-without-a-button exception.
      **Per-vehicle hazard immunity fixed 2026-07-16** (was the one item above still listed as a
      documented gap): verified directly against `Decor.cpp:5497-5528` — Over/Jeep/Tank grant
      immunity to Spike/Drip/Saw specifically (`!m_blupiOver && !m_blupiJeep && !m_blupiTank`,
      identical clause across all 3), but NOT Helicopter or Skateboard, and NOT Lava/Blitz/Crusher
      (those 3 check only Shield/Hide/SuperBlupi, confirmed no vehicle clause at all — a real,
      deliberate asymmetry, not an oversight). New `GEBlupiController::HasVehicleHazardImmunity()`
      wired into `GalaxyEggbertCnaGame.cpp`'s Spike/Drip/Saw death checks (Lava/Blitz/Crusher
      deliberately untouched). The real safe-position-FIFO gate (`Decor.cpp:6467-6478`) does NOT
      check vehicle state at all even for these 3 tiles (confirmed directly) — left as-is, already
      correct. New `VerifyBlupiMovement` assertions covering all 5 vehicle modes' immunity status;
      the `GalaxyEggbertCnaGame.cpp` wiring itself isn't unit-testable (no test harness at that
      layer) — verified via full regression suite (only the pre-existing unrelated
      `easy-gl-resource-smoke-tests` failure) + a live headless launch/exit smoke check on both
      backends, not a dedicated hazard+vehicle live scenario (lower cost/value than the accessor's
      own direct unit coverage justified for this single boolean-gate change).
      **`TriggerMount()`'s own gate fixed 2026-07-16**: verified directly against
      `Decor.cpp:5649/5669/5687` — real mount gate ALSO excludes Balloon/Ecrase, which were
      missing from `TriggerMount()` entirely (its own header comment wrongly claimed "Ecrase
      already blocks separately via its own state elsewhere" — never actually true, no other
      check anywhere blocked mounting while squashed/ballooned). Now
      `if (IsInVehicle() || inNage || inSurf || m_suspended || m_balloon || m_ecrase)`. New
      `VerifyBlupiMovement` assertions (mount fails while ballooned, mount fails while squashed).
      **Found and fixed a real, pre-existing collision bug while live-testing this** (not vehicle-specific):
      `GEBlupiController::TryMoveAxis()` silently froze ALL horizontal movement once `m_y` fell
      far enough negative during a sustained fall through a floorless column (below roughly -2) —
      the step-up check compared the destination's ground height directly against the falling
      Blupi's own deeply-negative Y, misreading "I'm far below because I'm falling" as "that's an
      unclimbable wall". Fixed by skipping the step-up restriction entirely while airborne
      (`!m_onGround`) — it only makes sense while grounded/walking. Undetected until now because
      every existing fall test dropped straight down with no horizontal input during the fall.
      New demo (one Jeep) added to the sample world. Verified: 1 new regression test for the
      TryMoveAxis fix + 15 new `VerifyBlupiMovement` assertions (mount/dismount gates, Cloud/Hide
      cancellation vs. Shield/Power preserved, real accel ramp, coasting after input release,
      Helicopter ascend/descend) + full suite (63/63 unit tests, all verify tools) + live headless
      verification (temporary debug instrumentation — forced action press + held movement input
      for several seconds, reverted before committing — confirmed continuous driving across
      varied terrain with no freezing) + both backends.
- [x] `172` Shared power-up timer — done 2026-07-12 alongside `170` above (single `SecretPower`
      state + shared gauge, decrementing at each power's own real rate: Shield 0.25s/level (25s
      total), Power 0.15s/level (15s), Cloud/Hide 0.2s/level (20s each) — all 4 direct
      transcriptions from `Decor.cpp` ~5071-5137, not approximations) and their real warning-sound
      thresholds (Shield@10/Power@20/Cloud@25/Hide@20, channels 43/45/56/63).
- [x] `173` Suction-cup(26)/Drink(30)/Charge(31) real 2-stage pickup delay — done 2026-07-14,
      verified directly against `Decor.cpp:6025-6087` (contact) and `3048-3235` (completion).
      Real exact tick counts (not the earlier "~32/36" approximation): Sucette=32 ticks(1.6s),
      Drink=36(1.8s), Charge=64(3.2s) — all three genuinely freeze Blupi (`m_blupiFocus=false`,
      confirmed) for the full duration, a THIRD application of the same freeze-timer template
      already built for `TriggerTeleport()`/the death lock (`GEBlupiController::
      TriggerPickupFreeze()`/`IsPickupFrozen()`/`ConsumePickupFreezeResolved()`). A death lock
      started while pickup-frozen always cancels it (real `BlupiDead()` unconditionally overwrites
      whatever action was active).
      - **Key asymmetry found**: Sucette/Drink are genuinely 2-stage — `m_blupiPower`/
        `m_blupiHide` grant ONLY at completion (32/36 ticks later), not at contact. **Charge is
        NOT actually deferred** — `m_blupiCloud` grants immediately at contact in real source too
        (confirmed) — only the freeze + immediate "grab" sound (ch58) + completion sound (ch55)
        were missing; this engine's existing instant `TriggerCloud()` call was already correct and
        is unchanged. Real source also redundantly re-arms Charge's gauge a second time at
        completion (same values) — NOT re-applied here (a documented, minor simplification: the
        gauge decays for the ~3.2s freeze instead of being refreshed).
      - Real immediate "grab" sounds (previously entirely skipped by this engine — only the
        completion sound played, and only at contact instead of after the delay): Sucette ch50,
        Drink ch57, Charge ch58. Real completion sounds (moved from contact-time to the real
        deferred point): Sucette ch44, Drink ch62, Charge ch55.
      - Real completion also RE-SPAWNS the same pickup at its original position (`ObjectStart`,
        speed=0, static) — a detail missing from `mobile-eggbert-reference/13-object-pickups.md`,
        now implemented (`GEInteractionSystem::RespawnPickupItem()`).
      - **Action-button gate — done as its own follow-up, 2026-07-14 (later the same day,
        user-requested)**: real Sucette/Drink require the action button held at contact
        (`getButtonPressedProperty()==PlayAction && setButtonPressedProperty(None)`,
        Decor.cpp:6025/6053) — this engine previously granted both automatically on contact alone.
        Fixed via a new `blupiActionPressedEdge` parameter on `GEInteractionSystem::Update()`
        (edge-detected, same idiom as this engine's own existing switch-activation check —
        real source's own `setButtonPressedProperty(None)` "consume the press" is this engine's
        existing per-frame edge-detect, not separately modeled since nothing else in this engine
        currently double-consumes the same press). Charge(31) deliberately has NO button gate
        (confirmed: `Decor.cpp:6069-6087` has no `getButtonPressedProperty()` check at all) —
        real, genuine asymmetry, not an oversight. New tests confirm both the positive case (button
        held → grants) and the negative case (no button → nothing happens) for Sucette/Drink, plus
        an explicit "Charge needs no button" confirmation.
      - Verified: new `GEBlupiController`-level tests (all 3 durations including idempotent
        no-op/cancellation-by-death-lock) in `VerifyBlupiMovement`, new `GEInteractionSystem`-level
        tests (contact position capture, `RespawnPickupItem()`, the action-button gate) in
        `VerifyInteractionSystem`. Full
        suite: 78 tests on `build-cna` (99%, only the pre-existing unrelated
        `easy-gl-resource-smoke-tests` failure), 73/73 (100%) on `build-cna-vulkan`.
- [x] `174` Charge/Cloud(31) — gated against ALL other buffs including itself (loosest-guard
      opposite is Mirror/Invert(40), gated only against Hide) — done 2026-07-12 alongside `170`
      (`GEBlupiController::TriggerCloud()`'s gate: `== None`, the strictest of the 4, matching the
      real `Decor.cpp` condition exactly). Mirror/Invert(40) itself is a separate, NOT-modeled
      effect (`m_blupiInvert`, not one of the 4 `SecretPower` values) — out of scope for `170`.
- [x] `175` Bullet pack(29) — done 2026-07-12, verified against
      `mobile-eggbert-reference/13-object-pickups.md`'s "Bullet pack" section (`Decor.cpp`
      ~5731-5744). New `GEInteractionSystem::BulletCount()` + `kBulletCap=10`, same shape as
      `dynamiteCount_`: automatic on contact (no button), gated on `bulletCount_ < kBulletCap` —
      touching a pack already at the cap is a genuine no-op (object stays active, count
      unchanged), matching the real source exactly rather than a running `+=10` total (the real
      `+= 10` then clamp has the same net effect here since the gate already guarantees the prior
      count was below the cap, so a plain `= kBulletCap` assignment is equivalent and simpler).
      Channel 54 fanfare on pickup. One demo pack added to the sample world (auto-pickup, unlike
      the Jeep demo next door which needs the action button). Verified: 6 new
      `VerifyInteractionSystem` assertions (starts at 0, tops up to 10, pickup deactivates,
      cap-gated no-op via a synthetic second pack) + full suite (63/63 unit tests, all verify
      tools) + both backends. The actual firing mechanic itself is `BULLET-001` below (done
      2026-07-13).

- [x] `BULLET-001` Tank firing (`ObjectType23` projectile) — **done 2026-07-13**, closing `175`'s
      own deferred gap, per explicit user request ("implementovat střelbu"). Preceded by a
      dedicated research pass into `Decor.cpp` (real firing code, not guessed) that overturned an
      initial assumption: real bullets are **NEVER a weapon against enemies** — an exhaustive
      search of all ~25 real `ObjectType23` references found no code anywhere where a bullet
      damages or destroys an enemy `MoveObject`. The ONLY real collision consequence is
      bullet-vs-Blupi (already implemented in this engine from an earlier session,
      `GEInteractionSystem.cpp`'s existing `ObjectType23` contact-kill block, `E3D-MIG-134`) — real
      bullets are a hazard, identical whether enemy-fired (blupih/blupit, already modeled) or
      player-fired (this task). Real trigger: a **dedicated `Fire` key** (`KeyPressFlags::Fire`),
      NOT the Action button used for dynamite/Perso/switches/vehicle mount — this engine's own "F"
      keyboard pick (real source is touch/gamepad-only, no keyboard binding to match). Only usable
      while riding **Tank** (`Decor.cpp:4308-4344`) — Helicopter's own real firing branch
      (`HelicoGlu`) was NOT independently confirmed to actually spawn a projectile during research
      (only that channel 52 covers both vehicle paths), so it is deliberately NOT modeled, a
      documented gap rather than a guess. Real gates: a 0.5s cooldown
      (`Config::ScaleTime(10)`@20fps, ticked down every frame regardless of input, reset only on an
      actual shot) and `bulletCount_ > 0` (checked AFTER the cooldown, BEFORE the raycast — an
      empty-cooldown Fire press with 0 ammo plays the real out-of-ammo click, channel 53, without
      starting the cooldown, matching the real gate order exactly). A real shot consumes exactly 1
      bullet and reuses the SAME `MakeBullet()`/`SearchAirDistance()` helpers blupih/blupit's own
      fired shots already used (horizontal raycast to the next solid cell along Blupi's own
      facing) — no new projectile-physics code needed, just a new spawn trigger wired into the
      existing mechanism. One Tank pickup + a wall a few cells away added to the sample world
      (`tools/GenerateSampleWorld3D.cpp`) right next to the existing bullet-pack demo, so the full
      pickup → mount → fire → hit-wall loop is genuinely playable, not just unit-tested. Verified:
      6 new `VerifyInteractionSystem` assertions (canFire=false gate, ammo consumption, cooldown
      gating both directions, no-underflow-at-0-ammo) — caught and fixed a real test-design bug
      while writing them (a leftover synthetic bullet pack from an earlier `175` test sat at the
      exact same position and silently re-topped ammo every frame, initially producing 2
      unrelated-looking test failures including a cascaded failure in a much later, otherwise
      untouched test) — plus a live two-stage headless verification: forced Tank-mount + fire in
      the running game (not just the isolated test), confirming via direct object-list inspection
      that a real in-flight `ObjectType23` spawned exactly at Blupi's fire position and traveled
      toward the placed wall, AND that `BulletCount()` dropped by exactly 1 despite holding Fire
      held for 6 consecutive frames (proving the cooldown gates correctly in the live game loop,
      not just the isolated test). Full regression suite green (64/64 unit tests, all verify
      tools, both backends).
- [x] `176` Pickup sparkle-fx(39) — cosmetic only, spawned by treasure/key pickups — **already
      done, stale duplicate closed 2026-07-17**: this is the exact same mechanic as `VISUAL-012`
      (done 2026-07-14). Confirmed directly in `GEInteractionSystem.cpp`: `AppendSparkleBurst()`
      is called for `ObjectType5` (treasure, line 1824) and all 3 keys (`ObjectType49/50/51`,
      lines 1846/1853/1860), matching real `Decor.cpp:5948-6006` exactly (treasure and all 3 keys
      fire the same 4-direction `ObjectType39` burst). No further work needed.
- [ ] `177` Ecrase/pancake collision-box mode and Suspended (hanging, no accel ramp) movement
      mode — the two collision/movement modes not covered by `E3D-MIG-171`'s vehicle list.
      **Researched 2026-07-12, split into two distinct outcomes, neither implemented this
      session:**
      - **Ecrase collision-box mode: documented non-goal, not a gap to close.** The real source
        uses a genuine AABB collision system where squashing shrinks Blupi's hitbox to a distinct
        50×19 pancake (vs. normal 36×47), letting him fit under low gaps. `GalaxyEggbertCNA`'s
        collision model is single-point (`GroundHeightAt`'s column scan) — there is no hitbox to
        shrink, so this isn't a partial-fidelity gap to close incrementally; it's architecturally
        non-portable without a full AABB collision rewrite, a large, unrelated undertaking well
        outside this task's scope (same category of change already declined for the teleporter
        collision mismatch, `E3D-MIG-147`).
      - **Suspended (hanging on a bar) mode: real, fully-speced, cheap mechanic — but blocked on a
        pending render-geometry decision, not further gameplay research.** Verified directly
        against `Decor.cpp:4732-4790`/`GetTypeBarre` (~7158): grabbing triggers by standing under
        tile icon 138 or 202 (probe point `pos+(30,22)`), horizontal move is direct `speedX*5` (no
        accel ramp, matching this task's own name), jump is a 10-tick wind-up then a fixed
        `vitesseY=-11` launch + 5-tick no-regrab grace timer, turn takes 10 ticks — every constant
        is known and portable. `GEBlupiController.hpp` already carries the `BlupiAction` enum
        values (`StopSuspend`/`MarchSuspend`/`TurnSuspend`/`JumpSuspend`, from an earlier blanket
        transcription) but zero gameplay logic — genuinely unstarted. The blocker: icon 202
        ("thin-bar") is currently flagged in `mobile-eggbert-reference/02-tiles.md` as a NEW,
        not-yet-implemented render geometry ("thin rectangular-prism bar," not billboard/
        DirectionalCube/ThinMechanical), and in the real source the SAME tile icon serves as both
        decor AND the grab trigger — there's no separate interactable tile to key off instead.
        Implementing the mechanic now would force exactly the kind of new visual/render-geometry
        call the user explicitly asked to skip this session (`needs_human`, same category as the
        Saw blade). **Recommended next step once a human makes that render-geometry call**:
        implement Suspended mode using the constants above — it's cheap once unblocked.
- [x] `178` Vehicle/mode-specific movement table (max speed/accel/vertical behavior per mode) —
      done 2026-07-12 as part of `171` itself (`kJeepMaxSpeed`/`kJeepDecel`, `kTankMaxSpeed`/
      `kTankDecel`, `kOvercraftMaxSpeed`/`kOvercraftDecel`, `kSkateboardMaxSpeed`/
      `kSkateboardDecel`, `kHelicopterMaxSpeed`/`kHelicopterDecel` + its own
      `kHelicopterAscendSpeed`/`kHelicopterDescendSpeed`, `kOvercraftAscendSpeed`/
      `kOvercraftDescendSpeed`, shared `kVehicleAccel`/`kVehicleVerticalAccel` — see `171`'s own
      entry for the full proportional-scaling rationale). No separate work needed; this task's
      scope and `171`'s implementation were the same table, not two deliverables.
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

**This is the current, actively-maintained status tracker (see `## 1`'s own orientation note for
the full relationship) — its own `BLUPI-xxx`/`PICKUP-xxx`/etc. IDs are a separate space from `## 1`'s
`E3D-MIG-xxx` phase IDs, not aliases; when `## 1`'s narrative and this checklist disagree on
whether something is done, trust this section.**

### 2.1 Engine & Build

CNA is the sole long-term target (see CLAUDE.md "Current Direction Lock"). Its CMake wiring is
already done and working: `-DGALAXY_EGGBERT_BUILD_CNA=ON` (default `ON`), target `GalaxyEggbertCNA`,
builds cleanly under `build-cna` and `build-cna-vulkan`. The tasks below are re-scoped to CNA;
anything that was specific to the dead Simple3D/U3D/Nova3D/Android direction is dropped.

- [x] BUILD-001 — CMake target `GalaxyEggbertCNA` builds on Linux (CNA backend) (CNA, 2026-07-10)
- [x] BUILD-002 — `GALAXY_EGGBERT_BUILD_CNA` option wired and defaults to `ON` (CNA, 2026-07-10)
- [x] BUILD-003 — Web build (Emscripten / WebAssembly) for `GalaxyEggbertCNA` — **done 2026-07-17**
      (user request: a web prototype build to publish on their own site). The CNA/easy-gl/meta-gl/
      SDL3 stack turned out to already be Emscripten-ready end to end (vendored SDL3 has a
      `.sdl-prebuilt-emscripten` install path, `EasyGLGraphicsBackend` already requests
      `SDL_GL_CONTEXT_PROFILE_ES` unconditionally — maps directly to WebGL2 under Emscripten, no
      patch needed — and `Game.cpp` already had an `emscripten_set_main_loop` path) — this task was
      almost entirely CMake wiring + asset-size triage, not engine work. Changes: `CNA_GRAPHICS_BACKEND`
      now defaults to `EASYGL` under `EMSCRIPTEN` (Vulkan has no web bridge; the existing Vulkan
      default is untouched for every other platform); new `if(EMSCRIPTEN)` block on the
      `GalaxyEggbertCNA` target — `SUFFIX .html`, `-sMIN/MAX_WEBGL_VERSION=2 -sFULL_ES3=1`,
      `-sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=256MB`, a new `cmake/web/shell-cna.html` (adapted
      from the old Simple3D-era `cmake/web/shell.html`, U3D-specific `SetRendererSize` hook
      removed, rebranded), and `--preload-file` entries baking every asset directory into the
      `.wasm`'s virtual filesystem at link time (there's no "copy next to the binary" on the web).
      **Asset-size triage**: preloading mobile-eggbert's full `Content/` (~460MB) would make an
      unreasonable web download — direct grep confirmed `GalaxyEggbertCNA` only ever reads
      `Content/icons/`, `Content/backgrounds/`, `Content/sounds/` (never `icons4x/`/`backgrounds4x/`,
      mobile-eggbert's own 4x-scale variants, ~437MB combined) — so only those 3 subdirectories are
      preloaded, plus `worlds3d/`/`textures3d/`/`avatars3d/`, for a ~24MB total `.data` package.
      **Save persistence**: `GESaveData::kSavePath` is now `#if defined(__EMSCRIPTEN__)` (a
      platform-path difference, not an engine-API one, so it doesn't fall under CLAUDE.md's
      "no `#ifdef` for engine differences" rule, which is scoped to Simple3D-vs-CNA) —
      `/save/savedata.txt` under Emscripten, unchanged `savedata.txt` natively — paired with a new
      `cmake/web/pre.js` (`--pre-js`) that mounts IDBFS at `/save` and syncs it on load/unload, so
      progress survives a page reload via IndexedDB (`-lidbfs.js` linked in). `CNA_ENABLE_NET` must
      stay `ON` for the web build too (tried `OFF` first — `GalaxyEggbertCnaGame.cpp` unconditionally
      references `AvatarRenderer`/`SkinnedModelEXT` symbols from `CNA_GamerServices` regardless of
      this flag, confirmed by an actual `wasm-ld` link failure); ENet itself builds fine under
      Emscripten (only its symbols are needed here — Galaxy Eggbert never actually uses networking).
      Verified via a real headless-Chrome run against the built `.html`/`.wasm`/`.data` (served over
      local HTTP, `--use-angle=swiftshader` software WebGL, driven over the DevTools protocol so the
      real async load could be awaited rather than screenshotted mid-download): console output
      confirmed a real WebGL2 context (`OpenGL ES 3.0 (WebGL 2.0 (OpenGL ES 3.0 Chromium))`),
      `world001.vwr` loading, terrain/background/sound assets loading (93/93 sound channels), a
      terrain mesh uploading, and a screenshot showing the real Init/gamer-select screen (Speedy
      Blupi branding art, Player A/B/C gate/lives stats) rendering correctly. Native `build-cna`
      rebuilt and full `ctest` re-run after these changes (only the pre-existing unrelated
      `easy-gl-resource-smoke-tests` failure) to confirm nothing native regressed. Known gap: no
      CI/hosting wiring yet (that's `BUILD-009`/`BUILD-010`, still open) — this task is just "the
      build exists and demonstrably runs in a browser," publishing the artifact is a separate step
      for the user to do on their own site.
      **Real bug found + fixed same day (black-screen-on-first-run)**: the above verification had
      bypassed `cmake/web/shell-cna.html`'s own "Click or tap to begin." gate by patching
      `Module.noInitialRun` to `false` directly in a throwaway test copy — masking a genuine bug in
      the shell's normal, unmodified flow. The user tried the real, unpatched build and got exactly
      what a faithful re-test then reproduced: a near-black screen forever, click or no click. Root
      cause, found by reading the actual compiled `GalaxyEggbertCNA.js` glue: modern Emscripten's
      `run()` reads `Module["noInitialRun"]` into a local variable ONCE, synchronously, before any
      click can possibly happen, then never re-checks the `Module` property again — so the shell's
      old (U3D-era) recovery pattern of "flip `Module.noInitialRun = false` inside the click
      handler if `run()` hasn't reached that point yet" is dead code against this toolchain version;
      clicking never actually invoked `callMain()`. Canvas still went visible (a completely separate,
      unconditional `setStatus("Running...")` call inside `run()` fires regardless of
      `noInitialRun`), which is exactly why it looked like "the page loaded fine, just nothing ever
      draws" rather than an obvious error. Fix: removed the whole click-to-begin gate from
      `cmake/web/shell-cna.html` (dead/broken code, not a working feature to preserve) and set
      `noInitialRun: false` so the game starts immediately on page load — re-verified with a FRESH,
      completely unpatched Chrome run (no click simulated, no test-only JS patches) showing the real
      Init/gamer-select screen rendering immediately. Also fixed a related CMake gap while here:
      `--shell-file`/`--pre-js` are just linker-flag strings to CMake, not tracked dependencies, so
      editing either file alone (no source touched) left `cmake --build` believing the target was
      already up to date and it kept serving the stale old `.html`/`.js` — added
      `LINK_DEPENDS` on both files to `GalaxyEggbertCNA` so future edits to either always trigger a
      real relink. Lesson: a smoke test that patches around the exact interaction path a real user
      takes doesn't verify that path — the original verification should have used the literal
      shipped `.html` unmodified, including its click-gate, not a hand-patched copy.
- [ ] BUILD-005 — Windows cross-compile (MinGW-w64) for `GalaxyEggbertCNA` — not attempted yet
- [x] BUILD-007 — `GalaxyEggbertWorldsTests` unit tests build and all pass — **confirmed 2026-07-14**, current count is 64/64 (see TEST-001 in §13).
- [ ] BUILD-008 — `ctest --test-dir <build-dir>` discovers and runs the world tests
- [ ] BUILD-009 — CI: automated build on push (GitHub Actions), CNA target only (Linux; Web once BUILD-003 exists)
- [ ] BUILD-010 — Package installer / distributable (Linux AppImage or .tar.gz with bundled assets) for `GalaxyEggbertCNA`

Dropped (dead Simple3D/U3D/Nova3D/Android direction, do not carry forward): old BUILD-001..002 as
originally scoped to `GalaxyEggbertSimple3D`/U3D, old BUILD-004 (Android via U3D/Nova3D), old
BUILD-006 (Nova3D backend switch).

---

### 2.2 Menu & Screens (PRIORITY)

Menu screens use the same PNG backgrounds as mobile-eggbert (`Content/backgrounds/*.png`,
`Content/icons/*.png`). **Stale intro note removed 2026-07-14**: this paragraph used to say CNA
"has not started on HUD/menu work" — no longer true as of the extensive 2026-07-13 menu/HUD
session below (each subsection already carries its own accurate, per-item correction dates; this
was just a leftover boilerplate reset note above them, not a sign the items themselves are wrong).

#### 2.1 Phase: First / Wait (loading screen)

Corrected 2026-07-13 against real source (a dedicated research pass into `Game1.cpp`'s real
`Phase::First -> Wait` transition) — several of the descriptions below were imprecise in the
original draft; see each item for what the research confirmed.

- [x] MENU-001 — Render `wait.png` as full-screen image during boot loading phase — **done**,
      `GEInputPad::DrawWait()`. Confirmed exact 640×480, direct pixel match.
- [x] MENU-002 — Display animated loading gauge (`jauge.png`, yellow fill) at bottom-centre, same
      position as mobile-eggbert (196, 426 in 640×480 space) — **done**, real position/zoom (2.0)
      confirmed via research and ported exactly; real sprite sheet is 124×88 (4 rows of 22px: row 0
      = empty-gauge background, rows 1-3 = Red/Blue/Yellow fill).
- [x] MENU-003 — Gauge fills from 0→100% as resources load (replicate `DrawWaitProgress` logic) —
      **done, but ADAPTED**: research found the real `waitProgress` is a **fixed 5.0s wall-clock
      cosmetic timer** (`ticks/50,000,000`), completely decoupled from actual asset loading — this
      engine already loads everything synchronously in `LoadContent()` (matching the real source's
      own synchronous `First` step), so the gauge fill here is real-formula-faithful (the exact
      non-linear `waitTable` lookup curve, 12 threshold/level pairs, ported verbatim) but purely
      cosmetic, same as the real one.
- [x] MENU-004 — Transition from Wait → Init after loading completes (≥1 s minimum) — **done, but
      CORRECTED**: real minimum is not "≥1s" as the draft said, it's the fixed 5.0s timer above
      (confirmed via research: `if (waitProgress > 1.0) SetPhase(Init)`).
- [x] MENU-005 — Hide wait gauge if resuming a saved game (ContinueMission path) — **done via
      ADAPTED trigger**: real `ContinueMission`/`Decor::CurrentRead()` is a WP7-only OS-reactivation
      snapshot mechanism with no desktop equivalent (confirmed via research); this engine instead
      checks `GESaveData::GetHasProgress()` at the end of the same 5.0s timer and goes straight to
      `Resume` instead of `Init` when true — same adapted trigger already established for Resume
      itself (plan.md MENU-040..045), not a separate new mechanism.

#### 2.2 Phase: Init (main menu / gamer select)

Corrected 2026-07-13 against real source (same research pass) — the draft's MENU-007/017/018
animation descriptions were WRONG on direction/effect; see each item below.

- [x] MENU-006 — Render `init.png` as full-screen background — **done**, `GEInputPad::DrawInit()`.
      Confirmed exact 640×480. **Pillarbox color fixed 2026-07-18** (user-reported: the blue margin
      strips on the sides of the main menu should be dark blue, not the visibly lighter/different
      shade they were): `GalaxyEggbertCnaGame::Draw()`'s `device.Clear(...)` call — the color
      visible in the pillarbox margins around `init.png`'s own scaled-to-fit content rect during
      Wait/Init (per that call site's own pre-existing comment) — was still the generic default
      XNA/MonoGame "CornflowerBlue" template color (0.392, 0.584, 0.929), never customized. Sampled
      `Content/backgrounds/init.png`'s own corner pixel directly (RGB(0,35,98), a dark navy) and
      changed the clear color to match exactly, eliminating the visible seam between the pillarbox
      and the actual background art. Also investigated a separately user-reported "different shade
      outline" around the SPEEDY BLUPI logo/Blupi image (suspected possible CNA bug) — direct pixel
      inspection of the source PNG's semi-transparent edge pixels (dark gray, ~20-30 RGB, not any
      off color) plus a manual alpha-blend calculation over the (now-corrected) dark navy background
      reproduces the exact "lighter blue ring" visual exactly — this is the mathematically correct,
      expected result of alpha-blending this real asset's own soft anti-aliased edge over a blue
      backdrop, not a rendering bug (confirmed NOT a `cna` defect).
- [x] MENU-007 — Render `speedyblupi.png` (title logo) — **done, but draft was WRONG on
      direction**: real entry is a **vertical slide DOWN from above the screen** (`num=1-(1-t)^2`
      ease-out over 1.0s, Left/Right FIXED at 80/720 the whole time), not "sliding in from top" in
      the sense of a horizontal motion — confirmed directly against the real formula, which also
      contradicts the real source's OWN doc-comment (`Game1.hpp`) claiming a rightward slide; the
      literal code, not the doc-comment, was treated as ground truth.
- [x] MENU-008 — Render `blupiyoupie.png` — **done**: real scale-in 50%→100% + fade-in 0.25→1.0
      opacity over the same 1.0s, centered at real (468,280) — confirmed NO rotation/spin despite an
      earlier doc-comment claiming one.
- [x] MENU-009 — Three gamer-slot buttons (A/B/C) — **done**: real `pad.png` icons 4/16 (A),
      5/17 (B), 6/18 (C) unselected/selected, real positions confirmed to need NO proportional
      adaptation at this engine's own 480 reference height (same rare "literal port" situation as
      PlaySetup's own row).
- [x] MENU-010 — Each gamer slot shows real per-slot text — **done, but CORRECTED**: real label is
      "Player {0}" (confirmed via `MyResource`'s real EN string table), NOT "Gamer {0}" as the draft
      said. Door-count lines ("Main gates : {n}/12" / "Secondary gates : {n}/52") are rendered with
      the real STRING verbatim but a STATIC "0" — per explicit user decision, since this engine has
      no per-gamer 200-door-flags array (a single hand-authored .vwr world, not the real
      100+-level/3-gamer-slot structure) — only the title/lives lines reflect real per-slot state
      (`GESaveData`'s own 3-independent-gamer-slot extension, plan.md MENU-019/020 below).
- [x] MENU-011 — "PLAY" button (`InitPlay` glyph) — **done**: real position needs no adaptation
      either. Label rendering deferred (see this section's own closing note).
- [x] MENU-012 — "SETUP" button (`InitSetup` glyph) — **done**, same icon as `PauseSetup` (19).
- [ ] MENU-013 — "RANKING" button (`InitRanking` glyph) — **NOT ported**: research found the real
      visibility gate (`getIsTrialModeProperty()`/`getIsRankingModeProperty()`) resolves to "never
      shown by default" in this port (both hardcoded false / QA-cheat-only) — same "unreachable in
      this port" precedent already established for the Trial phase itself, not a missing feature.
- [ ] MENU-014 / MENU-015 — Semi-transparent panels behind gamer slots / action buttons — **NOT
      ported**: pure cosmetic background decoration, no functional value, a documented gap (Init's
      own panels are distinct from MainSetup/PlaySetup's rotating gear.png decorations, MENU-059/060,
      which ARE now ported).
- [x] MENU-016 — Keyboard Back/Escape → exit game (from Init phase) — **done, but ADAPTED**:
      research found the real source's OWN Escape key unconditionally maps to `Pause` regardless of
      phase (including from Init) — flagged by that research as a likely-UNINTENDED quirk of the
      real source rather than deliberate design. NOT replicated: this engine's Escape from Init
      instead reuses the real hardware-Back-button behavior (`Exit()`), which reads as the clearly
      intentional one.
- [x] MENU-017 / MENU-018 — Animated fade-out transitions (Init→Play, Init→MainSetup) —
      **done 2026-07-13 (plan.md MENU-088/089 pass), and draft was WRONG on both effects**: real
      Init→Play is speedyblupi sliding back UP off-screen at 2x speed (not "out") while blupiyoupie
      scales UP to 11× native size while linearly fading (not "zooms out"); Init→MainSetup is
      speedyblupi sliding RIGHT off-screen while fading (draft was directionally correct there)
      while blupiyoupie stays FIXED size and just fades (not "zooms out" either, no gear.png
      appears — gear.png is MainSetup/PlaySetup's OWN decoration, not Init's). Both verified live via
      headless screenshots mid-fade.
- [x] MENU-019 — Gamer selection persisted — **done**: `GESaveData` extended (2026-07-13) with a
      real `selectedGamer` field (matches real `data[2]`) + 3 independent `GamerSlot`s
      (lives/missionNumber/hasProgress each) — see `GESaveData.hpp`'s own Phase-3 comment. A single
      tap on a gamer slot immediately selects AND persists it, matching real `Game1::SetGamer()`
      (confirmed via research this does NOT also enter Play — a separate InitPlay tap is required).
- [x] MENU-020 — Gamer slot info read from save data — **done** for lives (real per-slot data);
      `lastWorld`/doors are NOT read (no equivalent exists here, see MENU-010's own note above).

Real `SetupReset` ("Erase progress", MainSetup-only) is now also wired for real (2026-07-13): confirmed
via research this is the SAME full `gameData.Reset()` as Cheat5, not a per-gamer-only reset, despite
its own real label implying otherwise — maps directly onto `GESaveData::Reset()`. InitPlay/InitSetup
button TEXT labels ("Play"/"Setup") are not yet rendered (deferred, same low-priority-polish status as
a few other unlabeled buttons already noted elsewhere in this document, e.g. WinLostReturn/SetupReturn).

#### 2.3 Phase: Play (active gameplay)

- [x] MENU-021 — Hide all menu UI elements during Play phase — **done 2026-07-13** now that
      Init/Wait actually exist (`phaseHasRealScreen` gate in `GalaxyEggbertCnaGame::Draw()` already
      covers Wait/Init/MainSetup alongside Pause/Win/Lost/PlaySetup/Resume).
- [x] MENU-022 — "PAUSE" button (`PlayPause` glyph) visible during Play — **done 2026-07-13** (`GEInputPad::DrawPlay`/`UpdatePlay`) — real icon 3, top-right, real edge/release-triggered press semantics; toggles Play→Pause exactly like the pre-existing Escape key (OR'd, see MENU-027)
- [x] MENU-023 — On-screen directional pad (`pad.png` icons 0, 1) for touch/gamepad emulation — **done 2026-07-13** — real discrete {-1,0,+1}-per-axis drag (20px reference-space threshold), current-drag-point tracking (not a fixed grab offset), proportionally-adapted position/size (real drawBounds-relative coordinates don't fit this engine's fixed 640×480 reference space at all — see `GEInputPad.hpp`'s class comment)
- [x] MENU-024 — On-screen "JUMP" button (`PlayJump`) visible during Play — **done 2026-07-13** — real LEVEL-triggered semantics (fires every frame the pointer is inside the rect while held, no release needed), OR'd with the existing keyboard jump (LCtrl)
- [x] MENU-025 — On-screen "ACTION" button (`PlayAction`) visible during Play — **done 2026-07-13** — real EDGE/release-triggered semantics (single-fire on release, regardless of release position, as long as the press started on the button), OR'd with the existing keyboard action (Space)
- [ ] MENU-026 — On-screen "DOWN" button (`PlayDown`) visible during Play (when applicable) — **not implemented** (2026-07-13 session scoped to D-pad/Jump/Action/Pause only, per explicit user request — `PlayDown` is real, icon 23, same level-triggered semantics as `PlayJump`, but has no engine-side crouch-toggle use case identified yet; a real, deliberately-deferred gap, not an oversight)
- [x] MENU-027 — Keyboard: Back/Escape during Play → Pause phase — **done** (this engine's own keyboard binding — the real source has no keyboard binding at all here, gamepad-Back/touch-only, an XNA/WP7 port; added alongside the `Def::Phase` state machine, HUD-023 2026-07-13, now also OR'd with the on-screen `PlayPause` button, MENU-022 above)

#### 2.4 Phase: Pause

- [x] MENU-028 — Render `pause.png` as full-screen background — **done 2026-07-13** (`GEInputPad::DrawPause`) — confirmed exact 640×480, a direct pixel match for the existing reference space, no cropping/UV math needed
- [~] MENU-029 — Render `blupiyoupie.png` scaling/rotating in (same animation as Init but centred at 418,190) — **art+position done 2026-07-13** (confirmed 410×380, centered at real position (418,190)); **static/un-animated** — the real scale/rotate-in intro animation is a documented simplification, not implemented
- [~] MENU-030 — "MENU" button (`PauseMenu`) with label below — **real position/icon + real "Home" text label done 2026-07-13** (icon 11, unconditional; label added in the same pass as MENU-046..057, verified against `Game1::DrawButtonsText()`'s real `DrawTextUnderButton(PauseMenu, TX_BUTTON_MENU)` call — note the real EN string is "Home", not "Menu"); **intentionally inert** — no destination screen (main menu) exists yet, a documented gap not a silent omission
- [x] MENU-031 — "BACK" button (`PauseBack`) — shown only when mission ≠ 1 — **real position/icon + real conditional visibility + real "Back" text label done 2026-07-13**; functionally inert like MENU-030 (no hub-navigation screen exists yet, see MENU-035)
- [~] MENU-032 — "SETUP" button (`PauseSetup`) with label below — **real position/icon + real "Setup" text label done 2026-07-13** (icon 19, unconditional); **intentionally inert** — no settings screen exists yet
- [x] MENU-033 — "RESTART" button (`PauseRestart`) — shown only when mission ≠ 1 AND mission % 10 ≠ 0 — **done 2026-07-13**, the real conditional visibility, real "Restart" text label, AND a functional (simplified) restart: resets Blupi to the origin spawn + resumes Play (see MENU-036 — not a real level reload)
- [x] MENU-034 — "CONTINUE" button (`PauseContinue`) with label below — **done 2026-07-13**, real "Continue" text label, fully functional (resumes Play in place, real edge/release-triggered press)
- [x] MENU-035 — PauseBack goes to previous hub world (MissionBack logic: if
      mission%10==0 → Init, else mission/10*10) — **done 2026-07-17**, real
      formula ported exactly as `GEWorldRuntime::ComputeMissionBack()`
      (verified directly against `Game1::MissionBack()`), wired to the
      real `PauseBack` button (its own press-tracking already existed,
      `kPauseControlBack`, just was never surfaced into `GEInputPad::
      PauseInput` before — added `backPressed`). Goes to `LoadMission(...)`
      + stays in Play (real destination for the `mission%10==0` case is
      Init, not directly back into gameplay -- an already-established,
      unchanged simplification, same one `WinLostReturn` below already used
      before this session even existed).
- [x] MENU-036 — PauseRestart restarts current mission — **upgraded to a
      genuine reload 2026-07-17**: verified directly against
      `Game1.cpp:357-358` (`case PauseRestart: SetPhase(Play, mission)` —
      the SAME real mission, which DOES route through `StartMission()`'s
      full reload, confirmed by comparing against Lost's very different
      `DoorsLost()` path which does NOT reload). Now calls
      `LoadMission(worldRuntime_.GetMissionNumber())` — a real fresh level
      reload (vehicle/secret-power/key/dynamite/treasure state all reset,
      lives preserved), replacing the previous origin-respawn-only
      simplification.
- [x] MENU-037 — PauseContinue resumes play without reloading — **done 2026-07-13**
- [ ] MENU-038 — Animated fade-out from Pause → Play (blupiyoupie.png zooms out) — not implemented, transitions are instant (same simplification as MENU-029's static art)
- [ ] MENU-039 — Animated slide-out when Pause → PlaySetup (blupiyoupie.png slides right) — N/A, PlaySetup phase doesn't exist yet

**MENU-021..027/028..039 summary (2026-07-13):** implemented `GEInputPad` (`src/GalaxyEggbertCNA/Game/GEInputPad.hpp`/`.cpp`), a mouse-driven port of the real `InputPad` class covering the Play on-screen D-pad/Jump/Action/Pause controls and the full Pause screen (real `pause.png` + `blupiyoupie.png` art, 5 real `pad.png` buttons with real conditional visibility, Continue/Restart functionally wired). Verified via a dedicated scripted tool (`tools/VerifyGEInputPad.cpp`, 24 checks, synthetic `MouseState` values, no `GraphicsDevice` needed) covering D-pad discrete-drag thresholding, Jump's level-trigger vs Action/Pause's edge-trigger semantics, Pause-row conditional visibility, and `ResetTouchState()`'s phase-transition safety — plus live headless screenshots on both backends (EasyGL/Vulkan) confirming on-screen Play control placement and the real Pause screen layout (mission 0 correctly hides `Restart` via the real `mission%10!=0` gate). Real `drawBounds`-relative button coordinates from `InputPad.cpp` do not fit this engine's fixed 640×480 reference space at all (confirmed by computing them directly — the real Pause row alone would span off both edges) — every rect here is a proportionally-adapted layout preserving real order/relative placement/icon choices, not a literal pixel port. `PlayDown` (MENU-026) and every animated transition (MENU-029/038/039) are explicitly out of scope for this pass — see their own entries above.

#### 2.5 Phase: Resume (saved game continue prompt)

- [x] MENU-040 — Render `pause.png` background (same as Pause) — **done 2026-07-13** (`GEInputPad::DrawResume`), confirmed via `Game1.cpp`'s real `SetPhase()` background dispatch sharing the exact same `case Phase::Pause: case Phase::Resume: BackgroundCache("pause");`
- [~] MENU-041 — Render `blupiyoupie.png` with rotation spring animation — **art+position done 2026-07-13** (reuses Pause's own static character draw, same documented non-animation simplification already established for Pause)
- [~] MENU-042 — "MENU" button (`ResumeMenu`) → Init — **real position/icon/label done 2026-07-13** (icon 11, own distinct rect independently re-derived from `InputPad.cpp`'s bsf2=140 formula, NOT reused from PauseMenu's rect); **intentionally inert** — no Init/main-menu screen exists yet, same reasoning as Pause's own Menu button
- [x] MENU-043 — "CONTINUE" button (`ResumeContinue`) → ContinueMission() — **done 2026-07-13**, functional: restores the checkpointed lives count (`GEInteractionSystem::SetLives()`, new) and resets Blupi to spawn (no real mid-level position/treasure/key state exists to restore — a documented simplification matching PauseRestart/WinLostReturn's own precedent)
- [~] MENU-044 — Resume phase triggers when app reactivates with a saved mid-game state — **ADAPTED trigger implemented 2026-07-13**: the real trigger (`Game1::OnActivated()`, a WP7 app-reactivation OS lifecycle event gated on a real serialized mid-level `Decor::Current*()` snapshot — a separate, heavier mechanism than `GameData`/`GESaveData`) has no desktop equivalent and is far beyond this engine's single-`.vwr`-world scope; adapted to: offered at startup whenever `GESaveData::GetHasProgress()` is true (set at the real Win/Lost checkpoint, see MENU-046..057's own summary) — verified via a live two-process test (forced a Win checkpoint in run 1, confirmed `phase_` started as `Resume` with the correct saved lives at the start of a separate run 2)
- [x] MENU-045 — Keyboard Back during Resume → Init — **done 2026-07-13**, adapted: Escape starts a fresh Play WITHOUT restoring saved lives (distinguishing "new game" from "continue", matching the two real buttons' own distinct intent) — Init doesn't exist, this engine's own keyboard binding choice

**MENU-040..045 summary (2026-07-13):** extended `GEInputPad` with `UpdateResume()`/`DrawResume()`, reusing Pause's own background+character draw (same `pause.png`/`blupiyoupie.png`, confirmed shared in the real source) with its own distinct 2-button set (ResumeMenu icon 11 / ResumeContinue icon 10, real rects independently re-derived, NOT reused from Pause's row). Extended `GESaveData` with `lives`/`missionNumber`/`hasProgress` fields, checkpointed at the exact real Win/Lost transition points (confirmed via the same `GameData` research this session already did for MENU-067) — NOT continuous autosave. Added `GEInteractionSystem::SetLives()` to restore the checkpointed count on Continue. Verified via 2 new `VerifyGEInputPad` checks (32 total), 1 new `VerifyGESaveData` check (7 total, round-tripping all 3 new fields), a live headless screenshot of the real Resume screen, and a live two-process integration test proving the full checkpoint → restart → Resume-with-restored-lives cycle actually works end to end, not just each piece in isolation.

#### 2.6 Phase: Win

- [x] MENU-046 — Render `win.png` as full-screen background — **done 2026-07-13** (`GEInputPad::DrawWinLost`) — confirmed exact 640×480, direct pixel match, no cropping/UV math needed
- [x] MENU-047 — Render `blupiyoupie.png` with pulsating scale (sin wave animation, amplitude 1.0±0.5) — **done 2026-07-13**, verified directly against `Game1.cpp:744-754`'s real formula (`num = sin(phaseTime/ScaleTime(3))/2+1`, i.e. `sin(t/0.15s)/2+1` at the real 20fps base rate — a perpetual pulse between 0.5x/1.5x native size, no rotation), centered at real position (418,238) — a DIFFERENT Y than Pause's 190 (confirmed, not assumed)
- [x] MENU-048 — "RETURN" button (`WinLostReturn`) → Init — **done 2026-07-13** with the same real icon (3, shared with `PlayPause`) and a REAL, independently-derived rect (confirmed via `InputPad.cpp`'s own `bsf1=drawBoundsHeight/5` formula: (428.8,19.2)-(524.8,115.2) in this engine's 640×480 reference space — NOT PlayPause's smaller/more corner-flush rect, a distinct real button); real destination is `Init` (confirmed via `Game1.cpp`'s `WinLostReturn -> SetPhase(Init)`), which doesn't exist here — reuses the same return-to-Play-at-spawn simplification already established for the keyboard path (HUD-023), not a new one
- [ ] MENU-049 — Display mission elapsed time in text overlay — **searched for directly in `Game1.cpp`'s `Draw()`/`DrawButtonsText()`/`DrawButtonsBackground()` (2026-07-13) and NOT found anywhere** — treated as unconfirmed/likely-not-backed-by-found-source, not implemented (same rigor as MENU-050/051 below, extended here)
- [ ] MENU-050 — Display score in text overlay — confirmed unconfirmed/likely-fictional (see HUD-009's original finding — no real score variable found anywhere in mobile-eggbert)
- [ ] MENU-051 — Display "NEW RECORD!" text if score exceeds saved high score — same as MENU-050, depends on a score concept not confirmed to exist
- [ ] MENU-052 — Auto-advance to next level after N seconds (optional: like mobile-eggbert) — not implemented; real source has no fixed auto-timer either (explicit `WinLostReturn` input only, confirmed HUD-023)

#### 2.7 Phase: Lost (game over)

- [x] MENU-053 — Render `lost.png` as full-screen background — **done 2026-07-13**, same exact-640×480 confirmation as MENU-046
- [x] MENU-054 — Render `blupiyoupie.png` with spin animation (6× rotation, quadratic ease-in, same as mobile-eggbert) — **done 2026-07-13**, verified directly against `Game1.cpp:725-743`'s real formula: grows from nothing to native size once over a real 5s (`num = min(phaseTime/ScaleTime(100),1)` = `min(t/5s,1)`), with a decaying spin (`rotation = (1-num)^2 * 360*6` degrees while num<1, converging to exactly 0° as it reaches full size) — centered at the same real (418,238) as Win. Verified live via headless screenshots at phaseTime=1.0s (small + visibly rotated, matching the formula's predicted ~302° at that instant) and phaseTime=5.5s (full native size, upright, no rotation)
- [x] MENU-055 — "RETURN" button (`WinLostReturn`) → Init — **done 2026-07-13**, shares the exact same button/rect/simplification as MENU-048
- [ ] MENU-056 — Display lives remaining and score — **searched for directly in `Game1.cpp` (2026-07-13) and NOT found** — no lives/score text draw call exists for `Phase::Lost` in the real source; not implemented (see MENU-049's note)
- [ ] MENU-057 — If 0 lives: "GAME OVER" text; if lives remain: "TRY AGAIN" hint — **not implemented, and the "TRY AGAIN" branch is likely inapplicable to this engine's own Lost trigger**: this engine's real Lost gate (`GEInteractionSystem::GameOverCount()` incrementing) fires ONLY when lives are exhausted (already confirmed real behavior, HUD-023) and immediately resets lives to 3 in the same event (real `DoorsLost()` behavior) — so by the time Lost is ever reached here, "lives remain" is always true in the post-reset sense, but the phase itself only ever represents the true-game-over case. No text draw call for either branch was found in `Game1.cpp` regardless (see MENU-049's note)

**MENU-046..057 summary (2026-07-13):** extended `GEInputPad` with `UpdateWinLost()`/`DrawWinLost()` (real win.png/lost.png backgrounds + blupiyoupie.png animations + the shared `WinLostReturn` button), and — while researching the real Win/Lost draw code in `Game1.cpp` — also found and fixed a real gap in the already-shipped Pause screen: `Game1::DrawButtonsText()`'s real `DrawTextUnderButton()` calls for `Phase::Pause` (real EN strings "Home"/"Back"/"Setup"/"Restart"/"Continue" — note `PauseMenu`'s real text is "Home", not "Menu") were not yet ported; added via a new `GEInputPad::AppendCenteredLabel()` helper (own `text.png` instance, same glyph-is-ASCII-code convention as `GEHud`). Also added `GalaxyEggbertCnaGame::phaseTimeSeconds_`, a real `phaseTime` port (verified against `Game1.hpp`'s own "phaseTime==0 is a SetPhase() postcondition" doc comment and `Game1.cpp:237`'s unconditional per-tick increment) — expressed as elapsed seconds rather than a raw frame counter since the real formulas are all `phaseTime/Config::ScaleTime(N)` at a real 20fps base rate, making a seconds-based port exactly equivalent and framerate-independent. Verified via 3 new `VerifyGEInputPad` checks (27 total) plus live headless screenshots at 3 distinct animation timepoints (Pause labels, Win mid-pulse, Lost at phaseTime 1.0s and 5.5s) confirming the exact real formulas. MENU-049/050/051/056/057 (mission time/score/lives-remaining text) were searched for directly in `Game1.cpp` and found to not exist anywhere in the real Win/Lost draw code — treated as unconfirmed rather than invented, matching the established MENU-050/051 precedent.

#### 2.8 Phase: MainSetup / PlaySetup (settings)

- [x] MENU-058 — Render `setup.png` as full-screen background — **done 2026-07-13** (`GEInputPad::DrawSetup`) — confirmed exact 640×480, direct pixel match; confirmed via `Pixmap::BackgroundCache("setup")` + `DrawBackground()` (a genuine per-phase full-screen backdrop selection, distinct from the animated foreground decoration researched for MENU-059/060 below)
- [x] MENU-059 — Render `speedyblupi.png` sliding in (ease-out quadratic) — **done 2026-07-13**
      (plan.md MENU-088/089 pass), `GEInputPad::ComputeSetupFadeAnim()`/`DrawSetup()`. **Draft was
      WRONG on direction**: real is a slide in from the RIGHT (`Left=720-640*num, Right=1360-640*num`),
      not "from left" — Top/Bottom fixed at native 0/160, no vertical component.
- [x] MENU-060 — Render two rotating `gear.png` icons — **done 2026-07-13**, same pass. Draft's own
      formula citation was already correct (`rotation1=-num2*250°`, `rotation2=+num2*125°`
      counter-rotating at half rate, opacity `0.5-num*0.4` entering) and is ported verbatim,
      including the real perpetual slow rotation once settled/idle (`num2` keeps growing past the
      first 1.0s, 400 frames = 20s per unit) and the two real FIXED rects — confirmed via research
      to be a genuine intentional size asymmetry (gear2 is literally 2x native 226×226 size, not a
      mistake). Real exit reuses the identical formula with `num`/`num2` both inverted (confirmed
      via research), including gear opacity ramping the OPPOSITE direction on exit (`0.1→0.5`) —
      flagged by research as visually odd but genuinely real, not a bug, so reproduced faithfully.
- [x] MENU-061 — "SOUNDS" toggle button (`SetupSounds`) — shows ON/OFF state — **done 2026-07-13**, fully functional: real icon SWAP (13 on/21 off, confirmed via `Pixmap.cpp`'s `selected ? 13 : 21` — a DIFFERENT "pressed" convention from every other button in this class, which only ever change opacity), wired to the pre-existing `GESound::SetEnabled()`/`IsEnabled()` — a genuinely meaningful desktop equivalent of the real mute toggle, now persisted across restarts too (see MENU-067)
- [~] MENU-062 — "JUMP" mode toggle (`SetupJump`) — left/right jump direction — **real position/icon/label done 2026-07-13**; **intentionally inert** — no meaningful desktop equivalent (this is about touch-button screen-side preference)
- [~] MENU-063 — "ZOOM" toggle (`SetupZoom`) — auto-zoom on/off — **real position/icon/label done 2026-07-13**; **intentionally inert** — no auto-zoom camera concept exists in this engine yet
- [~] MENU-064 — "ACCEL" toggle (`SetupAccel`) — accelerometer on/off — **real position/icon/label done 2026-07-13**; **intentionally inert** — no meaningful desktop equivalent (accelerometer-tilt controls)
- [x] MENU-065 — "RESET Gamer X" button (`SetupReset`) — with gamer letter in text — **done
      2026-07-13, now fully wired** (was inert before Init/`GESaveData`'s 3-gamer-slot extension
      existed): real position/icon, shown ONLY on `MainSetup` (confirmed via `Game1::
      DrawButtonsText()`'s own `if (phase==MainSetup)` label gate — `showReset` is the caller's
      job, same pattern as Pause's showBack/showRestart). Real handler is `gameData.Reset();
      gameData.Write();` — the SAME full reset as Cheat5, not a per-gamer-only reset despite the
      label — maps directly to `GESaveData::Reset()`/`Save()`. Real 2-line label ("Player {0}
      :\nErase progress") collapsed to one line — no multi-line text renderer exists in this class,
      a formatting-only simplification (the gamer-letter content itself is now real, via
      `GESaveData::GetSelectedGamer()`).
- [x] MENU-066 — "RETURN" button (`SetupReturn`) → Init (from MainSetup) or Play (from PlaySetup)
      — **done 2026-07-13, BOTH branches now reachable** (MainSetup used to be unreachable — this
      changed once Init landed later the same session): confirmed via `Game1.cpp`'s real handler
      (`if (playSetup) SetPhase(Play,-1); else SetPhase(Init);`) — NOT a level-reload simplification
      like Win/Lost/PauseRestart, since PlaySetup never actually stops gameplay progress
- [x] MENU-067 — All toggles persist to GameData immediately on press — **Sounds+Reset done
      2026-07-13** via `GESaveData` (`src/GalaxyEggbertCNA/Game/GESaveData.hpp`/`.cpp`), a
      deliberately minimal, NOT-byte-compatible substitute for the real `GameData` (see its own
      class comment for the full research/reasoning — the real format is a fixed 640-byte blob
      shaped around a 3-gamer-slot/100+-level/200-door structure, written via a WP7-only
      `IsolatedStorageFile` API with no desktop equivalent; porting that BYTE LAYOUT would buy
      nothing since no real save file could ever cross between the two engines — though the real
      3-gamer-slot SHAPE itself was later ported faithfully via `GESaveData`'s own Phase-3
      extension, MENU-019/020). Real write-on-toggle-press behavior matched exactly. Jump/Zoom/Accel
      have nothing to persist (they're inert, MENU-062..064) — not a gap, since they have no real
      state to save
- [x] MENU-068 — Animated slide-in/out of settings panel (matching mobile-eggbert timing) — **done
      2026-07-13**: no separate "panel" element was found beyond the speedyblupi/gear decoration
      already covered by MENU-059/060 (the real `setup.png` background itself never animates,
      confirmed) — this entry describes the same mechanic, not a distinct one.
- [x] MENU-069 — Keyboard Back during Setup → Init/Play — **done 2026-07-13, CORRECTED**: Escape
      now goes to Init from MainSetup (real destination, now that Init exists) or Play from
      PlaySetup (matching `SetupReturn`'s own real per-screen destination, MENU-066) — an earlier
      pass of this entry said Escape always returns to Play, which was only true before MainSetup
      was reachable.

**MENU-058..069 summary (2026-07-13, updated same day once Init/MENU-006..020 landed):** extended
`GEInputPad` with `UpdateSetup()`/`DrawSetup()`, reachable via Pause's real Setup button
(`PlaySetup`) AND, once Init existed later the same session, via Init's own `InitSetup` button
(`MainSetup`) — both share this same pair of methods, gated by a `showReset`/`isMainSetup` bool the
caller passes (the one real difference between the two screens, `SetupReset`). A rare case where
the real `InputPad.cpp` button rects (`bsf2=drawBoundsHeight*140/480`) need ZERO proportional
adaptation: at this engine's own 480 reference height, bsf2 is EXACTLY 140, so every rect is used
unadapted, unlike the Pause row's bsf1-based layout. Added `GESaveData` (see MENU-067) after a
dedicated research pass into the real `GameData.hpp`/`.cpp` format, confirming byte-COMPATIBILITY
is a non-goal — a small, new, engine-appropriate persisted set instead, later extended (same
session) to the real 3-independent-gamer-slot SHAPE once Init needed it. Verified via
`VerifyGEInputPad` (including 2 new `SetupReset` checks: fires when `showReset=true`, never fires
when `showReset=false`), `VerifyGESaveData`, a live headless screenshot confirming exact
layout/labels/icon state, and a live two-run save/load round-trip test. The speedyblupi.png slide +
2 rotating gear.png decorations (MENU-059/060/068) were initially out of scope here and implemented
later the same session as part of the MENU-088/089 fade-transitions pass, once real entry/exit
timing needed them anyway.

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

- [ ] MENU-079 — Level intro title card: world name text, 3 s duration (fade-in 0.5s, hold 2s, fade-out 0.5s) — not implemented; this engine tracks only a mission NUMBER (`Worlds::World::missionNumber()`), not a real world name string, so this would need new data this engine doesn't parse yet
- [x] MENU-080 — Training level hint bar: show tutorial text from `table_training1..4` based on Blupi position — **done 2026-07-13, plan.md `HUD-024`** (`GETrainingHints.hpp`/`.cpp`, all 43 real hint records + real gate semantics transcribed)
- [x] MENU-081 — Training hint rendered as overlay bar (pad.png icon 15 background, text centred) — **done, `HUD-024`** (`GEHud::Draw()`'s `trainingHint` parameter, full-width top-of-screen panel)
- [x] MENU-082 — Training hint auto-scales down if text is too wide (min 0.5×) — **done, `HUD-024`** (real `min(640/textWidth, 1.0)` shrink-to-fit, approximated via this engine's own fixed-glyph-advance model)

#### 2.12 Button Font & Text Rendering

- [x] MENU-083 — Render button labels using `text.png` font sheet (32×32 per glyph) — **done**, shared by `GEHud` (HUD-023/024) and `GEInputPad` (Pause/Setup labels, 2026-07-13) — each with its own `text.png` instance per the established one-instance-per-draw-path convention
- [x] MENU-084 — `Text::DrawText` equivalent: render text string using glyph atlas — **done** (glyph index == ASCII code, read directly off the asset per `GEHud.hpp`'s own class comment, not transcribed from `table_char`)
- [x] MENU-085 — `Text::DrawTextCenter` equivalent: centre-aligned text rendering — **done** (`GEHud`'s treasure-counter/overlay-message text, `GEInputPad::AppendCenteredLabel()` for the Pause row); a LEFT-aligned variant (`Text::DrawTextRightButton()`) was also added 2026-07-13 (`AppendLeftAlignedLabel()`) for the Setup screen's real label alignment
- [x] MENU-086 — Text scaling (0.45×, 0.7×, 1.0×) used for different label sizes — **done** at the specific real scales actually confirmed in use so far: 0.7 (Pause/Setup button labels, Perso HUD counter), 1.0 (treasure counter), 1.5 (Win/Lost/overlay message, this engine's own choice for a big centered message) — 0.45× not yet needed (no ported screen uses it yet)
- [x] MENU-087 — Localised strings (MyResource strings): port key TX_ constants for button labels — **done for every button label ported so far** (real English strings only, from `MyResource::InitializeEN()` — "Home"/"Back"/"Setup"/"Restart"/"Continue" for Pause, "Sound effects"/"Jump button on the right"/"Automatic zoom on action"/"Accelerometer" for Setup); FR/DE localization is out of scope (this is a single-locale EN port, matching every other text this session)

#### 2.13 Phase Transitions & Animations

- [x] MENU-088 / MENU-089 — Fade-out animation + `fadeOutPhase` deferred transition — **done 2026-07-13**,
      `GalaxyEggbertCnaGame::SetPhase()`/`Update()` (`fadeOutPhase_` member). A dedicated research
      pass into the real mechanism found it's more precise than the draft assumed:
      - **Only 5 real phases ever defer**: `Init`, `MainSetup`, `PlaySetup`, `Pause`, `Resume`
        (confirmed via `Game1.cpp`'s own `if (this->phase == Init || MainSetup || PlaySetup ||
        Pause || Resume) && fadeOutPhase==None`). Every other phase (`Play`, `Win`, `Lost`,
        `Wait`/`First`) commits INSTANTLY regardless of destination — **Play↔Pause is a real hard
        cut, not merely fast**, and entering/leaving Win/Lost is likewise always instant (no
        separate transition fade exists for either, only the already-ported continuous idle
        pulse/grow-in). This corrects an implicit assumption in the original MENU-088 draft that
        ALL transitions might eventually fade.
      - **Real commit timer confirmed as exactly `Config::ScaleTime(20)` = 1.0s** at this build's
        pinned 20fps (matches the already-known Init-specific fades from an earlier research pass).
      - **Real exception**: `Resume`→`Play` via `ResumeContinue` (`ContinueMission()` → `SetPhase
        (Play, -2)`) is ALWAYS instant even though `Resume` is a deferring phase — the real
        `mission==-2` sentinel bypasses the defer mechanism entirely. Ported as `SetPhase()`'s new
        `bypassFade` parameter, wired only at that one real call site.
      - **Real per-destination exit-fade formulas** (all confirmed via research, verified live via
        7 headless screenshots): Pause/Resume→Play and Init→Play both reuse the "blow up to 11x
        native size while linearly fading out" idiom (same formula, different real center
        coordinates); Pause/Resume→Init is the entrance grow+spin formula run in reverse (shrinks
        while spinning UP into a full 360°, leaving a real ~0.25s "dead"/invisible window before
        the actual 1.0s commit — reproduced faithfully, not "fixed"); Pause→PlaySetup is a fixed-
        size/opacity horizontal slide-right, quadratic ease; Init→MainSetup slides the title right
        while fading and leaves blupiyoupie fixed-size, fading only (confirmed NO zoom, despite an
        earlier doc-comment in the real source itself claiming one).
      - **Real input/simulation freeze during the fade window**: `GalaxyEggbertCnaGame::Update()`
        now returns immediately (skipping all per-phase input handling) whenever `fadeOutPhase_ !=
        None`, matching the real source's own early-return before `inputPad.Update()`/
        `decor.MoveStep()`. Real buttons are ALSO hidden entirely during an active exit fade
        (confirmed: `DrawButtonsBackground()`/`inputPad.Draw()`/`DrawButtonsText()` all gated on
        `fadeOutPhase==None`) — NOT during the entry side, which renders as a decorative overlay on
        an already-interactive screen.
      - **Real Pause/Resume entrance flourish now ALSO ported** (was previously static/instant, a
        SEPARATE mechanic from the deferred-transition fade itself, confirmed bit-for-bit shared by
        both phases): a real 0.75s grow-from-a-point + decelerating 360° spin, same category as
        Win's pulse/Lost's grow-in which this engine already had.
      - **Real `PauseMenu`/`ResumeMenu` buttons are now ALSO wired** (previously documented as
        inert since "no Init/main-menu screen exists" — no longer true once Init landed): both now
        go to `Init`, exercising the new Pause/Resume→Init shrink+reverse-spin fade for real.
      - A genuine bug was caught and fixed during live verification: the initial commit logic
        pre-cleared `fadeOutPhase_` before calling `SetPhase()` again, which defeated `SetPhase()`'s
        own "already deferring → commit, don't re-defer" guard (itself a real, confirmed mechanism)
        — causing an infinite re-defer loop that silently froze `phaseTimeSeconds_` at exactly 1.0
        forever. Fixed by calling `SetPhase(fadeOutPhase_)` while it's STILL set to the pending
        target, letting `SetPhase()`'s own guard correctly commit.
- [ ] MENU-090 — `missionToStart1/2` two-stage mission loading pipeline (background swap before Start) — **partially superseded 2026-07-17**: a real mission-loading pipeline now exists (`GalaxyEggbertCnaGame::LoadMission()`, see `SCORE-013`), but it's a single-step synchronous reload (background+terrain rebuilt together, no separate "swap background first, commit world second" staging) — the real 2-stage `missionToStart1`/`missionToStart2` distinction itself is still not modeled.
- [x] MENU-091 — Phase time counter reset on each phase entry — **done 2026-07-13** (`GalaxyEggbertCnaGame::phaseTimeSeconds_`, reset to 0 inside `SetPhase()` — the real `phaseTime` port that drives the Win/Lost `blupiyoupie.png` animations, `MENU-046..057`)

#### 2.14 Cheat Menu (hidden)

Corrected 2026-07-13 against real source (`Game1::CheatAction(ButtonGlyph)` →
`Decor::CheatAction(Tables::CheatCodes)`) — several of the descriptions below were wrong/imprecise
in the original draft; the real behavior is documented here, and the `CHEAT-001..009` IDs (cited
directly in code comments) are the canonical per-cheat task IDs going forward, kept alongside the
pre-existing `MENU-092..102` numbering.

- [x] MENU-092 — Cheat gesture recognition — **done 2026-07-13**, `GEInputPad::UpdateCheatGesture()`.
      **Draft was wrong on tap count**: real is a **10-tap sequence** (`cheatGesteLength=10`), not
      6. The 6 distinct glyphs (`Cheat11/12/21/22/31/32`) are tapped in exact order
      `12,22,32,12,11,21,22,21,31,32`, tracked by `cheatGesteIndex`, reset to 0 on any wrong glyph,
      active only during real `Phase::Play`, no timeout. Real zones are invisible (no icon/text);
      this engine's zones are likewise fully invisible, laid out as a 3-col×2-row grid spanning the
      top-left ~⅔ width × ~57% height of the reference space (`cheatButtonSizeFactor =
      drawBoundsHeight/3.5`). Simplification: real source additionally resets progress on ANY other
      button press, not just a wrong gesture-zone tap — not modeled (a minor forgiving deviation,
      not a functional loss).
- [x] MENU-093 — Cheat menu overlay: 9 cheat action buttons (Cheat1..Cheat9) — **done 2026-07-13**,
      `GEInputPad::UpdateCheatMenu()`/`DrawCheatMenu()`. Real buttons are a row of nine 80×80
      ABSOLUTE-pixel boxes at the literal top-left (not scaled by `drawBoundsHeight` — a genuine
      real inconsistency vs. every other button in the game); adapted here as 9 equal columns
      proportionally spanning the full reference width, since the real fixed-pixel row doesn't fit
      this engine's own reference space. Icon: pad.png icon 0 (D-pad ring, reused generically) + a
      single-letter text label per cheat (D/B/S/E/R/T/C/T/G for cheats 1-9 — cheats 6 and 8 both
      really show "T", a confirmed real ambiguity in the original game, not a transcription error).
      No background swap (real source renders this as a transparent overlay atop whatever's already
      on screen, confirmed via live headless screenshot — the 3D scene and normal Play HUD/D-pad
      remain visible behind/through it) and no confirmation step (pressing any Cheat1-9 immediately
      closes the overlay and applies the effect instantly, matching real `showCheatMenu = false`).
- [x] MENU-094 / CHEAT-001 — Cheat 1: OpenDoors — **done**, `GEInteractionSystem::CheatOpenDoors()`.
      Draft was correct: toggles the real `m_bCheatDoors` flag and calls `AdaptDoors()`; ported here
      as opening every key-gated (`BlockTypes::isDoor()`) and treasure-gated (icon 421-440) door
      tile in the level via the existing `OpenDoorAt()` helper.
- [x] MENU-095 / CHEAT-002 — Cheat 2: SuperBlupi — **done**, `GEBlupiController::m_cheatSuperBlupi`
      + `IsInvincible()`. **Draft overstated the effect**: real `m_bSuperBlupi` is a PURE
      invincibility flag, OR'd with `!m_blupiShield && !m_blupiHide` at ~30 real hazard-death call
      sites — it does **not** grant "all abilities" as the original draft claimed.
- [ ] MENU-096 / CHEAT-003 — Cheat 3: ShowSecret — **not implemented, documented gap**. Real
      `m_bDrawSecret` gates rendering of "hidden ObjectType12 secret-decor icons," but this engine's
      own `ObjectType12` is already a confirmed, unrelated real type (pushable crate). Not resolved
      whether the real citation means the `MoveObject` enum or an unrelated static tile icon —
      deliberately left unimplemented rather than guessed and risk conflating two different real
      concepts.
- [x] MENU-097 / CHEAT-004 — Cheat 4: LayEgg — **done**, `GalaxyEggbertCnaGame::ApplyCheat()` case 4.
      **Draft was wrong**: despite the name, the real effect is `m_nbVies = 9` (sets lives to 9) —
      it does NOT spawn any egg. Confirmed live (headless run): lives 3 → 9 on trigger.
- [x] MENU-098 / CHEAT-005 — Cheat 5: Reset gamer progress — **done**, `GESaveData::Reset()`. Draft
      correct: real `gameData.Reset()`; ported as resetting every `GESaveData` field to its default
      and writing immediately.
- [x] MENU-099 / CHEAT-006 — Cheat 6: Simulate trial mode toggle — **done** (no-op by design). Draft
      correct on the real effect (`simulateTrialMode = !simulateTrialMode`), but it has no
      observable effect in this engine since the Trial phase is unreachable here; wired as a
      documented no-op case in `ApplyCheat()` rather than left out, to keep the button's real
      dispatch order intact.
- [x] MENU-100 / CHEAT-007 — Cheat 7: CleanAll — **done**, `GEInteractionSystem::CheatCleanAll()`.
      **Draft was imprecise**: NOT "remove all mobile objects" — real effect only converts a
      specific type list (ObjectType 2/3/4/16/17/20/32/33/44/54/96/97 — the existing shared
      hazard-kill-list set plus wasp/large-creature/blupih/blupit) into an explosion decoration +
      screen-shake; treasures/pickups/vehicles are untouched. Ported as deactivating exactly that
      type list.
- [x] MENU-101 / CHEAT-008 — Cheat 8: AllTreasure — **done**, `GEInteractionSystem::CheatAllTreasure()`.
      Draft correct: every `ObjectType5` is collected, `m_nbTresor++`, `OpenDoorsTresor()` (this
      engine's `ScanAndOpenTreasureDoors()`), channel-11 sound.
- [x] MENU-102 / CHEAT-009 — Cheat 9: EndGoal — **done**, `GEInteractionSystem::CheatFindExit()` +
      `ApplyCheat()` case 9. **Draft was imprecise**: NOT unconditional "win immediately" — real
      effect always teleports Blupi to the exit (`ObjectType7`); it only actually wins if
      `m_nbTresor >= m_totalTresor` already, otherwise Blupi is just moved there (real source also
      plays a "not enough" sound in that case, not modeled — ported as the teleport only). Confirmed
      live (headless run): Blupi position moved from spawn to the level's real exit marker.
- [x] CHEAT-010 — F12: second, direct way to open/close the cheat overlay — **done 2026-07-20**,
      real `InputPad.cpp:678-683` (`#ifdef MODERN`) — layered on top of (not instead of) the
      existing 10-tap gesture (`MENU-092`), same "layered" idiom as the typed-cheat methods below.
- [x] CHEAT-011 — "quick" typed cheat: unlocks F7/F8 (`GameSpeed::Faster`/`Fastest`, see
      `SCORE-009/010`) — **done 2026-07-20**, real `quick_cheat_enabled` (`InputPad.cpp:` typed-
      cheat table). Extends `GEInputPad::UpdateTypedGhostCheat()`'s existing rolling buffer (now
      returns a `TypedCheatResult{ghostTyped, quickTyped}` instead of a plain bool) — both share the
      SAME buffer, matching real source's own single-buffer `cheatEntries[]` design (typing
      "quickghost" fires both, same as real source).
- **Deferred, documented but not implemented (2026-07-20)**: real `InputPad.cpp:686-838+` has a much
  larger ~19-entry typed-cheat-word table beyond the 9-button menu + ghost/quick — most already
  covered by the `CHEAT-001..009` buttons under different names (opendoors/superblupi/showsecret/
  layegg/roundshield/quicklollypop/tenbombs/birdlime/drivetank/powercharge/hidedrink/iovercraft/
  udynamite/weelkeys), plus 3 standalone sub-features not yet triaged: `debug` (a debug overlay),
  `zoom` (camera zoom-level cycling), `cheats` (a 5-second on-screen cheat-list display). Each is a
  real, separate, faithful port task (own research + verification pass, not a blind mass-port) —
  left as a future task, not silently dropped.

Live headless verification (2026-07-13): forced the overlay open and captured a screenshot
confirming the 9-button row renders correctly (icons, letters, transparent overlay with the 3D
scene and Play HUD/D-pad visible behind it), then triggered Cheat 4 and Cheat 9 directly and
confirmed their real effects (lives 3→9; Blupi teleported to the exit marker's exact position) via
stderr diagnostics. All temporary debug code was reverted before commit. Unit coverage: 19 new
assertions in `VerifyInteractionSystem` (fresh, isolated `GEWorldRuntime`/`GEInteractionSystem` per
test, to avoid state leakage from the large shared fixture) + 6 new assertions in
`VerifyGEInputPad` (full 10-tap gesture unlock, wrong-tap reset, out-of-zone tap ignored, first/last
menu button dispatch, press-vs-release gating).

---

### 2.3 HUD (Heads-Up Display)

**Stale intro note removed 2026-07-14**: this paragraph used to say CNA "has no real HUD yet" —
no longer true (real `GEHud`, 2026-07-10/11/13 across multiple sessions; each item below already
carries its own accurate per-item date/citation, this was just a leftover boilerplate reset note).

- [x] HUD-001 — Life icons: Blupi head sprite (icon 48 from `blupi.png`) × nbVies, bottom-left row (CNA, 2026-07-11; since 2026-07-10 at the REAL `DrawInfo` position (210,417), X+=16, via `GEHud`)
- [x] HUD-002 — **Resolved 2026-07-13, this entry's own premise was wrong**: the real
      `DrawInfo` (`Decor.cpp:1190-1194`) draws exactly `m_nbVies` icons in a plain, uncapped loop
      (`for (i=0; i<m_nbVies; i++) { HudIcon(...); pos.X += 16; }`) — there is NO 5-icon cap and
      NO "+N" overflow text anywhere in the real source. `GalaxyEggbertCNA`'s existing uncapped
      `GEHud` implementation (`HUD-001`) was already correct; no change needed.
- [x] HUD-003 — Treasure counter "N/total" text, bottom-centre panel (CNA, 2026-07-10, `GEHud`): real position (460,450), glyphs from `text.png` whose sheet index IS the ASCII code (read off the asset, not `table_char`); fixed 17px advance approximates the real proportional widths
- [x] HUD-004 — Panel background behind treasure counter (pad.png icon 15) (CNA, 2026-07-10, `GEHud`)
      — **restored to the real 0.6 opacity 2026-07-18** (user-reported: main-menu buttons/panels
      render as solid opaque white instead of translucent). Root cause: `kPanelOpacity` (and
      `GEInputPad::DrawInit()`'s own equivalent Init-screen panel opacity, same `pad.png` icon 15)
      had been forced to 1.0 since 2026-07-10 specifically because Vulkan was the default backend
      and CNA's Vulkan `BasicEffect` doesn't render an Alpha<1 draw at all (a genuine, still-unfixed
      CNA bug). Confirmed the source `pad.png` asset itself already has real alpha baked in
      (icon 15 sampled directly: uniform RGBA(255,255,255,200), i.e. ~78% opaque white) — the
      renderer was simply discarding the ADDITIONAL 0.6 multiplier on top of that, not a texture
      problem. Now that `CMakeLists.txt`'s default backend is EasyGL (switched the same day, see
      `E3D-MIG-069`'s own writeup), which renders Alpha<1 correctly, both call sites restored to the
      real 0.6 value. Building with `-DCNA_GRAPHICS_BACKEND=VULKAN` will still make these panels
      vanish — the underlying CNA bug itself remains unfixed, tracked separately.
- [x] HUD-005 — Key icon — red key (element.png icon 215) shown when Key1 held (CNA, 2026-07-11; since 2026-07-10 at the REAL position (520,418) via `GEHud`)
- [x] HUD-006 — Key icon — green key (element.png icon 222) shown when Key2 held (CNA, 2026-07-11; real position (530,418))
- [x] HUD-007 — Key icon — blue key (element.png icon 229) shown when Key3 held (CNA, 2026-07-11; real position (540,418))
- [x] HUD-008 — Shared Shield/Power/Cloud/Hide countdown gauge (jauge.png yellow fill) at
      (90,428) — visible while any of the 4 is active (CNA, 2026-07-13, `GEHud`, verified
      directly against `Jauge.hpp` + `Decor.cpp:5071-5137`: all 4 real states reuse the SAME
      `m_blupiTimeShield` variable and `m_jauges[1]` widget, not 4 separate gauges — corrects
      this entry's own "Shield timer" framing, which undersold the real scope)
- [ ] HUD-009 — `[?]` Score display (text label, top-right area). **Flagged 2026-07-13**: after
      fully reading the real `Decor::DrawInfo` (`Decor.cpp:1185-1311`, the actual in-game HUD
      draw function -- every other `HUD-0NN`/`HUD-1NN` item cross-checked against it this
      session came from this exact function) end to end, there is NO score display or numeric
      score variable drawn anywhere in it, and no `m_score`-style field exists in `GameData.hpp`
      either. The only real "score" concept found is a separate high-score/`Ranking` MENU screen
      (`Def.hpp`'s `GameState::Ranking`, `Game1.hpp`) -- ## 2 §2 MENU-* territory, not part of the
      in-game HUD at all. Do not implement an in-game score display without first finding the
      real source of this claim; it may be a stale/mistaken entry from before `DrawInfo` was
      fully read. **Re-checked 2026-07-13 (follow-up research)**: a broad grep for `score`/`Score`
      across the ENTIRE mobile-eggbert `src/`+`include/` tree (not just `Decor.cpp`) still finds
      nothing but the `Ranking` menu screen and doc-comment prose — no numeric score field exists
      anywhere in the codebase. This is very likely simply a mistaken/invented entry, not an
      under-researched one; do not implement.
- [ ] HUD-010 — `[?]` World name + elapsed level timer. **Flagged 2026-07-13**: same as `HUD-009`
      -- not present anywhere in the real `DrawInfo`. **Confirmed 2026-07-16**: read the complete
      real `Decor::DrawInfo()` function end to end (`Decor.cpp:1185-1311`, its full extent, not an
      excerpt) -- no world name, no elapsed timer, no clock/timer variable of any kind anywhere in
      it. Same likely-mistaken/invented status as `HUD-009`; do not implement.
- [ ] HUD-011 — `[?]` Game speed indicator label (SLOW / NORMAL / FAST). **Flagged 2026-07-13**:
      `GameSpeed` itself IS real (`Game1.hpp`'s `gameSpeed`/`SetGameSpeed()`/`getGameSpeed()`), but
      it lives in the `Game1` application/settings layer, not `Decor`. **Confirmed 2026-07-16**: the
      complete real `DrawInfo()` (`Decor.cpp:1185-1311`) draws lives/bullets/Perso/dynamite/keys/
      treasure-count/gauges/training-hint only -- no speed label anywhere. Likely a settings-menu
      display (unconfirmed, not part of this task's scope), not an in-game HUD element as this
      entry assumed; do not implement here.
- [x] HUD-012 — Water/Nage breath gauge (jauge.png, real `m_jauges[0]`) at (90,450), Blue normally
      (CNA, 2026-07-13, `GEHud`, verified directly against `Decor.cpp:5310-5326`, wired to the
      already-implemented `GEBlupiController::IsNage()`/`GetWaterGaugeLevel()`, `E3D-MIG-148`)
- [ ] HUD-013 — `[?]` Hit flash: red full-screen overlay panel, 0.4 s fade on damage. **Flagged
      2026-07-13**: not found in `DrawInfo`. **Confirmed 2026-07-16**: read the complete real
      `DrawInfo()` (`Decor.cpp:1185-1311`) end to end -- no full-screen overlay/flash/fade drawing
      of any kind. Camera shake on hit (`HUD-014`/`CAM-008..013`) is the real, confirmed hit-
      feedback mechanic -- this entry's separate red-flash-panel premise appears to be
      conflating/inventing on top of that already-real mechanic. Do not implement.
- [x] HUD-014 — Camera shake on hit — **done 2026-07-14, filed under CAM-008..013 (camera work),
      not `GEHud`** — see those entries for the full implementation writeup. Was blocked on
      `table_decor_action`'s data-table transcription approval; the user granted blanket approval
      2026-07-14, unblocking this along with the rest of the camera-shake system.
- [x] HUD-015 — Bullet counter: element.png icon 176 × bullets held, X+=4 fanned row at (570,442)
      (CNA, 2026-07-13, `GEHud`, verified directly against `Decor.cpp:1197-1201`)
- [x] HUD-016 — Dynamite count: element.png icon 252 at (505,414), shown only while carrying one
      (CNA, 2026-07-13, `GEHud`, verified directly against `Decor.cpp:1212-1217`)
- [x] HUD-017 — Perso decoy counter — **done 2026-07-13**, both the HUD element AND the
      underlying place/retrieve mechanic. Researched what "Perso" actually is (not previously
      known): a deployable `ObjectType200` decoy statue (same `blupi.png` look as Blupi himself)
      — verified directly against `Decor.cpp:4818-4841` (placement), `6088-6101` (pickup start),
      `10291-10294` (pickup completion). Real gate: mutually exclusive with dynamite (an
      `else if` in the real source — modeled the same way here, dynamite placement takes
      priority on the same action-button press). New `GEInteractionSystem::TryPerso()`: a single
      call handles BOTH real branches — picks up an already-placed decoy within range if one
      exists (real: takes priority over placing a new one), otherwise places a new one if
      `persoCount_ > 0` and grounded. Real cap 5 (gates pickup, not placement). HUD: button.png
      icon 108 (40px/6-col tiles, `Pixmap.cpp:592-600`) at (0,438) + "= N" text at (32,452) at
      real scale 0.7 (smaller than the treasure counter's scale-1.0 text).
      **Mystery resolved 2026-07-16**: what placing a decoy actually does gameplay-wise (the
      original search targeted enemy-AI code and missed it) — verified directly against
      `Decor.cpp:7957-7975`/`Decor::MovePersoDetect()` (~9835-9865): small enemies (4/32/33) that
      patrol into contact with ANY real 200-203 object (the placed decoy itself, OR one of the
      201-203 lethal decorations, `PICKUP-069`) mutually destroy each other — an explosion +
      channel 10 + SmallShake at the enemy's position, then an `ObjectType37` dissolve effect, the
      decoy/decoration also deleted. Implemented as a new block in `GEInteractionSystem::Update()`
      right before the Cloud-aura check (which targets the same 3 enemy types), reusing the
      existing `AppendExplosionFlash()` helper for both spawned effects. 4 new
      `VerifyInteractionSystem` assertions (trigger + mutual destruction, no-trigger when far
      apart). Full suite green both backends + live headless launch smoke check. **Still NOT
      modeled**: the real voyage-flight-animation before the pickup counter increments (skipped,
      same simplification as every other pickup this session); and the real starting count, which
      is level-authored save data (`_blupiPerso_`, default 0, no world-pickup grants it at all) —
      this engine has no level-authored-starting-inventory concept yet, so `persoCount_` always
      starts at 0 (matching the real common-case default) and the mechanic is exercised only via
      synthetic test objects, not a sample-world demo placement (there is no real pickup object
      to place one). Verified: 8 new `VerifyInteractionSystem` assertions (starts at 0, placement
      no-ops at 0, full pickup->place round trip via a synthetic decoy) + full suite (63/63 unit
      tests, all verify tools) + live headless HUD screenshot (temporary forced count, reverted
      before committing) + both backends.
- [x] HUD-018 — **Corrected 2026-07-13**: this is NOT a separate "charge mode" gauge — it's
      `HUD-012`'s SAME water/Nage gauge (`m_jauges[0]`) switching from Blue to Red at the real
      low-air warning threshold (level <= 25, `Decor.cpp:4621-4623`), still driven by the same
      `m_blupiLevel`/breath variable, not a distinct charge-level mechanic. Done as part of
      `HUD-012` (CNA, 2026-07-13, `GEHud`).
- [x] HUD-019 — **Corrected 2026-07-13**: same widget as `HUD-008` (`m_jauges[1]`, shared
      Shield/Power/Cloud/Hide countdown, not shield-specific) — done together with `HUD-008`
      (CNA, 2026-07-13, `GEHud`).
- [ ] HUD-020 — `[?]` "EXIT OPEN!" popup text (3 s timed, big centred text) when all treasures
      collected. **Not found in `DrawInfo`** (2026-07-13 sweep) -- a grep for "EXIT"/"OPEN"/
      "SORTIE" across `Decor.cpp`/`Game1`-layer headers found nothing; may be a localized string
      resource triggered from a different, not-yet-read code path, or may be a stale entry. Needs
      its own verification before implementing. **Re-checked 2026-07-13 (follow-up)**: also grepped
      for `EXIT`/`ExitOpen`/`OpenExit` across the ENTIRE `src/`+`include/` tree — zero hits. Same
      likely-mistaken status as `HUD-009`/`025`, though not as conclusively ruled out (a
      localized-string-only trigger could still exist unread). **Fully ruled out 2026-07-17**:
      grepped `MyResource.cpp`/`.hpp` (the real localized-string resource file, the last unread
      possibility) for `exit`/`open`/`sortie` — zero hits there either. No localized string, no
      code path, nothing — likely mistaken/invented like the others, not merely under-researched;
      do not implement.
- [ ] HUD-021 — `[?]` Controls hint bar fades after 8 s (re-show on new level). Not in `DrawInfo`
      (real training-hint overlay there is `HUD-024`, mission-gated, a different thing) --
      **Re-checked 2026-07-13 (follow-up)**: grepped for hint-bar/8-second-timer patterns across
      the whole tree, zero hits. **Confirmed 2026-07-16**: read the complete real `DrawInfo()`
      (`Decor.cpp:1185-1311`) end to end -- the ONLY hint-related element is the mission-gated
      training overlay (`HUD-024`'s own position-triggered, per-mission array lookup, not a
      generic "controls hint bar"), with no separate 8-second fade timer anywhere. Likely
      mistaken/invented, not merely under-researched; do not implement.
- [ ] HUD-022 — `[?]` Pause button icon visible during Play phase (top area). Not in `DrawInfo` —
      likely a `Game1`/menu-layer button, not a `Decor` HUD element; needs its own verification.
      **Confirmed real 2026-07-13 (follow-up research)**: lives in `InputPad.cpp`
      (`Def::ButtonGlyph::PlayPause`/`PauseMenu`/`PauseBack`/`PauseSetup`/`PauseRestart`/
      `PauseContinue`, ~lines 160-968) — the touch/button-overlay control system, a completely
      separate class from `GEHud`'s own `Decor::DrawInfo` port. Genuinely real and unimplemented,
      but belongs with a future touch/button-overlay task, not folded into `GEHud` as-is.
- [x] HUD-023 — HUD hidden during non-Play phases — **done 2026-07-13**, per explicit user
      request to implement the full real `Def::Phase` state machine (not a trimmed-down subset).
      Verified directly against `Def.hpp:51-66` (the enum itself) and `Game1.cpp:394-438`/
      `979-1058` (`Update()`'s `if (phase==Play)` simulation gate, `SetPhase()`'s own transition
      funnel).
      - The real 13-value enum was already ported verbatim as `GalaxyEggbert::GamePhase`
        (`include/GalaxyEggbert/def/GamePhase.hpp`, from an earlier session's blanket
        transcription pass, previously unused anywhere). `GalaxyEggbertCnaGame` gained a real
        `phase_` field + `SetPhase()`, matching the real source's own "never assign `phase_`
        directly, always through `SetPhase()`" discipline.
      - **Real simulation gate**: confirmed `Decor::MoveStep()` (this engine's own
        `worldRuntime_`/`blupi_`/`interaction_` per-frame Update() calls) is the ONLY thing gated
        behind `Phase::Play` in the real source — ported as a single early-return
        (`if (phase_ != Play) return;`) wrapping this engine's entire existing gameplay-update
        block, verified live (a forced Escape-press/release at two different frames showed
        `phase_` toggling Play<->Pause exactly on cue, temporary debug instrumentation, reverted
        before committing).
      - **Real Pause**: the real source's own trigger is gamepad-Back/a touch button — genuinely
        no keyboard binding exists in mobile-eggbert at all (an XNA/WP7 port) — Escape is this
        engine's own pick. Real `PauseContinue` (resume in place) is implemented; real
        `PauseRestart`/`PauseMenu`/`PauseSetup` (reload level / return to main menu / open
        settings) are NOT modeled, since no level-reload or menu system exists yet.
      - **Real Win/Lost triggers**: verified they map exactly onto state this engine already
        tracks — real Lost (`Decor.cpp:6374-6435`, a death animation completing while lives are
        exhausted, `DoorsLost()`'s reset-to-3) is precisely what
        `GEInteractionSystem::GameOverCount()` already increments at; real Win
        (`Decor::IsTerminated()`, reaching the exit with all treasure) is precisely what
        `GEInteractionSystem::ExitReached()` already gates on. No new gameplay logic needed for
        either trigger. Real `WinLostReturn` has no fixed auto-timer (explicit input only) — the
        Action key returns to Play here, resetting Blupi to the origin spawn point (NOT a full
        real level-reload — lives/treasure/keys/etc. are deliberately left as-is, a documented
        simplification; the real per-mission win-screen nuance found during research, where only
        the very last mission shows a real Win screen and every other "win" silently advances to
        the next sub-level while staying in `Play`, doesn't map cleanly onto this engine's
        single-world setup and wasn't attempted).
      - **Superseded note (originally written before `Wait`/`Init`/`MainSetup` existed):** this
        entry originally said `GalaxyEggbertCNA` starts directly in `Play` since no async
        content-loading step or main menu existed. That changed later the same session
        (MENU-001..020): the engine now starts in `Wait` and reaches `Play` via a real
        `Wait`→`Init`→`Play` (or `Wait`→`Resume`→`Play`) flow — see §2.1/§2.2 above for full
        detail. `Trial`/`Ranking` remain real enum values with NO trigger wired to them at all
        (no upsell/ranking screens exist).
      - **New `GEHud::Draw()` `overlayMessage` parameter**: when non-null, the real `DrawInfo` HUD
        this class ports is skipped ENTIRELY (matching the real "HUD hidden outside Play"
        behavior exactly) and replaced with just one big centered message ("PAUSED"/"YOU WIN!"/
        "GAME OVER") — not a claim of real Pause/Win/Lost SCREEN parity (those are full menu
        screens needing score/level-slot/level-time infrastructure this engine doesn't have,
        `GalaxyEggbertSimple3D`'s own already-shipped phase machine has some of this — read for
        design reference only, not copied/linked, per `CLAUDE.md`), just an honest, minimal
        placeholder so a non-Play phase isn't a silent, feedback-free freeze.
      - Verified live: default Play behavior unchanged (screenshot comparison), all 3 overlay
        messages render correctly centered with the normal HUD fully hidden, and the Play<->Pause
        toggle responds correctly to simulated input across multiple frames (temporary debug
        instrumentation, reverted before committing) + full suite (64/64 unit tests, all verify
        tools — this file's own state-machine logic isn't independently unit-testable, tied to
        the full CNA `Game`/window lifecycle like the rest of `GalaxyEggbertCnaGame.cpp`, same as
        every other change to this specific file this session) + both backends.
- [x] HUD-024 — Training hint overlay at screen top (missions 11-14 only) — **done 2026-07-13**,
      per explicit user approval to (1) implement a real mission-number concept, (2) transcribe
      all 4 `Tables::table_training1`-`4` arrays, and (3) find and transcribe their real English
      hint text. Verified directly against `Decor.cpp:1258-1310`/`1313-1340` (`IsDisplayInfo`) and
      `Tables.cpp:1945-2004`.
      - **Mission concept**: `Worlds::World` gained a real `missionNumber()`/`setMissionNumber()`
        (format v2's first already-reserved header field put to use — see that class's own
        `saveToFile()`/`loadFromFile()` doc comments — no format/size change, still v2, existing
        `.vwr` files load unaffected with `missionNumber()==0`). `GEWorldRuntime::
        GetMissionNumber()` passes it through. `LoadFromMobileEggbertFile()` (the `.txt` loader)
        always resets it to 0 — the real `m_mission` is derived from which level FILE is loaded
        (`world011.txt` -> mission 11), not a header field within the file, unlike `region=`; not
        modeled, a documented gap matching `skyRegion_`'s own existing precedent there. The shared
        sample world (`worlds3d/world001.vwr`) is deliberately left at mission 0 (no training
        hints) rather than forced to 11 — its own layout is unrelated to the real tutorial
        world's, so training hints would fire in nonsensical places overlaid on unrelated demo
        content; verified instead via `VerifyInteractionSystem` + a live headless screenshot with
        a temporary hardcoded mission override (reverted before committing).
      - **New `GETrainingHints.hpp`/`.cpp`**: all 43 real hint records across the 4 missions
        (transcribed and cross-checked against `Tables.cpp`'s own doc comments identifying each
        record's text-resource-ID slot position, confirming the record grouping is right), plus
        the real `IsDisplayInfo` gate semantics (`>=0` = exact treasure-count match, `-1` =
        always, `-2`/`-3` = not-in-any-vehicle / in-any-vehicle, `-4`/`-5` = not-carrying-dynamite
        / carrying-dynamite — the real source's own `-2`/`-3` only ever distinguish Jeep from
        Helicopter/Skateboard/Tank to resolve to the same "in ANY vehicle" result either way,
        collapsed to one bool here). First rect+gate match wins, matching the real source's own
        `break` — including the handful of real slots whose text is genuinely empty (matches but
        shows nothing, doesn't fall through to a later record).
      - **Real English text found and transcribed** from `MyResource.cpp`'s own `InitializeEN()`
        (the D-pad/button control variant, not the accelerometer `...a` variant — this engine has
        no tilt input). Inline control-glyph placeholders (nul, and other low control bytes in the 0x02-0x09 range) that
        the real `Text::DrawChar` renders as button-icon pictograms are NOT modeled (this engine's
        text renderer only supports the printable ASCII glyph range) — replaced with short
        bracketed labels (`[Move]`/`[Jump]`/`[Action]`) instead of being silently dropped, a
        documented simplification, not a claim about the exact original icons.
      - **HUD rendering**: full-width `pad.png` icon-15 panel at the very top of screen
        ((0,0)-(640,40), real opacity 1.0 — a distinct draw call from the treasure counter's own
        0.6-opacity panel, its own dedicated quad batch/renderer, not sharing `kPanelOpacity`),
        text centered and shrunk to fit (real `min(640/textWidth, 1.0)`, approximated via this
        class's existing fixed-glyph-advance model rather than the real proportional font width).
      - Verified: 1 new `WorldSerializationTests` (mission-number save/load round trip) + 12 new
        `VerifyInteractionSystem` assertions (one representative case per mission, the
        exact-treasure-count gate, the vehicle/dynamite gates producing different text at the same
        rect, the empty-slot case, out-of-rect and out-of-mission no-hint cases) + full suite
        (64/64 unit tests, all verify tools) + 2 live headless HUD screenshots (a short and a long
        hint, confirming panel position, text centering, and the shrink-to-fit scaling all work;
        temporary hardcoded mission/position overrides, reverted before committing) + both
        backends.
- [ ] HUD-025 — `[?]` Score popup: "+N" floating text. Same as `HUD-009` — no real score variable
      found anywhere; do not implement without first finding its actual real source.
- [ ] HUD-026 — `[?]` "EXIT OPEN!" text: centre screen, large font, 3 s duration. Same flag as
      `HUD-020` (likely the same real event, described twice under 2 numbers) — needs its own
      verification, not found in `DrawInfo`.

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
- [x] BLUPI-004 — Crouch: Left Shift sets Down state — **stale, corrected 2026-07-16**: already
  implemented (`crouchHeld = keys.IsKeyDown(Keys::LeftShift)` in `GalaxyEggbertCnaGame.cpp`,
  drives `AnimState::Down` in `GEBlupiController::UpdateAnim()`). Only the visible SPRITE for
  this state is blocked (no 3D Blupi model, `069`) — the state-tracking itself works.
- [ ] BLUPI-005 — Look up / glide: Right Shift in air → reduced gravity, capped fall speed —
  **premise likely wrong, corrected 2026-07-16**: verified directly against `Decor.cpp:3236-3243`
  — real `BlupiAction::Up` requires `!m_blupiAir` (explicitly GROUND-ONLY, a cosmetic look-up pose
  triggered by the 2D Y-axis input while standing), not an airborne glide. A broad grep for
  "glide"/reduced-gravity-while-holding-up across all of `Decor.cpp` found nothing. This looks
  like another invented/hallucinated premise (same category as the already-cancelled `BLUPI-126`/
  `131` stomp-kill) rather than a real missing mechanic — `Right Shift` (`lookUpHeld`) IS already
  wired in this engine, correctly, for the ground look-up pose AND (separately, correctly) for
  Helicopter/Overcraft vertical-flight ascend; there is no third "glide" case to add.
- [x] BLUPI-006 — Auto step-up: 1-tile ledges climbed automatically (CNA, 2026-07-10) *(3D adaptation)*
- [x] BLUPI-007 — Grid-based tile collision (CNA, 2026-07-10) — collision-only, not the 2D AABB/CharacterController transcription
- [x] BLUPI-008 — Respawn at blupiStart on death — **stale, corrected 2026-07-16**: already
  implemented as the real 10-slot safe-position FIFO respawn (`GEBlupiController::
  UpdateSafePosition()`/`GetValidX/Y/Z()`, plan.md `067`), verified directly against
  `Decor.cpp:6467-6478`/`6654-6673` back on 2026-07-11 — this specific checklist entry was just
  never marked done at the time.
- [x] BLUPI-009 — Respawn invincibility: 2 s grace period after death — **CONFIRMED NON-FEATURE,
      2026-07-14, cross-referenced 2026-07-17** — the same claim as `103` above (Phase 10), just
      duplicated under this section's own numbering and never closed here too. Direct source
      research (grepped all of `Decor.cpp`/`Decor.hpp`) found no invincibility flag/timer tied to
      respawn; the real post-respawn "safety" is purely spatial (the FIFO safe-position system,
      `067`), not temporal immunity. Per `CLAUDE.md`'s faithful-remake rule, not implemented —
      closed as a confirmed non-feature, not a remaining gap. See `103`'s own entry for the full
      citation.
- [x] BLUPI-010 — Flash during invincibility (10 Hz sprite show/hide) — **closed 2026-07-17,
      same reason as `BLUPI-009`**: there is no invincibility window to flash during (confirmed
      non-feature) — nothing to implement here either.
- [ ] BLUPI-011 — Blob shadow (scans downward, scales with height) *(3D adaptation)*
- [ ] BLUPI-012 — Sub-pixel accumulator (m_blupiSubPixelX/Y) — prevents drift at high FPS.
  **Clarified 2026-07-16**: this is a real 2D-integer-position-truncation workaround (the real
  engine stores position as integers, so sub-pixel motion needs an explicit fractional
  accumulator). This engine stores position as native floats throughout — there is no integer
  truncation to work around, so this isn't a gap to port, it's architecturally moot (same
  category as the already-documented Ecrase-hitbox non-goal above).
- [ ] BLUPI-013 — BlupiBloque: directional collision query (can I move here?) — **clarified
  2026-07-16**: the EQUIVALENT real functionality (can Blupi move to a given position) is already
  implemented via this engine's own 3D-native collision (`GEBlupiController::TryMoveAxis()`/
  `GroundHeightAt()`, `BLUPI-006`/`007`, already marked done as "3D adaptation, not the 2D
  transcription"). Not a gap — a literal `BlupiBloque` port was never the goal, just left
  unclosed as its own checklist line.
- [ ] BLUPI-014 — BlupiAdjust: push Blupi out of penetrated tiles after movement — same
  clarification as `BLUPI-013`: covered by this engine's own 3D collision-resolution shape in
  `TryMoveAxis()`, not a separate gap.
- [x] BLUPI-015 — SCROLL_SPEED = 8 px/tick camera scroll toward Blupi — **cross-referenced and
      closed 2026-07-17, same item as `CAM-018`** — see that entry for the full evaluation
      (already delivered by `CAM-006`'s exponential damping; the flat px/tick figure has no
      portable 3D equivalent).
- [x] BLUPI-016 — m_blupiLevel: charge level gauge for vehicles/actions — **stale AND label
  imprecise, corrected 2026-07-16**: verified directly against `Decor.cpp:4619-5324` — real
  `m_blupiLevel` is specifically the water/Nage breath gauge (100→0 while submerged, Shield/Hide
  immune), not a general "vehicles/actions charge gauge". Already implemented as `148`'s water
  breath gauge (`GEBlupiController::GetWaterGaugeLevel()`/`kWaterGaugeMax`).
- [ ] BLUPI-017 — m_blupiTimeNoAsc: timer preventing lift re-entry after a dismount —
      **cross-referenced 2026-07-16/17, still correctly blocked**: this is the same real timer
      already researched at `PICKUP-027`, not a standalone lift-cooldown — it's an intrinsic part
      of the `AscenseurVertigo` wide/shiftable-lift-teeter mechanic (icons 311-316), blocked on
      the same deferred render/icon-selection decision as `PICKUP-024`/`153`. Not independently
      implementable.
- [ ] BLUPI-018 — m_blupiTimeMockery: timer for enemy mockery animation — **cross-referenced
      2026-07-17, still correctly blocked**: part of the same idle "fidget" animation-state
      system flagged blocked in `NEXT.md` (needs `AnimState` values this engine doesn't have,
      same prerequisite as the missing 3D Blupi model) — see the `SOUND-046`-`057`-family
      "idle fidget channel" cross-references and the `PICKUP-086/087` cancellation note.
- [ ] BLUPI-019 — m_blupiTimeOuf: relief animation timer (Ouf variants) — **cross-referenced
      2026-07-17, same blocker as `BLUPI-018`**: the Ouf1a-5 reaction animations are part of the
      same idle-fidget system (channels 48/49/etc.), blocked on the missing `AnimState` values.
- [x] BLUPI-020 — m_blupiFifoPos[10]: history of last 10 positions — **stale AND wrong
  parenthetical, corrected 2026-07-16**: already implemented as part of `067`'s safe-position FIFO
  (same as `BLUPI-008` above). The "(used for teleporter exit placement)" claim is wrong — verified
  directly against `Decor.cpp:6475-6671` — the real FIFO is used SOLELY for `m_blupiValidPos`
  (safe respawn), never referenced by teleporter code at all; the real teleporter-exit mechanism
  is the separate paired-teleporter lookup (`Decor::SearchTeleporte()`, already ported as this
  engine's own `GEWorldRuntime::FindTeleportDestination()`).
- [x] BLUPI-021 — Blupi "front" flag (m_blupiFront): determines draw order vs objects —
      **closed 2026-07-17, architecturally moot, same category as `BLUPI-012`**: this is purely a
      2D-era manual sprite-layering workaround (which of two overlapping flat sprites paints on
      top). This engine's real depth buffer already resolves draw order correctly for actual 3D
      geometry/billboards without needing an explicit per-object flag — not a gap to port.

#### 4.2 BlupiAction State Machine (87 states)

- [x] BLUPI-022 — None (uninitialised) (CNA, 2026-07-10 — trivial default state)
- [x] BLUPI-023 — Stop (idle standing) (CNA, 2026-07-10) — **upgraded 2026-07-18** (user question:
      "does Blupi get bored and tap his foot after standing still a while?"): the real Stop
      BlupiAction (`table_blupi` id=1) is genuinely 330 frames long, not 1 -- this was previously a
      Simple3D-era single-frame placeholder (`kStopFrames[]={0}`), now replaced with the real full
      table. Confirmed this is NOT a separate "boredom timer" mechanism — it's simply the natural
      consequence of a long (16.5s at the real 20Hz reference rate) idle cycle: mostly icon 0 with
      periodic short twitches (23, 133) and one longer gesture (135-138) baked directly into the
      table, restarting from frame 0 every time Blupi re-enters Stop. Live-verified via a temporary
      debug harness (reverted before commit): standing still long enough genuinely reaches icon 23
      (the real "twitch/foot-tap" frame). Full regression clean.
- [x] BLUPI-024 — March (walking) (CNA, 2026-07-10)
- [ ] BLUPI-025 — Turn (turning around)
- [x] BLUPI-026 — Jump (jumping) (CNA, 2026-07-10)
- [ ] BLUPI-027 — Air (airborne / falling) — note: CNA AnimState list is Stop/March/Jump/Down/Up; confirm Air is distinct before marking
- [x] BLUPI-028 — Down (crouch) (CNA, 2026-07-10 — state exists in AnimState enum; verify crouch logic itself, see BLUPI-004)
- [x] BLUPI-029 — Up (look-up / glide) (CNA, 2026-07-10 — state exists in AnimState enum; verify glide logic itself, see BLUPI-005)
- [ ] BLUPI-030 — Vertigo (hanging on ledge in fear — ACTION_VERTIGO)
- [ ] BLUPI-031 — Recede (moving backward — ACTION_RECEDE)
- [ ] BLUPI-032 — Advance (moving forward — ACTION_ADVANCE)
- [x] BLUPI-033 — Clear1..Clear8 (clearing animations — used for special level events) — **stale
      checkbox, closed 2026-07-19** (this stub predates the detailed per-cause breakdown done
      2026-07-16/18): Clear1-4/Glu/Drown are wired via `DeathLocked`'s own per-cause frame table
      (see the line-1468-1491 note above and `mobile-eggbert-reference/08-animations.md` §8).
      Clear5-Clear8 are confirmed real dead code (re-verified 2026-07-19 by a dedicated research
      fork): `Decor::BlupiDead()` (`Decor.cpp:6547-6552`) never assigns them as a death cause —
      intentionally not implemented, not an engine gap.
- [ ] BLUPI-034 — Set (placing object — ACTION_SET)
- [ ] BLUPI-035 — Win (level-win celebration animation)
- [x] BLUPI-036 — Push (pushing a crate — ACTION_PUSH) — **done 2026-07-18**, see `BLUPI-047`'s shared
      writeup — real single looping cycle (`table_blupi` ID 14, no idle/moving split in real source)
      while `Step()`'s new `pushingCrate` parameter is true. The real crate-push LOOP SOUND (ch38) was
      already implemented independently (`SOUND-048`, 2026-07-16, via `wasPushingCrate_`) — this only
      adds the missing visual/icon layer on top of already-correct audio.
- [x] BLUPI-037 — StopHelico (helicopter hover) — **done 2026-07-18**, see the shared writeup at `BLUPI-047`.
- [x] BLUPI-038 — MarchHelico (helicopter fly forward) — **done 2026-07-18**, see `BLUPI-047`.
- [ ] BLUPI-039 — TurnHelico (helicopter turn) — NOT modeled, see `BLUPI-047`'s own Turn-variant gap note.
- [x] BLUPI-040 — StopNage (treading water) — **done 2026-07-18**, see `BLUPI-047`.
- [x] BLUPI-041 — MarchNage (swimming forward) — **done 2026-07-18**, see `BLUPI-047`.
- [ ] BLUPI-042 — TurnNage (turning while swimming) — NOT modeled, see `BLUPI-047`'s own Turn-variant gap note.
- [x] BLUPI-043 — StopSurf (surfboard idle) — **done 2026-07-18**, see `BLUPI-047`.
- [x] BLUPI-044 — MarchSurf (surfing forward) — **done 2026-07-18**, see `BLUPI-047`.
- [ ] BLUPI-045 — TurnSurf (turning on surfboard) — NOT modeled, see `BLUPI-047`'s own Turn-variant gap note.
- [x] BLUPI-046 — Drown (drowning in deep water) — **already done** (real per-cause `DeathLocked` hurt-sprite table, `kDrownFrames`, added 2026-07-16) — not a separate `AnimState`, see `BLUPI-120`.
- [x] BLUPI-047 — StopJeep / MarchJeep / TurnJeep (jeep vehicle states) — **Stop/March done 2026-07-18,
      Turn NOT modeled** (user-reported gap: "the HUD icon only shows a limited set of animations" —
      full audit of `table_blupi` found only ~10 of the real 87 `BlupiAction` states had ever been
      transcribed). Extracted the REAL data directly: wrote a small parser mirroring
      `Decor.cpp:2393-2400`'s own `{actionId, frameCount, holdFrame, icon0..iconN}` record-scan
      exactly, ran it against mobile-eggbert's actual `Tables::table_blupi[2911]` literal (not
      hand-transcribed), cross-validated by reproducing the ALREADY-approved `kChargeFrames`/
      `kTeleportingFrames` byte-for-byte before trusting it for anything new. Added real Stop/March
      icon pairs for all 5 vehicle modes this covers (Helico/Jeep/Tank/Skate/Over, see `BLUPI-037/038/
      040/041/043/044/053/058/069`), Swim/Surf (`040/041/043/044`), Hide (`051`), Push (`036`), and the
      3 one-shot actions Switch/TakeDynamite/PutDynamite (`073/076`) — selected in
      `GEBlupiController::UpdateAnim()`'s existing precedence cascade by the SAME flags that already
      drive the underlying (already-functional) mechanics: `m_vehicleMode`/`m_nage`/`m_surf`/
      `SecretPower::Hide`/a new `pushingCrate` `Step()` parameter (the caller's own
      `GEInteractionSystem::CrateBeingPushedThisFrame()`, one frame delayed since that's only known
      after `interaction_.Update()` runs). **Real Turn variants (TurnHelico/Jeep/Tank/Skate/Over/Nage/
      Surf) are NOT modeled** — precise real turn-trigger detection (a direction-change edge, distinct
      from just "moving") needs its own dedicated research pass, same open gap as the base humanoid
      `Turn` action itself (`BLUPI-025`, never implemented either). Skateboard's own real JumpSkate/
      AirSkate airborne variants also NOT modeled (StopSkate/MarchSkate keep showing while airborne on
      a board). `TakeSkate`/`DeposeSkate`/`SlowdownSkate`/`TakeDynamite`'s own real frame data exists
      in the extracted table but isn't wired to a live trigger yet (no edge-detected "just
      mounted/dismounted skateboard" or "just picked up dynamite" signal exists to hook it to — the
      skate mount/dismount and dynamite pickup are both already-functional mechanics, just not
      exposed as the kind of one-frame edge event `TriggerOneShotAnim()` needs). Live-verified via a
      temporary debug harness (reverted before commit): `TriggerMount(Jeep)` correctly selects
      `StopJeep` and cycles the real `111,110,111,112` icon sequence exactly. Full regression clean
      (only the pre-existing unrelated `easy-gl-resource-smoke-tests` failure).
      **Follow-up 2026-07-19**: JumpSkate/AirSkate, TakeSkate/DeposeSkate, and FireTank (the three
      gaps called out above) are now wired — see `BLUPI-053/054/055/058`'s own updated entries for
      the details. Real Turn variants remain the one deliberately-deferred category; two research
      forks this session found the exact real trigger condition (`Decor::BlupiStep()`'s
      direction-mismatch between `m_blupiSpeedX`'s sign and `m_blupiDir`) and reconfirmed
      Clear5-Clear8 as genuine dead code (`Decor::BlupiDead()` never assigns them) — see
      `mobile-eggbert-reference/08-animations.md` §8 for the full citations.
- [ ] BLUPI-048 — StopPop / Pop (pop-star costume idle/dance) — real data extracted (table_blupi IDs
      28/29) but NOT wired: no "pop-star costume" mechanic/flag exists in this engine to select it.
- [x] BLUPI-049 — Bye (farewell exit animation) — **done 2026-07-17** (user-reported gap: teleporting
      into a hub world-select portal did nothing visually, unlike real mobile-eggbert's ~1.5s "turn
      and wave"). Verified directly against `Decor.cpp:6436`: `m_blupiPhase == Config::ScaleTime(30)`
      — 30 ticks at the real 20Hz reference rate = exactly 1.5s, a direct transcription (the
      `SCORE-013`/hub-system writeup's earlier "~1.5s" note was already precisely right). Real
      trigger scope is narrow — confirmed via `Decor.cpp:5476-5488`: Bye fires ONLY for
      `Decor::IsWorld()` (hub-screen world-select contact), never for the level-exit-reached/
      PauseBack/PauseRestart paths, which stay instant in real source too — so only the
      `WorldSelect1-12` portal-touch path in `GalaxyEggbertCnaGame.cpp`'s ground-tile check got this;
      `DemoPortal`/exit/PauseBack/PauseRestart are deliberately unchanged. New
      `GEBlupiController::TriggerBye()`/`IsBye()` (+ member timer, same "freeze everything, count a
      timer down, auto-resume" shape as `TriggerTeleport()`) and a new `AnimState::Bye` (falls
      through to the existing static Stop-pose in `GetAnimIcon()` — checked Tables.cpp/Decor.cpp
      directly, no dedicated per-frame "wave" sprite table exists in real source; the real
      "farewell" is genuinely just standing still, turned front-on, for 1.5s). The caller
      (`GalaxyEggbertCnaGame.cpp`'s world-select touch handler) calls `SetYaw()` once at trigger time
      to face the camera (real `m_blupiFront=true`) and plays the real entry sound (channel 32,
      `Decor.cpp:5480-5483`); `LoadMission()` itself now happens off a `wasBye`/`IsBye()`
      before/after-`Step()` completion check, the same detection shape already used for teleport-
      transit completion, instead of firing instantly on contact. Live-verified via a temporary
      debug harness (reverted before commit): confirmed the freeze holds for the correct ~1.5s
      duration, yaw updates to face the camera, and `LoadMission()` fires exactly once with the
      correct target the frame the freeze naturally elapses. Full regression clean (only the
      pre-existing unrelated `easy-gl-resource-smoke-tests` failure).
- [ ] BLUPI-050 — StopSuspend / MarchSuspend / TurnSuspend / JumpSuspend (rope hanging) — real data
      extracted (table_blupi IDs 31/32/33/34) but NOT wired: the underlying `m_suspended` mechanic
      itself is real code (`GEBlupiController.cpp:898`) but never actually triggered by
      `GalaxyEggbertCnaGame.cpp` (no rope-tile world-side detection exists) — genuinely unreachable in
      practice, matching the already-deferred Suspend mechanic itself (`177`/`BLUPI-101`, a pending
      icon-202 render-geometry decision the user asked to defer). Not worth animating a state that
      can never actually be entered; revisit once `BLUPI-101` is un-deferred.
- [x] BLUPI-051 — Hide (hiding in object) — **done 2026-07-18**, see `BLUPI-047`'s shared writeup —
      real single static pose (`table_blupi` ID 35, a 9-frame idle-fidget cycle) while
      `SecretPower::Hide` is active.
- [ ] BLUPI-052 — JumpAie (hurt jump on hazard contact) — real data extracted (table_blupi ID 36) but
      NOT wired: no distinct "hurt jump, not yet dead" state exists in this engine (hazard contact
      goes straight to `DeathLocked`) — would need its own trigger research, not attempted this pass.
- [x] BLUPI-053 — StopSkate / MarchSkate / TurnSkate / JumpSkate / AirSkate (skateboard) — **Stop/
      March done 2026-07-18; JumpSkate/AirSkate done 2026-07-19** (`vehicleAnimState()`'s
      `Skateboard` case now splits airborne by `m_velocityY` sign, same as the base `Jump`/`Air`
      split — the only vehicle mode with its own airborne icon pair; `table_blupi` actions 40/41,
      3/8 frames, extracted and cross-validated against the real table this session). Turn variant
      still NOT modeled (same Turn-variant gap as every other vehicle, see `BLUPI-047`'s own note).
- [x] BLUPI-054 — TakeSkate (picking up skateboard) — **done 2026-07-19**: real data (`table_blupi`
      ID 42, 20 frames) wired via `TriggerOneShotAnim()` at the real Skateboard mount hook point
      (`GalaxyEggbertCnaGame.cpp`'s vehicle-mount scan) — confirmed the only vehicle mode with a
      dedicated mount pose (checked the reference doc for the other 4 modes; none exist).
- [x] BLUPI-055 — DeposeSkate (putting down skateboard) — **done 2026-07-19**: real data
      (`table_blupi` ID 43, 20 frames) wired via `TriggerOneShotAnim()` at the real dismount hook
      point (`DismountAndDepositVehicle()`), same reasoning as `BLUPI-054`.
- [ ] BLUPI-056 — Ouf1a / Ouf1b / Ouf2 / Ouf3 / Ouf4 / Ouf5 (relief animations) — real data extracted
      (table_blupi IDs 44/45/46/47/48/65) but NOT wired: blocked on the same "idle-fidget
      system"/"close call" detection prerequisite as `BLUPI-018/019` (`SOUND-046`-family) — a genuinely
      separate, larger sub-system, out of scope this pass.
- [ ] BLUPI-057 — Sucette (collecting lollipop/suction-cup power-up) — already done via `PickupBusy`'s
      own per-kind frame table (`kSucetteFrames`, added 2026-07-16) — not a separate `AnimState`.
- [x] BLUPI-058 — StopTank / MarchTank / TurnTank / FireTank (tank vehicle) — **Stop/March done
      2026-07-18; FireTank done 2026-07-19**: real data (`table_blupi` ID 53, 6 frames) wired via a
      new `GEInteractionSystem::TankFiredThisFrame()` per-frame signal (mirrors the existing
      `CrateBeingPushedThisFrame()` pattern, since that class has no `GEBlupiController` access) —
      fires only the frame a bullet actually launches, not the empty-clip click; independent of the
      existing 0.5s fire cooldown, which only gates re-firing. Turn variant still NOT modeled, same
      gap as every other vehicle.
- [x] BLUPI-059 — Glu (stuck in glue) — already done via `DeathLocked`'s own per-cause frame table
      (`kGluFrames`, added 2026-07-16) — not a separate `AnimState`.
- [x] BLUPI-060 — Drink (drinking power-up animation) — already done via `PickupBusy`'s own per-kind
      frame table (`kDrinkFrames`, added 2026-07-16) — not a separate `AnimState`.
- [x] BLUPI-061 — Charge (being charged at by enemy) — already done via `PickupBusy`'s own per-kind
      frame table (`kChargeFrames`, added 2026-07-16) — not a separate `AnimState`.
- [ ] BLUPI-062 — Electro (electrocuted by electric field) — real data extracted (table_blupi ID 57)
      but NOT wired: no electric-field hazard mechanic exists in this engine yet (`BLUPI-149`/
      `SOUND` note, both channels also blocked on the same missing mechanic).
- [ ] BLUPI-063 — HelicoGlu (helicopter stuck in glue) — real data extracted (table_blupi ID 58) but
      NOT wired: no "helicopter stuck in glue" interaction exists in this engine.
- [ ] BLUPI-064 — TurnAir (turning while airborne) — real data extracted (table_blupi ID 59) but NOT
      wired, same Turn-variant detection gap as `BLUPI-047`'s own note (applies to the base humanoid
      too, not just vehicles).
- [ ] BLUPI-065 — StopMarch (decelerating from walk to stop) — real data extracted (table_blupi ID 60)
      but NOT wired: this engine's own movement model has no separate "decelerating" sub-state
      distinct from Stop (instant stop on released input, no coast-down blend frame).
- [ ] BLUPI-066 — StopJump / StopJumph (jump landing, high-jump landing) — real data extracted
      (table_blupi IDs 61/62) but NOT wired: no distinct "just landed" transition frame exists (this
      engine snaps straight from Jump/Air to Stop/March on landing).
- [x] BLUPI-067 — Mockery / Mockeryi / Mockeryp — **done 2026-07-18, and the label above was
      backwards** (user question: "does Blupi stick his tongue out at a nearby enemy, without
      anything happening to him?"). Verified directly against `Decor.cpp:9518-9601`
      (`MockeryDetect()`) and its caller (`Decor.cpp:5627-5636`): real source has BLUPI taunt a
      NEARBY enemy, not the reverse ("enemy mocking Blupi") — this session's own earlier note here
      had the direction wrong. Confirmed genuinely proximity-only (a bounding-box overlap check
      against `m_blupiPos`, NOT contact/collision), against a real, specific list of 11 enemy
      ObjectTypes (2/4/16/20/23/32/33/44/54/96/97 — all of which already exist as real, placed
      creatures in this engine), gated on Blupi being idle (`m_blupiAction==Stop`) and off a real
      300-tick (15s) cooldown (`m_blupiTimeMockery`) after the previous trigger. `ObjectType54`
      (large creature) always gets Mockeryp regardless of side; every other qualifying type picks
      Mockery (ahead of Blupi's facing) or Mockeryi (behind) — except `ObjectType2` (standard patrol
      enemy) specifically, which never gets the "ahead" Mockery variant (only Mockeryi when behind),
      ported as-is even though the real reasoning for that asymmetry isn't stated in source.
      Deliberately NOT modeled as a freeze (real source never drops `m_blupiFocus` for these, unlike
      Bye/Teleport/pickup-freeze) — new `GEBlupiController::TriggerMockery()` is cancelled the
      instant `moving` becomes true, matching that real "movement just overwrites the action away"
      behavior, with no explicit "cancel" needed. Real per-variant entry sound also verified
      directly: Mockery/Mockeryi both use ch65 (`Decor.cpp:3151/3165`), but Mockeryp uses a
      DIFFERENT channel, ch47 (`Decor.cpp:3179`) — not assumed from the other two. Proximity scan
      lives in `GalaxyEggbertCnaGame.cpp` (this class has no MobileObjects access), a natural 3D
      adaptation of the real ~60px (~1 tile at 64px/tile) box to a ~1-tile grid radius; real
      source's extra reach while airborne (`m_blupiAir`) is NOT modeled. Live-verified via a
      temporary debug harness (reverted before commit): standing near world999.vwr's own wasp
      (`ObjectType44`) correctly triggered `Mockery` (not `Mockeryi`/`Mockeryp`) with the exact real
      `263,264,265,264` icon cycle. Full regression clean.
- [x] BLUPI-068 — Balloon (balloon flight mode) — **stale checkbox, closed 2026-07-18** (found while
      auditing this whole section): already done, `AnimState::Balloon`/`kBalloonFrames`, added
      2026-07-11 (plan.md E3D-MIG-064).
- [x] BLUPI-069 — StopOver / MarchOver / TurnOver (flat/squashed mode) — **Stop/March done
      2026-07-18**, see `BLUPI-047`'s shared writeup (this is the real Overcraft vehicle mode,
      `VehicleMode::Overcraft`, NOT the Ecrase "crushed flat" state despite the similar name — those
      are two distinct real mechanics). Turn variant NOT modeled, same gap as every other vehicle.
- [ ] BLUPI-070 — Recedeq / Advanceq (quick backward/forward movement) — real data has NO
      `table_blupi` record at all (confirmed via the full-table extraction) — these 2 real
      `BlupiAction` values are never actually looked up by `Decor::BlupiSearchIcon()` in practice,
      genuinely dead/unreachable in the original game too, not just unmodeled here.
- [x] BLUPI-071 — StopEcrase / MarchEcrase (crushed under object) — **stale checkbox, closed
      2026-07-18**: already done, `AnimState::StopEcrase`/`MarchEcrase`, added 2026-07-11.
- [x] BLUPI-072 — Teleporte (teleporting animation) — **stale checkbox, closed 2026-07-18**: already
      done, `AnimState::Teleporting`/`kTeleportingFrames`, added 2026-07-11.
- [x] BLUPI-073 — Switch (activating a switch) — **done 2026-07-18**, see `BLUPI-047`'s shared
      writeup — real one-shot 0.5s animation (`table_blupi` ID 82) via the new
      `TriggerOneShotAnim()`/`IsOneShotAnimPlaying()` mechanism, wired at `TryActivateSwitch()`'s own
      success site (purely cosmetic on top of the switch already having activated — its own real
      sound, ch76/77, was already correctly wired).
- [ ] BLUPI-074 — Non (refusing / head-shake animation) — real data extracted (table_blupi ID 84) but
      NOT wired: no "refusal" trigger exists in this engine (real trigger condition unclear without
      further `Decor.cpp` research, not attempted this pass).
- [ ] BLUPI-075 — SlowdownSkate (skateboard braking) — real data extracted (table_blupi ID 85, a
      single static frame) but NOT wired: no "braking" input/state exists distinct from just
      decelerating normally.
- [x] BLUPI-076 — TakeDynamite / PutDynamite (dynamite pickup/place) — **PutDynamite done
      2026-07-18**, see `BLUPI-047`'s shared writeup — real one-shot 1.3s animation (`table_blupi` ID
      87) via `TriggerOneShotAnim()`, wired at `PlaceDynamite()`'s own success site. TakeDynamite's
      own real data (table_blupi ID 86) was extracted too but NOT wired: the real pickup goes through
      the deferred "Voyage" reward system (`RequestVoyage()`), not an immediate edge event, and it was
      unclear whether the real hand-gesture belongs at touch-time or at Voyage-resolution time without
      further research — left for a future pass rather than guessing.

#### 4.3 Blupi Sprite Animation

- [x] BLUPI-077 — Frame tables from `table_blupi` (2911 entries) — **extracted in full 2026-07-18**: a
      small parser (mirroring `Decor.cpp:2393-2400`'s own record-scan exactly) walked the REAL,
      complete `Tables::table_blupi[2911]` literal and recovered all 84 named `BlupiAction` records
      that actually have one (3 of the 87 real enum values — Set/Recedeq/Advanceq — genuinely have no
      table_blupi record at all, confirmed dead/unreachable in the original game too). Of those 84,
      ~24 are now wired into `GEBlupiController`'s `AnimState` system (see `BLUPI-047`'s shared
      writeup); the rest have their real data sitting ready in the extraction but no live trigger yet
      (each one's own entry above says exactly why, mostly "no matching mechanic/edge-event exists in
      this engine").
- [ ] BLUPI-078 — Billboard sprite from `blupi.png` (60×60 px cells) — CNA has no visible first-person Blupi yet; only a temporary 2D sprite HUD indicator / optional third-person placeholder model
- [x] BLUPI-079 — Direction flipping: mirror sprite when moving right (table_mirror) — **wired
      2026-07-20, approximated by user decision**: confirmed directly against `Decor.cpp:2412-2444`
      that `table_mirror` isn't a cosmetic nicety -- it's the mechanism that makes EVERY
      `table_blupi` icon this session has been transcribing correct for `Direction::Left`
      specifically (the base data is right-facing by convention). The blocker: this engine has no
      discrete left/right facing concept at all (`GEBlupiController` only tracks a continuous 3D
      `m_yaw`) -- asked the user whether to defer until the real 3D Blupi model decision, or
      approximate now for the temporary debug icon; user chose to approximate now. New
      `GEBlupiController::GetDisplayAnimIcon()` wraps `GetAnimIcon()`: `std::sin(m_yaw) < 0.0f`
      stands in for `Direction::Left` (same sign convention already established for
      `blupiFacingDX`/`GalaxyEggbertCnaGame.cpp:2501`), `m_invert` flips the sense exactly like the
      real `m_blupiInvert` twist, the real table_mirror[335] substitution applies on the blupi.png
      channel, and the real +4 special case applies for element.png icons 168-171 (confirmed this
      is exactly `DeathCause::Glu`'s own real frame range). Deliberately does NOT model the 3
      `StopSuspend`-specific remaps in the same real block -- that `AnimState` doesn't exist here
      (`BLUPI-101`). Only the one HUD call site was updated (`GetAnimIcon()` itself, and all its
      existing ~100 unit-test call sites, are untouched -- they only ever exercise the default
      yaw=0 case, where no mirroring applies either way, so nothing broke). 6 new
      `VerifyBlupiMovement` assertions (unmirrored-right, mirrored-left via the real
      `table_mirror[17]==20`, Invert flipping the sense back, and the Glu element.png +4 case).
- [x] BLUPI-080 — All 87 BlupiAction frames resolved from table_blupi via action+phase+dir lookup —
      **partially done, see `BLUPI-077`**: extraction is complete for all 84 real records; "+dir"
      (left/right mirroring) is now approximated too, see `BLUPI-079`. Live-trigger coverage (~24
      states wired at the start of this session, +7 enemy icon families and 2 sound cues added
      since) is tracked separately per-`AnimState`, not by this single line item.
- [ ] BLUPI-081 — `blupi1.png` alternate skin channel (Blupi1_11/12/13 variants for ObjectType200-203)
- [ ] BLUPI-082 — Shield tint: cyan/blue sprite overlay when m_blupiShield active
- [ ] BLUPI-083 — Shield blink at < 1.5 s remaining (blink 10 Hz)

#### 4.4 Vehicle Modes (each = new movement model + sprite sheet section)

- [x] BLUPI-084 — Helicopter mode (m_blupiHelico): 8-direction flight, no gravity, propeller
      sound loop (ch16/ch18) — **stale checkbox, closed 2026-07-17**: flight mechanic done via
      Phase 17 `171` (free vertical flight, no gravity, reusing crouch/lookUp for ascend/descend
      — this engine's tank-control scheme adapts the real 2-axis flight to forward/back+turn
      instead of a literal 8-direction free-move, a documented adaptation not a gap); motor sound
      now done too, see `SOUND-008`.
- [x] BLUPI-085 — Helicopter boarding: touch ObjectType13 → sets m_blupiHelico, removes object —
      **stale checkbox, closed 2026-07-17**: done via `171`'s real pickup->vehicle mapping.
- [x] BLUPI-086 — Helicopter dismount: reverts to normal mode, drops the vehicle pickup back into
      the world — **resolved 2026-07-20**: the task's own "press Down" title was wrong, unverified
      from the start (no research citation ever backed it). Confirmed directly against
      `Decor.cpp:3889` — real Helicopter voluntary dismount checks
      `getButtonPressedProperty() == Def::ButtonGlyph::PlayAction`, the exact same shared action
      button every other vehicle mode uses (Jeep/Tank/Skateboard/Overcraft all follow the
      identical pattern at their own `ObjectStart(..., ObjectType19/28/24/46, ...)` sites nearby).
      No per-vehicle "Down" dismount key exists anywhere in real source. This engine's existing
      shared-action-button dismount (`171`'s `TriggerDismount()`) was already correct; no code
      change needed.
- [ ] BLUPI-087 — Helicopter destroyed by creature (ObjectType54): ByeByeHelico debris effect —
      still correctly NOT modeled, see `VISUAL-018`/`156` (needs a particle/fragment rendering
      system that doesn't exist yet).
- [x] BLUPI-088 — Jeep mode (m_blupiJeep): horizontal drive, jump, carry Blupi — **stale
      checkbox, closed 2026-07-17**: done via `171`/`178` (ground-gravity movement path,
      per-mode max speed/accel table).
- [x] BLUPI-089 — Jeep boarding: touch ObjectType19 → sets m_blupiJeep — **stale checkbox, closed
      2026-07-17**: done via `171`'s real pickup->vehicle mapping.
- [x] BLUPI-090 — Jeep motor sound loop (ch29/ch31, high/low pitch via m_blupiMotorHigh) —
      **done 2026-07-17**, see `SOUND-008`.
- [x] BLUPI-091 — Tank mode (m_blupiTank): drive + fire projectiles (ch FireTank) — **stale
      checkbox, closed 2026-07-17**: drive done via `171`/`178`; firing done via `BULLET-001`.
- [x] BLUPI-092 — Tank boarding: touch ObjectType28 → sets m_blupiTank — **stale checkbox, closed
      2026-07-17**: done via `171`'s real pickup->vehicle mapping.
- [x] BLUPI-093 — Tank fire sound (ch 53) on FireTank animation frames — **stale checkbox
      closed, but this entry's channel claim is WRONG, corrected 2026-07-17**: per `BULLET-001`'s
      own research, ch53 is actually the real OUT-OF-AMMO click, not the fire sound — the real
      fire sound is ch52 (shared with dynamite placement/explosion, `SOUND-062`). Both are
      already correctly wired in `BULLET-001`'s firing implementation; only this entry's channel
      label was wrong.
- [x] BLUPI-094 — Skateboard mode (m_blupiSkate): faster horizontal, higher jump — **stale
      checkbox, closed 2026-07-17**: done via `171`/`178` (own `kSkateboardJumpSpeed`, reuses the
      shared ground gravity/jump path per real source's own note).
- [x] BLUPI-095 — Skateboard pickup: touch ObjectType24 → TakeSkate animation → sets m_blupiSkate
      — **stale checkbox, closed 2026-07-17**: the MOUNT mechanic is done via `171`'s real
      pickup->vehicle mapping — only the `TakeSkate` ANIMATION-STATE selection remains blocked on
      the missing visible-Blupi-model work, same as `BLUPI-116`/`120` above.
- [x] BLUPI-096 — Balloon mode (m_blupiOver/m_blupiBalloon): float up/down, limited horizontal —
      **stale checkbox, closed 2026-07-17, and note the real "Balloon" naming trap already flagged
      elsewhere**: `ObjectType46` actually grants Overcraft (`m_blupiOver`), not a separate
      Balloon ride (see `171`'s own correction of `ObjectType.hpp`'s misleading doc comment) —
      Overcraft's free vertical flight is done via `171` (shares the Helicopter flight branch).
      The genuinely separate wasp-sting "Balloon" status effect (`m_blupiBalloon`) is a different
      mechanic, also already implemented (`TriggerBalloon()`/`IsBallooned()`).
- [x] BLUPI-097 — Balloon pickup: touch ObjectType46 → sets m_blupiOver — **stale checkbox,
      closed 2026-07-17**: done via `171`'s real pickup->vehicle mapping (grants Overcraft, per
      the naming-trap note on `BLUPI-096` above).
- [x] BLUPI-098 — Swimming (m_blupiNage): entered when Blupi falls into water
      (`table_vitesse_nage`) — **stale checkbox, closed 2026-07-17**: done via Phase 14 `148`
      (`GEBlupiController::IsNage()`).
- [x] BLUPI-099 — Surfing (m_blupiSurf): surfboard on water surface (`table_vitesse_surf`) —
      **stale checkbox, closed 2026-07-17**: done via Phase 14 `148` (`GEBlupiController::IsSurf()`).
- [ ] BLUPI-100 — Vent (fan) propulsion (m_blupiVent): blown by ventilator tile — **researched
      2026-07-17, genuinely NOT modeled**: grepped both `GEBlupiController.cpp` and
      `GalaxyEggbertCnaGame.cpp` for any Fan-tile physics interaction — none exists; Fan
      tiles (`BlockTypes::FanLeft/Right/Up/Down`) are purely a visual terrain animation
      (`GETerrainRenderer.cpp`), with no effect on Blupi's movement at all. A genuine, real,
      still-open gap — correctly left unchecked, not stale. **NOT the same thing as the Fan
      hazard/`IsVentillo` mechanic already implemented (Phase 14 `149`)** — confirmed
      2026-07-20 (re-checked while a research fork mistakenly proposed this as a "next task",
      having only read this stale-sounding checklist line without cross-checking the actual
      already-shipped Fan hazard code first): `m_blupiVent` is a genuinely separate real field
      (`Decor.hpp:358`), set by standing on icons 110-125 specifically (`Decor.cpp:2796-2798`,
      a continuous per-tick push: 9px/tick horizontal for 110/114, 20px/tick vertical for
      118/122, while not in Jeep/Tank/Skate), NOT by the fan HEAD icons 126-137 `IsVentillo`
      already handles. Blocked on the exact same prerequisite as `149`'s own already-documented
      gap (its trail-chain-walk, which reads these same icons 110-125 but doesn't act on them
      either): these icons have no `BlockTypes` constant, no render-geometry decision, and are
      never placed in any level this project has inspected yet (`mobile-eggbert-reference/
      12-hazards-and-interactables.md`'s own "Fan tiles" section, last paragraph). Implementing
      the propulsion physics alone, with no tile family to trigger it and nothing to test it
      against, would mean either inventing placement data or leaving it permanently unexercised
      — needs the same user render-geometry decision as `AscenseurVertigo`/`177` before this is
      a real, verifiable task, not a per-mechanic gap to just go implement.
- [ ] BLUPI-101 — Suspend (rope hang): entered when Blupi grabs a rope tile — still correctly
      blocked, see `177`'s own research (real, fully-speced, cheap mechanic, but blocked on the
      pending icon-202 render-geometry decision the user asked to defer this session).
- [x] BLUPI-102 — Motor sound crossfade: one-shot start/stop sounds + looped motor sound —
      **done 2026-07-17**, see `SOUND-008`.
- [x] BLUPI-103 — m_blupiMotorHigh: pitch variant selection (fast vs slow motor) — **done
      2026-07-17**, see `SOUND-008` (`GEBlupiController::IsVehicleMotorHigh()`).

#### 4.5 Blupi Special States & Power-ups

- [x] BLUPI-104 — Shield (m_blupiShield): 5 s invincibility from ObjectType25; bypasses all hazards
      — **stale checkbox, closed 2026-07-17**: fully done via Phase 17 `170`
      (`GEBlupiController::SecretPower::Shield`/`IsInvincible()`, `ObjectType25` grant). This
      entry's "5s" figure is superseded by `172`'s more careful research (real 0.25s/level, 25s
      total) — not a separate gap, just an older/less precise duration claim.
- [x] BLUPI-105 — Shield timer (m_blupiTimeShield): counts down 0→100 ticks; gauge shows progress
      — **stale checkbox, closed 2026-07-17**: done via `m_secretPowerTimer`/`m_secretPowerLevel`
      (Phase 17 `172`) + the shared HUD gauge (`HUD-008`).
- [x] BLUPI-106 — Shield trail sparkle (ObjectType57 spawned while shield active) — **stale
      checkbox, closed 2026-07-17**: done — see the `PICKUP-086/087` cancellation note for the
      full citation (`GEInteractionSystem::TickMagicTrail()`/`ResetMagicTrail()`, `kShieldTrack`).
- [x] BLUPI-107 — SuperBlupi (m_bSuperBlupi): cheat mode, full invincibility + all powers —
      **stale checkbox, closed 2026-07-17**: done, see `MENU-095`/`CHEAT-002`
      (`GEBlupiController::m_cheatSuperBlupi`/`SetCheatSuperBlupi()`, folded into `IsInvincible()`).
- [x] BLUPI-108 — ~~Cloud mode (m_blupiCloud): from ObjectType31, floats through blocks for N
      ticks~~ **HALLUCINATED — CANCELLED (confirmed by user 2026-07-14)**: grepped real
      `Decor.cpp` for every `m_blupiCloud` reference — none gate any collision/movement-bypass
      function; Cloud's actual real gameplay effect is the `BlupiElectro` enemy-kill aura
      (destroys small enemies within a real 40px radius while active), already implemented — see
      item `068` and `PICKUP-010` above for the full citation. The "floats through blocks" premise
      is not supported by any source and is confirmed invented — do not implement it under any
      item number.
- [x] BLUPI-109 — Invert mode (m_blupiInvert): from ObjectType40, inverted controls for 100 ticks
      — **stale checkbox, closed 2026-07-17**: the actual control-inversion IS implemented (real
      `Decor::SetSpeedX`'s "if (m_blupiInvert) speed = -speed", `GEBlupiController::Step()`'s
      `m_invert` branch negating `horizontalSpeed`) — see `PICKUP-011` for the full grant/timer
      citation. Only `BLUPI-110`'s particle-burst half was previously marked done; this entry
      (the actual gameplay effect) was the still-unmarked duplicate.
- [x] BLUPI-110 — Invert start/stop particle burst (ObjectType41/42 in 4 directions) — **done
      2026-07-14, see `plan.md` VISUAL-014/015 for the full implementation writeup.**
- [x] BLUPI-111 — Ghost mode (m_blupiGhost): cheat, passes through walls, no interactions —
      **done 2026-07-14**. Initially researched as blocked (the on-screen button-gesture cheat
      dispatch, `Game1.cpp`'s `CheatAction()`, never maps any real cheat-number/gesture to
      `CheatCodes::Ghost` — only 7 of the ~26 real cheats are reachable that way). The user then
      provided the missing piece directly: real mobile-eggbert has a SECOND, independent
      cheat-entry method — confirmed via direct source read at `InputPad.cpp:686-753` — a rolling
      lowercase-letter buffer (A-Z keys, capped at 32 chars, built only during real `Phase::Play`,
      appending on each key's down-EDGE not held-repeat) whose own SUFFIX is matched against a
      real ~26-entry cheat-name table (`"ghost"` among them, `#ifdef MODERN` which is
      unconditionally live) to call `Decor::CheatAction()`. Ported as
      `GEInputPad::UpdateTypedGhostCheat()` (only `"ghost"` wired for now — the other ~25 real
      names each need their own individual verification pass before wiring, a separate future
      task, not a blind mass-port). Movement: real `BlupiGhostStep()` (`Decor.cpp:2639-2705`) is a
      genuinely separate top-priority early-return (checked before even teleporting) — free flight
      at a real exact 4x normal speed, no gravity, no collision, world-bounds clamp only; ported as
      `GEBlupiController::Step()`'s own top-priority `m_ghost` branch (`kGhostSpeed = kMoveSpeed *
      4.0f`), adapting the real 2 independent screen axes to this engine's tank-control scheme
      (forward/back-along-yaw + turn for horizontal, reusing jumpPressed/crouchHeld for vertical
      flight since this engine has no other real-mapped use for them while every other Step()
      branch is skipped). "No interactions": real `MoveObjectDetect()` unconditionally returns "no
      object found" while ghosting, which nearly every hazard/pickup/enemy-contact/lift-riding
      check in `GEInteractionSystem` is built on (all proximity tests against Blupi's own
      position) — ported by substituting a sentinel far-outside-the-world position for Blupi's
      real coordinates in the `GEInteractionSystem::Update()` call site whenever
      `GEBlupiController::IsGhost()`, making every proximity check fail shut with zero changes
      inside `GEInteractionSystem` itself; patrol/animation logic (which never references Blupi's
      position) continues unaffected, matching real behavior. Toggle-on clears any active vehicle
      mount (real behavior); toggle-off is silently rejected while standing inside solid geometry
      (real `!DecorDetect(BlupiRect(m_blupiPos))`, `Decor.cpp:2065`) rather than stranding Blupi
      mid-wall. NOT modeled: the real 0.75-opacity billboard tint while ghosting (no consistent
      visible Blupi sprite exists yet to tint — same documented gap as every other Blupi-visual
      simplification this session). 12 new `VerifyBlupiMovement` checks (toggle/state, vehicle
      clearing, no-gravity float, vertical flight both directions, collision bypass, real 4x speed,
      toggle-off in open air vs. rejected inside solid geometry) + 4 new `VerifyGEInputPad` checks
      (letter-by-letter typing, Play-phase gate, held-key-doesn't-repeat, suffix-match-through-a-
      prefix) + full regression on both backends, all pass.
- [x] BLUPI-112 — Hide mode (m_blupiHide): concealed in object — **stale checkbox, closed
      2026-07-17**: done via Phase 17 `170` (`SecretPower::Hide`, `ObjectType30` grant,
      `IsInvincible()`).
- [x] BLUPI-113 — Sucette/suction-cup (m_blupiPower): from ObjectType26, walk up walls — **stale
      checkbox, closed 2026-07-17, and this entry's "walk up walls" premise is wrong**: verified
      directly against every real `m_blupiPower` reference in `Decor.cpp` — all of them are jump-
      velocity boosts (e.g. `-25` vs `-19`), none reference wall-climbing/collision-bypass; "walk
      up walls" doesn't appear to be real. The actual real effect (higher jump while Power is
      active) IS done — `kJumpSpeedPowered`/`kSkateboardJumpSpeedPowered` etc. throughout
      `GEBlupiController.cpp`, granted via `173`. Do not implement wall-climbing under this item.
- [x] BLUPI-114 — Dynamite (m_blupiDynamite): from ObjectType55; TakeDynamite / PutDynamite actions
      — **stale checkbox, closed 2026-07-17**: fully done, see Phase 15 `155`.
- [x] BLUPI-115 — Bullet count (m_blupiBullet): from ObjectType29; FireTank expends bullets — **firing done 2026-07-13, plan.md `BULLET-001`** (ammo pickup itself already done 2026-07-12, E3D-MIG-175); see `BULLET-001`'s own entry (§ Bullets) for the full real-behavior citation and what's NOT modeled (Helicopter firing, enemy damage -- there is none, real bullets are a hazard not a weapon)
- [x] BLUPI-116 — Ecrase mode (m_blupiEcrase): crushed flat under object (StopEcrase/MarchEcrase)
      — **stale checkbox, closed 2026-07-17**: the mechanic itself is done
      (`GEBlupiController::TriggerCrush()`, a duration-timed crushed-flat state, wired from the
      Crusher hazard in `GalaxyEggbertCnaGame.cpp`) — only the `StopEcrase`/`MarchEcrase`
      ANIMATION-STATE selection remains blocked on the missing visible-Blupi-model work, same as
      every other `BlupiAction` animation item.
- [x] BLUPI-117 — m_blupiPerso: persona counter (shown in HUD as button icon 108 + count) —
      **stale checkbox, closed 2026-07-17**: fully done, see `HUD-017`.

#### 4.6 Blupi Death & Respawn

- [x] BLUPI-118 — Death: BlupiDead() triggers explosion effect, resets lives-1, respawn — **stale
      checkbox, closed 2026-07-17**: fully done via `GEBlupiController::TriggerDeathLock()` +
      `GEInteractionSystem::LoseLife()` (the "explosion effect" is the death-cause-specific
      hurt-sprite/pickup-freeze frame array, `DeathCause`-keyed, see the session's earlier
      `kClear1Frames`/etc. writeup).
- [x] BLUPI-119 — Death freeze: 1 s input lock before respawn — **stale checkbox, closed
      2026-07-17**: done, though more precisely than "1s" — real per-cause duration
      (`kDeathLockTicks[cause]`), not a flat 1-second lock.
- [x] BLUPI-120 — Drown death: different animation (ACTION_DROWN) in deep water — **stale
      checkbox, closed 2026-07-17**: the death-CAUSE distinction is done
      (`GEBlupiController::JustDrowned()`/`DeathCause::Drown`, channel 26, Phase 14 `E3D-MIG-148`)
      — only the `ACTION_DROWN` ANIMATION-STATE selection remains blocked on the missing visible-
      Blupi-model work, same as `BLUPI-116` above.
- [ ] BLUPI-121 — Electro death: ACTION_ELECTRO animation + electric shake
- [ ] BLUPI-122 — Glu death: Blupi stuck (ACTION_GLU) for several frames then die
- [ ] BLUPI-123 — Charge death: enemy charge-hit animation (ACTION_CHARGE)
- [ ] BLUPI-124 — Ouf recovery: after close call, play one of Ouf1a..Ouf5 animations
- [ ] BLUPI-125 — Mockery: enemies mock Blupi (ACTION_MOCKERY/i/p) for m_blupiTimeMockery ticks
- [ ] BLUPI-126 — ~~Stomp kill: velY < -1.0 on contact with enemy → BounceUp() + kill enemy~~
      **HALLUCINATED — CANCELLED (confirmed by user 2026-07-14)** — a Mario-style "jump on an
      enemy to kill it" mechanic. Grepped all of `mobile-eggbert/src/WindowsPhoneSpeedyBlupi/` and
      `include/WindowsPhoneSpeedyBlupi/` for `bounce`/`Bounce`/`stomp`/`Stomp` (case-insensitive) —
      zero matches for any such function or concept; `BounceUp()` does not exist anywhere in
      source. Also checked `MoveObjectStep()`'s own real enemy-contact code
      (`Decor.cpp:7940-7965`, the exact function whose own doc comment says enemy types 4/32/33
      "electrocute Blupi" on contact) and every other Type4/32/33 contact site — none has any
      velocity/vertical-speed condition gating a different outcome. Confirmed invented — do not
      implement. The real, confirmed side of this (enemy-contact death, no stomp exception) is
      just the shared hazard-contact kill logic already covered by `IsGenericHazard()`
      (galaxy-eggbert's own `GEInteractionSystem.cpp`) — already implemented, see the correction
      below (ENEMY-CONTACT-001 was a false claim, retracted).
- [ ] BLUPI-127 — ~~BounceUp: upward impulse kJumpSpeed × 0.65~~ **HALLUCINATED — CANCELLED, same
      as BLUPI-126 above — no such function exists in real source.**
- ~~ENEMY-CONTACT-001 — Blupi has NO contact-collision detection against enemy `MoveObject`s at
      all yet~~ **RETRACTED 2026-07-14, this claim was itself wrong** — direct re-verification of
      `GEInteractionSystem.cpp` found Blupi-enemy contact detection is already comprehensive and
      source-confirmed: the shared 8-type hazard-contact kill list (`IsGenericHazard()`, types
      2/3/4/16/17/20/96/97, line ~1203), wasp balloon-touch (type 44, line ~1129), large-creature
      turn-dwell lethal contact (type 54, line ~1167), fired-projectile contact (type 23, line
      ~1107), and blupih/blupit's own body (32/33) CORRECTLY excluded from any contact-kill (real
      `Decor.cpp` only ever harms Blupi via their fired projectile, never their body — already
      documented in place at line ~1090). Also checked `ObjectType18` (absent from
      `IsGenericHazard()`) against real source: it has exactly one reference in all of `Decor.cpp`
      (the dynamite-blast destructible list, already mirrored in galaxy-eggbert) and no confirmed
      contact-damage behavior anywhere — correctly excluded, not a gap. No remaining real
      Blupi-enemy contact gap was found. (Meta-note: this whole ENEMY-CONTACT-001 item was added by
      a research fork earlier the same session and not independently re-verified before being
      committed — a "trust but verify" lesson, same category as this session's earlier false-negative
      sound-wiring audit finding.)

#### 4.7 Blupi Sounds

CNA's `GESound` loads all 93 real .wav files with the real per-channel volume/conflict table and
is wired to jump/land/footstep events, so the base plumbing exists — but per-item wiring below is
re-verified individually since it is not a full port yet.

- [x] BLUPI-128 — Jump sound: ch1 on jump (CNA, 2026-07-10)
- [x] BLUPI-129 — Footstep sound: ch3 per march stride (CNA, 2026-07-10 — plain footstep wiring only; surface-dependent remap NOT done, see BLUPI-133)
- [x] BLUPI-130 — Landing sound: ch4 on ground contact (CNA, 2026-07-10)
- [ ] BLUPI-131 — ~~Stomp kill sound: ch5~~ **HALLUCINATED — CANCELLED, same as BLUPI-126.**
- [x] BLUPI-132 — Death sound: ch8 — **stale checkbox, closed 2026-07-17**: done, see
      `SOUND-018` (already wired across Lava/Blitz/etc.).
- [x] BLUPI-133 — Surface-specific footstep: SoundEnviron maps ch3/ch4 to ch78-91 based on tile
      type — **stale checkbox, closed 2026-07-17**: fully done and tested
      (`GESound::FootstepChannelFor()`, `E3D-MIG-084`), wired via `sound_.PlayStep()`/`PlayLand()`
      in `GalaxyEggbertCnaGame.cpp`.
- [ ] BLUPI-134 — Walk in water sound: ch36 (shallow water ambient) — still correctly blocked,
      see `SOUND-046` (idle-fidget `AnimState` gap, same as every other `SOUND-010c`-family item).
- [ ] BLUPI-135 — Swim bubble sound: ch37 — still correctly blocked, see `SOUND-047` (same
      idle-fidget gap as `BLUPI-134`). Not to be confused with the real ambient water bubble
      (`BLUPI-151`/ch24, a different, already-implemented mechanic).
- [x] BLUPI-136 — Glide sound: ch41 — **stale checkbox closed 2026-07-17, "Glide" label is
      wrong**: no glide mechanic exists (see `BLUPI-005`'s own cancellation) — real ch41 is the
      wasp balloon-status recovery/expiry cue (shared with Crusher's own recovery), already wired
      (`SOUND-051`, `ENEMY-039`).
- [x] BLUPI-137 — Teleport in/out sounds: ch9 / ch12 — **stale checkbox, closed 2026-07-17**:
      done, see `SOUND-019`/`SOUND-022`.
- [x] BLUPI-138 — Shield-on sound: ch50 when shield activated — **stale checkbox closed
      2026-07-17, channel claim is wrong**: real ch50 is the Sucette pickup-start ("grab") sound
      (`SOUND-060`, `173`), not shield-related at all — Shield's own real activate sound is a
      different channel (see `PICKUP-073`, ch42). Already correctly wired under its real meaning.
- [x] BLUPI-139 — Shield-off sound: ch44 when shield expires — **stale checkbox closed
      2026-07-17, channel claim is wrong**: real ch44 is the Sucette/Power 2-stage-delay
      COMPLETION sound (`SOUND-054`, `173`), not shield-off — no distinct "shield expires" sound
      was found in real source. Already correctly wired under its real meaning.
- [x] BLUPI-140 — Dynamite pickup/place sounds: ch52 — **stale checkbox, closed 2026-07-17**:
      done, see `PICKUP-082`/`155`.
- [x] BLUPI-141 — Tank fire sound: ch53 — **stale checkbox closed 2026-07-17, channel claim is
      wrong, same correction as `BLUPI-093` (exact duplicate entry)**: ch53 is the real
      out-of-ammo click, not the fire sound (that's ch52, shared with dynamite) — both already
      correctly wired in `BULLET-001`.
- [x] BLUPI-142 — Switch activate: ch76/ch77 (off/on) — **stale checkbox, closed 2026-07-17**:
      done, see `PICKUP-076`.
- [ ] BLUPI-143 — Rope suspend sounds: ch47 (attach), ch65 (detach) — still correctly NOT
      modeled: the Suspend mechanic itself is blocked (`177`/`BLUPI-101`, deferred render
      decision), and both channel labels are independently wrong anyway (ch47 is an idle-fidget
      channel per `SOUND-057`; ch65 is the real Mockery idle-taunt sound per `SOUND-075`, not
      "detach").
- [x] BLUPI-144 — Drink sound: ch58 on Drink action — **stale checkbox closed 2026-07-17,
      channel claim is wrong**: ch58 is actually Charge's real immediate "grab" sound (`173`) —
      Drink's real immediate grab sound is ch57 (`SOUND-067`). Already correctly wired under its
      real meaning.
- [x] BLUPI-145 — Key pickup sound: ch11 — **stale checkbox, closed 2026-07-17**: done, see
      `PICKUP-004/005/006`/`PICKUP-071` (already noted here as wired, just never checked off).
- [ ] BLUPI-146 — Life / egg pickup sound: ch42 — left as `[ ]`, own note already correctly
      flags the ch42 claim as needing re-verification (real egg sound is ch3, see `PICKUP-072`);
      ch42 itself is the real Shield activate sound (`PICKUP-073`), unrelated to eggs.
- [x] BLUPI-147 — Sucette pickup sound: ch62 — **stale checkbox closed 2026-07-17, channel claim
      is wrong**: ch62 is actually Drink's real 2-stage-delay completion sound (`SOUND-072`) —
      Sucette's real completion sound is ch44 (`SOUND-054`, see the `BLUPI-139` correction above
      too). Already correctly wired under its real meaning.
- [x] BLUPI-148 — Balloon motor sounds: ch28/ch30 (start/stop), ch29/ch31 (loop low/high) —
      **done 2026-07-17, and this entry's "Balloon" label is wrong**: these are the real Jeep/
      Tank/Overcraft motor sounds (`SOUND-008`), not Balloon-specific — see `BLUPI-096`'s note on
      the same naming trap (`ObjectType46` grants Overcraft, not a separate Balloon ride).
- [ ] BLUPI-149 — Electro sounds: ch38 (long arc) / ch90 (spark) — **researched 2026-07-17, both
      channel claims are wrong, likely an invented entry**: ch38 is the real crate-push loop
      sound (`SOUND-048`, done, unrelated to Electro), and ch90 is one of the real
      `SoundEnviron()` obstacle-range footstep/landing remap channels (`BLUPI-133`, icons
      107-109's landing variant), not an "electro spark" sound. No genuine Electro-specific sound
      channel was found in this pass; whether `BlupiAction::Electro` has any real dedicated sound
      at all is unresearched — left open rather than guessed.
- [x] BLUPI-150 — Glu splash sounds: ch51 — **stale checkbox closed 2026-07-17, channel claim is
      wrong**: real ch51 is the generic hazard-contact death sound (`PICKUP-081`/`SOUND-061`), no
      glue-specific sound exists.
- [x] BLUPI-151 — Water splash sounds: ch23 (small plouf), ch64 (tiplouf), ch24 (blup bubble) —
      **done 2026-07-17**, see `SOUND-032/033/034/074`/`PICKUP-078/079/080` for the full writeup.
- [x] BLUPI-152 — Secret exit found sound: ch21 — **stale checkbox closed 2026-07-17, this is a
      duplicate of `PICKUP-083`'s own already-corrected finding**: no dedicated ch21 secret-exit
      sound exists — reuses the regular exit's win (ch14)/reject (ch13) sounds. Real ch21 is
      actually the look-up transition sound (`SOUND-031`).
- [x] BLUPI-153 — Door open sound: ch7 — **stale checkbox closed 2026-07-17, channel claim is
      wrong**: real door-open channel is ch33 (`PICKUP-041/075`), already wired — ch7 is actually
      the descend-start sound (`SOUND-017`).

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
- [x] TILE-006 — Level progression: win → next world, wraps at world 5 —
      **the real progression MECHANISM is now done 2026-07-17**, see the
      hub/mission-progression system writeup at `SCORE-013`-`019`. "Wraps
      at world 5" specifically is not modeled — this session only
      authored world 1's own hub+2 sublevels (`worlds3d/world010/011/
      012.vwr`) as a minimal proof of the mechanism, per explicit user
      scope (worlds 2-5's full content is a separate, larger content-
      authoring task, `TILE-005`, not attempted here).
- [ ] TILE-007 — Terrain depth fill (cliff edges extrude dark fill blocks downward) *(3D)*
- [ ] TILE-008 — Sky dome per world (`backgrounds/decorNNN.png`)
- [ ] TILE-009 — Per-world sky colour (ambient + fog)
- [x] TILE-010 — Blupi spawn position parsed from world data (CNA, 2026-07-10 — collision point spawn only, no visible Blupi, see BLUPI-078)
- [x] TILE-011 — Background texture selection by region — **done differently** (**corrected 2026-07-14**): not a mobile-eggbert-style text `region=` header line (this engine's `.vwr` format is different), but the `.vwr` v2 format's own `skyRegion` binary header field, read via `GEWorldRuntime::GetSkyRegion()` and used to pick `Content/backgrounds/decorNNN.png` directly (`GalaxyEggbertCnaGame::LoadContent()`) — same real end result (per-world background selection), different real source field.
- [ ] TILE-012 — `music=` header parsed → ambient music track

#### 5.2 Animated Tiles

CNA uses real per-type divisors from `Decor.cpp Config::ScaleDiv()`, not a uniform tick rate:
Saw/Fan div 1 (50ms), Lava div 2 (100ms), Water1/Crusher/Water2/Marine div 3 (150ms), Spike/Temp
div 4 (200ms). Any task below that previously assumed a uniform "6 fps" animation rate is corrected
accordingly.

- [x] TILE-013 — Per-type animation phase timing using real `ScaleDiv()` divisors, not a uniform tick counter (CNA, 2026-07-10 — corrected from the old uniform-6fps assumption)
- [x] TILE-014 — Lava tiles (icon 68, 8-frame: {68,69,70,71,72,71,70,69}), div 2 / 100ms (CNA, 2026-07-10) — kill-on-contact **also done** (**corrected 2026-07-14**, stale note said "blocked on hazard/lives system": real hazard-contact check is in `GalaxyEggbertCnaGame.cpp`, not `GEInteractionSystem.cpp` — `GetGroundBlockType()==Lava` gate, confirmed live, all 5 real terrain hazards are working as of 2026-07-11/12).
- [x] TILE-015 — Crusher tiles (10-frame: {317..323...}), div 3 / 150ms (CNA, 2026-07-10) — kill-in-frames hazard logic **also done** (**corrected 2026-07-14**, same false-negative as TILE-014 — `GalaxyEggbertCnaGame.cpp`'s `GetGroundBlockType()==Crusher` gate).
- [x] TILE-016 — Saw tiles (6-frame: {378..383}), div 1 / 50ms (CNA, 2026-07-10) — kill-on-contact **also done** (**corrected 2026-07-14**, same false-negative — `GetGroundBlockType()==Saw` gate, plus the real switch/saw 41-cell linking, `GEWorldRuntime::TryActivateSwitch()`).
- [x] TILE-017 — Spike tiles (16-frame: table_decor_piege1), div 4 / 200ms (CNA, 2026-07-10) — kill-on-contact **also done** (**corrected 2026-07-14**, same false-negative — `GetGroundBlockType()==Spike` gate).
- [x] TILE-018 — Water1 tiles (6-frame: {92..95,94,93}), div 3 / 150ms (CNA, 2026-07-10 — animated decoration, alpha-blended cube, not wavy-edge surface)
- [x] TILE-019 — Water2 tiles (6-frame: {91,96..98,97,96}), div 3 / 150ms (CNA, 2026-07-10 — same caveat as TILE-018)
- [x] TILE-020 — Ventilator/fan tiles, icons 126-128, 3-frame (`table_decor_ventillog`) —
  **stale, corrected 2026-07-16**: already implemented (`GETerrainRenderer.cpp`'s
  `case FanLeft: return 126 + (phase % 3);`, `BlockTypes::FanLeft = 126`). **Compass-direction
  label uncertain**: this checklist calls 126-128 "Up" but this engine's own `BlockTypes.hpp`
  calls it `FanLeft` (matching French `ventillo-G`=gauche/left more plausibly than "up") — the
  exact real compass direction wasn't re-verified here (this engine doesn't model directional
  wind-push physics at all, only Fan-as-hazard, so the label has no gameplay consequence either
  way; flagged for whoever eventually needs the exact mapping, not blocking this entry's own
  "done" status).
- [x] TILE-021 — Ventilator/fan tiles, icons 129-131, 3-frame (`table_decor_ventillod`) — already
  implemented (`case FanRight: return 129 + (phase % 3);`). Same compass-direction-label caveat as
  `TILE-020` above.
- [x] TILE-022 — Ventilator/fan tiles, icons 132-134, 3-frame (`table_decor_ventilloh`) — already
  implemented (`case FanUp: return 132 + (phase % 3);`). Same compass-direction-label caveat.
- [x] TILE-023 — Ventilator/fan tiles, icons 135-137, 3-frame (`table_decor_ventillob`) — already
  implemented (`case FanDown: return 135 + (phase % 3);`). Same compass-direction-label caveat.
- [ ] TILE-024 — Water drip tiles (icons: table_decor_goutte, 48-frame) — **researched 2026-07-17,
      still correctly blocked, not stale**: the real 48-frame table (`Tables.cpp:1835-1842`,
      `table_decor_goutte`, icons 404-410, includes real `-1` "no sprite this tick" gaps) is a
      known, exact transcription target, and `BlockTypes.hpp`'s own `Drip` comment already
      correctly identifies WHY it isn't wired: the real visual is a Billboard-rendered vase/bulb
      sprite, not a terrain-cube atlas swap like Lava/Water/Saw — animating it faithfully needs
      the same kind of render-mode decision as the already-deferred Saw-blade/`ThinMechanical`
      items, not just a table transcription. Left blocked, not implemented.
- [x] TILE-025 — Temperature tile animation (table_decor_temp, 20-frame), div 4 / 200ms —
  **stale, corrected 2026-07-16**: already implemented (`GETerrainRenderer.cpp`'s
  `case Temp: return kAnimTemp[phase % 20];`).
- [x] TILE-026 — Marine tile (icon 203: table_marine, 11 frames, Object channel), div 3 / 150ms —
  **stale, corrected 2026-07-16**: already implemented (`GETerrainRenderer.cpp`'s
  `case Marine: return kAnimMarine[phase % 11];`). Note: this "icon 203" is a terrain-tile
  `BlockTypes` icon, unrelated to the `ObjectType203` (a `MoveObject`, the lethal decoration
  implemented under `PICKUP-069` this session) despite sharing the same number — separate data
  spaces.
- [x] TILE-027 — Terrain renderer's per-frame animation-phase update loop covers all animated tile types (CNA, 2026-07-10)

#### 5.3 Interactive / Hazard Tiles

**Badly stale, corrected 2026-07-14** — this subsection's own intro claimed "none of these have
hazard/gameplay logic wired... Blupi's collision does not test hazard tiles at all", but Phase 14
(`E3D-MIG-140`-`149`) implemented almost all of it across 2026-07-11/12; that phase's own
completion was never back-propagated to this checklist. Verified directly in
`GalaxyEggbertCnaGame.cpp` (most of this logic lives there, NOT `GEInteractionSystem.cpp` — same
false-negative risk flagged earlier in this file's own 2026-07-13 correction notes) and
`GEWorldRuntime.cpp`/`GEBlupiController.cpp`.

- [x] TILE-028 — Lava (icon 68): kill Blupi on contact — done (`GetGroundBlockType()==Lava` gate, `GalaxyEggbertCnaGame.cpp`).
- [x] TILE-029 — Spike (icon 373): kill Blupi on contact — done (`GetGroundBlockType()==Spike` gate).
- [x] TILE-030 — Crusher (icon 317-323): phase-gated (`IsCrusherActiveAtPhase()`, an approximation of the real 3-of-10-frame danger window, see that function's own comment) — **description corrected**: real effect is NOT a kill, it's a temporary squash/debuff (`TriggerCrush()`: reduced move speed, no jump, ~10s auto-recovery), confirmed directly against source.
- [x] TILE-031 — Saw (icon 378, active variant only — `SawStopped`/379 is a separate, safe value): kill Blupi on contact — done, real channel 75.
- [x] TILE-032 — Water drip (`IsGoutte`) — **description was wrong, corrected and implemented
      2026-07-14**: despite the "drip"/glue-sounding name, real behavior (confirmed via a direct
      `Decor.cpp` read, `IsGoutte`/`Decor.cpp:7220-7241`, call site `Decor.cpp:5513-5519`) is a
      deterministic KILL on contact (`BlupiAction::Glu`), mechanically identical to Spike — same
      gate shape, same real channel 51 sound — not a slow/glue debuff. Real trigger icon is 404
      (a second icon, 410, is excluded from the real safe-respawn-position FIFO but isn't itself a
      kill trigger — not modeled, a minor accepted simplification). `BlockTypes::Drip` added; real
      visual appearance is a green vase/bulb-on-a-neck (`Billboard` render mode per
      `mobile-eggbert-reference/02-tiles.md`), not a liquid graphic despite the name — falls back to
      the default `UniformCube`, an already-correct resolution (no dedicated visual identified as
      more faithful). Now a real 6th confirmed instant-kill terrain hazard alongside lava/spike/saw/
      crusher/Blitz. `GetGroundBlockType()` recognition test added (same shape as every other
      hazard's own test — the kill trigger itself lives in `GalaxyEggbertCnaGame::Update()`, not yet
      separately unit-testable for any of the 6 hazards). Full regression + both backends pass.
- [x] TILE-033 — Blitz/lightning tile: electric instant death — done, one of the real 6 confirmed hazards (see NEXT.md §2).
- [x] TILE-034 — Spring/ressort tile (icon 211): launches Blupi upward — done (`plan.md E3D-MIG-145`), correctly NOT a hazard (real behavior: bounce, not damage).
- [x] TILE-035 — Temp tile: brief passability change — done (`plan.md E3D-MIG-146`), `GEBlupiController::GroundHeightAt()`'s own `tempPassable` phase-gated skip.
- [x] TILE-036 — Door tile: locked door, opened by matching key — done (`plan.md E3D-MIG-160`/`161`, key- and treasure-gated families both real and working). The render-mode note (closed doors should be `Billboard`, not `UniformCube`) is confirmed STILL not done — explicitly deferred 2026-07-12 per user direction (no new render-geometry decisions that session), tracked separately (`E3D-MIG-516`/`163`, see §2.7 PICKUP-037's own note).
- [x] TILE-037 — Teleporter tile: pair of tiles, teleport Blupi — done (`plan.md E3D-MIG-147`), real ~6.4s transit duration.
- [x] TILE-038 — Switch tile: toggles linked door/bridge/saw state — done (`plan.md E3D-MIG-142`, `GEWorldRuntime::TryActivateSwitch()`, real 41-cell X±20 saw-linking window).
- [x] TILE-039 — Bridge tile: builds a bridge (`ObjectType52` animation) — **done 2026-07-13** (`plan.md PICKUP-064`, see §2.7's own entry for the full writeup — a real live terrain-collision toggle, not cosmetic).
- [x] TILE-040 — Ventilator tile: blows Blupi when in the fan stream — done (`plan.md E3D-MIG-149`, `GEWorldRuntime::TryConsumeFan()`).
- [x] TILE-041 — ~~Normal jump tile: forces a jump when stepped on~~ **description was wrong,
      corrected 2026-07-14; implemented same day**: `Decor::IsNormalJump()` is NOT a special tile
      that forces a jump on contact — it's a ceiling-clearance headroom check that modulates the
      STRENGTH of Blupi's own regular jump input: full height (-16, or -26 with Power) if 2 stacked
      cells above him are clear, a reduced "bumped head" height (-12, or -16 with Power) if either
      is blocked — preventing a full jump from clipping a nearby ceiling
      (`mobile-eggbert-reference/12-hazards-and-interactables.md`'s "Jump physics" section). Done via
      `GEBlupiController::HasJumpHeadroom()` + 3 new proportionally-anchored speed constants
      (`kJumpSpeedPowered`/`kJumpSpeedReduced`/`kJumpSpeedReducedPowered`, same technique as
      `kSpringBounceHeld`/`NotHeld`). Real source offsets the probe 15px toward Blupi's facing
      direction to disambiguate near a tile boundary — simplified to a single column (this engine's
      own current grid-snapped position), same "single stance, not multi-candidate" simplification
      already used for `TryActivateSwitch`/the bridge trigger scan. Verified against the real south
      tunnel's low BrickWall ceiling (exact expected velocity match, not just a pass/fail) + full
      regression on both backends — no interaction issues found with `GroundHeightAt()`'s own
      roofed-interior fix (headroom probe only reads terrain, doesn't touch ground resolution).
- [x] TILE-042 — Water surface: enter surf mode — done (`plan.md E3D-MIG-148`), real Surf/Nage state split.
- [x] TILE-043 — Deep water: enter swim/drown mode — done (`plan.md E3D-MIG-148`), real ~25s breath gauge (`kWaterGaugeMax`/`kWaterGaugeTickSeconds`).
- [x] TILE-044 — Out-of-water exit: exit swim mode on a dry tile — done, same Surf/Nage state machine as TILE-042/043.
- [x] TILE-045 — ~~Barre / barrier tile: blocks certain vehicle types~~ **description was wrong,
      corrected AND implemented 2026-07-14**: `Decor::GetTypeBarre()` has nothing to do with
      blocking vehicles — it's the trigger/classifier for a real hanging/suspended-on-a-bar-or-rope
      movement mode (`mobile-eggbert-reference/10-blupi-mechanics.md`'s "Suspended/hanging mode"
      section + a direct `Decor.cpp` read, `Decor.cpp:4732-4790`/`7158-7193`). Real trigger icons
      **138 and 202** — confirmed against 2 independent sources: the real source code itself, and
      icon 202 was ALREADY separately confirmed by the user's own 2026-07-07 questionnaire answer in
      `mobile-eggbert-reference/02-tiles.md` as "a rod/pole Blupi walks on and climbs over a
      dangerous obstacle beneath it," an exact match found before this mechanic was ever connected
      to that icon. Implemented via `GEBlupiController::GetBarreCellType()` (3-way: `None` = no bar
      tile; `Hanging` = bar tile with open air below, real type 1, grabbable; `LandingAvailable` =
      bar tile with solid ground below, real type 2, releases gracefully onto it) +
      `m_suspended`/grace-timer/drop-hold-timer state. Grab is automatic (no button), matching the
      real trigger exactly; horizontal movement while hanging reuses the plain no-ramp `moveInput*
      speed` shape this engine's normal walk already has (`kSuspendMoveSpeed`, no established
      proportional anchor so it just reuses `kMoveSpeed` directly); `kSuspendReleaseSpeed` IS
      proportionally anchored to `kJumpSpeed` (real fixed `-11.0` vs. the real jump baseline `-16.0`
      already anchored there). Real 10-tick jump-release wind-up animation NOT modeled (instant
      release instead — no visible Blupi model to show a wind-up); real 5-tick no-regrab grace timer
      IS modeled as a direct `ScaleTime(5)`-at-20Hz transcription. `TriggerMount()` now also excludes
      `m_suspended` (the real gate already documented this exclusion, just had nothing to check
      before this feature existed). Icon 202 now has its own render geometry too (`TILE-055`,
      done 2026-07-14) — the demo world uses both icon 138 (via the existing `InnerFlatPlate`
      table) and icon 202 (via the new `ThinBar` geometry). 11 new `VerifyBlupiMovement` checks
      (grab, climb, graceful landing, free-fall drop off the near end, jump-release, grace-timer
      block + expiry) + a live headless screenshot sanity check + full regression on both
      backends, all pass.

#### 5.4 Tile Adaptation (visual smoothing)

- [ ] TILE-046 — `table_adapt_decor` (144 entries): smooth corner blending based on neighbour mask
- [ ] TILE-047 — `table_adapt_fromage` (32 entries): cheese tile corner blending
- [ ] TILE-048 — `table_decor_quart` (7056 entries): full tile replacement lookup by neighbour mask

#### 5.5 Background & Sky

- [x] TILE-049 — Background sky PNG per region (`decor000.png`..`decor031.png`, not all consecutive) — **done** (see TILE-011): loads the exact real file matching the world's `skyRegion`, degrades gracefully (no crash) for the 4 real region ids confirmed never used by any real level.
- [ ] TILE-050 — 5 sky colour palettes (ambient + fog per world region)
- [ ] TILE-051 — Per-zone fog colour changes mid-level (region changes between areas)
- [ ] TILE-052 — Lightning tile visual effect (icon 66-68 drawn 13 px higher, ch69 sound) --
  **premise wrong, checked 2026-07-20**: icons 66-68 are NOT lightning
  (`mobile-eggbert-reference/02-tiles.md`: 66 = unnamed ladder-like ThinMechanical column, 67 =
  unnamed brick Billboard, 68 = `Lava`'s own base animation frame). The real lightning/Blitz
  hazard is icon 305 (`Decor::BlitzActif`, `Decor.cpp:620-634`), already fully implemented in this
  engine (one of the 4 lethal hazard tiles, Phase 14 `141`). Whatever this line item's icon-66-68
  claim was originally based on, it does not correspond to any real mechanic at those icons --
  don't implement as written; either correct with a real citation or drop.

#### 5.6 Open Rendering-Mode Decisions (CNA-specific, new)

- [ ] TILE-053 — Decide and implement `ThinMechanical` render-mode geometry for saws/springs/switches/fans/bridge/pipes/grates (~25 icons) — decision not yet made
- [ ] TILE-054 — Distinct water/liquid surface treatment (wavy-edge surface) to replace the current alpha-blended-cube placeholder
- [x] TILE-055 — "Thin-bar" new geometry for icon 202 — **done 2026-07-14**. New
      `GEThinBarTiles.hpp`/`.cpp` (`TryGetThinBarFaces()`), modeled directly on the existing
      `GEInnerPillarBoxTiles` precedent: an `Easy3D::DirectionalCubeItem` sized `(1.0,
      kThinBarThickness=0.3, kThinBarThickness=0.3)` — full block width along the bar's own X
      axis (so adjacent bar blocks connect seamlessly), thin in Y/Z. 4 long sides use the tile's
      real texture; the 2 end caps (PosX/NegX) use a flat `SwatchUv(tileUv, kMidSwatchV)`
      fallback colour, matching the user's 2026-07-07 questionnaire description ("thin
      rod/prism, not a cube; texture on the 4 long sides, 2 small square blue end faces").
      Wired into `GETerrainRenderer::AppendSpecialGeometry()`/`IsSpecialGeometryIcon()`.
      Direct pixel inspection of `object-m.png` found icon 202's real crop is a thin stripe on an
      otherwise fully-transparent 64x64 tile — rendering it through the normal opaque static-mesh
      pass showed a solid white block (the alpha=0 background pixels' own RGB) instead of the
      thin rod. Fixed by adding icon 202 to `NeedsAlphaBlend()` in `GETerrainRenderer.cpp`,
      routing it through the same alpha-respecting "static-but-transparent" pass already used for
      icons 30/31 and the 4 teleporter pillars. Confirmed via live headless screenshots at close
      range and 3/4 angle: the tile now renders as a thin, mostly-transparent rod with visible
      colour, no solid white block. Demo world (`tools/GenerateSampleWorld3D.cpp`'s Suspended/
      hanging bar demo, `TILE-045`) now places icon 202 in its own row alongside icon 138. Full
      regression on both backends passes (only the known pre-existing unrelated
      `easy-gl-resource-smoke-tests` failure).
- [ ] TILE-056 — Architectural kit modular assembly
- [ ] TILE-057 — ~~Secret-power (Sp0-7) billboard rendering and behavior~~ **obsolete premise, corrected 2026-07-14**: resolved 2026-07-12 (see `## 3 Open Questions`) — "Sp0-Sp7" are real hub-screen world-select icons (`Decor::IsWorld()`), not secret-power tiles at all. The real 4 `SecretPower` buffs come from `MoveObject` pickups 25/26/30/31 instead, already fully implemented (§2.7 PICKUP-007/008/010/012). This item itself has nothing left to do.

---

### 2.6 Enemy AI

**Re-verified against source 2026-07-13 — this section was badly stale** (still said "not started
at all" from before the 2026-07-11/12 implementation push). Ground truth is Phase 13
(`E3D-MIG-130`-`137`, marked complete) above; this subsection maps that work onto the older
ENEMY-XXX numbering rather than re-deriving it.

#### 6.1 Common Enemy Behaviour

- [x] ENEMY-001 — Patrol movement: oscillate between posStart and posEnd — done (`131`, the
      shared 4-phase dwell/advance/dwell/recede state machine in `GEInteractionSystem`'s
      `AdvancePatrolStep()`).
- [ ] ENEMY-002 — Stationary enemies get ±2 tile default patrol range — NOT modeled; per `131`'s
      own note, patrol ranges are level-authored per instance, not auto-derived from a
      stationary flag.
- [ ] ENEMY-003 — Directional sprites: flipX when moving right — NOT modeled (`131`'s own note:
      "direction-mirrored animation-table selection... is NOT modeled — no directional walk/turn
      sprite tables exist for these types yet, only simple icon-cycling").
- [ ] ENEMY-004 — Stomp kills all enemy types on velY < -1.0 contact — NOT modeled; confirmed no
      velocity-gated "stomp" concept exists anywhere in `GEInteractionSystem.cpp` — contact-kill
      (`132`) is an unconditional touch check, not stomp-specific.
- [ ] ENEMY-005 — Enemy respawns at posStart after 5s kill timer — **likely hallucinated,
  investigated 2026-07-16**: verified directly against `Decor.cpp:7881-7917` (`Decor::
  ObjectDelete()`, the real enemy-destruction function) — it unconditionally sets the slot to
  `ObjectType0` (freed) with no timer or respawn logic at all, just an optional cosmetic
  "goodbye" particle for a few types. A broad grep for "respawn"/"revive" across all of
  `Decor.cpp` found nothing enemy-related either. This looks like another invented claim, same
  category as the already-cancelled stomp-kill entries — not confirmed against source anywhere;
  do not implement without finding the actual citation first.
- [ ] ENEMY-006 — Blob shadow under enemies *(3D adaptation)* — not done, visual polish only.
- [ ] ENEMY-007 — Y-proximity check: aerial enemies don't hit ground-level Blupi — no explicit
      per-type rule exists; contact uses a plain 3D-distance sphere check (includes Y implicitly,
      but not a deliberate aerial/ground distinction).
- [x] ENEMY-008 — `MoveObjectStepLine`: advance/recede speed + end-dwell timer logic — done (`131`).
- [x] ENEMY-009 — `MoveObjectStepIcon`: per-type animation phase counter update — done, resolved
      by `131`/`GEWorldRuntime::Update()`'s existing generic per-instance phase advance.

#### 6.2 Per-Type Enemy Implementation

- [x] ENEMY-010 — ObjectType2: patrol enemy A — patrol (`131`) + contact-kill (`132`, part of the
      real shared 8-type `IsGenericHazard()` list) done; distinct sprite tables not verified.
- [x] ENEMY-011 — ObjectType3: patrol enemy B — same as ENEMY-010 (`131`/`132`); real duck-immunity
      for this type specifically IS modeled (`blupiCrouching` parameter).
- [x] ENEMY-012 — ObjectType4: bulldozer — patrol + contact-kill done (`131`/`132`).
- [x] ENEMY-013 — ObjectType4: bulldozer real per-direction/turn-transition icon animation —
      **resolved 2026-07-20, and the original title was based on a misconception**: direct
      research (`Decor.cpp:8628-8668`) found no distinct "charge" AI at all -- the bulldozer uses
      the exact same shared 4-phase dwell/advance/dwell/recede patrol as every other `MoveObject`
      (already implemented, `AdvancePatrolStep()`); what's real and was genuinely missing is its
      own 4-table turn/walk icon selection (`table_bulldozer_left/right/turn2l/turn2r`,
      `Tables.cpp:1186-1205`) -- this engine's `GetObjIcon()` only ever used the "left" table
      regardless of direction or turn-transition step (documented simplification, `GEObjectIcons.
      hpp`'s own header comment). Added `GEObjectIcons::GetBulldozerIcon(patrolGoesLeftFromStart,
      patrolStep, patrolTimeTicks)` -- a direct port of the real switch, fed by the exact same
      `MobileObjSpec::posStartX/posEndX/patrolStep/patrolTime` fields `AdvancePatrolStep()` already
      maintains -- wired at the one render call site with real patrol-state access (the
      element.png billboard batch in `GalaxyEggbertCnaGame.cpp`); `GetObjIcon()`'s own generic
      ObjectType4 case is left untouched as the fallback for context-free callers (e.g. the editor
      palette). 14 new `VerifyInteractionSystem` assertions (all 8 direction×step combinations
      plus 2 modulo-wraparound checks), byte-matched against the real table values. No bulldozer
      is placed in the current sample world, so no live screenshot was taken -- verification relies
      on the exhaustive unit-level match against real source instead.
- [x] ENEMY-014 — ObjectType16: spider — contact-kill done (`133`, part of `132`'s widened list);
      the real 9-frame crawl / vertical hang↔drop-specific visual pattern is not separately
      verified (uses the same generic patrol/icon-cycling as every other type).
- [x] ENEMY-015 — ObjectType17: fish — contact-kill done (`133`); water-specific patrol context not
      separately modeled (functionally unnecessary since collision doesn't distinguish).
- [x] ENEMY-016 — ObjectType17: turn animation (48 frames) — **resolved 2026-07-20**, same pass as
      `ENEMY-013`: added `GEObjectIcons::GetFishIcon()`, a direct port of the real
      `Decor.cpp:8670-8711` switch (`table_poisson_left/right/turn2l/turn2r`,
      `Tables.cpp:1212-1236`), wired at the same bulldozer render call site.
- [x] ENEMY-017 — ObjectType20: bird — contact-kill done (`133`); aerial Y=3.0 patrol positioning
      is level-authored data, not a special-cased behavior.
- [x] ENEMY-018 — ObjectType20: turn animation (10 frames) — **resolved 2026-07-20**, same pass as
      `ENEMY-016`: added `GEObjectIcons::GetBirdIcon()`, real `Decor.cpp:8712-8753`
      (`table_oiseau_left/right/turn2l/turn2r`, `Tables.cpp:1243-1254`).
- [x] ENEMY-019 — ObjectType33: blupit — patrol + turn-dwell-timed attack done (`134`).
- [x] ENEMY-020 — ObjectType33: blupit fires ObjectType23 at phase 3/21 during turn — done (`134`,
      real two-horizontal-shots-per-turn-dwell behavior, direction/frame pairing corrected during
      verification vs. this file's own earlier (wrong) prose summary).
- [x] ENEMY-021 — ObjectType32: blupih — patrol + turn-dwell-timed attack done (`134`).
- [x] ENEMY-022 — ObjectType32: blupih fires ObjectType23 during turn — done (`134`, vertical drop
      at dwell-frame 21).
- [x] ENEMY-023 — ObjectType44: wasp/bee — fast patrol + real balloon-status trigger/hazard-pop
      interaction done (`135`).
- [x] ENEMY-024 — ObjectType44: turn animation (5 frames) — **resolved 2026-07-20**, same pass:
      added `GEObjectIcons::GetWaspIcon()`, real `Decor.cpp:8754-8795`
      (`table_guepe_left/right/turn2l/turn2r`, `Tables.cpp:1261-1270`) -- the patrol/idle animation
      only, distinct from the already-implemented balloon-transform hazard interaction.
- [x] ENEMY-025 — ObjectType54: creature — slow patrol + turn-dwell-gated lethality done (`136`).
- [~] ENEMY-026 — ObjectType54: long turn animation (152 frames) — **turn/walk icon resolved
      2026-07-20**: added `GEObjectIcons::GetCreatureIcon()`, real `Decor.cpp:8796-8837`
      (`table_creature_left/right/turn2`, `Tables.cpp:1278-1307`) -- confirmed the real
      left/right walk tables are byte-identical and both turn steps (1 and 3) share one table, so
      unlike the other 4 in this family there's no direction parameter to take. **Still open**: the
      real unconditional taunt icon is NOT modeled (no idle-taunt animation system exists at all,
      same gap as `136`'s own note) -- a separate, larger feature, not attempted this pass.
- [x] ENEMY-027 — ObjectType54: destroys Blupi's helicopter on contact (ByeByeHelico) — **stale
  cross-reference, corrected 2026-07-16**: this entry's own premise was already corrected in
  `136`'s own writeup — contact does NOT spare Blupi/only-destroy-the-vehicle instead of killing;
  it's an unconditional Glu death either way, which ALSO clears vehicle/Balloon/Ecrase state when
  applicable (via `GEBlupiController::TriggerDeathLock()`, fixed 2026-07-16, matching real
  `BlupiDead()`). `ByeByeHelico()` itself is purely a cosmetic particle effect with no state
  change (see `136`'s own note) — not worth new plumbing for. See `136` for the full writeup.
- [ ] ENEMY-028 — ObjectType18: additional patrol enemy variant — NOT modeled as its own enemy;
      `ObjectType18` appears only in the dynamite-blast destructible-object list, not in
      `IsGenericHazard()`'s real 8-type kill-list.
- [x] ENEMY-029 — ObjectType96: follow enemy 1 (dormant) — done (`137`).
- [x] ENEMY-030 — ObjectType97: follow enemy 2 (awake) — done (`137`).
- [x] ENEMY-031 — ObjectType96/97: wake + homing toward Blupi — done (`137`, Chebyshev-style X/Y
      step toward Blupi's live position at the real 1px/tick speed, self-destructs into a solid
      cell rather than continuing).

Note: `ObjectType32`/`33` (blupih/blupit) need `blupi1.png`, not `element.png` — if any prior
assumption used a single shared sprite sheet for all enemies, that assumption is wrong; see
ObjectType-to-sheet corrections repeated in §7.

#### 6.3 Projectiles

- [x] ENEMY-032 — ObjectType23: fired projectile — done (`134`, spawned by blupih/blupit via
      `SearchAirDistance()`'s real grid-cell raycast).
- [x] ENEMY-033 — Projectile travels toward Blupi position — done (`134`); the specific "expires
      after 55 frames" figure isn't independently re-verified, but travel + arrival/expiry is real
      and working.
- [x] ENEMY-034 — Projectile hit detection: damage Blupi if shield inactive — done; the real
      `!m_blupiShield && !m_blupiHide` hazard-immunity gate (Phase 17 `170`) applies across all
      ~15 real call sites including projectiles.
- [x] ENEMY-035 — Projectile fire sound — done, but the real channel is **52**, not the guessed
      ch27 (`FireBlupihShot()`/the blupit-shot branch in `GEInteractionSystem.cpp` both play
      `SoundChannel52`).

#### 6.4 Enemy Sounds

- [x] ENEMY-036 — Stomp kill sound: ch5 — **closed 2026-07-17, stale checkbox**: no stomp concept
      exists (confirmed hallucinated, per the standing `SOUND-015`/`BLUPI-126`/`131` cancellation);
      the real generic contact-kill death sound (channel **74**) IS already wired for every
      `IsGenericHazard()` kill (`GEInteractionSystem.cpp:1546/1681/1738`), always (real source is
      a 50/50 coinflip between ch74 and silence, per `132`'s own note — simplified here to always
      ch74, not modeled as a coinflip, a documented simplification not a gap).
- [ ] ENEMY-037 — Bulldozer turn sound (ch33) — NOT modeled; no per-type turn sound exists (ch33
      is actually the real door-open channel, unrelated — see §2.7's PICKUP-041/075 correction).
- [x] ENEMY-038 — Enemy destruction sound varies by type — **closed 2026-07-17, stale checkbox**:
      corrected description (this engine deliberately simplifies the real source's per-type
      variation to a single uniform channel 74 for every `IsGenericHazard()` kill, already wired
      — see `ENEMY-036`) — not an unmodeled gap, a documented, working simplification.
- [x] ENEMY-039 — Wasp/bee balloon-status sound — done, but the real channels are **40** (balloon
      entry) and **41** (recovery/expiry, shared with Crusher's own recovery cue) — not ch72/73 as
      guessed; both wired in `GalaxyEggbertCnaGame.cpp`. No separate continuous "movement" sound
      was found.
- [ ] ENEMY-040 — Creature movement sound — **researched further 2026-07-17, still not found, now
      leaning hallucinated/non-existent rather than merely unverified**: grepped every enemy
      movement function (`Decor::MoveObjectStep()`/`MoveObjectStepLine()`/`MoveObjectStepIcon()`,
      `7944-8296`) for any `PlaySound` call — found exactly 2, both one-shot event sounds (ch10
      blast-contact, ch59 an unrelated pickup-adjacent cue), neither a continuous/looping
      movement cue. No `bulldozer`/`creature`/`enemy`-tagged sound call exists anywhere in
      `Decor.cpp`. Consistent with the "no separate continuous movement sound was found" note
      directly above (`ENEMY-039`'s own research) — left as an open, not a confirmed-hallucinated
      item (no explicit "movement sound" claim was ever traced to a specific real source line to
      definitively refute, unlike the 3 already-cancelled hallucinations), but there is now no
      positive evidence for it after 2 independent research passes.
- [x] ENEMY-041 — ObjectType32 (blupih): real per-direction/turn-transition icon animation —
      **added 2026-07-20**, found while completing the same real gap for bulldozer/fish/bird/wasp/
      creature (`ENEMY-013/016/018/024/026`) -- no pre-existing checklist item covered this one.
      `GEObjectIcons::GetBlupihIcon()`, direct port of `Decor.cpp:8838-8887`
      (`table_blupih_left/right/turn2l/turn2r`, `Tables.cpp:1314-1333`). The real projectile-fire
      trigger at the same source site is `ENEMY-022`, already implemented separately -- untouched.
- [x] ENEMY-042 — ObjectType33 (blupit): real per-direction/turn-transition icon animation —
      **added 2026-07-20**, same pass as `ENEMY-041`. `GEObjectIcons::GetBlupitIcon()`, direct port
      of `Decor.cpp:8888-8927` (`table_blupit_left/right/turn2l/turn2r`, `Tables.cpp:1340-1360`).
      Real projectile-fire trigger is `ENEMY-020`, already implemented separately -- untouched.

---

### 2.7 Pickups & Objects

**Re-verified against source 2026-07-13 — badly stale.** This section predates the 2026-07-11/12/13
implementation push (Phases 15-17 above, which are current) and still marked most of that work
`[ ]`. Corrected in place below; ground truth is Phases 15/16/17 and a direct source check of
`GEInteractionSystem.cpp`/`GEBlupiController`, not a re-derivation.

**Sound-channel correction, applies throughout this section:** treasure/key pickup = Channel 11 (or
19 if the pickup completes a set), egg = Channel 3, door open = Channel **33** (not ch7, corrected
2026-07-13 — see PICKUP-041/075), exit-reached = Channel **14** (not ch57, PICKUP-074), generic
hazard/enemy contact-kill death = Channel **74** (not ch5/varies-by-type, see §2.6 ENEMY-036/038),
dynamite/blupih/blupit-fire = Channel **52** (shared by both real actions), wasp balloon
entry/recovery = Channels **40/41** (not ch72/73/46) — where a line item's channel number below
conflicts with these, the corrected value governs. **Sprite-sheet correction:** `ObjectType1`/`12`
need `object-m.png`, not `element.png`; `ObjectType32`/`33` need `blupi1.png`, not `element.png` —
any task assuming one shared sheet for all pickups/objects is wrong.

#### 7.1 Static Collectibles

- [x] PICKUP-001 — ObjectType5: treasure — collection/removal-on-contact works, required for exit gating (CNA, 2026-07-10) — score/HUD-fly-animation still TBD, see PICKUP-049/HUD-025
- [x] PICKUP-002 — ObjectType6: egg — collection works, MAX_EGG_COUNT=10 cap (CNA, 2026-07-10) — note: real channel is ch3, not ch42 as originally listed
- [x] PICKUP-003 — ObjectType7: exit goal — gated on treasures-collected (CNA, 2026-07-10)
- [x] PICKUP-004 — ObjectType49: red key — sets Key1 flag, real sound channel (11, or 19 if set-completing) (CNA, 2026-07-10)
- [x] PICKUP-005 — ObjectType50: green key — sets Key2 flag, same channel correction as PICKUP-004 (CNA, 2026-07-10)
- [x] PICKUP-006 — ObjectType51: blue key — sets Key3 flag, same channel correction as PICKUP-004 (CNA, 2026-07-10)
- [x] PICKUP-007 — ObjectType25: shield orb — done (Phase 17 `170`/`172`), grants `SecretPower::Shield` instantly on contact (real 2-stage delay not modeled); pickup sound wired (ch42, see PICKUP-073 — corrected 2026-07-13, an earlier audit pass this same session wrongly flagged this as unwired since the `sound.Play()` call lives in `GalaxyEggbertCnaGame.cpp`, not `GEInteractionSystem.cpp`).
- [x] PICKUP-008 — ObjectType30: drink — done (`170`/`173`), grants `SecretPower::Hide` instantly (real name is "Drink→Hide", not "+1 life"; the two-stage grab/delayed-activate animation is NOT modeled, see `173`); pickup sound wired (ch62, see PICKUP-073).
- [x] PICKUP-009 — ObjectType21: secret exit — **done 2026-07-16**, verified directly against `Decor.cpp:6158-6184` (one `if` covers both `ObjectType7`/`21`): shares the exact same real exit-gate contact logic as the regular exit (treasure-gated win/reject sound). Previously this engine only recognized `ObjectType7`, so touching a secret exit did nothing. Real `m_bFoundCle`-equivalent flag still not modeled (no consumer exists, see PICKUP-038/039). New `VerifyInteractionSystem` assertion.
- [x] PICKUP-010 — ObjectType31: cloud power-up — done (`170`/`172`/`174`), grants `SecretPower::Cloud` (strictest gate of the 4, matching real source) + real Cloud offensive `BlupiElectro` aura (2026-07-13, ch59); pickup sound wired (ch55, see PICKUP-073).
- [x] PICKUP-011 — ObjectType40: invert/mirror power-up — **implemented 2026-07-13**: independent
      of the 4 SecretPower buffs (own gauge, real gate only `!Hide`), grants instant on contact,
      negates normal ground movement (`GEBlupiController::TriggerInvert()`/`IsInverted()`, real
      `Decor::SetSpeedX` "speed = -speed" behavior — vehicles deliberately unaffected, they use a
      separate real speed system), real ~15s duration (`ScaleTime(3)`, same rate as Power, no
      warning stage), pickup/expiry sounds ch66/67. Demo placement added to the secret-powers room
      in `tools/GenerateSampleWorld3D.cpp`. Not modeled: the 4-direction `ObjectType41`/`42`
      particle bursts (no particle system exists) and sprite-mirroring (no visible Blupi model).
- [x] PICKUP-012 — ObjectType26: suction-cup ("Sucette") — done (`170`/`173`), grants `SecretPower::Power` instantly; real 2-stage delay + wall-climbing behavior NOT modeled; pickup sound wired (ch44, see PICKUP-073).
- [x] PICKUP-013 — ObjectType29: bullet ammo — done (Phase 17 `175`), tops up to `kBulletCap=10`, real no-op-at-cap behavior, ch54 fanfare.
- [x] PICKUP-014 — ObjectType55: dynamite — done (Phase 15 `155`), caps at 1 carried, action-button placement, real 9-blast fuse sequence.
- [x] PICKUP-015 — ObjectType13: helicopter — done (Phase 17 `171`), `VehicleMode::Helicopter`, real free vertical flight.
- [x] PICKUP-016 — ObjectType19: jeep — done (`171`), `VehicleMode::Jeep`, real accel ramp.
- [x] PICKUP-017 — ObjectType28: tank — done (`171`), `VehicleMode::Tank`; also the only vehicle that can fire (`BULLET-001`).
- [x] PICKUP-018 — ObjectType24: skateboard — done (`171`), `VehicleMode::Skateboard`, reuses ground gravity/jump path.
- [x] PICKUP-019 — ObjectType46: — done (`171`), but corrected name/effect: real touch sets `m_blupiOver` (**Overcraft**, free vertical flight like Helicopter), **not** a separate "Balloon" vehicle or `m_blupiBalloon` (that flag is the unrelated wasp-sting status, Phase 13 `135`) — `ObjectType.hpp`'s own doc comment is confirmed misleading here.

#### 7.2 Platform Lifts

- [x] PICKUP-020 — ObjectType1: platform lift patrols posStart↔posEnd (CNA, 2026-07-10) — now also carries Blupi, see PICKUP-023.
      **North-hill lift re-designed 2026-07-17** (user-reported, confirmed via screenshot: the lift
      near the wasp visibly "clips through" the crow's-nest floor near the wasp): the underlying
      patrol math was actually correct (verified twice independently against the real cube-rendering
      convention — object cubes span `[Y+0.5, Y+1.5)`, terrain blocks span `[gridY-0.5, gridY+0.5)`,
      so `posStartY=4.0`/`posEndY=7.0` land exactly flush with the plateau floor and the crow's-nest
      floor respectively, zero geometric overlap) — the REAL problem was the crow's-nest floor's own
      single 1-cell carved shaft opening (from the 2026-07-12 fix) being surrounded on all sides by
      solid rock, which reads as "the platform vanishes into stone" from nearly any camera angle
      regardless of the underlying math. A first attempt widened the hole into a full 3-wide trench
      along the lift's own X row — live-verified this STILL looked wrong: even a full-width, 1-cell-
      deep trench gets visually "filled back in" by the solid neighbouring rows the instant the
      camera isn't looking exactly down the trench axis. The real fix, matching the exact same
      conclusion `liftA`/`liftB` already reached for this identical class of problem (their own
      comment: open-sky placement to sidestep the enclosed-shaft issue entirely) — removed the
      enclosing 3x3 floor slab entirely; the shaft column (x=50,z=28) is now open air all the way up,
      and the chest/exit-goal that used to sit on the slab's east/west edges now perch on two small,
      visually separate single-cell stepping-stones instead. Live-verified via a temporary debug
      camera (reverted before commit): the platform now floats clearly in open space with visible
      background on all sides, fully separated from both stepping-stones, at every patrol height from
      4 to 7. Full regression clean (only the pre-existing unrelated `easy-gl-resource-smoke-tests`
      failure).
- [x] PICKUP-021 — ObjectType47: platform lift rightward conveyor nudge — done (Phase 15 `154`), folded into `RideDeltaX()`'s `kConveyorNudgeSpeed`; exact real px/tick magnitude is an approximation (no established unit conversion), not a transcription.
- [x] PICKUP-022 — ObjectType48: platform lift leftward conveyor nudge — done (`154`), same note as PICKUP-021.
- [x] PICKUP-023 — Platform boarding/riding — done (Phase 15 `152`), was the root blocker for this whole subsection, now resolved: `GEInteractionSystem` detects standing on an active lift's footprint/height, reports its per-tick displacement, `GEBlupiController::RideLift()` applies it (X/Z as a delta preserving walking input, Y snapped absolutely).
- [ ] PICKUP-024 — AscenseurVertigo (edge-hang on wide/shiftable platforms, icons 311-316) — NOT started (`153`); blocked on a deferred render/icon-selection decision, not a gameplay-logic gap.
- [x] PICKUP-025 — AscenseurShift: shift Blupi with moving platform — done, this IS `152`'s `RideLift()` delta-shift mechanism (same feature, different name in this older checklist).
- [ ] PICKUP-026 — AscenseurSynchro: synchronise multiple lifts — NOT modeled; no evidence of any special multi-lift linking (each lift patrols independently on its own clock, which may already be sufficient for this world's needs).
- [ ] PICKUP-027 — m_blupiTimeNoAsc: cooldown preventing immediate re-entry — NOT modeled.
  **Identified 2026-07-16**: this description was too generic — verified directly against
  `Decor.cpp:9232-9265` (`Decor::AscenseurVertigo()`): it's not a standalone cooldown, it's an
  intrinsic PART of the `AscenseurVertigo` wide/shiftable-lift-teeter mechanic itself (icons
  311-316) — set to 10 ticks specifically when Blupi teeters off one end of a "shift" lift, so he
  slides off rather than balancing forever. Genuinely blocked on the SAME render-geometry decision
  as the rest of `AscenseurVertigo` (`PICKUP-024`/`153`) — not independently implementable.

#### 7.3 Crates (ObjectType12)

- [x] PICKUP-028 — ObjectType12: crate renders as a real cube (CNA, 2026-07-10) — was previously a static billboard placeholder in Simple3D; CNA's is a real `UniformCube`
- [x] PICKUP-029 — Crate push: walking into a crate pushes it 1 tile, X-axis only (CNA, 2026-07-10) — non-X pushing still not done, but crates now stack/link, see PICKUP-031.
- [x] PICKUP-030 — Crate stops when hitting a wall or another crate (adjacency/floor-support/occupancy checks) (CNA, 2026-07-10)
- [x] PICKUP-031 — Crates can be stacked and linked crates push as a group — done (Phase 15 `150`), a real flood-fill (not the real `SearchLinkCaisse` array structure itself, but behaviorally equivalent) restricted to crates at/above the seed's row, matching the real "push the stack, not the floor it rests on" rule; all linked members move atomically, any one blocked cancels the whole push.
- [x] PICKUP-032 — m_rankCaisse / m_nbRankCaisse: array of crate object indices — achieved differently: `150`'s flood-fill walks `worldRuntime`'s live object list directly rather than maintaining a separate rank array, same net effect.
- [x] PICKUP-033 — TestPushCaisse: check if push is valid (clear path) — folded into `150`'s flood-fill validity check (every linked member must have a clear destination).
- [x] PICKUP-034 — CaisseInFront: detect crate directly in front of Blupi — folded into the existing single-crate push detection this subsection already had `[x]` (PICKUP-029).
- [ ] PICKUP-035 — SmallShake on crate land / impact — NOT modeled. **Corrected 2026-07-16**: the
  premise "no screen-shake system exists" is now stale (the camera-shake system was implemented
  `CAM-008`/`009`, 2026-07-14) — the real blocker is that crates in this engine have no vertical
  fall/gravity physics at all (confirmed: `IsCrate()` only appears in the horizontal-push and
  dynamite-blast-destroy code paths), so there is no "landing" event to hook a shake onto. Same
  prerequisite as `151` (crate "pop"/fall variant, not started — needs a deeper rework of crate
  movement to be continuous/timed).
- [x] PICKUP-035b — Conveyor nudge for ObjectType47/48 — done, same as PICKUP-021/022 (Phase 15 `154`).

#### 7.4 Doors & Keys

- [x] PICKUP-036 — DoorKeyFlags — done differently (Phase 16 `161`): `keys1_`/`keys2_`/`keys3_` are plain pickup counters, not a persisted 3-bit flag, but behaviorally identical for the realistic case (one key granted before a door needs it).
- [x] PICKUP-037 — Door tile (IsDoor): opens when Blupi touches and holds matching key — done (`161`), automatic on approach (2-cell probe in Blupi's facing direction), no action-button gate. Render-mode correction (closed doors should be `Billboard`, not `UniformCube`) is confirmed still NOT done — explicitly skipped 2026-07-12 per user direction (no new render-geometry decisions that session), tracked as `E3D-MIG-516`/`163`.
- [ ] PICKUP-038 — InitializeDoors: restore door states from GameData on level load — NOT modeled; no door-state persistence exists in `GESaveData` (which isn't byte-compatible with real `GameData` by design).
- [ ] PICKUP-039 — MemorizeDoors: save door states to GameData on level exit — NOT modeled, same reason as PICKUP-038.
- [x] PICKUP-040 — Door open animation: ObjectType22 (self-removing slide) — done (`160`), real `Config::ScaleTime(50)`=2.5s slide-up-by-1-grid-unit, then self-destructs; the slide object's own icon is not rendered accurately (no confirmed icon data for type 22 regardless, a pre-existing unrelated gap).
- [x] PICKUP-041 — Door open sound — done, but real channel is **33**, not ch7 (verified directly in `GEInteractionSystem.cpp`'s `OpenDoorAt()`).

#### 7.5 Visual Effects (transient objects)

Re-verified 2026-07-13: confirmed **still entirely NOT implemented**, this subsection was NOT
stale (unlike 7.1-7.4 above) — no particle/transient-visual-effect/debris system exists anywhere in
`GEInteractionSystem`/`GEHud` beyond what's already noted elsewhere (e.g. dynamite's blast has "no
debris/particle visuals", Phase 15 `155`/`156`). PICKUP-042 through PICKUP-063 all remain `[ ]` as
originally listed — no changes.

#### 7.6 Special Level Objects

- [x] PICKUP-064 — ObjectType52: bridge construction (live terrain-collision toggle) — **implemented
      2026-07-13**: while grounded on a Bridge (icon 364) tile, spawns an `ObjectType52` object that
      writes the construction sequence's icon directly into the terrain grid cell every tick, real
      "genuinely removes ground collision for most of the sequence" behavior — 28 ticks ascending
      through icons 365-372, 112 ticks holding the real `-1` "no tile" sentinel (mapped to `Air` via
      `BlockTypes::fromMobileIconId()`, which already correctly treats 367-372/-1 as non-solid, so
      no `GroundHeightAt()` changes were needed), 17 ticks descending back to the original 364. Real
      mobile-eggbert's own exact per-tick `table_bridge` array is NOT transcribed (no pre-approved
      in-repo source for it) — this reproduces the documented 28/112/17-tick aggregate shape with
      this engine's own uniform pacing within each window. Demo gap+bridge added to
      `tools/GenerateSampleWorld3D.cpp`. 5 new `VerifyInteractionSystem` checks (spawn, mid-sequence
      hollow, an independent `GEBlupiController` actually falling through, restoration, self-delete)
      + full 7-tool suite + both backends pass. Not modeled: sprite mirroring (no visible Blupi
      model) and the mid-sequence sound (ch73) firing purely as its own scripted cue is modeled, but
      the construction-start sound was assigned ch72 (matching `07-sounds.md`) rather than the
      stale ch20 guess this checklist's own PICKUP-084 line still has (see that item's own note).
- [x] PICKUP-065 — ObjectType56: dynamite fuse — done (Phase 15 `155`), real per-blast phase-driven timing (ticks 50/53/55/56/59/62/64/67/69), not the rounded "phases 50-69" this line originally guessed.
- [x] PICKUP-066 — DynamiteStart: blast clears tiles — done (`155`), 2×2-tile area per blast (not a general "radius"), real exact 28-type destructible-object list.
- [ ] PICKUP-067 — ObjectType200-203: Blupi avatar skins — render icons (257-262) exist in `GEObjectIcons.cpp`, but no world currently places any and no costume-select gameplay hook exists — see PICKUP-068's correction below for what `ObjectType200` actually does instead.
- [x] PICKUP-068 — ObjectType200 — **description was wrong**: real implemented behavior (`GEInteractionSystem::TryPerso()`, "Perso" mechanic, NEXT.md's HUD-017) is a decoy placement/retrieval pickup (carry a decoy, place it on the ground, or pick an already-placed one back up), **not** a "costume select pickup → player-select voyage" trigger — no such voyage/costume-select system exists.
- [x] PICKUP-069 — ObjectType201-203: damage Blupi on contact if shield/hide/SuperBlupi inactive — **done 2026-07-16**, found via direct `Decor.cpp:6088-6115` read (the previous "no contact-damage logic found" was wrong — it lives right next to the already-implemented `ObjectType200` Perso pickup, same `>= 200 && <= 203` range check, easy to miss reading only around the Perso branch). Real contact: same `BlupiDead(Clear1, Clear2)` 50/50 coinflip death as the generic-hazard list (Shield/Hide immune via `blupiInvincible`, SuperBlupi not modeled — no such concept exists), no `m_blupiRestart=true` anywhere in this real block (`shouldRespawn=false`) — but ALWAYS channel 10 + `SmallShake` + an `ObjectType10` pop effect (no fish/bird `BigShake` variant here, unlike the generic-hazard list's own split). New block in `GEInteractionSystem::Update()` right after the generic-hazard block, reusing `RollClear2Coinflip()`/`RequestClear2Ascend()`/`AppendExplosionFlash()`. 6 new `VerifyInteractionSystem` assertions (invincibility gate, lethal contact, object destroyed, death-lock requested with the real Clear1/Clear2 kind, `shouldRespawn=false`, `SmallShake` always fires). Full suite green both backends + live headless launch smoke check.

#### 7.7 Pickup Sounds

Per the channel corrections at the top of this section (treasure/key = ch11/19, egg = ch3, door
open = ch33, exit = ch14, generic hazard death = ch74, dynamite/projectile-fire = ch52, wasp
balloon = ch40/41), the line items below are corrected in place rather than reset blindly.

- [x] PICKUP-070 — Treasure collect sound wired: ch11 (ch19 if set-completing) (CNA, 2026-07-10) — corrected from the old "ch10 always restarts" assumption
- [x] PICKUP-071 — Key pickup sound wired: ch11 (ch19 if set-completing) (CNA, 2026-07-10) — same channel as treasure, corrected from old ch11-only assumption (still ch11, but conflict/set-completing behavior added)
- [x] PICKUP-072 — Egg pickup sound wired: ch3 (CNA, 2026-07-10) — corrected from the old "ch42" assumption
- [x] PICKUP-073 — Shield/Power/Hide/Cloud pickup sound — **correction 2026-07-13**: an earlier audit pass this same session wrongly marked this as NOT wired (it only checked `GEInteractionSystem.cpp`, where the ObjectType25/26/30/31 branches indeed call no `sound.Play()` — but the actual wiring lives one layer up, in `GalaxyEggbertCnaGame.cpp`'s `*GrantedThisFrame()` consumption, added in an earlier commit this session, `426ae1c`). Real channels, confirmed against `mobile-eggbert-reference/07-sounds.md`: Shield=**42**, Power=**44** (real Sucette-complete sound, reused since the 2-stage delay isn't modeled), Cloud=**55**, Hide=**62** — not the originally-guessed ch50.
- [x] PICKUP-074 — Win/exit sound — done, but real channel is **14** (not ch57) — `GEInteractionSystem.cpp`'s exit-reached branch; a separate ch13 plays when reaching the exit tile without enough treasures yet.
- [x] PICKUP-075 — Door open — done, real channel **33** (not ch7), duplicate of PICKUP-041 above.
- [x] PICKUP-076 — Switch activate/deactivate sound — **correction 2026-07-13**: same false-negative as PICKUP-073 (the earlier audit pass only checked `GEInteractionSystem.cpp`) — actually wired correctly in `GalaxyEggbertCnaGame.cpp`'s `TryActivateSwitch()` call site: ch77 on activate, ch76 on deactivate, exactly matching the original guess.
- [x] PICKUP-077 — dynamite-blast/bullet-wall-impact sound — done, but real channel is **10** (not ch40) for both the dynamite center-blast boom and a fired bullet hitting a solid wall; the originally-guessed "ch40 explosion" is actually the real wasp-balloon-entry channel (see ENEMY-039/PICKUP note), unrelated.
- [x] PICKUP-078 — Water plouf: ch23 — **done 2026-07-17**, verified directly against
      `Decor::MoveObjectPlouf()` (`Decor.cpp:6991-7003`). Spawns on the real dry->Surf/dry->Nage
      transition (`GalaxyEggbertCnaGame.cpp`'s water-status block, `wasDry && !nowDry`), gated to
      one active instance at a time (`GEInteractionSystem::HasActiveObjectOfType()`, matching the
      real pre-check exactly). Found and fixed alongside this: real channel 22 was actually wired
      to the WRONG transition (entry, not exit — see `SOUND-032`'s correction) and ch23 wasn't
      wired to anything. Also fixed `GEObjectIcons.cpp`'s `ObjectType14` icon formula (was a wrong
      monotonic-range approximation; real `table_plouf` oscillates 99->102->99, non-monotonic).
- [x] PICKUP-079 — Water bubble: ch24 — **done 2026-07-17**, verified directly against
      `Decor::MoveObjectBlup()` (`Decor.cpp:7027-7070`). Real ambient bubble spawned periodically
      while fully submerged (Nage), rising exactly as many tiles as the clear water column above
      Blupi (`GEInteractionSystem::SpawnWaterBubble()`, scans via `BlockTypes::isWater()`),
      self-deleting on arrival at the surface (folded into the shared `AdvancePatrolStep()`
      ObjectType23-arrival branch — real source treats both types identically there). Also fixed
      `GEObjectIcons.cpp`'s `ObjectType15` icon formula (was a wrong growing-range approximation;
      real `table_blup` is a 20-frame shuffle of just 4 icons, 103-106).
- [x] PICKUP-080 — Water small plouf: ch64 — **done 2026-07-17**, see `SOUND-074`'s writeup for
      the jump-exit-specific trigger and its documented approximation. Also fixed
      `GEObjectIcons.cpp`'s `ObjectType35` icon formula (was a wrong monotonic-range
      approximation; real `table_tiplouf` is only 3 frames, `{244,99,244}`).
- [ ] PICKUP-081 — Glu/glue sound: ch51 — NOT modeled (ObjectType34 has no dedicated implementation); real ch51 is actually the generic hazard-contact death sound used elsewhere (crusher/spike/etc. contexts), an unrelated reuse — the "glue" association in this line item appears to be a guess, not confirmed against source.
- [x] PICKUP-082 — Dynamite fuse sounds: ch52 (placement / explosions) — confirmed correct, matches Phase 15 `155`; also shared by blupih/blupit's projectile-fire sound (`FireBlupihShot()`), a real reused channel not specific to dynamite alone.
- [x] PICKUP-083 — Secret exit pickup: ch21 — **description was wrong**: there is no dedicated "ch21" secret-exit sound in real source — done as part of PICKUP-009, it reuses the SAME win (ch14)/reject (ch13) sounds as the regular exit, not a distinct channel.
- [x] PICKUP-084 — Bridge construction sound — **implemented 2026-07-13** alongside PICKUP-064:
      real channels are **72** (construction start) and **73** (mid-sequence progress cue at tick
      137), confirmed against `mobile-eggbert-reference/07-sounds.md` — not the originally-guessed
      ch20.
- [x] PICKUP-085 — "Balloon" pickup sound — **description was based on the wrong premise** (see PICKUP-019's correction: `ObjectType46` grants Overcraft, not a Balloon ride) — the real wasp-sting balloon-status sounds are channels **40/41** (entry/recovery), confirmed wired in `GalaxyEggbertCnaGame.cpp`, not ch46. No vehicle-mount-specific pickup sound was found for any of the 5 vehicles.
- [x] PICKUP-086/087 — **HALLUCINATED — CANCELLED (confirmed 2026-07-16)**: "Shield trail sound
  ch48"/"Shield loop sound ch49" don't exist in real source at all. Verified directly:
  (1) the real Shield-trail magic-particle VISUAL effect (`ObjectType57`, `m_blupiPosMagic`
  distance-triggered spawn, `Decor.cpp:5221-5236`) already IS implemented in this engine
  (`GEInteractionSystem::TickMagicTrail()`/`ResetMagicTrail()`, `kShieldTrack` in
  `GEObjectIcons.cpp`) — this pair of entries' own "no shield-trail visual... system exists"
  premise was already stale/wrong when written. (2) Real channels 48/49 have nothing to do with
  Shield at all — they're the `BlupiAction::Ouf3`/`Ouf4` idle-fidget reaction sounds
  (`Decor.cpp:6286-6304`, part of the same idle "fidget" system already flagged as a known,
  blocked-on-`AnimState`-values gap in `NEXT.md`). The real trail-spawn code itself
  (`Decor.cpp:5204-5256`, Power/Shield/Hide) has no `PlaySound()` call anywhere in it — no looped
  or one-shot trail sound exists for any of the 3. Not a gap to close.

---

### 2.8 Score & Progression

**HALLUCINATED — CANCELLED (confirmed by user 2026-07-14): SCORE-001 through SCORE-007 and the
"total score"/"new-record" parts of SCORE-012.** Grepped all of
`mobile-eggbert/src/WindowsPhoneSpeedyBlupi/` and `include/WindowsPhoneSpeedyBlupi/` for
`score`/`Score`/`m_score`/`GetScore`/`SetScore`/`AddScore` — no actual game-state score variable
exists anywhere. The only hits are UI label text (`TX_BUTTON_RANKING`,
`MyResource.cpp:110,317,453,587`) and a `Def::Phase::Ranking` screen name (`Def.hpp:65`,
`Game1.cpp:94,347-351`) — a gamer/profile stats screen name, not a numeric score mechanic.
`GameData.hpp`'s save-format class documents its entire byte layout explicitly (lines 12-39:
global header + per-gamer lives/lastWorld/200 door-state bytes) — no score field anywhere in 640
bytes of save data. The specific point values (10/25/50/50/50/100) were not traceable to any real
source at all. Confirmed invented — do not implement any numeric score mechanic. The rest of this
section (mission numbering/hub progression/save-linked progression/game speed, SCORE-008-011 and
SCORE-013 onward) is a SEPARATE, real category unrelated to the score hallucination — NOT
specifically re-verified this pass, treat as unconfirmed pending its own source check, not as
cancelled either.

Not started in CNA beyond what's implied by the interactive-object system's counting (treasures
collected, egg cap). No score numbers exist yet, and per the above, never will. All other items
reset to `[ ]`.

- [ ] SCORE-001 — ~~+10 score per treasure collected~~ **HALLUCINATED — CANCELLED, see note above.**
- [ ] SCORE-002 — ~~+25 score per enemy stomped~~ **HALLUCINATED — CANCELLED, see note above.**
- [ ] SCORE-003 — ~~+50 score per key collected~~ **HALLUCINATED — CANCELLED, see note above.**
- [ ] SCORE-004 — ~~+50 score per egg collected~~ **HALLUCINATED — CANCELLED, see note above** (mobile-eggbert: ch3 + life — channel corrected, see §7 note; that part is real, only the "+ score" part was invented)
- [ ] SCORE-005 — ~~+50 score per drink collected~~ **HALLUCINATED — CANCELLED, see note above.**
- [ ] SCORE-006 — ~~+100 bonus when all treasures collected~~ **HALLUCINATED — CANCELLED, see note above.**
- [ ] SCORE-007 — ~~High score per gamer slot persisted~~ **HALLUCINATED — CANCELLED, see note above (depends entirely on the non-existent score value).**
- [ ] SCORE-008 — Level elapsed timer displayed in HUD
- [x] SCORE-009 — Game speed selector — **done 2026-07-20.** Full research
      pass into real `InputPad.cpp:582-684` (`#ifdef MODERN`-gated, which is
      mobile-eggbert's own default/active build mode, not an unused legacy
      variant): F5=Normal/F6=Fast always work; F7=Faster/F8=Fastest need
      `quick_cheat_enabled` (see SCORE's own "quick" typed-cheat note
      below); Tab toggles Slow<->Normal; a real safety net drops back to
      Normal every frame if speed>Fast with neither quick/ghost/Shift-boost
      active. Real mechanism is discrete "N simulation ticks per rendered
      frame" (`Game1.cpp:394-416`, `for(i<ToRaw(speed)) decor.MoveStep()`,
      Slow = alternate-frame skip via a `slow_frame` toggle) — **user
      explicitly chose NOT to replicate this literally** (would mean
      repeating the whole ~1000-line Play-phase `Update()` block N times
      per frame, risking already-tuned once-per-frame sound/camera/HUD
      logic that assumes "runs exactly once"). Implemented instead as a
      continuous dt-scale factor (`GameSpeedFactor()`: Slow=0.5x/Normal=1x/
      Fast=2x/Faster=4x/Fastest=8x) applied to the 3 core simulation calls
      only (`worldRuntime_.Update`, `blupi_.Step`, `interaction_.Update`) —
      a deliberate, user-approved approximation, not a literal port. Real
      Shift-hold temporary boost-to-Fast(-or-Fastest) deliberately NOT
      ported: both LeftShift and RightShift are already this engine's own
      crouch/look-up controls, so there is no free Shift key without a real
      conflict — documented gap. Verified live under Xvfb via a temporary,
      since-reverted env-var hook: `simDt`/`animPhase` advance at exactly
      0.5x/1x/2x for Slow/Normal/Fast. Also added the real on-screen speed
      indicator (`InputPad.cpp:1383-1404`: "0.5x"/"Nx" text over a
      `pad.png` icon-15 panel, shown only while speed != Normal).
- [x] SCORE-010 — GameSpeed::Faster and GameSpeed::Fastest modes — **done
      2026-07-20, see SCORE-009.** Reachable via F7/F8 once the real
      "quick" typed cheat (extends the existing ghost-cheat rolling
      buffer, `GEInputPad::UpdateTypedGhostCheat()` → `TypedCheatResult`)
      is typed — matches real `quick_cheat_enabled`'s gate on F7/F8 exactly.
- [x] SCORE-011 — Slow game speed — **done 2026-07-20, see SCORE-009.**
      Real alternate-frame-skip mechanism approximated as a continuous
      0.5x dt-scale factor (same average simulation rate, no frame-parity
      bookkeeping needed) — user-approved simplification, not a literal
      port of the real `slow_frame` toggle.
- [ ] SCORE-012 — ~~Win screen: display total score~~, elapsed time, new-record indicator —
      **the "total score"/"new-record" parts are HALLUCINATED — CANCELLED** (see note above);
      "elapsed time" display itself is unrelated to the score hallucination and stays open pending
      its own check (see SCORE-008).
- [x] SCORE-013 — Mission numbering: world hub (X0) → levels (X1-X5) → next
      hub ((X+1)0) — **done 2026-07-17, extended to FULL real scope the
      same day** (initial 3-world proof, then expanded on explicit user
      request to match mobile-eggbert's actual world/hub count exactly).
      Full hub/mission-progression system, verified directly against
      `Decor.cpp`'s real `Bye`/`Win`-action mission handlers, `Game1::
      MissionBack()`, and `Decor::AdaptDoors()`: global hub (mission 1) →
      world-select marker N → world hub (mission N*10) → level-select
      marker N → sublevel (mission `X0+N`) → exit reached/`PauseBack`/
      `PauseRestart` → back to `(mission/10)*10`, or to the global hub if
      already at a hub. New `GEWorldRuntime::ComputeWorldSelectTarget()`/
      `ComputeMissionBack()`/`ComputeWinExitTarget()` (all pure, unit-
      tested — the last one is the DISTINCT real win-exit formula,
      `Decor.cpp:6411-6434`, which additionally special-cases mission 1's
      own exit → the real final bonus world 199, and mission 199's own
      exit → looping back to the global hub, this engine's simplification
      of the real true-ending sentinel since no distinct "game complete"
      screen exists), `GalaxyEggbertCnaGame::LoadMission(int)` (loads
      `worlds3d/world{N:03d}.vwr`, rebuilds terrain/background, resets
      `blupi_`/`interaction_` to fresh per-level defaults preserving only
      lives — real `PlayPrepare()`'s exact reset scope), and 12 `BlockTypes`
      world-select constants (`Sp0`-`Sp7` renamed + extended to
      `WorldSelect1`-`12` — the real icons 158-165 already correctly
      identified at `170`/`TILE-057` but never wired to anything, extended
      with 4 more repurposed icons to cover all 12 real world hubs).
      **Real progress-gated doors also implemented** (`Decor::AdaptDoors()`'s
      `m_mission%10==0` branch, `SearchDoor()`/icon `182`, confirmed
      directly against `worlds/world010.txt`'s own real sign+door layout):
      within each world hub, sublevel-select marker 1 is always open
      (matches real source exactly — no adjoining door tile for sign 174 in
      any real world-hub file); markers 2-8 each sit behind a real solid
      door (new `BlockTypes::ProgressDoor2`-`8`) that opens once the
      PRECEDING sublevel has been won — new `GESaveData::
      IsMissionDoorUnlocked()`/`UnlockMissionDoor()` (a per-gamer-slot
      persisted flag array keyed directly by mission number, mirroring real
      `m_doors[mission]` exactly, only the un-modeled per-world cosmetic-
      gold half of the real array is skipped, see `SAVE-006`/`007`'s own
      updated note), applied at `LoadMission()`-time (matches real
      `AdaptDoors()` running inside `StartMission()`, before the level is
      ever shown — no live open-animation needed) and set at the win-exit
      handler (mirrors real `Decor::OpenDoorsWin()`'s `m_doors[mission+1]=1`
      exactly).
      **Full real world/hub scope, not just a proof-of-mechanism**: all 78
      real mobile-eggbert world files now exist here too, with byte-for-
      byte identical filenames/mission numbers — 1 global hub (`world001`),
      12 world hubs (`world010`/`020`/.../`120`, real per-world sublevel
      counts: 4,5,4,6,8,6,5,4,5,7,5,5), 64 minimal placeholder sublevels
      (small floor + real exit marker each), and `world199` (the real final
      bonus world, likewise a minimal placeholder). Per explicit user
      instruction, every NEW world stays a near-empty placeholder until a
      real 3D world editor exists to flesh them out.
      **`world001`/`world999` split (2026-07-17, explicit user request)**:
      `world001.vwr`'s pre-existing rich demo/mechanics-showcase content
      (dynamite/crates/vehicles/water/doors/every tile-icon exhibition/etc.,
      built earlier this session) has been moved to a genuine, engine-
      specific **79th world**, `world999.vwr` — beyond mobile-eggbert's real
      78, purely for this engine's own dev/test use, never confused with a
      real mission number. `world001.vwr` itself is now the LEAN real
      global hub: an 18x18 plaza (grid 41-58) holding exactly the 12
      `WorldSelect` portals + the real `ObjectType7` exit (→ mission 199)
      and nothing else, matching real mobile-eggbert's own global hub scope
      exactly (no extra content). A new `BlockTypes::DemoPortal` (icon 177,
      engine-specific, not a real mobile-eggbert concept) sits in the
      global-hub plaza and leads directly to mission 999 — the only way to
      reach the demo/test world in-game, reusing the exact same contact-
      trigger/no-debounce portal-touch handling as `WorldSelect`. Every
      `VerifyBlupiMovement`/`VerifyBigDecorParsingCna`/`VerifyInteractionSystem`
      default world path updated `world001.vwr`→`world999.vwr` accordingly.
      **World-hub plaza enlarged 2026-07-17** (explicit user request — "at
      least 10x10, enlarge the floors so everything fits comfortably"):
      `GenerateWorldHub()`'s spawn area grew from an 11x11 floor to an
      18x18 plaza (matching the global hub's own size), holding the always-
      open marker 1; markers 2+ still extend east along the existing tested
      3-wide gated corridor (wall+`ProgressDoorN`+marker triplets), unchanged
      mechanism, just attached to a bigger plaza instead of a bare corridor
      start. Confirmed via the full existing test suite plus a fresh live
      headless run (see below) that both the enlarged plazas and the
      `DemoPortal` round-trip work correctly.
      **Known, deliberate divergence**: every level's exit still shows this
      engine's own Win screen (pulsing `blupiyoupie.png` + Return button)
      before advancing — real source only shows a genuine Win screen for
      the true final mission (199); every other level completion silently/
      instantly reloads the next mission with no screen at all. Keeping
      the Win screen for every completion is an intentional, already-
      built-and-tested UX choice from earlier this session, not a new gap.
      Real per-level hidden-secret mechanic (`m_bFoundCle`, an alternate
      "found the hidden gold → straight back to the global hub" win-exit
      branch) is NOT modeled — a per-level hidden-pickup mechanic, out of
      scope here, same "not attempted" precedent as every other hidden/
      secondary mechanic this session.
      New tests: `VerifyInteractionSystem` (mission-math table incl.
      `ComputeWinExitTarget()`, `isWorldSelect()`/`worldSelectIndex()` at
      the extended 1-12 range, `isProgressDoor()`/`progressDoorIndex()`,
      all 3 originally-proven worlds still load with the right
      `missionNumber()`), `VerifyGESaveData` (door-unlock bitset round-
      trips through Save()/Load() correctly, per-mission not per-neighbor).
      Live headless verification: the full navigation chain (mission 1 →
      portal → mission 10 → marker → mission 11) AND the door-gate
      mechanism itself (fresh world010's marker-2 door starts closed;
      `UnlockMissionDoor(12)` + reload opens exactly that door, leaves
      marker-3's door still closed) — both confirmed via stdout
      diagnostics, temporary instrumentation reverted before commit. Full
      regression clean both times (only the pre-existing unrelated
      `easy-gl-resource-smoke-tests` failure). Re-verified again 2026-07-17
      after the `world001`/`world999` split and plaza enlargement: temporary
      debug instrumentation confirmed `world001.vwr` (mission 1) has
      exactly 12 `WorldSelect` markers + 1 `DemoPortal`, `LoadMission(999)`
      correctly loads the demo world (0 `WorldSelect`/`DemoPortal`, as
      expected — the exhibition excludes those icons), the enlarged
      `world010.vwr` (mission 10) shows its correct 4 `WorldSelect`
      markers, and reloading mission 1 afterward round-trips cleanly —
      reverted before commit; full regression clean (same single known
      pre-existing failure).
- [x] SCORE-014 — 78 world files (world001.txt … world055.txt + hubs)
      supported — **done 2026-07-17**: all 78 real mobile-eggbert world
      files now exist here too, with identical filenames/mission numbers
      and the real per-mission filename FORMULA (`world{N:03d}.vwr`)
      genuinely used by `LoadMission()` (see `SCORE-013`'s full writeup).
      Every world beyond `world001` is a minimal placeholder, not real
      themed level content — see `TILE-005`'s own scope note (fleshing out
      real level content for all 78 is separate, much larger work, blocked
      on a real 3D world editor not existing yet).
- [x] SCORE-015 — IsTerminated: -1=lost, -2=win, ≥1=advance to mission N —
      **the ≥1 case is done 2026-07-17** (`ComputeWorldSelectTarget()`/
      `ComputeMissionBack()` feed `LoadMission()` directly). -1/-2
      (Lost/true-final-Win) were already correctly handled by the existing
      phase system (`HUD-023`) before this session; not re-touched.
- [x] SCORE-016 — Mission advance: m_term = m_mission/10*10 + next_level_in_world
      — **done 2026-07-17**, see `SCORE-013` (`ComputeWorldSelectTarget()`
      is the exact real formula, contextually applied depending on whether
      the current mission is the global hub or a world hub).
- [x] SCORE-017 — Hub mission (mission % 10 == 0): no treasure counter in
      HUD — **wired 2026-07-20**: real gate confirmed directly against
      `Decor.cpp:1236`, `(m_mission != 1 && m_mission % 10 != 0) ||
      m_bPrivate` (`m_bPrivate` never applies here, no custom-level-load
      path exists). New `showTreasureCounter` parameter on `GEHud::Draw()`,
      computed at the call site the same way the Pause-menu's own
      `showRestart` already does (`mission != 1 && mission % 10 != 0`).
      Previously approximated as just "does this world have any
      treasures" -- true in practice since hub worlds have none, but not
      the real gate.
- [x] SCORE-018 — Training missions (11-14): show tutorial hint overlay —
      **stale checkbox, closed 2026-07-17**: already fully done, see
      `HUD-024`/`MENU-080..082` (`GETrainingHints`, gated on exactly
      missions 11-14) — this entry just hadn't been cross-referenced here.
- [x] SCORE-019 — MemorizeGamerProgress: save lives and doors after each
      win/loss — **lives half done, doors half still correctly not
      modeled**: `saveData_.Save()` at Win/Lost checkpoints already
      persisted lives+mission (`MENU-019`/`020`/`067`); as of 2026-07-17
      the mission number is now also genuinely READ BACK on Resume
      `CONTINUE` (`saveData_.GetMissionNumber()` → `LoadMission()`), closing
      the previously write-only half of this gap. Real per-gamer door-
      unlock state (`MemorizeDoors`/`InitializeDoors`) is still correctly
      not modeled — see `SAVE-006`/`007`'s own note (no per-gamer 200-door-
      flags array exists in `GESaveData` by design).
- [ ] SCORE-020 — LastWorld: updated when completing a hub (mission divisible by 10)

---

### 2.9 Sound System

CNA has real substance here: `GESound` loads all 93 real .wav files via CNA's own
`SoundEffect`/`SoundEffectInstance` API, reuses the real per-channel volume/conflict table, and is
wired to jump/land/footstep events. Not done: pitch application, `SoundEnviron()` terrain-specific
footstep/bump remapping (7 terrain pairs, channels 78-91), idle "fidget" periodic sounds (channels
36/37/46-49/65), buff activate/expire-warning channel pairs.

**Spot-checked against source 2026-07-13** (lighter pass than §2.6/§2.7 above — many of this
section's original per-channel guesses were unconfirmed speculation to begin with, not stale
completed-work; only channels directly cross-verified while auditing §2.6/§2.7 were corrected
below, the rest are unchanged/still genuinely unconfirmed).

- [x] SOUND-001 — 93 WAV files (`sounds/sound000.wav`..`sound092.wav`) loaded (CNA, 2026-07-10)
- [x] SOUND-002 — Per-channel volume/conflict table reused from the real data (CNA, 2026-07-10)
- [x] SOUND-003 — Sound on/off toggle — **done, corrected 2026-07-16**: this entry's own premise
  was wrong — real behavior (persisted mute toggle, `GESound::SetEnabled()`/`IsEnabled()`) doesn't
  need GameData byte-compatibility to work, just persistence at all. Already implemented via this
  engine's own independent `GESaveData` (plan.md MENU-067, `GESaveData::GetSoundEnabled()`/
  `SetSoundEnabled()`, wired to `GESound::SetEnabled()` on load).
- [x] SOUND-004 — Sound loop support via `SoundEffectInstance` (CNA, 2026-07-10)
- [ ] SOUND-005 — Positional (panned) audio: volume/balance based on screen X position (SoundEnviron)
- [x] SOUND-006 — SoundEnviron: maps ch3/ch4 footstep to tile-surface variant (ch78-91) — **stale,
  corrected 2026-07-16**: this was already done 2026-07-12 (`GESound::FootstepChannelFor()`,
  covers all 7 real terrain ranges: 78/80/82/84/86/88/90) — this entry was never updated
  afterward.
- [x] SOUND-007 — Vehicle motor loop: ch16/ch18 (helicopter high/low), ch29/ch31 (jeep/tank/over)
      — **done 2026-07-17**, see `SOUND-008`'s writeup for the full implementation.
- [x] SOUND-008 — Motor sound crossfade: start sound (ch15/ch28) + stop sound (ch17/ch30) —
      **done 2026-07-17**, verified directly against `Decor::AdaptMotorVehicleSound()`
      (`Decor.cpp:1599-1639`): a real per-vehicle-mode crossfade — Helicopter gets ch15
      (start)/ch16-18 (loop high/low)/ch17 (stop); Jeep/Tank/Overcraft share ch28 (start)/
      ch29-31 (loop high/low)/ch30 (stop); Skateboard has no motor sound at all (confirmed: the
      real function's own `if`/`else if` chain checks only the other 4 modes). New
      `GEBlupiController::HasVehicleMotor()`/`IsVehicleMotorHigh()` (the real per-mode
      `m_blupiMotorHigh` pitch-select flag — Jeep uses `m_blupiAction != BlupiAction::Stop`,
      i.e. nonzero horizontal velocity including coasting, translated here as nonzero
      `m_vehicleSpeed`; Helicopter/Overcraft translated as nonzero vertical `m_velocityY`, the
      flight-mode equivalent) + `GalaxyEggbertCnaGame::UpdateVehicleMotorSound()` (a direct port
      of the real crossfade state machine, called once per frame, `activeMotorLoop_` mirroring
      the real `m_blupiMotorSound` sentinel). 4 new `VerifyBlupiMovement` assertions
      (`HasVehicleMotor()` per mode including the Skateboard exception, `IsVehicleMotorHigh()`
      idle-vs-moving for both a ground vehicle and a flight vehicle). Full regression clean (only
      the pre-existing unrelated `easy-gl-resource-smoke-tests` failure); live headless launch
      smoke check clean.
- [ ] SOUND-009 — PosSound: update panned position of active motor loop each frame — **NOT
      modeled, deliberately**: this engine's `GESound::Play()` has no positional/panned audio at
      all (same architectural gap as `SOUND-005`) — there is no pan state to update per frame.
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
- [ ] SOUND-012 — ch2: unknown — **researched 2026-07-16**: grepped every real `.cpp` file, not
  just `Decor.cpp` — zero uses of `SoundChannel2` anywhere in real mobile-eggbert source. Not a
  research gap, genuinely unused/reserved in the real game itself; nothing to wire.
- [x] SOUND-013 — ch3: footstep AND/OR egg pickup — note: ch3 is used for both footstep (BLUPI-129) and egg pickup (PICKUP-072) in the real data; confirm both wire-ups are distinct calls, not a collision (CNA, 2026-07-10 for footstep)
- [x] SOUND-014 — ch4: landing (CNA, 2026-07-10)
- [ ] SOUND-015 — ch5: stomp kill — **HALLUCINATED, corrected 2026-07-16**: same already-cancelled
  premise as `BLUPI-131`/`ENEMY-036` — no velocity-gated "stomp" concept exists anywhere in real
  source; enemy contact-kill is an unconditional touch check. This is the 3rd copy of the same
  debunked claim in this file, missed when the other 2 were cancelled. Not blocked on "enemy hit
  detection" (that already exists and is unconditional) — there is simply no real stomp mechanic
  to wire a sound to.
- [ ] SOUND-016 — ch6: unknown — **identified 2026-07-16**: verified directly against
  `Decor.cpp:4907-4943` — real ch6 is the `AscenseurVertigo` edge-teeter sound (entering
  `BlupiAction::Vertigo`/`Advance` on a wide/shiftable lift). Genuinely blocked on the same
  render-geometry decision as `PICKUP-024`/`027` — not independently implementable.
- [x] SOUND-017 — ch7: door open — **corrected 2026-07-16, wired 2026-07-20**: label is wrong.
  Verified directly against `Decor.cpp:3283-3286` — real ch7 plays on the `Down`(crouch)-action
  transition (screen scrolls down 150px), not door open (that's ch33, see `SOUND-043`). Now wired
  under the correct real meaning: new `GEBlupiController::DownEntrySoundFiredThisFrame()` (fires
  once, real `Config::ScaleTime(4)`=0.2s after entering `Down`), consumed in
  `GalaxyEggbertCnaGame.cpp` right after `blupi_.Step()`.
- [x] SOUND-018 — ch8: death / hit — **stale, corrected 2026-07-16**: already wired (Lava/Blitz/
  fall-death `triggerDeath()` calls in `GalaxyEggbertCnaGame.cpp`), label is accurate.
- [x] SOUND-019 — ch9: teleport in — **label wrong AND stale, corrected 2026-07-16**: verified
  directly against `Decor.cpp:10173-10177` — real ch9 is the life-loss Voyage sound (icon 48/Blupi
  channel, `m_nbVies--`), not "teleport in". Already wired
  (`GEInteractionSystem::BeginVoyage()`'s `VoyageKind::LifeLoss` case) under the correct real
  meaning.
- [x] SOUND-020 — ch10: **corrected 2026-07-13** — real use is dynamite-blast center boom AND a fired bullet hitting a solid wall, not "collect" (confirmed in `GEInteractionSystem.cpp`).
- [x] SOUND-021 — ch11: key pickup AND treasure pickup (CNA, 2026-07-10 — corrected: shared by both per real data)
- [x] SOUND-022 — ch12: teleport out — **label wrong AND stale, corrected 2026-07-16**: verified
  directly against `Decor.cpp:10179-10182` — real ch12 plays on egg-pickup Voyage completion
  (`m_voyageIcon==21 && Element channel`), not "teleport out". Already wired
  (`GEInteractionSystem::BeginVoyage()`'s `VoyageKind::Egg` case) under the correct real meaning.
- [x] SOUND-023 — ch13: **corrected 2026-07-13** — real use is reaching the level exit tile without enough treasures collected yet, not "bridge build phase 1" (confirmed in `GEInteractionSystem.cpp`).
- [x] SOUND-024 — ch14: **corrected 2026-07-13** — real use is exit-reached/win (`exitReached_`), not "bridge build phase 2" (see PICKUP-074).
- [x] SOUND-025 — ch15: helicopter motor start — **done 2026-07-17**, see `SOUND-008`.
- [x] SOUND-026 — ch16: helicopter motor high (loop) — **done 2026-07-17**, see `SOUND-008`.
- [x] SOUND-027 — ch17: helicopter motor stop — **done 2026-07-17**, see `SOUND-008`.
- [x] SOUND-028 — ch18: helicopter motor low (loop) — **done 2026-07-17**, see `SOUND-008`.
- [x] SOUND-029 — ch19: treasure/key pickup, set-completing variant (CNA, 2026-07-10 — new correct meaning, was previously listed as generic "teleport (alternate)")
- [x] SOUND-030 — ch20: bridge completed — **label wrong, corrected 2026-07-16, wired 2026-07-20**:
  verified directly against `Decor.cpp:3628-3633` — real ch20 plays when standing back up from a
  crouch (`Down`→`Stop` transition), not "bridge completed". Now wired: new
  `GEBlupiController::DownReleaseSoundFiredThisFrame()`, fires exactly on that transition -- real
  gate is `m_blupiSpeedX==0 && m_blupiSpeedY==0`, which needed no separate modeling since this
  engine's own `crouchHeld` ternary only ever resolves to `Stop` (not `March`) under the same
  condition; verified a `Down`→`March` transition (crouch released WITH movement held) does NOT
  fire it, matching the real gate.
- [x] SOUND-031 — ch21: secret exit found — **label wrong, corrected 2026-07-16, wired
  2026-07-20**: verified directly against `Decor.cpp:3278-3282` — real ch21 is the look-up ("Up"
  action) transition sound (paired with ch7's look-down/crouch transition, `SOUND-017`), not
  "secret exit found" (no such distinct sound exists at all — confirmed via `PICKUP-083`'s own
  research, the secret exit reuses the regular exit's ch13/ch14). Now wired: new
  `GEBlupiController::UpEntrySoundFiredThisFrame()`, same real 0.2s-delay shape as `SOUND-017`.
- [x] SOUND-032 — ch22: **corrected again 2026-07-17** (the 2026-07-16 pass had it backwards) —
  verified directly against all 4 real `PlaySound(SoundChannel22, ...)` call sites
  (`Decor.cpp:5356/5378/5392/5405`): every one is in a water-EXIT path (jump-out, transport-out,
  drift-out onto shore), none in entry. `GalaxyEggbertCnaGame.cpp`'s water-status block was
  wired backwards (played on entry, `wasDry && !nowDry`) — fixed to fire on exit
  (`!wasDry && nowDry`) instead.
- [x] SOUND-033 — ch23: water plouf — **done 2026-07-17**, alongside the ch22 fix above. Real
  entry splash (`Decor::MoveObjectPlouf`, `ObjectType14`) was previously entirely unwired (this
  was ch22's actual real meaning, confused with the exit sound above). Now spawns via
  `GEInteractionSystem::SpawnWaterSplash()` on the real dry->Surf/dry->Nage transition
  (`wasDry && !nowDry`), single-instance-gated (`HasActiveObjectOfType()`).
- [x] SOUND-034 — ch24: water bubble rise — **done 2026-07-17**. Real ambient bubble
  (`Decor::MoveObjectBlup`, `ObjectType15`) while fully submerged (Nage), triggered twice per
  ~3.5s cycle (`m_time % ScaleTime(70) == 0 || == ScaleTime(28)`, reproduced via this engine's
  own `GEWorldRuntime::GetAnimPhase() % 70`). New `GEInteractionSystem::SpawnWaterBubble()` scans
  the water column above Blupi (`BlockTypes::isWater()`) and spawns a bubble that rises exactly
  that many tiles, self-deleting on arrival (folded into the existing `AdvancePatrolStep()`
  ObjectType23-arrival branch, matching real source's identical treatment of both types at that
  exact junction).
- [x] SOUND-035 — ch25: **stale, corrected 2026-07-16** — real use is the start-surfing/
  resurfacing sound (Nage→Surf transition specifically, distinct from ch22's general water-entry
  splash). Already wired.
- [x] SOUND-036 — ch26: **stale, corrected 2026-07-16** — real use is the drowning death sound (a
  dedicated channel distinct from every other death cause's ch8/51/75, per `07-sounds.md`).
  Already wired.
- [ ] SOUND-037 — ch27: unknown — **corrected 2026-07-13**: the real projectile-fire channel is ch52, not ch27 (see SOUND-062); this line item's original guess is unconfirmed against source.
- [x] SOUND-038 — ch28: jeep/tank start — **done 2026-07-17, and also covers Overcraft** (the real
      function shares this channel set across all 3), see `SOUND-008`.
- [x] SOUND-039 — ch29: jeep/tank motor high (loop) — **done 2026-07-17**, see `SOUND-008`.
- [x] SOUND-040 — ch30: jeep/tank stop — **done 2026-07-17**, see `SOUND-008`.
- [x] SOUND-041 — ch31: jeep/tank motor low (loop) — **done 2026-07-17**, see `SOUND-008`.
- [x] SOUND-042 — ch32: **done 2026-07-17** (was "identified 2026-07-16, out of scope" — the Bye
  animation itself is now implemented, see `BLUPI-049`): verified directly against
  `Decor.cpp:5476-5489` — real ch32 is the hub-screen world-select entry sound (`Decor::IsWorld()`
  match, `BlupiAction::Bye`), now played (`SoundChannel32`) at the world-select portal contact site
  the instant `TriggerBye()` succeeds.
- [x] SOUND-043 — ch33: **corrected 2026-07-13** — real use is door open (confirmed in `GEInteractionSystem.cpp`'s `OpenDoorAt()`), not "bulldozer turn" — no bulldozer-turn sound exists.
- [ ] SOUND-044 — ch34: unknown — **identified 2026-07-16**: verified directly against
  `Decor.cpp:5440-5453` — real ch34 is the Suspended (hanging-on-a-bar) mode's entry sound.
  Genuinely blocked on the same render-geometry decision as `AscenseurVertigo`/`Suspended`
  (`mobile-eggbert-reference/`'s own "Suspended mode... blocked on a pending render-geometry
  decision" note) — not independently implementable.
- [ ] SOUND-045 — ch35: unknown — **identified 2026-07-16**: verified directly against
  `Decor.cpp:4780-4790` — real ch35 is the Suspended mode's jump-off/release sound. Same
  render-geometry blocker as `SOUND-044` above.
- [ ] SOUND-046 — ch36: water walk ambient (idle fidget channel, see SOUND-010c)
- [ ] SOUND-047 — ch37: swim bubble (idle fidget channel, see SOUND-010c)
- [x] SOUND-048 — ch38: electric arc (long) — **done, label wrong, corrected 2026-07-16**: real
  use is the crate-push loop sound (verified directly against `Decor.cpp:6138/6147` start,
  `:3637` stop on leaving `BlupiAction::Push`), not "electric arc". Now implemented: new
  `GESound::Stop(channel)` (a per-channel stop, since `StopAll()` would incorrectly kill every
  other playing sound), new `GEInteractionSystem::CrateBeingPushedThisFrame()` signal (true the
  one frame a push actually moves a crate), and `GalaxyEggbertCnaGame::wasPushingCrate_` turning
  that per-frame fact into a real start-once/stop-once loop (`sound_.Play(ch38, loop=true)` on
  the false→true transition, `sound_.Stop(ch38)` on true→false). New `VerifyInteractionSystem`
  assertions. Full suite green both backends + live headless launch smoke check.
- [ ] SOUND-049 — ch39: key sparkle effect — **label wrong, corrected 2026-07-16**: verified
  directly against `Decor.cpp:3291-3324` — real ch39 is the crate-"Pop" sound (landing on top of a
  crate from above, `BlupiAction::Pop`/`StopPop`), not "key sparkle". Genuinely blocked on the
  same crate fall-physics prerequisite as `151`/`PICKUP-035` — no vertical crate-landing event
  exists in this engine to hook a sound onto.
- [x] SOUND-050 — ch40: **corrected 2026-07-13** — real use is wasp balloon-status entry (Phase 13 `135`), not "explosion".
- [x] SOUND-051 — ch41: **corrected 2026-07-13** — real use is wasp balloon-status recovery/expiry (shared with Crusher's own recovery cue), not "glide".
- [x] SOUND-052 — ch42: **corrected 2026-07-13** — real use is Shield power-up activation (`ObjectType25`, confirmed against `mobile-eggbert-reference/07-sounds.md` + wired in `GalaxyEggbertCnaGame.cpp`), not "life/drink pickup" — see PICKUP-073.
- [x] SOUND-053 — ch43: **stale, corrected 2026-07-16** — real use is the Shield secret-power
  warning-threshold sound (@10 levels remaining, `JustCrossedSecretPowerWarning()` in
  `GalaxyEggbertCnaGame.cpp`). Already wired.
- [x] SOUND-054 — ch44: **label wrong, corrected 2026-07-16** — real use is the Sucette/Power
  pickup's real 2-stage-delay COMPLETION sound (`ResolvePickupFreeze()`'s
  `PickupFreezeKind::Sucette` case), not "shield off". Already wired.
- [x] SOUND-055 — ch45: **stale, corrected 2026-07-16** — real use is the Power secret-power
  warning-threshold sound (@20 levels remaining). Already wired.
- [ ] SOUND-056 — ch46: balloon mode sound (idle fidget channel, see SOUND-010c)
- [ ] SOUND-057 — ch47: suspend attach (idle fidget channel, see SOUND-010c)
- [ ] SOUND-058 — ch48: shield sparkle (idle fidget channel, see SOUND-010c) — **label wrong**:
  ch48 is actually the `Ouf3` idle-fidget sound (see the cancelled `PICKUP-086`/`087` entry), not
  shield-specific at all; still correctly categorized as idle-fidget/not done.
- [ ] SOUND-059 — ch49: shield loop (looped while active) (idle fidget channel, see SOUND-010c) —
  **label wrong**: ch49 is actually the `Ouf4` idle-fidget sound, same correction as ch48 above.
- [x] SOUND-060 — ch50: **stale, corrected 2026-07-16** — this entry's own "ch50 itself is unused
  here" claim (written 2026-07-13) is now stale: the real Sucette pickup-*start* ("grab") sound IS
  wired as of the `173` 2-stage-delay work (2026-07-14), exactly at the real contact-time trigger
  point this entry originally described. Label/citation were already correct, only the "unused"
  status needed updating.
- [x] SOUND-061 — ch51: **corrected 2026-07-13** — real use is the generic hazard-contact death sound (confirmed in `GEInteractionSystem.cpp`), not "glu/glue splash" — no glue-specific sound found.
- [x] SOUND-062 — ch52: confirmed correct — dynamite placement/explosion; also reused for blupih/blupit's real projectile-fire sound (see SOUND-037's correction).
- [x] SOUND-063 — ch53: **label wrong, corrected 2026-07-16** — real use is the real out-of-ammo
  click when Fire is pressed at 0 bullets (`GEInteractionSystem.cpp`'s Tank-fire block, `else`
  branch), not "tank fire" itself (that's ch52, see `SOUND-062`). Already wired under the correct
  meaning.
- [x] SOUND-064 — ch54: **label wrong, corrected 2026-07-16** — real use is the BulletPack Voyage
  completion sound (`VoyageKind::BulletPack` case), not "long explosion (creature death?)".
  Already wired.
- [x] SOUND-065 — ch55: **stale, corrected 2026-07-16** — real use is the Charge pickup's real
  2-stage-delay COMPLETION sound (`PickupFreezeKind::Charge` case) — already correctly
  cross-referenced by `SOUND-068`'s own note, just never marked done itself. Already wired.
- [x] SOUND-066 — ch56: **stale, corrected 2026-07-16** — real use is the Cloud secret-power
  warning-threshold sound (@25 levels remaining). Already wired.
- [x] SOUND-067 — ch57: **label wrong, corrected 2026-07-16** — real use is the Drink pickup's
  real pickup-*start* ("grab") sound, not "exit open / win" (that's ch14, see `SOUND-024`).
  Already wired.
- [x] SOUND-068 — ch58: **corrected 2026-07-13** — real use is the Charge/Cloud (`ObjectType31`) power-up's real pickup-*start* sound, not "drink pickup" (per `mobile-eggbert-reference/07-sounds.md`); this engine plays ch55 instead at grant time (see PICKUP-073) — ch58 itself is unused here.
- [x] SOUND-069 — ch59: **corrected 2026-07-13** — Cloud secret-power `BlupiElectro` electric-aura kill sound (added 2026-07-13, `GEInteractionSystem.cpp`).
- [x] SOUND-070 — ch60: **confirmed 2026-07-16** — real use is the Perso/Dynamite Voyage
  completion sound (`VoyageKind::Perso`/`Dynamite` cases). Already wired.
- [x] SOUND-071 — ch61: **stale, corrected 2026-07-16** — real use is the Dynamite/Perso
  placement sound (shared, per `07-sounds.md` — see the code's own citation in
  `GalaxyEggbertCnaGame.cpp`). Already wired.
- [x] SOUND-072 — ch62: **label imprecise, corrected 2026-07-16** — real use is specifically the
  Drink pickup's real 2-stage-delay COMPLETION sound (`PickupFreezeKind::Drink` case), not
  Sucette's (that's ch44, see `SOUND-054`). Already wired.
- [x] SOUND-073 — ch63: **stale, corrected 2026-07-16** — real use is the Hide secret-power
  warning-threshold sound (@20 levels remaining). Already wired.
- [x] SOUND-074 — ch64: small water plouf — **done 2026-07-17**, alongside `SOUND-032/033/034`
      above. Real "Tiplouf" (`ObjectType35`), the jump-triggered exit's extra splash on top of
      ch22 — this engine approximates "deliberate jump exit" (vs. passive drift-out, which plays
      only ch22) as `jumpPressed` being true at the exact water-exit frame, a documented
      simplification of this engine's single-point collision model (see `GalaxyEggbertCnaGame.cpp`'s
      own comment for the full reasoning).
- [ ] SOUND-075 — ch65: suspend detach / rope release (idle fidget channel, see SOUND-010c) —
  **label wrong, corrected 2026-07-16**: verified directly against `Decor.cpp:3147-3168` — real
  ch65 is the `Mockery`/`Mockeryi` idle-taunt-animation sound, not "suspend detach". Correctly
  categorized as an idle-fidget channel either way (same blocker as `SOUND-010c`).
- [x] SOUND-076 — ch66: **stale, corrected 2026-07-16** — real use is the Mirror/Invert activate
  sound (confirmed against `mobile-eggbert-reference/13-object-pickups.md`). Already wired.
- [x] SOUND-077 — ch67: **stale, corrected 2026-07-16** — real use is the Mirror/Invert expire
  sound. Already wired.
- [ ] SOUND-078 — ch68: unknown — **researched 2026-07-16**: grepped every real `.cpp` file --
  zero uses of `SoundChannel68` anywhere in real mobile-eggbert source. Genuinely unused/reserved
  in the real game itself, same as `SOUND-012`; nothing to wire.
- [x] SOUND-079 — ch69: lightning strike — **done 2026-07-17**, see `VISUAL-024`'s writeup for the
      full implementation (`GEInteractionSystem`'s lazy Blitz/BlitzEmitter world scan).
- [x] SOUND-080 — ch70: **stale, corrected 2026-07-16** — real use is the Crusher squash-entry
  sound (`TriggerCrush()` succeeding). Already wired.
- [x] SOUND-081 — ch71: **stale, corrected 2026-07-16** — real use is the Teleporter entry sound
  (`TriggerTeleport()` succeeding). Already wired.
- [x] SOUND-082 — ch72: **label wrong, corrected 2026-07-16** — real use is the bridge-
  construction START sound (a new `ObjectType52` bridge spawning), not "wasp approach". Already
  wired.
- [x] SOUND-083 — ch73: **label wrong, corrected 2026-07-16** — real use is a bridge-construction
  PROGRESS sound (fires at a fixed tick partway through the build sequence), not "wasp attack".
  Already wired.
- [x] SOUND-084 — ch74: **corrected 2026-07-13** — real use is the generic hazard/enemy contact-kill death sound (confirmed repeatedly in `GEInteractionSystem.cpp`), not "teleport in".
- [x] SOUND-085 — ch75: **label wrong, corrected 2026-07-16** — real use is the Saw death
  "cut apart" sound (`BlupiDead()`'s Clear4 branch, `SpawnSawDeathBurst()`), not "teleport out".
  Already wired.
- [x] SOUND-086 — ch76: switch deactivate — confirmed correct, see PICKUP-076.
- [x] SOUND-087 — ch77: switch activate — confirmed correct, see PICKUP-076.
- [ ] SOUND-088 — ch78-91: surface-specific footstep/landing variants (7 terrain pairs, mapped by SoundEnviron) — see SOUND-006
- [x] SOUND-089 — ch92: confirmed correct — follower (ObjectType96→97) wake sound.
- [x] SOUND-090 — Sound enable/disable respects `enabled_` flag — **done** (**corrected 2026-07-14**): `GESound::Play()` gates on `enabled_` and `SetEnabled(false)` calls `StopAll()`, wired to the real Setup sound toggle + persisted via `GESaveData`.

---

### 2.10 Camera *(3D-specific)*

Camera is relatively more advanced than other CNA systems: first-person default plus third-person
toggle (real GPU-skinned placeholder model, "C" key) with exponential framerate-independent
damping already work.

- [x] CAM-001 — First-person default view with third-person toggle ("C" key, GPU-skinned placeholder model) (CNA, 2026-07-10) — note: replaces the old Simple3D-era "3rd-person orbit is the only mode" framing; CNA defaults first-person
- [ ] CAM-002 — RMB pitch control
- [ ] CAM-003 — Scroll-wheel zoom (smooth lerp)
- [ ] CAM-004 — Pitch auto-reset to a default angle when RMB released
- [x] CAM-005 — Wall collision (DDA ray march from Blupi to desired camera
      position) — **done 2026-07-17**, user-requested after noticing the
      real risk: a third-person chase camera with no wall check can clip
      through geometry to end up outside a tunnel/room, looking back in
      through the wall from an impossible vantage point. New
      `RaymarchWallDistance()` (`GalaxyEggbertCnaGame.cpp`, anonymous
      namespace) walks from Blupi's own look-at point toward the desired
      chase-camera position in small fixed steps (`0.1` world units),
      stopping at the first solid (`!isAir()`) block — a simplified fixed-
      step raymarch rather than a literal cell-boundary DDA voxel traversal
      (this task's own original wording), since the real max distance
      involved is tiny (a few world units, the chase distance) and a small
      step is both simpler and plenty precise at that scale; same
      "documented simplification, not the textbook algorithm" category as
      several other collision adaptations this session. Solidity uses a
      plain non-air check (a visual-occlusion query, deliberately NOT
      reusing `GEBlupiController`'s own movement-collision rules — e.g. a
      real non-solid-for-Blupi Teleporter pillar still correctly blocks the
      camera's view through it). First-person is unaffected (the eye IS
      Blupi's own position, nothing to collide with). Live headless
      verification: Blupi placed mid-tunnel in `world001.vwr`'s own south
      tunnel facing a nearby wall — confirmed the naive desired chase
      distance (4 units) gets clamped to the real available distance
      (1.5 units) by the wall, keeping the camera inside the tunnel instead
      of clipping through it (temporary debug instrumentation, reverted
      before commit). Full regression clean (only the pre-existing
      unrelated `easy-gl-resource-smoke-tests` failure).
- [x] CAM-006 — Exponential, framerate-independent damping on camera follow (CNA, 2026-07-10)
- [ ] CAM-007 — FOV tuned/finalized (verify current FOV value against original 65° reference)
- [x] CAM-008 — Camera shake: SmallShake — **fully wired 2026-07-14**. The "7 near-identical
      dynamite-blast sites" a first research pass reported turned out, on direct re-verification,
      to be a MIX of different real mechanics, not one uniform trigger — corrected via a direct
      `Decor.cpp` read of every cited site: (1) the generic-hazard contact-kill branch
      (`Decor.cpp:5782-5814`) — the SAME site already implemented as galaxy-eggbert's own
      `IsGenericHazard()` contact-kill logic (`GEInteractionSystem.cpp`) — plays SmallShake for
      every one of its 8 types except fish/bird (see CAM-009); (2) the REAL dynamite blast
      (`Decor::DynamiteStart()`, `Decor.cpp:9068-9070`) plays SmallShake only ONCE, at the blast's
      own center tile (`dx==0 && dy==0`), not per-destroyed-object — matches galaxy-eggbert's
      existing `if (blast.dx==0.0f && blast.dy==0.0f)` ch10-sound branch exactly, just needed the
      shake signal added alongside it; (3) the CleanAll cheat's enemy-destruction loop
      (`Decor.cpp:1811`, already `CheatCleanAll()` in this engine) plays SmallShake once per
      destroyed enemy in the real source, modeled here as once-per-cheat-invocation (via a new
      `bool` return value signaling "anything destroyed"), a reasonable simplification since the
      camera can't display more than one shake at a time anyway. Three OTHER cited sites turned
      out to be different, NOT-YET-implemented mechanics entirely (Perso-decoy-destroys-enemy via
      `MovePersoDetect()`, `ObjectType201-203` contact damage — plan.md's own already-flagged
      `PICKUP-069` gap — and forced-vehicle-dismount-on-large-creature-contact) — correctly left
      unwired, since each would need its own new gameplay logic built first, not just a shake
      signal. `GEInteractionSystem::SmallShakeTriggeredThisFrame()`/`BigShakeTriggeredThisFrame()`
      (new, reset every `Update()` call, same idiom as `DiedThisFrame()`) signal both (1) and (2)
      back to `GalaxyEggbertCnaGame.cpp`; `CheatCleanAll()` signals (3) via its own return value.
      6 new `VerifyInteractionSystem` checks + full regression on both backends, all pass.
- [x] CAM-009 — Camera shake: BigShake — **done 2026-07-14**. Description corrected: the real
      trigger (`Decor.cpp:5468`, direct source read) is Fan/Ventillo contact specifically killing
      Blupi (`IsVentillo(m_blupiPos)` inside the same real gate as `BlupiDead()`, matching this
      engine's own `!IsInvincible()` gate on `triggerDeath()` exactly) — NOT "large explosion"
      (unconfirmed) nor "triggered by ObjectType11" (`ObjectType11` is a cosmetic particle spawned
      *alongside* the real shake, not a separate trigger condition; this particle IS now also
      modeled, see VISUAL-008's writeup — `GEInteractionSystem::SpawnFanHitFlash()`, wired into the
      same Fan-hazard block right next to the shake trigger). Wired into
      `GalaxyEggbertCnaGame.cpp`'s existing Fan hazard block. A SECOND real BigShake trigger was
      also found and wired the same day (see
      CAM-008's writeup): the generic-hazard contact-kill site plays BigShake instead of SmallShake
      specifically for fish (`ObjectType17`) and bird (`ObjectType20`) — confirmed via
      `Decor.cpp:5820-5823` and a dedicated `VerifyInteractionSystem` check.
- [x] CAM-010 — Camera shake: ElectricShake — **done 2026-07-14**. Description corrected: the real
      trigger (`Decor.cpp:5864`, direct source read) is specifically the WASP-STING/balloon-entry
      event (the same real site that sets `m_blupiBalloon=true` and plays channel 40, both already
      implemented in this engine) — despite the `DecorAction::ElectricShake` enum's own generic
      "contacts an electric field (ObjectType90 spark object)" doc comment, there is no separate
      electric-field-tile hazard anywhere in source; `ObjectType90` is a cosmetic spark spawned
      alongside this same wasp event, not modeled (no particle system). Wired into
      `GalaxyEggbertCnaGame.cpp`'s existing wasp-balloon block.
- [x] CAM-011 — Camera shake: table_decor_action per-frame (dx, dy) offsets × 3 multiplier —
      **done 2026-07-14**. `Tables::table_decor_action[519]` verbatim-transcribed into
      `GECameraShake.cpp` (data-table transcription approved by the user 2026-07-14) and
      INDEPENDENTLY byte-verified via a script comparing every one of the 519 values against the
      real source directly (exact match, not just visually proofread). The real ×3 multiplier
      (`Decor.cpp:1367-1368`) is applied exactly as found, not approximated.
- [x] CAM-012 — Camera shake: fixed N-frame duration, self-clears to None after last frame —
      **done 2026-07-14**, ported 1:1 from `DecorNextAction()`'s own record-walk/self-clear logic
      (`Decor.cpp:1353-1397`) — confirmed the real self-clear only becomes observable on the
      Update() call *after* the last real frame is consumed (an off-by-one that looked like a bug
      in an early test draft but is exactly how the real source behaves too, verified by matching
      the real `if (m_decorPhase < frameCount) {...} else { None }` structure line-for-line).
- [x] CAM-013 — Camera shake implementation ported into CNA/Easy3D (StartShake equivalent) —
      **done 2026-07-14**. New `GECameraShake.hpp`/`.cpp` (engine-agnostic, no CNA/graphics
      dependency, same precedent as `GEBlupiController`/`GETerrainAnimDivisor`): `Trigger(type)`
      (unconditional restart from frame 0, matching the real trigger sites' own plain
      `m_decorAction = X; m_decorPhase = 0;` — no priority gating found anywhere) + `Update(dt)`
      (real 20Hz tick, `Config::CURRENT_FPS`, confirmed) + `GetOffsetX()/GetOffsetY()` (the real
      per-frame offset in pixel units, already ×3'd). `GalaxyEggbertCnaGame.cpp` ticks it once per
      Play-phase frame (matching the real `m_bPause` freeze gate) and applies the real 2D
      screen-space `(dx,dy)` as a small 3D camera-space TRANSLATION (added equally to eye and
      target, preserving look direction — the closest natural adaptation of a scroll-offset shake
      to a 3D perspective camera, per CLAUDE.md's allowed "perspective camera" adaptations),
      converted from real pixels to this engine's world units via the same 64px-per-tile
      conversion already used throughout its own atlas math. 18 new `VerifyCameraShake` checks
      (per-type first-frame offset values, self-clear timing for all 3 types, unconditional
      re-trigger override, sub-tick dt accumulation, idle no-op) + full regression on both
      backends, all pass.
- [ ] CAM-014 — HotSpot zoom: MoveHotSpot() eases camera zoom toward target
- [ ] CAM-015 — HotSpot target: m_hotSpotFinalZoom/X/Y interpolated over N frames
- [ ] CAM-016 — HotSpot: triggered on special events (secret exit found, level end zoom)
- [ ] CAM-017 — SCROLL_MARGX = 80 px / SCROLL_MARGY = 40 px viewport scroll margins (2D-era concept — evaluate whether a 3D-equivalent framing margin applies at all before implementing)
- [x] CAM-018 — Smooth scroll: camera eases toward Blupi at SCROLL_SPEED = 8 px/tick (2D-era
      concept) — **evaluated and closed 2026-07-17**: this is the same real "camera eases toward
      Blupi" behavior as `CAM-006` (done, exponential framerate-independent damping) — a 3D
      perspective camera has no direct equivalent to a flat 2D px/tick scroll rate, so the real
      8px/tick figure isn't a portable magnitude (same category as every other "no established
      real-to-3D unit conversion" approximation this session, e.g. `kConveyorNudgeSpeed`).
      `CAM-006`'s damping already delivers the same functional purpose (smooth follow, not an
      instant snap) — not a separate gap to close.

---

### 2.11 Save Data

**Badly stale, corrected 2026-07-14** — this section's own header claimed "not started at all",
but `GESaveData` (`src/GalaxyEggbertCNA/Game/GESaveData.hpp`/`.cpp`) is a real, working, tested
(`tools/VerifyGESaveData`) save system: 3 independent gamer slots (lives/missionNumber/
hasProgress), a global `soundEnabled` flag, a persisted `selectedGamer` index, auto-save wired to
the real Win/Lost/reset/gamer-select trigger points. It is a plain `key=value` text file, NOT
byte-compatible with real `GameData`'s 640-byte binary layout — a deliberate, confirmed decision
(`GESaveData.hpp`'s own header comment): the real format is shaped around a 100+-level/3-gamer-
slot/`IsolatedStorageFile` structure this engine's single hand-authored `.vwr` world doesn't have,
so byte compatibility would buy nothing. Do NOT flip SAVE-001/003/006/007 to `[x]` on that basis —
those are specifically about the real byte layout, which stays undone by design.

- [ ] SAVE-001 — GameData: 640-byte flat binary format, binary-compatible with mobile-eggbert — still correctly undone (deliberate, see this section's own intro).
- [x] SAVE-002 — Global header — **partially real, done differently**: `selectedGamer` and `soundEnabled` ARE persisted (the two settings with real desktop behavior behind them); `jumpRight`/`autoZoom`/`accelActive` are NOT (their own UI toggles — `SetupJump`/`SetupZoom`/`SetupAccel` — are themselves intentionally inert in this engine, no touch/accelerometer hardware to back them, see §2.2's own notes).
- [x] SAVE-003 — 3 gamer slots — **done differently**: `GamerSlot{lives, missionNumber, hasProgress}` × 3 (`kGamerCount`), matching the real slot COUNT and the lives/lastWorld-equivalent fields; the `doors[200]` byte range is NOT ported (see SAVE-006/007).
- [x] SAVE-004 — Auto-save — **done for win/lost/reset/gamer-select** (confirmed via 5 real `saveData_.Save()` call sites in `GalaxyEggbertCnaGame.cpp`: Win transition, Lost transition, Cheat5/SetupReset full reset, sound toggle, Init gamer-slot tap). **Correction 2026-07-14**: this entry previously called the missing quit/window-close save "a real, if minor, gap" — checked directly against real `Game1::OnExiting()`/`OnDeactivated()` (`Game1.cpp:186-208`) and that premise is wrong: real `OnExiting()` calls `decor.CurrentDelete()` (removes the mid-game snapshot), and `OnDeactivated()` calls `CurrentWrite()`/`CurrentDelete()` — neither ever calls `GameData::Save()` (the profile `GESaveData` actually mirrors). Both belong to the separate `CurrentWrite`/`CurrentRead`/`CurrentDelete` mid-game-save mechanism, already correctly noted as out of scope below (SAVE-009/010). Real mobile-eggbert does NOT save the gamer profile on quit either — galaxy-eggbert's current behavior already matches. No gap here.
- [x] SAVE-005 — Persistence mechanism chosen and wired for CNA — **done**: plain `key=value` text file (no JSON library is linked in this project; a hand-rolled parser was simpler than adding one for a handful of scalars), a real, working, CNA-appropriate answer to this question.
- [x] SAVE-006 — doors[0..179]: secondary door states (180 secondary doors)
      — **the functional per-sublevel unlock half is now done 2026-07-17**,
      see `SCORE-013`'s hub/mission-progression writeup: `GESaveData::
      IsMissionDoorUnlocked()`/`UnlockMissionDoor()` persist exactly this
      real semantic (does sublevel N's door open), just keyed directly by
      mission number rather than porting the real `m_doors[0..179]` index
      scheme literally (same net effect). Init's "Secondary gates" HUD text
      line itself is still a static "0/52" (unrelated display code, not
      touched by this change) — only the underlying gameplay-functional
      door-gating is what's now real.
- [ ] SAVE-007 — doors[180..199]: main door states (20 main doors / hub
      worlds) — still correctly NOT ported, but **the "purely cosmetic"
      characterization above is wrong/incomplete (found 2026-07-20, direct
      re-read of `Decor::AdaptDoors()`, `Decor.cpp:11533-11621`)**. On the
      global hub (`m_mission==1`) it's actually TWO distinct sub-effects:
      (1) a real FUNCTIONAL removal of a gold-covered obstacle tile (icon
      183) per unlocked main door, via the SAME animated door-slide-open
      object (`ObjectType22`) the regular door system (`160`) already
      uses -- gated on `m_doors[180+i]==1`, i.e. `IsMissionDoorUnlocked()`
      already has the real backing data for this, it's the reveal LOGIC
      that's missing, not the persistence; (2) a genuinely cosmetic icon
      swap for world-select markers (icons 158-165/309/410-415, +8/+5) --
      this half really is purely visual and matches the earlier summary.
      **Subtlety worth flagging for whoever implements this**:
      `Decor::SearchGold(n, cel)` (`Decor.cpp:11486-11501`) ignores its own
      `n` parameter entirely -- it's a linear scan for the first tile with
      icon==183 in the whole 100x100 grid, self-consuming (a match only
      gets cleared to icon=-1 if that specific door index is ALSO
      unlocked; otherwise the same tile is found again on the next `i`).
      So the mapping from "gold tile in scan order" to "which of the 20
      main-door indices reveals it" is first-come-first-served, not a
      fixed per-world assignment -- easy to mis-port without noticing.
      **Root blocker found 2026-07-20 (the actual reason this needs its
      own pass, not just careful porting)**: `m_doors[180+i]` is ONLY ever
      set by `Decor::OpenGoldsWin()` (`Decor.cpp:11707-11710`,
      `m_doors[180 + m_mission/10] = 1`), which is ONLY ever called from
      the real win-trigger (`Decor.cpp:6425-6429`) gated on `m_bFoundCle`
      being true. `m_bFoundCle` is the real per-level hidden-secret flag
      ("found the hidden gold -> straight back to the global hub" instead
      of the normal "back to this world's own hub" exit) -- and this is
      **already documented elsewhere in this file (§2.8/`SCORE-013`'s own
      writeup, `PICKUP-009`'s note) as NOT modeled, deliberately out of
      scope** (a per-level hidden-pickup mechanic with no defined content
      in this project's single hand-authored world yet). So `SAVE-007`
      has no real path to ever fire in this engine as it stands --
      implementing just the hub-side reveal logic without also
      implementing `m_bFoundCle` detection would be dead code with
      nothing to trigger or test it against. Genuinely blocked on that
      separate, already-deferred prerequisite, not just "needs careful
      porting" -- don't attempt this in isolation.
- [ ] SAVE-008 — GetGamerInfo: return lives, mainDoors, secondaryDoors per
      gamer slot — **secondaryDoors (the functional half) now has a real
      backing store** (`IsMissionDoorUnlocked()`, see `SAVE-006`), but no
      single `GetGamerInfo()`-shaped accessor bundles lives+doors together
      the way this item literally asks — stays `[ ]` for the full ask;
      `mainDoors` (the cosmetic gold half) is still fake/static per
      `SAVE-007`.
- [ ] SAVE-009 — CurrentWrite / CurrentRead: mid-game save/load (on app deactivate/activate) — confirmed NOT modeled; real trigger is a WP7 OS lifecycle event (`Game1::OnActivated()`) with no desktop equivalent, and the real mechanism itself is a separate, heavier serialized-`Decor`-state snapshot than `GameData`, explicitly out of this engine's single-world scope (`GESaveData.hpp`'s own comment). Resume is offered instead whenever `hasProgress==true` from a prior Win/Lost — a documented simplification of the trigger, not a port of this item.
- [ ] SAVE-010 — CurrentDelete: remove mid-game save (on OnExiting or normal level exit) — same reasoning as SAVE-009, not modeled.
- [ ] SAVE-011 — Accelerometer sensitivity setting — NOT modeled, no accelerometer hardware exists on desktop.
- [ ] SAVE-012 — JumpRight setting — NOT modeled; `SetupJump` toggle is real UI (renders/responds) but intentionally has no behavioral effect (documented gap, §2.2).
- [ ] SAVE-013 — AutoZoom setting — same as SAVE-012 (`SetupZoom`).
- [ ] SAVE-014 — Ranking mode persisted when isRankingMode is active — NOT modeled; Ranking mode itself is confirmed unreachable in this port (hardcoded-false gate, same "never shown by default" precedent as the Trial phase and `InitRanking` button).

---

### 2.12 Visual Polish *(3D-specific and faithful to mobile-eggbert)*

Everything here depends on systems (Blupi visibility, enemies, pickups, camera shake) that are
themselves mostly not started in CNA yet. All items reset to `[ ]`.

- [ ] VISUAL-001 — ~~Blob shadow under Blupi (scales with height, disabled in helicopter/
      balloon)~~ **HALLUCINATED — CANCELLED (confirmed by user 2026-07-14)**: grepped all of
      `mobile-eggbert/src/WindowsPhoneSpeedyBlupi/` and headers (including French terms —
      "ombre"/"shadow") — zero hits anywhere. No blob-shadow concept exists in real source.
- [ ] VISUAL-002 — ~~Blob shadow under enemies (disabled for birds)~~ **HALLUCINATED — CANCELLED,
      same finding as VISUAL-001 — no real source.**
- [ ] VISUAL-003 — ~~Pickup bobbing: sine-wave Y offset on collectibles~~ **HALLUCINATED —
      CANCELLED (confirmed by user 2026-07-14)**: no `sin`/bobbing animation tied to any pickup
      type found anywhere in `Decor.cpp`.
- [ ] VISUAL-004 — ~~Score popups: rising "+N" text, 1 s fade at collection position~~
      **HALLUCINATED — CANCELLED, same invented premise as the §2.8 SCORE section** (see that
      section's own correction above) — no numeric score concept exists in mobile-eggbert at all,
      so a "+N" popup tied to it can't be real either.
- [ ] VISUAL-005 — ~~Respawn flash: Blupi billboard blinks at 10 Hz for 2 s after respawn~~
      **HALLUCINATED — CANCELLED (confirmed by user 2026-07-14)**: no respawn-flash/blink effect
      found in `Decor.cpp` — the only "blink" hit anywhere is an unrelated comment about a timed
      platform tile's solidity cycle (icon 324, line ~6693).
- [ ] VISUAL-006 — ~~Shield tint: cyan sprite when m_blupiShield active~~ **HALLUCINATED —
      CANCELLED (confirmed by user 2026-07-14)**: no tint/color-modulation effect for Shield found
      in `Decor.cpp` — real Shield rendering (`Decor.cpp` ~line 780 area, already cited elsewhere
      this session) draws a `table_shieldloop` sparkle icon overlay, not a tint.
- [ ] VISUAL-007 — ~~Shield blink at < 1.5 s remaining~~ **HALLUCINATED — CANCELLED, same finding
      as VISUAL-006 — no real blink-at-low-time effect found, only the sparkle-overlay behavior
      already noted.**
- [x] VISUAL-008 — Explosion billboard effects: ObjectType8-11 from `explo.png` (128×128 px,
      Explosion channel) — **all 4 types done as of 2026-07-14**. ObjectType11 (Fan-hit
      shockwave), ObjectType8 (dynamite-blast flash / generic-hazard explosion flash), ObjectType10
      (fish/bird explosion flash), and ObjectType9 (follower-blocked-path debris flash) are all
      implemented.
      **ObjectType11** — the cosmetic particle spawned alongside CAM-009's BigShake (see CAM-009's
      writeup, now updated). Real spawn site confirmed via direct source read (`Decor.cpp:5467`,
      the same real Fan/Ventillo-kill block CAM-009 already cites): `ObjectStart(celSwitch,
      ObjectType11, 0)` — `speed=0` means no direction/offset encoding at all (unlike every other
      particle effect shipped so far), so `GEInteractionSystem::SpawnFanHitFlash()` is a
      single-instance spawn exactly at the given position, no burst. Real self-delete at
      `phase>=9` (`Decor.cpp:8431-8440`, a 9-frame lifetime). Found and fixed a FOURTH real bug in
      `GEObjectIcons.cpp`'s icon formula for this type: wrong divisor (6 instead of the real
      `Config::ScaleDiv(1)==1`) AND wrong ascending-arithmetic assumption — real `table_explo4`
      (`Tables.cpp:1391`) is non-monotonic (`12,13,14,15,7,8,9,10,11` — jumps back from 15 to 7
      partway through), transcribed verbatim as a lookup table, same category of bug as
      VISUAL-012's `table_tresortrack`. 7 new `VerifyInteractionSystem` checks (spawn
      position/no-offset, self-delete timing, corrected icon values including the phase=4
      non-monotonic jump point) + full regression on both backends, all pass.
      **ObjectType8** — the cosmetic flash spawned once per blast in the real 9-blast dynamite-chain
      sequence (`Decor::DynamiteStart()`, `Decor.cpp:9058-9065`: `ObjectStart(posStart,
      ObjectType8, 0)`, again `speed=0`/no offset), at each blast's own already-computed
      `(centerX,centerY,centerZ)` — this engine's dynamite-blast logic (`GEInteractionSystem.cpp`'s
      `ObjectType56` fuse block, plan.md E3D-MIG-155/CAM-008) already computed these 9 real
      per-blast centers for its destruction/SmallShake logic, so the flash only needed a new
      `AppendDynamiteBlastFlash()` free function (pendingSpawns idiom, same reasoning as
      VISUAL-012's `AppendSparkleBurst()` — the real spawn site is inside this class's own
      per-object loop) called once per blast, not just the center one. Real self-delete at
      `phase>=39` (`Decor.cpp:8397-8399`, `Tables::table_explo1Length==39`, the longest of the 4
      particle lifetimes modeled so far). Found and fixed a FIFTH real bug in `GEObjectIcons.cpp`:
      same wrong-divisor bug, plus real `table_explo1` (`Tables.cpp:1368-1374`, 39 frames)
      repeatedly bounces back and forth between adjacent values rather than advancing
      monotonically (e.g. `...,4,3,4,4,3,4,3,3,4,4,...`), transcribed verbatim as `kExplo1[39]`.
      7 new `VerifyInteractionSystem` checks (real spawn exercised via the existing dynamite-blast
      test, isolated self-delete timing, corrected icon values including a bounce-back point) +
      full regression on both backends, all pass.
      **ObjectType8, second real trigger site, found 2026-07-14 (later the same day):** the SAME
      flash is ALSO spawned at the real generic-hazard contact-kill site (`Decor.cpp:5807-5816`,
      already this engine's own existing `IsGenericHazard()`/`CAM-008` SmallShake block) for every
      hazard type EXCEPT fish/bird — reusing the exact same spawn (renamed the free function from
      `AppendDynamiteBlastFlash()` to the more general `AppendExplosionFlash(type, x, y, z, ...)`,
      now parameterized by `ObjectType` since both real sites need the identical static/no-offset
      spawn shape, just at different positions and for different types).
      **ObjectType10 — done 2026-07-14 (found alongside the ObjectType8 second-site discovery
      above):** the SAME generic-hazard contact-kill site spawns ObjectType10 instead of
      ObjectType8 specifically for fish (`ObjectType17`) and bird (`ObjectType20`) — the exact
      same real fish/bird split already modeled for BigShake vs SmallShake (`CAM-008/009`), now
      also driving which explosion-flash type spawns. Real self-delete at `phase>=20`
      (`Decor.cpp:8419-8430`). Found and fixed a SIXTH `GEObjectIcons.cpp` bug: wrong divisor (6
      instead of 1) and wrong ascending-arithmetic assumption — real `table_explo3`
      (`Tables.cpp:1384-1388`) is a repeating oscillation (`32,32,34,34` ×3, then `32,32,35,35`
      ×2), transcribed verbatim as `kExplo3[20]`. **Found and fixed a test false-positive along the
      way:** adding this spawn exposed a latent bug in an EXISTING test (`2.5. Generic hazard
      contact`) — its `findFirst()` helper doesn't filter by active state, and the new explosion
      flash can legitimately reuse the just-killed hazard's own now-inactive pool slot (matching
      real `MoveObjectFree()`'s own slot-reuse semantics), which made a blind type-based re-lookup
      find the sample world's OTHER real `ObjectType2` placement instead (still active) — a false
      failure, not a real regression. Fixed by matching on position, same established pattern used
      for several other tests this session. 5 new `VerifyInteractionSystem` checks (both real
      spawn sites, isolated self-delete timing, corrected icon values) + the 1 existing test fixed
      + full regression on both backends, all pass.
      **ObjectType9 — done 2026-07-14 (found while re-reading the follower-blocked-path
      self-destruct block, already this engine's own existing `ObjectType97` homing-follower
      logic):** the real spawn site (`Decor.cpp:8057-8064`, `TestPath` fails ->
      `ObjectDelete` + `ObjectStart(end, ObjectType9, 0)`) was already correctly identified in an
      earlier session comment, which claimed it was blocked on the renderer's missing `-1`
      blank-frame support — that blocker was quietly resolved by the bullet-splat effect
      (VISUAL-009) earlier the same day but the follower comment was never revisited until now.
      **Also found at this exact site while fixing it: the real self-destruct ALSO triggers
      `SmallShake`** (`m_decorAction = DecorAction::SmallShake`, `Decor.cpp:8062-8063`) — a site
      the earlier CAM-008 SmallShake-trigger-site audit missed entirely (it was scoped around the
      "7 near-identical dynamite-blast sites" description and didn't crawl every
      `DecorAction::SmallShake` assignment in the file); now wired too. Real self-delete at
      `phase>=20` (`Decor.cpp:8407-8417`). Found and fixed a SEVENTH `GEObjectIcons.cpp` bug: wrong
      divisor and wrong ascending-arithmetic assumption — real `table_explo2` (`Tables.cpp:
      1377-1381`) has real `-1` blank-frame sentinels interspersed throughout, transcribed verbatim
      as `kExplo2[20]`, reusing the already-existing renderer `-1`-skip support with no further
      changes needed. 7 new `VerifyInteractionSystem` checks (real spawn site now also verifies
      the newly-wired SmallShake, isolated self-delete timing, corrected icon values including a
      blank frame) + full regression on both backends, all pass.

      VISUAL-008 is now the first particle-effect checklist item completed at 4-of-4 real types.
      Not live-visually re-verified for any of the 4 (same already-proven element.png/explo.png
      billboard rendering path as every other particle effect this session).
- [x] VISUAL-009 — Water splash billboard effects: ObjectType98-100 from `explo.png` —
      **done 2026-07-14, description corrected**: despite the `ObjectType.hpp` enum's own "water
      splash"/"spawned when entering water" doc comments, direct source read found the ONLY real
      spawn site is `Decor::StartSploutchGlu()` (`Decor.cpp:7763-7789`), called exactly once, from
      the real ObjectType23-bullet-kills-Blupi block (`Decor.cpp:5914-5947` — already this
      engine's own existing `ObjectType23` contact-death branch, plan.md E3D-MIG-134) — a "Blupi
      hit by a bullet, gets glued (`BlupiAction::Glu`)" splat reaction, not water entry at all; no
      other real trigger for these 3 types exists anywhere in source. Spawns a scattered
      7-instance splat (1x ObjectType98, 4x99, 2x100) at small fixed real-pixel offsets from the
      bullet's own position, all `speed=0` (static, no offset/interpolation, same shape as
      `SpawnFanHitFlash()`). New `AppendSplatEffect()` free function, wired directly into the
      already-existing `ObjectType23` contact block. Real self-delete: `phase>=10` (98), `>=13`
      (99), `>=18` (100).

      Found and fixed 3 more `GEObjectIcons.cpp` bugs: `ObjectType98`'s wrong divisor (6 instead of
      1, `table_sploutch1` is a plain ascending range so only the divisor needed fixing);
      `ObjectType99`/`100` were previously static "first real frame" stubs — their real tables
      (`table_sploutch2/3`, `Tables.cpp:1436-1448`) are now transcribed verbatim and fully
      animated, including their real leading `-1` "invisible frame" delay (3 ticks for 99, 8 for
      100, modeling debris fallen from progressively greater heights). This REQUIRED adding real
      `-1`-sentinel support to the explo.png billboard render pass itself
      (`GalaxyEggbertCnaGame.cpp`: skip drawing when the icon is negative) — the first render-side
      change any particle effect this session has needed, and the same fix already flagged as a
      prerequisite for `ObjectType9` (`table_explo2`) if that type is ever wired.

      17 new `VerifyInteractionSystem` checks (spawn count/type-split at the real bullet-contact
      site, self-delete timing for all 3 types, corrected icon values including the invisible-delay
      frames) + full regression on both backends, all pass. Not live-visually verified (same
      already-proven explo.png billboard rendering path as every other Explosion-channel particle
      effect this session).
- [x] VISUAL-010 — Electric arc: ObjectType92 long arc from `explo.png` (128 frames) —
      **done 2026-07-14, description corrected**: despite `ObjectType.hpp`'s own "spawned when
      Blupi uses a charged attack" doc comment, direct source read found the ONLY real spawn site
      (`Decor.cpp:5593-5606`) is the TELEPORTER trigger — already this engine's own existing
      `GEBlupiController::TriggerTeleport()` call site (`GalaxyEggbertCnaGame.cpp`,
      plan.md E3D-MIG-147) — spawned once, static (real `speed=0`), at
      `(blupiX, blupiY+5/64, blupiZ)` (real `celSwitch = (blupiPos.X, blupiPos.Y-5)`, the usual
      screen-Y-to-world-Y sign flip). New `GEInteractionSystem::SpawnTeleportArc()`, called
      directly at the existing teleporter-trigger site (same shape as `SpawnFanHitFlash()`). Real
      self-delete at `phase>=128` — a long 6.4s lifetime that exactly matches this engine's own
      `GEBlupiController::kTeleportDuration` (the arc plays for the whole real teleport transit,
      a nice confirmation the two independently-ported real constants agree).

      Fixed the existing `GEObjectIcons.cpp` static "first-frame only" stub: real `table_explo7`
      (`Tables.cpp:1407-1422`) is a 128-frame "large multi-particle scatter" with `-1` blanks
      interspersed THROUGHOUT (not just a leading/trailing delay like `table_sploutch2/3`) — only
      6 distinct icons (60-65) ever appear, comfortably within the sheet, so the earlier "would
      exceed the sheet" assumption was wrong the same way `ObjectType57`'s was. Transcribed
      verbatim and INDEPENDENTLY byte-verified via a script comparing all 128 values against the
      real source directly (exact match). Reuses the `-1`-skip render support added for the
      bullet-splat effect (VISUAL-009) with no further renderer changes needed. 7 new
      `VerifyInteractionSystem` checks (spawn position/offset, self-delete timing, corrected icon
      values including a mid-sequence blank) + full regression on both backends, all pass. Not
      live-visually verified (same already-proven explo.png billboard rendering path as every
      other Explosion-channel particle effect this session).
- [x] VISUAL-011 — Shield sparkle loop: ObjectType57 trail behind Blupi while shielded —
      **done 2026-07-14**, the SIXTH real particle effect, built alongside its sibling Power/Magic
      trail (ObjectType27, same mechanic, see VISUAL-017). Description corrected: not a continuous
      "loop" — real `Decor.cpp:5204-5237` drops a
      single STATIC breadcrumb marker exactly at Blupi's current position every time he has moved
      a real Manhattan (X+Y, screen-space, Z ignored) distance of >=40px since the last drop
      (`m_blupiPosMagic`, a SINGLE tracker shared with the Power trail below, safe since Shield/
      Power/Cloud/Hide can never be simultaneously active), confirmed via direct source read. Real
      self-delete at `phase>=20` (Shield, `table_shieldtrack`, `Decor.cpp:8373-8380`) /
      `phase>=24` (Power, `table_magictrack`, `Decor.cpp:8365-8372`). No offset/interpolation
      needed (`speed=0`-equivalent, same static shape as `SpawnFanHitFlash()`). `ResetMagicTrail()`
      mirrors every real grant site's own `m_blupiPosMagic = m_blupiPos` reset (confirmed at
      multiple sites, e.g. `Decor.cpp:6022`) — wired at this engine's own existing Shield/Power
      grant sites (`GalaxyEggbertCnaGame.cpp`, right next to their existing grant-sound calls).
      Found and fixed TWO more `GEObjectIcons.cpp` bugs: `ObjectType27`'s existing formula had the
      usual wrong-divisor-and-ascending-arithmetic bug (real `table_magictrack` repeats icons
      152-156 TWICE before continuing, `Tables.cpp:1754-1759`); `ObjectType57` was previously a
      static "first-frame only" return under the WRONG assumption that a naive ascending 20-frame
      range would overflow this engine's element.png sheet (274+19=293 > 289) — the REAL table
      (`table_shieldtrack`, `Tables.cpp:1769-1773`) only reaches 288, comfortably within bounds, so
      the full animation is now modeled instead. Real Hide's own afterimage trail (`ObjectType58`,
      a snapshot of Blupi's OWN current sprite, not a fixed icon) is NOT modeled — blocked on the
      same "no visible Blupi model/animation" gap as Pollution puff's Jeep/Tank simplification, not
      a quick add. Real level-load/respawn resets of `m_blupiPosMagic` (several additional real
      call sites beyond the 4 power-grant ones) are also NOT modeled — a minor, accepted
      simplification (cosmetic only: a stale tracker just shifts the first post-respawn marker's
      exact trigger point). 15 new `VerifyInteractionSystem` checks (no-op without Shield/Power,
      threshold-crossing spawn/no-spawn, static position, tracker reset on spawn, Z-axis ignored,
      self-delete timing for both types, corrected icon values including the repeat-then-continue
      point) + full regression on both backends, all pass. Not live-visually verified (same
      already-proven element.png billboard rendering path as every other particle effect this
      session).
- [x] VISUAL-012 — Treasure sparkle: ObjectType39 on each treasure pickup — **done 2026-07-14**,
      the second real particle effect built (see VISUAL-014/015's writeup for the shared
      `SearchDistRight()`-short-circuit/64px-conversion technique this reuses). Real spawn site
      confirmed via direct source read (`Decor.cpp:5948-5960`, the real ObjectType5/treasure-
      collect site): 4 `ObjectStart(pos, ObjectType39, speed)` calls, same `{-60,60,10,-10}`
      direction encoding as Invert, no pre-offset (matches Invert's GRANT shape, 500 real-px).
      Real self-delete at `phase>=11` (`Decor.cpp:8382-8389`, an 11-frame lifetime, shorter than
      Invert's 16). Found and fixed a THIRD real bug in `GEObjectIcons.cpp`'s existing icon
      formula for this type — unlike the Invert pair's simple arithmetic-range bug, real
      `table_tresortrack` is a genuinely oscillating table (`166,165,164,163,162,161,162,163,
      164,165,166` — shimmers down to 161 and back, not a plain ascending range), transcribed
      verbatim as a lookup table; divisor was also wrong (6 instead of the real
      `Config::ScaleDiv(1)==1`). New free function `AppendSparkleBurst()` (not a public method
      like Invert's `SpawnInvertBurst()` — the real treasure-collect site is INSIDE
      `GEInteractionSystem::Update()`'s own per-object loop, so it can use the existing
      `pendingSpawns` deferred-spawn pattern directly). Real source also fires this same burst for
      `ObjectType49/50/51` (key-gated doors, not key pickups) — NOT wired, since this engine's own
      door model uses static terrain tiles, not `MobileObjSpec` instances, for those; a separate
      follow-up if picked up. 7 new `VerifyInteractionSystem` checks (spawn count/distance
      filtered by proximity to the chest, since the sample world's own object-type exhibition
      already places one static specimen of every type including this one, self-delete timing,
      corrected icon values at 3 points including the shimmer's low point) + full regression on
      both backends, all pass. Same live-visual-verification caveat as VISUAL-014/015 — not
      independently re-attempted (same rendering path, same conclusion would apply).
      **Correction, 2026-07-14 (later the same day):** same animation bug as VISUAL-014's own
      correction (see its writeup for the full explanation) — this effect also spawns AT the
      chest's position and slides toward the real 500px-out `posEnd` over 78 ticks, self-deleting
      at phase>=11 (~14% of the way) rather than appearing instantly at the full distance. Fixed
      identically (posStart=origin, posEnd=target, linear interpolation via `phase/78`). Existing
      tests updated to check `posEnd` (the fixed real target) and the real starting-at-origin
      position. Full regression re-run on both backends after the fix, all pass.
      **Correction, 2026-07-14 (later the same day):** the "ObjectType49/50/51 (key-gated doors,
      not key pickups)" note above had it BACKWARDS — direct re-read of `ObjectType.hpp`'s own doc
      comments confirms these ARE the 3 key pickups themselves ("Key 1/2/3 collectible"), not door
      tiles, and this engine already collects them as such (`GEInteractionSystem.cpp`'s existing
      `ObjectType49/50/51` cases, `keys1_/keys2_/keys3_`). No door-model blocker ever applied —
      wired the same `AppendSparkleBurst()` call into all 3 existing key-pickup cases. 1 new
      `VerifyInteractionSystem` check (key pickup spawns the same 4-instance burst, filtered by
      `posEnd` proximity like the treasure test) + full regression on both backends, all pass.
- [x] VISUAL-013 — Pollution puff: ObjectType36 on environmental triggers — **done 2026-07-14**,
      the FIFTH real particle effect built, and the most complex one so far. Real trigger is
      `Decor::MoveObjectPollution()` (`Decor.cpp:6877-6989`, confirmed function name/range via
      direct read), called once per tick regardless of vehicle state and gated by 4 separate
      `if (m_blupiXxx)` blocks (Helicopter/Overcraft/Jeep/Tank — NOT mutually-exclusive `else if`s
      in the decompiled source, but only one vehicle can realistically be active at a time), each
      with its OWN hand-tuned `m_time`/`m_blupiPhase` modulo-based emission schedule (e.g. Jeep:
      every `m_blupiPhase%50` hits 0/12/20/35 while stationary, or `%20` hits 0/3/5/11/15 while
      moving) and its OWN per-vehicle nozzle offset (`tinyPoint.X/Y`) and puff lifetime/speed
      (`num`, mostly 20, Overcraft's ascending case uses 58 with a small real random X jitter).
      Since `MoveObjectPollution()` is itself the gate (`if (!flag) return;`), a single
      unconditional per-frame call (`GEInteractionSystem::TickPollutionPuff()`, called from
      `GalaxyEggbertCnaGame.cpp` right after `interaction_.Update()`) is behaviorally equivalent
      to the real 4 separate call sites. Real self-delete at `phase>=16` (`Decor.cpp:8563-8567`).
      Found and fixed a bug in the existing `GEObjectIcons.cpp` formula for this type (wrong
      divisor 6 instead of the real `Config::ScaleDiv(2)==2`) — real `table_pollution` is a plain
      ascending range (`Tables.cpp:1494`, 179..186), so only the divisor needed fixing, unlike the
      non-monotonic tables fixed elsewhere.

      Two real simplifications, both documented in `TickPollutionPuff()`'s own comment: (1) real
      `m_blupiPhase` (Jeep/Tank's schedule) is Blupi's own animation-state-phase counter, reset on
      every real action-state change — this engine has no equivalent action-state machine yet (no
      visible Blupi model/animation exists at all, still the single largest remaining gap), so a
      monotonic never-resetting tick counter stands in for it (shared with Helicopter/Overcraft's
      real `m_time`, which IS an exact match since that one really is a monotonic global counter).
      (2) the real `m_random.get()->Next(-10,10)` (Overcraft's ascending-case X jitter) has no
      real analogue since no RNG exists anywhere else in this engine; a small deterministic LCG
      seeded by the tick counter stands in, since only cosmetic jitter (not gameplay) depends on it.

      Architecturally different from Invert/treasure/Fan-hit/dynamite-blast: this is the FIRST
      particle effect to use the engine's own EXISTING generic `AdvancePatrolStep()` machinery
      (`stepAdvanceTicks`/`patrolStep`/`posStart`/`posEnd`, already used by every other
      non-lift/crate `MobileObject`, plan.md E3D-MIG-131) directly, by setting `patrolStep=2` at
      spawn (matching real `ObjectStart()`'s own `step=2`, skipping the dwell-at-start phase) —
      rather than a bespoke hand-rolled interpolation block. **Found while building this: Invert
      burst/treasure sparkle's own posStart->posEnd slide (VISUAL-014/015/012's own corrections,
      above) duplicated this same already-existing mechanism instead of reusing it — cleaned up
      the same day** (2026-07-14, later): `SpawnInvertBurst()`/`AppendSparkleBurst()` now also set
      `patrolStep=2`/`stepAdvanceTicks=78` at spawn instead of hand-rolling their own
      `phase`-driven interpolation in the per-object loop, which now only checks the real
      self-delete condition and falls through to the shared `AdvancePatrolStep()` call, same as
      Pollution puff. Confirmed mathematically identical (both `phase` and `patrolTime` advance by
      the same `dt*20` per frame from a spawn-time 0, so the two formulas were always numerically
      equal) — a pure refactor, zero behavior change, all existing tests pass unmodified. Full
      regression re-run on both backends, all pass.

      16 new `VerifyInteractionSystem` checks (no-vehicle no-op, Jeep stationary schedule count/
      exact spawn position/posEnd/stepAdvance/patrolStep, facing-direction sign flip, Overcraft
      ascending Y-offset/stepAdvance/nozzle-offset, self-delete timing, confirmed real movement via
      the shared `AdvancePatrolStep()` slide, corrected icon values) + full regression on both
      backends, all pass. Not live-visually verified (same already-proven element.png billboard
      rendering path as every other particle effect this session).
- [x] VISUAL-014 — Invert power-up particles: ObjectType41 (4-direction burst on pickup) — **done
      2026-07-14, the FIRST real particle effect built in this engine** (data-table transcription
      approved by the user the same day). Real spawn site confirmed via direct source read
      (`Decor.cpp:6039-6051`): 4 `ObjectStart(m_blupiPos, ObjectType41, speed)` calls
      (`speed ∈ {-60,60,10,-10}`, encoding up/down/+X/-X) at Blupi's exact position, no pre-offset.
      Real `Decor::SearchDistRight()` (confirmed name, `Decor.cpp:7628-7653`) short-circuits to a
      flat `return 500` for this specific ObjectType (and 36/39/42/93) — no raycast/wall-collision
      logic needed at all for this effect, just a fixed 500-real-px offset in the bucketed
      direction, converted to this engine's world units via the same 64px-per-tile scale used
      throughout. Real self-delete at `phase>=16` (`Decor.cpp:8575-8582`,
      `Config::ScaleTime(16)==16` at this build's 20Hz reference rate, confirmed identity function)
      — `phase` itself is already advanced generically by `GEWorldRuntime::Update()`, no new
      increment logic needed. New `GEInteractionSystem::SpawnInvertBurst()` (called directly by
      `GalaxyEggbertCnaGame.cpp` at its own existing grant site, since `GEInteractionSystem::
      Update()` has already returned by the time that fires). Along the way, found and fixed a
      real bug in the ALREADY-WRITTEN (but until now unused) `GEObjectIcons.cpp` icon formula for
      this type: wrong divisor (6 instead of the real `Config::ScaleDiv(2)==2`) — see VISUAL-015
      for the ObjectType42 half of this same fix. 8 new `VerifyInteractionSystem` checks (spawn
      count, real distance, self-delete timing, corrected icon values) + full regression on both
      backends, all pass. **Live visual verification was attempted but inconclusive** — the real
      sprite (element.png icon 179, directly pixel-inspected: a small ~5%-coverage white/gray
      blob) is confirmed to exist at the exact right position via a live debug print, but was not
      clearly distinguishable in headless screenshots at several distances/angles within a
      reasonable time budget; the underlying position/timing/icon-value math is independently
      confirmed correct via the unit tests above and via direct comparison against the real
      source, and reuses the SAME already-proven billboard rendering path used successfully by
      every other element.png-sourced object in this engine (treasures, keys, etc.) — not a new
      rendering mode needing its own visual proof the way TILE-055 did.
      **Correction, 2026-07-14 (later the same day):** found and fixed a real animation bug in this
      original implementation, discovered while investigating a DIFFERENT ObjectType (Pollution
      puff, `ObjectType36`) that led to re-reading `Decor::ObjectStart()`'s full body directly for
      the first time (`Decor.cpp:7805-7873`). The real function does NOT place the object
      instantly at its final offset target as a static burst — it sets `posStart` to the ORIGINAL
      spawn point (Blupi's own position, unchanged) and `posEnd` to the computed 500px-out target,
      `step=2`, and `stepAdvance=Config::ScaleTime(|speedMagnitude*500/64|)` (78 ticks for this
      effect's real ±10 magnitude at this build's 20Hz rate) — the shared `MoveObjectStepLine()`
      state machine (`Decor.cpp:8005-8145`) then linearly interpolates `posCurrent` from `posStart`
      toward `posEnd` over those 78 ticks. Since this object always self-deletes at `phase>=16`
      (~20% of the way), it visibly slides a SHORT distance from Blupi's exact position rather than
      appearing instantly ~7.8 world units away and sitting there for its whole life — the previous
      implementation had the right final target distance but the wrong mechanism (static instead of
      animated-from-origin). Fixed by keeping `posStart` at the origin and only setting `posEnd` to
      the offset target, then linearly interpolating `currentX/Y/Z` from `posStart` to `posEnd`
      using `phase/78` each tick (reusing `phase` directly as the interpolation counter, since nothing
      else needs a separate `time` counter for these short-lived objects). Existing tests updated to
      check the real `posEnd` target and the real starting-at-origin position instead of an
      instant final distance. Also found, while re-deriving the exact per-direction arithmetic, that
      the SAME real function short-circuits at `speed=0` to skip the whole offset/step-2 branch
      entirely — already correctly modeled for `SpawnFanHitFlash()`/`AppendDynamiteBlastFlash()`
      (both use `speed=0`), so those two are NOT affected by this bug. Full regression re-run on
      both backends after the fix, all pass. **Also found (not yet investigated further):** a THIRD
      real `ObjectType41` trigger site exists (`Decor.cpp:6608-6612`, gated on `BlupiAction::Clear4`)
      that spawns only 3 directions (`-70,20,-20`, no "down") — distinct from the two already-cited
      grant sites (`Decor.cpp:5189-5192`/`5583-5586`/`6048-6051`, which are 3 literal call sites for
      the SAME real trigger event and don't need separate handling). What `BlupiAction::Clear4`
      actually corresponds to (and whether this engine already models it under a different name) is
      unresearched — flagged as an open question, not fixed here.
- [x] VISUAL-015 — Invert expire particles: ObjectType42 (4-direction burst on expiry) — **done
      2026-07-14, see VISUAL-014's writeup for the shared implementation** (`SpawnInvertBurst(...,
      isGrant=false)`). Real spawn site confirmed via direct source read (`Decor.cpp:5137-5158`):
      same 4 directions, but pre-offset 100 real-px TOWARD Blupi before the same 500px push,
      netting exactly 400 real-px (closer than grant's 500px) — confirmed by working through the
      real per-direction arithmetic by hand for all 4 cases, not assumed symmetric with grant.
      Fixed a second real bug in `GEObjectIcons.cpp`'s existing icon formula for this type: it
      ascended past 186 (`186 + (p/6)%8`, reading out-of-range/unrelated sprite-sheet icons)
      instead of matching the real `table_invertstop` array's exact reverse order (186 down to
      179) — corrected to `186 - (p/2)%8`.
- [x] VISUAL-016 — Goo particle: ObjectType34 sticks to geometry (element.png, 25 frames) —
      **fixed 2026-07-20**: 2 real bugs, same shape as `VISUAL-021`. (1) `GEObjectIcons.cpp`'s icon
      formula (`168 + (p/6)%25`) assumed an ascending range with the wrong divisor -- the real
      `table_glu` (`Tables.cpp:1610-1615`) is byte-identical to `GEBlupiController`'s own already-
      approved `kGluFrames` (same real table, reused for Blupi's Glu death-cause animation),
      oscillating within icons 168-171 with `Config::ScaleDiv(1)==1` (no division). (2) the real
      "sticks to geometry" behavior (`Decor.cpp:8099-8104`) was entirely unmodeled: arriving at
      posEnd collapses `posStart`/`posEnd` onto the landing spot (added as a 3rd branch in
      `AdvancePatrolStep()`'s existing per-type arrival special-case, alongside `ObjectType23`/`15`'s
      self-delete), so it never recedes back via step 4 like a normal patrol object. **Confirmed
      via a direct search of every `worlds/*.txt` file that ObjectType34 is never actually placed
      in any real mobile-eggbert level** (no `MoveObject: type=34` anywhere), and no `ObjectStart`
      call spawns it dynamically either -- genuinely dead/unexercised content in the original game
      itself, same category as `Clear5-Clear8`, not a gap unique to this port. Fixed anyway since
      the fix is small, safe, and reuses already-validated data, in case a future custom/edited
      world ever places it. 6 new `VerifyInteractionSystem` assertions (icon formula + a synthetic
      patrol-to-arrival rig, since no real placement exists to test against).
- [x] VISUAL-017 — Magic track sparkle: ObjectType27 trail effect — **done 2026-07-14**, built
      together with VISUAL-011 (Shield trail, ObjectType57) — same mechanic, same
      `GEInteractionSystem::TickMagicTrail()`/`ResetMagicTrail()`, see VISUAL-011's full writeup
      for the real trigger/self-delete/icon-formula-fix details specific to this type
      (`table_magictrack`, phase>=24 self-delete, Power's own grant site).
- [ ] VISUAL-018 — Helicopter debris: ByeByeHelico float-based debris pool when helico destroyed
- [x] VISUAL-019 — Bridge construction animation: ObjectType52 (157 frames) modifies static decor
      — **stale checkbox, closed 2026-07-17**: the billboard genuinely renders (`IsObjectMPngSourced()`
      dispatch, `GalaxyEggbertCnaGame.cpp:2707`) and the real GAMEPLAY effect (terrain grid
      overwritten live during construction) is done (`157`/`PICKUP-064`) — the only real gap is
      that `GEObjectIcons.cpp`'s icon is frozen at frame 1 (`365`) rather than animating through
      all 157 frames, because the real frame range overflows `object-m.png`'s bounds
      (`365+156=521 > 439`, already documented in the code's own comment) — a genuine, previously
      acknowledged sheet-size limitation, not an unstarted feature.
- [x] VISUAL-020 — Dynamite fuse animation: ObjectType56 (100 frames) with blast events at phases
      50-69 — **stale checkbox, closed 2026-07-17**: same situation as `VISUAL-019` — the billboard
      renders via the generic element.png path (not excluded by `IsUniformCubeObject()`/
      `IsObjectMPngSourced()`), and the real blast-event GAMEPLAY (9 real blast ticks, `Phase 15
      155`) is fully done. The icon is frozen at frame 1 (`253`) rather than animating, same reason
      as `VISUAL-019` — element.png's real 290-icon grid (29 rows x 10 cols, `GEObjectIcons.cpp`'s
      own comment) is too small for the real 100-frame range (`253+99=352 > 289`), a genuine,
      already-documented sheet-size limitation, not an unstarted feature.
- [x] VISUAL-021 — Tentacle hazard animation: ObjectType53 (45 frames, explo.png) — **fixed
      2026-07-20**: the previous frozen-at-frame-86 case (`GEObjectIcons.cpp`) reasoned "45 frames
      would exceed the sheet" from a naive ascending-range assumption -- checked the real
      `table_tentacule` (`Tables.cpp:1457-1464`) directly and it's bounded, oscillating within
      icons 70-86 (rise, blank at the peak, descend through full extension, blank fully retracted)
      -- it never overflows explo.png's valid range at all, same category of previously-wrong
      assumption as `kExplo1-4`/`kTresorTrack` (all fixed 2026-07-14). Now animates through the
      real 45-frame table, including its 2 real `-1` blank frames (the renderer's existing
      `-1`-skip support already handles them). 6 new `VerifyInteractionSystem` assertions.
- [ ] VISUAL-022 — Sky gradient per world region (zenith/horizon colours)
- [ ] VISUAL-023 — Per-world fog (fog color + fog range per region)
- [ ] VISUAL-024 — Lightning visual: tiles 66-68 draw 13 px higher; ch69 sound — **split
      2026-07-17**: the ch69 zap sound half is now done (see `SOUND-079`) — real
      `Decor::BlitzActif()` (`Decor.cpp:620-634`), a Blitz(305)-floor-with-BlitzEmitter(304)-above
      pair, on a fixed 6-tick-per-100 pattern; this engine plays it once per matching tick
      globally rather than per-tile (its `GESound::Play()` has no positional audio at all, so a
      one-time lazy "does any qualifying pair exist in the world" scan is behaviorally equivalent
      to a real per-visible-tile check, at negligible cost — new `GEInteractionSystem::
      HasBlitzEmitterPair()`, `BlockTypes::BlitzEmitter=304`). The "tiles 66-68 draw 13px higher"
      visual half is a DIFFERENT, unrelated mechanic — verified directly against
      `Decor.cpp:716-735`: it's a pure 2D sprite-corner-anchor pixel nudge in the separate
      `m_bigDecor` decoration-layer draw loop (icons 66-68 there, nothing to do with the Blitz
      floor grid), the same category of non-portable 2D-anchoring artifact already dismissed
      elsewhere in this codebase for center-anchored billboards (e.g. `SpawnFanHitFlash()`'s own
      -34/-34 pixel offset) — left un-implemented, not a gap.
- [ ] VISUAL-025 — "EXIT OPEN!" text pop-up with sparkle effect when exit unlocks
- [x] VISUAL-026 — `GetObjIcon()` systematic audit sweep — **7 more real bugs found and fixed
      2026-07-20**, a follow-up pass after `VISUAL-016`/`021` (systematically checked every
      remaining case against its real `Decor.cpp`/`Tables.cpp` source, not just a sample):
      - `ObjectType37`: real `table_clear` (`Tables.cpp:1623-1632`) oscillates within icons 40-47
        (identical to `GEBlupiController`'s own already-approved `kClear1Frames`), was a naive
        ascending-range guess, same category as `VISUAL-016`/`021`; wrong divisor too (6 vs real 1).
      - `ObjectType97`: real `table_follow2` (`Tables.cpp:1539`) steps by 2 (256,258,260,262,264),
        was assumed step-of-1; wrong divisor too (6 vs real 1).
      - `ObjectType31`/`2`/`3`/`16`/`200`/`201`/`202`/`203`: all plain ascending ranges with the
        shape already correct, but the divisor was wrong in every case (mistranscribed as 6
        instead of the real per-type `Config::ScaleDiv()` value -- 2 for `31`/`2`/`3`, 1 for
        `16`/`200`-`203`), so each animated 2-6x too slowly. Verified directly against
        `Decor.cpp:8202-8246`/`8359-8363`.
      - `ObjectType5` (treasure chest): the wave's reversal point was off by one -- real
        `Decor.cpp:8297-8307` peaks at icon 11 (not 10) and descends to icon 1 (not 0) before
        wrapping, an asymmetric 0..11..1 triangle, previously computed as a smooth symmetric
        0..10..0 wave.
      20 new `VerifyInteractionSystem` assertions, one per confirmed real value.

      **Part 2, same day**: finished the sweep. `ObjectType38`'s `kElectro` (`Tables.cpp:1640-1651`)
      cross-checked byte-for-byte -- already exactly correct, no bug. The remaining object-m.png
      Category B types and the key/shield/power/invert/chenille/follow1 family turned up **12 more
      real bugs** (11 wrong divisors + 1 undersized array), all in the same "mistranscribed
      `Config::ScaleDiv()` divisor" shape found in part 1 above -- every affected array's *values*
      were already byte-correct, only the pacing was wrong:
      - `ObjectType14`/`15`/`35` (object-m.png water splash/bubble/tiplouf): divisor was 1
        (undivided), real is `ScaleDiv(2)==2` (`Decor.cpp:8607/8619/8625`).
      - `ObjectType49`/`50`/`51`/`21` (the 4 key types): divisor was 9, real is `ScaleDiv(3)==3`
        (`Decor.cpp:8319-8337`).
      - `ObjectType25` (shield pickup): divisor was 6, real is `ScaleDiv(2)==2`
        (`Decor.cpp:8344-8348`) -- **and** the real `table_shield` (`Tables.cpp:1709-1713`) has 16
        entries (144-151, then 266-273), not 8 -- `kShield` was silently missing its entire second
        half.
      - `ObjectType24` (skateboard pickup shimmer): divisor was 3, real is `ScaleDiv(1)==1`
        (`Decor.cpp:8339-8343`).
      - `ObjectType26` (power pickup): divisor was 6, real is `ScaleDiv(2)==2`
        (`Decor.cpp:8349-8353`).
      - `ObjectType40` (invert pickup): divisor was 4, real is `ScaleDiv(2)==2`
        (`Decor.cpp:8354-8358`).
      - `ObjectType47`/`48` (caterpillar lift tracks): divisor was 6, real is `ScaleDiv(1)==1`
        (`Decor.cpp:8193-8200`).
      - `ObjectType96` (follower wake indicator): divisor was 3, real is `ScaleDiv(1)==1`
        (`Decor.cpp:8217-8221`).
      Deliberately left alone: `ObjectType4`/`17`/`20`/`32`/`33`/`44`/`54` (bulldozer/fish/bird/
      blupih/wasp/creature)'s `GetObjIcon()` fallback cases -- these have no real matching formula
      to compare against at all (the real animation for these 7 types is driven by the patrol
      turn/walk step machine, `GetBulldozerIcon()`/`GetFishIcon()`/etc. above, already fixed
      separately, `ENEMY-013` onward); this fallback is a deliberate, already-documented
      approximation for context-free callers (the editor palette), not a mistranscription.
      15 more `VerifyInteractionSystem` assertions (35 total for `VISUAL-026`), plus 4 pre-existing
      `Plouf`/`Tiplouf` assertions elsewhere in the file updated (they were unknowingly written
      against the old wrong divisor -- same array indices, phase values doubled to match).

---

### 2.13 Tests & Quality

**Re-verified against source 2026-07-14**, and **TEST-002 fixed the same day**. There are now **9
real test/verify binaries**: the gtest-based `GalaxyEggbertWorldsTests` (already
`gtest_discover_tests()`-registered) plus 8 scripted `VerifyXxx` tools (`VerifyBlupiMovement`,
`VerifyInteractionSystem`, `VerifyGEInputPad`, `VerifyGESaveData`, `VerifyMoveObjectTypesCna`,
`VerifyBigDecorParsingCna`, `VerifyTerrainAnimDivisor` added for TEST-007, and `VerifyTileUvBounds`
added for TEST-004, both the same day) that are ctest-registered — `add_test()` exists for all 8 in
`CMakeLists.txt` (with an explicit repo-root `WORKING_DIRECTORY` for the 3 that default to relative
asset/world paths), so `ctest --test-dir build-cna` alone now runs and reports all 9 tools' full
results (77 tests total including third-party dependency suites; confirmed passing on both EasyGL
and Vulkan build trees). TEST-008/009/010's underlying *behavior* was never an unverified gap (the
manual tools already covered it) — only the "automated/CI-checked" framing was missing, and that's
now closed too.

- [x] TEST-001 — `GalaxyEggbertWorldsTests`: engine-independent unit tests (BlockTests, BitPackingTests, ChunkTests, WorldTests, BlockMetadataTest, MoveObjectRecordTests) — **count reconciled 2026-07-14**: 64/64 (confirmed via a fresh `TEST(...)`/`TEST_F(...)` grep across `tests/GalaxyEggbert/`), not the previously-quoted 63.
- [x] TEST-002 — ctest discovery in the CNA build dir — **fixed 2026-07-14**: `GalaxyEggbertWorldsTests` was already `gtest_discover_tests()`-registered (confirmed live: `ctest -N` found all 64 cases even before this fix, since a sibling dependency's own CMakeLists.txt already calls `enable_testing()` transitively) — the real gap was the 6 `VerifyXxx` binaries having no `add_test()` at all. Added one for each (`CMakeLists.txt`), with `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` for the 3 that default to repo-root-relative paths. `ctest --test-dir build-cna` now runs all 7 tools' full suites in one command.
- [x] TEST-003 — Test: all mobile-eggbert world files parse without error — **done 2026-07-14**: `VerifyMoveObjectTypesCna` now also sweeps every real `../mobile-eggbert/worlds/*.txt` file (enumerated at runtime via `std::filesystem::directory_iterator`, not a hardcoded list, so it stays accurate as files are added/removed) and confirms `GEWorldRuntime::LoadFromMobileEggbertFile()` returns success for each — all 78 real world files parse without error. The pre-existing curated per-`ObjectType`-example checks are unchanged and still run alongside it.
- [x] TEST-004 — Test: `BlockTypes::tileUV` returns valid UV for all known icon IDs — **done
      2026-07-14, and found a genuine but low-impact boundary bug along the way.**
      Directly computed `tileUV()`'s implied atlas-pixel rect for every icon near the end of the
      0..440 range and cross-checked against the real `object-m.png` (1301×1431, confirmed via
      direct Python/PIL inspection): icon 440's computed rect is `(1,1431)`-`(65,1495)` —
      **entirely outside the real image's 1431px height** (a crop attempt fails outright, would
      read past the file). Icons 0..439 all fit correctly (20 cols × 22 rows × the real 65px
      pitch = exactly 1431px tall, confirmed `1 + 22*65 == 1431`) — this is specifically a
      1-icon overshoot at the very last slot, not a broader miscalibration. Corroborating evidence
      found independently elsewhere in this same codebase: `GEObjectIcons.cpp`'s own
      `ObjectType52` comment already reasons about sheet overflow using bound **439** ("157 frames
      would exceed the sheet (365+156=521 > 439)"), and a nearby comment already calls this
      "object-m.png's 440-icon grid" — i.e. this discrepancy between the real 440-icon sheet and
      `BlockTypes.hpp`'s `kPassable[441]`/exhibition-loop's `icon<=440` assumption of 441 icons has
      apparently been latent and unreconciled across files for a while. **Real-world impact is
      confirmed zero**: grepped every real mobile-eggbert `worlds/*.txt` level file for icon 440 —
      zero placements anywhere; the ONLY place in this entire repo that ever rendered it was
      galaxy-eggbert's own synthetic exhibition demo (`tools/GenerateSampleWorld3D.cpp`), which
      has now been fixed to exclude icon 440 (2026-07-14, see its own updated comment) — the
      demo's own render is the sole practical fix needed today. **Deliberately NOT touched**:
      `BlockTypes.hpp`'s `kPassable[441]` array size / `isMobileTransparent()`'s `icon < 441` bounds
      check / `tileUV()` itself — these are foundational, heavily-relied-upon functions used by
      effectively everything, and reconciling the real 440-vs-441 count there needs careful,
      deliberate attention (is icon 440 a real `ObjectType`/gameplay-referenced id that simply has
      no valid `object-m.png` backing content at all, an intentional sentinel, or something else?
      `Decor.hpp`'s `MAXQUART=441` and `Decor.cpp`'s own `case 440:` branch confirm icon 440 IS a
      real, meaningful id in mobile-eggbert's data model even though it has no valid image — not
      something to guess a resolution for in a rushed pass). The scripted assertion itself is now
      written (`tools/VerifyTileUvBounds.cpp`, ctest-registered): confirms every icon 0..439 maps to
      an atlas rect that actually fits inside `object-m.png`'s real bounds, and explicitly locks in
      icon 440 as a KNOWN, tracked exception (asserts it does NOT fit) rather than silently ignoring
      it — if the underlying `kPassable[441]`/`tileUV()` question above is ever resolved in a way
      that makes icon 440 valid too, this test will correctly need updating, rather than staying
      silently green on a stale assumption forever.
- [x] TEST-005 — Test: `GEWorldRuntime::LoadFromMobileEggbertFile` round-trip — **done**, covered by `VerifyMoveObjectTypesCna`/`VerifyBigDecorParsingCna` against real `../mobile-eggbert` world files, now ctest-integrated (TEST-002).
- [ ] TEST-006 — Test: GameData read/write round-trip (640-byte format) — still correctly blocked: `GESaveData` (real, working, tested via `VerifyGESaveData`) deliberately does NOT use the real 640-byte binary format (see §11's own note) — this item is specifically about byte-compatible format round-tripping, which was never pursued.
- [x] TEST-007 — Test: animation-phase timing matches the real per-type `ScaleDiv()` divisors (Saw div 1, Lava div 2, Water1/Crusher/Water2/Marine/the 4 Fan icons div 3, Spike/Temp div 4) — **done 2026-07-14**. `AnimDivisor()` was a pure function trapped in `GETerrainRenderer.cpp`'s anonymous namespace with no graphics dependency of its own — extracted into `GETerrainAnimDivisor.hpp`/`.cpp` (behavior unchanged, `GETerrainRenderer.cpp` now calls the extracted version) so it could be linked into a new lightweight, engine-independent tool (`tools/VerifyTerrainAnimDivisor.cpp`, no CNA/graphics link needed, same precedent as `VerifyGESaveData`/`VerifyBlupiMovement`), now ctest-registered. 13 checks (all 8 real per-type divisor values + the non-animated-icon default fallback) confirm the exact mapping this item asked for. Full regression on both backends passes (76 tests on EasyGL, only the known pre-existing unrelated `easy-gl-resource-smoke-tests` failure; 71/71 on Vulkan).
- [x] TEST-008 — Test (new): `GEInteractionSystem` — treasure/egg/exit/key pickup collection, removal-on-contact, MAX_EGG_COUNT=10 cap, exit gating on treasures-collected — done and now ctest-integrated (`VerifyInteractionSystem`, 190+ checks as of 2026-07-13, TEST-002).
- [x] TEST-009 — Test (new): crate push validity (adjacency/floor-support/occupancy checks) and platform-lift ping-pong patrol motion — done and now ctest-integrated (`VerifyMoveObjectTypesCna`/`VerifyBlupiMovement`/`VerifyInteractionSystem`, TEST-002).
- [x] TEST-010 — Test (new): `BigDecor` billboard parsing round-trip — done and now ctest-integrated (`VerifyBigDecorParsingCna`, TEST-002).

## 3. Open Questions

Carried forward from prior research passes; none resolved yet, listed here so future work
doesn't silently re-open them or silently guess an answer:

- `[?]` **`Config::ScaleTime()`'s real scale factor** — unresolved anywhere in the research so
  far. Blocks precise conversion of tick-domain durations to real seconds across multiple
  systems: footstep interval (`E3D-MIG-083`), door-slide duration (`E3D-MIG-160`), teleporter
  delay (`E3D-MIG-147`), dynamite fuse (`E3D-MIG-155`).
- ~~`[?]` **`explo1`-`explo8` → `ObjectType` trigger mapping**~~ — **RESOLVED 2026-07-16**: the
  "many-to-one or context-dependent" premise was wrong — verified directly against
  `Decor.cpp:8395-8489`: it's a plain 1:1 mapping (`explo1`→`ObjectType8`, `explo2`→`9`, `explo3`→
  `10`, `explo4`→`11`, `explo5`→`90`, `explo6`→`91`, `explo7`→`92`, `explo8`→`93`), each with its
  own frame count/self-delete duration, no cross-cutting trigger logic to trace. Already
  implemented in `GEObjectIcons.cpp`'s `GetObjIcon()` for explo1-4/7 (fixed 2026-07-14); explo5/6/8
  were the 3 remaining cases still using an approximation formula (`(p/6) % N`) instead of an exact
  transcription — fixed 2026-07-16 alongside this research (new `kExplo5`/`kExplo6`/`kExplo8`,
  same "wrong divisor" bug class as every other explo case). 7 new `VerifyInteractionSystem`
  assertions. This open question is now fully closed, not just partially.
- ~~`[?]` **Icon 95**~~ — **RESOLVED 2026-07-17**: not ambiguous at all; the "boundary-only
  reference" framing was stale. `Tables.cpp:1872`: `table_decor_eau1[6] = {92, 93, 94, 95, 94,
  93}` — real `Decor.cpp:1068-1072` triggers this table only when the *placed* icon is `92`
  (`if (num2 == 92) { num2 = Tables::table_decor_eau1[(i*13+j*7+m_time/Config::ScaleDiv(3))%6];
  ... }`), i.e. icon 92 is the only value ever written into a world file's decor grid for
  Water1; icons 93/94/95 are pure animation-derived output frames, never placed directly.
  Icon 95 is simply phase index 3 of that 6-frame cycle. Matches
  `mobile-eggbert-reference/questionnaire-unused-tiles.md`'s own existing answer for icon 95
  ("Co to je? Odpověď: voda") and is already correctly implemented in this engine —
  `GETerrainRenderer.cpp:298`'s `kAnimWater1[6] = {92,93,94,95,94,93}` is an exact transcription,
  keyed off `BlockTypes::Water1` the same way (see TILE-018, already `[x]`). No further action
  needed; this question is fully closed, not just partially.
- ~~`[?]` **Icon 440's real meaning despite having no valid `object-m.png` backing content**~~
  — **RESOLVED 2026-07-17**. `Decor.cpp:11636-11658`'s `Decor::OpenDoorsTresor()` (with its own
  doc comment already in the real source) confirms the scheme directly: treasure-gated doors use
  consecutive icons starting at 421, where icon `421 + (N-1)` is a door requiring `N` treasures
  (`if (icon >= 421 && icon <= 421 + m_nbTresor - 1) { OpenDoor(...); }`). Icon 440 = `421 + 19`
  = a door requiring **20 treasures** — a real, legitimate id in the numbering scheme, exactly as
  `Decor.cpp`'s own `case 440:` branch (in the `IsRightBorder`-family passability switch,
  `10602-10626`, grouped with 421-439 as "passable vertically, blocked horizontally") already
  implied. Swept all 78 real `worlds/*.txt` files for icons 421-440: **421-435 and 437 are each
  placed at least once (1-15 occurrences); 436, 438, 439, and 440 are never placed in any real
  level** — the highest treasure requirement any shipped level actually uses is icon 437 (17
  treasures). Icons 436-439 nonetheless still have valid `object-m.png` backing art (confirmed by
  TEST-004: 0-439 all fit), so "unused in practice" alone doesn't explain 440's specific
  out-of-bounds gap — the actual cause is that the atlas grid is sized for exactly 440 slots
  (20 cols × 22 rows = indices 0-439), one short of the numbering scheme's theoretical maximum
  (a 20-treasure door). In other words: the treasure-door formula supports up to 20 doors
  mathematically, but the atlas was only ever laid out for 440 icons, and since no real level
  design ever approached needing a 20-treasure door, this final slot was simply never allocated
  art — not a bug ever actually hit in the shipped game, an intentional sentinel, or a
  misplaced-offset error. **Still deliberately not touched**: `BlockTypes.hpp`'s
  `kPassable[441]`/`tileUV()`/`isMobileTransparent()` bounds themselves remain unchanged (per
  TEST-004's own note) — this research explains the discrepancy, it doesn't newly justify
  editing those foundational, heavily-relied-upon functions.
- `[?]` **`E3D-MIG-015`**: whether to ask mobile-eggbert maintainers for a future
  `add_library()` target covering `Tables`/`Def`/`GameData`/`ObjectType`/`SoundChannel` — still
  open, would need explicit user approval as a separate task even if pursued.
- `[?]` **Save-format byte compatibility** (`easy3d.md` §12 Q7) — whether galaxy-eggbert should
  ever read/write mobile-eggbert's real save byte layout, vs. a fresh format. Not a default
  either way; decide when `E3D-MIG-106` is actually scoped.
- ~~`[?]` **Secret power (`Sp0`-`Sp7`) behavior**~~ — **RESOLVED 2026-07-12** (`E3D-MIG-170`):
  the "Sp0-Sp7" premise itself was wrong (those are hub-screen world-select icons, per
  `Decor::IsWorld()`, not secret-power pickups); the real 4 `SecretPower` buffs come from
  `MoveObject` pickups 25/26/30/31 instead, now implemented. See `170`'s own entry for the full
  writeup.
- `[?]` **Enemy billboard walk-cycle direction mismatch** — enemy sprites only have left/right
  side-view frames; no resolution proposed for how they should look when viewed at an oblique
  angle in true 3D (`E3D-MIG-179`). The equivalent question for Blupi himself is **resolved**
  (2026-07-10): billboard rejected outright, real 3D model required instead (`E3D-MIG-069`) —
  but enemies don't have that option yet (no enemy 3D models exist or are planned), so this
  remains genuinely open for them.
- ~~The 7 partial-support `ObjectType`s (jeep/secret-exit/skateboard/suction-cup/mirror/balloon/
  dynamite)~~ **RESOLVED 2026-07-16**: all 7 are now fully implemented (jeep `171`,
  skateboard `171`, suction-cup `170`/`173`, mirror/invert PICKUP-011, balloon/Overcraft `171`,
  dynamite `155`, secret-level exit `ObjectType21` fixed 2026-07-16 — see PICKUP-009/083) — see
  §2.7 for each. `ObjectType21` now shares the exact same real exit-gate contact logic as
  `ObjectType7` (`Decor.cpp:6158-6184`, one `if` covering both). Still NOT modeled: the real
  `m_bFoundCle`-equivalent flag it also sets, which has no consumer in this engine (door-open-on-
  win isn't modeled, see PICKUP-038/039) and so would have no observable gameplay effect anyway —
  low priority until door-state persistence exists.

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

**Status (2026-07-19): PAUSED, by explicit user request.** EDITOR-100 through EDITOR-110 are
implemented, tested, and pushed (see "What's actually built" below) — the editor is genuinely
usable today (enter from the menu, fly around, place/remove blocks and objects, box-fill, undo/
redo, save, play-test). Development stops here for now; the user wants to work on other areas of
`galaxy-eggbert` next. EDITOR-111 (sky-region picker) and EDITOR-112 (hardening pass + a
`build-cna-vulkan` re-verification) are the two remaining milestones of the approved plan, **not
started**. Picking this back up later should start from NEXT.md §8 (which still lists them as the
next smallest tasks), not from the old draft list at the bottom of this section (see its own note).

### What's actually built (EDITOR-100..110, 2026-07-18/19)

Full detail and exact commit history live in NEXT.md §2/§3 (kept there since it changes every
session); summary for anyone picking this up cold:
- Enter the editor from the real Init menu ("Editor" button), scoped to the selected gamer slot;
  a browser screen lists/creates/opens/deletes that slot's worlds under `customworlds/gamer<N>/`.
- Free-fly camera (WASD/Space/Ctrl, RMB mouse-look, scroll = fly speed), voxel raycast crosshair
  (Amanatides–Woo DDA), left-click place / middle-click remove a block, `F`-key or toolbar
  box-fill (drag a cuboid across two clicks, fills as one undo command), `U`/`R` undo/redo
  (200-deep), `Enter`/toolbar save, and a full Play-Test loop (saves, launches a real gameplay
  session against the custom world, returns to the editor on win/loss/pause-back).
- Objects mode places real `MoveObjectRecord`s (enemies, pickups, lifts) with no save/reload
  round-trip; `G`/`T`/`Tab`/`OemPlus`/`OemMinus`/`Delete` select an already-placed object, give it
  a real patrol path, and edit its speed/timing fields or remove it (EDITOR-110).
- Palette UI (redesigned twice this session, both times from direct user screenshot feedback): a
  single vertical column of solid-green buttons at the screen's left edge, matching free-eggbert's
  own editor palette — 8 paired action buttons (Undo/Redo, Save/Back, Play-Test/mode-toggle,
  BoxFill/Confirmed-All-tab), a paging row, then the current page's block/object icons one per
  row. Every cell draws the real per-type sprite (`GEObjectIcons::GetObjIcon()` plus its
  atlas-selection predicates — the same lookup already used to render real gameplay billboards),
  not a placeholder color.

### Known problems / open concerns when resuming

- **Unresolved, potentially significant**: the user reported that on a real desktop test of
  `build-cna`, no keyboard shortcut did anything at all (not even WASD camera movement), while the
  screen kept rendering/updating normally. A full read of the real SDL/CNA input pipeline
  (`Game::PollEvents()`, `SdlInputBridge`, `Keyboard::GetState()`) found nothing in it or in
  `GalaxyEggbertCnaGame`/`GEWorldEditor` that gates on window-focus state or game phase — input
  reading is unconditional every frame. A live reproduction (a windowless Xvfb display + `xdotool`
  window-focus calls, which errored since no window manager was present) made the window's
  `IsActive` flag drop to false and never recover, while the game kept rendering — the same visible
  symptom the user described. This strongly points at an OS/desktop window-focus issue (the user
  also mentioned a GNOME crash earlier the same session) rather than a code bug, but **this has not
  been confirmed fixed by the user** — the suggested next step (click directly into the game window
  before pressing keys) was given but not yet confirmed to work. If resuming editor work, check
  this first; don't assume it's resolved.
- `build-cna-vulkan` has still never been rebuilt or re-tested since the editor work began
  (EDITOR-100..110, 11 commits) — closing this gap is EDITOR-112's own job.
- The palette needed two live redesigns this session, both triggered by the user reacting to an
  actual screenshot rather than a description — first flat category-colored cells → real per-type
  icons, then that grid layout → a single-column green layout matching a `free-eggbert` reference
  screenshot the user provided (`/rv/data/documents/freeeggbert_editor.jpg`, not part of this repo).
  Worth remembering: for UI work like this, a live screenshot check earlier (before writing much
  code) would likely have caught the layout mismatch in one pass instead of two.
- No text rendering anywhere in the editor (toolbar buttons/tabs are distinguished by position and
  color only) — a deliberate, documented simplification, not an oversight; see NEXT.md §5.

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

**The task list immediately below (`EDITOR-000`-`EDITOR-010`) is the original pre-implementation
draft, written before the user approved a different, more detailed 13-milestone plan
(`EDITOR-100`-`EDITOR-112`) that the actual implementation followed instead.** Kept here only as
historical scoping context (it's still a reasonable description of the overall problem space) —
**do not use it for task tracking going forward; NEXT.md §3/§8 is the real, current source of
truth for what's done and what's next.**

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

## 7. Correctness Infrastructure & Dual-Renderer — Vision (not a committed task list)

Source documents (2026-07-20, merged from a separate branch into `develop`): `REMAKE-ANALYSIS.md`
(root-cause analysis of why the same bug classes keep recurring), `renderers.md` +
`renderers-next-steps.md` (a dual-renderer architecture proposal), and the first data-contract
artifact of that proposal, `src/GalaxyEggbertCNA/Game/GESceneFrame.hpp` (currently unwired —
nothing includes it, `Draw()` is not repointed to it, no `IGameRenderer` exists yet). This section
records the assessment of that material after independent review, so a future session doesn't have
to re-derive it. It is deliberately written as **direction, not a queue of tasks to execute** —
nothing here is authorized to start without its own explicit scoping and user sign-off, same as
every other "needs the user's own call" item elsewhere in this file.

### The diagnosis is accurate, not theoretical

`REMAKE-ANALYSIS.md`'s central claim — **correctness is verified by a human looking at a
screenshot, and that is the actual bug factory** — was independently checked against this
codebase (not taken on faith): `GEObjectIcons.cpp` really does carry 34 `Fixed 202...` annotations,
`GalaxyEggbertCnaGame::Update()`/`Draw()` really are ~2073/~817 lines respectively. The Saw-blade
render-orientation saga the same day (2026-07-20, 6+ rounds of "here's a screenshot, is this
right?" before landing on a stable answer) is a live, first-hand instance of exactly the failure
mode the document describes (RC-1) — not a hypothetical.

### What is cheap and worth doing when there is appetite for it (hours, not a 30-hour epic)

These are small, self-contained, and each pays for itself immediately — no dependency on the
riskier items below:

- **A permanent, committed golden-screenshot harness (`REMAKE-ANALYSIS.md` P0-1a/b, scoped down to
  a first useful slice).** The ad-hoc pattern already used every session (`xvfb-run` + an
  env-var-gated debug hook, a screenshot, then a full revert before commit) is 90% of this already
  — the only change is to stop throwing it away and instead commit one small, deterministic
  headless-capture target wired into `ctest`, with a handful of golden images diffed. This does not
  require the `SceneFrame`/`IGameRenderer` refactor below; it can be built directly against
  today's `Draw()`.
- **A first data-integrity pass over `GetObjIcon()` (P0-2, scoped down).** A test that checks frame-
  array lengths and known values against `mobile-eggbert-reference/08-animations.md` for the
  objects already flagged `Fixed` would have caught several of this session's own bugs before a
  screenshot was needed. Does not require or license copying mobile-eggbert data — it validates
  numbers already transcribed into this repo, against the existing reference *doc*, not against
  `../mobile-eggbert` source directly (that would need separate approval, see `easy3d.md` §5.7 and
  this file's own reuse rules).
- **Marking the ~137 unverified render-mapping icons as an explicit, queryable set (P0-3).**
  Bookkeeping, not code.

### What is a real, multi-day undertaking and should NOT be started casually

- **A shared collision/movement resolver (P1-1)**, replacing the fragmented per-mechanic probes
  (`GroundHeightAt`/`CeilingHeightAt`/`TryMoveAxis`'s step-up gate/etc.) with one swept resolver all
  movement modes route through. Real value (it would close the known airborne-wall-clip gap as a
  side effect), but it touches already-tuned, already-verified movement across every mode
  (grounded, airborne, every vehicle) — exactly the kind of change that needs its own scoped task,
  a behavioral-trace safety net, and the user's explicit go-ahead before a line of code changes.
- **A handler-table refactor for `ObjectType` dispatch (P1-2)**, replacing the open-coded
  `if (type == N)` chains inside `GalaxyEggbertCnaGame::Update()` (~2073 lines) and
  `GEInteractionSystem::Update()` (~1677 lines, `ObjectType` referenced 214 times) with a per-type
  handler table. Real leverage (would make "fix object X" touch one place instead of scattered
  sites across two god-methods), but ~70 object types is a large, multi-session migration with
  real regression risk if not done one family at a time behind a golden harness.
- Both P1 items explicitly depend on the P0 harness existing first (per `REMAKE-ANALYSIS.md`'s own
  sequencing) — doing them before P0 exists would reintroduce the exact "unverified churn" problem
  the whole document is arguing against.

### The dual-renderer proposal (`renderers.md`) is a separate, bigger decision — do not fold it in

`renderers.md` proposes something structurally different from "reduce the bug factory": a second,
hi-fi renderer (`GERendererHi`) alongside today's renderer, both consuming a shared `SceneFrame`.
This **widens the "Direct CNA + Easy3D is the sole target" lock** (see "Current Direction Lock" in
`CLAUDE.md`) rather than just fixing how correctness is checked — a materially different kind of
decision that belongs to the project owner alone, same weight as the original Simple3D-to-CNA
pivot. The document's own owner-decision log records path A (same CNA/Easy3D base, hi-fi via
upgraded shaders/materials, not a second engine) as already chosen (2026-07-20) — but choosing
*which* dual-renderer path is not the same as choosing *to build one at all*, and that broader
choice is not recorded as settled anywhere in this file. Note also the genuine synergy if it is
ever pursued: `renderers.md` Phase 1's `SceneFrame` is the same artifact the P0-1 golden harness
above needs, so a future decision to do the `SceneFrame` extraction would advance both efforts at
once — but that is a reason to notice the overlap, not a reason to start the extraction
unprompted.

### Recommendation, if and when this is picked up

Start (if at all) with the cheap P0 slice above, scoped to a single small deterministic scenario —
it is useful on its own, blocks nothing else, and does not require deciding anything about
collision, object dispatch, or a second renderer. Treat P1-1, P1-2, and the dual-renderer direction
as three independent, later decisions, each requiring its own explicit user go-ahead before any
code is written — do not bundle them into one "modernization" effort.

### Task breakdown — correctness infrastructure only (2026-07-21)

Turned into concrete tasks at the user's explicit request, **scoped to the correctness-
infrastructure thread only** (`REMAKE-ANALYSIS.md`'s P0/P1/P2) — the dual-renderer thread
(`renderers.md`'s 5 phases) is deliberately NOT broken into tasks here; it stays as direction-only
prose above until the user separately asks for that breakdown too. Listed as a normal, unblocked
task queue (the user's own call, given when asked) — this does **not** relax the P1 items' own
"needs its own scoped task and explicit sign-off before a line of code changes" requirement from
the assessment above; that requirement is about the *moment code starts*, not about whether the
task may be described here. Confirm with the user before starting `INFRA-005`/`INFRA-006`
specifically, same as any other large/risky item elsewhere in this file.

- [x] `INFRA-001` (`REMAKE-ANALYSIS.md` P0-1a) — **done 2026-07-21.** Promoted the ad-hoc
      `xvfb-run` + env-var-gated debug-hook pattern (used and fully reverted every session before
      this — see NEXT.md §3's own Saw/death-lock/life-loss writeups) into a **permanent, committed**
      deterministic capture mode, not a new tool target: `GalaxyEggbertCnaGame::
      EnableGoldenCaptureMode()` (set via a real `--golden-capture` CLI flag parsed in `main.cpp`,
      not an env var this time — a genuinely discoverable, documented feature). On enable: skips to
      Play, loads the fixed `worlds3d/world999.vwr` demo world, lets the game's already-deterministic
      fixed timestep (CNA's default `IsFixedTimeStep=true`, 1/60s, confirmed in `Game::Game()` — no
      new timestep-injection code needed) run, captures `golden_frame_0060/0120/0180.png` at 3 fixed
      tick indices, then calls `Exit()` once the last one is written — self-terminating, no external
      `timeout` needed (confirmed: exit code 0, not a `timeout`-forced 124). **Reproducibility
      confirmed empirically**: 2 independent runs produced byte-identical PNGs (`md5sum` match on
      all 3 files). No golden-image diffing yet (`INFRA-002`) — this step only proves the capture
      itself is stable/reproducible, which it now is. Deliberately **not** wired into the default
      `ctest` run (needs a real display/GL context, same reason the existing live-headless-check
      step is already separate from `ctest` throughout this project) — documented as its own command
      in `NEXT.md` §7 instead.
- [x] `INFRA-002` (`REMAKE-ANALYSIS.md` P0-1b) — **done 2026-07-21.** Golden-image diffing on top
      of `INFRA-001`: `tests/golden/golden_frame_0060/0120/0180.png` are the approved reference
      copies (captured the same way `INFRA-001` proved reproducible — no true "original game"
      golden exists for this engine-specific demo world, so these are previously-approved Galaxy
      Eggbert frames, catching *regressions*, which is the framing this task itself allows). New
      `tools/verify_golden_frames.sh <build-dir>`: runs `GalaxyEggbertCNA --golden-capture`, then
      exact-byte-compares (`cmp`, not a perceptual/fuzzy diff — full determinism is already proven)
      each fresh capture against its reference, printing `PASS`/`FAIL` per frame plus an
      `ALL CHECKS PASSED`/`SOME CHECKS FAILED` summary line and a matching exit code, same
      convention as the `VerifyXxx` C++ tools. Verified both directions live: passes against the
      real references, and correctly reports `FAIL` (confirmed via a deliberately corrupted
      reference copy, restored after). **Not wired into the default `ctest` run** (needs a real
      display/GL context, same reason `INFRA-001` isn't) — run explicitly, documented in `NEXT.md`
      §7.
- [x] `INFRA-003` (`REMAKE-ANALYSIS.md` P0-2) — **done 2026-07-21.** New
      `tools/VerifyGetObjIcon.cpp` (headless, engine-agnostic — `GEObjectIcons.cpp` only depends on
      `def/ObjectType.hpp`, same precedent `GenerateSampleWorld3D` already established), registered
      with `ctest`. 232 checks covering every `case` in `GetObjIcon()`'s switch (all confirmed-real
      `ObjectType`s, not just the 34 with a `Fixed 202...` note — the extra coverage was cheap once
      the harness existed): exact per-tick sequence for every table-driven type (transcribed
      directly from `GEObjectIcons.cpp`'s own already-committed arrays), exact formula for every
      plain-ascending/descending/triangle-wave type, and the fixed icon for every static type.
      Frame-array length (period) cross-checked inline against `mobile-eggbert-reference/
      08-animations.md` §3.1/§3.2/§4 wherever that doc documents one — including one genuine
      discrepancy the check surfaced: `08-animations.md`'s own `ObjectType25` row still says "8
      frames," but that row's own text already flags itself as stale (the real `table_shield` has
      16 entries, matching `GEObjectIcons.cpp`'s current, already-fixed array) — the test asserts
      16, the correct current value, with a comment explaining the doc's own self-flagged staleness
      rather than silently trusting either source. A regression test, not a fresh independent
      re-verification against `../mobile-eggbert` (out of scope, would need separate approval) —
      **verified it actually catches regressions**, not just trivially passing: deliberately
      mutated one divisor in `GEObjectIcons.cpp` (6→9 for `ObjectType17`), confirmed the test fails
      with a precise mismatch message, then reverted (confirmed via a clean `git diff`).
- [x] `INFRA-004` (`REMAKE-ANALYSIS.md` P0-3) done (2026-07-21). Marked the still-unverified 2D→3D
      render-mapping icon identities (`mobile-eggbert-reference/15-3d-render-mapping-design.md`
      §10.2-§10.5, as applied per-icon in `02-tiles.md`'s Category column) as an explicit,
      queryable "unverified" set:
      - New `include/GalaxyEggbert/UnverifiedRenderMapping.hpp` — a header-only, engine-agnostic
        table (`GalaxyEggbert::UnverifiedRenderMapping::kEntries[]`, `IsUnverified(iconId)`)
        listing every icon ID whose render-mode recommendation is still only a first-pass agent
        visual guess, never confirmed by direct user identification the way §11's other 34 icons
        were. **131 icons**, not the ~137 originally estimated in this doc's own prose — generated
        directly from `02-tiles.md`'s actual `"(§10.N)"`-cited rows (the current, applied ground
        truth) rather than recounting the design doc's older approximate estimate; the 4-icon gap
        is explained by icons 61/62/65/67 (Billboard, but tagged `"(user 2026-07-07)"` — already
        resolved by direct identification in §11, correctly excluded) and Saw/SawStopped
        (378/379, resolved to `InnerFlatPlate` via the 6-round live-feedback saga, also correctly
        excluded — see `NEXT.md`).
      - New `tools/VerifyUnverifiedRenderMapping.cpp`, wired into `ctest` (pure data table, no
        graphics dependency, same precedent as `VerifyTileUvBounds`): asserts the exact count
        (131, so a future identification pass shrinking the table is a deliberate, visible edit to
        this test rather than a silent drift), no duplicate/out-of-range icon IDs, and spot-checks
        `IsUnverified()` both ways (378/379/401/61/10 must read as NOT unverified; 201/391/92/182
        must read as unverified).
      - Verified the test has teeth: deliberately duplicated an entry (count 131→132), rebuilt,
        confirmed both the count-mismatch and duplicate-entry checks failed with precise messages,
        then reverted and confirmed a clean re-run (131 entries, all 8656 checks pass).
      - Bookkeeping only, as scoped — no rendering code touched. This table is what `INFRA-001`/
        `INFRA-002`'s golden-frame harness can be checked against before/after any future render
        change to one of these 131 icons, so such a change can't quietly be treated as "already
        correct" without going through the same direct-identification process §11 used for the
        other 34.
- [x] `INFRA-005` (`REMAKE-ANALYSIS.md` P1-1) done (2026-07-21). Scoped with the user first (full
      unified resolver, sub-tile fidelity, `BlupiAdjust`-style penetration recovery all explicitly
      chosen over the smaller/cheaper alternatives offered), then researched real mobile-eggbert
      before writing any code:
      - **Research** (2 forks): `Decor::TestPath` is a Bresenham pixel march of one rect from
        `start` to `end`, testing `DecorDetect()` at every step, rewinding to the last clear
        position on the first hit — X and Y resolved TOGETHER in one march, no grounded/airborne
        branch anywhere, called unconditionally from `BlupiStep()` for every mode including every
        vehicle (confirms this engine's old `!m_onGround` bypass was a real, confirmed divergence,
        not a simplification). `DecorDetect()`'s real solidity is a 4x4 (16px) per-icon quarter-cell
        mask (`Tables::table_decor_quart`, 7056 entries), not whole-tile. `BlupiAdjust()` (real
        penetration recovery, called unconditionally first thing every frame except ghost mode) is
        5 phases (down/right/left/right-again/left-again, 50 iterations of 2px each, silently
        accepts residual penetration) — phases 2/3 vs 4/5 only differed by which vertical band of
        the real RECT (`BlupiRect`, this engine has always used a point instead) they tested,
        meaningless for a point, so this collapses to 3 real distinct attempts.
      - **Data**: `Tables::table_decor_quart` (441 icons × 16 subcells, 0/1, row-major top-left
        origin) extracted mechanically (not hand-retyped) and independently re-extracted a second
        time to cross-check — byte-identical, zero discrepancies. Transcribed verbatim into
        `include`-free, header-only `src/GalaxyEggbertCNA/Game/GEDecorQuartTable.hpp`
        (`kDecorQuartTable[7056]`), explicit user approval (a real data transcription beyond the
        standing blanket approval for small tables).
      - **New API** (`GEBlupiController`): `IsPointSolid(world,x,y,z,tempPassable,checkSubcell=true)`
        (real `DecorDetect()` equivalent — sub-tile precision, real per-icon mask extruded uniformly
        along this engine's own Z axis since the real mask is inherently 2D and the original never
        had a depth axis), `ResolveMove(...)` (real `TestPath()` equivalent — one march per call,
        rewinds to last-clear on block, reports which axis blocked + `onGround`), and
        `RecoverFromPenetration(...)` (real `BlupiAdjust()` equivalent, called unconditionally at
        the top of `Step()` right after the ghost-mode early-return, matching the real call site
        exactly). `GroundHeightAt`/`CeilingHeightAt`/`TryMoveAxis` removed entirely — their only
        3 call sites now go through the new API. `HasJumpHeadroom`/`GetBarreCellType`/`ToggleGhost`
        keep using the existing coarse `IsSolidAt` (their own pre-existing "single stance"
        simplifications, unaffected by this refactor).
      - **Real bug found and fixed during implementation** (not by the research, empirically via
        `VerifyBlupiMovement` failures): this engine's Y axis uses a DIFFERENT grid convention than
        X/Z. X/Z are center-anchored (grid cell `g` owns continuous `[g-0.5, g+0.5)`, matching the
        terrain renderer's own `CubeMesh` — confirmed by reading `easy-3d/src/CubeMesh.cpp` directly:
        `min = Center - half; max = Center + half`). Y is bottom-anchored (grid cell `g` owns
        `[g, g+1)`, confirmed against the pre-existing `GetGroundBlockType()`'s own
        `lround(m_y) - 1` and old `GroundHeightAt()`'s own "+1 sits at the top surface" contract) —
        a real, pre-existing asymmetry between collision-space Y and render-space Y already known
        and explicitly worked around elsewhere in this codebase (`GalaxyEggbertCnaGame.cpp`'s
        `kPlaceholderModelYOffset`, "GetY() sits a constant 0.5 world units above the true visual
        ground surface... Deliberately NOT fixed by changing GetY()/GroundHeightAt()"). My first
        `IsPointSolid` draft wrongly used the center-anchored formula for Y too, breaking 59 of
        311 `VerifyBlupiMovement` checks (starting with "spawns grounded on the ground floor"); the
        real fix was recognizing Y needed `floor(y)`, not `floor(y+0.5)`, while X/Z keep the
        center-anchored formula.
      - **Real design tension found and resolved**: several `IsGenericHazard()` tiles Blupi's own
        hazard-detection depends on physically resting on (`GetGroundBlockType()`, checked by
        `GalaxyEggbertCnaGame.cpp`) — Lava(68), Crusher(317), Saw(378), Blitz(305), and Drip(404) —
        turned out to have an all-zero real quarter-cell mask (genuinely thin/non-bulk, independently
        corroborating `15-3d-render-mapping-design.md`'s own "ThinMechanical" findings for the exact
        same icons). Applying sub-tile precision to GROUND detection would make Blupi fall straight
        through these already-verified hazard tiles. Resolved via `checkSubcell` (default true):
        ground/ceiling resolution (`ResolveMove`'s vertical-only calls, `FindColumnGroundY`, the
        `onGround` probe) passes `false` (coarse, unaffected by real tile shape, preserving existing
        hazard-detection behavior exactly); horizontal wall collision keeps the default fine check
        (no pre-existing dependency to break, and this is where sub-tile precision + the airborne
        fix both matter). A deliberate, documented scope boundary — "one shared resolver algorithm,
        axis-appropriate solidity granularity" — not an architectural inconsistency.
      - **Step-up** (`kStepLimit`, already a CNA-only invention pre-dating this refactor — real
        `TestPath` has no such concept at all) kept as its own explicit, separate mechanism
        (`FindColumnGroundY`, mirrors old `GroundHeightAt`'s exact shape, built on the same coarse
        `IsPointSolid(checkSubcell=false)`), tried only when grounded and the general resolve found
        a horizontal block — not folded into `ResolveMove` itself, which stays a faithful,
        real-behavior-only sweep.
      - **Verification**: `VerifyBlupiMovement` went from 252/311 (59 failures, mostly the Y-axis
        bug) → 311/311 after both fixes (Y-axis convention, `checkSubcell` ground/wall split). One
        pre-existing test's expected value corrected (not a regression): the balloon-ceiling-stop
        test's old `-0.5` boundary was itself inconsistent with ground's own `+1` convention for the
        identical physical relationship — the unified resolver fixed this minor, previously-
        unnoticed inconsistency as a side effect, moving the real stopping boundary from ~2.5 to
        ~3.0 (documented inline in the test). A NEW dedicated test added for the actual confirmed
        bug this task set out to fix (airborne wall collision) — **verified it has real teeth**: a
        first draft of this test passed even with the old bug deliberately reintroduced (the jump
        landed before ever reaching the wall, never exercising the airborne case at all); redesigned
        to start right at the wall so contact happens within the brief airborne window, then
        re-confirmed it correctly FAILS with the bug reintroduced and PASSES with the fix, before
        reverting the temporary bug re-injection (confirmed via `git diff`). Full regression clean
        (80/81, only the pre-existing unrelated `easy-gl-resource-smoke-tests` failure); golden-frame
        byte-match unchanged (idle-spawn baseline untouched); live headless smoke run clean.
- [ ] `INFRA-006` (`REMAKE-ANALYSIS.md` P1-2) **Needs its own scoping session + explicit user
      go-ahead before any code changes — do not start from this line alone.** Replace the
      open-coded `if (obj.type == ObjectTypeN)` chains inside `GalaxyEggbertCnaGame::Update()`
      (~2073 lines) and `GEInteractionSystem::Update()` (~1677 lines, `ObjectType` referenced 214
      times) with a per-type handler table (`{ObjectType → update fn, icon fn, hitbox}` — a plain
      data-oriented table, not a class-per-object rewrite). Migrate one object-type family at a
      time, each behind `INFRA-001`/`INFRA-002`'s golden harness, so "fix object X" stops requiring
      a search across two god-methods.
- [ ] `INFRA-007` (`REMAKE-ANALYSIS.md` P2-1) Replace the 17 parallel `*ThisFrame()` one-frame
      boolean flags (`GEInteractionSystem` → `GalaxyEggbertCnaGame` signal bus) with one typed
      per-frame event queue. Keeps the existing, deliberately-reaffirmed `GEInteractionSystem`/
      `GEBlupiController` decoupling (see `plan.md` §6/`NEXT.md` §9 on that boundary) — only the
      hand-rolled parallel-boolean *transport* changes, not the architectural boundary itself.

      **Design proposed to and approved by the user 2026-07-21** (before any code was touched, per
      this task's own precondition above): exact count confirmed as 17 (14 in
      `GEInteractionSystem`, 3 in `GEBlupiController`), which turn out to be 3 different real
      shapes, not one:
      - 14 simple one-shot signals, no payload (11 in `GEInteractionSystem`: shake×2, TankFired,
        5× secret-power grant, balloon touch/pop, Died; 3 in `GEBlupiController`: the crouch/
        look-up sound cues).
      - 2 one-shot signals WITH payload, and which can both be true in the SAME frame, consumed
        independently (`VoyagePendingThisFrame()` — 7 extra parallel fields: kind/iconId/
        isButtonChannel/worldX/worldY/isAscend/ascendOffsetY; `DeathLockRequestedThisFrame()` —
        1 extra field, `PendingDeathKind`).
      - 1 that is NOT actually a one-shot event at all: `CrateBeingPushedThisFrame()` is documented
        in its own comment as "a plain per-frame fact... no memory of its own" — the caller already
        does its own start/stop edge detection (`wasPushingCrate_`). **Explicitly excluded from the
        event queue** — forcing continuous per-frame state into a discrete event model would be a
        worse fit than what exists today, not an improvement.
      Side finding while surveying this: `DiedThisFrame()` is exercised by `VerifyInteractionSystem`
      (real, checked behavior) but never actually read by `GalaxyEggbertCnaGame.cpp` — the shipped
      game has no consumer for it today. **Decision: carry it into the new queue unchanged, don't
      use this transport-only refactor as an occasion to also drop/change it** — that's a separate
      decision for a separate day if it ever comes up.
      Approved shape: **two independent event queues** (`GEInteractionSystem::EventsThisFrame()`,
      `GEBlupiController::EventsThisFrame()`), not one shared type — preserves the existing
      deliberate decoupling between the two classes. Each event is a tagged struct (`Kind` enum +
      a few optional payload fields), matching this codebase's existing style, not `std::variant`
      (explicitly considered and declined — no precedent for it anywhere else in this codebase).
      Migration ordered smallest/lowest-risk first: **(1) `GEBlupiController`'s 3 sound-cue flags
      (done, see below) → (2) `GEInteractionSystem`'s 11 no-payload flags → (3) the 2
      payload-carrying ones (Voyage, DeathLock), most care since they touch camera-projection
      timing.** `CrateBeingPushedThisFrame()` stays a plain bool throughout, out of scope.

      **Step 1 done (2026-07-21):** `GEBlupiController`'s 3 sound-cue flags
      (`DownEntrySoundFiredThisFrame`/`UpEntrySoundFiredThisFrame`/`DownReleaseSoundFiredThisFrame`)
      replaced by `GEBlupiController::EventKind`/`Event`/`EventsThisFrame()` (a
      `std::vector<Event>`, cleared and refilled every `Step()` call the same way the 3 booleans
      used to be reset/set). Consumer (`GalaxyEggbertCnaGame.cpp`, the 3 `if (blupi_.FooThisFrame())`
      checks around the crouch/look-up sound cues) rewritten as one loop + `switch` over
      `EventsThisFrame()`. `tools/VerifyBlupiMovement.cpp`'s 10 direct-getter assertions rewritten
      against a small `hasEvent(controller, kind)` test helper, same coverage as before (entry-delay
      timing, exactly-one-frame firing, the Down→Stop-only release gate). Clean, scoped diff (4
      files: the two `GEBlupiController` files, the one `GalaxyEggbertCnaGame.cpp` call site, the
      one test file) — no old getters/members left behind (`grep` confirms zero remaining
      references). Full regression clean (80/81, only the pre-existing unrelated
      `easy-gl-resource-smoke-tests` failure); `VerifyBlupiMovement`'s own sound-cue assertions
      re-run individually and confirmed passing.

      **Step 2 done (2026-07-21):** `GEInteractionSystem`'s 11 remaining flags (excluding
      `CrateBeingPushedThisFrame` and the step-3 Voyage/DeathLock pair) replaced by
      `GEInteractionSystem::EventKind`/`Event`/`EventsThisFrame()`. Re-checking the exact payload
      shape while implementing this surfaced a correction to the design summary above: 3 of the
      11 (`PowerGranted`/`CloudGranted`/`HideGranted`) actually carry a real payload (the pickup's
      world position, previously 9 separate float members + getters,
      `Power/Cloud/HidePickupX/Y/Z()`) — only 8 of the 11 are truly payload-free. The approved
      tagged-struct shape already accommodated this without any design change, same as
      Voyage/DeathLock's own extra fields. `Died`'s "tested but no live-game-loop consumer" status
      carried over unchanged, as decided. Consumer side: added a
      `GalaxyEggbertCnaGame::FindInteractionEvent(EventKind)` private helper (returns
      `const Event*`, `nullptr` if absent this frame) so payload-carrying kinds can read their
      pickup position at the same call site, no separate position getter needed; every one of the
      8 scattered consumption sites in `GalaxyEggbertCnaGame.cpp` (`Update()`/
      `ResolvePickupFreeze()`) rewritten against it in place — same order, same interleaving with
      surrounding logic, only the storage/lookup mechanism changed (in scope: transport only, not
      restructuring when/where each signal is consumed). `tools/VerifyInteractionSystem.cpp`
      (~64 references, this task's largest test-file impact) rewritten via two small local
      helpers (`hasEvent`/`findEvent`); message-string wording updated to match (no code meaning
      change). All stale doc comments referencing the old getter names (in
      `GEInteractionSystem.hpp`/`.cpp`, `GEBlupiController.hpp`, `VerifyBlupiMovement.cpp`) updated
      too — confirmed via `grep` that zero references to any of the 11 old getter/member names
      remain anywhere in `src/`/`include/`/`tools/`. Full regression clean (80/81, only the
      pre-existing unrelated `easy-gl-resource-smoke-tests` failure); `VerifyInteractionSystem`
      itself: 547 checks, all passing. Live smoke check: `--golden-capture` run against
      `worlds3d/world999.vwr` (this refactor's own test world, exercises
      `GEInteractionSystem::Update()` every frame) completes cleanly with no crash, and
      `tools/verify_golden_frames.sh`'s 3 reference frames still byte-match exactly — confirms the
      render pipeline is untouched by this transport-only change.

      **Step 3 done (2026-07-21) — INFRA-007 fully complete, all 17 original *ThisFrame() flags
      migrated.** Voyage/DeathLock turned out to carry a much larger payload than steps 1/2's own
      design summary estimated: re-reading the header carefully before touching anything found
      Voyage actually has **11** payload fields (kind, iconId, isButtonChannel, worldX/Y/Z,
      fixedX/Y, worldIsStart, isAscend, ascendOffsetY — not "7" as first estimated), and DeathLock
      has **2** (kind, shouldRespawn — not "1"). The approved tagged-struct shape absorbed this
      without any design change, same as step 2's own Power/Cloud/Hide correction.

      New complication this step actually needed to solve (steps 1/2 didn't): unlike every other
      Kind, at most ONE `VoyageRequested`/`DeathLockRequested` event may exist per frame — the
      pre-queue flat-member representation gave this "last request this frame wins, an earlier one
      is silently dropped" behavior for free via plain assignment (already a documented, accepted
      simplification on `VoyageKind`'s own class comment, predating this queue). A `std::vector`
      wouldn't give this for free — pushing both would silently break the "at most one" invariant.
      Solved with a new private `GEInteractionSystem::ReplaceEvent(EventKind, Event)` (removes any
      existing entry of that Kind via `std::remove_if`/`erase`, then appends) — used by
      `RequestVoyage()`/`RequestClear2Ascend()` (now build an `Event` via named-member assignment
      and call `ReplaceEvent()` instead of 8-10 flat member assignments each) and all 4 death-lock
      trigger sites inside `Update()` itself (each replaced its 3-line flat-assignment block with a
      local `Event` + `ReplaceEvent()` call, same surrounding control flow/order untouched). Two
      `enum class ... : std::uint8_t;` forward declarations added (`VoyageKind`, `PendingDeathKind`)
      so `Event`'s new payload fields can be typed by them before their full definitions (which stay
      exactly where they've always lived, next to `RequestVoyage()`/the death-lock trigger sites,
      not moved for this refactor) — the only structural header change beyond straight field/getter
      swaps.

      Consumer side: `GalaxyEggbertCnaGame::ResolvePendingVoyage()`/`ResolveDeathLock()` (the only 2
      places these ever get read) rewritten against `FindInteractionEvent()` (added in step 2),
      reading payload straight off the returned `Event*` instead of ~9 individual getters each —
      confirmed the pointer stays valid across the `BeginVoyage()`/`TriggerDeathLock()` calls in the
      same function (neither touches `events_`). Test file: `tools/VerifyInteractionSystem.cpp`'s 2
      self-contained helper lambdas (`completePendingVoyage`/`completeDeathLock`, mirroring the real
      consumer functions since no test can construct a full `GalaxyEggbertCnaGame`) plus ~10 direct
      call sites rewritten the same way. All stale doc comments referencing the old getter names
      updated too (`GEInteractionSystem.hpp`'s own class comment, `GalaxyEggbertCnaGame.hpp`'s
      `ResolvePendingVoyage()`/`ResolveDeathLock()` comments). Confirmed via `grep`: zero references
      to any of `VoyagePending*`/`DeathLockRequestedThisFrame`/`DeathLockPendingKind`/
      `DeathLockShouldRespawn` remain anywhere in `src/`/`include/`/`tools/`.

      Full regression clean (80/81, only the pre-existing unrelated `easy-gl-resource-smoke-tests`
      failure); `VerifyInteractionSystem`: still 547 checks, all passing (same count as step 2 —
      confirms no coverage was lost). Live smoke check: `--golden-capture` clean, no crash, all 3
      golden reference frames still byte-match exactly. **Honest gap, not silently glossed over:**
      `ResolvePendingVoyage()`/`ResolveDeathLock()`'s real camera-projection consumer path
      (`GEHud::ProjectWorldToHudSpace()`, `BeginVoyage()` with real view/projection matrices) has no
      dedicated test coverage — this predates this refactor (no test constructs a full
      `GalaxyEggbertCnaGame`, which needs a real graphics device) and this step did not add live
      instrumentation to specifically exercise it, since every line touched there is a mechanical
      1:1 substitution (getter call → pointer-member read) with zero logic/arithmetic change, on
      top of a projection call chain this refactor never modified. Flagged here rather than
      claiming a live check that wasn't actually done.
- Standing rule, not a one-shot task (`REMAKE-ANALYSIS.md` P2-2): **reuse before re-deriving.**
  When a CNA render/math bug has a plausible 2D/pixel root cause, check whether the engine-agnostic
  `include/GalaxyEggbert/` tree or `GalaxyEggbertSimple3D` (historical reference only, but still
  readable) already solved it before re-deriving the logic from scratch — `missing.md`'s own
  lesson (a UV-bleed bug CNA reintroduced despite `BlockTypes::tileUV()` already solving it
  years earlier). No task ID; this is a practice to apply on every future render/math bug, not a
  thing to close.
- [x] `INFRA-009` (`REMAKE-ANALYSIS.md` P2-3) done (2026-07-21). New `NEXT.md` §2 "Sibling-repo
      commit pins" subsection: a table recording the exact commit each of the 4 sibling repos
      (`../easy-3d` `e2d1cfa2`, `../cna` `ac3aaaeb`, `../easy-gl` `62c0a248`, `../sharp-runtime`
      `d2cc9cce`) is currently verified against, plus a consolidated quarantine list for the known
      upstream failures (`easy-gl-resource-smoke-tests`; `build-cna-vulkan`'s `BasicEffect` `Alpha
      < 1` bug) cross-referencing `NEXT.md` §5's full root-cause writeups, with explicit guidance:
      a *different* failure showing up in a future full `ctest` run is a real signal, not more of
      the same known issue.
      While recording this, found and fixed one real staleness in the existing quarantine notes:
      the `SkinnedEffect` Vulkan Y-flip fix (§3's 2026-07-18 entry) was noted as "sits uncommitted
      in `../cna`" — checked `../cna`'s current shader source directly and confirmed all 4
      originally-reported `skinned3d*.vert.glsl` files now carry the fix line, committed. Noted in
      passing, not investigated further (out of this task's bookkeeping scope): a 5th, related
      shader (`pbr3d_skinned.vert.glsl`) lacks the same line — flagged in `NEXT.md` in case a
      floating-model bug resurfaces under a PBR-skinned material specifically. Also noted, but
      explicitly did **not** re-verify: `../cna`'s Vulkan blend-state handling has a merged fix
      (`ddef5e8f`, predates the `BasicEffect Alpha<1` bug report) that's plausibly related but
      unconfirmed — attempted one live headless check via `build-cna-vulkan` (a temporary,
      fully-reverted `phase_ == Init` screenshot gate) but the Vulkan backend never reached the
      Init screen within 15s wall-clock under `xvfb-run` in this environment, so the bug's current
      status is recorded as "not independently re-verified this session," not asserted either way.
- [ ] `INFRA-010` (`REMAKE-ANALYSIS.md` P2-4) Consider a compact, authoritative "current truth"
      index, separate from `plan.md`'s own historical log (505 KB / 5500+ lines) and `NEXT.md`
      (115 KB), so a session doesn't have to re-derive current state by reading the whole history
      every time. Lowest priority of this list — opportunistic, not blocking anything else; revisit
      only if doc-reading overhead becomes a recurring complaint.
