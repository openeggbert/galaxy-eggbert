# World 081 — mission 1 of world 8 (the sky steps)

Built by `tools/worlds/world081.py` → `worlds3d/world081.vwr` (mission 81, region 26 as the
original). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (no birds or mines).

**Reference** (`worlds/world081.txt`, video 6:07:50-6:11:23): a climb through the sky from Blupi's
start low in the west to the red arrow high in the east, over small machine platforms (panel 10,
pipe runs 251-253, bolts) and, between them, the saucers of the Temp tile (324): all of them
vanish together for two frames of their twenty (0.4 s in every 4 s, `Decor::IsPassIcon`), and
Blupi standing on one then falls. Twenty-one birds (20) fly round the way; a causeway of saucers
laid over a ribbon of lava, with two still mines (16) hovering over it; eleven eggs on the
platforms; a fall is the end (the original's lava floor, here the void).

**3D design**: the climb winds round the sky, out of any single line.
- Thirteen platforms (panel decks banded with pipe round their flanks, bolts under the corners),
  from the start (x 10–16, z 76–82, walk 20; spawn (13, 20, 80) facing −Z) round west, north, east
  and back south to the arrow's (x 76–84, z 68–74, walk 56), each a little higher than the last.
- Between consecutive platforms a straight chain of saucers along their shared line: saucers two
  by two (one where the length asks) with a gap of one between them, so each hop is a jump of two
  and never more than two up; a bird crosses each chain at its middle. A player lands, steps to the
  saucer's far edge, and jumps before the saucers next vanish.
- Halfway up, the causeway east (x 49–71, z 21–23, walk 40): saucers in three lanes over a ribbon
  of lava on a panel keel, rests of panel every six cells to wait on while the saucers vanish, the
  two still mines over the middle lane; two birds cross it.
- More birds round the higher platforms; eggs: two at the start, one each on five platforms, three
  at the causeway's east end.

**Checks**: stage 1: `levelcheck.py` reaches all eleven eggs and the arrow; the real world stops
where the birds' patrols stand (levelcheck keeps off them). `VerifyShippedWorlds`. Live (`--mission
81`, spawns on the platforms and the causeway, direct view F3): the chains of saucers between the
platforms, the birds, the causeway and its mines. Played live: the first hop from the start
platform onto a saucer (an earlier layout of single-cell saucers, too small a target in 3D, was
replaced by the two-by-two ones). Not played through: the whole climb.
