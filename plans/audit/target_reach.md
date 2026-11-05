# Target reach: every treasure, egg, key and exit of the campaign, and the evidence it is reachable

Generated 2026-11-05 by `python3 tools/audit/target_reach.py build-probe/play-pass-23 --md ...`
(play pass 23, the 2026-11-05 build; classes in the tool's docstring). `tools/levelcheck.py` alone
flags targets in 63 of 78 worlds because it does not model vehicles, crates, dynamite or timed
passes; this puts its graph together with where each playthrough actually stood.

Result: of 921 targets, 216 levelcheck reaches from the spawn, 589 the playthroughs take or
stand at (every treasure and every exit: all 78 runs pass and take all their treasures), 86
levelcheck's graph reaches from the cells the playthroughs stood on with its `--rides` moves
(off a lift anywhere along its track, the helicopter's flight), 4 behind treasure doors the
playthroughs' treasures open (061, 124), 13 on the worlds' own check stages (055's overcraft
shaft, 199's key doors), **13 eggs only with timing** past a patrol, follow stars or hanging mines
(051's tank lane, 053, 054, 062's big creature, 111, 122: geometrically there, each world's
generator describes the timed pass; not yet taken by a run), **0 unknown**.

| world | levelcheck | playthrough | from-run | earned-door | timed | staged | unknown | not shown (kind and cell) |
|---|---|---|---|---|---|---|---|---|
| world001 | 1 |  |  |  |  |  |  |  |
| world010 |  |  |  |  |  |  |  |  |
| world011 | 5 |  |  |  |  |  |  |  |
| world012 |  | 1 |  |  |  |  |  |  |
| world013 |  | 4 |  |  |  |  |  |  |
| world014 |  | 4 |  |  |  |  |  |  |
| world020 |  |  |  |  |  |  |  |  |
| world021 | 5 | 5 | 2 |  |  |  |  |  |
| world022 | 4 | 13 | 1 |  |  |  |  |  |
| world023 | 4 | 11 | 2 |  |  |  |  |  |
| world024 | 10 | 9 | 1 |  |  |  |  |  |
| world025 | 2 | 7 | 1 |  |  |  |  |  |
| world030 |  |  |  |  |  |  |  |  |
| world031 | 2 | 6 | 1 |  |  |  |  |  |
| world032 |  | 7 |  |  |  |  |  |  |
| world033 | 8 | 5 |  |  |  |  |  |  |
| world034 | 7 | 4 | 1 |  |  |  |  |  |
| world040 |  |  |  |  |  |  |  |  |
| world041 |  | 13 |  |  |  |  |  |  |
| world042 |  | 22 |  |  |  |  |  |  |
| world043 | 3 | 12 |  |  |  |  |  |  |
| world044 | 1 | 23 |  |  |  |  |  |  |
| world045 | 2 | 21 | 6 |  |  |  |  |  |
| world046 | 1 | 11 |  |  |  |  |  |  |
| world050 |  |  |  |  |  |  |  |  |
| world051 |  | 7 |  |  | 2 |  |  | egg (16, 20, 55); egg (18, 20, 55) |
| world052 | 2 | 11 |  |  |  |  |  |  |
| world053 | 2 | 30 | 3 |  | 4 |  |  | egg (78, 27, 85); egg (86, 27, 85); egg (78, 27, 91); egg (86, 27, 91) |
| world054 |  | 4 | 4 |  | 1 |  |  | egg (44, 40, 58) |
| world055 |  | 9 |  |  |  | 2 |  |  |
| world056 | 29 |  |  |  |  |  |  |  |
| world057 | 4 | 6 | 4 |  |  |  |  |  |
| world058 | 2 | 12 | 5 |  |  |  |  |  |
| world060 |  |  |  |  |  |  |  |  |
| world061 | 4 | 19 |  | 3 |  |  |  |  |
| world062 | 13 | 9 |  |  | 1 |  |  | egg (47, 11, 60) |
| world063 |  | 2 | 6 |  |  |  |  |  |
| world064 | 2 | 17 |  |  |  |  |  |  |
| world065 | 1 | 16 |  |  |  |  |  |  |
| world066 |  | 4 |  |  |  |  |  |  |
| world070 |  |  |  |  |  |  |  |  |
| world071 |  | 9 |  |  |  |  |  |  |
| world072 |  | 7 |  |  |  |  |  |  |
| world073 |  | 8 |  |  |  |  |  |  |
| world074 |  | 14 |  |  |  |  |  |  |
| world075 |  | 12 |  |  |  |  |  |  |
| world080 |  |  |  |  |  |  |  |  |
| world081 | 2 | 3 | 7 |  |  |  |  |  |
| world082 | 2 | 8 |  |  |  |  |  |  |
| world083 | 1 | 4 |  |  |  |  |  |  |
| world084 | 1 | 11 | 4 |  |  |  |  |  |
| world090 |  |  |  |  |  |  |  |  |
| world091 |  | 10 |  |  |  |  |  |  |
| world092 | 2 | 9 |  |  |  |  |  |  |
| world093 | 1 | 12 |  |  |  |  |  |  |
| world094 | 4 | 7 |  |  |  |  |  |  |
| world095 |  | 9 |  |  |  |  |  |  |
| world100 |  |  |  |  |  |  |  |  |
| world101 |  | 9 |  |  |  |  |  |  |
| world102 | 2 | 5 |  |  |  |  |  |  |
| world103 | 11 |  |  |  |  |  |  |  |
| world104 | 2 | 8 |  |  |  |  |  |  |
| world105 | 3 | 10 | 6 |  |  |  |  |  |
| world106 | 12 | 12 |  |  |  |  |  |  |
| world107 | 1 | 6 | 2 |  |  |  |  |  |
| world110 |  |  |  |  |  |  |  |  |
| world111 |  | 5 | 2 |  | 4 |  |  | egg (78, 34, 45); egg (79, 34, 45); egg (80, 34, 45); egg (81, 34, 45) |
| world112 | 5 | 2 | 2 |  |  |  |  |  |
| world113 | 2 | 2 | 2 |  |  |  |  |  |
| world114 | 2 | 23 | 8 |  |  |  |  |  |
| world115 | 7 | 7 | 4 |  |  |  |  |  |
| world120 |  |  |  |  |  |  |  |  |
| world121 | 7 | 3 |  |  |  |  |  |  |
| world122 | 3 | 15 | 4 |  | 1 |  |  | egg (44, 14, 64) |
| world123 | 9 | 9 | 1 |  |  |  |  |  |
| world124 | 11 | 11 | 1 | 1 |  |  |  |  |
| world125 | 1 | 2 | 2 |  |  |  |  |  |
| world199 | 11 | 13 | 4 |  |  | 11 |  |  |

totals: levelcheck 216, playthrough 589, from-run 86, earned-door 4, timed 13, staged 13, unknown 0
