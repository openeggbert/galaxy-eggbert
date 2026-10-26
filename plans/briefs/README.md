# Authoring Galaxy Eggbert's campaign worlds

How the campaign worlds are made: one Python script per world in `tools/worlds/`, writing
`worlds3d/worldNNN.vwr`, with a brief here (`plans/briefs/worldNNN.md`). Worked examples: the main
hub `world001.py`, world 1's hub `world010.py` and missions `world011.py`–`world014.py`. Read
`plans/GALAXY_EGGBERT_COMPLETION.md` §1–2 and CLAUDE.md first; this file is the practical part.

## Direction (owner, non-negotiable)

- A genuine third-person 3D game: free movement in X/Z, real heights, levels designed for 3D space.
  **Never** convert, extrude or trace the original's 2D map. Its map is a design reference: which
  lessons, obstacles, objects, in what order, how hard.
- Every mechanic comes from Mobile Eggbert (`../mobile-eggbert`, read only). Invent nothing:
  only the game's existing tiles and objects, used as the original uses them.
- Each world is its own design — no cloned arenas, no randomised layouts, no shared template
  dressed with different tiles.

## Inputs for a world NNN

- The original's map and objects: `../mobile-eggbert/worlds/worldNNN.txt`. `tools/world_inventory.py`
  summarises every world (region, objects, signature tiles); `parse_reference()` there reads one.
  Print an ASCII map of it to see the structure; count its tiles to get the world's vocabulary
  (`collections.Counter(grid.values())`).
- The header's `region` is the world's background: pass it as `World(mission=NNN, sky=region)`.
- The video: `plans/evidence/video-timeline.csv` gives each mission's time span in
  `/rv/big/my-blupi-videos-on-youtube/Speedy_Blupi_for_Windows_Phone/20241215 Speedy_Blupi_for_Windows_Phone_gameplay_all_levels.mp4`.
  A contact sheet into `build-probe/video-evidence/wNNN/`:
  `ffmpeg -loglevel error -y -ss START -t LENGTH -i "$V" -vf "fps=1/3,scale=400:-1,tile=6x4" -frames:v 1 sheet.jpg`
- Training hints exist only for missions 11–14 (`src/GalaxyEggbert/Game/TrainingHints.cpp`); place
  them as hint zones (`World.hint_zone`).
- Tile classes: `tools/data/blocks.csv` (render mode, collision mask, hazard, sign, door).
  Tiles 15–31 and 49–60 have transparent parts: fine as props/plates, not as wall cubes. Since
  2026-10-26 a sprite tile (a quarter or more clear: doors, treasure doors, pillars, girders,
  bars, cobwebs) is drawn cut out, so it shows the scene behind it, not black; a tile with only
  small gaps (stone walls) stays opaque. A tile whose collision mask is empty (185, the
  backgrounds) is walk-through as in the original: no walls of it. Check a tile's look in `Content/icons/object-m.png`
  (icon i at x = 1 + (i % 20)·65, y = 1 + (i // 20)·65, 64 × 64).

## Tools

- `tools/worldkit.py`: `World` (set, fill, box, stairs, place(ObjectType…), bigdecor, hint_zone,
  rotate_plate, spawn, save), `ObjectType`, `letter_cells`.
- `tools/flightcheck.py`: levelcheck plus the helicopter; `--mine-margin 0` lets still mines be
  jumped as in the game.
- `tools/worlds/kit.py`: `slab`, `wall` (machine faces mixed in — pass `faces=`/`block=` for other
  themes), `rail`, `props`, `lift(start, end, ticks, stop)`, `mine`, `boundary`, `rect`, `line`,
  `sign_niche` (hub signs; gated ones two deep with the bar in the doorway). Add helpers there
  when a world needs them; keep them generic.
- Coordinates: raw grid 0–99, Y up. A block at y makes y + 1 the walking level. Objects sit at
  the cell they occupy (a treasure on a floor at y 0 is placed at y 1). Spawn facing −Z.

## What Blupi can do (design to it)

- Jump: 2 blocks up, or across a 1-cell gap (2 is marginal). No step-up: every 1-block step is a
  jump. A full jump needs two free cells above him (else a weak one). No fall damage; falling off
  the world kills slowly (below y −27).
- Crates (ObjectType12): solid, he stands on them; pushed a cell along X or Z by walking into
  them, only onto support (a block or a crate) — they never fall; pulled by crouching (LeftShift)
  facing one and pressing Down. Stacks are built by pushing a crate off a ledge onto a crate.
- Dynamite (55): one stick at a time, put down at his feet, blast after 2.5 s reaching ±1.5 cells
  in Y/Z and wider in X; it destroys crates (with their linked stacks), saws, small enemies — and
  Blupi if he stays within it.
- Lifts (ObjectType1): `kit.lift` with the original's tick timings, kept exactly since D43 (dwell
  `stop`, travel `ticks`, dwell, travel back); lifts with the same timing move in step (put several
  side by side for a wide platform). A lift does not block Blupi sideways and passes through him
  coming down; the original's 3- and 20-tick dwells are often too short to step across a cell in
  3D: give a lift-to-lift step a rest column, or 40-tick dwells.
- Vehicles: helicopter (13, RightShift up / LeftShift down), jeep, tank, skateboard (24),
  overcraft (46: `ObjectType.OVERCRAFT`, not a balloon; since D46 it hovers, climbing only within
  1.25 cells of the ground, so it crosses a lava floor but cannot fly up); taken and left with
  Action (Space). Jeep, tank and skateboard run over mines. Any
  vehicle that meets water is wrecked and Blupi drops in (013's "it dislikes water!").
- The flight barrier (214): a wall that stops Blupi on foot and in the jeep, tank and skateboard,
  and lets the helicopter and the overcraft through (D49, `Decor::DecorDetect`); a route only a
  flying vehicle may take. `flightcheck.py` flies through it.
- Signs and flat plates read upright and unmirrored from both sides since D48: a sign needs no
  particular facing.
- Water: Blupi floats at the surface (feet ≈ 0.28 into the top water cell), dives with LeftShift,
  swims up with Ctrl/RightShift and drifts back up when idle (air gauge ≈ 25 s while under). He
  gets out by jumping at the surface onto a bank whose top block is level with the water's top
  cell (open air above him; the original's weak jump, -12 px/tick, clears no more): build every
  pool's water up to its banks' tops. A bank one block higher traps him (measured by world 7's
  agent; `build-probe/watertrap.py` finds water without an exit).
- Saws (378) and switches (384) lie in the floor; a switch stops the saws in its own row (same Z
  and Y, ±20 along X); a stopped saw (379) is a still, harmless floor (D42).
- Bars and rods (138, 202): walking onto one over a drop, or a jump from under one, catches it
  (D45); he hangs in the bar's cell, out of reach of enemies a cell below.
- Hanging mines (3) kill within 0.7 of his feet: one a cell above his walking cell is harmless to a
  walker and deadly to a jump; a low mine must hang in his walking cell.
- The drink's Hide hides him from the follow stars too (D44).
- Teleporters (330-333): standing under one, after 6.4 s he arrives one cell +Z of the cell under
  its partner, facing on (+Z): leave that cell and the one beyond it open.
- Hubs: world signs and mission signs in niches (`sign_niche`); mission N > 1 behind a blue bar
  (182) one or two cells from its sign — it opens when mission N − 1 is won, and Blupi then
  arrives just outside it. Each world's last mission ends with the golden key (ObjectType21).

## Make it fair in 3D

- Corridors three wide; doorways may be one wide but keep the camera's room (no long one-wide,
  one-high tunnels; dive under one-block curtain walls).
- Check for shortcuts: a crate next to a ledge is a stair; a wall top next to a goal is a path.
  Use `levelcheck.py` — and for crate/dynamite gates a `--solved` variant of the script (see
  `world012.py`, `world014.py`) plus a staged check (lessons 1..k solved → how high he gets).
- Enclose what a helicopter must not skip (walls and roofs).

## Checks before committing a world

1. `python3 tools/worlds/worldNNN.py` and `python3 tools/levelcheck.py worlds3d/worldNNN.vwr`
   (treasures, keys and exit reachable; or unreachable unsolved / reachable solved).
2. `build-cna-opengl3/VerifyShippedWorlds worlds3d` — every world loads and round-trips.
3. Live: `QA_LIVE_RUN=$PWD/build-probe/qa-run-<you> tools/qa_live.sh build-cna-opengl3 <out-dir> <steps> --mission NNN [--spawn x,y,z,yawDegrees]`
   (steps: `sleep`, `shot`, `key`, `keydown`, `keyup`; see the script's header). The idle camera
   swings to Blupi's side by design — take shots while he walks to see over his shoulder. Look
   at the key moments of each lesson.
4. Brief `plans/briefs/worldNNN.md`: Reference (what the original teaches, objects, video time),
   3D design (where everything is and why), Checks (what was verified, how).
5. Commit the script, the `.vwr` and the brief together, one world per commit, a message saying
   what the original does and how the 3D version does it. Never add AI attribution. Never push.

## Boundaries

- Do not change engine or game code (`src/`, `include/`); if a mechanic misbehaves in 3D (a
  2D-era rule, a crash), stop using it, write down exactly what happens, and report it.
- Do not modify `../mobile-eggbert` or any sibling repository. Do not build C++; the game binary in
  `build-cna-opengl3/` is current. Scratch files go in `build-probe/`, never `/tmp`.
