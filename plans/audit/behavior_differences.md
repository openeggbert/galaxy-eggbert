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

## BD-0009 — pillars drawn as sprites on boxes (visual) — FIXED_AND_VERIFIED for 76, 77, 377, 399; 375/376 open

- 76 (a stone column) and 77 (the golden branched support) were sprite tiles on an inner box. Their
  original scenes: `Decor/support1.rd4` (IoU 0.92) and `Decor/support2.rd4` (0.995). Converted and
  drawn as still block models (`BlockModels.hpp` `kStillModelIcons`; the terrain leaves those cells
  out and lists them, the host draws the model at the cell); their decor role is unchanged.
- Also 377 (the barrel stack, `Decor/pile1.rd4`, 0.93) and 399 (the white column,
  `Decor/palais/big0108.rds`, 0.99), sources found by the earlier tile survey
  (`build-probe/rd4-survey/object-m_icons.csv`); live in world999.
- Open: 375/376 (turned and twisted wooden columns): no source found. `Decor/colonne1.rds` (the
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

## Reviewed, not changed (yet)

- Contact reach: Galaxy's hazard contact is a 0.7-cell sphere; the original's boxes are 28 px
  across (0.44 of a cell) with type-specific vertical bands (`MoveObjectDetect`, 9689-9767). A
  documented approximation (InteractionSystem.cpp); narrowing it changes every encounter in the
  campaign: a decision for a session of its own, with all playthroughs.
- Kills during busy actions: the original kills only with `m_blupiFocus` (or in Air/JumpAie),
  otherwise the hazard explodes and he survives (5789); Galaxy always kills. Rare; open.
- The patrol step machine spends one tick more per phase in the original (8077-8141); minor; open.
- Cosmetic: the destroyed enemy's spinning debris, the bulldozer's 90-tick electrocution
  (ObjectType38), the wasp's sting effect (90), the creature's glue splat (53), the balloon pop
  effect (91) are not drawn. Open (visual).
- Validation for BD-0010..0013: 110/110 and all 78 playthroughs pass (2026-11-01).
