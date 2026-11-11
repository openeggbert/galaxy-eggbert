# World 206 — multiplayer: the metal factory

Built by `tools/worlds/world206.py` → `worlds3d/world206.vwr` (mission 206, region 14 (not among Mobile Eggbert's backgrounds: 29, the machine parts, instead), music 9). A network
game's world (`plans/plan_gemultiplayer.md`).

**Reference** (Speedy Blupi 2, `DATA/WORLD206.BLP`): the riveted metal plates (78-84, 87-90); the four starts in one row all facing the same way; no lift; 12 helicopters, 12 tanks, 32 ammunition packs and thirteen bombs of each team (200-203).

**3D design**: a closed metal hall (x 32-68, z 38-62, walk 10, roof 24). The four starts along its south wall facing north (38, 46, 54, 62; 61), three of each team's bombs before its start. A stage in the middle (x 44-56, z 46-54, walk 13) up a stair on its north and its south side; a gallery along each side wall (walk 15) on girder masts, up a stair at its north end. Helicopters on the galleries, the stage and the floor; tanks and ammunition about.

**Checks**: `arena.check()` (every start and object stands in an empty cell over a solid one) when
it is built; `VerifyShippedWorlds`; a two-player match started in it (`NET_WORLD=206
tools/net_two_players.sh`).
