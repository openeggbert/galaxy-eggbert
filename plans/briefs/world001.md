# World 001 — the main hub

Built by `tools/worlds/world001.py` → `worlds3d/world001.vwr` (mission 1, region 0, as the
original's header). Validated by `tools/levelcheck.py` (every sign and the exit reachable with the
bars open) and `VerifyShippedWorlds`.

**Reference** (`worlds/world001.txt`, read only): a machine shaft (tile 10) climbed level by level,
a world sign on each side of every level — bottom to top (left, right): (12, 1), (11, 3), (9, 4),
(2, 7), (5, 10), (6, 8) — and at the top two rows of gold bars (183, twelve in all) before the exit
(→ bonus world 199). The blocks above spell DADA. Every world is open from the start; one gold bar
opens per completed world (`Decor::AdaptDoors`, `OpenGoldsWin`); Blupi starts beside the sign of
the world he last played (`MainSwitchInitialize`).

**3D design**: a stepped machine ziggurat, seven tiers two blocks apart (each a jump up), the
twelve world signs in one-cell niches in the south faces, paired per tier as the original pairs
them per level (left x 45, right x 55), a lit panel over each. The summit (walk 13) holds a
roofed one-wide tunnel along z 44 with the twelve gold bars (x 45–56, opening from the entrance
at x 57 inwards) and the exit at its far end (43, 13, 44); DADA stands above it, read from the
south. A parapet rings the bottom tier. Galaxy's own showcase sign (414 → world 999) sits in the
bottom tier's east face. Spawn (50, 1, 75) facing the tower.
