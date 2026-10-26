# World 040 — the hub of world 4 (the cheese wheel)

Built by `tools/worlds/world040.py` → `worlds3d/world040.vwr` (mission 40, region 30 as the
original). World 4's tiles and pieces: `tools/worlds/cheese.py`.

**Reference** (`worlds/world040.txt`, video 1:35:01, 1:37:25, 1:42:47, 1:48:54, 1:55:46,
2:05:41): one corridor through a block of Swiss cheese (246–249 with their drip edges 264–282)
under the region-30 background of leaves. Blupi starts at its west end beside mission 1's sign
(174); going east the signs of missions 2–6 (175–179) follow, each behind a blue bar (182) in
the corridor, so the corridor opens one sign further each time a mission is won. No objects.

**3D design**: a cheese wheel (radius 16, centre (50, 50)) lying among the leaves; inside it a
round courtyard (radius 10, walking level 5) walled four high, out of a jump's reach.
- The six signs in niches round the wall, clockwise as the original's corridor runs: 1 open at
  (48, 5, 39) and 2 at (52, 5, 38) in the north wall, 3 at (62, 5, 48) and 4 at (62, 5, 52) in
  the east wall, 5 at (52, 5, 62) and 6 at (48, 5, 62) in the south wall. Signs 2–6 stand two
  deep, their bars in the doorways (`kit.sign_niche`); a bar opens once the mission before it
  is won, and Blupi then arrives in front of the sign last opened.
- The west quarter of the rind is cut down to a terrace level with the courtyard, a parapet
  one high round its edge: there the leaves show over the rim (with the wall all round, the
  chase camera saw nothing but cheese). Plants (BigDecor 23–25) on it.
- A low lump of cheese in the middle; craters dented into the rind's top and outer face.
- Spawn (48, 5, 45) facing −Z, in front of sign 1.

The edge tiles 264–282 are sprites with only small drips: cut out in 3D they all but vanish
(checked live), so the cheese is carved round instead.

**Checks**: `levelcheck.py` (all six signs reachable, bars counted open); `VerifyShippedWorlds`.
Live (`--mission 40`): the start view with sign 1 and bar 2; a walk round the courtyard past
every sign, the open west terrace with the leaves; with a probe save (`doorsUnlocked` 42, 43 in
the QA run's own `savedata.txt`) bars 2 and 3 open and Blupi arrives in front of sign 3.
