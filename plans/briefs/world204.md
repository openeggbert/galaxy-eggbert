# World 204 — multiplayer: the metal works

Built by `tools/worlds/world204.py` → `worlds3d/world204.vwr` (mission 204, region 1, music 4 as
the original). A network game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD204.BLP`): the riveted metal plates (78-84) with grey
girders (201); the four starts in two pairs on one row; five lifts; 22 helicopters, 14 shields, 6
tanks, 28 ammunition packs, 14 drinks, a charge, 11 personal bombs of each team.

**3D design**: a closed metal hall (x 34–66, z 36–64, walk 10, roof 24). Yellow and orange start
at its west end, blue and green at its east end, facing in, three bombs of each team beside its
start. A gallery of plates across the middle (walk 15) on girder masts, reached by four lifts; a
high platform in the north (walk 19) on the fifth. Helicopters on the gallery, the high platform
and the floor; tanks, shields, ammunition, drinks, the charge on the gallery.

**Checks**: `VerifyShippedWorlds`; a look from the start (`--mission 204`).
