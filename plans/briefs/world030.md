# World 030 — the hub of world 3 (the village)

Built by `tools/worlds/world030.py` → `worlds3d/world030.vwr` (mission 30, region 16 as the
original). World 3's tiles and pieces: `tools/worlds/village.py`.

**Reference** (`worlds/world030.txt`, video 1:12:46, 1:15:42, 1:20:43, 1:25:34): a village street
on grass (107) under the region-16 sky. At each end a stone mound (35/156) with conifers
(BigDecor 20-22); in the middle a terraced house of four storeys, each storey set back and one
step to the right of the one below, its walls window tiles (186-190) under slate roofs with
dormers (193-197). Mission 1's sign (174) on the ground floor beside the red house door (192);
missions 2, 3, 4 (175-177) each one storey higher, behind a blue bar (182). Blupi starts at
sign 1. A bar slides up once the mission before it is won.

**3D design**: a village green (x 34–66, z 28–64, walking level 5) on a block of stone floating in
the sky, stone walls three high along its west, east and north sides, a stone parapet along the
south, four conifers beside the side walls.
- The terraced house (x 40–60, back at z 30): storey 1 fronts at z 47, storey 2 at z 44 from
  x 43, storey 3 at z 41 from x 46, storey 4 at z 38 from x 49. Each storey is one wall row
  under one slate row, so each roof is a terrace two blocks above the last: one jump.
- Signs in the storeys' facades, one step further east per storey as the original's stair:
  1 at (44, 5, 47) open, beside the house door at x 54; 2 at (48, 7, 43), 3 at (52, 9, 40),
  4 at (56, 11, 37), each two deep behind its bar in the facade, under the roof row (no way
  round a bar). Blupi arrives on the terrace outside the bar last opened.
- Facades face +Z (the window tiles show only there); their end cells are blank-window tiles
  (a window tile's side faces show a dark swatch). Two stone chimneys on the top roof.
- Spawn (44, 5, 52) facing −Z, in front of sign 1.

**Checks**: `levelcheck.py` (all four signs reachable, bars counted open); `VerifyShippedWorlds`.
Live (`--mission 30`): the start view shows storeys 1 and 2 with sign 1 and bar 2; climbed
the terraces to the top with one jump each; the house's west end and roofs looked at (window
corners replaced by blank tiles after the first look).
