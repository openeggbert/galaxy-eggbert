# World 210 — multiplayer: the village street

Built by `tools/worlds/world210.py` → `worlds3d/world210.vwr` (mission 210, region 17 (not among Mobile Eggbert's backgrounds: 6, the dark rock, instead), music 8). A network
game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD210.BLP`): a wide map of the village's stone (156, 35-38, 144-145, 152-155) and its houses (185, 186); the four starts far apart along it; two lifts; 40 helicopters, 8 shields, 20 tanks, 29 ammunition packs, 15 drinks, three charges, each team's bombs (14).

**3D design**: a long walled street of stone (x 22-78, z 42-58, walk 10, roof 23). Five houses along its north side (village.house: windows on the street, slate roofs walking at 14) with alleys between them; two lifts before the second and the fourth house up to the roofs; a well in the middle. The four starts along the street (yellow 26 facing west, green 36, orange 56, blue 70 facing east; z 54), each team's three bombs south of its start. Helicopters on the roofs and the street, charges and drinks on the roofs, shields, tanks, ammunition.

**Checks**: `arena.check()` (every start and object stands in an empty cell over a solid one) when
it is built; `VerifyShippedWorlds`; a two-player match started in it (`NET_WORLD=210
tools/net_two_players.sh`).
