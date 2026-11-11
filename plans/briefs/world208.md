# World 208 — multiplayer: the library

Built by `tools/worlds/world208.py` → `worlds3d/world208.vwr` (mission 208, region 3 (the red curtains), music 2). A network
game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD208.BLP`): the library's bookshelves and cupboards (139-143); the four starts at one place on the map's west edge facing right; one lift; 42 helicopters, 10 shields, 14 tanks, 37 ammunition packs; no personal bomb.

**3D design**: a library hall walled with shelves on a brick floor (x 32-68, z 38-62, walk 10, roof 24). The four starts side by side along its west wall facing east (x 32, z 47-56). Rows of shelves two high across its middle with aisles between them; a brick gallery along its north wall (walk 15) on pillars, up the one lift in its middle and a stair at its east end. Helicopters on the gallery and the floor, shields, tanks, ammunition.

**Checks**: `arena.check()` (every start and object stands in an empty cell over a solid one) when
it is built; `VerifyShippedWorlds`; a two-player match started in it (`NET_WORLD=208
tools/net_two_players.sh`).
