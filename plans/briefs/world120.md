# World 120 — the hub of world 12 (the brick tower)

Built by `tools/worlds/world120.py` → `worlds3d/world120.vwr` (mission 120, region 24 as the
original). World 12's brick: `tools/worlds/station.py` (`BRICK` 261 with its worn variants).

**Reference** (`worlds/world120.txt`, video 0:06:17): a little tower of brick (261-263, 283) on
stone posts (76). Blupi starts on its lowest floor between mission 1's sign (174, open) and
mission 4's (177); a floor up, missions 2 (175) and 5 (178); at the top, mission 3 (176); each
later sign behind its blue bar (182). World 12 has five missions.

**3D design**: the tower's floors step back north as they rise, each a jump (two) above the last,
standing in the sky on piers of brick.
- The ground floor (x 42–58, z 44–58, walk 21): spawn (55, 21, 54) by sign 1 (east end, open);
  sign 4 at the west end; a low parapet along the south edge.
- The first floor (z 44–51, walk 23): sign 2 east, sign 5 west.
- The top floor (z 44–47, walk 25): sign 3 facing south.
- The posts (76) are left out (an empty-mask tile is a floor to the vertical test, D41); piers of
  brick stand under the ground floor's corners instead.

**Checks**: `levelcheck.py` (all five signs reachable, bars counted open); `VerifyShippedWorlds`.
Live (`--mission 120`, spawns on the floors, direct view F3): sign 1 readable, the floors, the
kiosks with their bars.
