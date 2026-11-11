# World 209 — multiplayer: the cobbled cave

Built by `tools/worlds/world209.py` → `worlds3d/world209.vwr` (mission 209, region 11, music 10). A network
game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD209.BLP`): the grey cobbles (156, 35-38, 144-152); the four starts in the middle, yellow and orange facing blue and green; no lift; 22 helicopters, 18 tanks, 47 ammunition packs and a charge; no personal bomb.

**3D design**: a cave cut into cobble (x 34-66, z 38-62, walk 10, roof 23), its faces dressed with the cobble's variants. A terrace along its north and its south wall (walk 12) a step up along their whole edge, a ledge along its west and its east wall (walk 15) up a stair. The four starts in the middle: yellow and orange (46) facing east, blue and green (54) facing west, a tank between them. Helicopters on the terraces, the ledges and the floor; the charge on the west ledge.

**Checks**: `arena.check()` (every start and object stands in an empty cell over a solid one) when
it is built; `VerifyShippedWorlds`; a two-player match started in it (`NET_WORLD=209
tools/net_two_players.sh`).
