# Mission 014 — the dynamite lesson and world 1's golden key

Built by `tools/worlds/world014.py` → `worlds3d/world014.vwr` (mission 14, region 18 as the
original); hints: `Tables::table_training4` (5 records, gated on carrying dynamite) as hint zones.

**Reference** (`worlds/world014.txt`, video 4:56–6:11): six sticks of dynamite beside the start;
a wall of crates ("Put down the dynamite with [Action], then clear off!"); three eggs above; a plug
of crates in the floor ("Put down another stick of dynamite here to move on"); down the shaft the
golden key (ObjectType21), which completes world 1 and returns to the main hub.

**3D design** (upper floors at walk 11, the key room at walk 1):
- The start room, three sticks each side. Blupi carries one stick at a time.
- A roofed corridor (x 47–49) sealed by nine crates (3 × 3) at z 48: put a stick at their foot,
  run back down the corridor (two cells along Z clear the 3D blast), the linked crates all go.
- The room beyond: three eggs on a ledge two blocks up; a 3 × 3 plug of crates in the floor over
  a shaft — a second stick beside it, and the shaft opens.
- The key room below, the golden key at (51, 1, 44).

**Checks**: `levelcheck.py` on the world (eggs and key sealed off) and on `--solved` (crates gone:
everything reachable); `VerifyShippedWorlds`; the 3D blast and the one-stick rule in
`VerifyInteractionSystem`.
