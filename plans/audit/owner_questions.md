# Open questions for the owner

The owner (2026-11-02): do not wait at the PC for answers; write the open points here and ask
interactively when the owner asks for it. Each entry: the question, the options, the
recommendation, and what was done meanwhile. Answered entries move to the end with the answer.

## Open

### Waiting on the owner (updated 2026-11-05, MR-0007 and MR-0008)

Answered and built 2026-11-05: Q-012 (B; the ball cubes' plain tops), Q-013, Q-014 ((b), M3D-0006),
053's cage eggs (moved), the playability audit (automated playthroughs suffice), D11's one item (the
treasure doors' number plates and the mission signs' digits sat in front of their post and board:
`83d7e4bc`), OR-0028/0029/0033/0034/0035/0038/0040/0044/0048/0053 (fine), OR-0042 (analysed: the
original's timing, smoother in 3D). Still the owner's:

- D11: any further model that looks wrong (the owner named the sign plates; nothing else so far).
  (OR-0041 answered and fixed 2026-11-05: BD-0074.)

### Q-001 — Grass (107-109, 157) in 3D: how? — DONE (b) 2026-11-04

`Decor/herbe1.rd4` matches the grass tiles (IoU 0.91): a lumpy block of soil with leaves on top.
As a terrain tile it would not join its neighbours (the lumps leave gaps), so the options are:
(a) keep the textured cube and put only the scene's leaves on its top as 3D geometry
(recommended), (b) the whole lumpy model per cell, (c) leave the grass as it is. Meanwhile: as it
is (a cube with the grass texture and its top overlay).
Done 2026-11-04 (the owner: the whole lumpy models with the joining variants): near the camera a
grass cell is `Decor/herbe1.rd4`'s slab twice, back to back (their leaves cover the top), with the
crosswise slab where the row ends; farther off the cube and top plate as before (Q-012,
`plans/evidence/terrain-reliefs-7-worlds.png`).

### Q-002 — The lightning bolt (305-307): it has no Ray Dream scene — DONE (a) 2026-11-04

The emitter is now `Decor/eclair.rd4`. The bolt between two emitters was painted, not modelled.
Options: (a) a generated thin glowing 3D bolt following each sprite's path (three shapes taking
turns as the original's frames; recommended), (b) keep the sprite plates. Meanwhile: the sprite
plates.

### Q-003 — The clay bridge's surface — DONE (a), the owner's request

The bridge is now its scene's model; its surface is the scene's smooth clay brown. The owner
asked for a clay texture on top. `Decor/terre.tif` (a clay texture) exists but no scene uses it.
Options: (a) map `terre.tif` onto the bridge model's top (recommended), (b) leave the scene's
colour. Done 2026-11-03, (a), since the owner asked for it: `tools/rd4_to_gltf/top_texture.py`
lays `terre.tif` over the exported model from above, recoloured to the scene's mean clay brown so
the calibrated tone stays (`plans/evidence/clay-bridge-terre.png`, before and after). The sprites
show smooth lumps; say if the grain should go again (b).

### Q-004 — The terrain tiles as their Ray Dream models? — DONE (c) for 53 tiles 2026-11-04

`plans/audit/model_sources.md`: 129 scenes are the terrain tiles (rock, cave, wood, techno,
slime, cheese, toys, houses, pipes, grass). Galaxy draws them as cubes textured with their sprite,
so that neighbours join into floors and walls; their scenes are lumpy blocks that would leave gaps
and seams between cells and cost far more to draw. Options: (a) keep the textured cubes (the
recommendation; the sprites are those renders), (b) models for the free-standing pieces only
(pillars, pipes' ends), (c) every tile as its model. Meanwhile: (a).
Done 2026-11-04 (the owner: every tile as its model): 53 tiles, near the camera, as Q-012 describes;
the tiles without a scene of their own stay cubes.

### Q-005 — The electrode (238-242) without its sparks? — DONE (a) 2026-11-04

`Decor/electro.rd4` gives the ball on its spring (IoU 0.79-0.81), but the sparks round the ball
were painted on the renders: the model alone looks dead. Options: (a) the model with the sprite's
sparks as a camera-facing decal (recommended), (b) the model alone, (c) keep the sprite.
Meanwhile: the sprite.
Note (2026-11-04): no campaign world places the electrode (only the gallery 998 and the showcase
999), so the choice touches no mission.

### Q-006 — The glue death (element 168-171) from `Blupi/glu.rd4` — DONE (strands) 2026-11-04

The scene gives Blupi's struggling pose (IoU 0.80-0.85) but converts yellow: the green slime on
him and the strands of glue at his feet are a material and extrusions the converter does not read
yet. Options: (a) teach the converter the scene's glue (recommended, some work), (b) the yellow
pose tinted green, (c) keep the sprite. Meanwhile: the sprite (as for the burn death, which has no
scene at all).

Found since (2026-11-02 night): the scene holds no glue at all, only Blupi's parts, every one
yellow: the sprites' green, strands and puddle were added to the render afterwards (as the signs'
digits were). Done meanwhile, closest to (a): the converter recolours the scene's yellow to the
sprites' green, its flat parts and its face texture's skin (`texrecolour`, the eyes and lashes
kept), and the game draws that model with his own model flattened under it as the dark green
puddle; the strands are not drawn. Still yours to decide: keep this, add strands, or another
option.

Answer (2026-11-04): add the strands. Done: `tools/glue/glue_strands.py` traces the thin green
strands of each painted frame 168-171 (outside the body's silhouette) to centre lines
(`GlueStrands.hpp`), and the game draws them as thin tubes in the strands' own green, turned with
Blupi and stood on the puddle; evidence `plans/evidence/glue-strands.png`.

### Q-012 — The terrain models: what the scenes turned out to be, and B or C? (2026-11-04) — DONE: B; the toys' ball cubes balls only on the sides (2026-11-05)

Found while starting Q-001/Q-004: the 127 terrain scenes are **not blocks**. Each is a slab modelled
for the front view only: 0.15-0.55 of a block deep and up to 1.37 blocks wide (the lumps of
neighbouring tiles overlap, so the cropped renders join). The heavy ones are 30,000-58,000
triangles each (rock, cave, grass); the campaign has 1.32 million exposed terrain cells (world125
alone 56,000 exposed faces). So a cell is drawn as its scene's **relief on each exposed face**
(`tools/terrain_relief/build_relief.py`: the front half, decimated to ~1,500 triangles, which keeps
the lumps; 500 does not), textured by the tile's own sprite projected from the front (so head-on it
is exactly the original tile), and only **near the camera**: farther terrain stays the textured cubes
(the whole world at 1,500 triangles a face would be ~80 million triangles; this machine has an
integrated GPU). The grass gets its own composition: the model as two strips back to back (their
leaves cover the top) and crosswise strips at the row's open ends (the joining variants 107/108/109/157:
middle, right end, left end, single).

Mock-up (`plans/evidence/terrain-relief-mockup.png`; rock above, grass below; the grass's top there is
still the plain per-face relief, to be replaced by the strips):
- **A** today's cubes;
- **B** the relief clipped where it would hang into air: the cell's shape stays exact, so what you
  see is what Blupi stands on (recommended);
- **C** the whole relief with its overhangs: more organic, but lumps stick ~0.18 of a block past
  edges into the air, where Blupi does not collide with them.

Meanwhile: B is being built (C is the same relief with the overhangs kept, a switch).

Found next (2026-11-04): the "127 terrain scenes" of `model_sources.md` came from the survey's
silhouette match, which cannot tell tiles that fill their square apart (every square scene scores
an IoU of 1): the bookshelves 139-141 were given a ball on a plate, the planks 32/33 and the bricks
261 the cave's lumps, the machine panels 6/10/73 one panel. `tools/terrain_relief/match_tiles.py`
now ranks every Decor scene by the tile's pixels, and the 80 tiles with the most exposed faces in
the campaign (98.7 % of them) were checked by eye: `tools/terrain_relief/tiles.txt` lists the 54
whose scene is certain (rocks, caves, cheese, slime, toys, four machine panels, the palace's
facades, the grass). The others stay textured cubes: no scene of the archive is theirs (the bricks
261-263/283, the grey metal plates 78-84/396, the panels 4-8/11-13, the stars, the roof tiles).

Built 2026-11-04 (B): now 158 tiles in `avatars3d/terrain/` (5.7 MB), each relief ~700 triangles, drawn
within 8 blocks of the camera's target and built on worker threads (093: 1.5 million triangles near the
camera, 0.4 s to build at load; a door opening rebuilds only its chunk). Measured on this machine's GPU
(AMD Phoenix, radeonsi; SDL's offscreen driver, each playthrough's whole run including its load, a frame
every tick): 021 0.40 -> 0.69 ms a frame, 082 0.28 -> 0.43, 093 0.93 -> 1.40, 101 0.27 -> 0.50, 125
1.21 -> 1.22: well inside 16.7 ms. Evidence
`plans/evidence/terrain-reliefs-7-worlds.png`. Still yours to say: B or C; whether the toys' floors
(a ball standing proud of its plate, the plate 0.37 under the face: Blupi walks on the balls' tops)
should rather have the plate at the face. Also done: the cave's edge plates 285/302 (3,000 cells each in
090-095) as grotte01/grotte18's reliefs. Open on my side: the little-used square tiles whose scene is not
confirmed yet.

## Answered

### Q-000 — Wind and fans along Z (2026-11-02)

Answer: yes, add turning fans and wind 90 degrees (pushing along Z) as a 3D adaptation (M3D-0005).

## Q-007 — the follower star's wake: the original's box or Galaxy's sphere? (2026-11-03) — DONE: the box

- The original wakes a sleeping star when Blupi is within 144 px across and from 149 px over
  him to 158 under (a box, `MoveObjectFollow`); Galaxy uses a sphere of 132 px (2.06 cells).
  The box also wakes stars two cells off on a diagonal and two cells over him.
- With the box, 052 and 053 (101 stars, laid out with the sphere) fail their playthroughs: the
  stars round the treasures wake as he lands or passes. 053 can be adapted (done once: a column
  a cell higher, five route changes); 052 needs its helicopter section's stars re-laid.
- Options: (a) the box, with 052/053 re-laid (faithful; a day's level work); (b) keep the sphere
  as the 3D adaptation (as now). Recommendation: (a), when the levels are revisited.
- Taken 2026-11-03 as recommended, (a): the box, 052 and 053 re-laid (BD-0053). Reverting is one
  line in `InteractionSystem` and the two worlds' generators in git.


## Q-008 — world 105's lava floor: the void, or a cheaper lava to draw it? (2026-11-03) — DONE (b) 2026-11-04

- The original's helicopter cavern (105) has a floor of lava: a fall burns Blupi (Clear3). Galaxy's
  hall is bottomless: a fall ends in the angel (Clear2). Same end of a life, different death.
- A full lava floor under the hall (61 x 61 animated cells; the largest lava in any shipped world
  is 848) made the playthrough ~6x slower (185 s against 32 s): the animated terrain mesh is
  rebuilt and uploaded at every animation tick. Caching each frame's mesh instead would cost about
  90 MB of vertices for that floor, too much for the web build.
- Options: (a) keep the void (as now, documented as a 3D adaptation, LV-0063); (b) a cheaper lava
  for large pools (a merged, coarser surface for cells with lava all round, the full wave only at
  the edges), then the original's floor. Recommendation: (b) later, as a renderer task that also
  speeds up the lava worlds (084, 094, 106).
- Meanwhile: the void stays; 105's teleporter pair (the hut and the roof) is in.

## Q-009 — Android: KVM or a phone for a real test (2026-11-03, OR-0011)

- The APK builds and starts in the emulator (assets copied, the title scene drawn:
  `plans/evidence/android-emulator-loading.png`), but this workstation has no `/dev/kvm`
  (`kvm_amd` is not loaded; VirtualBox's `vboxdrv` is). The x86_64 emulator then runs in software
  emulation: Android's own services time out, and the game needs minutes per screen. Enough to
  show that the port starts, not to play it or judge touch and tilt.
- Options: (a) load KVM for a session (`sudo modprobe kvm_amd`; VirtualBox's XP VM cannot run
  at the same time, so only while it is off), then the emulator runs at full speed;
  (b) a phone with USB debugging (`./gradlew assembleDebug` builds arm64-v8a too;
  `adb install -r`); (c) leave Android at "builds and starts".
- Whatever the choice: on a GPU whose vertex uniform limit is 256 (the emulator; many Adreno
  phones) no 3D model is drawn until CNA fixes UB-007; a Mali or newer Adreno phone may not hit it.
- Nothing is changed on the machine without the owner.

## Q-010 — the cheese with holes (339): see through, or the holes as dents? (2026-11-03) — black dots fixed, holes now filled

- Tile 339 is cheese with two round holes that are clear in the sprite (857 of its 4,290 texels):
  in the original's 2D the backdrop shows through them. In 3D it is a cube drawn with an alpha
  test, so through a hole one sees the cube's own inside, unlit: black dots on the cheese walls
  (18 cells: 042 and 045; found by the play pass, `build-probe/qa-shots/cheese339/v.png`).
- Options: (a) see-through holes (the cube's inner faces drawn, so a hole shows the room behind
  the wall where there is one, cheese where the wall goes on); (b) holes as dents: the clear texels
  drawn in a darker cheese colour, the cube closed; (c) leave it.
  Recommendation: (b) -- the walls stay walls, as everywhere else in the cheese worlds.

## Q-011 — the brick edge pieces (262, 263, 283): ragged edges or whole bricks in 3D? (2026-11-03) — slits fixed, edges now filled

- The brick wall 261 has three edge pieces whose bricks stop short of the tile on one side
  (262 left, 263 right, 283 bottom; clear texels there): in the original's 2D they make a wall's
  ragged end against the backdrop, and the auto-tiling (ED-0007) chooses them. In 3D they are
  cubes drawn with an alpha test, so their clear strip is a slit into the cube: thin black slits
  all over the brick walls (73,773 cells: 124, 123, 125, 122, 121, 034, 051, 120, 199; the play
  pass of 2026-11-03, e.g. `build-probe/qa-shots/v2-after-124/v.png`).
- Options: (a) draw them as the whole brick 261 in 3D (a cube's own shape already ends the wall;
  the editor keeps placing the edge pieces, they just look like the wall); (b) keep the ragged
  edge only on a face that borders air on that side, whole brick elsewhere; (c) leave them.
  Recommendation: (a), simplest and without slits; (b) if the ragged ends matter.

### Q-010 / Q-011 update (2026-11-04)

The black was a bug, not a choice: every tile drawn opaque (under a quarter clear) showed black
where its sprite is clear -- the cheese holes, the brick and slime edge pieces (347-363), 69 tiles
in all. Their clear texels now take the tile's own colour round them (`TileCoverage.hpp`
`FillOpaqueTilesClearTexels`, `plans/evidence/opaque-tiles-clear-texels.jpg`), i.e. option (a) of
Q-011 and the closest to (b) of Q-010. Still the owner's to say: whether the cheese holes should
be see-through or the brick ends ragged on faces that border air.

### Q-008 done (2026-11-04)

Option (b) without coarsening anything visible: all lava cells show the same frame at a time, so
the renderer now keeps each frame's mesh once built (`TerrainRenderer` `m_lavaFrames`) and a
pool's inner cells draw their wave on a 4 x 3 grid (`AppendLavaCell`, VerifyTileFaces). 105's
playthrough with the original's lava floor (61 x 61 cells): 18.7 s against 17.3 s for the void
(it was ~60 s before the cache). 105 has its lava lake again (`tools/worlds/world105.py`): a
fall burns Blupi (Clear3, channel 8), as the original; its playthrough and the 100 chain pass.
`plans/evidence/lava-floor-105.png`. LV-0063 updated.

## Owner's answers of 2026-11-04 (asked interactively)

- **Q-009 Android**: leave it at "builds and starts" (no KVM, no phone).
- **Q-001 grass**: the whole lumpy model; the soil-with-leaves scenes have variants that join with
  the neighbouring grassy soil -- use them (not only the leaves on the cube).
- **Q-002 bolt**: (a) a generated thin glowing 3D bolt along each sprite's path.
- **Q-010 cheese 339**: see-through holes.
- **Q-011 brick edges**: not asked separately; the owner chose only the cheese's see-through holes,
  so the brick edge pieces stay filled (as `4205ae3`).
- **Q-004 terrain tiles**: every terrain tile as its Ray Dream model.
- **Q-005 electrode**: the model with the sprite's sparks as a camera-facing decal.
- **Q-006 glue death**: add the strands.
- **MR-0001 3ds Max**: no licence; leave the .max files unused.

## Q-013 — the lattice cube "textured only outside" (owner 2026-11-04): where was it seen? — DONE: fixed by OR-0069 (owner 2026-11-05)

- Report: "the cube of grids has textures only outside and not inside; it should probably be 3D";
  then "only the outer side is visible on the cube of grids of metal, probably rods as edges".
- What the girder 201 (the metal truss, `kLatticeModelIcons`) is now: its Ray Dream model
  (`Decor/structure1.rd4`) on each side of the cell that does not join another 201, in every
  campaign world that has it (050, 051, 053, 054, 056, 057, 058, 062, 063, 064, 075, 095). Checked
  live 2026-11-04 in 054 and 056, from outside and with Blupi standing inside a tower: 3D bars from
  every side, the far sides' backs visible through the near side.
- Found: the model has its rivets only in front (the sprite camera's side); inside, the plates are
  bare. Mirroring the frame onto its back covers the front plates (its diagonals lie at the back);
  mirroring each rivet through its plate puts them where the diagonals hide them, at twice the
  triangles (19 348 a frame, 20 rivets of 960). Neither was kept.
- Not found: any place where 201 is a flat textured cube (that would be textured outside only,
  back faces culled). Needed: a screenshot or the world and place, and whether it was in the game,
  the editor or the web build.
- Likely explained and fixed by OR-0069 (later the same night): the girder had no collision, and
  the chase camera stops only at blocks with collision (`BlocksCamera`), so it looked through a
  girder tower, or from inside one, where the frames' backs are bare (no plates, no rivets). World
  056's lift beside its tower (`plans/evidence/girder-camera-before-after.jpg`: play pass 20 above,
  the tower between the camera and Blupi; play pass 22 below, the camera at the open side). The
  girder is solid now: neither Blupi nor the camera goes inside it. To be confirmed by the owner.

## Q-014 — the big decorations (BigDecor: trees, palms, plants, houses, machines, palace columns): solid too? (2026-11-04) — DECIDED (b) 2026-11-05

- OR-0069 made the decor tiles that stand in Blupi's way solid. The BigDecor -- the original's
  background pictures (trees 20-22, palms 16-19, plants 23-25, houses 26-27, machines 28-31, the
  kids' toys 36-47, the slime 66-69, the palace's columns and fountain 87-89), drawn in Galaxy as
  their Ray Dream models standing in the rooms -- are still walked through: 316 of them in 35
  campaign worlds (020-025, 030-034, 040-046, 051-058, 070-075, 090-095).
- Options: (a) solid, a column of cells over its anchor as high as its picture (a tree 2-3 cells,
  a plant one); every route through one checked and the worlds adjusted where it blocks a way;
  (b) solid only the large ones (trees, palms, houses, machines, columns, fountain), the plants
  and toys passable as grass; (c) leave them passable (background pictures, as the original).
  Recommendation: (b).
- **Decided (b), built 2026-11-05** (`mechanics_3d_decisions.md` M3D-0006): the palms, trees,
  houses, machines, columns and fountain fill a column of two cells; 025 and 053's routes went round
  a palm and a machine standing in their way.
