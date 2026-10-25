# Mission 033 — the haunted house of the eight wasps (the village)

Built by `tools/worlds/world033.py` → `worlds3d/world033.vwr` (mission 33, region 15 as the
original: grey smoke). `--check STAGE out` writes the staged variants for `levelcheck.py`.

## Reference

`worlds/world033.txt`, played at 1:20:50–1:25:34 of the video (0/10 → 10/10 at 1:24:43). The
inside of a great house: walls of blank window panels (190, 2119 of them), dark arches (198),
cobwebs (401–403), a few windows and the red house door (192). Rooms on several levels joined
by passages hold ten treasures; 41 mines — most hanging under ceilings (ObjectType 3), the rest
on floors; eight wasps (44) whose touch puts Blupi in a balloon that carries him up to the
ceilings (a hanging mine pops it); two eggs; a drink (30: Action, then Blupi is unseen for a
while — `m_blupiHide`, mines and wasps leave him alone) just before the thickest knot of mines,
behind which lies the last treasure. The red arrow stands beside the start: it counts only
with all ten treasures (`m_nbTresor >= m_totalTresor`), so the mission ends where it began.

## 3D design

A house block x 28–72, z 26–70, y 0–24, all panel tiles, carved into three floors (walking
levels 5, 12, 19; rooms five high on the lower two, four in the attic); stone flags on every
floor (a panel tile's flat top face shows stripes in 3D); window and door tiles on the walls
turned to +Z, cobwebs in the rooms' top corners, doorways two high under a frieze of arches.
Raw cells, Y up.

| Part | Where | What |
|---|---|---|
| Entrance hall | x 44–56, z 58–68, walk 5; spawn (50, 5, 65) facing −Z | The red arrow at (46, 5, 59) before the house door: seen at the start, used at the end |
| Atrium | x 42–58, z 38–56; its middle x 45–55, z 41–53 open to the roof (y 22) | Treasure in the middle with a wasp sweeping over it, a wasp along the low west side; mines hang in the doorways to the hall and the north room; six mines hang under the roof over the open middle, where a balloon ends |
| Ground floor wings | west x 30–40, east x 60–70 (z 38–56), north x 42–58 (z 28–36) | West: a treasure, the stair up (three steps of two through the ceiling). East: a treasure on a pedestal two high with mines hanging a jump above its west and east sides (jump up from north or south), two mines hanging inside the doorway. North: a row of four floor mines, a treasure and an egg beyond |
| First floor | gallery round the atrium (walk 12), wings above the lower ones, a room over the hall | Gallery: three floor mines, a wasp along its east side, a treasure on its south side. West wing 1: a treasure under three mines hanging a jump high, two mines in its doorway. North room 1: a treasure among three floor mines and a wasp. Over the hall: an egg. East wing 1: the stair up to the attic |
| Attic | east x 60–70 (walk 19), the bridge across the atrium (x 45–55, z 46–48), west x 36–40 | East: a treasure, a wasp across the room. The bridge: a treasure in its middle, two fast wasps sweeping across it. West: the drink and a wasp; then the passage to the last treasure, filled across by two rows of floor mines with mines hanging a jump above them — crossed only unseen, never jumped |

Objects as the original's: 10 treasures, 41 mines (25 hanging), 8 wasps (12–30 ticks, 4
stop), 2 eggs, 1 drink.

## Checks

- `levelcheck.py` in stages: the world itself reaches six treasures and the exit (the check
  avoids every cell a wasp sweeps); `--check timed` (wasps left out) reaches all but the last;
  `--check drink` (also the mine knot left out, as crossed unseen) reaches all ten and the exit.
- `VerifyShippedWorlds`.
- Live (`--mission 33`, `--spawn`): the start view with the red arrow before the house door and
  the arched doorway to the atrium; the atrium with its wasps and treasure; the stair hole; the
  east pedestal and its hanging mines; the gallery; the bridge with its wasps and the roof's
  mines; the drink taken and the knot crossed to the last treasure (1/10 counted). A probe
  confirmed the drink's immunity: with it Blupi walks through a row of mines, without it he
  dies. First look fixes: striped panel floors replaced by stone, window tiles kept off doorway
  jambs (their side faces showed), the knot's passage made four high for the camera.
