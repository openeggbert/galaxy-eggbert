# World 216 — multiplayer: the station's brick hall

Built by `tools/worlds/world216.py` → `worlds3d/world216.vwr` (mission 216, region 1, music 2). A
network game's world of Galaxy's own design (Q-022).

**Identity**: Mobile Eggbert's world 5, the space station (`station.py`): its brick halls (261-263,
283) with water (92) and lattice girders (201).

**3D design**:

- A closed brick hall (x 32-68, z 36-64, walk 10, roof 24).
- A dais in the middle (x 46-54, z 46-54, walk 13), climbed by a stair cut into its north and its
  south side.
- A moat of water round the dais, two cells wide, its surface a cell under the floor, crossed by a
  bridge on each side.
- A balcony along the north and the south wall (x 42-58, walk 14), each with a lift.
- Four girder masts.
- The four starts in the corners, three of each team's bombs before them along the side walls.
- Four helicopters (two on the dais, two on the balconies), four tanks, nine ammunition packs, two
  shields, two drinks, a charge on the dais.

**Checks**:

- `arena.check()`.
- `VerifyShippedWorlds`.
- A two-player match started in it (`NET_WORLD=216`).
