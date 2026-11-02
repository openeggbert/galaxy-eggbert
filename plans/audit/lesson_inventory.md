# What each world leaves out of its original (generated)

`tools/audit/lesson_inventory.py`; the audit of each finding is in `world_lessons.notes.csv`.
Clusters are the original map's cells (x, y; 64 px tiles), `xN` the cells in the cluster.

| World | Missing (original's cells) | Thinned (original -> Galaxy) | Added |
|---|---|---|---|
| 010 | spikes: (15,5)x6 (56,4)x11 | - | - |
| 013 | - | helicopter 13 -> 4 | - |
| 014 | water: (37,55)x5 | - | - |
| 046 | - | crate 8 -> 1 | - |
| 061 | bridge: (75,60)x7 | - | - |
| 066 | bridge: (10,3)x1 | - | - |
| 084 | crusher: (57,37)x1 (68,71)x1 (70,57)x1 (80,63)x1 (90,65)x1 | - | - |
| 102 | saw: (21,45)x1; fan: (20,45)x2 (68,13)x1; blitz: (34,4)x4 (45,5)x4; switch: (16,45)x1; bridge: (76,92)x5 | - | - |
| 105 | lava: (22,44)x21; teleport: (19,33)x1 (21,1)x1 | - | - |
| 107 | fan: (8,76)x3; temp: (5,40)x6 | - | - |
| 111 | lava: (24,80)x4 | - | - |
| 112 | saw: (38,31)x2; water: (22,26)x11 (19,17)x2 | - | - |
| 113 | fan: (50,35)x1 | - | - |
| 114 | lava: (12,10)x8 (47,19)x3 (64,15)x10 (84,4)x1; fan: (24,13)x9 (23,1)x1; blitz: (34,12)x11 (42,5)x2 (55,9)x4 | - | - |
| 115 | fan: (22,16)x6 (17,25)x4 (42,11)x1 (44,22)x1 | - | - |
| 122 | lift 3; lava: (67,78)x6 (74,68)x5 | - | - |
| 124 | lava: (33,38)x2 (33,65)x5 | mine 18 -> 2; moving mine 4 -> 1 | - |
| 125 | lava: (32,6)x3 | - | - |
| 199 | wasp 1; drip: (32,74)x1 (44,67)x1; fan: (49,96)x1 (57,46)x1 (69,96)x1 (86,67)x9 (85,25)x1 (96,21)x7 (96,45)x1; blitz: (17,21)x3 (34,22)x4; teleport: (1,5)x2 (1,25)x2; bridge: (8,49)x5 (34,3)x1 (73,95)x3; temp: (88,8)x5 | creature 4 -> 1; helicopter 22 -> 6; mine 18 -> 2; moving mine 36 -> 8 | fish 1; crusher |
