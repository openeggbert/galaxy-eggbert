# World 074 — mission 4 of world 7 (the toy depot)

Built by `tools/worlds/world074.py` → `worlds3d/world074.vwr` (mission 74, region 22 as the
original). Staged variants for `levelcheck.py`: `--stage K out.vwr` (1: the mines gone, the bars
taken away; 2: the mines gone, bars in place; 3: also the door "5" open; 4: also the door "7"
open; 5: also bridges where Blupi swims).

**Reference** (`worlds/world074.txt`, video 3:45:32–3:48:18): all of the toy box mixed (215–234),
candy poles, lava (68), a pool (92), bars (138) and rods (202), warning signs (48). Nine treasures,
four eggs, the red arrow at the top. Blupi starts between two skateboards (24) on posts; rows of
mines (2) only a skateboard crosses; a stair down with lava in each step's gap; mines under warning
signs; bars over lava; a stair of five treasures to the treasure door "5" (425); two pits under
bars; a relay of three lift pairs in step (40 ticks, 10 dwell): the first rises, the second comes
down beside it, the third waits above; an upper corridor back with mines and a floor of lava under
bars and rods, a treasure on a cell between its pools; a shelf of skateboards with eggs above; the
treasure door "7" (427); mines; a pool with two treasures on pillars, swum to; a second relay up to
the arrow. 20 mines, 12 lifts, 14 skateboards.

**3D design**: a U of toy halls round a pool court open to the sky.
- A, the start court (x 13–27, z 69–83, walk 10, open, posts with balls at its corners): spawn
  (20, 10, 78) facing −Z; a skateboard on a low toy table (one block high, the post of the
  original as a table: landing on a one-cell post top was fiddly) either side.
- B, the skate street (x 19–21, z 32–68, three high) north: a row of mines four deep and one
  three deep across it, a lintel of warning signs before each; alcoves between the rows with a
  spare skateboard on a table each side.
- C, the lava stair east (z 32–34): three steps down (walk 10, 9, 8), each three long, a gap of
  lava between, level with the next step; the foot corridor (walk 7) with three rows of mines under
  warning signs; a pit four long, lava four below, rods (202) across it at the walking level: walk
  off the edge and hang along.
- D, the treasure tower (x 50–57, z 29–37): a stair of five steps round a newel (walk 8 … 12), a
  treasure on each; the door "5" (425) out at walk 12.
- E, the pits (walk 12): two pits two wide, lava five below, rods across both.
- F, the east relay (x 72–75, z 36–37): pair A rises from the pits' floor four, pair B beside it
  comes down to meet it, both go back; B rises to meet pair C over A, which rises to the upper
  storey (all 40 ticks, 40 dwell).
- G, the upper storey (walk 24): the landing L2 with two skateboards on tables; the corridor south
  with three rows of mines, then a lava floor (two pools four long) under bars one above the
  walking level (a jump catches them, D45), an island between with a treasure (no bar over its
  first cell: he drops onto it), a treasure at the corridor's end; the corner room (to y 30): a
  shelf two high with two skateboards, the eggs' ledge two higher with three eggs; the door "7"
  (427) west; the west corridor with three rows of mines to the balcony over the court.
- H, the pool court (x 28–55, z 44–66, walk 15, open to the sky): two terraces down from the
  balcony (walk 21, 18), the pool (x 34–45), water three deep, its top cell level with the banks'
  top; two pillars level with the water, a treasure on each; an egg in the court's corner; toy
  decor.
- I, the west relay (x 29–31, z 52–53): pair D rises from the court, pairs E either side come down
  to it, F over D rises to the landing DL (walk 27).
- J, the pavilion: two jumps west from DL (walk 29, 31) to the arrow's platform (walk 33) on a
  striped post, under a dome of balls on candy poles.

**Checks**: `levelcheck.py`: the real world stops at the street's first row (the start court);
stage 1 at the foot corridor's pit; 2 the tower's five treasures; 3 also the upper storey to the
door "7" (seven treasures, the shelf's three eggs); 4 the same (the court needs swimming); 5 all
nine treasures, four eggs and the arrow. `flightcheck.py --mine-margin 0` agrees with the real
world. `VerifyShippedWorlds`: 80/80. Live (`--mission 74`, probes through `QA_LIVE_WORLDS` with
markers, shots `build-probe/world7-agent/shots/74*`): every part's view; the skateboard taken
from a start table and ridden into the street, both street rows run over (they explode under it,
the marker between them taken); the lava stair on foot and on the skateboard; the pit's rods to
the tower; the tower climbed to the door "5", shut at four treasures (a minimal probe: shut at
five shown until the last treasure's flight ends, then open; here it stayed shut with four); the
pits' rods; both relays ridden end to end with long-dwell probes (A→B→C→L2, D→E→F→DL); the lava
floor's bars, the drop onto the island's treasure and on; the shelf and the eggs' ledge; the west
corridor's mines on a skateboard; swimming to a pillar and out onto it and onto the west bank;
the pavilion's jumps to the arrow. A first build had the pool's water a block under the banks:
he could not jump out (reported: the guide's rule says one above is fine). The relays at the
authored timing and a full run were not played.
