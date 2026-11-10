# World 203 — multiplayer: the toy box

Built by `tools/worlds/world203.py` → `worlds3d/world203.vwr` (mission 203, region 22, music 8 as
the original). A network game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD203.BLP`): the toy tiles (215-234: balls, striped poles,
stars, trees); the four starts side by side on one row; six lifts; 15 helicopters, 9 shields, 22
tanks, 42 ammunition packs, one charge (31); no personal bomb.

**3D design**: a closed hall of toy balls (x 34–66, z 36–64, walk 10, roof 25). The four starts in
a row along the west wall facing east (36, 44 / 48 / 52 / 56). A low platform in the middle (walk
14) on two lifts, two high ones in the east corners (walk 17) on two lifts each; striped posts and
candy poles about. Helicopters on the platforms and the floor, tanks, shields, ammunition, the
charge on the middle platform.

**Checks**: `VerifyShippedWorlds`; a look from the start (`--mission 203`).
