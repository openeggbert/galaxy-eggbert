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

## Answered

### Q-000 — Wind and fans along Z (2026-11-02)

Answer: yes, add turning fans and wind 90 degrees (pushing along Z) as a 3D adaptation (M3D-0005).
