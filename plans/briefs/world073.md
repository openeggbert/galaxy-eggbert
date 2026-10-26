# World 073 — mission 3 of world 7 (the toy works)

Built by `tools/worlds/world073.py` → `worlds3d/world073.vwr` (mission 73, region 22 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (1: every lift's low end raised
level with its ledge, as levelcheck boards a lift only level with it; 2: also the treasure door
open; 3: also stand-in lifts for the lollipop's jumps; 4: also the skate corridor's mines gone;
5: also the bulldozers left out).

**Reference** (`worlds/world073.txt`, video 3:43:07–3:45:29): all of the toy box mixed (215–234 in
about equal numbers), candy poles, lava (68). Four treasures, three eggs, the red arrow at the
top. Blupi starts by a maze of lifts in narrow shafts with lava at their bottoms (60/20, 40/10,
60/10, 40/10 and a quick 18/3); two treasures in it and the treasure door "2" (422), which lets him
out once both are taken; a strip of lava crossed on small shuttles (20 ticks); the lollipop (26)
and two licked jumps up to a shelf with four skateboards (24); a corridor with two rows of five
mines and two cells of lava in its floor, ridden on the skateboard (it runs mines over and
jumps); lifts at its east end up to an upper corridor back west with two bulldozers (4) and two
shuttles meeting over a gap (10 ticks), two eggs; lifts on striped poles in the open sky up to the
arrow, a treasure on the way. 10 mines, 25 lifts, 4 skateboards, 2 bulldozers.

**3D design**: stacked from south to north: the lift hall, the river, the landing and the shelf,
the skate corridor, the upper corridor, the climb in the sky.
- B, the lift hall (x 43–57, z 66–89, walls to y 23, open above), lava over its floor: the start
  shore S (walk 8, spawn (50, 8, 87) facing −Z), pillars P1 (walk 11), P2 (8, the low pocket,
  treasure 1), P3 (15), P4 (10, the egg's pocket) and the north ledge N (18, treasure 2). Lifts
  beside the pillars, each boarded by a step down onto it and left level with the ledge it
  serves: L1 S→P1 (60 ticks), L2 P1↔P2, L3 P1→P3, L4 P3↔P4 (pockets: level both ends, ridden
  there and back), L5 P3→N (18 ticks). The door "2" (422) three wide through the north wall.
- C, the river (x 46–54, z 42–62): islands stepping down one each (walk 18, 17, 16, 15), a strip of
  lava between each two, a raft of two lifts in step (20 ticks) one below the island it leaves and
  level with the one it reaches. The lollipop on the last island (50, 15, 42).
- D, the landing L (z 36–41, walk 20, treasure 3) five over the last island and the shelf SH
  (z 30–35, walk 25, four skateboards) five over L: two licked jumps.
- K, the skate corridor (x 55–88, z 37–39, walk 20, three high): two rows of mines five long and
  three wide (x 59–63, 78–82), two lava cells across its floor (x 68, 73) to jump. E at its east end:
  lifts E1 (to a ledge, walk 24) and E2 (to FL, walk 28).
- F, the upper corridor (walk 28 east of its gap, 27 west of it) back west: the bulldozers'
  stretches one wide (60 and 90 ticks), the gap x 70–73 over lava with two shuttles meeting in its
  middle (10 ticks), the two eggs at its west end; F2 north out to the open sky.
- G, the climb (walk 27 → 36): platforms of balls on striped posts with candy poles under them;
  Ga up to G2, Gb down to G2b with treasure 4 and back, Gc up to G3, Gd up to G4 with the arrow
  (54, 36, 9) under a candy pole.
- Lift dwells are 40 ticks (2 s) where the original's are 3–20: in 3D Blupi must walk a cell on
  or off (the guide's rule). No lift needs a jump off it.

**Checks**: `levelcheck.py`: the real world reaches the start pocket (two treasures and P4's egg);
stage 1 the same; 2 also the river; 3 also the landing (treasure 3); 4 also the skate corridor's
far end and the upper corridor to the bulldozers; 5 everything: four treasures, three eggs, the
arrow. `flightcheck.py --mine-margin 0` agrees (stage 3 stops at the mines). `VerifyShippedWorlds`:
80/80. Live (`--mission 73`, probes through `QA_LIVE_WORLDS` with markers, shots
`build-probe/world7-agent/shots/73*`): every part's view; L1 boarded at its dwell, ridden and left
onto P1; L2 and L4 round trips into the pockets; L3, L5; the door "2" stays shut with one
treasure and opens at two; the rafts crossed (a probe with long dwells for all three, one at the
authored timing); the lollipop licked and both licked jumps (onto L, onto SH); a skateboard taken
and ridden over both mine rows (they explode under it) and jumped over the lava cells; E1, E2;
the shuttle transfer over the gap; Ga–Gd; touching the arrow wins.
Found while building it: Blupi could not jump off a lift (reported, fixed as D52) and could drop
off a lift he walked off onto a level ledge (reported, D54); with both fixed the lifts above
were re-checked. Not played in one run from start to arrow.
