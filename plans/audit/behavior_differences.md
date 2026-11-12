# Behaviour differences (BD-NNNN)

One finding each: what differs, the reference evidence, how it was reproduced, the fix and its
validation. Kinds: `gameplay`, `visual`, `editor`, `doc` (a wrong statement in our own documents).
Status values as in `../GALAXY_EGGBERT_FIDELITY_AUDIT.md` §2.

## BD-0001 — crates jumped a whole cell ahead of Blupi (gameplay) — FIXED_AND_VERIFIED (D60)

- Owner report (2026-11-01): a crate can be pushed but visually jumps away.
- Galaxy before: `InteractionSystem::Update` (crate branch) moved a crate's group a whole cell
  (`MoveCrateGroup`, `currentX += dir`) the frame Blupi, walking at it, came within 1.1 of its
  centre; he walked on at his march speed (2.5 cells/s) until within 1.1 again. The crate leapt
  64 px in one frame and his hands were 1.1-2.1 cells from its centre.
- Reference: `Decor.cpp:6130-6157` (Push entered on contact; the crate put at `pos.X + 55` /
  `- 59`), `:6200-6224` (every Push tick: the same), `:3386-3390/3444-3448` (Blupi moves
  `CaisseGetMove(5)` px a tick in Push), `:9487-9507` (`CaisseGetMove`: `max -= (links-1)/2`, min 1,
  Power x2, ramp `max * phase / 20` over 20 ticks, min 1), `:9321-9346` (`TestPushCaisse`: group
  atomic; on failure Blupi goes back).
- Reproduced: source reading plus the old `VerifyInteractionSystem` ("the crate outpaces Blupi's
  own per-frame step once pushed") and the playthrough traces (`crate (48 1 66) -> (48 1 65)` in
  one tick).
- Fix: see `../GALAXY_EGGBERT_FIDELITY_AUDIT.md` §4 and M3D-0001.
- Validation: `VerifyInteractionSystem` (per-20 Hz-tick positions equal to the original's for
  2.5 s; no tick moves more than 5 px; three crates 4 px; Power 10 px; wall; gap; stacking; release
  mid-cell; walking past a crate's side does not push it); `VerifyBlupiMovement` (a crate between
  two cells is a cube where it is; a jump beside it rises; FollowCrate; CanStandAt); 110/110. Live
  game trace (mission 012's first crate, every frame): the crate on the original's positions, Blupi
  0.890-0.896 behind it until he jumps, no frame over 5 px a 20 Hz tick
  (`plans/evidence/crate-push-012.png`, `.csv`); a live look from the side: his nose on the crate's
  face. The 18 crate playthroughs pass (see `test_coverage.csv`).
- Found on the way and fixed before the commit (the fix's own first cut): a jump while pushing had
  no forward speed (the crate held him); the point collision counted a moving crate in its nearest
  cell (a ceiling over his jump); Push/Pop stayed on while the crate finished its cell alone (no
  air speed); a crate grazed at its corner (0.6 off its line) was pushed.
- Scripts changed for the new timing (assumptions of the old jump, each checked against the
  failing trace): every `crate X Y Z max N` wait raised to 60 ticks (the crate finishes its cell
  at push speed); 012's pull (two cells take 162 ticks); 022's last push (let go past x 67).

## BD-0002 — pulling a crate: 4 px a tick, no ramp, crate jumping at a 1.9 gap (gameplay) — FIXED_AND_VERIFIED (D60)

- Galaxy before: `BlupiController::kPullSpeed = 4 px/tick`, Blupi backed away freely and the crate
  jumped a cell once he was 1.9 away.
- Reference: `Decor.cpp:3408-3412/3466-3470` (`CaisseGetMove(3)` in Pop), `:6225-6248` (the crate
  held at 55/59 px every tick; Pop ends when it cannot move).
- Validation: `VerifyInteractionSystem` (no tick over 3 px; a cell from rest in 98 frames = the
  original's 32.7 ticks; contact distance kept).

## BD-0003 — a crate could be pushed out over water or empty-mask decor (gameplay) — FIXED_AND_VERIFIED

- `CrateGroupCanMove` (was `MoveCrateGroup`) counted any non-air block under the seed row as
  support (water, decor with an empty mask). `TestPushOneCaisse` (`Decor.cpp:9354-9385`) needs
  `DecorDetect` to hit a solid quarter-cell under both lower corners.
- Reproduced 2026-11-01: `VerifyInteractionSystem` pushes a crate toward water two cells away; it
  went out over the water. Fixed: support is a block with a non-zero collision mask (or a crate);
  the crate now stops at the bank. 110/110; the 18 crate playthroughs pass with the same tick
  counts (none relied on it).

## BD-0004 — follow stars and shot ranges stopped at any non-air cell (gameplay) — FIXED_AND_VERIFIED

- `InteractionSystem::IsSolidAt` claimed to match `BlupiController::IsSolidAt` but counted every
  non-air cell: a follow star (97) homing on Blupi exploded on a cobweb, a sign or water.
  The original moves it with `TestPath` -> `DecorDetect`: blocked only by a solid quarter-cell of a
  collision mask, or a crate. Reproduced by a test (a follow star crossing a cobweb, 403; water).
- The shots' range (`SearchAirDistance`, the original's `SearchDistRight`, Decor.cpp:7628) stopped
  at any non-air cell; the original stops only at `IsBlocIcon` cells (Decor.cpp:7522): all sixteen
  quarter-cells solid, or the vanishing saucer 324 while there. Now the same (71 block icons have
  a part mask: shots pass them as in the original). The saucer's two passable ticks in twenty
  are not modelled for shots (it counts as there).
- Fix: both as the original; `VerifyInteractionSystem` (cobweb and water passed, a block stops
  it); 110/110; all 78 playthroughs (every mission, every world chain, 199) pass with it.

## BD-0005 — the asset inventory undercounted the original 3D sources (doc) — FIXED_AND_VERIFIED

- `GALAXY_EGGBERT_COMPLETION.md` §4/§7.1 said 44 `.max` and 6 `.a3d`, "not readable without
  proprietary tools". The tree holds 56 `.max` (14 stamped "3D Studio MAX: 3.1") and 105 `.a3d`,
  whose bytes start `AMAP`/`BLOKSHAP`/`blokgeom` (Amapi 3D chunks with big-endian floats: a
  reader looks feasible). Evidence: `tools/audit/asset_sources.py` (this session).

## BD-0006 — lifts faster than any lift of the original; 061's shuttle ladder has a ~0.1 s jump window (gameplay / world data) — FIXED_AND_VERIFIED (all 31 lifts; a human play pass still to do)

- Owner report (2026-11-01): a mission expects jumps between moving platforms whose speed makes the
  jump impossible or unreasonably precise.
- Envelope (`tools/audit/lift_envelope.py`, this session): the original's 424 moving lifts (52
  worlds) run at most **4.01 cells/s** (`world061`, `world113`), dwell at least 0.10 s. Galaxy's
  shipped worlds hold **31 lifts faster than that**: world061 (six shuttles at **8.33 cells/s**,
  dwell 0.15 s; others 4.4-6.1), world113 (twelve at 5-6), world024 (5.33, 4.44), world062 (5.0),
  world073 (4.44). Cause: the world builders lengthened the original's paths (061's shuttles: 4
  cells in the original, `posStart=3136;1472 posEnd=2880;1472`, 24 ticks) and kept its tick counts
  (10 cells in 24 ticks), so the speed rose with the length.
- Reproduced with the engine (`tools/autoplay_search.py` on the ladder's first jump, from its
  shuttle's arrival at z 45, delays 30-82 every 2 frames): only 64, 66 and 68 pass — a window of
  about 5-7 frames (~0.1 s) once per 2.7 s cycle. Not a human-playable jump.
- Momentum checked: the original's Blupi does not keep a lift's motion when he jumps off it
  (`Decor::MoveObjectStep`, Decor.cpp:7946-7948 clears `m_blupiVector`/`m_blupiTransport` every
  frame, :8159-8171 sets them only while he stands on the lift); Galaxy's does not either. A jump
  from one moving lift onto another therefore always asks for timing, as in the original; only
  the speeds make it inhuman.
- Engine timing checked: lifts step as `Decor::MoveObjectStepLine` (linear posStart -> posEnd over
  stepAdvance 20 Hz ticks; `AdvancePatrolStep`, `kTicksPerSecond` 20, D43). The engine is not the
  cause; the world data is.
- Fix, world061 (2026-11-01): every lift at its original counterpart's speed (`tools/worlds/
  world061.py`: shuttles 60 ticks, the chasm pair 25, the X-and-Z lift 60, the riser 43, the tower
  48). At the original's speed alone the ladder's window was still ~0.1-0.3 s (delays 174-180 of a
  378-frame cycle): the shuttles move in step and Blupi rightly loses a lift's motion in a jump, so
  the target must be dwelling when he lands. With a 30-tick dwell at each end (the precedent: this
  world's chasm pair, D43) boarding passes at every delay 2-40 and each ladder jump at 4-56 frames
  into the dwell (~0.9 s): a timed jump a person can make. The mission's playthrough re-timed
  (jump 30 frames into a dwell) and passes (16 933 ticks). Still to do: a human play pass; the
  other worlds over the envelope (113: twelve lifts at 5-6 cells/s; 024; 062; 073), each measured
  and re-timed the same way.
- Fix, the other worlds (2026-11-01): each at its original counterpart's speed — 113's gallery
  lifts 15/18 ticks for 3 cells (4.0/3.33 cells/s, were 6.0/5.0) and its shaft lift 60 ticks (3.33,
  was 5.0); 024's quick lift out in 20 ticks (4.0, back still 66; was 5.33) and its upper lift 24
  (3.33, was 4.44); 062's long lift 60 (3.33, was 5.0); 073's L5 24 (3.33, was 4.44). No dwell
  changed (none of them is a jump between lifts in step). `lift_envelope.py`: 0 lifts over the
  original's fastest. Playthroughs: 113 and the world-110 chain pass unchanged; 024, 062, 073 and
  the chains 020, 060, 070: see the commit.

## BD-0007 — lava drawn as a cube with its tile cut out on every face (visual) — FIXED_AND_VERIFIED (VIS-0069)

- Owner report (2026-11-01): lava as billboard cards / disconnected vertical strips. Galaxy drew
  every lava cell (animated tiles 68-72) as a cube with the current tile on all six faces, cut out
  where the sprite is clear: a grid of fire strips with holes (`plans/evidence/lava-before-after.png`,
  left).
- Reference: the tiles are a pool rendered from Amapi slabs `Decor/lave/lave-1..5.a3d` (their tops an
  11 x 6 height field); tile 68+k is `lave-(k+1)` seen from the far side (silhouette correlation
  0.82/0.91/0.66/0.53/0.48 mirrored, against much lower unmirrored).
- Fix: `LavaSurface.hpp` draws each lava cell as that pool — a body to the rim (0.25 of the cell)
  and a lit height-field top from the original frames (`LavaSurfaceData.hpp`, generated by
  `tools/a3d/lava_surface.py`, heights fitted to the tiles' silhouettes within 0.026 of a cell),
  coloured from the tile's own body pixels, cycled with the tile animation; walls only where no
  lava is beside, the rim flat so cells join. Collision and the hazard are unchanged.
- Validation: `VerifyTileFaces` (winding as the engine's faces, heights, flat rim, walls); 110/110;
  live in worlds 062 and 083 (before/after). Left: the owner's look, a low-angle close view.

## BD-0008 — the fan drawn as a cube with its icon on every face (visual) — FIXED_AND_VERIFIED (VIS-0127)

- Owner report (2026-11-01): a fan sprite glued on the sides of a cube. Galaxy drew fan cells (126-137)
  as DirectionalCubes: the fan picture cut out on the faces, flat strips from most angles
  (`plans/evidence/fan-before-after.png`, left).
- Source: all four directions' tiles were rendered from one Ray Dream scene, `Decor/ventillo-h.rd4`
  (a fan on a stem standing on a wall plate with two lights), animated in three frames; converted
  with the object pipeline it matches the up tiles 132-134 at IoU 0.993 (`model_parity.md`).
- Fix: the model `avatars3d/objects/fan_ventillo-h`; fan cells are left out of the terrain mesh and
  drawn as that model turned about the view axis to blow the cell's way (`BlockModels.hpp`: left
  -X, right +X, up, down; `VerifyTileFaces` checks the directions), its rotor on the tile
  animation's frame. Without the model (no avatars3d) the cubes stay. Gameplay unchanged.
- Validation: 110/110; live in worlds 999 and 051 (before/after). Left: the owner's look; the
  wind column tiles (110-125, `Decor/vent-norm/`) are the next candidates.

## BD-0009 — pillars drawn as sprites on boxes (visual) — FIXED_AND_VERIFIED for 76, 77, 377, 399, 376; 375 has no source

- 76 (a stone column) and 77 (the golden branched support) were sprite tiles on an inner box. Their
  original scenes: `Decor/support1.rd4` (IoU 0.92) and `Decor/support2.rd4` (0.995). Converted and
  drawn as still block models (`BlockModels.hpp` `kStillModelIcons`; the terrain leaves those cells
  out and lists them, the host draws the model at the cell); their decor role is unchanged.
- Also 377 (the barrel stack, `Decor/pile1.rd4`, 0.93) and 399 (the white column,
  `Decor/palais/big0108.rds`, 0.99), sources found by the earlier tile survey
  (`build-probe/rd4-survey/object-m_icons.csv`); live in world999.
- 2026-11-04: 376 (the twisted column) is `Decor/enfant22.rd4`'s relief (Q-012, by its pixels;
  the toy sticks 235/236 share the scene). 375 (the turned column) still has no source:
- Earlier: 375/376 (turned and twisted wooden columns): no source found. `Decor/colonne1.rds` (the
  survey's best match, 0.90) holds one plain `Cube` primitive with a procedural wood shader and no
  deformer (its tree read 2026-11-01): a narrow rectangle scoring against a narrow column, not the
  turned column's scene. (An earlier note here guessed a deformer; the scene shows none.)

## BD-0010 — enemy contact right after a vehicle, a teleport or a balloon pop (gameplay) — FIXED_AND_VERIFIED

- Found by a read-only review of the enemy types (2026-11-01), verified here against both sources.
- Reference: `m_blupiPosHelico` is set where Blupi leaves a vehicle (Decor.cpp:3910, 4031, 4303,
  4446, 4507), where a balloon is popped (5780) and where a teleport puts him (6360); while it is
  set the hazards 2/3/4/16/17/20/96/97 neither kill him nor are destroyed, and none pops a balloon
  (5767, 5786); it clears once he is more than 30 px away (6341-6345). Galaxy had nothing alike:
  stepping off a vehicle or out of a teleporter onto a hazard killed him, and a spider that popped
  his balloon killed him the next frame.
- Fix: `InteractionSystem::StartContactGrace` (a balloon pop starts it inside; the host on leaving a
  vehicle and on a teleport's arrival); it lasts until he is out of the hazards' contact reach
  (Galaxy's contact sphere, 0.7 of a cell, is wider than the original's 28 px box, so 30 px would
  end it while he still touches). `VerifyInteractionSystem`.
- Also: the hazard's explosion (channel 10) now plays when it kills him too (Decor.cpp:5822).

## BD-0011 — the wasp's sting kept Power and the Cloud (gameplay) — FIXED_AND_VERIFIED

- Reference Decor.cpp:5845-5847 clears Shield, Power, Cloud and Hide on the sting; Galaxy's
  `TriggerBalloon` cleared only the vehicle. `VerifyBlupiMovement`.

## BD-0012 — the Cloud's arc reached too short (gameplay) — FIXED_AND_VERIFIED

- Reference `BlupiElectro` (Decor.cpp:9614-9634): centres within 100 px across and 87 px up or down;
  Galaxy used a sphere of 72 px. Now the original's box, its reach across on both X and Z.

## BD-0013 — big creatures rested what the world file said, not the original's 152 ticks (gameplay) — FIXED_AND_VERIFIED

- Reference `Decor::Read` (11366-11370) forces 152 ticks at each end for every type 54 (its rest
  animation's length), whatever the file stores. Galaxy kept the stored values (200 in 061, 062,
  065, 066; 30 in 199; 40 in 999). Now forced at load (`WorldRuntime`). `VerifyInteractionSystem`.

## BD-0014 — crates moved whole cells; a pull ran on after the keys and needed a free cell (gameplay) — FIXED_AND_VERIFIED

- Owner reports OR-0021 (a pull sometimes does not start), OR-0022 (released, a pulled crate keeps
  moving for about a second), OR-0023 (Mobile Eggbert lets a crate be pulled onto another so that
  it overhangs; Galaxy did not). All three came from M3D-0001's first reading (D60): a move once
  started finished its cell; support was checked for the destination cell; a pull needed room for
  Blupi a whole cell behind him.
- Reference: `Decor.cpp:6130-6157/6200-6248` (Push/Pop each tick: the crate put at Blupi's hands;
  a Push that cannot go takes Blupi back, a Pop that cannot go becomes Stop), `:9321-9346`
  (`TestPushCaisse`: the linked group all or nothing), `:9354-9385` (`TestPushOneCaisse`: the box
  against the decor, then for the seed row a 20 px patch under each lower corner must hit solid
  ground or a crate), `:9456-9479` (`CaisseInFront`), `:9487-9507` (`CaisseGetMove`).
- Fix: crates are boxes at any X/Z (`InteractionSystem::UpdateCratePush`, `CrateGroupCanMove`,
  `CrateSupported` with four corner patches in 3D, `CrateBoxHoldsBlupi`); each frame the group
  moves by the original's speed if it all fits, else nothing; release stops it the same frame; a
  blocked pull restarts the ramp (Stop). Autoplay: `crate X Y Z [tol T]`, crate rest lines
  wherever a crate stops. M3D-0001 rewritten.
- Validation: `VerifyInteractionSystem` (pushed along Z and resting between cells; stays when let
  go of; stops at a wall at most one step short; over a gap, off a step onto a crate and over water
  by at most the 20 px support; pulled onto another crate until only its corner holds (OR-0023);
  a pull stops the frame he stops backing away and stays (OR-0022); pulled again from there and
  with a wall less than a cell behind him (OR-0021); timing per 20 Hz tick unchanged); 110/110.
  The 24 playthroughs with crates: see `test_coverage.csv` (push targets recomputed: Blupi stops
  57 px behind the crate's goal, `crate ... tol 0.06`).

## BD-0015 — a spring put the vehicle down instead of wrecking it (gameplay) — FIXED_AND_VERIFIED

- Found checking OR-0003's dismount code. Reference `Decor.cpp:2835-2893`: on a spring
  (`IsRessort`), the helicopter, overcraft, jeep, tank and skateboard are wrecked (ObjectType9 at
  his position, small shake, sound 10) unless the shield, Hide or SuperBlupi protect him. Galaxy
  deposited the vehicle's pickup intact. Now `WreckVehicle()`, shared with water and the
  helicopter's ceiling (OR-0019).

## BD-0016 — creatures moved at the builders' own speeds, many far faster than the original (gameplay) — FIXED_AND_VERIFIED

- Owner report OR-0005 (wasps, fish, birds too fast). Reference: every MoveObject record of a
  creature type in `../mobile-eggbert/worlds` (about 1,000) takes the same 20 Hz ticks a cell
  along its path's longer axis, there and back, and rests the same ticks: bulldozer (4) 30/20,
  moving mine (16) 10/0, fish (17) 15/47, bird (20) 8/8, robot (32) 8/25, creature (33) 10/23,
  wasp (44) 6/4, big creature (54) 20/200 (152 at load, BD-0013) -- the original editor's
  per-type values. `Decor::MoveObjectStepLine` runs them unchanged.
- Galaxy's world builders set ticks freely: 359 creatures differed, mostly faster (moving mines
  up to 14 cells/s, birds 2-7x); some slower (a one-cell mine given a three-cell mine's 30 ticks).
- Fix: `tools/worldkit.py` `CREATURE_TIMING` gives every patrolling creature its type's timing
  whatever a builder passes; the same table in `ObjectDefinitionRegistry`
  (`patrolTicksPerCell`, `patrolRestTicks`) times a patrol set in the 3D editor; all worlds
  rebuilt (46 changed, only creature timing: checked field by field). Where the original speed
  closed a gap a Galaxy design relied on, the creature got the original's path length for its
  ticks instead (051's deck mine and 046's two stair mines: three cells in 30 ticks, as the
  original's 30-tick mines). `tools/audit/patrol_envelope.py` (exact per-type check): 0 off
  (world999, the C++-generated exhibition used by the verifiers, keeps its own).
- Validation: the playthroughs re-timed where they synced on creatures (032, 033, 046, 051, 057,
  058, 081, 084, 092, 093, 094, 114: waits searched with `tools/autoplay_search.py`, now with
  `--jobs`, the middle of each passing window kept); all 78 pass.

## BD-0017 — a teleporter put Blupi one cell beside its partner, turned about (gameplay) — FIXED_AND_VERIFIED

- Owner report OR-0004 (teleporters, no detail given). Reference: `Decor::SearchTeleporte`
  (Decor.cpp:7406-7429) lands him in the partner tile's own column, one row down: under it, as
  he stood under the one he entered. He does not go straight back because the arrival sets
  `m_blupiPosHelico` (6360) and a teleporter takes him only while that is clear (5594); it
  clears once he is 30 px from it (6341-6345). His direction is kept.
- Galaxy put him one cell +Z of the partner and turned him to face +Z: it had no re-entry
  guard, so landing under the partner sent him back at once (WorldRuntime's comment guessed
  that the original's levels avoided it by design).
- Fix: `FindTeleportDestination` lands under the partner; the arrival starts the contact grace
  (Galaxy's `m_blupiPosHelico`, `InteractionSystem::StartContactGrace`, already used for the
  hazards, BD-0010), and the trigger waits while it is active, as the original's does, so
  also just off a vehicle or out of a popped balloon. The yaw is kept.
- Worlds: 106's three well-top pillars had been put one cell short of the wells for the old
  landing; they now stand over the wells' mouths (`tools/worlds/world106.py`), so he drops through
  the treasures as the brief and the original have it. The other teleporter worlds needed no
  change (the cell under each pillar is floor); builders' comments corrected.
- Validation: `VerifyInteractionSystem` (destination under the exit pillar); the playthroughs
  through teleporters (034, 051, 065, 066, 094, 095, 102, 106, 124) pass; 066's arrival check
  now expects the partner's cell, 102's wait on top of the lift re-searched (1-5 pass, 3 kept).
- The arrival seen in the original running (the Linux build of the port, world 34's map in a
  probe copy, `plans/evidence/teleport-arrival.png`): he appears under the partner amid two
  sparkle clusters, settles into his idle stance and stays visible -- the looping Teleporte
  record does not play on. Galaxy now adds those clusters (`SpawnTeleportArrival`: ObjectType27
  sliding +X and -X to the nearest wall, 20 ticks a cell, Decor.cpp:6355-6356) and no longer
  plays channel 71 a second time on arriving (the original plays it only on entering, 5603).

## BD-0018 — six of the original's Blupi sounds were never played (audio) — FIXED_AND_VERIFIED

- Found by matching every `PlaySound(SoundChannelN, ...)` in Decor.cpp (63 channels) against
  Galaxy's play sites: 5, 34, 35, 36, 37 and 39 had none.
- Now played, as events of `BlupiController` (the host maps them to channels): 34 on catching a
  hanging bar (Decor.cpp:5452), 35 on jumping off it (4789), 36 hanging still at ticks 118, 230
  and 278 of StopSuspend's 328 (2379-2382), 37 standing still at Stop's taps, ticks 125-139 and
  215-269 of its 330 (2371-2378), 39 on each grip of a crate (StopPop) and start of a pull (Pop)
  (3291-3323). Channel 5, the turn about (3364-3371, 3422-3429), plays as the Turn clip starts:
  in 3D he turns in place; there is no turn about in the air (INTENTIONAL_3D_ADAPTATION).
- Validation: `VerifyBlupiMovement` (each event at its tick: the taps 4 ticks apart from 125,
  the hanging fidget at 118, the grip once per entry).
- Hanging Blupi played the Air clip: Galaxy had no hanging states. Now `AnimState::StopSuspend`
  and `MarchSuspend` with the original's tables (table_blupi 31 and 32, Tables.cpp), their clips
  built from Ray Dream's own `suspendu*.rd4` scenes (every icon matched confidently; added to
  `avatars3d/blupi/` alone: a whole rebuild today also changes Hide and StopTank, converter drift
  since their export, left as shipped); the hanging fidget sound follows StopSuspend's phase.
  `plans/evidence/hanging.png`.
- The jump off a bar: the original makes him hold Jump (or Up) 10 ticks first, JumpSuspend's
  wind-up (Decor.cpp:4769-4789; let go earlier and he hangs on); Galaxy let go at once. Now the
  same, with the JumpSuspend clip (`suspendu-jump.rd4`, table_blupi 34). The playthroughs leave
  bars at their ends, so the eight bar worlds replay unchanged.

## BD-0019 — the action button with nothing to do did nothing; the original shakes his head (gameplay) — FIXED_AND_VERIFIED

- Found listing the original's Blupi actions with no Galaxy state: Non (84). Decor.cpp:6366-6372:
  the action button pressed while he is still in Stop -- nothing before it in the tick used the
  press (a switch, dynamite, the decoy, a vehicle, a dynamite stick taken) -- makes him shake his
  head (Non, 18 ticks, 3188) with channel 27.
- Galaxy: `AnimState::Non` with the original's table (table_blupi 84) and its clip from Ray Dream's
  `blupi-ouf2.rd4` (icons 226-229 matched confidently; exported alone). The host marks every use
  of the press (`actionUsed`; `InteractionSystem::ActionPressSpent()` for the dynamite stick) and,
  if none, standing in Stop, starts the reaction (cut short by moving) and plays channel 27.
- Validation: `VerifyBlupiMovement` (18 ticks, then Stop); live `plans/evidence/head-shake.png`;
  all playthroughs.

## BD-0020 — with a ledge behind him Blupi stood still; the original steps forward off it (gameplay) — FIXED_AND_VERIFIED

- Found listing the original's actions with no Galaxy state: Advance (10). Decor.cpp:4935-4943:
  standing still with one foot over the void and not facing it (the teeter, Vertigo, is for the
  void in front), he steps forward, 4 px a tick (3657-3664) for 5 ticks (5064), with the teeter's
  channel 6. Galaxy had only the front case (Vertigo, then Recede).
- Now the same on foot (`m_advanceTimer`, `AnimState::Advance` with table_blupi 10 -- the
  Recede icons, so its clip equals Recede's). 20 px is 0.31 of a cell.
- The vehicles, which Galaxy's ledge rules left out: the landed helicopter and overcraft take the
  same probes but recede at once without teetering (4908-4912) and advance without the sound; a
  parked jeep, tank or skateboard with one wheel over the void (2-18 px in from a side, 12-19
  for the skateboard) rolls on 5 px a tick for 10 ticks the way it faces (4944-5030,
  3667-3680) -- off the ledge if it faces it. Now the same (`m_rollTimer`; not on a lift, where
  the original measures the lift's own edges).
- Validation: `VerifyBlupiMovement` (state, sound, 0.31 cell forward then Stop; the jeep rolling
  on and off a ledge; the helicopter receding without a teeter); all playthroughs, the 33 with
  vehicles again after the vehicle rules.

## BD-0021 — hazards and pickups reached Blupi from a 0.7-cell sphere; the original's boxes are smaller (gameplay) — FIXED_AND_VERIFIED

- Open since the BD-0010 review ("contact reach: 0.7 sphere vs 28 px box"). Decor::MoveObjectDetect
  intersects two boxes: Blupi 28 px across (pos+16..+44), from 11 to 58 px below his cell's top;
  an object 28 px across and, by type, the bottom 24 px of its cell (most, pickups included),
  the top 24 (the hanging mine, 3; not while he crouches), the middle 28 (fish 17, bird 20, wasp
  44, big creature 54), or a bullet (23) 16 px across from 10 to 28. So a still mine kills with
  its middle within 28 px (0.44 of a cell) of his; Galaxy's sphere reached 0.7 (45 px), about
  60 % farther, up and down alike. The still mine's near-miss box (Ouf4) is Blupi's grown 20 px
  each side, 40 above and 30 below; the contact grace ends at 30 px (6341-6345).
- Now `MoveObjectTouches` in InteractionSystem, across in X and Z alike, for the generic hazards,
  their near miss, bullets, the wasp's balloon, the big creature and lethal decor; the grace ends
  at 30 px. A moving object meeting an enemy Blupi keeps the sphere (not Blupi's contact).
- The teleporters' re-entry guard was the grace (BD-0017). The original's 30 px suffices there
  only because its trigger is a 7 px band (IsTeleporte, `pos.X % 64 <= 6`); Galaxy's is the
  whole cell under the pillar, so with 30 px he went straight back (034's chi). Now a teleporter
  also waits until he has left the cell he arrived in (`teleportArrivalGuard_`).
- Pickups too, in a second step: with the boxes 31 of the first 46 playthroughs missed an item,
  their `go` targets chosen for the 0.7 reach. A temporary probe build logged every item the
  sphere would have taken and the box did not, with the playthrough's tick; each was mapped to
  the script line running then and fixed: a detour onto the item before that line (so the line
  still ends where it did), a lick or drink point moved to 0.15 from its item, an exit's
  `until exit` target moved onto it, swims and balloon dips held long enough to reach an item on
  a floor (the box reaches 22 px below his feet), and three timings re-searched where the longer
  ways changed them (032's gallery a shared cycle later, 052's stars, 053's star columns passed
  beyond their wake reach). Two wasp waits moved into its box (054, 055).
- Validation: `VerifyInteractionSystem` (a mine at 0.5 cell misses, at 0.4 touches, a cell below
  misses; the grace's 30 px); all playthroughs.

## BD-0022 — a pickup taken with the camera right against it never paid its reward (gameplay) — FIXED_AND_VERIFIED

- Found with BD-0021 (043's niche treasure): a pickup's reward comes when its icon's flight to
  the panel ends (`VoyageInit`/`VoyageStep`); Galaxy starts that flight from the item projected
  onto the screen. With the camera pressed against the item the point lay almost in the camera's
  plane: a tiny W, coordinates thousands of pixels out, a flight that never ended -- the treasure
  gone and never counted. The original's items are always on its screen.
- `Hud::ProjectWorldToHudSpace` now clamps onto the 640 x 480 reference screen: an off-screen
  item's flight starts from the screen's edge.
- Validation: `VerifyInteractionSystem` (a point almost level with the camera lands on the edge);
  043 passes; all playthroughs.

## BD-0023 — keys were counted, so a second key of a colour was taken and then lost (gameplay) — FIXED_AND_VERIFIED

- Wave C, the keys (ObjectType 49/50/51). The original holds a key or not (m_blupiCle flags):
  a key of a colour he holds, or one still flying to the panel, is not taken (Decor.cpp:5962,
  5977, 5992); a door clears his key (5619), after which the next is taken. Galaxy counted keys
  (`++keys1_`) and took every one, while its doors clear the colour to 0 -- so a second key of a
  colour picked up early was lost at the first door, and a world with two doors of a colour
  could not be finished that way.
- Now the original's gate and `keysN_ = 1` on arrival. (A first cut let the treasure, which
  shared the keys' switch case, fall into the key gate: caught by 023, and the verifier now
  checks a treasure taken with all keys held.)
- Also reviewed against the source and matching: the egg (6: MAX_EGG_COUNT 10, nothing at the
  cap, icon 21 to the lives row), the treasure (5: icon 6 to (430, 430), the four sparkles).
- Validation: `VerifyInteractionSystem` (a second key of a held colour stays; a treasure is taken
  with all keys held); the worlds with repeated key colours (023, 199) and all playthroughs.

## BD-0024 — vehicles were boarded from 1.1 cells; a floating helicopter needed the button (gameplay) — FIXED_AND_VERIFIED

- Wave C, the vehicle pickups (helicopter 13, jeep 19, skateboard 24, tank 28, overcraft 46). The
  original boards one inside MoveObjectDetect's `bNear` contact (Decor.cpp:5646-5760): the
  boxes of BD-0021, with the action button -- except a helicopter floating over nothing (the cell
  under it passable, Decor::IsFloatingObject), boarded by touching it: he jumps or drops in.
  Galaxy boarded any vehicle from a 1.1-cell sphere on the button, and a floating helicopter only
  with it.
- Now `InteractionSystem::BlupiTouches` (the boxes) for boarding, and the floating helicopter
  boarded on contact. The playthroughs' 36 boardings made from outside the box re-aimed to 0.15
  from their vehicle (two jump landings in 074 and a helicopter left in 105 by hand).
- World 065's garage had its four jeeps on grates, which are passable: within a jeep's box he
  stands in its cell and fell through. The cells under the jeeps are the garage's floor now (set
  last in the builder, so no other cell's random face changes); the original's garage had them
  on lifts.
- Validation: all playthroughs.

## BD-0025 — an invert sign touched while inverted was left in place (gameplay) — FIXED_AND_VERIFIED

- Wave C, the invert sign (ObjectType 40). The original takes it whenever Blupi is not hidden
  (`!m_blupiHide && m_blupiFocus`, Decor.cpp:6040): touched while already inverted it is taken
  again and the inversion starts over (`m_blupiTimeShield = 100`, sound 66, the four sparkles).
  Galaxy refused it while inverted (`TriggerInvert` and the host's `canGrantInvert`), so the
  sign stayed and the old inversion ran out.
- Now only Hide refuses it. The original's invert shares `m_blupiTimeShield` with the powers,
  the balloon and the squash (Decor.cpp:5071-5205): each state that starts sets it to 100 (an
  invert taken under a power restarts the power's time too), each active one counts it down at its
  own pace, all of them end when it reaches 0, and a popped balloon zeroes it (5771), ending an
  invert running with it. Galaxy kept separate timers until 2026-11-04; now one count
  (`BlupiController::GetTimeShield`, the four states' accumulators; `VerifyBlupiMovement`: the
  restart, the two paces ending together, the pop).
- Validation: `VerifyBlupiMovement` (taken again 60 frames into an inversion); the ten worlds
  with invert signs (034, 044, 045, 055, 095, 111, 114, 121, 122, 199) pass.

## BD-0026 — an exit checked only once per contact; the original checks every 51 ticks (gameplay) — FIXED_AND_VERIFIED

- Wave C, the exits (ObjectType 7 and the secret exit 21). The original keeps `m_goalPhase`
  (Decor.cpp:6158-6189): touching an exit with the phase at 0 checks it -- every treasure held,
  the fanfare (14) and the win; else the refusal (13) -- and sets the phase to 50, counted down
  each tick he stays on it; stepping off sets it to 0. So a refusal repeats every 51 ticks, and
  when the last treasure's reward lands (it flies to the HUD first) while he stands on the exit,
  the next check wins. Galaxy checked once per contact: there he had to step off and on again.
- Now the original's counter (`exitGoalTicks_`); no check after the win. The `m_blupiFocus`
  gate stays unmodelled, as everywhere in InteractionSystem.
- Validation: `VerifyInteractionSystem` (refused, the reward lands, the win 51 ticks after the
  refusal without stepping off); all playthroughs.

## BD-0027 — dynamite and the decoy could be put down on a lift (gameplay) — FIXED_AND_VERIFIED

- Wave C, the dynamite (ObjectType 55/56) and the decoy (Perso, 200). The original puts either
  down only with `m_blupiTransport == -1` (Decor.cpp:4792-4794): not while a lift carries him.
  Galaxy asked for no vehicle, balloon or squash, and ground under him, which a lift is; the
  stick was left lit in the air where the lift had been.
- Now the host's `blupiCanUseHands` also asks `!IsRidingLift()`.
- Also reviewed against the source and matching: taking a stick (the action press, none
  carried, none flying to the panel, Decor.cpp:6116), its fuse (ObjectType 56, the blasts at
  ticks 50 and 53); the lollipop (26: the action press, no shield, vehicle, balloon or squash;
  Cloud and Hide end; Power with sound 44 at the lick's tick 32, the lollipop back in place,
  Decor.cpp:6025-6039, 3212-3223) and the drink (30, its Hide at tick 36, back in place).
- Validation: the playthroughs with dynamite or the decoy (014, 025, 065, 091, 092, 095, 101).

## BD-0028 — a decoy met a small enemy from a 0.7-cell sphere; the original's boxes are smaller (gameplay) — FIXED_AND_VERIFIED

- Wave C, the bulldozer and the shooters (4, 32, 33) against the decoy and the lethal
  decorations (200-203). The original destroys both when MovePersoDetect's boxes meet
  (Decor.cpp:7957-7975, 9835-9865): the enemy's Blupi's box (28 px across, 11 to 58 px under its
  cell's top), the decoy's an object's (28 px, its bottom 24). Galaxy used the 0.7-cell sphere
  BD-0021 left only here, which met a decoy up to 45 px off.
- Now `MoveObjectTouches` with the enemy in Blupi's place; the sphere is gone.
- Also reviewed and matching: the Cloud's aura destroying them (BlupiElectro, BD-0012) and the
  bulldozer's patrol (BD-0016). Open: the original draws 4, 32, 33 (and 54) 2 px right and 6 px
  (BLUPIOFFY) lower than their position (Decor.cpp:867-875); Galaxy places every object at its
  cell (ResolveObjectVisualCenterY). Seen live (probe world 992, 2026-11-02): the bulldozer's
  model rests its tracks on the floor (its model carries the offset); the shooters and the big
  creature still want a close look. Closed 2026-11-03 by measure: the element sprites are 60 px in
  a 64 px cell, and BLUPIOFFY (6) brings their bottom from 4 px over the cell's floor to 2 px into
  it, Blupi's own convention; the creature's sprites reach their box's bottom row (0 px margin),
  the bulldozer's 1 px; Galaxy stands each model's sprite box on the floor, so what is left of
  the offset is 2 px down and the 2 px to the right, 0.03 of a cell each: not worth a shift.
- Validation: `VerifyInteractionSystem` (32 px beside: not met; 26 px: met). No playthrough has
  a decoy and a small enemy in one world.

## BD-0029 — an updraught held a falling Blupi in the air; the original only slows his fall (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:2965-2968, 2785-2789): in the air `m_blupiVitesseY` grows by 2 a tick while
  under 20 and moves him `2 * m_blupiVitesseY` px: his fastest fall is 40 px a tick (12.5 blocks/s).
  The updraught (wind 118) takes 20 px a tick off, so a falling Blupi sinks at 20 px a tick, and one
  standing in it is lifted until his fall outweighs it: he bobs about a block over the floor. The
  downdraught (122) adds 20.
- Galaxy: `kFallLimit` was 10 blocks/s (the cap read as 20 px a tick) and `kWindVerticalSpeed` the
  same, so in an updraught he hung in the air for ever: a shaft with a fan at its foot (the
  original's world 102 descends one, Wave E) could not be gone down.
- Found: Wave E, world 102's grey works (video 4:02:10-4:02:16: he sinks down the updraught shaft in
  about two seconds and the fan at its foot kills him and is wrecked).
- Fix: `kFallLimit` -12.5, `kWindVerticalSpeed` 6.25 (20 px a tick). No shipped world had an up- or
  downdraught (only the showcase 999).
- Validation: `VerifyBlupiMovement` (lifted and comes down again; sinks at 20 px a tick; the cap);
  ctest 111/111; 76 of 78 playthroughs passed unchanged, 011 (and 010, which plays it) missed the
  water channel's bank by a hair (a jump's last moment falls a little faster): its take-off moved
  0.15 closer to the channel (`jump 3.25`), both pass.

## BD-0030 — a jump under wind, a bar or lightning was shortened as under a ceiling (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp IsNormalJump -> IsPassIcon): the two cells over his head shorten the jump
  only when one of them has a quarter of its collision mask set (and a vanishing saucer 324 not
  while it is gone); wind (110-125), bars (138, 202), lava, lightning, drips, saws, switches, water
  and every other empty-mask tile leave it whole.
- Galaxy: `HasJumpHeadroom` used `IsSolidAt` (anything but plain decor), so a jump under any of
  those tiles was the reduced one (12/16): found in world 107, where a band of wind over the
  lollipop's ledge (Wave E) turned the Power jump (26/16) into the reduced powered one (16/16),
  four cells short of the tower's floor.
- Fix: `HasJumpHeadroom(world, tempPassable)`: a crate cell or a non-zero collision mask blocks,
  the saucer while gone does not.
- Validation: `VerifyBlupiMovement` (wind or a bar overhead: the whole jump; a block: reduced);
  ctest 111/111; 74 of 78 playthroughs passed at once; 107 (its new saucer ledges: moved to the
  ledges whose next hop is straight on, the hop off a saucer taken a little earlier) and 124 (its
  wind tunnel's jump now rose above the wind: the wind fills the tunnel's height) re-planned, and
  their chains 100/120: all 78 pass (2026-11-02).

## BD-0031 — every sound effect played at its file's pitch; the original pitches each channel (gameplay audio) — FIXED_AND_VERIFIED

- Original (mobile-eggbert Sound.cpp:96-110, `tableVolumePitch`): each channel's volume multiplier
  and pitch (XNA SoundEffectInstance.Pitch, octaves -1..1, clamped below 0) are applied at play:
  most effects 0.1-0.5 octave up; Blupi's crate efforts (38 push, 39 pull) and channels 1-2 a whole
  octave up at half volume. The push's effort plays once (`PlayImage(..., bLoop = false)`), cut by
  StopSound when the push ends (Decor.cpp:6138/6147, 3637).
- Galaxy: the table's volume only (its pitch was "never verified"), so every effect sounded lower,
  the crate efforts a full octave (owner OR-0028: a moan); and ch38 was played looped.
- Fix: `Sound::Play` sets the table's pitch; ch38 plays once. Also new: the Setup's effects volume
  multiplies the table's (OR-0029).
- Validation: ctest. Measured 2026-11-06 without playing anything aloud: the playthroughs of 012 and
  045 recorded in real time through SDL's disk audio driver and searched for every sound file at
  every pitch from 0 to 1 octave (`tools/audit/sound_pitch.py`): 14 channels heard, each at its
  table pitch (normalised cross-correlation 0.96-1.00): 16 and 18 at 0.0; 12 at 0.1; 3, 4, 11, 14,
  37, 65, 80 and 88 at 0.2; 1, 38 and 39 (the crate efforts) a whole octave up. None was heard at
  another pitch (channel 2's file resembles channel 1's an octave apart: it is found where 1 plays,
  at 0.4, not on its own). The ear's verdict on the result is the owner's.

## Reviewed, not changed (yet)

- Matching (2026-11-02): the shooters 32 (blupih: one shot straight down at tick 21 of each
  turn) and 33 (blupit: two along its walk at ticks 3 and 21), Decor.cpp:8838-8970; the shot
  (23) travels 5 ticks a cell to the first solid cell (ObjectStart), and sound 52 plays even
  against a wall (ObjectStart returns the freed slot, not -1). Galaxy's FireBlupihShot /
  FireBlupitShot do the same; in 3D the blupit fires along whichever axis it walks.
- Matching (2026-11-02): the fish (17) and the bird (20) met by Blupi (Decor.cpp:5782-5815): the
  creature goes (sound 10, flash 10, the big shake; the others flash 8 and shake small), he dies
  unless in the jeep, the tank or on the skateboard, shielded, hidden or in the contact grace;
  their speeds and rests are BD-0016's. The overcraft (46) boards as every vehicle, ending Cloud
  and Hide (Decor.cpp:5668-5684).

- Contact reach: Galaxy's hazard contact is a 0.7-cell sphere; the original's boxes are 28 px
  across (0.44 of a cell) with type-specific vertical bands (`MoveObjectDetect`, 9689-9767). A
  documented approximation (InteractionSystem.cpp); narrowing it changes every encounter in the
  campaign: a decision for a session of its own, with all playthroughs.
- Kills during busy actions: the original kills only with `m_blupiFocus` (or in Air/JumpAie),
  otherwise the hazard explodes and he survives (5789); Galaxy always killed. Fixed 2026-11-04:
  `InteractionSystem::Update`'s `blupiFocused` (the host: in the air, or not taking a pickup,
  teleporting or waving goodbye): a mine or small enemy met while he is busy explodes and he lives,
  a lethal decoration (201-203) is not met at all (6088 gates its whole block on m_blupiFocus);
  `VerifyInteractionSystem`.
- The patrol step machine spends one tick more per phase in the original (8065-8140: a phase's
  time advances while short of its end, and only the next tick sees it done and switches), lifts
  included: a cycle is the four counts plus four ticks. Fixed 2026-11-04
  (`AdvancePatrolStep`'s `kPatrolSwitchTick`; `VerifyInteractionSystem`'s lift and projectile
  timings follow it).
- Cosmetic: the destroyed enemy's spinning debris, the bulldozer's 90-tick electrocution
  (ObjectType38), the wasp's sting effect (90), the creature's glue splat (53), the balloon pop
  effect (91) were not drawn when this was written. Done since: the flying pieces (ByeByeAdd /
  ByeByeStep, `InteractionSystem::ByeByePieces`), the flashes 90 and 91 and the tentacle 53
  (BD-0054), the Cloud's kill sliding down (38, BD-0055). Nothing open here (checked 2026-11-04).
- Validation for BD-0010..0013: 110/110 and all 78 playthroughs pass (2026-11-01).

## BD-0032 — the crusher sat in the floor, solid; the original's hangs in Blupi's own cell (gameplay, worlds) — FIXED_AND_VERIFIED

- Original (`Decor::IsEcraseur`, Decor.cpp:7277-7289, 5549-5591): Blupi's own cell (`m_blupiPos`
  + 30 px across) holds icon 317 while `m_time / 3 % 10 <= 2`: he is squashed (`m_blupiEcrase`,
  sound 70, the shield gauge as its timer). Icon 317's `table_decor_quart` mask is empty: he walks
  into its cell. In the original's maps all 13 crushers hang in a passage one tile high, solid
  above and below (`build-probe/wave-e/icon_ctx.py 317`).
- Galaxy: crushers placed as floor tiles (045, 053; and an invented row in 199, whose original has
  none), kept solid, detected under his feet.
- Fix: a crusher's cell is passable (`BlupiController::IsPointSolid`, `BlockTypes::isCrusher`),
  detected in his own cell (`GetBlockTypeAt`), drawn as `Decor/aplatisseur.rd4` frame by frame
  (`kAnimatedModelBases`; owner OR-0031). Worlds: 045's booth doorway (66, 8, 24), the corridor's
  lintel row (53, 36, 22-24) and the cage landings' gates (69/79, 44, 23-25); 053's narrowing
  (64, 30, 19) and the treasure hall's two-deep doorway (71-73, 20, 57-58); 199's row removed.
  levelcheck: a crusher is a way, not a floor.
- Validation: `VerifyBlupiMovement` (he walks through a crusher's cell under its lintel);
  playthroughs 045 (in and out of the booth while it is up), 053 (squashed, waits), 199 pass;
  live `plans/evidence/crusher-053.png`.
- Also: the original's squash ends any vehicle (lost), the balloon, swim, surf, the bar and the
  secret powers (Decor.cpp:5556-5573); Galaxy's `TriggerCrush` only squashed. Now as the original
  (045's corridor can be crossed in the helicopter on the ground).

## BD-0033 — the slime drip read as a floor and drawn still (gameplay, render) — FIXED_AND_VERIFIED

- Original (`Decor::IsGoutte`, Decor.cpp:7220-7240, 5513; the draw at 1041): the drip 404 is a
  tile of a ceiling row; Blupi dies when his own cell is the drip's (his centre 15-49 px into it),
  whatever its frame; it is drawn through `table_decor_goutte` (404-410 and gaps, 48 steps of 2
  ticks, each cell's phase offset by `i * 13 + j * 7`).
- Galaxy: the drip is a ground hazard (`GetGroundBlockType() == Drip`, standing on it) and has no
  animation (`BlockDefinitionRegistry` sets none): a still drop.
- In the original's maps every drip hangs right under a solid tile with open cells below it
  (062, 066, 199): he dies going up into it (a jump), or walking into one hung lower.
- Fix: the drip's cell is passable (`IsPointSolid`), it kills in his own cell (`GetBlockTypeAt`),
  his safe position avoids it (as `IsGoutte(.., true)`), it animates through `table_decor_goutte`
  (the registry; the cells beat together, as the lava's), drawn as `Decor/goutte/goutte01-04.rds`
  frame by frame (`kAnimatedModelBases`; 404-408 matched, the two smallest drops 409-410 have no
  scene and are not drawn); levelcheck: a drip is no floor.
- Validation: `VerifyBlupiMovement` (its cell passable, he is in it); the drip worlds'
  playthroughs (062, 063, 066) pass; seen live, the drop forming under its ceiling as its model
  (`plans/evidence/drip-3d.png`).

## BD-0034 — a ceiling met rising slowly made no sound; channel 4 and its seven remaps were never played (audio) — FIXED_AND_VERIFIED

- Original (Decor.cpp:2950-2961): when his rise is stopped by a ceiling, a fast one (faster than
  14 of 16, not dying, not on the skateboard) plays channel 40 and arms JumpAie; any other plays
  channel 4 through `SoundEnviron` by the ceiling's icon: 79, 81, 83, 85, 87, 89 or 91 by its
  material, channel 4 otherwise.
- Galaxy played channel 40 for the fast bump only; channels 4, 79, 81, 83, 85, 87, 89 and 91
  were never played (found by the use case ledger: their only Galaxy references were the enum).
- Fix: `BlupiController` reports `CeilingTapped` with the ceiling's icon (on foot or on the
  skateboard, not swimming); the host plays `Sound::CeilingChannelFor(icon)` (the footstep
  remap's second sound of each material).
- Validation: `VerifyBlupiMovement` (an ordinary jump under a ceiling taps it, the ceiling's icon
  reported), `VerifyInteractionSystem` (the seven remaps and the fallback); ctest 112/112.

## BD-0035 — the mockery's sound played at once and ran on after he left it (audio) — FIXED_AND_VERIFIED

- Original (Decor.cpp:3147-3183, 6457-6470): Mockery plays channel 65 at its tick 1, Mockeryi
  channel 65 at its tick 6, Mockeryp channel 47 at its tick 4; leaving the action stops its sound.
- Galaxy played it the moment the mockery was triggered and never stopped it, so it ran on after
  he walked off (found by the use case ledger: the original's StopSound calls had no Galaxy
  counterpart).
- Fix: `GalaxyEggbertGame::UpdateMockerySound` plays it at the variant's tick and stops it when
  the anim state leaves the variant.
- Validation: build, ctest 112/112; sound timing, not covered by a verifier.

## BD-0036 — a glue shot's death played the soul's ascent (channel 74), not the glue splash (51) (audio) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5914-5946, 7763-7792): a shooter's glue (ObjectType23) touching Blupi
  starts the Glu death through `StartSploutchGlu`: the splash objects, the motors stopped, and
  channel 51.
- Galaxy played channel 74 there (the Clear2 soul's ascent), while its other glue death (the
  glue on the floor, Decor.cpp:5867-5910) played 51. Found reviewing channel 74's call sites.
- Fix: channel 51 at both.
- Validation: build, ctest 112/112.

## BD-0037 — footsteps every 0.3 s while walking; the original has none (audio) — FIXED_AND_VERIFIED

- Original: channel 3 and its material remaps (78-90, SoundEnviron) play only as Blupi lands --
  on the ground (Decor.cpp:2986), on a lift (3018, 4220, 4392), in a vehicle (4204, 4365) -- and
  as an item arrives at the HUD (10268-10303). No sound accompanies the march.
- Galaxy played a footstep every 0.3 s while marching (its own comment called the interval an
  unsourced approximation).
- Fix: the march timer and `Sound::PlayStep` are gone; the landing keeps its material remap.
- Validation: build, ctest 112/112; every channel 3 call site of Decor.cpp read.

## BD-0038 — no reaction after dropping off a bar (Ouf5 missing) (gameplay, animation) — FIXED_AND_VERIFIED

- Original (Decor.cpp:4758-4767, 6277-6284, 3134-3139; table_blupi action 65): letting go of a bar
  by holding Down, or the bar ending under him, arms Ouf5; standing idle 10 to 50 ticks later he
  plays it (icons 288-290, 44 ticks, `blupi-ouf5.rd4`), channel 47 at its 4th tick.
- Galaxy had no Ouf5 (AnimState, table, arming, sound or clip).
- Fix: `AnimState::Ouf5`, `kOuf5Frames` (table_blupi's), armed in both drops of the hanging code,
  played in the window `kOuf5After`..`kOuf5Within`; its sound with the other reactions'; its clip
  built by `build_clip_library.py` from `blupi-ouf5.rd4` (icons 288-290 matched confidently) and
  only the new clip taken into `avatars3d/blupi` (with Clear4's, BD-0039; the rest of the library
  unchanged).
- Validation: `VerifyBlupiMovement` (dropped off a bar four above the floor, he plays Ouf5); seen
  live (`plans/evidence/ouf5-reaction.png`); ctest 112/112.

## BD-0039 — the saw death's frame table was two ticks short (animation) — FIXED_AND_VERIFIED

- `kClear4Frames` had 108 of table_blupi's 110 entries for Clear4 (two closing 329s missing); the
  death lasts 110 ticks, so its last two ticks wrapped round to the first frames (324, 325).
  Found comparing every Galaxy frame table with table_blupi by action (all others equal).
- Fix: the two entries; the DeathLocked.Clear4 clip rebuilt from it.
- Validation: the table comparison; ctest.

## BD-0040 — the Cloud lasted into the water; coming onto the surface ends it (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5287-5307): on the tick Blupi comes onto the surface from dry ground or the
  air (`!m_blupiNage && !m_blupiSurf && IsSurfWater`), besides the splash, the Cloud ends
  (`m_blupiCloud = false`, its gauge hidden). Diving in deeper at once, past the surface band,
  does not end it (that branch has no such line).
- Galaxy kept the Cloud in the water. Found reviewing IsSurfWater's use case (GP-0461).
- Fix: `BlupiController::Step` ends the Cloud on that transition.
- Validation: `VerifyBlupiMovement` (he falls into a pool with the Cloud: it lasts through the fall
  and ends on the surface); live, probe world 995 with the typed `powercharge`: the Cloud's gauge
  goes as he lands in the pool (`plans/evidence/cloud-ends-in-water.png`); the 37 worlds with
  water replayed, all passed; ctest.

## BD-0041 — the jump out of the water splashed when he left the water, and only with Jump held (audio, effects) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5336-5357): the jump out of the water plays channel 22 and splashes
  (`MoveObjectTiplouf`: ObjectType35, channel 64) at the jump itself, ending Surf and Nage there;
  leaving the water any other way plays channel 22 alone (5359-5408).
- Galaxy played both when he left the water cells, the splash only if Jump was still held then.
- Fix: `BlupiController::JumpedOutOfWater()` reports the jump's tick; the host splashes and plays
  22 then, and leaving the water on that jump plays nothing more.
- Validation: `VerifyBlupiMovement` (the jump is reported once and he leaves the water on it);
  the water worlds' replay; ctest.

## BD-0042 — touching a fan with the shield: no explosion, no shake, no sound (gameplay, effects) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5459-5474): `IsVentillo` destroys the fan with its wind, then only the death
  is gated (`m_blupiFocus && !m_blupiShield && !m_blupiHide && !m_bSuperBlupi`); the explosion
  (ObjectType11), the big shake, the stopped motors and channel 10 come with every touch.
- Galaxy destroyed the fan silently while Blupi was shielded, hidden or SuperBlupi (its comment
  claimed the shake sat inside the death's gate).
- Fix: the host explodes, shakes and plays channel 10 on every touch; the death keeps its gate.
- Validation: live, probe world 995 with the typed shield (`roundshield`): walking into a fan he
  survives, the fan goes, the explosion shows and the view shakes
  (`plans/evidence/fan-shield-explosion.png`); without the shield he dies as before; ctest.

## BD-0043 — a door's key flew back from its own HUD place; the original starts every one at x 520 (HUD) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5614-5625): opening a coloured door with its key starts the key's voyage
  from (520, 418) to the door for all three keys, though the second and third arrive at 530 and
  540 when picked up (5960-6001).
- Galaxy started it from 520, 530 or 540 by the key.
- Fix: 520 for every door. Cosmetic.
- Validation: build, ctest.

## BD-0044 — taunts: no height check, none after a jump, none over lava or a crusher, Mockeryp on cooldown (gameplay, animation) — FIXED_AND_VERIFIED

- Original (`MockeryDetect`, Decor.cpp:9518-9601; called every tick he is in control and on foot
  or swimming, 5627-5635; played 6311-6324; the actions 3147-3186): an enemy of the list (2, 4,
  16, 20, 23, 32, 33, 44, 54, 96, 97) whose box (60 px across, the bottom 24 px of its cell) meets
  Blupi's (60 across, 11-60 px below his top; to 150 in the air) arms a taunt in the Ouf slot:
  Mockeryp at the big creature (54), Mockery at one ahead (none at the tracking enemy 2 ahead),
  Mockeryi at one behind; in the air, lava or a crusher one or two cells under his middle arms
  Mockeryi. He plays it if he stands idle within 20 ticks of the last detection, and walking on
  does not disarm it. Mockery and Mockeryi start the 300-tick cooldown at their sound (ticks 1
  and 6); Mockeryp starts none, and nothing is armed during the cooldown.
- Galaxy looked only while he stood idle, within a cell across and at any height (an enemy a
  floor above or below was taunted), never in the air (no taunt after jumping over an enemy, none
  over lava or a crusher), kept searching past a tracking enemy ahead, and put every taunt,
  Mockeryp too, on the cooldown at once.
- Fix: `InteractionSystem::MockeryDetect` (the original's boxes and order; lava looked for a cell
  further down, Galaxy's lava being the floor, GP-0447); the host arms its answer every frame
  (`BlupiController::ArmOuf`); the controller plays an armed taunt within `kMockeryWithin`, keeps
  it through walking, starts the cooldown at the sound tick; the host's sound follows
  `MockeryStarted()`.
- Validation: `VerifyInteractionSystem` (ahead, behind, the tracking enemy, the big creature, out
  of reach, a floor above or below, below him in the air, a type off the list, over lava at three
  heights, over a crusher); `VerifyBlupiMovement` (Mockeryp leaves no cooldown; armed while
  walking it plays on stopping; the cooldown from the sound tick; more than 20 ticks late, none);
  live, probe world 995: jumping over a lava cell and stopping, he turns and taunts
  (`plans/evidence/mockery-over-lava.png`); the full playthrough regression; ctest.

## BD-0045 — a switch could be pressed with the shield, Hide or SuperBlupi (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5529-5531): the switch's gate is `... && !m_blupiShield && !m_blupiHide &&
  !m_bSuperBlupi && m_blupiFocus`, besides the vehicles and the balloon.
- Galaxy gated it on the vehicles and the balloon only.
- Fix: the host's `canActivateSwitch` adds `!IsInvincible()` (shield, Hide, SuperBlupi).
- Validation: build, ctest; the switch worlds (063-065) in the full playthrough regression.

## BD-0046 — the dynamite never cut a saw in the floor nor a drip; Blupi died a cell and a half from a blast (gameplay) — FIXED_AND_VERIFIED

- Original (`DynamiteStart`, Decor.cpp:9064-9166): each blast clears saws (378/379) and drips
  (404/410) in its own row and the one above; Blupi dies if his position is from 64 px before to
  60 px past the blast's, in X and in Y.
- Galaxy cleared saws in the blast's own row only -- its saws lie in the floor, a row lower (the
  floor convention, GP-0450), so no blast ever cut one -- and no drips (its comment said drips had
  no block constant; BD-0033 added it); Blupi died within 1.5 cells in X and Y.
- Fix: saws in the floor and his own row, drips in his own row and the one above; Blupi's box the
  original's in X and Y. Along Z (no original counterpart) the neighbouring cell stays in reach,
  as decided on 2026-10-25.
- Validation: `VerifyInteractionSystem` (the floor saw beside the stick and the drip over it cut, a
  saw out of reach kept; 3.5 cells right of the stick he dies by the last blast, 4.2 he lives; the
  2026-10-25 Z checks still pass); the full playthrough regression; ctest.

## BD-0047 — no vertigo at a lift's edge (gameplay, animation) — FIXED_AND_VERIFIED

- Original (`AscenseurVertigo`, Decor.cpp:9238-9269, called at 4889-4893): standing still on a lift,
  his vertigo measures the lift's edges -- more than 26 px off its middle toward the side he faces,
  he teeters and steps back; with the edge behind him he steps forward. On a conveyor lift
  (icons 311-316) the sides are swapped and the lift lets go of him for 10 ticks: he walks off.
- Galaxy measured only the ground under his feet; a lift is no ground, so he never teetered on
  one.
- Fix: the host passes the ridden lift's middle (`InteractionSystem::RideLiftX/Z`,
  `RideLiftIsConveyor`) to `BlupiController::SetLiftUnder`; on a plain lift the vertigo's feet
  test the lift's top square, 26 px in from its edges, and he counts as standing while the lift
  holds him. Conveyors keep carrying him off their end (their swapped vertigo is not modelled).
- Validation: `VerifyBlupiMovement` (29 px off a lift's middle facing out he teeters and steps
  back onto it; 22 px off he stands; on a conveyor's edge no teeter); the full playthrough
  regression; ctest.

## BD-0048 — the respawn position could be taken where enemies pass, on a lift, a bridge, a ledge, with the shield (gameplay) — FIXED_AND_VERIFIED

- Original (`BlupiAddFifo`'s gate, Decor.cpp:6467-6471; `SetMoveTraj` 8150-8156, `SetBalleTraj`
  7638-7642): his position goes into the respawn FIFO only if, besides the hazards, he is not
  shielded or hidden, not at a ledge (the vertigo's feet), not on a lift, not on a clay bridge
  (`IsBridge`), and not in a cell any object has been in since the mission began (a lift: the
  cell over it) or a shot has crossed (`IsMoveTraj`, `IsBalleTraj`).
- Galaxy checked the hazards, the teleporters and the water only: he could respawn in a
  patroller's path, on a bridge that had crumbled, at a ledge, or where he stood shielded.
- Fix: `InteractionSystem` marks every active object's cell each frame (a lift's the cell over
  it) and every new shot's whole path (`IsTrajectoryCell`); the host's gate adds the shield, Hide,
  the ledge (Vertigo, Recede, Advance), the lift, the bridge and the trajectories.
- Validation: `VerifyInteractionSystem` (an enemy's cells stay marked, a lift marks the cell over
  it, other cells are not); the full playthrough regression (scripts that die and respawn);
  ctest.

## BD-0049 — nothing flew away: destroyed enemies, crates, the invert potion, fans and the lost helicopter (visual) — FIXED_AND_VERIFIED

- Original (`ByeByeAdd` / `ByeByeStep` / `ByeByeDraw`, Decor.cpp:10058-10128; `ObjectDelete`
  7881-7916; `ModifDecor` 7926-7934; `ByeByeHelico`, called wherever the helicopter is lost):
  a destroyed object of the list 4, 12, 16, 17, 20, 40, 96, 97 (by the dynamite, the Cloud, a
  decoy, its own contact with Blupi, a wall for the homing star, the invert potion taken), a fan
  destroyed by touching it, and the helicopter lost (wrecked, stung, glued, dead, at the exit)
  leave a copy of their picture that rises on a slowing curve for 10 ticks, falls, drifts
  sideways 10-19 px a tick and spins (7 the bulldozer and the helicopter, 33 the fish and the
  bird, 17 the rest; a blasted crate 7-22 either way), gone after 30 ticks (the invert potion and
  the helicopter at half speed).
- Galaxy just removed them.
- Fix: `InteractionSystem::Retire` (ObjectDelete) at those deletions, `ByeByePiece`s stepped
  once a 20 Hz tick with the original's formula (sideways along X); the host adds the fan's and
  the helicopter's and draws each piece as its model (the crate its cube, the helicopter its
  parked model, a sprite without a model turned as the original's), spinning about the view
  axis. The invert potion's animated frames while it flies are not reproduced (its picture
  stays as it was).
- Validation: `VerifyInteractionSystem` (a bulldozer caught by the blast flies, spinning 7, rises
  about 10 ticks, gone after 30); live, the probe world's fan and dynamite; ctest.

## BD-0050 — the cleanall cheat removed the enemies without their explosions (cheat, visual) — FIXED_AND_VERIFIED

- Original (`CheatAction` CleanAll, Decor.cpp:1793-1822): every enemy becomes an explosion
  (ObjectType8) where it was, with channel 10 and the small shake.
- Galaxy removed them with the shake only.
- Fix: `CheatCleanAll` leaves an explosion at each; the host plays channel 10.
- Validation: `VerifyInteractionSystem` (explosions where the enemies were); ctest.

## BD-0051 — dynamite and the decoy put down at a ledge; the decoy beside a charger or with dynamite carried (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:4792-4848): the action puts dynamite down only with both feet on the ground
  (2 px probes at 18-20 and 40-42 px of his 60); else, without dynamite, the decoy with both feet
  down and no charger (ObjectType31) beside him (`MoveChargeDetect`: its box 10 px wider each side);
  refused, the decoy gives his shrug (Ouf3, channel 27).
- Galaxy asked only that he stand on the ground, and tried the decoy also when the dynamite was
  refused.
- Fix: `BlupiController::BothFeetOnGround` (11 px either side of his middle along his facing)
  for both; `TryPerso` refuses beside a charger and while he carries dynamite; the shrug only
  without dynamite.
- Validation: `VerifyBlupiMovement` (one foot over a ledge, both inside a floor);
  `VerifyInteractionSystem` (no decoy beside a charger nor with dynamite, one elsewhere); ctest.

## BD-0052 — the overcraft could not climb over a lift beneath it, only on it (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:3936-3967, `MoveAscenseurDetect(m_blupiPos, OVERHEIGHT)` 9815-9842): the
  overcraft climbs while the ground or a lift lies within OVERHEIGHT (80 px) under it -- a
  lift's top from 18 px over his feet to 79 under them, within 50 px across.
- Galaxy counted a lift only while riding it.
- Fix: `InteractionSystem::LiftWithinOverHeight`, passed by the host
  (`BlupiController::SetLiftBelow`) while he drives the overcraft.
- Validation: `VerifyInteractionSystem` (in reach at 0.5 and 1.1 cells under, not at 1.4 nor
  0.9 across); ctest.

## BD-0053 — the follower star woke within a sphere, not the original's box (gameplay) — FIXED_AND_VERIFIED

- Original (`MoveObjectFollow`, Decor.cpp:9646-9678; `BlupiRect` on foot 11-58 px): the dormant
  star (96) wakes (97, channel 92) when its cell padded by 100 px meets Blupi's 16-44 px strip,
  11-58 px under his top: 144 px either way across, from 149 px over him to 158 under.
- Galaxy woke it within a sphere of 132 px (2.06 cells): the same straight across, but not two
  cells off on a diagonal nor two over him, where the box wakes it.
- Fixed 2026-11-03 (Q-007 taken as recommended, the faithful-remake rule): `InteractionSystem`
  wakes it in the box, across on X and on Z alike (`kFollowerWakeAcross/Over/Under`);
  `VerifyInteractionSystem` 1.555 (a diagonal two cells off and 2.2 over wake; 2.4 over and 2.3
  across sleep). The worlds laid out with the sphere re-laid where the box would wake a star the
  route must pass asleep:
  - 053: the lava crossing's southern star at z 59 (it woke as he waited for lift A); the gallery's
    north treasure taken from x 56.3 and the door "13" reached by (57, 6.5); the treasure hall's
    columns at y 21-24, its crushers waited out at z 59.6, its first treasure reached along z
    59.6; the cage's north row woken from the doorway's west then east side.
  - 052: S2's stars under the ceiling now wake as he passes (as the original's would, 2.33 cells
    over him): the drop is timed to S2's lift so that he rides up into the pocket, where they
    burst on its floor; the hangar's eight stars round the column, the alcove and the pen re-laid
    from the route's traced flight to clear the box by 0.3 cells or more; the well foot's four
    stars three over the floor.
  Validation: 050, 052, 053 pass; the full regression 78/78 (`build-probe/wave-e/bd53-full.txt`);
  CTest 112/112.

## BD-0054 — no sparks or flash at the crusher, no flash as the balloon bursts, no tentacle at the trap or the big creature (visual) — FIXED_AND_VERIFIED

- Original: the crusher catching him plays 70 with four sparks (ObjectType41 at -60, 60, 10,
  -10), a flash (90, 12 ticks) and the big shake (Decor.cpp:5582-5591); springing up out of it,
  41 with the same sparks and flash (5185-5196); the balloon ending, by time or a hazard, 41 with
  a flash (91, 6 ticks; 5165-5174, 5768-5777); the trap (373) and the big creature (54) catching
  him, a tentacle (53, 90 ticks, its frames two ticks each) where he stands (5504-5510,
  5871-5873).
- Galaxy played the sounds only (the types' pictures and frame tables existed, unspawned; the
  tentacle's table ran at one tick a frame).
- Fix: `InteractionSystem::SpawnEffect` and the three expiring entries; the host spawns them at
  those moments (the crusher's sparks through `SpawnInvertBurst`, the same ObjectType41 burst),
  the creature's tentacle in `InteractionSystem`; the tentacle's frames two ticks each.
- Validation: build, ctest; live, the probe world's crusher and spikes.

## BD-0055 — the decoy trap's dissolve never went away; the Cloud's kill had no flash (visual) — FIXED_AND_VERIFIED

- Original: the dissolve (ObjectType37) left where an enemy met a decoy goes after 70 ticks
  (Decor.cpp:8976-8986); an enemy the Cloud's arc kills leaves an electric flash (ObjectType38,
  90 ticks, its first 30 from blupi1.png) sliding down at 5 px a tick to the floor under it
  (7976-7983; `SearchDistRight`'s fall less a cell, 7647).
- Galaxy spawned the dissolve with no lifetime -- it looped where it was for the rest of the
  mission -- and the Cloud's kill played channel 59 only.
- Fix: the two expiring entries; the Cloud's kill spawns the flash with its slide.
- Validation: `VerifyInteractionSystem` (the dissolve lasts 70 ticks); build, ctest.

## BD-0056 — Hide left no trail (visual) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5236-5250, 870-881, 8391): while hidden, every 10 px he moves leaves a
  copy of his current picture (ObjectType58) that fades from 30 % opacity to nothing over 20
  ticks.
- Galaxy drew him at 30 % (OR-0016) but left no copies.
- Fix: the host keeps the copies of his pose (world transform, clip and its time, face) taken
  every 10 px (Manhattan, over X, Y and Z) while hidden and draws them fading over 20 ticks.
- Validation: build, ctest; live, the typed `hidedrink` and a walk.

## BD-0057 — a bar over wind read as a landing, not hung from (gameplay) — FIXED_AND_VERIFIED

- Original (`Decor::GetTypeBarre`, Decor.cpp:7158-7186): a bar (138, 202) is a landing (2) only
  where the tile under it is not passable (`!IsPassIcon`: its quarter mask not empty); over wind,
  water, a drip or a teleporter's pillar he hangs from it. The original's 113 hangs its bars
  (51-53 and 55-58, row 34) right over the wind of its fan corridor (row 35, fan 129 at x 50).
- Galaxy (`BlupiController::GetBarreCellType`) asked `IsSolidAt`, which is "not air and not
  decor": the wind's cell counted as a floor, the bar as a landing, so a jump up to such a bar never
  caught it (found building 113's windy stretch: he rose into the bar's cell and fell back).
- Fix: the tile under the bar is a floor only when its collision mask (the original's quarter
  mask) is not empty, or a crate stands there (as in Blupi's collision).
- Validation: `VerifyBlupiMovement` (a bar over wind is hung from, over a block it is a landing);
  ctest 112/112; 113 and its hub 110 played through (the bars over the windy stretch). No other
  shipped world has a bar over a passable tile (scanned).

## BD-0058 — in the water Blupi went as fast as on foot (gameplay, timing) — FIXED_AND_VERIFIED

- Found 2026-11-04 by the original's tables Galaxy did not cite (`table_vitesse_nage`,
  `table_vitesse_surf`).
- Original (Decor.cpp:4574-4610 Nage, 4693-4729 Surf): along, his speed `m_blupiVitesseX` eases toward
  8 px a tick by 1 a tick (by 2 back to 0 with no key), and each tick he moves that times the stroke's
  table: under the water `table_vitesse_nage[phase % 14 / 2] / 7` ({2,1,5,10,8,6,4}: 5.9 px a tick on
  average, a pulse each stroke), at the surface `table_vitesse_surf[phase % 12 / 2] / 10`
  ({0,2,5,8,3,0}: 2.4 px a tick). On foot he walks at 8.
- Galaxy moved him at his walking speed (2.5 blocks/s) in the water: 1.4 times too fast under it, 3.3
  times at the surface.
- Fix: `BlupiController` `kWaterAlongMax`/`Accel`/`Decel`, `m_waterVitesse`, `m_waterStrokeTicks`.
- Validation: `VerifyBlupiMovement` (about a block a second at the surface; out onto a low bank in
  2.7 s); the playthroughs (play passes 15-17, re-timed with BD-0059 and BD-0060).

## BD-0059 — the walk started at full speed, and the power did not quicken it (gameplay, timing) — FIXED_AND_VERIFIED

- Original (Decor.cpp:3392-3404, 3447-3459): walking moves `table_vitesse_march[min(m_blupiPhase, 3)]`
  px a tick ({2, 4, 6, 8}: he eases in over four ticks from a stand; 8 in the air), times 3 / 2 with
  the power (12 px a tick under way).
- Galaxy walked a flat 8 from the first tick (the ease-in "not modeled") and the power only made him
  jump higher.
- A landing starts the ease-in over: it sets `StopJump` at phase 0 (Decor.cpp:2988-2998) and, the stick
  held, `March` at phase 0 the same tick (3376-3380).
- Fix: `BlupiController` `m_marchTicks` (counted on the ground only) and the march table in Step();
  `VerifyBlupiMovement` (2 px the first tick, 28 px in five, 42 with the power; 4 px the second tick
  after a hop's landing); the golden trace re-baselined (its walk now eases in: 2, 4, 6, then 8 px a
  tick).
- Playthroughs: the scripts that walked off a ledge to an endpoint just past its edge (055, 056, 058)
  now go a body's half-width (18 px) clear of it; 053's north-west alcove is entered along z 11.15
  (its wall's corner stopped him from 11.22); 057's tank corridor waits 84 frames (60-108 pass).

## BD-0060 — the conveyor carried him at half the original's pace; the big creature caught him busy (gameplay) — FIXED_AND_VERIFIED

- Original: a conveyor lift (47/48) adds 2 px a tick to his motion (Decor.cpp:8161-8168), 0.625 blocks
  a second; the big creature (54) catches him only with `m_blupiFocus` (5867-5868).
- Galaxy: 0.3 blocks a second ("no unit conversion established"); the creature caught him while he
  took a pickup or teleported.
- Fix: `kConveyorNudgeSpeed` 2 * 20 / 64; the creature's gate takes `blupiFocused` (BD-0010's busy
  Blupi). The same gate for the spike trap (Decor.cpp:5504-5505) and the teleporter (5593-5594), which
  the code said "focus isn't modeled" for: `GalaxyEggbertGame::BlupiFocused()`.
- Validation: ctest 112/112; the playthroughs (play pass 17: 102 has the conveyor -- its caterpillars
  re-timed --, 061, 062, 065, 066, 199 the creature).

## BD-0061 — the big creature catching him in a vehicle did not burst it (audio-visual) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5884-5905): when the creature (54) catches him in a vehicle (helicopter,
  overcraft, jeep, tank, skate) or squashed, the vehicle bursts where the creature is
  (`ObjectStart(..., ObjectType10)`), channel 10 plays and the screen gives a small shake; on foot
  only channel 51.
- Galaxy always played channel 51 (the code said InteractionSystem could not tell; the caller already
  passes "neither in a vehicle nor squashed" for the crates).
- Fix: `InteractionSystem` creature branch: ObjectType10 at the creature, channel 10,
  `SmallShakeTriggered` when `!blupiCanPushCrate`.
- Validation: `VerifyInteractionSystem` (a shake in a vehicle, none on foot); ctest.

## BD-0062 — a shot or a hazard could kill him cheering at the exit; a shot, waving goodbye (gameplay) — FIXED_AND_VERIFIED

- Original: Win (the exit, Decor.cpp:1902-1904, 6174-6176) and Bye (a world's door, 5486-5488) clear
  `m_blupiFocus`, so a hazard he touches then explodes and he lives (5788-5790, BD-0010); a shot (23)
  meets him only outside Win, Bye, the deaths (Clear1-8, Glu, Electro) and the gone phase after them
  (Hide) (5914-5917) -- not gated by focus, so it still meets him taking a pickup or teleporting.
- Galaxy: `BlupiFocused()` left out the exit's cheer (`winCheerActive_`), and the shot asked only for
  the shield, the drink or superBlupi: shot while cheering or waving goodbye, he died there.
- Fix: `BlupiFocused()` counts the cheer; `BlupiOutOfPlay()` (cheer, goodbye, death lock, gone) gates
  the shot (`InteractionSystem::Update`'s `blupiOutOfPlay`).
- Validation: `VerifyInteractionSystem` (a shot passes him out of play and goes on, kills him in play);
  ctest 112/112.

## BD-0063 — the spring threw him no higher with the Power (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:2900-2907): the spring's launch is -25 with Jump held and the Power, -19 held
  without it, -16 not held with it, -10 without; "held" counts only with `m_blupiFocus`.
- Galaxy had only the two without the Power (its comment: "Power isn't modeled yet", stale since the
  Power came) and took the held Jump whatever his focus.
- Fix: `kSpringBounceHeldPowered`/`NotHeldPowered` (25/16 and 16/16 of `kJumpSpeed`, the same
  anchoring as the others); the caller passes Jump held and focused.
- Validation: `VerifyBlupiMovement` (with the Power a held bounce goes higher); ctest; the
  playthroughs (play pass 17).

## BD-0064 — the training hints showed "[Move]"/"[Jump]"/"[Action]", not the pad's pictograms; no accelerometer texts (UI) — FIXED_AND_VERIFIED

- Original (Decor.cpp:1258-1310, MyResource's TX_TRAINING101-410 and their "a" variants): the hint
  strings carry bytes 0-13 that Text::DrawChar draws as text.png's glyphs 0-13 -- the directional
  wheel with each of its eight directions lit, Jump, Action, Down, the tilted phone -- inline in the
  sentence; with the accelerometer on the text's id is +10000 (the tilted phone's variants).
- Galaxy replaced the pictograms with bracketed labels (some of them wrong: the box pushed "with
  [Action]" where the original shows the wheel), and had no accelerometer texts.
- Fix: `TrainingHints.cpp`'s records generated from the original tables and English strings (both
  variants, the control bytes as octal escapes); the lookups return `std::string_view` (glyph 0 is a
  NUL byte) and take `accelerometer`; the host passes the Setup's accelerometer switch.
- Validation: `VerifyInteractionSystem` (the original texts, the accelerometer's); live, mission 11's
  first hint shows the wheel inline ("Use the directional wheel" + the wheel, its right lit).

## BD-0065 — every animated tile of a kind on one frame; the original offsets each cell (visual) — FIXED_AND_VERIFIED

- Original (Decor.cpp:1036-1078): lava, the spike trap, the drip, the saw and the first water draw
  `table[(i * 13 + j * 7 + m_time / div) % n]` -- each cell its own place in the cycle; the second
  water `table_decor_eau2[(i * 11 + j * 7 + m_time / (3 + (i * 17 + j * 13) % 3)) % 6]`, its own pace
  as well. The crusher, Temp, the wind and the fans are in step.
- Galaxy drew every cell of a kind on the same frame (`TerrainAnimDivisor.hpp` said "not modeled").
- Fix: `BlockDefinitionRegistry::ResolveAnimationIconAt` (row = -y; depth adds 11 z / 13 z, Galaxy's
  own, so cells side by side in depth differ too); `TerrainRenderer` builds each cell on its frame (the
  lava's meshes cached by the cycle's step), the host's animated models (saw, drips) likewise.
- Validation: `VerifyDefinitionRegistries` (lava's neighbour 13 frames on, the crusher in step, the
  second water's paces); live, 082's lava pit; the golden frames re-baselined (an animated model
  at the right edge on its own cell's frame).

## BD-0066 — the drip, the saw and the crusher caught him busy (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:5513-5514, 5521-5522, 5551-5552): like the spike trap (BD-0060), the drip, the
  saw and the crusher meet him only with `m_blupiFocus` -- not while he takes a pickup, teleports,
  waves goodbye or cheers at the exit (on the ground). Lava and the lightning do not ask it.
- Galaxy asked focus of the spike trap only; nor of the bridge that builds itself under him
  (IsBridge, 5608).
- Fix: `BlupiFocused()` in the host's drip, saw and crusher checks; `blupiFocused` in
  InteractionSystem's bridge trigger.
- Validation: build, ctest; the playthroughs (play pass 18).

## BD-0067 — the spike traps did not snap while Blupi was not focused (visual) — FIXED_AND_VERIFIED

- Original (Decor.cpp:1032-1037): the trap (373) draws `table_decor_piege1` (16 frames, 4 ticks each)
  while `m_blupiFocus`, else `table_decor_piege2` ({374, 373, 347, 373}, 2 ticks each) -- the jaws
  snapping, e.g. on Blupi caught in one (BlupiDead clears the focus, 6559), or while he takes a pickup.
- Galaxy always drew piege1.
- Fix: `ResolveAnimationIconAt(..., blupiFocused)`; `TerrainRenderer::Update` takes the focus (and
  rebuilds when it changes); the host passes `BlupiFocusRaw()` (death lock, gone, pickup, teleport,
  goodbye, the exit's cheer).
- Validation: `VerifyDefinitionRegistries` (piege2 while unfocused); build, ctest.

## BD-0068 — hanging from a bar he went at his walking speed (gameplay, timing) — FIXED_AND_VERIFIED

- Original (Decor.cpp:4734-4743): along a bar `end.X += (int)(m_blupiSpeedX * 5.0)` -- 5 px a tick
  with the stick full over, no ramp: 1.56 blocks a second.
- Galaxy reused the walk's speed (8 px a tick, 2.5 blocks a second; its comment said no anchor was
  established for a conversion -- the walk's own 8 px = `kMoveSpeed` is that anchor).
- Fix: `kSuspendMoveSpeed` = 5 * 20 / 64.
- Validation: `VerifyBlupiMovement` (50 px in ten ticks along the bar); ctest; the playthroughs with
  bars (play pass 18).

## BD-0069 — squashed he went at half his walk and fell like a stone (gameplay, timing) — FIXED_AND_VERIFIED

- Original (Decor.cpp:4109-4175, 2823-2833): squashed (`m_blupiEcrase`) he never enters the common
  fall (`!m_blupiEcrase` keeps `m_blupiAir` off); his own block moves him: up to 4 px a tick on the
  ground and 7 off it, gaining 1 a tick toward it with the stick held (never cut back when the limit
  drops), losing 2 a tick without it; down, his speed gains 1 px a tick up to 2 (a faster one he
  keeps) -- he drifts down off a ledge.
- Galaxy: half his walk's speed (`kEcraseSpeedMultiplier` 0.5, "an approximation") and the common
  gravity.
- Fix: `m_ecraseVitesse` with `kEcraseGroundMax`/`AirMax`/`Accel`/`Decel`; `kEcraseGravity`/`FallMax`
  in the vertical step.
- Validation: `VerifyBlupiMovement` (34 px in ten ticks, a 2 px slide, 19 px of fall in ten ticks);
  ctest; the playthroughs with crushers (play pass 18).

## BD-0070 — the helicopter flew the original's unused "normal" figures and hovered still (gameplay, timing) — FIXED_AND_VERIFIED

- Original: `Def::EasyMove` is always true (Def.hpp:210), so the helicopter's "easy" branch is the one
  played (Decor.cpp:3724-3835): along, up to 12 px a tick gaining 0.5 a tick (2 lost a tick without
  the stick); up to 7 px a tick climbing, 8 diving, 0.5 a tick either way; with neither, its vertical
  speed goes by 1 a tick to 1 px a tick down -- in the air it sinks slowly.
- Galaxy flew the other branch (16 px a tick, gaining 1; -10/+12) -- its comment said "easy move isn't
  modeled, always using the normal-setting figures" -- and held still in the air without input.
- Fix: `kHelicopterMaxSpeed` 12 px, `kHelicopterAccel` 0.5, `kHelicopterAscendSpeed`/`DescendSpeed` 7/8
  px, the idle sink (`kHelicopterSink`, 1 a tick toward it); the 0.5 to 1 px lift-off snap.
- Validation: `VerifyBlupiMovement` (10 px of sink in ten idle ticks, 105 px along in twenty); ctest;
  the helicopter playthroughs (play pass 18).
- Follow-up: the glue drop's kick up (Decor.cpp:3067, `m_blupiVitesseY -= 10`) had borrowed the climb's
  limit and so fell to 7 px a tick with it; it is its own 10 px (`kHelicoGluKick`).

## BD-0071 — the jeep and the tank fell as Blupi; the jeep did not roll and tip over an edge (gameplay, visual) — FIXED_AND_VERIFIED

- Original: off the ground the jeep and the tank gain 5 px a tick down each tick, to 50
  (Decor.cpp:4193-4206, 4356-4369; Blupi's own fall 4, to about 40). On the ground the jeep feels for
  it under each end (16 px strips 2-18 px in from its sides, 4180-4192): with ground under one end
  only, it rolls toward the other at 1 px a tick more each tick, to 20, whatever the stick says
  (4224-4239), and tips that way 5 degrees a tick, to 45 (4280-4291, `m_blupiRealRotation`). With
  the parked vehicles' vertigo (BD-0020) this is what takes a jeep off an edge.
- Galaxy: the common fall for both; no strips, no tipping.
- Fix: `kHeavyVehicleGravity`/`FallLimit` for the jeep and the tank in the air; the jeep's strips
  (12-28 px from its middle along its facing) and `m_vehicleTilt` in its ground step; the host turns
  the jeep's model nose down by `GetVehicleTilt()`.
- Validation: `VerifyBlupiMovement` (front over the void: it rolls forward unasked and tips nose down;
  on the middle of its block: still and level; BD-0020's parked checks still pass); ctest; the jeep
  and tank playthroughs (play pass 19).

## BD-0072 — the wind pushed him while he took a pickup (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:2773-2798): the wind's push asks `m_blupiFocus` besides no lift, jeep, tank or
  skate; its strengths (9 px a tick across, 20 up or down) Galaxy had already.
- Galaxy pushed him through a pickup's freeze, a teleport and the goodbye wave.
- Fix: `BlupiController`'s wind lookup skipped while `m_pickupFrozen`, `m_teleporting` or `m_bye`.
- Validation: `VerifyBlupiMovement` (taking a pickup in the wind he stays put); ctest.

## BD-0073 — eggs were capped by the eggs taken, not by his lives (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:6007, 10264-10267, `MAX_EGG_COUNT` 10 at :96): an egg is a life; it is taken
  only while `m_nbVies` (his lives) is under 10, and its voyage ends at `VoyageGetPosVie(m_nbVies + 1)`.
  At ten lives touching one does nothing, it stays; after a death it can be taken again.
- Galaxy gated on `lifeEggCount_`, the eggs taken so far: from his 3 lives ten eggs made 13 (062's
  playthrough reached 12), and once ten were taken no egg could ever be taken again. Found
  2026-11-05 by the target audit (`tools/audit/target_reach.py`), probing 062's corridor egg.
- Fix: `InteractionSystem` gates the touch and the voyage's reward on `lives_ < 10` and aims the
  voyage at the lives row's next slot; `lifeEggCount_` is only the autoplay's tally.
- Validation: `VerifyInteractionSystem` (from 4 lives six eggs make 10; at 10 an egg stays; after a
  death the same egg is taken); ctest 112/112.
- Playthroughs: 17 had waited for eggs a full lives row no longer takes (and 034's lift boarding
  moved, the egg's voyage now ending at the next life slot). Each such wait became `until mtick >= T`,
  T the mission tick the egg was taken at in play pass 23 (so a run keeps its old timing whether
  the egg is taken or not: alone from 3 lives, or in a hub chain with the row already full); egg
  counts expected at a mission's end became `expect lives >= min(10, 3 + eggs)`; 034 lets a lift
  that is leaving go. Play pass 25 (below in `handoff.md`).

## BD-0074 — Blupi walked crouched; a step now stands him up (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:3252-3270, 3377-3383, 3435-3441): Down starts with the down key while not
  moving and never moves him (both the march and its displacement exclude Down); the pad gives one
  direction at a time, so going on means standing up and walking. A hanging mine (3) spares him
  while he is down (MoveObjectDetect, 9722). All 276 of the original's hanging mines stand still.
- Galaxy let him walk crouched (the Down pose sliding along), and its worlds used that to pass
  hanging mines at his height. Owner 2026-11-05 (OR-0041, MR-0008): "the crouch should be
  cancelled and Blupi walk normally as in Mobile Eggbert"; asked, the owner chose the original's
  rule and the worlds adjusted.
- Fix: `BlupiController::Step` drops the crouch on foot while he moves (not on a bar, swimming, in a
  vehicle, nor pulling a crate: Down with a direction away from a held crate is the original's
  Pop); the hanging mine's exemption follows his Down state, not the key. Worlds: the mines that
  hung at his height over a way he must take now hang a cell higher, over his head (walked under,
  a jump meets them): 045's gallery pads (`cheese.low_mine`, also 044's), 104's narrow passage,
  107's low passage.
- Validation: `VerifyBlupiMovement` (crouched, a step forward stands him up and he walks); play
  pass 30.

## BD-0075 — Blupi's own sounds played while he was hidden (gameplay audio) — FIXED_AND_VERIFIED

- Original (Decor.cpp:1518-1572, `Decor::PlaySound`): while `m_blupiHide` (the drink's Hide power,
  3230; 400 ticks, a level every 4) the channels 1-7, 20-25, 27, 32, 34-40, 46-49, 64, 65 and 78-91
  -- his jumps, landings and taps, the water's, the crates' efforts, his falls -- are not played;
  the world's sounds are. The sounds played straight through `m_sound->PlayImage` (the vehicles'
  motors, the voyages to the HUD, the menu's click) do not pass that filter.
- Galaxy: every sound played whether he was hidden or not (GP-0408 had compared the call sites, not
  the filter in front of them). Found 2026-11-06 by listing the original's functions outside
  Decor.cpp that Galaxy never names (`Sound::PosImage`, `GetBalance`, ...) and reading their callers.
- Fix: `Sound::SetBlupiHidden` (set every frame of play from `BlupiController::IsHidden`),
  `Sound::MutedWhileHidden` (the list), `Sound::PlayDirect` for the voyages' arrival (channel 3, the
  only listed channel the original plays past the filter).
- Validation: `VerifyInteractionSystem` (the list, channel by channel); a probe run of 051 (take the
  drink, jump hidden; a temporary log of every play): the jumps before the drink played channel 1,
  the two while hidden asked for it and were left out. 112/112.
- Reviewed with it, unchanged: the original's volume by the sound's place on screen (full on screen,
  silent 320 px beyond its sides or 160 px above or below, `Sound::GetVolume`) and its stereo
  balance (GP-0412, an adaptation): see BD-0076.

## BD-0076 — the world's sounds were heard from anywhere; the original's only within three screens of its view (gameplay audio) — FIXED_AND_VERIFIED

- Original (Sound.cpp `PlayImage` -> `GetVolume`, Decor.cpp `PlaySound` passing the source's place
  less `m_posDecor`): the volume is `1 + (X / 640) * 2` left of the screen, `1 - ((X - 640) / 640) * 2`
  right of it, the same with 480 and 3 above and below, clamped to 0..1 -- in whole numbers (`pos.X`
  is an int), so a sound is heard fully on the screen and up to one screen beyond each side and not at
  all farther: three screens by three round the view. The sources: the shooters' shots (52, Decor.cpp
  8884-8967), objects meeting and blasts (10), the doors sliding open (33, 11565/11687), the clay
  bridge (73), the follower's wake (92), the creatures' glue (51), and the lightning's zap (69), which
  `BlitzActif` plays from the drawing of the screen's bolts (1079): only a visible column zaps.
- Galaxy: every sound at full volume wherever it was made; the zap whenever the world held any
  column, its own note calling that equivalent "since Sound has no positional audio".
- Fix: `Sound::PlayAt` with `Sound::SetListener` (Blupi, every frame; the original's view follows
  him): heard within 15 cells of him along X and along Z (half a screen and a screen, 960 px) and
  11.25 up or down (720 px), not farther (`Sound::Audible`). The 13 world-object sites in
  `InteractionSystem` play from the object's place; the zap from each column's (`blitzColumns_`).
  Blupi's own sounds stay as they were (always on his screen). Panning stays out (GP-0412).
- Validation: `VerifyInteractionSystem` (the reach's bounds). Recorded (SDL's disk driver, real time)
  the first 47 s of 025's playthrough, whose one column (26, 28, 50) is 24 cells from the start and
  within reach only from tick 720 to 1740: the build before this change zapped 58 times from 0.0 s to
  47.3 s; this one 20 times, from 11.7 s to 27.6 s, none outside that stretch. 15 worlds hold columns
  (032's nearest 42 cells from the start, 102's 72): they zapped throughout before. 112/112.

## BD-0077 — Resume starts the mission again; the original continues the moment it was left (save, UI) — FIXED_AND_VERIFIED (Q-015, 2026-11-06)

- Original (Game1.cpp:186-208, 255-270; Decor.cpp:10974-11300): when the game is deactivated during
  play (`OnDeactivated`: a call, the home button, the window losing focus) `Decor::CurrentWrite`
  writes the whole mission as it stands -- the view, Blupi's place, action, speeds, vehicle and
  powers with their counts, the decor's cells, every moving object with its phase, the counters;
  a clean exit (`OnExiting`) deletes it. When the game starts again with one written, the Wait
  phase reads it (`CurrentRead`) and shows Resume; Continue goes on from that very moment.
- Galaxy before: Resume was shown when a gamer has progress (checkpointed at each win or loss) and
  Continue starts the checkpointed mission from its beginning with the lives. Its comment said no
  mid-level state existed in the original; that was wrong (corrected 2026-11-06). The ledger rows
  (GP-0521..0523, `CurrentDelete`, `CurrentWrite`, `CurrentRead`) had it as Galaxy's own save
  format, an adaptation; they now say this.
- Effect: on the desktop the snapshot matters only if the process ends while deactivated (a crash,
  the session closed); on the web a closed or discarded tab, on Android a background kill -- there
  a mission in progress is lost and starts again.
- Fix (Q-015, the owner's replay design): engine-independent `ResumeLog` stores the mission's
  start (build, mission, lives, speed, RNG seed, spawn/yaw, cheats, camera mode and initial
  SaveData settings/doors), then run-length encoded raw keyboard/gamepad-merged keys, mouse,
  touch and consumed accelerometer readings. Viewport size and sensor initialization are
  recorded because input interpretation and HUD voyages depend on them. Versioned text has a
  checksum and complete footer; truncated, damaged, excessive and foreign-build files are refused.
- The host uses a fixed 1/60 s `UpdateTick`, with one input acquisition point. Mission starts
  reset transient input/camera state; `InteractionSystem::SeedRandom` seeds both gameplay and
  cosmetic draws. Play, Pause and PlaySetup ticks are recorded. `OnDeactivated` writes
  `resume.txt` atomically during Play (`/save/resume.txt` on Emscripten); other phases, clean
  exit, mission completion and another mission delete it. Wait prefers a valid log. Continue
  reloads and replays 600 ticks per outer Update, without drawing, audio or autoplay advancement,
  then appends live input. The checkpoint fallback and editor play-tests remain separate.
- Determinism audit: `InteractionSystem::rng_` was the gameplay random source; it is now seeded.
  Live elapsed time and hardware input are replaced by fixed ticks and recorded readings.
  Hide trails were emitted in Draw; they now emit in UpdateTick using the same pose transform.
  Wall-clock calls remain only in render diagnostics/terrain scheduling; unordered gameplay
  containers are lookup sets/maps, not iteration-driven simulation.
- Verification: existing `build-cna-opengl3`, shared compiler cache; **112/112 CTest pass**.
  `VerifyAutoplay` tests exact float/input/start round trip, RLE, every truncated prefix,
  foreign build, corruption, trailing garbage, zero/excessive runs, invalid keys and zero ticks.
  `GALAXY_RESUME_SELFTEST=1500` serializes/parses/reloads/replays and compares Blupi's position,
  movement accumulators, action, vehicle and powers, counters, all active object positions and
  phases, voxel hash, clocks, camera and Hide trails. All five playthroughs still pass:

  | Mission | Self-test output | Playthrough output |
  |---|---|---|
  | 012 | `RESUME SELFTEST equal` | `GalaxyEggbertCNA: autoplay passed after 3296 ticks (autoplay_trace.txt).` |
  | 025 | `RESUME SELFTEST equal` | `GalaxyEggbertCNA: autoplay passed after 13680 ticks (autoplay_trace.txt).` |
  | 045 | `RESUME SELFTEST equal` | `GalaxyEggbertCNA: autoplay passed after 16815 ticks (autoplay_trace.txt).` |
  | 051 | `RESUME SELFTEST equal` | `GalaxyEggbertCNA: autoplay passed after 8308 ticks (autoplay_trace.txt).` |
  | 199 | `RESUME SELFTEST equal` | `GalaxyEggbertCNA: autoplay passed after 16608 ticks (autoplay_trace.txt).` |

- Desktop lifecycle verified under silent Xvfb: focus loss in Play writes, in Pause deletes;
  a killed process preserves the log; a fresh process shows Resume and Continue reconstructs
  the mission, including Pause/Continue mouse history, and appends ticks; WM close deletes it.
  Logs/screenshots: `build-probe/resume-world*.log`, `resume-ctest.log`, `resume-lifecycle.log`,
  `resume-live-run/`. The live-input equality check also accepts `GALAXY_RESUME_SELFTEST` without
  autoplay; tick 350 with live keyboard and Pause/Continue mouse input also reported
  `RESUME SELFTEST equal`. Results are printed to stdout and to the autoplay trace when open.
- The whole campaign (play pass 33, 2026-11-06, `GALAXY_RESUME_SELFTEST=1500`): 78/78 playthroughs
  pass; 77 report `RESUME SELFTEST equal`, none different (world 001's script ends at tick 500).
  Implemented with the Codex CLI to the reviewer's design brief; reviewed, built and tested here.
  The follow-up (format v2: the world file's hash; an executable-based build id) again: play pass 34,
  78/78, 77 replays equal.
- The golden frames and trace were re-baselined with it: the per-tick step is now the fixed 1/60 s
  from the first Update on (the autoplay's), where the very first Update of the process used to
  carry an elapsed time of 0, so the scenario's first walking tick no longer stood still; on it a
  fresh Blupi is not yet grounded and moves at the airborne 8 px (as on every mission start in
  play and autoplay already). The jump still starts at trace tick 40.
- Follow-up (2026-11-06): format v2 adds the mission path and 64-bit FNV-1a world hash. The host
  captures file bytes once, hashes and decodes that snapshot, and retains the identity through
  mission start. Continue validates the current bytes before changing settings/state and loads
  that same validated snapshot; a missing, changed or differently pathed world deletes the log
  and falls back to the checkpoint. Version 1 is refused, including checksum-valid files.
  The build id combines the old compile-time string with a lazy, process-cached executable hash:
  `/proc/self/exe` on Linux, executable paths on Windows/macOS; unreadable executables and
  Android/Emscripten retain the compile-time fallback. No executable hashing occurs per frame.
  `VerifyAutoplay` covers v2 path/hash round trip, v1 rejection, world mismatches and binary FNV
  vectors/file streaming. Build and **112/112 CTest pass**; 012 and 045 again report
  `RESUME SELFTEST equal` and pass after 3296 and 16815 ticks. The silent scratch-world probe
  changes its copy after load but before deactivation: the log retains the original hash,
  Continue refuses it, deletes it and loads checkpoint mission 12, then exits with code 0.
  A copied executable with an inert appended trailer (same compile-time timestamp) produces a
  different build hash, refuses the old log and also loads the checkpoint and exits cleanly.
  Shipped worlds are untouched. Evidence: `build-probe/bd0077-world-refusal.py`,
  `build-probe/bd0077-*.log`, `build-probe/bd0077-refusal-run/`.
- Limits: replay requires the same build and unchanged mission assets (only the world file and
  executable are hashed, not other assets or shared libraries). Reconstruction cost grows
  with mission history; files over 128 MiB or ten million ticks are refused. Desktop keyboard/mouse
  and synthetic touch/accelerometer serialization are verified; native touch/sensor hardware,
  Android lifecycle and Emscripten persistence still need platform testing.

## BD-0078 — a fresh Blupi stood in the air for his first tick; the original starts him standing (gameplay) — FIXED_AND_VERIFIED

- Original (Decor.cpp:329, 400; `PlayPrepare`): a mission's start sets `m_blupiAir = false` with the
  start position, Stop at phase 0 and focus; a respawn puts him at `m_blupiValidPos` (6386).
- Galaxy: `BlupiController` starts with `m_onGround = false` and `SetPosition()` clears it, so on a
  mission's first tick a walk input moved him at the airborne 8 px a tick (`kMarch`'s `!m_onGround`
  case) instead of the march's first 2 px. Found 2026-11-06 by the golden trace (BD-0077).
- Fix: `BlupiController::SettleOnGround` (the move resolution with no displacement, his body's
  footprint, crates and the temporary tiles' phase; a start in the open air still falls) through
  `GalaxyEggbertGame::PlaceBlupi` at every mission start, hub arrival, editor play-test, respawn and
  Resume. Teleports and the other `SetPosition` callers keep their semantics. Implemented with the
  Codex CLI to a brief; reviewed here.
- Validation: `VerifyBlupiMovement` (the first walking tick 2 px, a respawn restarts the march, a
  mid-air placement falls); the golden trace changes only X on ticks 0-9; 112/112; play pass 35 (with the resume self-test): 78/78, 77 equal.

## BD-0079 — a lit stick of dynamite showed no spark and did not flicker (visual) — FIXED_AND_VERIFIED

- Original (Decor.cpp:8252-8256; Tables.cpp `table_dynamitef`): a lit stick (ObjectType56) shows
  `table_dynamitef[phase % 100]`, 100 entries flickering between element 252 (no spark) and 253-255
  (the fuse's spark in three shapes).
- Galaxy: icon 253 always (its comment took the table for icons 253..352), and the 3D model has no
  spark (painted on the sprites), so a lit stick looked unlit. Found 2026-11-06 by comparing every
  Tables.cpp table with Galaxy's arrays by value (105 tables: 93 found as they are, 11 ported in
  another form, this one missing).
- Fix: the table transcribed (standing approval for small animation tables); the spark cut from
  element 253-255 (`tools/rd4_to_gltf/dynamite_sparks.py`) drawn facing the camera at the fuse's tip,
  as the electrode's sparks (Q-005); none for 252. Implemented with the Codex CLI to a brief.
- Validation: `VerifyGetObjIcon` (phases 0, 1, 5, 50, 99, 100 -> 253, 252, 255, 253, 255, 253), 112/112,
  golden frames and trace, mission 014's playthrough; screenshots of the three sparks.


## BD-0080 — in play every house, tree, plant, toy and the Blupi statue let him through (collision) — FIXED_AND_VERIFIED

- Decision (M3D-0006, the owner's Q-014, MR-0007, MR-0008): the BigDecor standing in the rooms as
  their models are solid (`kSolidBigDecorIcons`: palms, trees, plants, houses, machines, toys, the
  palace pieces 87-88 and the Blupi statue 89), as large as their models.
- Galaxy since 34ddadb3 (2026-11-06 06:25): the boxes were given to Blupi in
  `RebuildWorldPresentation()` (`blupi_.SetSolidBoxes`), and `LoadMission`,
  `LoadCustomWorldForPlayTest` and the third load path then made a fresh `BlupiController`, which
  knows none. In play nothing was solid: the owner, 2026-11-06: "houses, trees and the others are
  still passable", "even the Blupi statue, which was solid, is passable now" (before 34ddadb3 the
  large ones were solid by the older `solidBigDecorCells_`). Play pass 28's 78/78 passed for that
  reason: no route met a box.
- Fix: the boxes given again after each of the three resets.
- Validation: a probe walking west from 025's spawn (50, 11, 56) into the column 87 at (44, 11, 56):
  the old binary walks into it (x 42.78 after 400 ticks), the fixed one stops at x 44.67. ctest
  112/112. The play pass then failed the routes that had walked through a plant, a toy or a machine
  (025, 031, 034, 051, 053, 054, 058, 071, 092, 094, 095 and their hubs' chains): rerouted round them
  (Codex CLI to a brief); all eleven and the chains 020, 030, 050, 070, 090 pass, no world changed.
- Left (level playability, queued for Codex in `handoff.md`): in 053 the machine (BigDecor 29 at
  (57, 30, 19)) stands in the approach to the door "14", leaving a lane 0.13 cells wide that the
  script threads with `tol 0.02` -- no lane for a human player; 058's pillar perimeter is tight
  too (`tol 0.05`). The BigDecor should move in the generators, not the routes squeeze past.


## BD-0081 — ghost killed by lightning, falling pose and opaque body — FIXED_AND_VERIFIED

Owner 2026-11-06 (OR-0075): lightning killed ghost Blupi; he should walk/stand see-through.

Reference: read-only `../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp`, MODERN guards:

| Reference | Galaxy implementation |
|---|---|
| `BlupiStep:2737-2741` returns through `BlupiGhostStep` before any tile/action checks | `BlupiController::CanInteractWithWorld`; one host block skips lava, lightning, spikes, drips, saws, crushers, springs, water, fans, world signs, teleporters, safe-position recording, switches, dynamite/Perso use and vehicle contacts. Controller free flight already skips gravity, collision, wind and conveyors. Pending skateboard actions, pollution and new power/Hide trails (3888/4009/4293/4436, 5204-5254) are suspended too. |
| `BlupiElectro:9613-9616` | `InteractionSystem::Update` suppresses the Cloud aura. |
| `MoveObjectFollow:9649-9652` | The same flag prevents dormant type 96 followers waking. |
| `MoveObjectDetect:9692-9696` | After autonomous object updates, skip the contact/pickup block; no enemy, mine, shot, wasp, creature, pickup, exit or avatar contact. Lift patrol continues without riding/conveyor displacement; crate motion already started may finish but cannot hold the ghost. |
| `MoveChargeDetect:9806-9809` | Host hand-action gate and interaction charge-pickup/fire gate. |
| `MovePersoDetect:9838-9841` | Suppress enemy-versus-Perso/avatar detection globally, as the original guard does, as well as Blupi's own Perso pickup/use. |
| `BlupiGhostStep:2659-2689` | Horizontal translation or vertical-only flight selects March; idle/turn-only selects Stop. Vertical movement preserves facing. This precedes all other animations and wind's Push/Vertigo override; model clips and sprite tables consume the same state. |
| `CheatAction:2054-2055` | Clear swimming/surface flags on entry (previously left stale in Galaxy). Existing toggle-off collision rule retained. |
| Draw `822-825`, `1160-1163` | `GetDrawOpacity` supplies 0.75 to the real model, placeholder model and first-person sprite indicator. Reuses Hide's premultiplied AlphaBlend/DepthRead; translucent models draw after opaque models, with alpha/blend/depth restored. HUD uses a separate faded sprite batch, preserving life/pickup icons. |

Exceptions deliberately preserved after checking the original beyond the brief:

- **Already-active type 97 followers still home** (`MoveObjectStepLine:8026-8064`, no Ghost guard),
  but cannot touch/hurt him. They now track his real location, instead of the former far-away
  sentinel. Hide still prevents homing.
- **Existing dynamite blasts still kill** (`DynamiteStart:9168-9174`, outside BlupiStep, no Ghost
  guard; `BlupiDead:6574` clears Ghost). Ghost does not place new dynamite. This is distinct from
  mine/bomb contact, which is suppressed. No blanket invincibility was added.
- World clocks, enemy patrols/shots and existing effects continue. `VoyageStep` is inside
  BlupiStep (`6532`), so an already-started pickup voyage pauses during Ghost.

Verification (2026-11-06):

- Full existing-tree build (`--parallel 4`), **112/112 CTest**; golden frames (all three) and trace
  byte-identical, no rebaseline. Logs: `build-probe/ghost/{build,ctest,golden-frames,golden-trace}.log`.
- `VerifyBlupiMovement`: hazard-cell bypass contract, flight/idle/vertical/turn animation, opacity,
  Hide precedence and interactions restored on toggle-off. `VerifyInteractionSystem`: 17 contact
  types, ordinary mine positive control, Cloud/Perso guards and restoration, sleeping/awake
  followers, moving conveyor lift, door/bridge gates and the dynamite exception.
  `VerifyAutoplay`: typed word presses/releases; autoplay now supports `type WORD` through the
  normal keyboard recognizer and `ghost`/`anim` expectations.
- Normal mission regressions **012, 014, 025, 051, 094: all pass** (3296, 4112, 13680,
  8357, 17738 ticks); `build-probe/ghost/regression-NNN.log`.
- `tools/autoplay/probes/ghost_lightning.txt`: mission 025, `--spawn 24,25,50,90`, types `ghost`,
  moves through the existing icon-305 column at (26,25..28,50), stops in it at active phase 4,
  waits a whole cycle, exits with three lives, checks March/Stop. Trace and captures:
  `build-probe/ghost/lightning-run/`; 635 ticks, pass.
- Clear standing/walking model captures in the existing showcase, using
  `tools/autoplay/probes/ghost_poses.txt`, mission 999, `--spawn 34,1,6,0`:
  `build-probe/ghost/poses-run/autoplay_shots/0000071.png` (Stop) and `0000109.png` (March).
  Ground pattern visibly shows through the body. First-person sprite checks:
  `build-probe/ghost/fallback-shots/ghost-sprite-{standing,walking}.png`.
- Hide followed by Ghost: typed `hidedrink`, waited for the drink, typed `ghost`, moved and
  stopped; 210 ticks, pass. `build-probe/ghost/hide-combination-run/autoplay_shots/0000209.png`
  shows the 0.75 March pose without spawning a Hide trail.

Reproduce the lightning probe (silent, isolated saves):

```bash
AUTOPLAY_RUN="$PWD/build-probe/ghost/lightning-run" tools/autoplay.sh build-cna-opengl3 \
  tools/autoplay/probes/ghost_lightning.txt 25 --spawn 24,25,50,90
```

Files changed for BD-0081: `BlupiController.{hpp,cpp}`, `InteractionSystem.{hpp,cpp}`,
`Hud.{hpp,cpp}`, `Autoplay.{hpp,cpp}` under `src/GalaxyEggbert/Game/`;
`src/GalaxyEggbertCNA/GalaxyEggbertGame.cpp`; `tools/VerifyBlupiMovement.cpp` (ghost assertions
at 2942), `tools/VerifyInteractionSystem.cpp` (ghost fixtures at 5764), `tools/VerifyAutoplay.cpp`
(typed-cheat test at 101); the two `tools/autoplay/probes/ghost_*.txt` scripts; `CURRENT.md`,
`handoff.md`, this ledger and `owner_reports.md`. Most of the host/interaction textual diff is
indentation under the new gates (`git show -w` isolates the functional change).


## BD-0082 — a lift wearing a tile other than 29 was drawn as the lift tile's bar and stem (drawing only) — FIXED_AND_VERIFIED

Owner 2026-11-06: why can a moving platform, as in Mobile Eggbert, not be any tile? Chosen: only
"drawing the lift as the tile" (rendering; collision and riding unchanged).

Reference: a lift (ObjectType1) carries its own icon, any decor tile of object-m (the original's
levels use 29 most, then 361, 387, 340, 153, 215 ...), drawn as that tile moving.

| Galaxy before | Galaxy now |
|---|---|
| `MoveObjectRecord::visualIcon` (serialized) -> `ResolveObjectVisual` already gave the lift the tile's icon; a tile with a Ray Dream model was drawn as it (`ConveyorLiftModel`, 29 = `Decor/techno29.rd4`), every other one was cut into tile 29's bar and stem UV rows (wrong rows of an unrelated tile) | `LiftVisual.hpp ChooseLiftDrawPath`: Model, BarAndStem only for tile 29 without its model, **TileCube** for any other tile: a unit cube moving with the lift, wearing the tile's faces from the atlas -- a DirectionalCube tile with the faces `ConfigureDirectionalCubeFaces` gives it in the terrain (a cut-out tile stays an opaque cube; the terrain's reliefs are not applied) |

- Verified: `VerifyTileUvBounds` (the three paths); ctest 112/112; golden frames and trace unchanged;
  probe world `build-probe/lift-tiles/` (made by `make.py`; lifts wearing 29, 361, 153, 387, 340, 215
  at x 44-54): `lifts-moving.png` -- 29 its model, the others their tile's cube, moving.
- Follow-up 2026-11-08 (owner): all 52 authored campaign worlds with general lifts now use their
  original mission's confirmed tile palette (`tools/worldkit.py ORIGINAL_LIFT_ICONS`); 829 records
  gain explicit looks, 45 different tile ids. Paths/timing/counts and every non-icon object field
  compare equal. Palace021 uses 387. Playthroughs 021/055 pass.
- The editor (owner 2026-11-07, option A): the four lifts in "Moving mechanisms" take the look of the
  block picked last (`objectVisualIconIds` -2: the selected block once one was picked, else the default
  look, the lift tile 29; the conveyor lift 47 keeps its own). `VerifyGEEditorPalette` covers the three
  cases. Before it a placed lift always got the default look.


## BD-0083 — solid BigDecor that left no lane a human could take (053's door, 058) — FIXED_AND_VERIFIED

Follows BD-0080 (houses, trees, plants, machines and the statue solid in play): two reroutes squeezed
past a BigDecor on lanes no player could take. Closed by moving the BigDecor in the generators, the
routes straight again (the way the playthroughs ran before BD-0080).

| World | Before | Now |
|---|---|---|
| 053 | ball machine 29 (box 1.7 x 1.6 cells) at (57, 30, 19) in front of the door "14" (east wall x 58, z 18-20): a lane 0.13 cells wide, `tol 0.02` | at (53, 30, 19) by the gallery's west edge: the east lane x 53.9-57.5 and the door approach are free; `go 57 19` straight |
| 058 | plant 25 (box x +-0.99, z +-0.7) in the middle of the pillar's 3 x 3 top (61, 30, 85): lanes of 0.5 cells, the circuit at `tol 0.05` | at (66, 8, 85) on the cave floor beside the pillar; the mine's diagonal and both treasures free |
| 058 | plant 23 at (54, 16, 73) in the island's north-east corner, where Blupi jumps off east (waypoints at z 74.3, `tol 0.05`) | at (50, 16, 73), the north-west corner; the jump-off at z 73.6 again |

- Found by: `build-probe/bigdecor-clearance/scan.py` (free gap beside every solid BigDecor of the
  campaign: the 0.5-0.9 gaps are wall-hugging dressing, no route) and `tight.py` (every `go ... tol <= 0.1`
  waypoint within 0.75 of a solid footprint: only 073's two lift-wait waypoints, which have free room).
- Verified: playthroughs 053, 058 (and the hub chain 050) pass; 023, 025, 063, 064, 065, 092, 102, 199
  also pass (switch change, OR-0078); ctest 112/112.
- BD-0080's "Left" lines are closed.

## BD-0084 — Blupi was invisible through the life-loss window; the original turns him on the spot (Hide) — FIXED_AND_VERIFIED

- Original: when a death's own animation ends with lives left, `m_blupiAction = BlupiAction::Hide` and
  `m_blupiIcon = -1` (Decor.cpp:6381-6382), `m_blupiFront = false`, and the icon-48 life voyage starts. BlupiStep
  runs every tick (MoveStep, Decor.cpp:513; no early return for Hide), and its `BlupiSearchIcon()` (5070) maps
  Hide to table_blupi's action 35: nine frames 276-284, looped, Blupi turning on the spot. The draw pass
  (794-842) draws `m_blupiIcon` whenever `!m_blupiFront`. Hide ends when the voyage arrives (VoyageStep,
  10255: Stop, focus back). So only the first tick is blank; the rest of the window shows him turning.
- Galaxy: the window (`IsDeathHidden()`, 2 s) kept the DeathLocked state and the host skipped the model.
  The Hide clip (9 frames, from the Ray Dream scenes) was built and never selected
  (plans/audit/animation_coverage.md item 4).
- Now: BlupiController selects `AnimState::Hide` in the window (sprite icons 276-284, the model's `Hide`
  clip, looped); the host draws the model through it. Lock, freeze, respawn and timing unchanged.
- Verified: `tools/autoplay/probes/death_hide_998.txt` (uses the new autoplay command `mortal`, which logs a
  death instead of failing): frames through the window show him turning
  (`plans/evidence/death-hide-turning.png`); `VerifyBlupiMovement` checks the state and icons; ctest
  112/112; golden frames and trace unchanged.


## BD-0085 — invisible voxel volumes killed Blupi before visible hazard contact — FIXED_AND_VERIFIED

Owner 2026-11-08: lava must touch Blupi, a saw's support cube must not kill, and a fan's motor/top
is safe while its blades are lethal. The existing 3D geometry is the contact reference: Amapi lava
height fields, Decor/scie.rd4's exposed disk and Decor/ventillo-h.rd4's motor/shaft/hub versus moving
blade bones. No new assets or mechanics. `LavaHeight`, `IsTouchingSawBlade`, `FanGeometry` and
`TryConsumeFan` replace full-voxel hazard approximations. Existing sound/death/immunity/vehicle/
focus and wind-removal rules remain. Pure tests cover height, landing, stopped/rotated saws and
all six fan directions; live probes777/778 prove safe above lava/on a fan motor and lethal actual
contact. Twelve relevant mission scripts and 113/113 CTest pass. This supersedes OR-0068's former
"any fan top kills" approximation.

Follow-up: requiring downward motion for saw contact missed walking onto the blade. The owner's
report is covered by a mortal walking regression: level-height and rising actual blade contact
are lethal too, without making the support cube lethal again.
The following report exposed passable support cubes: the original saw's empty 2D quarter mask
was still used horizontally. Registry collision masks378-383 now represent the rendered solid
support, including stopped blades. Their original mobile-transparent flag is unchanged. Mortal
X/Z walking tests prove support collision without blade contact; the blade remains above that cell.

## BD-0086 — special exits skipped Blupi's Win cheer — FIXED_AND_VERIFIED

The ordinary campaign used Decor.cpp:6411's 40ticks at20Hz, but 199/999/editor play-tests immediately
showed Win. All exits now finish that same duration before choosing their destination. Clip time
uses the simulation clock as action timers do. Showcase QA is still Play after90cheer ticks and
then reaches Win; the complete199 ending passes. No arbitrary duration extension.

## BD-0087 — model overlap recovery could push a vehicle through the floor — GUARDED_AND_TESTED

Owner's rare Jeep/palm impact (2026-11-08) could not be reproduced. Inspection found that the
down-first penetration recovery could lower the feet into solid ground when a BigDecor caught
the head. Only model overlaps now try clear boundaries along both horizontal axes at the same
height; a partly buried pose can recover onto its full solid floor. Ordinary tile and ceiling
recovery stays unchanged. Synthetic head-overlap tests cover Blupi, Jeep, tank and helicopter,
plus an already half-buried Jeep. All 113 tests and complete missions021/199 pass after the change.

## BD-0088 — editor lift endpoints and rear marker faces — FIXED_AND_VERIFIED

The reference editor draws icon30 (1) at a lift's start and icon31 (2) at its end
(`blupi_recon/src/decblock.cpp`, BUILD). Galaxy now draws these existing atlas tiles on numbered
open cubes in the 3D editor. The shared geometry draws each painted wall from both sides;
alpha-test depth writes preserve visible rear faces through the holes, including terrain tiles30/31.
Marker and winding tests pass; live editor captures show endpoints and interior/rear walls.

## BD-0089 — interrupted drinking permanently removed a reusable bottle — FIXED_AND_VERIFIED

Mobile Decor.cpp:6053-6068 hides the bottle at contact and3224-3235 restores it at the36-tick
completion. Galaxy only restored on that completion event, which glue/death cancels. The owner
requested that the bottle remain reusable after interruption too. `ReusablePickup` now returns
the item when the action finishes or is interrupted, independently of the deferred power grant;
unfinished drinking grants no Hide. Clearing the pending return prevents duplicates. The same
handling covers Sucette/Charge and a death on the initial contact frame. Tests use a real enemy
glue projectile while focus is off, restore the original bottle once, take it again and complete
the second drink normally.

## BD-0090 — lift positions were frozen in the 3D editor — FIXED_AND_VERIFIED

`blupi_recon/src/decblock.cpp` MoveStep calls MoveObjectStep before checking PLAY/BUILD (line1114);
BUILD therefore previews moving platforms too. Galaxy previously advanced only visual animation
clocks. The editor now advances active lift families1/47/48 through the same AdvancePatrolStep
routine as Play, including XYZ interpolation, dwell and return. It changes only runtime positions,
leaving authored endpoints, numbered markers and world save data intact. Tests compare complete
cycles against Play and verify that unrelated objects are neither moved nor collected.

## BD-0091 — owner-requested fairplay cheat-entry lock — FIXED_AND_VERIFIED

`fairplay` clears persistent cheat toggles and blocks other cheat words, F12 and the hidden menu,
while remaining available to unlock. It retains earlier rewards/world changes and does not
automatically reactivate toggles on unlock. InputPad owns the lock; host dispatch is guarded too.
Resume format5 records its initial value. All28-word parser/lock tests pass; CMake OFF input tests
confirm this word cannot bypass a compiled-out cheat system. Live toggle test retains resources
and reconstructs exactly with RESUME SELFTEST equal while locked.

## BD-0092 — givecopter could leave GhostStep overriding the helicopter — FIXED_AND_VERIFIED

The owner reported camera movement without a usable helicopter, then reported it working again.
Standalone givecopter passed; ghost→givecopter reproduced a helicopter flag with GhostStep still
controlling motion and pose. Vehicle cheats now replace Ghost (a Galaxy cheat-state correction
requested by the owner), without clearing independent megablupi. Normal Ghost toggling retains
its original blocked-position refusal; forced replacement can return to the last valid position.
Controller regression and live reproduction verify that the resulting helicopter really climbs.

## BD-0093 — physical Left Ctrl was remapped away from helicopter climb — FIXED_AND_VERIFIED

The earlier request added Ctrl fire to both armed vehicles. The owner clarified that helicopter
Left Ctrl must climb. The physical Ctrl→F remap now applies only in the tank. Helicopter F still
drops glue, Ctrl/RightShift rise, LeftShift descends; gamepad A/LB and separate RT fire are unchanged.

## BD-0094 — editor endpoint markers showed through opaque terrain — FIXED_AND_VERIFIED

The host disabled depth testing before the editor overlay pass. Setting DepthStencilState::Default
inside LiftEndpointRenderer did not restore CNA's independent explicit depth switch. The renderer
now enables that switch for the marker draw and disables it again for subsequent screen controls.
A scene enclosing both endpoints in solid terrain matches its no-marker baseline exactly; an
exposed-marker capture retains both numbers and the double-sided interior walls. No global culling
or terrain-rendering change. Evidence: build-probe/marker-occlusion/.

## BD-0095 — explicit aligned compass indicators — FIXED_AND_VERIFIED

Owner 2026-11-08: the second row shows `|` when Blupi faces the objective and the third `-`
when on its height. Existing shortest-turn and vertical arrows,3-degree/0.1-block tolerances,
rounding and objective selection remain. Unit checks cover wrapped heading and vertical-only
objectives; the live probe785 shows6 / | / -. All113 CTest pass.

## BD-0096 — tank glue was restricted to eight headings — FIXED_AND_VERIFIED

Owner 2026-11-08: tank shots should follow Blupi's actual3D heading. InteractionSystem now accepts
optional continuous fire yaw; legacy callers retain their tile-facing fallback. A shared DDA
extracted from the verified editor raycast finds the first solid/temporary blocker along that ray,
retaining cardinal half-cell stand-off,5ticks/block speed, ammo and firing cadence. Heli/enemy
shots are unchanged. Eight yaw tests include17°/73°, and cardinal/oblique wall tests plus the
editor verifier and all113 CTest pass. Live shots: build-probe/latest-controls/tank-run/.

## BD-0097 — helicopter motor survived leaving its world — FIXED_AND_VERIFIED

Original Game1.cpp:1023 calls Decor::StopSound on phase changes. Galaxy only updated motor loops
inside Play, so menus retained the last loop; custom play-test loads also missed the normal input
reset. StopVehicleMotorSound stops channels15-18/28-31 and clears activeMotorLoop_ when leaving
Play or replacing a custom play-test world. Continue restarts the current vehicle. Music/UI sounds
are unaffected. A sound-enabled live run reached Pause, Init and mission1 after a helicopter flight;
native channel15/16/18 states were Stopped (2), with no later motor starts. Temporary state logging
was removed. Evidence: build-probe/latest-controls/motor-menu-shots/. All113 CTest pass.

## BD-0098 — rapid givecopter typing was lost between frames — FIXED_AND_VERIFIED

Owner 2026-11-08: fresh launch → Editor → second world → Play → givecopter did nothing. On an
isolated read-only copy of gamerC/custom_002.vwr,150ms presses worked and25ms presses failed.
Keyboard snapshots miss a down/up between updates; multiple new letters were also processed in
alphabetical order. Non-touch Play now starts CNA TextInputEXT and queues ASCII letters in event
order, draining one/tick independently of held gameplay controls. Polling remains for touch and
autoplay; event mode never also feeds snapshot letters. Pending text clears on mode/world/focus
transitions. All28 words, repeated E, simultaneous snapshots, fairplay and OFF guards are covered.
Resume format6 records the event letter/mode. Live rapid typing now mounts/climbs in the same
editor world, and a final live text-event resume reports SELFTEST equal. Final113/113 CTest pass;
player source-world SHA256 is unchanged. Evidence: build-probe/latest-controls/.

## BD-0099 — cheat capture depended on snapshots or IME delivery — FIXED_AND_VERIFIED

The owner again reported givecopter failure in editor play and palace hub020, with help/compass
working and turning sometimes restoring it. The previous text-only path swallowed input when
native text mode was active but delivered no characters (reproduced with an SDL event fixture).
A temporary first-source snapshot fallback passed that case but still cannot retain short presses
between frames. With explicit owner approval, CNA594eb28dc adds optional Keyboard::KeyDownEXT:
ordered, non-repeat physical presses, preserving Keyboard.GetState and existing text dispatch.
Galaxy uses one queued source for typed cheats on all devices, including keyboard+touch, without
starting an IME/OSK. Autoplay retains snapshots; resume6 captures the event letter per tick.
CNA's input suite passes five shuffled iterations (653pass/1harness skip each). A live hub020 run
with25ms presses,180ms frames and all text events dropped mounts and climbs without turning.
The owner's exact intermittent desktop event stream was not captured; no cause was inferred solely
from the turning observation. Evidence: build-probe/cheat-source/.

## BD-0100 — large green decor remained passable — FIXED_AND_VERIFIED

Owner 2026-11-08, screenshot13-22-15: make the large green forms impassable. These are explo
BigDecor66-69 (recovered columns/vase), previously explicitly excluded from solid decor. They
now join the authoritative registry's solid set, activating the existing model-fit extraction and
collision path in campaign/custom play. Smaller plants23-25 were already included. The level
checker now reads the same registry constants rather than duplicating the classification.
A VWR metadata/shape regression covers all4 at actual shape height. Live probe786 walks through
before (z37.91) and stops at the model after (z46.91); the screenshot matches the owner's model.
Rebuilt native targets and113/113 CTest pass. Complete world199, which uses these forms in its
court and cave, still reaches Win and passes40259ticks. Evidence: build-probe/green-decor/.

## BD-0126 — the game in nine languages, and a language picker of flags (owner, Q-023) — FIXED_AND_VERIFIED

- Original: Mobile Eggbert shows its texts in French or English, from the system's language
  (MyResource::Init: "fr" French, anything else English); there is no choice in the game. Its
  Text::table_char draws the 15 accented letters of its sheet (glyphs 15-29), but GetOffset hands it
  the glyph instead of the character, so they came out as glyph 1: an evident defect, not
  reproduced.
- Galaxy before: English only; every byte above 127 drew glyph 1.
- Now: every text the player sees in English or French (Localization.hpp; the French of
  TranslationsFr.cpp, Mobile Eggbert's own where it has the text; the training hints' French in
  their table). The language is the system's (CNA's UI culture) as in the original, or the Setup's
  **Language** button (top right, a Galaxy addition): the language's own name on it, a press the
  next language, saved as `language=`. Text is UTF-8: the 15 accented letters as their glyphs,
  capitals without their accent. Two spelling mistakes of the original's French hints corrected
  ("Prends", "décolle"; Q-023 b). The cheat list, the debug panels, the logs and a demo's title in
  its file stay English.
- Validation: VerifyLocalization (glyphs, UTF-8 edge cases, every marked literal translated, no
  translation left over, placeholders kept); CTest 130/130; live in French: the menu, the Setup and
  its Language both ways, the help, the demo positions, the network game's list, the editor, the
  galaxy, the goodbye, mission 11's first hint, the pause (`build-probe/l10n/`).
- **2026-11-12, German and Czech (the owner's Q-023 answers):** two more languages, translated for
  Galaxy (TranslationsDe.cpp, TranslationsCs.cpp, the training hints' columns; German without the
  sharp s, written "ss"); the Language button goes English, Français, Deutsch, Čeština. text.png and
  its 2x sheet have eight more rows (glyphs 128-170, `tools/make_text_glyphs.py`: the letters Czech,
  German and French need, composed from the sheet's own strokes; the original's eight rows
  unchanged); a capital with a mark is drawn a few pixels shorter so that the mark fits its cell, in
  every language, French's É À Ç included (they were drawn without their accent before). Czech's
  plurals (2-4, 5 and more): `TrCount`. The network game carries names and chat in UTF-8 (protocol 4,
  "Hráč A"); CNA's local profile gets the name without its marks ("Hrac A", `Net::AsciiText`).
  Validation: VerifyLocalization (391 texts in each language, every character drawable, Czech's
  plural forms), VerifyNetProtocol (a UTF-8 name round trip), CTest 131/131; live in German and Czech:
  the menu, the Setup, the help, the demo positions, the network game's list and nickname panel, the
  editor, its tools and keys, the galaxy, the goodbye, mission 11's hint, the pause.
- **2026-11-12, the language's flag in the main menu (the owner):** right of Help, a blank face with
  the flag of the language the game speaks (Union Flag, France, Germany, Czechia; Content/icons/flags.png
  drawn by `tools/make_flags.py` from the flags' geometry). A press opens the Setup, whose Language
  button pulses for its first 3 s (`InputPad::SpotlightSetupLanguage`); the gear opens it without.
  Checked live: the flag, the pulse, the flag changing with the language.
- **2026-11-12, five more languages and a picker of flags (the owner):** Slovak, Spanish, Italian, Polish
  and Brazilian Portuguese (TranslationsSk/Es/It/Pl/Pt.cpp; the training hints now a column a language,
  `std::array` by `LanguageIndex`). text.png's glyphs 171-205 (ĺ ľ ŕ, ñ ¿ ¡, ì ò, ą ć ę ł ń ś ź ż, ã õ,
  their capitals, °; º drawn as °); Slovak and Polish plurals in `TrCount`. The menu's flag and the
  Setup's Language button open a picker (`InputPad::DrawLanguagePicker`): the nine flags with their
  languages' own names, the current one lit, Back; mouse, touch, the arrows and a gamepad (FocusNavigator);
  a flag chooses its language and closes it (the flag's pulsing Setup of the morning gone). Flags drawn
  by `tools/make_flags.py` (Slovakia's arms simplified, Spain's civil flag, Brazil without its stars).
  Validation: VerifyLocalization (392 texts in each of eight languages, every character drawable, the
  plurals, the spaces at the ends), CTest; live: the picker over the menu and the Setup, its focus, a
  choice, Escape; every checked screen in the five new languages.

## BD-0125 — a 1.5x game speed on F4 (owner request; on F9 in 0.1.2) — FIXED_AND_VERIFIED

- Owner (2026-11-11): a new key for a 1.5x speed. Not in the original (F5 1x, F6 2x, F7 4x and F8 8x
  with the quick cheat, Tab slow): a Galaxy addition.
- F4 sets `GameSpeed::OneAndHalf` (raw 0x20, a value of its own for the demo files, SpeedFactor 1.5);
  F5, F6 and Tab leave it as before. No cheat needed (under F6's 2x); the HUD shows "1.5x"; the help
  (F1) says "F5 / F4 / F6: speed 1x / 1.5x / 2x". Like the other speed keys it is left out of demo
  recordings.
- 0.1.2 had it on F9, wrongly called free: CNA's Game::PollEvents takes F9 and F10 itself, its debug
  context loss and restore, in every build (UB-013; the owner found it). On the web F9 left the game
  without a context until F10. 0.1.3 moves it to F4 (F2 and F4 are free; Alt+F4 needs Alt).
- Validation: static_asserts in GameSpeed.hpp (1.5, not beyond fast, not slow, Tab from it to 0.5);
  live in mission 11 (on F9 then, F4 since): "1.5x", F6 "2x", "1.5x" again, F5 none; CTest 134/134.

## BD-0124 — a new editor world had no Blupi (editor) — FIXED_AND_VERIFIED

- Owner (2026-11-11): the 3D editor's new, empty world (nine rock-pile cubes, a chest and the red exit
  arrow) has no yellow Blupi, which Speedy Blupi's 1998 editor's new world has.
- Original: Decor::InitDecor (Decor.cpp:263-323) fills a new world with a platform (icons 40, 38,
  39), the chest (ObjectType5) in its middle, the exit (ObjectType7) at its right end and Blupi's
  start at its left end facing right (m_blupiStartPos 66, m_blupiStartDir Right).
- Galaxy before: CreateEditorStarterWorld placed the 3x3 rock pile, the chest at (50,1,50) and the
  exit at (49,1,50), and no player start: the editor showed no Blupi and the play test used the
  default spawn.
- Fix: the yellow start (SetPlayerStart, colour 0) at (51,1,50), the board's other edge, one quarter
  turn (facing -X, the chest and the exit): Blupi, chest and exit in a row as in the original. The
  galaxy's mission starter (CreateGalaxyStarterWorld) has it too (it already spawned Blupi there).
- Validation: VerifyGEEditorStorage (the start at (51,1,50) facing -X, no other colour's); live: a new
  world in slot 3 shows "Yellow start X:51 Y:1 Z:50" and the yellow Blupi on the board facing the
  chest; CTest 134/134.

## BD-0123 — Nicknames for players A, B and C (owner request) — FIXED_AND_VERIFIED

- Owner (2026-11-11): in the settings, players A, B and C get a nickname besides their letter, shown
  for example in the network game. Not in the original (its players are A, B and C only): a Galaxy
  addition.
- The main Setup (from the menu) has **Nickname** in its top-left corner (the menu's Fullscreen face):
  a panel over the Setup, as the network game's lines, with the keyboard on the screen (arrows and a
  gamepad's d-pad and A too) and the real keyboard; OK or Enter keeps it, Cancel or Escape leaves it.
  At most 15 characters (the network game's name), printable ASCII, no spaces at either end; empty
  is none.
- Shown: the menu's player row ("Ann" in place of "Player A"), the Setup's Erase progress row ("Ann :"),
  and the network game's name (the lobby, over the Blupi, the chat, the results). Saved as
  `gamerN.nickname=` in savedata.txt (only when set); Erase progress (SaveData::Reset) keeps it.
- Validation: VerifyGESaveData (round trip, cleaning, Player B by default, kept by Reset); live: typed
  in the Setup, shown in the Setup and the menu; CTest 134/134.

## BD-0122 — channel 10 (small explosion) cut and restarted instead of overlapping (gameplay) — FIXED_AND_VERIFIED

- Report (2026-11-11, a Discord player on 0.1.0, relayed by the owner and the cna session): "every
  sound is played several times without limitation, one sound many times per split second".
- Original: Sound::PlayImage (mobile-eggbert Sound.cpp:232-256) starts every channel-10 call as a new
  play beside those still sounding (a list it prunes of finished plays at 10); other channels are not
  started again while they sound.
- Galaxy before: Sound::PlayChannel kept one instance per channel and, for channel 10, stopped it and
  played it again from the start (its comment claimed the original did so): explosions in quick
  succession cut each other's attack and stuttered.
- Fix: channel 10 keeps a list of instances; finished ones are dropped, a new one is started per
  call; with 10 still sounding the oldest stops (the original's list grew past 10). Stop/StopAll stop
  them all.
- Validation: mission 014's autoplay (dynamite) passes with GALAXY_SOUND_LOG=1, its two explosions
  started as separate plays; CTest 134/134.

## BD-0121 — hazards caught a dying Blupi again every tick: the death's sound, voyage and effects repeated (gameplay) — FIXED_AND_VERIFIED

- Report: as BD-0122 (the player hears sounds "played several times", and the game lags).
- Original: Decor::BlupiStep skips the whole hazard block (lava, traps, drips, saws, lightning,
  crushers, teleporters, switches) while m_blupiAction is a Clear* (Decor.cpp:5494-5495), and
  m_blupiFocus, which the traps, drips, saws and the large creature need, is false while dying.
- Galaxy before: UpdateTickActions ran the hazards every tick of the death lock, which leaves Blupi in
  the hazard. `triggerDeath` played the death's channel before TriggerDeathLock refused the second
  death, so the channel was asked for every tick and started again each time it ended; lava and the
  fall began the soul's voyage again (channel 74, its animation reset), the trap spawned a tentacle
  (ObjectType53) and the saw its burst (3 ObjectType41 and channel 75) every tick: some 1000 effects
  by the end of a saw's 5.5 s lock. The large creature (ObjectType54) caught him again every tick
  (channel 51 or 10 and a tentacle), its gate being BlupiFocused() (which counts the air).
- Fix: the fall, lava, trap, drip, lightning, crusher and saw checks need `!dying` (IsDeathLocked()
  or IsDeathHidden()); the creature's contact needs `!blupiOutOfPlay` (dying, gone, Bye, Win).
- Validation: `tools/verify_death_sounds.sh` (CTest VerifyDeathSounds): Blupi put over lava, a saw
  and a trap of world 999 dies once and is back with 2 lives, the death's channel asked for once
  (before: 27, 331 and 301 times); the demos replay equal (VerifyDemoReplays), golden trace and
  frames unchanged. Live: the lava death starts channels 8 and 74 once each (before 2 and 3), the
  saw's 75 once (before 4).
- Not done (the terrain part done 2026-11-12, OR-0074: rebuilt by chunks): the lag the report also names has other parts too: a full terrain rebuild on the main
  thread when dynamite cuts a saw or drip (OR-0074), then the fixed-step catch-up firing several
  ticks' sounds at once, and on the web the audio running on the main thread between frames.

## BD-0120 — The network game (owner request, Speedy Blupi 2's) — IMPLEMENTED on branch `multiplayer`

Owner (2026-11-10, owner_questions.md Q-019): multiplayer as Speedy Blupi 2 / Speedy Eggbert 2
plays it (../blupi_recon reconstructs it), though Mobile Eggbert has none; LAN, Linux and Android;
the original's evident defects not reproduced. Kind: gameplay (an approved addition).

As the original (games/sb2/src): 2-4 players, the lobby of four places (colour cycling, need not
differ; ready; the host's Start when all present are ready, two at least; the host's mission);
the chat in the lobby ("<name> text", six lines, event.cpp:4554); ten lives and each at his team's
start (decblock.cpp:1349); crates, goals and enemy Blupis taken out (decblock.cpp:1430); each
team's personal bombs (his own taken back and laid, another's lethal, decblupi.cpp:2379, 3721);
a death restarts at the last safe place (decblupi.cpp:4078); PK_DIE/LOST/ABANDON with the
original's notices (decmove.cpp:202-232; sb2-eggbert-en.rc 257-261); the last one present wins
(decblupi.cpp:4097, text 287); a pause stops everyone (PK_PAUSE); remote Blupis only drawn, no
collision between players; world changes as the original's MODIFDECOR/OBJECTSTART/OBJECTDELETE
(blocks, pickups, bombs, vehicles taken and left, dynamite, tank and helicopter shots); cheats and
speed per machine, as the original (not gated in a network game).

Differences:
- Transport: CNA SystemLink (ENet over UDP, LAN discovery) in place of DirectPlay; a content hash
  (the multiplayer worlds, the Blupi model's clips) in the session's properties, so only games of
  the same content are listed (blupi_recon's port does the same).
- Not reproduced (the original's defects, blupi_recon MULTIPLAYER_ASSESSMENT): the 32768-tick
  clock wrap; events of an old packet applied unordered (events are reliable and ordered here);
  the full event queue silently dropping events; the cloud-stop's slot/team mix-up.
- The host leaving ends the game for everyone (the owner's choice; the original never handled
  its DirectPlay host migration).
- The original closes the network game as the win/lost screen opens; here the player leaves that
  screen back to the same game's lobby (a rematch, Galaxy's: below), so a host knocked out does not
  end the others' match.
- A remote Blupi is drawn from his machine's model transform, clip and time (20 a second, moved
  between the last two), not from a sprite icon; his team's colour by recolouring the yellow
  materials (the editor's starts' RecolorPlayerStartPixel); each team's bombs likewise.
- Galaxy's own screens: the Multiplayer button in the menu's left panel, the games' list and the
  lobby as panels with the original's texts, "(host)" and "(you)" marks and a status line.
- Mobile Eggbert has no network game: the twelve multiplayer worlds 201-212 (206-212 since
  2026-11-11, Q-021) are Galaxy's 3D halls with SB2's missions' identity (plans/briefs/
  world201-212.md). SB2's regions 14, 17 and 23 are not among Mobile Eggbert's backgrounds: 206
  takes 29, 210 takes 6 and 211 takes 25 (no new assets copied).

Since 2026-11-11 (the owner's Q-021), as the original too:
- The others' sounds (PK_PLAYSOUND/PK_STOPSOUND, decnet.cpp:172-193): each sound his Blupi's step
  plays is sent with its place and heard by the others within the original's reach (Sound::PlayAt).
  Only the step's own (the original also sent the world's objects' sounds, which every machine plays
  itself: heard twice there); a channel asked for every frame is sent once while it plays.
- The host's lifts (PK_ASCENSEURSTART/END, decmove.cpp:571-630): at the start of each stop at an end
  the host sends the lift's index; the others put theirs so. The original copied the step to every
  lift of the same shape as well; here the host sends each of its own.
- The others' shield, power and cloud drawn round them, a hidden one (the drink) not drawn at all
  (m_netBlupiSec, decblock.cpp:1696-1718); a cloud electrocutes the Blupi it meets (NetPlayerCollide's
  reach, decblupi.cpp:3838-3893: not swimming, surfing, shielded, a cloud, hidden or Super Blupi,
  focused), who drops his vehicle, and the cloud is gone (PK_STOPCLOUD). Galaxy's Blupi model has no
  electrocution clip (ACTION 57): he dies as by lightning (Clear1, charred) with SOUND_ELECTRO (59).
  The original took only the first player met, cloud or not; here any cloud met.
Galaxy's additions (the owner, Q-021):
- The other players' names over their heads, in their colours, through walls, up to 30 cells.
- A rematch: the win or lost screen goes back to the game's lobby; the host shows who still plays
  and starts again once all are back and ready (XNA's EndGame lists the game again meanwhile).
- The match's results: each machine counts the deaths, who killed whom (a shot's or a laid bomb's
  maker, a world bomb's colour when one player has it, a cloud) and who went out when (Lost, left,
  silent); shown on the win and lost screens and beside each player in the lobby. PK_LOST carries
  the last death's killer.
- A player from whom nothing came for 6 s (20 s before his first pose) is gone, as if he had left;
  the host so, the game ends ("the host does not answer"). Poses go at least four times a second,
  paused too.
- A host's address (name or IPv4, ":port") searched besides the broadcast, kept in the user folder's
  multiplayer.txt (CNA's SystemLinkDiscovery); the host's game on UDP 61191 (another when busy), so
  a router can forward it with 61190. The original's DirectPlay TCP/IP asked for an address too.
Galaxy's additions (the owner, Q-022; none in SB2):
- The match's settings in the lobby: the host's lives (the original's ten, or 1-9) and a time limit
  (none, the original's; 3, 5, 10, 15, 20 or 30 minutes). The clock stops while anyone pauses; the
  host's decides: when it runs out, those still in rank by the lives they have left, then their
  kills, then the fewer deaths (the same rank shared: "You share the first place"), those out after
  them, the last out first (TimeUp). A host already out stays on his results screen until the time
  is up (the ranks are his to send).
- Messages during the match: who got whom ("Ann got Ben !", "Ann got you !", "You got Ben !") beside
  the original's notices, and a chat line typed over the play (Enter, Enter sends, Escape gives it
  up; the game goes on and Blupi stands meanwhile; the others see it as a notice), or Pause's
  **Message** (where a shipped world's "Original" stands; touch screens) on the screen's keyboard.
- A spectator: knocked out, left / right (the d-pad, a touch at the screen's left or right edge) on
  the Lost screen watch the others still playing from behind them (their name and lives at the
  top); Space goes back to the results, and so does the match being decided. His world goes on
  (the others' changes, the lifts, the shots), his own Blupi not drawn, nothing of his sent.
- A world of the host's editor: after the multiplayer worlds his `<` `>` go through his editor's
  slots ("My world n"); its bytes go to the others in pieces (WorldData, 1 KB each, its hash
  checked), who are ready once they have it whole. A team without its start in it starts at the
  yellow one (they stand together then). SB2 had no map transfer (both had to have the map).
- A player whose game dropped (silence, a crash, the game closed) comes back: his game, started
  again, finds the game in progress (the session allows joining in progress) and the host takes him
  at his place by his name, with his colour, the lives he had left, the match's statistics and
  clock, and the world's changes since the start (Snapshot: the blocks changed, the world's objects
  gone, the spawned ones with their makers' numbers; his new objects numbered past his old ones);
  the others see him back ("Cid is back !"). Anyone else asking during a match is not taken; one
  knocked out cannot come back. The original never let anyone join a game in progress.
- Internet games without a router to set up: with a gamer services server (libcna's
  cna-gamer-services-server, its address in **Server**), the players sign in with their accounts
  (CNA's Guide), their gamertags their names, and the games are PlayerMatch sessions through the
  server's relay. The local network's games stay as they were (one button switches). Whoever runs
  the server and its accounts is the owner's to decide (owner_questions Q-022).
- Windows: a MinGW cross-build (OPENGL33), tried under wine: its games and Linux's play together.
- Speedy Blupi 1's network worlds 101-110 are not added: they are SB2's 201-210 (the decor and the
  objects the same in eight, a few cells or objects in 103, 108 and 210), already here. In their
  place five more of Galaxy's own design (the owner's choice), 213-217, each of a world of Mobile
  Eggbert's that SB2's twelve do not use:
  - 213, the cheese cave (world 4);
  - 214, the slime pit with its spike traps (world 6);
  - 215, the sky works with the vanishing saucers (world 8);
  - 216, the station's brick hall with a moat (world 5);
  - 217, floating islands over a meadow (the title scene).
  Briefs in plans/briefs/world213-217.md.
Fixed on the way: a death on a world's bomb of another team counted no kill (the bomb's slot was
taken by its pop at once); the killing object's type is kept now.

Validation: VerifyNetProtocol (messages and their refusal of damaged packets, the lobby, the
rules, a two-machine replication round); FuzzNetProtocol and tools/fuzz_net_sanitized.sh (Q-022:
random and damaged packets, ASan/UBSan clean; the decoder takes only exact messages, the
replication keeps the world in bounds); tools/net_players.sh (Q-022: two to four instances) with
three players (a pause holds everyone, one leaves, the others play on), four players (kills and
the notices, a chat line), the time limit (ranks by lives then kills), the spectator, an editor's
world sent, a player dropping and coming back (the three worlds hash the same at the end), a
ten-minute match of four at random whose four worlds hash the same at its end, Windows builds
under wine with Linux ones (bomb, laid, four with two under wine), and internet games through a
local server (tools/net_internet.sh: bomb, and four with two under wine; the server's relays
counted); tools/net_two_players.sh, two instances on one machine
through the screens and their autoplay scripts in real time: bomb (another team's bombs kill, the
knocked-out one sends LOST, the other wins), laid (a bomb taken back and laid elsewhere kills the
other there), shot (a tank's shot kills the other there), vehicle (a tank taken and left
elsewhere), and since 2026-11-11 cloud (world 205: the cloud electrocutes, the cloud gone) and
rematch (NET_LIVES=0: won and lost, both back in the lobby, a second match), all PASS, and all five
again over a bad network (NET_LOSS=0.2, NET_LATENCY=150 ms: CNA's simulated loss and latency); a
frozen client (SIGSTOP) taken as gone after 6 s and the host winning, the host frozen ending the
client's game; the sounds and lifts in the logs; the address search with the machine's own address; two instances driven by the pointer under Xvfb (list, join, colour, ready,
chat, start; each sees the other in his colour). CTest 117/117 (single player unchanged:
VerifyDemoReplays). Evidence: build-probe/mp-shots/, build-probe/net-two-players/.

## BD-0119 — Each player's own galaxy (owner request) — FIXED_AND_VERIFIED

Owner (2026-11-08): each of the three players builds a world system of their own, a main hub, sub
hubs and worlds; the name was left open ("Space, or something else"): **Galaxy**. The reference is
Speedy Blupi's private worlds (Construction, per gamer: `m_bPrivate`, blupi_recon
`src/event.cpp:1968-2001`), here as a whole campaign. A gamer's galaxy is
`customworlds/gamerN/galaxy/world%03d.vwr` with the campaign's numbering: main hub 1, world hubs
10-140 (the main hub's world signs 1-14), their missions 1-8, the bonus world 199
(`CustomWorldStorage.hpp`). It is played with the campaign's own rules (signs, doors, gold bars,
golden key, win/back formulas); its progress is a second GamerSlot per gamer in SaveData
(`galaxyN.*`, written only once used), so the campaign's is untouched. Galaxy's own: the menu's
Galaxy button (left of Demo; the editor's galaxy screen while there is no main hub), the editor's
galaxy screen (world screen's Galaxy button), starters with their signs, main-hub sign 13 being
world 13's (no showcase), a galaxy world's play-test following its signs in the galaxy, deleting
a place from the galaxy screen (a bin, two clicks, into `galaxy/backups/`; Backups brings it back;
2026-11-09), `--galaxy` for the QA autoplay. Since 2026-11-09 a galaxy mission writes the resume
file as the campaign's does (focus lost in play); Continue goes back into the galaxy the log names
(`BeginResume()`). Since 2026-11-09 a whole galaxy is shared as one `.galaxy` file (the galaxy
screen's Export and Import; an import keeps the old places in their backups and starts the
galaxy's progress over, a pack carries its demos and its name) and the menu's player panel shows
its gates under its name. No change to the campaign.
Validation:
`VerifyGEEditorStorage`, `VerifyGESaveData`, `VerifyInteractionSystem`, `VerifyGEInputPad`, live
(handoff.md "Recordings, ten demos, xnjttjpo, galaxies" and "Galaxy: delete and Continue"), the
resume self-test in a galaxy mission, the sharing on the desktop and the web (handoff.md "Sharing
galaxies and demos"). Evidence: `build-probe/demo-live/galaxy/`, `galaxy-delete/`,
`galaxy-resume/`, `galaxy-resume-selftest/`, `galaxy-share-sheet.png`, `web/`.

## BD-0118 — xnjttjpo: the 3D editor on the world being played (owner request) — FIXED_AND_VERIFIED

Owner (2026-11-08): editing the existing worlds in the 3D editor, "a cheat, xnjttjpo as in Speedy
Blupi", typed in normal play. The original's XMISSION (`src/event.cpp:55`, typed with every letter
after the first one more, `:4228-4230`) toggles `m_bBuildOfficialMissions`: the Construction screens
then read and write the official `data\worldNNN.blp` (`:2230`, `:2321`, `:3083`). Kept: the word,
editing the shipped world file itself. Galaxy's own: no toggle -- typed in play it opens the editor
on the world being played (after that tick; not in a demo, a resume reconstruction, a recording or a
play-test), the editor's Stop goes back to the mission from its start with its lives, and the
version before each visit's first save is kept (three generations) in
`customworlds/campaign-backups/`, no thumbnail written beside the shipped worlds. A galaxy world
opens the same way, its backups beside it. Edited shipped worlds make the demos recorded in them
refuse to play (world hash) until recorded again. On the web (2026-11-09), where the shipped worlds
are the page's packed data, each save is also kept in `/save/customworlds/campaign-edits/` and laid
over them at every start, so an edit outlasts a reload; Pause's "Original" (two clicks) puts the
shipped world back, on the desktop too (the original kept in `customworlds/campaign-originals/`).
Validation: `VerifyGEInputPad` (the word),
`VerifyGEWorldEditor` (Stop, backups, no thumbnail, the web's copy), `VerifyGEEditorStorage`
(BackUpWorldTo, ApplyCampaignEdits), live, the web in headless Chrome.
Evidence: `build-probe/demo-live/xmission/`, `web/`.

## BD-0117 — Ten shipped demos (owner request) — FIXED_AND_VERIFIED

Owner (2026-11-08): ten demos, not three. The original chains `demo200`... while files exist;
Galaxy plays ten positions, `Content/demos/demo001-010.gdemo`: missions 011, 074, 105 (as BD-0115)
and 023, 033, 055, 065, 081, 112, 123, recorded from their QA autoplay scripts
(`tools/record_demo.sh`, commands in handoff.md). Validation: `VerifyDemoFiles` (ten, parsed, their
worlds' bytes), `VerifyDemoReplays` (all ten end as recorded).

## BD-0116 — The player records a demo into a position (owner request) — FIXED_AND_VERIFIED

Owner (2026-11-08): find how Speedy Blupi records demos in the game by a trick, then something more
standard: play a world so that it is saved into a demo position. The trick
(`plans/audit/speedy_blupi_demos.md`): with XMISSION on, play a world numbered 200-219 from the
Construction screens; `ChangePhase(WM_PHASE_PLAY)` starts `DemoRecStart()`, leaving Play writes
`data\demoNNN.blp` (`src/event.cpp:2094-2104`, `:3666-3710`), "REC" top left. Galaxy's own: the pause
screen's Record button (top right) shows the ten positions (shipped or recorded, with Original to
give a recorded one its shipped demo back, Cancel); one chosen starts the mission again from its
start and records it ("REC", as TX_DEMOREC), at the normal speed, F-keys and Tab left out. Leaving
Play (pause, the lost screen) or the mission (a sign, the exit), ten minutes, or closing the window
ends it; it is written as `demos/demo%03d.gdemo` beside savedata.txt (`/save/demos` on the web) and
said for four seconds; a mission change waits until it is written, so the recording ends exactly
as its replay does. The game is otherwise played as always (progress saved). Position n plays the
player's recording if there is one and it can be played, the shipped demo otherwise. Campaign and
galaxy worlds can be recorded (a galaxy recording names its galaxy file). Since 2026-11-09 the
positions' Export and Import share recordings as `.gdemo` files (`demos/shared/`; downloads and the
file picker on the web), and a galaxy recording finds its world by hash in any player's galaxy, on
either platform. Validation: `VerifyGEInputPad` (Record, positions, Export, Import),
`VerifyDemoReplay` (paths, export, import), live recordings replayed "equal" by `--verify-demo`.
Evidence: `build-probe/demo-live/record/`, `galaxy/`, `demo-share-sheet.png`, `web/`.

## BD-0115 — Main-menu demos (owner request, from Speedy Blupi) — FIXED_AND_VERIFIED

Owner (2026-11-08): demos like Speedy Blupi's automatic ones. Mobile Eggbert has none; the
reference is Speedy Blupi (blupi_recon `src/event.cpp:3666-3840` DemoRecStart/DemoPlayStart/
DemoPlayStop/DemoStep, TreatEvent `:4074-4090`, TX_DEMOPLAY "Demo" drawn at (10,10), sb1
`port/games/sb1/game/main.cpp:200-220`). Kept from it: DemoStep's idle start in Init (owner: 40 s
instead of DEF_TIME_DEMO's 1000 ticks, and reset by activity, which the original does not do),
always from the first demo, the next after each, the first after the last; a key/button stops it
and returns to Init, mouse movement is ignored; a "Demo" label. Galaxy's own: the Demo button
(the original's WM_PHASE_DEMO had no Init button), the stop input latched until released, focus
loss stopping it, and the format: the original's .BLP key-change lists suit only its 2D engine,
so Galaxy stores its resume log (seed, start state, raw input per 1/60 s tick) under a demo
header with a simulation revision and the world's hash (`DemoReplay.hpp`). The player's save,
settings, cheats and resume file are frozen during a demo. No gameplay change. Validation:
`VerifyDemoReplay`, `VerifyDemoFiles`, `VerifyDemoReplays` (in-game replays equal), the Demo
button in `VerifyGEInputPad`, live desktop checks (handoff.md "Main-menu demos"). Evidence:
`build-probe/demo-live/`.

## BD-0114 — Lava behind ghost Blupi painted over him — FIXED_AND_VERIFIED

Owner: lava looked as if it was in front of Blupi with the ghost cheat. The recovered pool
mesh samples the tile's solid body, but retained the old sprite cube's final blended pass.
Ghost/Hide Blupi does not write depth, so that later draw replaced his silhouette even when
the pool was farther from the camera. Cached lava frames now draw in DrawOpaque with normal
depth writes before models. Geometry, animation, collision and Blupi's opacity are unchanged.
`tools/verify_lava_depth.py` renders background/foreground pools with the actual game under
Xvfb, using isolated saves/worlds. It fails on the pre-fix binary and passes on the fix (2236
yellow body pixels visible over background lava;0 through foreground lava). Native build,
114/114 CTest and visual checks pass. The three ordinary golden captures are identical
before/after; both binaries differ from stored references in this environment, left unchanged.
Evidence: `build-probe/ghost-lava/`.

## BD-0113 — Start Blupis could not be selected/turned and were transparent — FIXED_AND_VERIFIED

Owner: all four coloured starts cannot be selected or rotated and should be opaque. The initial
renderer was a separate translucent marker pass, outside CurrentEditTarget and G's MoveObject
picker, with fixed model yaw. Starts now share the target, G/Tools Pick, O/Tools Turn and removal
routes, retaining colour identity and persistent selection after camera movement. Cursor/NumPad
coordinates work; palette colour disambiguates co-located starts, while object mode retains
access to an overlapping MoveObject. The selected posed model receives the same red overlay
and cell outline. Base alpha1/Opaque/DepthDefault removes the unwanted transparency.
Independent quarter turns use additive per-cell metadata8 (two bits per colour); old data
reads zero. Spawn history and moves/removal preserve each colour's heading, including shared
cells. Yellow heading applies at level entry, confirmed behaviour in Mobile Decor.cpp:388 and
SB2 decblock.cpp PlayPrepare; four horizontal directions are the 3D editor adaptation.
All114 CTest pass, including four shared starts plus a treasure, four-turn wrap, history,
reload and persistent G selection. Native shots show opaque bases, all four red selections and
independent poses; saved headings yellow1/orange2/blue3/green0 are inspected directly and
retained on reload. Evidence: build-probe/player-start-selection/.

## BD-0112 — All coloured starts aliased yellow and rendered yellow — FIXED_AND_VERIFIED

Owner rejected BD-0110: choosing orange moved yellow and still displayed yellow. The earlier
fix copied PlaceDecor10's custom-world rank0 alias, which was not the intended editor feature.
Owner explicitly confirmed four independent starts; ordinary play uses yellow. PlayerStart.hpp
now represents each colour independently, preserving yellow's header and adding non-yellow
metadata7 bits at their cells. Shared cells, per-colour moves/removal, Undo/Redo and save/load
preserve every other start. The original mesh is shown in yellow/orange/blue/green using body
colours from existing palette icons5/96/97/98; recolour preserves whites and other materials.
No multiplayer simulation added. Native tests place all four, move orange, Undo/Redo, delete and
restore it, save and reopen; all four colours and positions survive. Saved-file inspection keeps
yellow46,1,28, orange57,1,28, blue53,1,28, green56,1,28 with no MoveObjects. Native Play/debug
uses yellow46,1,28. Input/data/recolour regressions and all114 CTest pass. Evidence:
build-probe/player-start-colors/. The initial shared-start interpretation was incomplete.

## BD-0111 — Missing endpoint cubes for fish and other patrolling objects — FIXED_AND_VERIFIED

The marker builder filtered ObjectSemanticKind::Lift only. Reference decblock.cpp BUILD
draws object-m30/31 for all lift types, moving mines, bulldozers, fish, birds, wasps, creatures
and both enemy Blupis. ObjectPathMarkers now uses registry lift/timing/motion capabilities.
Fish17 and moving mine16 use special motion, so patrolMotion alone cannot identify them;
their patrolTicksPerCell does. Optional patrols get markers only with differing endpoints;
coincident fixed-route ends draw one cube. Treasure and chasing hazards remain unmarked.
No motion/timing or gameplay changes. Renderer retains double-sided faces and explicit depth
testing. All 11 reference route types and optional/static/excluded cases are tested. Native
fish placement produces cubes1/2 at50,1,28 and54,1,28; a mine route also shows vertical endpoints.
Saved fish timing/endpoints match the authored route. All 114 CTest pass.
Evidence: build-probe/editor-fixes/.

## BD-0110 — Coloured player starts placed personal mines — FIXED_AND_VERIFIED (mine mapping; start semantics superseded by BD-0112)

The Characters and goals palette mapped four Blupi buttons to real personal-mine IDs200-203.
Reference PlaceDecor10 indices1-4 edits m_blupiStartPos; custom worlds share rank0 across all
colours. All four buttons now use the existing unique world-header spawn, with separate button
highlighting. A translucent original Stop model and status XYZ mark it in the editor. Actual
personal mines and their records are preserved; no invented multiplayer or colour mechanic.
Neighbouring misread slots5/6/7 now select the source posts182/signs158/final key21 instead of
door/spawn/door-key. Tests exercise every menu cell, all four spawn buttons, no mine object,
save/load and Undo/Redo. Native shots show all four placements; the saved world has spawn51,1,32
and only fish17, moving mine16 and arrow7; Play uses the new start. All 114 CTest pass.
Evidence: build-probe/editor-fixes/.

## BD-0109 — One exit when placing arrows or final keys — FIXED_AND_VERIFIED

Owner requested a single red arrow. blupi_recon games/sb2/src/decdesign.cpp PlaceDecor10
indices 0/7 replace both TYPE_GOAL and TYPE_CLE. Editor-only placement now enforces that
rule through the registry levelExit capability; paste does too, including old exits outside
the stamp. One grouped Undo restores the old exit and overwritten object. Multiple exits
in a clipboard resolve to the last incoming exit. Generic world loading/data APIs and shipped
worlds are untouched. Regression tests cover placement, overwrite, Undo/Redo and paste; native
relocation/Undo/Redo and saved-file inspection retain exactly one arrow. All 114 CTest pass.
Evidence: build-probe/editor-fixes/.

## BD-0108 — Only the saw's upper semicircle at the cell bottom — FIXED_AND_VERIFIED

Owner explicitly rejects the full disk: remove/hide its lower half and rest the remaining
flat cut on the cell bottom. The original posed meshes are clipped at their centre (not
scaled), capped, and baked at the three original icon times. Export repeats this post-step;
source hashes/bounds are recorded. Diameter, texture and icon IDs are preserved. Runtime
selects one pose, grounds its cut and uses an upper-semicircle contact volume. The invisible
lower half and the clear space above half height are not lethal; stopped blades stay safe.
Asset bounds/caps/pose selection and movement tests pass;114/114 CTest, native front/below
captures and all15 affected mission scripts pass. Evidence: build-probe/saw-half/. The full
campaign's same baseline egg timeout26658 is unchanged. No sibling changes.

## BD-0107 — Debug panel shifted as numbers changed — FIXED_AND_VERIFIED

Owner: right-side diagnostics jumped with changing digits. DrawDebug measured the longest
current line and right-aligned the whole panel. It now has fixed width316 and fixed label/value
columns, with separate XYZ/velocity positions, secondary fields/time ticks and a fixed numeric
glyph advance. Touch stays centred between controls. Native ground/walking/heli captures keep
the same panel bounds; the325-tick toggle/fairplay script and touch layout pass. Existing114
CTest pass. Evidence: build-probe/debug-layout/. Gameplay and cheat parsing are unchanged.

## BD-0106 — NumPad selection drift after camera/terrain changes — FIXED_AND_VERIFIED

NumPad coordinates previously stayed relative to a raycast updated every frame. Flying the
camera or placing a region could change that base hit and shift the selected cell. Explicit
coordinate mode now holds the aim anchor until NumPad5/Tools Reset. Red selection and cyan
Place use one outline when coincident; Delete clears the entire extended region with one Undo.
Tests cover placement, camera movement, region removal/restoration; native camera/turn/delete
captures keep XYZ51,1,28 fixed and show one red border. All114 CTest pass. Evidence:
build-probe/editor-turn/anchored-shots/. Gameplay is unchanged.

## BD-0105 — Independent saw cell instead of an imposed floor — FIXED_AND_VERIFIED

Owner: a saw must be only a saw, independent of the block below. The renderer explicitly
invented a textured support cube and drew the blade one cell above, while collision gave
that support a full mask. Both conventions are removed for independent cells. The recovered
Decor/scie.rd4 mesh rests on its own cell bottom (offset+.425, measured Y[-.499,.433]); the
visible disk's body/swept contact uses the same offset and rotation. A stopped saw is harmless.
Original empty masks now prevent solid invisible terrain. No media assets were changed.

v2 flag bit0 marks independent data. Legacy floor-cell files are upgraded only in memory,
materializing their previous floor and moving the blade above it; occupied cells are never
overwritten. Rotation and type6 switch links survive save/reload and clipboard Undo/Redo.
World062 already puts its saws in walking layer21 over floor20: its flag alone changes, with
byte-identical generator output. This prevents an extra raised cube under its low ceiling.
All15 affected mission scripts pass unchanged, including062 and199;114/114 CTest pass.
Simulation runs use a temporary GPU-draw stub; normal rendering was checked over air, brick
and a blue panel. Evidence: build-probe/saw-independent/. The full campaign times out on
world021's egg at26658 ticks in both pre-change and final binaries; that mission passes alone.
This baseline verification failure is not claimed fixed by the saw change.

## BD-0104 — Selection/Place ambiguity, deletion and Shift descent — FIXED_AND_VERIFIED

Owner screenshot14-43-15 showed red edit tint and a different red Place cube. Selection now
has a red outline with status XYZ, Place a cyan outline. O, V, Delete/Backspace, middle button
and Tools removal share CurrentEditTarget, including explicit NumPad offsets and objects/decor
outside their palette modes. Previously removal ignored offsets and Delete required G selection.
All three kinds have removal/Undo tests; native Delete removes the intended window, Undo
restores it, with no change to the tree or ground. Shift-alone descent restored, keeping Z;
selection modifiers (including device Select 3D) suppress that descent. Unit and native checks
pass,113/113 CTest. Evidence: build-probe/editor-turn/selection-*.

## BD-0103 — O target, window orientation and visible selection — FIXED_AND_VERIFIED

Owner: O ignored a NumPad coordinate, and a house-window cube stayed facing the same way.
The first was an unconditional old voxel-ray target; the second stored a plate bit that the
single-face cube renderer never consumed. Turn and selection now share one target; explicit
offsets override the ray (including an empty coordinate), G selection persists, and house
windows use four object turns applied before flat/relief mesh construction. Rapid V/O commands
use ordered native events. The selected posed model is overlaid35% red with depth testing,
with a red cell marker for terrain and selection coordinates. Native NumPad51,1,28→O→save
visibly moves the window from its front to its side and leaves the old ground alone; a tree
shows its red model tint. Target/quarter-turn/empty-cell/Undo/command-order tests and113/113
CTest pass. Evidence: build-probe/editor-turn/. No sibling changes.

## BD-0102 — Short editor V presses lost between updates — FIXED_AND_VERIFIED

Owner reports most blocks no longer responding to V. Native snapshot polling lost complete
down/up pairs: a fixture delivered three non-repeat V presses in one event batch, leaving the
saved target unchanged. The editor now queues CNA KeyDownEXT presses and separates their
edges; the same run saves1→13, exactly three original-family steps. Physical hold does not
duplicate an event; focus/phase changes discard the queue. Autoplay keeps its snapshots.
All multi-variant terrain families and auto-tile materials accept the pulse and Undo; existing
variant tables are unchanged. Native build and113/113 CTest pass. Evidence: build-probe/editor-v/.
Specific owner block examples have not been supplied; no new variants were invented for tiles
that have none. No sibling changes.

## BD-0101 — Mobile MODERN debug diagnostics cheat — FIXED_AND_VERIFIED

Owner requested debug. The29th typed word toggles a read-only panel matching Mobile InputPad.cpp's
MODERN diagnostics fields, adapted to raw XYZ and world20Hz ticks. Velocity is measured across the
controller simulation step (blocks/s); heading/cells, animation, modes, input and speed are shown.
Desktop upper-right; touch upper-centre avoids Pause/Action. Active name/help catalogue included;
fairplay clears/blocks and compile-OFF suppresses it. Resume7 retains the initial debug flag; a
moving-state replay matches flag/velocity. Formatter tests cover clock rate,3D centre-based cells,
wrapped yaw and slow speed; all29 words/lock/round-trip tests pass. An isolated OFF verifier passes
while primary cache stays ON. Live toggle, heli, fairplay, help and touch checked;113/113 CTest pass.
Evidence: build-probe/debug-cheat/. No gameplay mutation or sibling change.
