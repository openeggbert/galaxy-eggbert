# World 090 — the hub of world 9 (the sinkhole of clods)

Built by `tools/worlds/world090.py` → `worlds3d/world090.vwr` (mission 90, region 27 as the
original: a dark red cave). World 9's tiles and pieces: `tools/worlds/clods.py`.

**Reference** (`worlds/world090.txt`, video 2:31:15, 2:36:18, 2:44:17, 2:50:32, 2:53:59): a sponge
of orange-brown clay clods (284 and 301, about 5 : 4, their rims drawn with the edge sprites
285–303) with the red cave showing through its holes. Blupi starts at the top left beside
mission 1's sign (174); the pockets of 2–5 (175–178) follow in a chain down to the right, each a
step lower than the last, each behind a blue bar (182) one cell before its sign. No objects.

**3D design**: a floating mass of clods (x 38–62, z 38–62) with a sinkhole in the middle (the pit,
x 45–55, z 45–55, floor at walk 12) and a ledge five wide round each of its sides, each three
lower than the last, joined at the corners by two one-block steps:
- North ledge (z 40–44, walk 26): spawn (47, 26, 43) facing −Z, sign 1 open in the north wall in
  front of him at (47, 26, 39).
- East ledge (x 56–60, walk 23, steps at z 45/46): sign 2 in the east wall at (62, 23, 53).
- South ledge (z 56–60, walk 20, steps at x 55/54): sign 3 in the south wall at (47, 20, 62).
- West ledge (x 40–44, walk 17, steps at z 55/54): sign 4 in the west wall at (38, 17, 50).
- From the west ledge's north end a stair of clods along the pit's west side (walk 16, 15, 14,
  13) down to the pit floor; sign 5 in the pit's north wall under the start at (50, 12, 43).
- Signs 2–5 two deep behind their bars (`kit.sign_niche`). A fall into the pit is a short cut
  down; the stair and the corner steps lead back up to every ledge.
- The walls stand three above the ledge beside them (out of a jump's reach), a lump higher here
  and there, so the red cave shows over them as through the original's holes; lumps (286/303) on
  their tops, clods floating in the cave round the hub (four or more cells off, out of reach) and
  one over the pit, lumps (285/302) and strings (300) hanging under them; plants on the ledges
  and the pit floor.

**Edge sprites in 3D** (`clods.py`): they are flat plates; since D48 a plate reads unmirrored from
both sides, so only the symmetric ones are used (285/302 under undersides, 286/303 on tops,
289/290, 297–300); 287/288 and 291–296 would show their edge on the wrong side from behind.
Plates and props are floors and ceilings to the vertical test (D41, open): `clods.audit()` lists
any within reach of a cell levelcheck reaches, and the script fails if there is one.

**Checks**: `levelcheck.py` OK (all five signs; bars counted open), and from the pit floor every
ledge is reachable again (`build-probe/world9-agent/reachfrom.py`); the D41 audit is clean;
`VerifyShippedWorlds` 80/80. Live (`--mission 90`, `build-probe/world9-agent/shots/hub*`): the
start view with sign 1 in the north wall; walked round onto the east ledge (the jagged walls with
the red cave over them); views from each ledge and the pit floor; walked into sign 1 → mission 91
loads; with a probe save (`doorsUnlocked` index 92) bar 2 is open, Blupi arrives in front of its
niche facing it, walks in and mission 92 loads.
