# World 199 — the bonus world (the grand tour)

Built by `tools/worlds/world199.py` → `worlds3d/world199.vwr` (mission 199, region 31 as the
original). Staged variant for `levelcheck.py`: `--stage 1 out.vwr` (the doors open; no mines,
creatures, crushers or saws; the lifts' and the lollipop's climbs as stairs).

**Reference** (`worlds/world199.txt`, video 6:24:04-6:53:43, the win screen 6:53:44): the finale,
reached through the main hub's twelve gold bars. A jungle of slime (341-363) holding a piece of
every world before it: blue machine panel and grey plates, white palace stone with saws, brick with
bookcases, a village street on cobble, cheese, toy balls and star blocks, brown rock. No treasure,
thirty-eight eggs; red, green and blue doors (334-336) and their keys (49-51), duplicates of each in
other parts; twenty-two helicopters, twenty-six lifts, thirty-six moving mines (16), fourteen follow
stars (96), nine birds (20), bulldozers (4), big creatures (54), enemy Blupis (33), the lollipop (26),
inverts (40); the red arrow at the end in a frame of star blocks.

**3D design**: a plaza cut in slime (x 34–66, z 34–66, floor walk 20), a ring gallery round it at
walk 30 where Blupi starts (65, 30, 35), a stair of slime from the ring down to the floor, two lifts
(60 ticks, dwelling 40) back up; a pool with a fish, helicopters, two bulldozers, three birds, four
bobbing mines, four sleeping follow stars, eight eggs. Off it eight enclaves, each of one world's
material, reached by a passage three wide through the plaza's walls:
- at the floor: **east, the machine** (open): a pit of lava crossed on a lift, two mines bobbing
  over it, the red key, two helicopters; **north, the brick library** (red door): a stair of
  bookcases, the green key on its top row, a crate; **west, the village street** (green door): a row
  of houses (windows on their street side, slate on top), an enemy Blupi walking the street, two
  mines, the blue key; **south, the cheese** (green door): a row of crushers across it, two bobbing
  mines, a blue key and a second green key;
- at the ring: **east, the toys** (red door): sleeping follow stars along the way in, a tower of balls
  and star blocks four high that only the lollipop's jump reaches, a second red key on it; **north,
  the palace** (blue door): two rows of saws in its white floor, each stopped by the switch at its
  west end, two birds, a third green key; **west, the rock** (green door): three big creatures, two
  pools of lava, the invert, a third blue key; **south, the finale** (blue door): a hall of star
  blocks, an inner blue door across it, the red arrow beyond in a frame of star blocks.
- Two red, three green and four blue doors; the keys a flag each, so the duplicates are spare
  routes. The red arrow ends the game: the win screen, as the original.

**Checks**: `levelcheck.py`: stage 1 reaches the thirty eggs, every key and the exit; the real world
stops at the first doors. `VerifyShippedWorlds`. Live (`--mission 199`, direct view F3 at a spawn in
every part): the ring and the plaza, each enclave in its material (the machine's lift and key, the
library's bookcases, the village's houses and mines, the cheese's crushers, the toys' tower and a
follow star, the palace's saws and birds, the rock's lava, the finale's star blocks and inner door).
Played live: walking into the red arrow from inside the finale's frame shows the win screen. Not
played through: the tour in one run (thirty minutes in the original), the keys' doors here (key
doors played in 102 and 107).
