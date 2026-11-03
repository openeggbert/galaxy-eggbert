# What each world leaves out of its original (generated)

`tools/audit/lesson_inventory.py`; the audit of each finding is in `world_lessons.notes.csv`.
Clusters are the original map's cells (x, y; 64 px tiles), `xN` the cells in the cluster.

| World | Missing (original's cells) | Thinned (original -> Galaxy) | Added |
|---|---|---|---|
| 013 | - | helicopter 13 -> 4 | - |
| 046 | bar: (47,19)x1 | crate 8 -> 1 | - |
| 105 | lava: (22,44)x21; teleport: (19,33)x1 (21,1)x1 | - | - |
| 112 | water: (22,26)x11 (19,17)x2 | - | - |
| 113 | fan: (50,35)x1 | - | - |
| 124 | - | mine 18 -> 2; moving mine 4 -> 1 | - |
| 125 | lava: (32,6)x3; bar: (46,23)x12 | - | - |
| 199 | wasp 1; fan: (49,96)x1 (57,46)x1 (69,96)x1 (86,67)x9 (85,25)x1 (96,21)x7 (96,45)x1; temp: (88,8)x5; bar: (57,94)x8 (58,24)x3 (91,97)x3 | creature 4 -> 1; helicopter 22 -> 6; mine 18 -> 2; moving mine 36 -> 8 | fish 1 |
