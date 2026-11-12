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

- Nothing waits on the owner. Q-015 answered "udelej" and built 2026-11-06 (BD-0077). D11 closed by the owner
  2026-11-06 ("Ne, D11 uzavřít", MR-0009); OR-0041 answered and fixed 2026-11-05 (BD-0074).

### Q-017 — Demo recording, xnjttjpo and the players' galaxies: three choices made meanwhile (2026-11-08) — DONE (kept) 2026-11-09

Built as below (BD-0116-0119); each is a small change if the owner prefers the other option.
The owner kept all three as built on 2026-11-09 (Galaxy; the shipped file; a pause ends it).

- **The name "Galaxy"** for each player's own world system (the owner: "Space, or something else").
  It matches the game's title: each player builds their own galaxy of worlds. Alternatives: Space,
  Universe, "My worlds". Recommendation: keep Galaxy.
- **xnjttjpo saves the shipped world file itself** (`worlds3d/worldNNN.vwr`), as XMISSION wrote
  `data\worldNNN.blp`; the version before each visit is kept in `customworlds/campaign-backups/`.
  (b) would keep edits as per-player copies that override the shipped worlds: safer for players,
  but every world load would have to look for them. Recommendation: keep (a). It is a cheat, and in
  a checkout it lets the owner edit the campaign. A hand edit is overwritten when tools/worlds/
  regenerates the world.
- **A recording ends when Play is left, pausing included**, as in Speedy Blupi (any phase but Play
  stops it). A demo replays only Play, so a pause cannot sit in the middle of one. The recording
  made up to the pause is kept. Recommendation: keep.

### Q-015 — Resume: continue the mission where it was left, as the original? (2026-11-06) — DONE (a) 2026-11-06

The original writes the whole mission in progress when the game loses focus during play and, if the
game is started again with it written, Resume continues from that moment (a clean exit deletes it).
Galaxy's Resume starts the checkpointed mission from its beginning (BD-0077). It matters when the
game ends while in the background: on the web a closed tab, on Android a background kill, on the
desktop a crash. Options: (a) build the snapshot as the original (WorldRuntime, objects, Blupi;
a session of work, every playthrough and a mid-run snapshot test); (b) leave it (desktop first;
Android stays at "builds and starts", Q-009). Recommendation: (a) when the web build is to be
released (a tab is closed easily), else (b). Meanwhile: as it is, recorded as open.

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

## Q-009 — Android: KVM or a phone for a real test (2026-11-03, OR-0011) — DONE: the owner played 1.0.0-rc1 on his phone 2026-11-10

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

## Q-011 — the brick edge pieces (262, 263, 283): ragged edges or whole bricks in 3D? (2026-11-03) — DONE (b) 2026-11-09

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
Q-011 and the closest to (b) of Q-010. Both were settled later: the cheese's holes see-through
(2026-11-04, below), the brick ends ragged towards the open (2026-11-09: (b), asked interactively):
a cell of 262/263/283 at a wall's open end (air on its clear side and across it, the wall going on
behind) is drawn as its four courses ending where the bricks end (`BrickEdges.hpp`,
`TerrainRenderer` `AppendRaggedBrick`); elsewhere a whole cube whose clear texels are the whole
brick 261's (`FillBrickEdgesFromWholeBrick`), which also removed the pale bands the mean fill left
where the worlds scatter the three in walls.

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
  so the brick edge pieces stay filled (as `4205ae3`). Asked on 2026-11-09: ragged only towards
  the open (option (b)), done the same day.
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

## Q-016 — the "zoom" cheat of Mobile Eggbert in Galaxy: feasible, and can the graphics take it? (owner 2026-11-06) — ANSWERED 2026-11-07: option (a) built (OR-0079)

- Mobile Eggbert (`InputPad.cpp:742-811`, `Decor.cpp:589, 668-677, 11630`): the cheat word `zoom`
  cycles the view 100 -> 50 -> 25 -> 12 % -> 100 % (`SetCheatZoom`, the hot-spot zoom times the
  factor, the tile loop widened to fill the larger view), reset when the level is left. It is under
  `#ifdef MODERN`: an addition of the modern port, not the 1998 game.
- Measured 2026-11-06 with a temporary probe (the chase camera's distance x1, x2, x4, x8; GALAXY_PERF,
  xvfb's software renderer, so the times are high in absolute terms):
  hub 070 x1 13.5 ms a frame, 3 models (7k triangles) -> x8 23.2 ms, 15 models (37k);
  world 094 x1..x8 37-41 ms, 3-4 models (22-42k triangles); world 025 x8 fewer models than x1 (a
  wall in the view). The terrain is one mesh of the whole world, drawn whole already, so a far
  camera costs it nothing; the far plane (1000) reaches. The graphics take it.
- The limit is the levels, not the graphics: they are rooms, and the chase camera stops at walls
  (`RaymarchWallDistance`). In 094's cave x4 and x8 look the same as each other (the wall stops it);
  in 025 at x8 it ended looking at a blank surface. Screenshots: `plans/evidence/zoom-cheat-094.jpg`
  (x1, x2, x4, x8) and `plans/evidence/zoom-cheat-open.jpg` (070 x1, x8; 025 x1, x8).
- Options: (a) the cheat as a farther and higher chase camera (x2, x4, x8; the elevation rising
  with it toward a bird's-eye view), still stopped by walls: open hubs and outdoor worlds show much
  more, caves little; a few hours. (b) as (a) plus a cut-away (the blocks between the camera and
  Blupi, or above his floor, left out or see-through), so that a cave shows too: needs the terrain
  drawn per chunk or a clip in the shader; days. (c) not at all (it is not the 1998 game's).
  Recommendation: (a), behind the cheat word as in Mobile Eggbert, reset on leaving the level.

## Q-018 — the release packages (owner's goal of 2026-11-10): Windows, glibc 2.35, version, where to publish

`tools/release/make_release.sh build-cna-opengl3 <out>` makes the Linux archive and the Android APK
with `SHA256SUMS.txt` and `RELEASE_NOTES.md` (1.0.0-rc1 built into `build-probe/release/`). Open:

1. **Windows zip.** Galaxy has never been cross-built for Windows here: it needs a MinGW build tree
   of CNA + Galaxy with SDL3 for MinGW (MinGW-w64 GCC is installed), and CURRENT.md asks for a live
   3D run on Windows before Windows is called supported. Build it (a new tree: which name?) and
   who runs it on Windows?
2. **glibc 2.35.** The Linux program needs glibc 2.38 (its C23 `strtol` symbols, GCC 14's
   libstdc++) and the distribution's FFmpeg; Debian 13 has no older toolchain. An archive for
   glibc 2.35 needs a build on Ubuntu 22.04 (a container or chroot: neither docker, podman nor
   debootstrap is installed). Install one of them, or accept 2.38 (Debian 13, Ubuntu 24.04+)?
5. **(2026-11-12 evening) glibc 2.35, the owner chose to build it.** Done: a rootless Ubuntu 22.04 root
   (`~/deps/ubuntu-22.04-root`, `~/deps/ubuntu-22.04-enter.sh`, GCC 13, the Android SDK's CMake 4.1.2),
   `tools/release/make_linux_glibc235.sh`, SDL3 built there. **Open:** CNA needs libcurl 7.85 and 22.04 has
   7.81; building curl 8.5 from Ubuntu 24.04's signed source in the root was refused by the agent's
   permission check ("code from external"). Allow that build (or another source of libcurl >= 7.85), or
   ship the older-systems archive without the network game? **Answered 2026-11-12: skip it** (glibc
   2.38+ stays the minimum; the root and the script stay ready). Next: replay the whole campaign, the
   models' fidelity, controls and camera.
3. **The version number.** `VERSION.txt` said 1.0.0-rc1 (Android's versionName reads it). Keep it?
   **Answered 2026-11-11: 0.1.0, not released yet**; versioned as CNA is (`docs/releasing.md`), the
   main menu shows the version and the build's date and time left of Play. The 0.1.0 packages for
   Windows, Linux and the web are built (`build-probe/release/0.1.0/`, 2026-11-11); the 1.0.0-rc1
   Windows zip is not used.
4. **Where to publish** (GitHub release of openeggbert/galaxy-eggbert? the web build on the site?).
   **Answered 2026-11-11: a GitHub release, `v0.1.0`, with the Windows, Linux, Android and web
   packages** (the site's question stays open).
   Nothing is pushed or published yet.

## Q-019 — multiplayer: the owner's decisions (2026-11-10, asked interactively) — ANSWERED

`plans/plan_gemultiplayer.md` §10 Q1-Q11:

1. **Scope (Q1):** approved, Speedy Blupi 1/2 (blupi_recon) as the source of truth although
   mobile-eggbert has no multiplayer.
2. **Rules (Q2):** SB2 / Speedy Eggbert 2 (the editor already follows Eggbert 2).
3. **Maps (Q3):** one multiplayer world first, then 3-5, their identity from SB2's world201-212.
4. **Platforms (Q4):** Linux and Android in v1 (Windows with the Windows build, Q-018); not the web.
5. **Internet (Q5):** LAN only in v1, with a ZeroTier-style VPN documented for play over the internet.
6. **Sign-in (Q6):** automatic, as the selected player A/B/C (no Guide screen).
7. **Host loss (Q7):** the session ends for everyone (as the original).
8. **Original defects (Q8):** not reproduced; each kept out is written into behavior_differences.md.
9. **Team colours (Q9):** derived by recolouring (yellow, orange, blue, green, as the editor's starts).
10. **Who and when (Q10):** an agent, before the AI policy (introduced after multiplayer is done), on a
    separate branch `multiplayer`, merged into `develop` only on the owner's acceptance.
11. **Speed and cheats (Q11):** as the original allowed them in a network game (to be found in
    blupi_recon).
12. **Chat:** in the first version already.

## Q-020 — multiplayer: acceptance of branch `multiplayer` (2026-11-10) — item 2 ANSWERED: merged into `develop` 2026-11-11

The network game is implemented on branch `multiplayer` (worktree `../galaxy-eggbert-multiplayer`;
`docs/handbook/09-multiplayer.md`, BD-0120, `plans/plan_gemultiplayer.md`'s status). Checked on one
machine with two instances (screens, chat, start, the rules, bombs, shots, vehicles, pause, leaving,
the win); single player unchanged (CTest, demo replays). For the owner:

1. **A game between real machines.** Two PCs on one network, or a PC and the phone (an arm64
   release APK of the branch is built: `android/app/build/outputs/apk/release/` in the worktree;
   it replaces the installed Galaxy Eggbert, same signature, data kept). Does it play as expected?
2. **Merge into `develop`?** On acceptance the branch is merged (Q-019 Q10); the AI policy follows.
   **Answered 2026-11-11: the owner had it merged into `develop`.**
3. **More worlds?** (Answered by Q-021: 206-212 built 2026-11-11.) 201-205 exist (machine arena, machine towers, toy box, metal works, stone
   courtyard: the five asked for). SB2 has 206-212 too (metal 206/210, stone 207/209, library 208,
   palace 211, clods 212): add more?
4. **Speed and cheats:** SB2 does not restrict them in a network game (each machine its own); the
   same here. Keep it so?

## Q-021 — multiplayer: the improvements after the first version (2026-11-11, asked interactively) — ANSWERED, implemented on branch `multiplayer` (BD-0120)

The owner, away from the computer (the real-machine test of Q-020 waits), asked what can still be
improved. Answers (all options taken):

1. **What SB2's network game has and the first version did not** — all three:
   - the other players' sounds (SB2 `PK_PLAYSOUND`/`PK_STOPSOUND`, decnet.cpp:172-193,
     decblock.cpp:2378, decmove.cpp:183-190);
   - the host's lift synchronisation (`PK_ASCENSEURSTART`/`END`, decmove.cpp:571-630, 283);
   - the others' power and cloud shown (`m_netBlupiSec`, decblock.cpp:1696-1715) and the cloud's
     touch (`NetPlayerCollide`, decblupi.cpp:3838-3848, `PK_STOPCLOUD`).
2. **Worlds:** all seven more, 206-212 (Q-020 point 3 answered).
3. **Galaxy additions (not in SB2, each a BD entry):** names above the players, a rematch with the
   same players (back to the lobby after a match), statistics at the end.
4. **Reliability:** a test on a bad network (simulated loss and delay), a player whose network
   drops seen as gone after a few seconds, joining by the host's IP address. CNA changes go to the
   `cna` Claude agent working on CNA (the owner: "na cna pracuje jiny claude agent cna. jemu posilej
   co potrebujes v cna upravit"); CNA stays read-only for this repository.

## Q-022 — multiplayer: the third round of improvements (2026-11-11, asked interactively) — ANSWERED, in progress on branch `multiplayer`

The owner, still away from the computer, asked again what can be improved and was asked. Answers
(all options taken; the branch waits for the owner's own test before any merge, Q-020):

1. **Tests and reliability:** a test with three and four players (only two had ever run), a pause
   test (autoplay to press Escape), robustness against broken packets (fuzzing the decoder), a long
   match with random play checking that the players' worlds do not drift apart.
2. **Galaxy additions (none in SB2, each a BD entry):** a spectator after being knocked out
   (follow the others until the match ends), messages during the match (who killed whom, who left,
   who lost the last life; chat during the match), match settings in the lobby (lives, a time
   limit), the editor's own worlds (the host sends the world to the others; SB2 had no map
   transfer).
3. **Content and platforms:** Speedy Blupi 1's network worlds 101-110 (built like 201-212 from the
   originals' identity), rejoining a running match after a drop, a Windows build with multiplayer,
   internet play without port forwarding (CNA PlayerMatch with the relay server).
4. **The branch:** wait for the owner's test; no merge into `develop`, no push.

**Q-022 point 3, Speedy Blupi 1's worlds 101-110: not built (2026-11-11), open for the owner.**
SB1's network worlds are SB2's 201-210. The comparison read Speedy Eggbert 1's
`DATA/WORLD101-110.BLP` against SB2's `WORLD201-210.BLP`, both from `../blupi_recon/build/freeapi-*`:

- **Identical in eight of ten:** the same decor cell for cell, the same objects at the same places,
  the same four starts, the same music and region.
- **103:** four cells differ.
- **108:** SB2 has 23 ammunition packs, 19 helicopters and one lift fewer.
- **210:** SB2 widened it (1154 cells) and added a shield and two bombs of each team.

Galaxy's 201-210 are built from that same identity, so a "101-110" would be the same worlds under
other numbers. Options:

- (a) Leave it: SB1's worlds are already in the game as 201-210.
- (b) More worlds of Galaxy's own design (none in SB1 or SB2), as many as wanted.
- (c) 101-110 as variants of 201-210 (another layout of the same theme), only to have twenty-two
  numbers.

The recommendation is (a), with (b) if more worlds are wanted.

**Q-022 point 3, internet play without port forwarding: what only the owner can decide (2026-11-11), OPEN.**
CNA's PlayerMatch sessions go through the relay of `cna-gamer-services-server` (libcna), so no
router needs forwarding. That needs:

1. **A server on the internet:** a machine with a public name, a TLS certificate (Let's Encrypt or
   similar), and the server (C++, Boost, OpenSSL, SQLite) running on one TCP port (47831 in its
   README).
2. **Accounts:** the server has no self-registration. The administrator creates every player's
   account (`cna-gamer-services-admin <db> user <name> <gamertag>`, password on stdin) and a
   title `galaxy-eggbert`.
3. **Galaxy's side**, which can be built and tried without the public server on this machine (the
   server on loopback, two instances):
   - a server address in the multiplayer list beside the host address;
   - sign-in with an account through CNA's Guide;
   - games found and joined over the internet as on the local network.

Questions:

- (a) Who runs the server, and where (a VPS, a home machine with one forwarded port)?
- (b) Its address, once there is one.
- (c) Who creates the accounts, for which players?

Until then the work goes on with the loopback server.

**Q-022's open points, ANSWERED (2026-11-11, asked interactively).**

- **SB1's worlds:** none built as such. Instead five more multiplayer worlds of Galaxy's own design,
  213-217 (101-110 stay free).
- **The internet server:** prepare its deployment, a guide and a script for a Linux server (TLS,
  systemd, the title, the accounts). The owner runs it on a machine of his; its address comes later.
- **Windows:** a zip to try on a real Windows machine.
- **The branch:** it still waits for the owner's own test (Q-020).

## Q-023 — the game in more languages: the minimal variant, English and French (2026-11-11, asked interactively) — ANSWERED, done on branch `localization`; open points ANSWERED 2026-11-12

The owner asked how much work more languages are and whether they cost performance (none
measurable: a text found by its key, a few instructions more per glyph for UTF-8), then had the
minimal variant made: English and French with text.png's existing glyphs. Answers (all the
recommended options):

1. **The original's French:** copy Mobile Eggbert's French strings (MyResource::InitializeFR: the
   buttons, the players' panel, the training hints with their accelerometer variants) into Galaxy,
   citing the source. Galaxy's own texts (help, editor, network game, the Setup's additions) are
   translated for Galaxy.
2. **Choosing the language:** the system's, as the original (MyResource reads it), and a Language
   item in the Setup (a Galaxy addition, BD-0126), saved.
3. **Which texts:** everything the player sees, the editor and the network game included; the cheat
   list, the debug panels and the logs stay English.
4. **Where:** branch `localization`, worktree `../galaxy-eggbert-localization` (its own
   `build-cna-opengl3`), merged into `develop` on the owner's acceptance; another session commits to
   `develop` in the shared worktree meanwhile.

**Open points for the owner (2026-11-11, written down, not asked):**

- **(a) Acceptance and merge:** the branch is ready for the owner's own look (Setup's top-right
  button switches English/Français). Merge into `develop` on acceptance, as Q-020.
- **(b) Two spelling mistakes of the original's French hints were corrected:** "Prend" → "Prends"
  (five hints), "décole" → "décolle" (one). The recommendation is to keep them corrected; say so if
  the original's spelling is wanted, word for word.
- **(c) A French speaker's reading:** the translations of Galaxy's own texts (about 300) were made by
  the agent; a native speaker's proofreading is recommended before a release.
- **(d) CNA's own notice stays English:** "Joueur A signed in" (the Guide's toast, top right of the
  multiplayer screen) is CNA's text; a French one would be a CNA change (the `cna` session).
- **(e) More languages next (German, Czech):** German needs its words only (ä ö ü are on the sheet;
  ß, Ä Ö Ü are not: written "ss" and without dots). Czech needs new glyphs (č ř š ž ě ů ...) drawn on
  text.png and its 2x sheet. Which, and when?

**The open points, ANSWERED (2026-11-12, asked interactively).**

- **(a)** Verify first (the web and Android builds, the lobby, the nickname panel and the Win/Lost
  screens in French), then merge into `develop`.
- **(b)** Keep the two corrected spellings.
- **(c)** No native reading before the release; the owner reads the Czech in the game afterwards.
- **(d)** Recorded for the `cna` session in `upstream_bugs.md`.
- **(e)** Czech and German next. text.png and its 2x sheet get the glyphs they lack, composed from
  the sheet's own strokes (the acute of é, the circumflex of ê turned over as a caron). A capital
  with a mark is drawn a few pixels shorter so that the mark fits the 32x32 cell, in every
  language (French's É À Ç included). Merged into `develop` once verified.
- **Then:** the rest of the reported lag (OR-0074: the terrain rebuilt on the main thread after
  dynamite), Czech and German, one release 0.2.0 with the four languages, published by the agent as
  0.1.x was (tag, GitHub release, the site), and the site's pages brought up to date (they still name
  1.0.0-rc1 in places).

**More languages and a picker of flags (2026-11-12, the owner).** Slovak, Spanish, Italian, Polish and
Portuguese; the menu's flag opens a menu of flags. Decided meanwhile, open to change:
- **Portuguese is Brazil's** (pt-BR, "você", Brazil's flag), as proposed: more players than Portugal's.
  A Portugal variant would be a second table of the same keys with its own flag.
- **Spain's flag without its arms** (the civil flag); Slovakia's arms simplified; Brazil's without its
  stars and motto (at 48 x 32 they are not legible).
- The French, German, Slovak, Spanish, Italian, Polish and Portuguese of Galaxy's own texts wait for a
  native speaker's reading; the owner reads the Czech (and can judge the Slovak).

**Six more and the release after them (2026-11-12, asked interactively).** The owner: release "after
the further languages"; Dutch, Hungarian, Romanian and Swedish; Russian and Ukrainian; besides the
languages a faster editor, the rest of the stutter and the languages on Android. Decided meanwhile,
open to change:
- **The Cyrillic letters are rendered from a font** (Comic Relief, SIL OFL 1.1, a hand like the sheet's),
  not composed: Cyrillic has no strokes on the sheet to compose from. The font stays outside the
  repository (`~/deps/fonts`); its pictures on text.png are not bound by the OFL. The letters that look
  like Latin ones (а е о р с у х і ї ...) are drawn by the Latin glyphs.
- **Romanian with the comma below** (ș ț), the correct letters, not the cedilla of old fonts.
- Their texts too wait for a native speaker's reading.
- **Release 0.3.0** once the six are verified: the option the owner chose read "first the languages
  chosen below, then one release 0.3.0; building and publishing it takes about an hour", so published
  by the agent as 0.2.0 was (tag, GitHub release, the site); Android versionCode 6.

