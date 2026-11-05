# Target reach: every treasure, egg, key and exit of the campaign, and the evidence it is reachable

Generated 2026-11-05 by `python3 tools/audit/target_reach.py build-probe/play-pass-25 --md ...`
(play pass 25, the build with BD-0073; classes in the tool's docstring). `tools/levelcheck.py`
alone flags targets in 63 of 78 worlds because it does not model vehicles, crates, dynamite or
timed passes; this puts its graph together with where each playthrough actually stood.

Result: of 921 targets, 216 levelcheck reaches from the spawn, 582 the playthroughs take or stand
at (every treasure and every exit: all 78 runs pass and take all their treasures), 94 levelcheck's
graph reaches from the cells the playthroughs stood on (a patrol's lane too: he was there) with
its `--rides` moves (off a lift anywhere along its track, the helicopter's flight), 4 behind
treasure doors the playthroughs' treasures open (061, 124), 13 on the worlds' own check stages
(055's overcraft shaft, 199's key doors), 12 eggs only with timing, **0 unknown**.

The 12 timed eggs, probed 2026-11-05 by short runs (a playthrough's script up to the place, then
a detour; the egg counter rose in each but 053's, where his lives row was already full):

| world | egg(s) | the detour | taken |
|---|---|---|---|
| 051 | (16, 20, 55), (18, 20, 55) in the bay at the end of the enemy tank's lane | after `go 19.6 45`: `near 17 20 38 0.3` (the tank at the lane's north end), `wait 30`, down the lane `go 17 46`, `go 17 53`, `go 16 55`, `go 18 55` | both |
| 054 | (44, 40, 58) in gallery G2 behind its hanging mines | off lift 46 at walk 40: `go 46 61`, `go 44 61` (the row clear of the mines), `go 44 58.1` | yes |
| 111 | (78-81, 34, 45) in the exit room | with the helicopter past the mine corridor: `fly X 34.10 45.1` for X 78-81 (low: at 34.6 the helicopter passes over them) | all four |
| 122 | (44, 14, 64) in the bulldozer's south tunnel | after treasure 7: `go 34 65`, `go 36 64`, `go 44 64` (lane z 64, a cell beside the bulldozer's z 63) | yes |
| 053 | (78/86, 27, 85/91), the four corners of the arrow's cage (twenty follow stars sleep round the arrow) | after the playthrough's luring: wake rows z 85-87 from the doorway three times (`go 82 85.2/86.2/87.2`, back out, `go 80 82.3`, `wait 700`: they burst on the cage's north wall), then `go 82 85`, `go 78 85` (or 86), `go 78 91` (or 86) | Blupi stood on all four (lives at 10: an egg stays). The near two are safe; **from the far two he died on the way back**, woken stars of rows z 89-90 homing into the side lane (three tries, also luring them first from z 86.8 / 88.6). Taking those two and surviving is not shown: a question for the owner's play |

Also probed: 062's (47, 11, 60) beside the big creature's corridor (taken; now `from-run`). The
probes found BD-0073 (eggs are lives: none is taken at ten lives).

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
| world057 | 4 | 7 | 3 |  |  |  |  |  |
| world058 | 2 | 13 | 4 |  |  |  |  |  |
| world060 |  |  |  |  |  |  |  |  |
| world061 | 4 | 19 |  | 3 |  |  |  |  |
| world062 | 13 | 9 | 1 |  |  |  |  |  |
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
| world081 | 2 | 1 | 9 |  |  |  |  |  |
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
| world105 | 3 | 7 | 9 |  |  |  |  |  |
| world106 | 12 | 12 |  |  |  |  |  |  |
| world107 | 1 | 4 | 4 |  |  |  |  |  |
| world110 |  |  |  |  |  |  |  |  |
| world111 |  | 5 | 2 |  | 4 |  |  | egg (78, 34, 45); egg (79, 34, 45); egg (80, 34, 45); egg (81, 34, 45) |
| world112 | 5 | 2 | 2 |  |  |  |  |  |
| world113 | 2 | 2 | 2 |  |  |  |  |  |
| world114 | 2 | 23 | 8 |  |  |  |  |  |
| world115 | 7 | 7 | 4 |  |  |  |  |  |
| world120 |  |  |  |  |  |  |  |  |
| world121 | 7 | 3 |  |  |  |  |  |  |
| world122 | 3 | 13 | 6 |  | 1 |  |  | egg (44, 14, 64) |
| world123 | 9 | 9 | 1 |  |  |  |  |  |
| world124 | 11 | 11 | 1 | 1 |  |  |  |  |
| world125 | 1 | 2 | 2 |  |  |  |  |  |
| world199 | 11 | 13 | 4 |  |  | 11 |  |  |

totals: levelcheck 216, playthrough 582, from-run 94, earned-door 4, timed 12, staged 13, unknown 0
