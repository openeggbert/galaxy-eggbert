# World 064 — mission 4 of world 6 (the slime well)

Built by `tools/worlds/world064.py` → `worlds3d/world064.vwr` (mission 64, region 31 as the
original). Staged variants for `levelcheck.py` and `flightcheck.py`: `--stage K out.vwr` (K
treasures taken: door "1" open from K ≥ 1, door "4" from K ≥ 4; from K ≥ 4 also the ways the
potion, the lollipop, the bars, the shuttles' crossing and the switch open as plain floors and
steps, no enemies on them, the flooded bend a doorway).

**Reference** (`worlds/world064.txt`, video 5:48:29-5:54:11): one tall well of slime, climbed from
Blupi's start at the bottom to the red arrow at the top; ten treasures, eight eggs. A zigzag of six
small shuttles (20 ticks, 20 dwell) between two blupits (33) that shoot across it; door "1" (421)
and a long lift (90 ticks, fifteen up); a pool with two fish (17) and a treasure and an egg under
water; two treasures by a moving mine; door "4" (424); a stack of corridors with moving mines (16),
a bird (20), two eggs and striped poles; the potion (30, Hide) before a stair of four mines and a
slope of seven saws (378) taken under its protection; two shuttles (30 ticks) crossing; the
bulldozer (4) under bars (138); the lollipop (26, Power) at the foot of a climb too high without
it; three hanging mines (3), a mine in a dip, bars over moving mines; a garage of five helicopters
(13) at the foot of a hall with four treasures on and under pillars, a second garage of four; at
the top the switch (384) stopping the saws before a treasure, a flooded bend (no helicopter in
water) before another, an egg on a ledge, and the arrow in a box of blue blocks (4) with a grate
roof (73), two eggs and machine parts inside, entered from above.

**3D design**: storeys of the well, each a room or corridor of its own, joined by lifts, holes
and steps.
- The bottom (x 40–58, z 44–58, walk 5): spawn (50, 5, 54) facing −Z, treasure 1, an egg, a blupit
  shooting along the room's north edge. The spiral (x 40–45, z 38–43): six lifts (2 × 2) round a
  core, each pumping two up and down (20 ticks, 20 dwell), odd ones rising as even ones sink, so
  each is level with the next in turn; Blupi steps across and is pumped up to the landing (walk
  17). A second blupit in a niche shoots across the spiral's north side (its other lane is safe).
- Door "1" from the landing; the long lift (2 × 2, 90 ticks, fifteen up) to the pool storey.
- The pool storey (walk 32): treasures 3 and 4 on ledges two up, a moving mine sweeping the floor to
  an egg; south the pool, flush with the floor and seven deep, two fish crossing it, treasure 2 and
  an egg on its bottom. Door "4" in the east wall.
- The corridors' spiral: five corridors two wide and three high round a core, each three above the
  last (walk 32, 35, 38, 41, 44), a step and a hole at each corner; moving mines in the first and
  the last, the bird in the third, each on the outer lane; eggs in the second and fourth; poles in
  niches; the potion at the end of the fifth.
- The potion's stair: one wide, eleven steps east, a mine on each of the first four, the last seven
  saws; under Hide Blupi climbs it unharmed (the tunnel six high, room for the camera).
- The shuttle room (walk 56): two shuttles (2 × 2, 30 ticks, 3 dwell) crossing in opposite
  directions side by side, the second two higher: ride the first, jump onto the second as they
  pass, ride it back, jump to the ledge (walk 60). A pit under them with steps back up.
- The bulldozer's corridor (walk 60): bars under its ceiling to hang from while it passes; the
  lollipop at its west end, under a column rising four to the next corridor: only Power's jump
  reaches it.
- Walk 64: one wide under three hanging mines (walk under, never jump), then west two wide, a
  moving mine in one lane and a dip with a mine in the other; up to the bars' corridor (walk 67):
  two moving mines sweep it, the first bar over its mouth: Blupi jumps up there, hangs and moves
  along above them, then steps up through a hole into the hall.
- The hall (walk 70, to y 80): five helicopters; two columns standing, two hanging from the vault,
  a treasure on and under each; a hole framed in girders up to the upper garage (walk 82): four
  helicopters.
- The top (walk 88), up a hole from the garage: the switch's corridor (one wide, the switch in its
  floor at the mouth, two saws in its floor under two hanging mines, treasure 10 at its end); the
  flooded bend under the north wall (dismount, dive down, along and up) to the niche of treasure 9;
  an egg on a ledge; the arrow's box (blue blocks, a grate roof with a gap to drop through, two eggs,
  machine parts).

**Checks**: the real world on foot pumps up the spiral to the landing (`levelcheck.py`; doors stay
closed there); stage 1 adds the pool storey (treasures 2-4, two eggs); stage 4 walks on to the hall's
floor and `flightcheck.py` reaches everything else (all ten treasures, eight eggs, the arrow).
`VerifyShippedWorlds`. Live (`--mission 64`, spawns per storey, direct view F3): the bottom with
the spiral overhead, the pool with a fish, the corridors, the stair's mines, the shuttles, the
bulldozer, the hanging mines, the bars over the mines, the garages, the top with the box. Played
live: the potion then the stair (alive among the saws under Hide); the bars, a jump at the mouth,
hanging across over the two mines (which led to D45: a bar overhead could not be caught).
