# World 010 — the hub of world 1 (blue machine)

Built by `tools/worlds/world010.py` → `worlds3d/world010.vwr` (mission 10, region 0).

**Reference** (`worlds/world010.txt`): a small machine room on two levels — below, mission 1's
sign (open, 174) and mission 3's behind its blue bar (182); above, missions 2 and 4 behind theirs;
Blupi starts between 1 and 3. A bar slides up once the mission before it is won
(`Decor::AdaptDoors`), and Blupi starts outside the last bar opened (`SearchDoor`), as the video
shows after mission 011 (start beside door 2, the bar slides up).

**3D design**: a machine hall x 40–60, z 40–58, floor at walk 1. A gallery along the north side
(z 43–45, walk 3, a jump up) carries signs 3 (x 45, behind a bar) and 1 (x 55) in its face; the
north wall above the gallery carries 4 (x 45) and 2 (x 55), both behind bars. Gated signs sit two
deep with the bar in the doorway under a lintel, so Blupi cannot step or jump round a bar.
Monitors and machine parts decorate the walls and the floor's edges. Spawn (50, 1, 53).
