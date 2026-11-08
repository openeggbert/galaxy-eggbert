# 5. Worlds

## The `.vwr` format in brief

Authoritative text: `World Format.md` (its "Recommended Future World Header" section is a
hypothetical, not shipped). Reader and writer: `src/GalaxyEggbert/Worlds/World.cpp`, `Chunk.cpp`.
Independent re-implementations that must be kept in step with them: `tools/worldkit.py` (writer) and
`tools/vwr_inspect.py` (reader). All numbers are little-endian.

- A world is a fixed 100 x 100 x 100 grid of blocks in 10 x 10 x 10 chunks (1000 chunks), raw
  coordinates 0-99 with Y up. A block is a `uint16`: type in bits 15-4 (the `object-m.png` icon
  index), metadata in bits 3-0. Type 0 is air.
- **File**: `VWR1`, a 40-byte header (version byte `2`, chunk size 10, chunks per axis 10, chunk
  count, chunk table offset, chunk data offset, then the sky/music word, the mission number and the
  spawn cell +1 on each axis; all-zero spawn means unset), a chunk table of 20-byte entries (offset,
  size, chunk x/y/z, flags, `0x0001` = empty), then chunk payloads. Only version 2 loads; older files
  are rejected.
- **Sky word**: low byte = background region 0-31, which selects
  `Content/backgrounds/decorNNN.png` (28 of the 32 ids have a file; a missing one falls back to a flat
  colour); second byte = music track (0 none, 1-10 the original's tracks).
- **Chunk payload** `VCH1` (version 2): a palette of raw block values (at most 256 per chunk),
  indices bit-packed into `uint64` words, and an optional sparse metadata section `BMD1`. A chunk
  that is all air *and* has no metadata is skipped.
- **Sparse metadata** (records anchored to a block position, payload by type): 1 MoveObject
  (`ObjectType` byte, start and end position, speed, four patrol-timing floats, optional visual
  icon), 2 plate turn, 3 BigDecor icon, 4 hint zone (record index and box size), 5 object turn.
  Readers that do not know a type keep the record, so additions are compatible without a version
  bump.
- A chunk with more than 256 distinct block values cannot be written; `worldkit.py` raises with the
  chunk's coordinates.

To look inside a world: `python3 tools/vwr_inspect.py worlds3d/world012.vwr` (header, counts per
block type and height, every object and BigDecor), or `--summary` for one line per world.

## What a campaign world is made of

- **Mission numbers**: 1 main hub; N0 = theme hub N (10-120); N1.. its missions; 199 the bonus
  world. The counts per theme (4, 5, 4, 6, 8, 6, 5, 4, 5, 7, 5, 5 missions) are the original's and are
  checked by `tools/campaigncheck.py`.
- **Themes** (the original's): 1 blue machines, 2 white palace, 3 village, 4 cheese, 5 space station,
  6 slime, 7 toys, 8 machine works in the sky, 9 clay clods, 10 grey cobbles. Each has its tiles'
  roles and building pieces in a kit module (`tools/worlds/kit.py` for theme 1, `palace.py`,
  `village.py`, `cheese.py`, `station.py`, `slime.py`, `toys.py`, `skyworks.py`, `clods.py`,
  `cobbles.py`). Themes 11 (hangar, blue machine panels) and 12 (brick house) reuse the kits of
  other themes (`world111.py`, `world121.py` import `kit`, `station`, `village`).
- **Hubs** use `kit.sign_niche` for world and mission signs. A mission N > 1 sits behind a blue bar
  (tile 182) a cell or two from its sign; it opens when mission N - 1 is won, and Blupi arrives just
  outside it. Gated signs are two deep with the bar in the doorway. The main hub's gold bars open as
  worlds are won.
- **Briefs**: `plans/briefs/worldNNN.md` has, per world, what the original teaches (with the video
  time span), the 3D design, and the checks that were run. `plans/briefs/README.md` is the authoring
  guide this chapter condenses. `plans/audit/world_lessons.csv` has one row per world (`LV-NNNN`)
  with the lesson, the 3D reading and the status.

## How worlds are generated

Every campaign world is the output of a Python script; the `.vwr` is a build product that is
nevertheless committed.

```bash
python3 tools/worlds/world012.py            # writes worlds3d/world012.vwr (deterministic)
python3 tools/worlds/gallery.py             # worlds3d/world998.vwr and plans/audit/gallery_map.md
python3 tools/worlds/title.py               # worlds3d/title.vwr
```

Re-running a generator must reproduce the committed file byte for byte (checked on 2026-11-07 for
world 012: same SHA-1). A diff in a `.vwr` after you changed nothing is a bug in the script.
Never hand-edit a `.vwr` that has a generator; change the script. The exception is a world authored
in the in-game editor, which has no script.

World 999 (the mechanics showcase used by several tests and the golden files) has no Python
generator: `tools/GenerateSampleWorld3D.cpp`, built as the `GenerateSampleWorld3D` target, writes
`worlds3d/world999.vwr` relative to the working directory, so run it from the repo root. **`GenerateSampleWorld3D --placeholders`
overwrites the hub and mission worlds with old placeholders**; do not use it.

### The authoring API

`tools/worldkit.py`:

```python
from worldkit import World, ObjectType as O
w = World(mission=11, sky=0)                 # sky = the original's region; music defaults to the original's
w.fill(40, 0, 40, 60, 0, 60, 10)             # a floor of block 10
w.box(48, 1, 48, 52, 3, 52, 261, hollow=True)
w.place(O.TREASURE, 50, 1, 55)               # objects sit at the cell they occupy
w.place(O.LIFT, 50, 1, 60, end=(50, 5, 60), advance=30, recede=30, stop_start=10, stop_end=10)
w.bigdecor(26, 55, 1, 55)
w.hint_zone(0, 40, 1, 40, 60, 3, 60)         # training hint 0 of the mission's table
w.spawn = (45, 1, 45)
w.save("worlds3d/world011.vwr")
```

Also `set`, `get`, `clear`, `stairs`, `rotate_plate`, `letter_cells`. `ObjectType` lists the ids used
in authoring (`include/GalaxyEggbert/Def/ObjectType.hpp` is the full set). `tools/worlds/kit.py` adds
`slab`, `wall`, `rail`, `props`, `lift(w, start, end, ticks, stop)`, `mine`, `boundary`, `rect`,
`line` and `sign_niche`. Put generic helpers in a kit module, theme-specific ones in the theme's.

A generator is a script with a docstring that names its brief, a `main()` that builds a `World` and
saves it, and, where a gate cannot be modelled by the checker, a variant that writes the "solved"
state (see `world012.py --solved`, `world055.py --stage K out.vwr`).

## Adding or changing a world

1. Read the world's brief and, for a new design, the reference: `../mobile-eggbert/worlds/worldNNN.txt`
   (the original's map; `tools/world_inventory.py` summarises every world and has `parse_reference()`),
   and the gameplay video span in `plans/evidence/video-timeline.csv` when you need to see it. The
   map tells you *what the world teaches*; the 3D layout is your own design.
2. Write or edit `tools/worlds/worldNNN.py`; run it; look at it in the game
   (`--mission NNN [--spawn x,y,z,yaw]`, or `tools/qa_live.sh` with a steps file for screenshots).
3. Run the checks of [04-testing.md](04-testing.md) ("a world generator").
4. Write or replay the playthrough `tools/autoplay/worldNNN.txt`; a changed route needs the waits
   retuned (mine, lift and cycle phases are tick-bound).
5. Update the brief (reference, 3D design, checks), and the audit rows the change affects
   (`python3 tools/audit/world_lessons.py --check`, `python3 tools/audit/lesson_inventory.py`).
6. Commit the script, the `.vwr`, the playthrough and the brief together, one world per commit,
   with a message saying what the original does and how the 3D version does it.

A *new* numbered world beyond the campaign's 78 is not a routine task: hubs, `HubDoors`,
`campaigncheck.py` and the autoplay chains encode the original's structure.

## Rules for worlds

**Direction.** Hand-authored true 3D only (see [01-overview.md](01-overview.md)). Each world is its
own design: no cloned arenas, no randomised layouts, no shared template dressed in different tiles.
Use only the game's existing tiles and objects, as the original uses them.

**Make it fair in 3D** (from the briefs; guidelines, not enforced):

- Corridors three cells wide; a doorway may be one wide but give the camera room: no long one-wide,
  one-high tunnels.
- Blupi jumps up to two blocks and across a gap of up to two cells (a one-cell gap is comfortable;
  `levelcheck.py` models two). He does not step up: every one-block step is a jump, needing two free
  cells above for a full jump. Falling does no damage; leaving the world's bottom kills slowly.
- Look for shortcuts: a crate beside a ledge is a stair; a wall top next to a goal is a path.
  Enclose what a helicopter must not skip.
- Keep the camera in mind: the chase camera swings behind Blupi and rises over obstacles, but
  tight rooms still pinch it.

**Tiles.** Look up a tile in `Content/icons/object-m.png` (icon i at x = 1 + (i % 20) * 65,
y = 1 + (i // 20) * 65, 64 x 64) and its class in `tools/data/blocks.csv` (render mode, collision
mask, hazard, role). Tiles 15-31 and 49-60 have transparent parts: fine as props or plates, wrong as
wall cubes. A sprite tile with a quarter or more clear (doors, pillars, girders, bars, cobwebs) is
drawn cut out; stone walls with small gaps stay opaque. Tiles with an empty collision mask and no
role (`BlockDefinition::decor`: backgrounds, pillars, girders, poles, props, signs) are passable in
every direction, so never use them as walls, floors or ceilings. Lava, saws, switches, bridges,
springs and the vanishing saucers (Temp) are floors. Block 214 is an invisible barrier that stops
Blupi on foot and in the jeep, tank and skateboard but lets the helicopter and the overcraft pass.

**Solid BigDecor.** The BigDecor in `kSolidBigDecorIcons` (palms, trees, plants, houses, machines,
toys, palace pieces, the Blupi statue) stand in Blupi's way as large as their models. Do not put one
in a route the player has to take: leave a lane a human can walk (the audit scripted every BigDecor
gap of the campaign; lanes under about 0.9 cells were treated as dressing, not routes, BD-0083).
`levelcheck.py` models these shapes.

**Creature timing.** Every moving creature of a type has one speed and one rest, the original's
(`CREATURE_TIMING` in `worldkit.py`, applied automatically by `World.place` whenever a creature has
an `end`; for example a moving mine takes 10 ticks a cell, a fish 15 and rests 47). Whatever you pass
for the timing is overridden. `python3 tools/audit/patrol_envelope.py` must report 0 creatures off
their timing. Type 54's rest in the file (200) is overridden to 152 at load.

**Lifts** (`ObjectType` 1, 47, 48): use `kit.lift` with the original's tick timings; lifts with the
same timing move in step, so put several side by side for a wide platform. A lift does not block
Blupi sideways and passes through him coming down. The original's short dwells (3 to 20 ticks) are
often too short to step across a cell in 3D: give a lift-to-lift step a rest column or longer
dwells. No lift may be faster than the original's fastest (`tools/audit/lift_envelope.py` reports
0), and a lift's end must not be inside a block.

**Switch rows.** A switch (tiles 384/385) turns the saws (378, stopped 379) of its own row: the same Y
and Z, within 20 cells along X (`WorldRuntime::TryActivateSwitch`). A stopped saw is a harmless
floor. Standing in the switch's own cell works since OR-0078.

**Crates, dynamite, vehicles.** Crates (type 12) are solid boxes at any X and Z, pushed by walking
into them and pulled by crouching and backing away, only onto support; they never fall. One stick of
dynamite at a time, blast after about 2.5 s, destroys crates, saws and small enemies (and Blupi). The
helicopter (13) cannot enter water and is lost there; the jeep, tank and skateboard run over mines;
any vehicle meeting water is wrecked.

**Water.** Blupi floats at the surface, dives with Left Shift and gets out by jumping onto a bank
whose top block is level with the water's top cell; a higher bank traps him. Build every pool's water
up to its banks' tops; `tools/watercheck.py` finds water without a way out.

**Teleporters** (330-333): standing under one, after about 6.4 s he arrives in the cell under the
partner pillar of the same icon; the arrival starts a short contact grace, so he does not go straight
back (BD-0017). (`plans/briefs/README.md` still says "one cell +Z of the cell under its partner":
that is the pre-fix behaviour.)

**Ends.** Each theme's last mission ends with the golden key (type 21); other missions end at the red
arrow (type 7). `campaigncheck.py` checks it.

**Hints.** Training hints exist only for missions 11-14 (`src/GalaxyEggbert/Game/TrainingHints.cpp`);
place them as hint zones.

**Music and sky.** `World(mission=N, sky=region)`: the region is the original's `region=` header
value; the music defaults to the original's for that mission (`worldkit.ORIGINAL_MUSIC`).

## The in-game editor and custom worlds

From the main menu the "3D World Editor" button opens the slot browser: ten slots for each of three
gamers. A slot is a normal `.vwr` file, `customworlds/gamerG/custom_00N.vwr`, written relative to the
game's working directory (in the build directory when you play from there, `/save/customworlds` on
the web, the app's internal storage on Android). Slot 1 is seeded once from the showcase world, and the
gallery gets its own slot. The browser also offers backups (three generations), export to and import
from `customworlds/shared/` (a download and a file picker on the web), delete with confirmation, and
a thumbnail per slot.

Inside a world: W/A/S/D fly, Ctrl rises, Shift/Z descends, Space places; the mouse aims; the icon palette
(ten groups in Eggbert 2's order) selects blocks, objects, BigDecor, start, background and music;
clicking again on the cell just placed cycles its variants (Ctrl places without cycling); rocks are
re-tiled to what borders them; undo and redo cover every edit; a box can be filled, copied and
stamped turned or mirrored; the "dice" saves and starts a play-test, and the pause menu's Back returns
to the editor. The editor places the game's existing blocks and objects, applies the creature timing
table above, and adds no gameplay. Key assignments are in `WorldEditorUpdate.cpp`; the parity
evidence against the original's editors is `plans/audit/editor_parity.md`.

Saws occupy their own cells, create no supporting block and keep the original empty collision
mask. Their actual disk contact kills; a stopped blade is harmless. Any block or air below is
independent. Old implicit-floor v2 files are upgraded in memory before editing/play, keeping
explicit floor blocks and switch links. New saves set the independent-cell flag; loading never
rewrites a player file. See `World Format.md` for the compatibility records.

An editor world has no generator. To ship one as a campaign-style world it would have to be named
`worldNNN.vwr` with a matching mission number (`VerifyShippedWorlds` enforces it); how the editor
assigns a mission number is not verified.
