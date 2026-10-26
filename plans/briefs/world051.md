# Mission 051 — the station and the cistern (world 5, region 24)

Built by `tools/worlds/world051.py` → `worlds3d/world051.vwr` (mission 51, region 24 as the
original); `--stage K out` writes the staged variants for `levelcheck.py`. World 5's tiles and
pieces: `tools/worlds/station.py`.

## Reference

`worlds/world051.txt`, played at 4:32:20–4:37:14 of the video (`build-probe/world5-agent/video/w051/`;
0/2 → 2/2 at 4:34:02, teleporter at 4:35:00). World 5's first mission, two places joined only by
teleporters. A metal station (78–84, girders 201): Blupi starts beside a shaft of four lifts
shuttling five cells in alternate directions (50 ticks, stop 10), the red arrow at its top
(counted only with both treasures); west of the start a creature's tank (ObjectType33) patrols
and fires along its line at each turn, two eggs past it; blue panels (10) and a column of four
fans (126) blowing −X over moving mines (30 ticks) in the wind (110), a lift shuttling across a
shaft and teleporter 330. A brick cistern (261–263): the arrival over a water tank, fish (17), a
channel into a gallery with the two treasures behind a moving mine, plants and water weed
(BigDecor 23–25, 203), cobwebs; the treasure door "2" (422) onto a corridor of mines, hanging
mines, moving mines, three saws (378) and a second tank, a drink (30: unseen and untouchable
for 20 s, `m_blupiTimeShield` 100) at its start and teleporter 331 at its end, landing Blupi at
the top of a shaft that falls into the start room. 2 treasures, 6 eggs, 10 fish, 12 mines, 3
hanging mines, 8 moving mines (one still), 2 tanks, 5 lifts, the drink.

## 3D design

The two places stand apart in the grid; raw cells, Y up.

| Part | Where | Lesson |
|---|---|---|
| Atrium | x 24–37, z 37–52, walk 20, plate walls to y 40 (south wall low for the camera); spawn (27, 20, 50) facing −Z | The lift ladder: four levels two cells apart, each a 2 × 2 platform shuttling five cells along Z (50 ticks, stop 10), alternate directions; levels 1 and 3 in lane x 30–31, 2 and 4 in lane x 32–33, so each next level passes beside the last and Blupi jumps across and up as they pass (in one lane the level above would be a ceiling). From level 4 a jump east onto the exit block (x 34–37, walk 29): the arrow, an egg, a plant. A still mine in the north-west corner |
| Return ledge | x 24–28, z 37–41, walk 38 | Teleporter 331's partner hangs over it; Blupi arrives one cell +Z and walks off into the atrium |
| W1, the tank's lane | corridor x 13–22, z 44–46, walk 20; lane x 17, z 37–55 | The tank drives the lane (140 ticks, stop 23) and fires along it at each turn; cross between its shots. The two eggs in the bay at the lane's south end, in its line of fire |
| W2, the fan shaft | x 4–12, z 38–50, walk 20, roofed | Blue panels P1 (walk 22) and P2 (24), a jump each; the wind deck (x 7–11, z 44–45, walk 26): four fans at its east end blow −X, the wind carries Blupi west onto the landing; the fans' mouths (x 11) kill. A mine sweeps across the deck at x 8 (pass when it is in the other lane), another sweeps P1. A ferry (two lifts, 30 ticks) to the alcove with teleporter 330 |
| Arrival room | x 69–77, z 34–39, walk 40 | Blupi arrives beside 330's partner and walks into the tank; its floor is two above the water, so there is no way back (the original's arrival has no floor either) |
| Tank, channel | tank x 69–77, z 41–50, water y 29–37; channel x 63–68, z 44–46, y 29–31 | Dive about six cells, swim the channel west, rise into the gallery's pool: about 12 s under water of the 25 s air. Six fish in the tank at six depths, two in the channel, two in the pool; water weed |
| Gallery | x 50–62, z 36–54, walk 34 | Climb out of the pool onto the floor; stone pillars, plants, cobwebs; the two treasures at the west end behind a mine sweeping along Z; two eggs in the corners |
| Door, drop | treasure door "2" at x 55–56, z 55, two high; shaft x 54–57, z 56–59 | The door opens at 2/2; Blupi drops fourteen cells to the corridor's foot |
| Corridor | x 54–77, z 57–59, walk 20 | The drink at its foot; a mine sweeping across, a band of six mines with three mines hanging a jump above, three saws across the floor, a second sweeping mine, a band of six mines under two bobbing ones, the second tank firing along it: crossed only unseen (about 8 s of the drink's 20) |
| Teleporter room | x 78–84, z 55–61, walk 20 | Teleporter 331 (off the tank's line of fire), an egg, a plant |

## Checks

- Staged `levelcheck.py` (`build-probe/world5-agent/stagecheck.py 051 0 … 5`): as built nothing
  (the timed mines and the tank close the west wing, the ladder is not modelled); + ladder: the
  exit and its egg; + timing: the bay's eggs, the fan shaft, teleporter 330 to the cistern;
  + swim: both treasures and the gallery's eggs; + door: the corridor's foot; + drink: every
  target.
- `VerifyShippedWorlds worlds3d`: 80/80.
- Live (`--mission 51`, `--spawn`, probe copies through `QA_LIVE_WORLDS`; shots in
  `build-probe/world5-agent/shots/051-*`): the start view with the ladder; a jump from level 1
  onto level 2 as it passes (eggs placed on level 2's path were collected; re-checked after the
  lift timing fix D43); the wind carrying
  Blupi along the deck onto the landing (mines left out), and with the mine in place a landing at
  the wrong moment killed by it; the ferry to the alcove; teleporter 330 to the arrival room;
  the full swim from the tank's surface through the channel to the gallery's bank (no fish in
  the probe; air about half used) and the jump out; the treasure door opening at 2/2 and the
  drop to the drink; the drink taken and the whole corridor walked through mines, saws and the
  tank's shot; teleporter 331 to the ledge and the fall into the atrium; with both treasures
  the arrow ends the mission (back in the hub). A probe before this showed wind did not push
  Blupi and 347 was a spike; both fixed in the engine (`be3ec9c`, `ecc5605`).
