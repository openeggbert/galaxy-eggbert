# World 100 — the hub of world 10 (the cobbles)

Built by `tools/worlds/world100.py` → `worlds3d/world100.vwr` (mission 100, region 2 as the
original: a dark cobbled sky). World 10's stone: `tools/worlds/village.py` (`STONE` 35 with its
edge and face variants).

**Reference** (`worlds/world100.txt`, video 3:54:27): a heap of grey cobbled blocks (35, edge
pieces 36-40 and 144-152, 156) on little posts (76) in a dark cobbled sky. Blupi starts beside
mission 1's sign (174, open); missions 2-7 (175-180) sit on the blocks round it and above it,
each behind its blue bar (182): 2 below and right of 1, 3 and 4 further right, 5 and 6 to the
left, 6 higher, 7 at the top right. World 10 has seven missions.

**3D design**: three tiers of cobbled terraces floating in that sky, each a jump (two) above the
one it steps up from.
- The plaza (x 42–58, z 48–58, walk 21): spawn (55, 21, 53) facing −Z by sign 1's kiosk on its
  east edge; signs 2 and 3 in its south edge, 5 in its west edge.
- The terrace north of it (x 40–60, z 38–47, walk 23): sign 4 at its east end; knobs of cobble
  standing on it.
- The west block (x 32–39, z 36–47, walk 25): sign 6 at its west end.
- The north-east step (walk 25) and the top (x 54–62, z 29–35, walk 27): sign 7.
- Low cobbled walls along open edges; under the tiers the heap tapers into the sky. The posts (76)
  are left out: an empty-mask tile is a floor and ceiling to the vertical test (D41).
- The kiosks are two deep with the bar in the doorway, so Blupi arrives outside the bar last
  opened (HubDoors).

**Checks**: `levelcheck.py` (all seven signs reachable, bars counted open); `VerifyShippedWorlds`.
Live (`--mission 100`, spawns on each tier, direct view F3): sign 1 readable in its kiosk, the
kiosks of 2 and 3 with their bars, the terrace's front, the west block, the steps to the top.
