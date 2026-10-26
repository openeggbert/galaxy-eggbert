# World 110 — the hub of world 11 (the machine pit)

Built by `tools/worlds/world110.py` → `worlds3d/world110.vwr` (mission 110, region 0 as the
original: the machine backdrop).

**Reference** (`worlds/world110.txt`, video 0:31:34): a cave in a block of the blue machine panel
(10), widening upward in steps. Blupi starts on its narrow floor beside mission 1's sign (174,
open); a step up, missions 2 (175) and 4 (177) face each other across it behind their blue bars
(182); another step up, missions 3 (176) and 5 (178). World 11 has five missions.

**3D design**: a pit in a block of panel open to the machine sky, its sides stepping out as they
rise (each ring one cell wider all round and a jump, two, higher).
- The floor (x 47–53, z 47–53, walk 21): spawn (49, 21, 50); sign 1 open on its east side.
- The first ring (walk 23): sign 2 east, sign 4 west, each two deep behind its bar.
- The second ring (walk 25): sign 3 east, sign 5 west.
- Machine parts along the top's rim.

**Checks**: `levelcheck.py` (all five signs reachable, bars counted open); `VerifyShippedWorlds`.
Live (`--mission 110`, spawns on the floor and both rings, direct view F3): sign 1 readable, the
rings and the kiosks with their bars.
