# World 202 — multiplayer: the machine towers

Built by `tools/worlds/world202.py` → `worlds3d/world202.vwr` (mission 202, region 0, music 5 as
the original). A network game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD202.BLP`): the blue machine panel (icon 10) over a larger
area than 201; two starts on the floor (row 40) and two above them (row 35); one lift; 23
helicopters, 16 tanks, 51 ammunition packs, 12 drinks, 11 personal bombs of each team.

**3D design**: a closed machine hall (x 32–68, z 38–62, walk 10, roof 26) with a tower along each
side wall (walk 16), joined by a bridge along the north wall. Yellow (44, 50) and blue (56, 50)
start on the floor facing the middle, orange (35, 50) and green (65, 50) on the towers above them.
A stair up each tower's inner face at its south end, the one lift beside the bridge's middle; three
bombs of each team beside its start; helicopters on the floor, the towers and the bridge; tanks,
ammunition and drinks about.

**Checks**: `VerifyShippedWorlds`; a look from the start (`--mission 202`).
