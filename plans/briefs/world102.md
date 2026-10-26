# World 102 — mission 2 of world 10 (the three keys)

Built by `tools/worlds/world102.py` → `worlds3d/world102.vwr` (mission 102, region 7 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (K ≥ 1 the mines ridden over,
the hanging mines crouched under, the caterpillars as floors; K ≥ 2 the three doors open).

**Reference** (`worlds/world102.txt`, video 3:58:54-4:03:05): a shaft of grey cobble where Blupi
starts by the exit, three treasures beside it, each behind a coloured door (334 red, 335 green,
336 blue), and teleporters (330-333, 4:02:52) to three other works, each holding a key:
- blue machine panels (10): five skateboards (24) by a row of nine mines (the skateboard sets
  them off and rides on), a lift up from lava, five caterpillar platforms (47) carrying Blupi
  along with a gap between each, the green key (50);
- grey riveted plates (78-83): a pool where two fish (17) swim, the blue key (51) in it, a mine;
- brown rock (284, 301): seven hanging mines (3) over a low passage (crouch under them), a wasp
  (44), lava, the red key (49).

**3D design**: the atrium cut in cobble, each work floating in the sky round it, a teleporter
pair to each (Blupi walks back to the pillar he came by).
- The atrium (x 40–60, z 68–90, walk 6): spawn (50, 6, 85) facing −Z, the exit behind him; the
  three doors in its north wall, a treasure in a niche behind each; the alcoves of teleporters 330
  (machine) and 332 (rock) in its west wall, 331 (plates) in its east wall.
- The machine deck (walk 20, north-west): the arrival deck with five skateboards; a walled yard
  east of it (walls four high), a block of nine mines across it, three deep: no jump clears it,
  the skateboard rides through; a lift up out of a pit of lava to walk 28; five caterpillar
  platforms (48, carrying west: the original's 47 mirrored to bring him back over the yard), a
  gap of one between them, to the green key's platform; from it a drop back onto the deck.
- The plate hall (walk 12, north-east): a pool level with the floor (climb out anywhere), two fish
  crossing it, the blue key on its bottom, a mine by it.
- The rock cave (walk 10, south-west): the arrival cave; a low passage two wide and two high,
  seven mines hanging in Blupi's walking cells (only a crouching Blupi passes them); the cave
  beyond: three pools of lava, a wasp crossing at his height, the red key between the pools.

**Checks**: `levelcheck.py`: stage 2 reaches the three keys, the three treasures and the exit;
stage 1 stops at the doors; the real world stops at the mines and the hanging mines.
`VerifyShippedWorlds`. Live (`--mission 102`, direct view F3 at spawns in each part): the atrium's
doors, a teleporter alcove, the deck's skateboards, the yard's mines, the plate hall's pool, the
passage's hanging mines, the cave's lava and wasp. Played live: crouching through the passage
under the seven mines; riding a skateboard into the block of mines, which go off one by one as
he rides on (a probe with a board set at the yard's mouth). Played through from the spawn to hub 100
by `tools/autoplay/world102.txt` (the three teleporters there and back, the skateboard through the
mines, the lift and the caterpillars jumped from their west edges, the dive for the blue key, the
crouch under the hanging mines, the wasp's lane at its far end, the doors and the exit).
