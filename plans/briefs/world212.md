# World 212 — multiplayer: the clod cave

Built by `tools/worlds/world212.py` → `worlds3d/world212.vwr` (mission 212, region 27 (the dark red cave), music 7). A network
game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD212.BLP`): the orange-brown clods (284, 301) with their edge lumps (285-290, 302, 303), trestles (199) and girders (201); the four starts in one row facing right; no lift; 48 helicopters, 20 shields, 26 tanks, 24 ammunition packs, a charge, each team's bombs (18).

**3D design**: a cave cut into clods (x 32-68, z 38-62, walk 10, roof 24), lumpy along its walls, lumps hanging from its ceiling. A mound in the middle (walk 13) up a stair on its north and its south side; two clod columns; two islands floating high in its corners (walk 18) that only a helicopter reaches (shields, ammunition and the charge on them); trestles about. The four starts along its south side facing north (38, 46, 54, 62; 61), each team's three bombs before its start.

**Checks**: `arena.check()` (every start and object stands in an empty cell over a solid one) when
it is built; `VerifyShippedWorlds`; a two-player match started in it (`NET_WORLD=212
tools/net_two_players.sh`).
