# World 217 — multiplayer: the floating islands

Built by `tools/worlds/world217.py` → `worlds3d/world217.vwr` (mission 217, region 7, the stars;
music 4). A network game's world of Galaxy's own design (Q-022).

**Identity**: Galaxy's title scene (`title.py`): grass islands (107-109, 157) with earthen
undersides (284, 300, 301) floating among the stars.

**3D design**: open.

- A large grass island in the middle (radius 9, walk 12).
- Four smaller islands at the compass points (radius 5), each holding one team's start and its
  three bombs, joined to the middle island by straight grass causeways.
- Far below, a meadow (walk 5) ringed by a low stone rim catches whoever falls. A lift beside each
  outer island takes him back up.
- Four helicopters (two on the middle island, two on the meadow), two tanks, ten ammunition packs,
  two shields, two drinks, a charge in the middle.

**Checks**:

- `arena.check()`.
- `VerifyShippedWorlds`.
- A two-player match started in it (`NET_WORLD=217`).
