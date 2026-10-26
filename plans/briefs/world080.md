# World 080 — the hub of world 8 (the sky works)

Built by `tools/worlds/world080.py` → `worlds3d/world080.vwr` (mission 80, region 26 as the
original: sky and cyan). World 8's tiles and pieces: `tools/worlds/skyworks.py`.

**Reference** (`worlds/world080.txt`, video 6:07:45): a small machine of cyan riveted panels (10)
and blue pipes (251, 252) floating in the sky. Below, mission 1's sign (174) open right of
Blupi's start and mission 3's (176) left of it behind its blue bar (182); above, missions 2 (175)
and 4 (177), each behind a bar; fittings and bolts (15-18, 56, 59, 60) on the panels. World 8's
missions use the same panels, pipe runs (250-255) and the Temp saucers (324) that shrink away and
come back.

**3D design**: a deck on two levels in the open sky.
- The lower deck (x 40–60, z 49–57, walk 21): spawn (50, 21, 54) facing −Z, the pods of 3 (west
  end, its bar facing east) and 1 (east end, open).
- The upper deck north of it, two higher (x 42–58, z 41–48, walk 23): the pods of 4 (west) and 2
  (east), both behind bars; a tank of panel between two columns of pipe machinery, a pipe over
  them.
- Pipe parapets round the edges (two high on the lower deck's south side), gaps at the pods; bolts
  along the lower deck's south edge; a pipe and stubs of panel under the decks.
- The step between the decks is a jump; the pods are two deep, the bar in the doorway, so Blupi
  arrives outside the bar last opened (HubDoors).

**Checks**: `levelcheck.py` (all four signs reachable, bars counted open); `VerifyShippedWorlds`.
Live (`--mission 80`, spawns on both decks, direct view F3): the start toward the upper deck, the
parapets, the pods with their bars (their walls plain panel, after bolt props mixed into them left
holes).
