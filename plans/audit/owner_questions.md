# Open questions for the owner

The owner (2026-11-02): do not wait at the PC for answers; write the open points here and ask
interactively when the owner asks for it. Each entry: the question, the options, the
recommendation, and what was done meanwhile. Answered entries move to the end with the answer.

## Open

### Q-001 — Grass (107-109, 157) in 3D: how?

`Decor/herbe1.rd4` matches the grass tiles (IoU 0.91): a lumpy block of soil with leaves on top.
As a terrain tile it would not join its neighbours (the lumps leave gaps), so the options are:
(a) keep the textured cube and put only the scene's leaves on its top as 3D geometry
(recommended), (b) the whole lumpy model per cell, (c) leave the grass as it is. Meanwhile: as it
is (a cube with the grass texture and its top overlay).

### Q-002 — The lightning bolt (305-307): it has no Ray Dream scene

The emitter is now `Decor/eclair.rd4`. The bolt between two emitters was painted, not modelled.
Options: (a) a generated thin glowing 3D bolt following each sprite's path (three shapes taking
turns as the original's frames; recommended), (b) keep the sprite plates. Meanwhile: the sprite
plates.

### Q-003 — The clay bridge's surface

The bridge is now its scene's model; its surface is the scene's smooth clay brown. The owner
asked for a clay texture on top. `Decor/terre.tif` (a clay texture) exists but no scene uses it.
Options: (a) map `terre.tif` onto the bridge model's top (recommended), (b) leave the scene's
colour. Meanwhile: the scene's colour.

### Q-004 — The terrain tiles as their Ray Dream models?

`plans/audit/model_sources.md`: 129 scenes are the terrain tiles (rock, cave, wood, techno,
slime, cheese, toys, houses, pipes, grass). Galaxy draws them as cubes textured with their sprite,
so that neighbours join into floors and walls; their scenes are lumpy blocks that would leave gaps
and seams between cells and cost far more to draw. Options: (a) keep the textured cubes (the
recommendation; the sprites are those renders), (b) models for the free-standing pieces only
(pillars, pipes' ends), (c) every tile as its model. Meanwhile: (a).

### Q-005 — The electrode (238-242) without its sparks?

`Decor/electro.rd4` gives the ball on its spring (IoU 0.79-0.81), but the sparks round the ball
were painted on the renders: the model alone looks dead. Options: (a) the model with the sprite's
sparks as a camera-facing decal (recommended), (b) the model alone, (c) keep the sprite.
Meanwhile: the sprite.

### Q-006 — The glue death (element 168-171) from `Blupi/glu.rd4`

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

## Answered

### Q-000 — Wind and fans along Z (2026-11-02)

Answer: yes, add turning fans and wind 90 degrees (pushing along Z) as a 3D adaptation (M3D-0005).

## Q-007 — the follower star's wake: the original's box or Galaxy's sphere? (2026-11-03)

- The original wakes a sleeping star when Blupi is within 144 px across and from 149 px over
  him to 158 under (a box, `MoveObjectFollow`); Galaxy uses a sphere of 132 px (2.06 cells).
  The box also wakes stars two cells off on a diagonal and two cells over him.
- With the box, 052 and 053 (101 stars, laid out with the sphere) fail their playthroughs: the
  stars round the treasures wake as he lands or passes. 053 can be adapted (done once: a column
  a cell higher, five route changes); 052 needs its helicopter section's stars re-laid.
- Options: (a) the box, with 052/053 re-laid (faithful; a day's level work); (b) keep the sphere
  as the 3D adaptation (as now). Recommendation: (a), when the levels are revisited.
- Meanwhile: the sphere stays (BD-0053 open).


## Q-008 — world 105's lava floor: the void, or a cheaper lava to draw it? (2026-11-03)

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
