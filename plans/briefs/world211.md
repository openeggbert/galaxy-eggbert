# World 211 — multiplayer: the palace hall

Built by `tools/worlds/world211.py` → `worlds3d/world211.vwr` (mission 211, region 23 (not among Mobile Eggbert's backgrounds: 25, the palace's mosaic, instead), music 6). A network
game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD211.BLP`): the palace's white stone (396, 397, 392-395) with its trim; the four starts in one row far apart, facing right; no lift; 33 helicopters, 16 shields, 20 tanks, 40 ammunition packs, five charges, 15 dynamite sticks (55), each team's bombs (17).

**3D design**: a white palace hall on grass (x 32-68, z 36-62, walk 10, roof 25), windows high on its north wall. Two rows of three columns; a dais along its north wall (walk 13) up a wide stair; a balcony along each side wall (walk 16) up a stair at its south end. The four starts along its south wall facing north (38, 46, 54, 62; 61), each team's three bombs before its start. Helicopters on the dais, the balconies and the floor; charges, dynamite and shields on the dais and the balconies.

**Checks**: `arena.check()` (every start and object stands in an empty cell over a solid one) when
it is built; `VerifyShippedWorlds`; a two-player match started in it (`NET_WORLD=211
tools/net_two_players.sh`).
