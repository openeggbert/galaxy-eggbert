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
