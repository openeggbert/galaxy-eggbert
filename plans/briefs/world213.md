# World 213 — multiplayer: the cheese cave

Built by `tools/worlds/world213.py` → `worlds3d/world213.vwr` (mission 213, region 20, the red sky;
music 3). A network game's world of Galaxy's own design (the owner's Q-022: five more after SB2's
201-212), not an original's mission.

**Identity**: Mobile Eggbert's world 4, the cheese (`cheese.py`): the four cheese tiles (246-249),
round holes, helicopters parked by the dozen (the cheese world's mechanic).

**3D design**:

- A closed cheese hall (x 32-68, z 36-64, walk 10, roof 25). Its long walls are thickened, with
  three round holes bitten into each, high up.
- A mound in the middle (x 44-56, z 45-55, walk 13), tunnelled through east-west at the floor
  (z 50) and climbed by a stair on its north and its south side.
- Two cheese pillars to the roof.
- The four starts in the corners facing along the hall, three of each team's bombs beside them.
- Six helicopters (two on the mound), four tanks, ten ammunition packs, two shields, three drinks,
  a charge on the mound.

**Checks**:

- `arena.check()` when it is built.
- `VerifyShippedWorlds`.
- A two-player match started in it (`NET_WORLD=213`, `tools/autoplay/net/start_*.txt`).
