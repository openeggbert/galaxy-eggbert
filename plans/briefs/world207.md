# World 207 — multiplayer: the stone islands

Built by `tools/worlds/world207.py` → `worlds3d/world207.vwr` (mission 207, region 16 (the clouds), music 1). A network
game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD207.BLP`): a sparse map of the village's stone (38, 144-146, 76); the four starts in two pairs back to back (yellow and blue facing left, orange and green right); no lift; 28 helicopters, 23 tanks, 46 ammunition packs, 20 drinks, two charges, each team's bombs (5, 5, 5, 4).

**3D design**: a closed stone hall with a grass floor (x 34-66, z 36-64, walk 10, roof 25). Four stone islands in its corners (6 x 6, walk 13) each up a stair from the middle, a tower in the middle (walk 16) up a stair from the south. The two pairs back to back south of the tower (43/44 and 56/57, z 54), each team's three bombs beside its start. Helicopters on the islands and the floor, a charge on the tower and one on an island, drinks, tanks, ammunition.

**Checks**: `arena.check()` (every start and object stands in an empty cell over a solid one) when
it is built; `VerifyShippedWorlds`; a two-player match started in it (`NET_WORLD=207
tools/net_two_players.sh`).
