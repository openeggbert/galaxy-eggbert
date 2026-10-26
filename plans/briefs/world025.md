# Mission 025 — the castle's trial and world 2's golden key (white palace, region 30)

Built by `tools/worlds/world025.py` → `worlds3d/world025.vwr` (mission 25, region 30).

## Reference

Mobile Eggbert's `worlds/world025.txt`, played at 3:27:28–3:36:10 of the full-game video
(`video-evidence/w025/`): world 2's last test, every lesson once more round a castle. Blupi
starts at the foot of a shaft whose twin lifts (36 ticks) rise to the golden key (ObjectType21),
which waits until the four treasures are found. A tower is climbed on a ladder of small lifts
shuttling one cell in alternate directions (10 ticks); a lightning column (304/305) is timed; a
switch (384) stops a row of saws; lava pits are crossed on balustrades and small lifts past
mines; skateboards (5) run over mines; four sticks of dynamite blast a cellar of 41 crates with
helicopters at its bottom; a water shaft is swum past six fish; a helicopter crosses lava under
two mines moving up and down. 4 treasures, 5 eggs, 41 crates, 4 dynamite, 9 helicopters, 14
mines, 2 moving mines, 6 fish, 5 skateboards, lava, saws, water. The golden key completes
world 2 and returns to the main hub.

## 3D design

A keep in the middle, the lessons round it in the original's order; falls off the open parts
are falls to death.

| Part | Where | Lesson |
|---|---|---|
| Keep | courtyard x 43–57, z 43–57, walk 11, walls six high; spawn (50, 11, 56) facing −Z | the key tower before him, its chamber (walk 19) reached on twin lifts (48 ticks for eight cells); the golden key inside with an egg and the statue; palms and columns |
| Ladder tower | x 28–36, z 46–54, walls to y 28, through the west door | six pairs of small lifts one above the other, each two higher, shuttling one cell along X in alternate directions (10 ticks, stop 10): the next one up is always one cell beside; three eggs on the top landing (walk 25) |
| Lightning bridge | one wide along X, walk 25, x 12–27, over the void | a lightning gate (emitter on a lintel, four cells of lightning) to walk through between flashes; a switch, then ten saws in the bridge's row (the switch reaches them); treasure 1 on the far platform |
| Lava terrace | x 8–40, z 56–68, walk 11, below | dropped onto from the far platform; lava crossed on a one-cell shuttle (10 ticks) and a four-cell ferry (30 ticks) between balustrades carrying mines; treasure 2 on the last balustrade |
| Skate corridor | x 41–58, z 61–63 | five skateboards; a band of nine mines three deep under a one-high slot: only the skateboard runs them over, and no helicopter passes it |
| Hall | x 59–79, z 55–78, walk 11, roofed | two sticks on the depot; the cellar's mouth (x 65–69, z 63–67) plugged by 25 linked crates: one blast clears it; a stack of four crates by the wall |
| Cellar | x 62–74, z 60–72, walk 1, brick, ten below | two sticks; two rooms behind doorways of six crates: the helicopters' (five) and treasure 4's; out by helicopter through the mouth (the hall is roofed, the corridor sealed, so the helicopter stays there) |
| Water tower | x 73–79, z 43–54, roofed with the lava hall | from the hall's pool through a tunnel under its wall, then fourteen cells up past six fish (along X and Z, six heights) to the lava hall's floor; jump out |
| Lava hall | x 54–79, z 22–41, walk 23, roofed | four helicopters on the pad; a channel three wide over lava, solid on both sides, with two gates leaving one cell where a mine moves up and down (30 ticks): fly through low when it is up; treasure 3 on the west ledge; the west door to a bridge back over the keep, from which Blupi drops into the courtyard |

Object counts follow the original: 4 treasures, 5 eggs, 41 crates (25 + 12 + 4), 4 dynamite, 9
helicopters, 14 mines, 2 moving mines, 6 fish, 5 skateboards; 16 lifts (the ladder's lifts are
pairs, two wide).

## Checks

- Staged `levelcheck.py` (`world025.py --stage K`, `build-probe/world2-agent/stagecheck.py 025
  0 … 7`; the ladder, the helicopter flights and the swim stood in by lifts, the lava hall's
  lava by floor, since levelcheck has none of them): as built only the key chamber's egg and
  the key; + ladder: the landing's eggs; + switch: treasures 1 and 2; + skateboard: the hall;
  + dynamite: treasure 4; + helicopter, swim, flight: treasure 3 and the key with all four.
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 25`, `build-probe/qa-shots/world2/w025*/`): the keep and the key tower; the
  ladder's lifts (first person) and Blupi getting onto them by timed jumps (open probe copy); the
  lightning gate, the switch and saws, the terrace; the skateboard run over the band of mines
  through the slot into the hall; a stick at the plug: all 25 crates gone, Blupi drops into the
  cellar; a cellar helicopter flown out through the mouth and left in the hall; the dive through
  the tunnel and the swim up the tower past the fish, with air to spare, and the jump out; a
  helicopter flown over the lava into a gate, killed by the moving mine when mistimed; the
  golden key touched (probe copy without treasures): world 2 ends, Blupi arrives in the main hub.
