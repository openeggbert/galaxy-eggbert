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

## BD-0003 — a crate can stand on a non-solid block (gameplay) — UNREVIEWED (suspected)

- `CrateGroupCanMove` (was `MoveCrateGroup`) counts any non-air block under the seed row as
  support (water, decor with an empty mask). `TestPushOneCaisse` (`Decor.cpp:9354-9385`) needs
  `DecorDetect` to hit a solid quarter-cell under both lower corners. Next: a test with water and
  decor under the destination; fix if it reproduces.

## BD-0004 — `IsPointSolid` excludes decor, `InteractionSystem::IsSolidAtCell` and projectile traversal do not (gameplay) — UNREVIEWED (suspected, from the campaign brief)

- Next: a follower and a projectile crossing a passable decor tile (D41's set) in a micro-world.

## BD-0005 — the asset inventory undercounted the original 3D sources (doc) — FIXED_AND_VERIFIED

- `GALAXY_EGGBERT_COMPLETION.md` §4/§7.1 said 44 `.max` and 6 `.a3d`, "not readable without
  proprietary tools". The tree holds 56 `.max` (14 stamped "3D Studio MAX: 3.1") and 105 `.a3d`,
  whose bytes start `AMAP`/`BLOKSHAP`/`blokgeom` (Amapi 3D chunks with big-endian floats: a
  reader looks feasible). Evidence: `tools/audit/asset_sources.py` (this session).
