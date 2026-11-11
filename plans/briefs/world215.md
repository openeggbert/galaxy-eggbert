# World 215 — multiplayer: the sky works

Built by `tools/worlds/world215.py` → `worlds3d/world215.vwr` (mission 215, region 26, music 8). A
network game's world of Galaxy's own design (Q-022).

**Identity**: Mobile Eggbert's world 8, the blue machine works in the sky (`skyworks.py`): the cyan
panel (10) and its fittings, pipes (250-255), and the Temp saucers (324) that vanish together for a
moment every few seconds.

**3D design**: open to the sky.

- A deck of panel (x 34-66, z 38-62, walk 10) walled by a pipe parapet.
- A long raised deck along the west and the east side (z 41-59, walk 14), each up a stair from the
  middle. The two are joined across the south by a bridge of Temp saucers: whoever stands on it
  when they vanish falls back to the deck.
- A high deck in the middle (x 46-54, z 46-54, walk 18) on four pipe columns, a lift up on each
  side.
- The four starts in the deck's corners, three of each team's bombs beside them.
- Five helicopters (two on the high deck), three tanks, nine ammunition packs, two shields, two
  drinks, a charge on the high deck.

**Checks**:

- `arena.check()`.
- `VerifyShippedWorlds`.
- A two-player match started in it (`NET_WORLD=215`).
