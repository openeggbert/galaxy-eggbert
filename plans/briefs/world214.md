# World 214 — multiplayer: the slime pit

Built by `tools/worlds/world214.py` → `worlds3d/world214.vwr` (mission 214, region 31, music 5).
A network game's world of Galaxy's own design (Q-022).

**Identity**: Mobile Eggbert's world 6, the green slime (`slime.py`: one tile set, each cube skinned
by what lies above and below it). Its spike trap (373, `Decor::IsPiege`) and the striped poles
(235/236) its lifts run along.

**3D design**:

- A closed slime hall (x 32-68, z 36-64, walk 10, roof 24).
- A pit in the middle (x 44-56, z 45-55, walk 7), entered by a stair cut into its north and its
  south side. Two spike traps lie in opposite corners of its floor.
- A ledge along the west and the east wall (z 42-58, walk 14), each reached by a lift between two
  poles.
- The four starts in the corners, three of each team's bombs beside them.
- Four helicopters (two on the ledges), four tanks, eight ammunition packs, two shields, a drink,
  a charge on a ledge.

**Checks**:

- `arena.check()`.
- `VerifyShippedWorlds`.
- A two-player match started in it (`NET_WORLD=214`).
