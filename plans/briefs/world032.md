# Mission 032 — the mountain and its two keys (the village)

Built by `tools/worlds/world032.py` → `worlds3d/world032.vwr` (mission 32, region 8 as the
original: the green backdrop). `--check STAGE out` writes the staged variants for
`levelcheck.py` (below).

## Reference

`worlds/world032.txt`, played at 1:15:46–1:20:43 of the video (no treasures, 0/0). From the
start in a street of houses: a bulldozer (ObjectType 4) driving to and fro (90 ticks, 20 stop);
a lollipop (26) at the street's end — lick it with Action and Blupi jumps very high for a while
(`m_blupiPower`, jump speed 26 instead of 16; it comes back after licking) — to climb a stone
mountain; a wasp (44) on the climb (its touch puts Blupi in a balloon); a floating island with
palms and a second lollipop; a fish (17) in a small pool one deep; three eggs in a nook; on the
top a village with three birds, the red key (49) in a house with an egg; the red door (334)
opens the way down into a machine room (tiles 78–84) with lightning (305 under 304 emitters: on
for 2.5 s, off for 2.5 s) and mines riding up and down (16, 40 ticks), the blue key (51) in it;
the blue door (336) closes the room of star tiles (231) with the red arrow and two mines sliding
across it (30 ticks). Objects: bulldozer, 2 lollipops, wasp, fish, 3 birds, 4 eggs, 2 keys,
5 moving mines, the exit.

## 3D design

Raw cells, Y up; everything on stone in the region-8 backdrop.

| Part | Where | Lesson |
|---|---|---|
| Start lane and street | lane x 9–11, z 85–93, spawn (10, 5, 92) facing −Z; street x 8–42, z 81–83 (walk 5) heading +X, houses along its north side (facades on +Z, toward the street), a parapet on the south | The bulldozer drives x 24–30 through a gate where two houses narrow the street to one cell (x 26–28): follow it through and step aside, or jump it |
| Lollipop 1 | (40, 5, 82), under the first cliff | Lick it (Action), then the high jump onto ledge L1 five above |
| L1, the south ledge | x 38–78, z 74–80, walk 10, heading +X | The wasp sweeps across it at x 58 (12 ticks, 4 stop) |
| L2, up the east face | x 73–78, heading −Z: walk 10, 12 (z 58–65), 14 (z 36–57) | Two ordinary steps; the power from lollipop 1 is spent by now (the way is ~80 cells) |
| The island | x 80–86, z 42–50, walk 14, one cell east of L2, a rock cone under it | Palms (BigDecor 16, 18) and lollipop 2: the high jump up the six-block cliff from L2 onto L3 at z 35 |
| L3, the north ledge | x 40–78, z 22–35, walk 20, heading −X | The fish pool across it (x 60–61, one deep, the fish swims across it, 15 ticks / 47 stop); the egg nook (three eggs) in the mountain's face at x 51–53; a step at x 44–47 (walk 22) up to the summit |
| The summit village | the mountain's top x 44–72, z 28–73, walk 24 | Three houses and trees; the key house (x 56–62, z 46–52) with a room behind an open doorway: the red key and an egg; a bird at Blupi's height before the doorway, two higher |
| The gatehouse | x 46–50, z 30–34 on the summit | The red door (two cells of 334) in its south wall; behind it a shaft drops 16 cells into the mountain |
| The machine gallery | x 46–50, z 31–59, walk 8, six high, lined with machine panels | Lightning curtains across it at z 38 and z 50 (emitters 304 in the ceiling, red-dot tiles 3 in the floor): pass while they are off; at z 43–45 the gallery narrows to three lanes with a mine riding up and down in each (in step); the blue key in an alcove at (53, 8, 55) |
| The exit room | x 46–54, z 61–70, behind the blue door (two cells of 336) at z 60 | Star tiles all round; two mines slide across it in opposite directions; the red arrow at (50, 8, 69) |

## Checks

- `levelcheck.py` in stages: the world itself stops at the bulldozer's gate (the check avoids
  every cell an enemy sweeps); `--check timed` (bulldozer, wasp, fish, the bird before the
  doorway, the moving mines and the lightning left out, as when passed at the right moment)
  stops under the first cliff; `--check power` (also a stair where each lollipop jump goes)
  reaches the red key but not the gallery; `--check solved` (also both doors open) reaches both
  keys, every egg and the exit.
- `VerifyShippedWorlds`.
- Live (`--mission 32`, `--spawn`, and probe copies through `QA_LIVE_WORLDS`): the start lane
  looking at the street's houses; the bulldozer through the gate; lollipop 1 licked (gauge
  shown) and the high jump onto L1; the wasp sweeping L1 and lifting Blupi; the island and
  its lollipop; a probe with a lollipop at the cliff's foot: licked, the jump clears the six
  blocks onto L3; the fish swimming across its pool; the key house's room and the red key; a
  probe with the red key beside the gatehouse: the door opens and Blupi drops into the
  gallery; the lightning curtains drawn and killing while on; the mine gate; a probe with the
  blue key beside the blue door: it opens onto the star room; the sliding mines.
