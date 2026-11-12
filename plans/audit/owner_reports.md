# Owner reports (OR-NNNN)

The owner's own findings, recorded as given (translated from Czech), each to be reproduced against
the reference and fixed or answered.

Renumbered 2026-11-04: the reports of 2026-11-04 were first filed as OR-0032..0045, numbers the
table of 2026-11-02 below already used (it runs to OR-0059); they are OR-0060..0072 now. In commit
messages of that day the old numbers stand: OR-0032 -> 0060 (teleporter booth), 0033 -> 0061 (3D
terrain setting), 0034 -> 0062 (hub doors), 0035 -> 0063 (enemy Blupis), 0036 -> 0064 (steering at
F6-F8), 0038 -> 0065 (wind), 0039 -> 0066 (fan beside), 0040 -> 0067 (switch), 0041 -> 0068 (side
fan's top), 0042 -> 0069 (solid decor), 0043/0044/0045 -> 0070/0071/0072 (editor). The owner stresses these come from a spot check only: the audit
must be deep and complete, not limited to this list. Status values as in
`../GALAXY_EGGBERT_FIDELITY_AUDIT.md` §2. Newest batch first.


## 2026-11-04 (OR-0060..0072, one section each)

## OR-0060 — the teleporter's booth half hidden by the rocks beside its niche (owner 2026-11-04) — FIXED

- Report: "the teleporters can be entered, but why is a piece of them covered?"
- Cause: the terrain reliefs overhang half a block into a solid neighbour (Q-012 B); the booth's cell
  (330-333, drawn as its model) counted as solid, so the reliefs of the rocks on both sides of a
  teleporter's niche stood in front of the booth's ends (102's (39, 7, 74): the lights hidden).
- Fix: `TerrainRenderer`'s relief neighbours (`reliefSolid`): a cell drawn as its model is not solid
  for them.
- Validation: live, 102's teleporter from its niche (the booth whole, Blupi teleporting); ctest; golden
  frames unchanged.

## OR-0061 — what the 3D terrain reliefs cost; a setting for textures or 3D (owner 2026-11-04) — DONE

- Question: what do the generated stones (the reliefs) cost in place of the textures; phones will not
  manage; a setting for textures or 3D.
- Measured (`/usr/bin/time -v`, llvmpipe under Xvfb, 25 s into the world): peak RAM with the reliefs
  1.65 GB (030) and 1.29 GB (021), without them 0.92 GB and 0.94 GB -- the reliefs take 0.35-0.73 GB.
  GPU time on the desktop's radeonsi: 0.4-1.4 ms a frame (Q-012's table); the vertex count doubles
  (the hub at the title: 67 172 against 33 288).
- Done: the Setup's "3D terrain" toggle (under Erase progress; `SaveData::GetTerrainRelief`, saved as
  `terrainRelief=`): at first on by default on the desktop, off on Android and the web build. Switching it
  builds the terrain again with or without the reliefs and frees their table when off.
  `GALAXY_TERRAIN_RELIEF=0/1` still overrides it (tests, measurements).
- Owner, the same evening: off by default everywhere (it had been on on the desktop); the toggle
  turns it on.
- Validation: live, Setup shows the toggle; off, the hub plays with the flat textures and the save has
  `terrainRelief=0`; ctest.
- Follow-up 2026-11-04 (performance): the object and tile models were drawn every frame whether in
  view or not, and some are heavy (the treasure chest 68 068 triangles, a plant 182 496, the
  lightning emitter 20 600, the girder 19 348). The model pass now skips a model whose bounding
  sphere (its vertices' farthest reach, `objectModelRadii_`, plus a block) is outside the camera's
  frustum. Measured with `GALAXY_PERF=1` (llvmpipe under Xvfb, during a play pass): the main hub
  25 models / 84 k triangles / 17 ms a frame -> 2 / 3 k / 8 ms; world 115's start 68 / 1.15 M /
  171 ms -> 15-20 / 0.26-0.38 M / 88-106 ms; world 063 under 3 frames a second -> 16 ms. Golden
  frames and trace unchanged; the hub and 115 shot with and without culling show the same models.
  `GALAXY_MODEL_CULL=0` draws them all.
- And the spheres: the converter made every Ray Dream sphere 960 triangles whatever its size (the
  chest's 59 studs of 2.5 sprite pixels were 56 640 of its 68 068). `tools/rd4_to_gltf/lower_spheres.py`
  (run by `export_objects.py`, and once on the shipped objects and vehicles) tessellates each to its
  size: full detail from a quarter block, 8 x 4 for a stud. 41 parts, 245 988 -> 107 268 triangles;
  the chest 68 068 -> ~8 200, the girder 19 348 -> ~1 100, rendered side by side alike. Golden frames
  re-baselined: 39 / 43 / 252 pixels of the small models' outlines.

## OR-0062 — the hub's world doors: a piece of the blue wall over their signs (owner 2026-11-04) — FIXED

- Report (screenshot of the main hub, 17:12): a piece of the blue part covers the door's sign
  ("teleport" in the owner's words: the doors one walks into to enter a world).
- Cause: a relief's lumps and backing hung into the corner square of its face whenever the two cells
  beside that corner were solid, whatever the corner cell itself was. Over a door in a wall (158 at
  (55, 1, 73), 160 at (55, 3, 70)) the panels diagonally above it hung into the front of the door's
  cell: the lower sign showed a monitor, the upper one lost its board. (OR-0060 had only covered the
  edges.)
- Fix: `ReliefAirSides` returns the four corner cells too (bits 16-128); `AppendReliefFace` leaves out
  the triangles and the backing square hanging into a corner whose cell is not solid (a cell drawn as
  its model counts as not solid).
- Validation: live, the hub's two doors at (55, 1-3, 70-73), their signs whole with the reliefs on (as
  with them off); `VerifyTileFaces` (corner bits); ctest; golden frames unchanged.

## OR-0063 — the enemy Blupis drawn as the bulldozer, mines, shield, angel and bomb (owner 2026-11-04) — FIXED

- Reports: "the blue Blupi that should throw green bombs ... turns into a bulldozer, a mine and a
  shield instead of the blue Blupi"; "the green slime that moves shows a white angel and a bomb --
  that is not ok".
- Cause: the enemy Blupis (32 blupih on his helicopter, 33 blupit in his tank) were looked up on
  element.png, where their icons (61-75, 271-275; 237-252) are the bulldozer, the mines and the
  shield's flask, the angel, the slime and the dynamite, and the objects with a 3D model for those
  icons drew that model. The world files give them channels 11-13, all blupi1.png
  (Pixmap.cpp:717-719): his own frames (blupi.png) with every hue turned 25 degrees, the body green,
  the vehicles blue (measured over the whole sheet, 99.8 % of its pixels within 2 levels).
- Fix: both read blupi1.png (`ObjectDefinitionRegistry`), with their patrol tables on any sheet
  (`ObjectSprite`); with the real model they are drawn as it (`EnemyBlupi.hpp`,
  `DrawEnemyBlupis`): his model with the helicopter or the tank in the clips the original's icons
  show (MarchHelico, its turn and the glue drop HelicoGlu with its bomb at tick 21; MarchTank, the
  shot FireTank at 3 and 21 and the turn between), every texture recoloured as blupi1.png's
  (`BuildEnemyBlupiTextures`), facing along the patrol and turning through the camera's side, as
  the sprites turn through their viewer. Without the real model they are blupi1.png billboards.
- Validation: live, world 056's slime cave: green Blupis in blue tanks on their lines, firing, one
  in mid-turn facing the camera; the helicopter one with the blue rotor pack dropping its green
  bomb; `VerifyDefinitionRegistries` (sheet, recolour, poses); ctest 112/112; golden frames
  unchanged.
- The same class checked for every type (2026-11-04): the channels the original's world files give
  (types 1, 12, 47 Object; 32 / 33 Blupi1; the rest Element) and those Decor.cpp sets each tick
  (Explosion 8-11, 53, 90-93, 98-100; Object 14, 15, 31, 35, 52; Blupi 200; Blupi1 201-203; 38 by
  phase; the rest Element) all match the registry's `textureSource` now. 32 / 33 were the only ones
  off.

## OR-0064 — at the higher game speeds (F6-F8) he turns terribly fast (owner 2026-11-04) — FIXED

- Report: raising the speed should not change how fast he turns; at F6/F7/F8 he turns terribly fast.
- Cause: the game speed scales the simulation's dt (`GameSpeedFactor`: 2, 3, 4), and the steering
  (Left/Right, `m_yaw += turnInput * kTurnSpeed * dt` in `BlupiController::Step`) turned with it.
- Fix: the steering is a Galaxy control (the original's Blupi only faces left or right), so it turns at
  the player's pace: the game passes `turnInput / GameSpeedFactor` to Step. Down's turn-around stays
  the original's Turn action, its ticks played at the game's speed as the original plays them.
- Validation: live, Left held 0.6 s in world 001 at normal speed and at F6 (2x): the same turn; ctest.

## OR-0065 — the wind from a fan is barely visible and does not seem to move (owner 2026-11-04) — FIXED

- Report: the wind object (from the fan) is hardly visible at all and seems not to move.
- Cause: the wind tiles (110-125) were still plates: one 0.75 x 0.9 plate in the middle of each
  cell, seen edge-on along the very corridor it blows through, and never animated (no animation
  entry: always the first frame). Their sprites are thin streaks, every texel at alpha 127.
- Fix: the original moves the streaks a frame every tick, every cell on the same frame
  (Decor.cpp:1085-1102, `table_decor_ventg/d/h/b[m_time % 4]`): `TerrainRenderer` keeps the wind
  cells apart (`m_windBlocks`) and builds each of the four frames' meshes once (`BuildWindFrame`),
  drawn in the semi-transparent pass (an alpha-tested pass would drop texels at 127). Each cell is
  two crossed plates a cell across, both holding the way it blows (its streaks along it): a side
  wind upright and level (turned, M3D-0005, along Z), an updraught or downdraught on two upright
  plates. So the streams join from cell to cell and show from every side.
- Validation: live, world 115's draught tunnel from its mouth: before, one faint line at its first
  curtain; after, the four curtains of streaks, moving between two shots 0.1 s apart. Golden
  frames re-baselined (the demo world's wind tiles: 87-90 pixels, at most 12 levels); ctest.

## OR-0066 — a fan explodes when Blupi is beside it, not only when he jumps onto it (owner 2026-11-04) — FIXED

- Report: the fan should explode only when Blupi jumps directly onto it, not when he is next to it.
- Cause: `WorldRuntime::TryConsumeFan` went off with Blupi anywhere in the cell in front of a fan's
  mouth. The original (`Decor::IsVentillo`, Decor.cpp:7667-7735) needs his middle inside the fan's
  own cell, within 16 px of the mouth: its fans are open but for a 16 px housing at the back
  (`table_decor_quart`), so he sinks into a floor fan or pushes into a side fan's mouth.
- Fix: a Galaxy fan is a whole block (M3D-0004), so the rule is contact with the mouth's face: his
  middle within the fan's cross-section and his body against the face (18 px + 2), his feet on a
  floor fan's top, his head against a ceiling fan's bottom (4 px). Beside a fan he is safe.
- Validation: live in world 102: standing in the cell before its side fan (89, 2, 12) the old
  binary killed him at once, now the wind blows him west; jumping onto its floor fan (86, 1, 17)
  still explodes it. `VerifyInteractionSystem` (beside / against / on the fan); world 115's gap
  81 now blows him onto the east mine (the brief and LV-0071 updated).

## OR-0067 — at the saw's switch machine Space does nothing; it works only from above (owner 2026-11-04) — FIXED (reopened by OR-0078: standing in the machine did nothing)

- Report: when Blupi comes to the machine that switches the saw on and off, pressing Space does
  nothing; it works only when he is above it.
- Cause: `WorldRuntime::TryActivateSwitch` looked only at the block under his feet. The original's
  switch (384/385, an ON/OFF panel) is a tile in Blupi's own cell, which he stands in front of
  (Decor::IsSwitch: his middle within it, 4 px from its sides; its `table_decor_quart` mask empty).
  A switch standing at his height -- a machine on the floor or on a pillar, as world 065's on its
  stepping stone -- could only be pressed by jumping on top of it.
- Fix: the switch he stands on, or else one beside him at his own height whose footprint his body
  touches (his middle within 18 + 8 px of it; the nearest of the eight around him). Its saws are
  toggled as before (its row, X +-20).
- Validation: `VerifyInteractionSystem` (a machine beside him a step away is out of reach, against
  it he turns it and its saws on); every playthrough with a switch (023, 025, 063, 064, 065, 092,
  102) passes with it.

## OR-0068 — Blupi stands on a side fan's invisible floor instead of setting it off (owner 2026-11-04) — FIXED

- Report: Blupi jumps onto a fan blowing sideways and stands on a transparent floor instead of the
  fan exploding.
- Cause: a fan is a whole block (M3D-0004), so its top held him; only a floor fan counted his feet
  on it as contact (OR-0066 kept the mouth contact of the side and ceiling fans). Its model does not
  fill the cell, so he stood on air.
- Fix: landed on any fan (his middle over it, his feet on its top) he sets it off, with the wind it
  blows (`WorldRuntime::TryConsumeFan`), as the owner asked of the fans in general.
- Validation: gallery 998, dropped onto its side fan (18, 1, 16): the old binary left him standing
  at y 2.00, now it explodes on landing. `VerifyInteractionSystem` (beside a side fan's top safe,
  on it it goes with its wind); the playthroughs with fans (034, 051, 066, 084, 102, 107, 113, 114,
  115, 124) and with switches (023, 025, 063, 064, 065, 092) pass.

## OR-0069 — the decorations Blupi passes through in 2D should be impassable in 3D (owner 2026-11-04) — DONE

- Request: the various decorations Blupi can walk through in Mobile Eggbert should probably be
  impassable in Galaxy Eggbert: in 2D he walks in front of them, but this is 3D.
- The original's decor has an empty quarter mask (`table_decor_quart`), so the registry made it
  `decor`: no collision (D41). The 58 such icons the campaign places were sorted by eye
  (`kSolidIn3DDecor`, BlockDefinitionRegistry.hpp):
  - solid now, a whole block each (as the fans, M3D-0004): the columns (76; the toys' striped
    posts 235/236), the girder (201), the machine pieces 49, 50, 51, the golden support (77), the
    trestle (199), the posts (200), the slime column (300), the balustrade (398);
  - passable still: the original's **secret passages**, walls one walks through by design (the
    original editor's "secret" families: tech-blocks 22/73, machine pieces 85/86; the cheese 339,
    `worlds/cheese.py` SECRET), the cobwebs (401-403), the small bolts and knobs at a cell's edge
    (53-64), the slime's lumps (285, 286, 302, 303), the drip (410), the arches (198, 400: ways
    through) and the world and mission signs (touched to travel).
- Worlds and playthroughs: 056's way to the lift goes round the girder tower (32, 62); 058's two
  pillars stood on its stepping stones over the lava, moved to the landing's and the pedestal's
  corners. `tools/data/blocks.csv` regenerated; flightcheck over every world: no target lost.
- Validation: `VerifyBlupiMovement` (Blupi stands on girders; a cobweb is still passed through),
  `VerifyDefinitionRegistries`; play pass 21 found the routes through the walls first made solid
  (the secret cheese and tech-blocks among them, since returned to passable); 042, 045, 056, 058,
  064 and the chains 040, 050, 060 pass with the final list.

## OR-0070 / OR-0071 / OR-0072 — the 3D editor: variants, turning, moving platforms (owner 2026-11-04) — FIXED

- Reports: in the 3D editor changing the variant does not work for many icons; it should be
  possible to set an element's rotation, which the world file probably stores; placing moving
  platforms seems impossible too.
- Causes: the original's variant cycling (ED-0001) works only by clicking again the block the
  previous click placed, and only within the original's variant families; the many tiles the
  re-tiling chooses (rock, grass, slime, brick, pipes, palace edges) have no family, and the
  re-tiling itself could change the block just placed, ending the cycle. The turn key O swapped only
  a plate's, fan's or wind tile's axis; the models with a front faced their open side by
  themselves. A lift's or creature's end was set only by G (pick it) and T (its end at the aim);
  and no key was shown anywhere.
- Fixes: **V** turns the aimed block to its next variant: its family's, else its re-tiling row's
  edge pieces (an edge the re-tiling chose can be changed by hand), undoable (OR-0070). **O** turns
  a model with a front (teleporters, signs, the switch, balustrade, trestle, emitter) a quarter at a
  time from the side it faces by itself, stored in the same per-cell byte as the plates' turn
  (`PlateRotationMetadata.hpp` `QuarterTurns`: 0-3; a plate, fan or wind tile reads only its odd
  bit, so the saved worlds read as before), undoable (OR-0071). A lift or a creature on a patrol
  placed, **the next click sets where it goes**, its step scaled to the cells, as the original
  editor's `m_liftEnd` (decdesign.cpp:479-506); a click on it again or Escape leaves it still
  (OR-0072). **H** shows the editor's keys over the view; a prompt there while an end is awaited.
  Later the same night: **O** also turns an object or a BigDecor in the preview's cell (where a
  click would put one) a quarter at a time, stored as that cell's metadata type 5
  (`kObjectTurnMetadataType`, its own, so a treasure lying in a wind cell turns without turning
  the wind; `World Format.md` "Turns"): drawn turned are the still objects (treasures, eggs, keys, pickups), the
  parked vehicles and the BigDecor models; a moving one keeps the facing of its travel.
- Validation: `VerifyGEWorldEditor` (V on a grass end and a fan, undone; O round a teleporter and
  a fan's two axes; a lift's end two up by the next click, no second lift); live: the keys shown by
  H; a lift placed and its end clicked, saved (49, 5, 26) -> (49, 7, 26); V turned the aimed grass
  end 108 to 107, saved; a treasure placed and turned twice (`build-probe/qa-shots/turnobj/`). Note for scripted runs: a key tapped faster than a frame is missed (the
  editor reads the keyboard each frame); hold it.

## 2026-11-02, during the Wave E session

| ID | Report | First notes | Status |
|---|---|---|---|
| OR-0028 | Blupi's sound pushing or pulling a crate sounds like a woman moaning in sex; Galaxy is a decent game: make it Mobile Eggbert's effort sigh. | Confirmed (BD-0031): Galaxy played every effect at the WAV's own pitch, but the original's Sound::Play sets each channel's table pitch (XNA octaves): the crate efforts 38/39 a whole octave up, most effects 0.1-0.5 up; and the push's 38 was looped for as long as he pushed (the original plays it once). Both fixed. Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0029 | The Setup: right of "Sound effects", volume for the sound and the music ("Sound effects", and right of it "Music"). | Done: two sliders in the Sounds row drawn as the original's Slider (jauge track, pad knob), saved (`effectsVolume`, `musicVolume`, 0-100); Erase progress moved a row lower. Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0030 | The fan should turn faster if it does in Mobile Eggbert. | Confirmed: the original turns it a frame every 2 ticks (Decor.cpp:1104-1122); Galaxy every 3. Fixed. | FIXED_AND_VERIFIED |
| OR-0032 | In the helicopter Blupi should climb with Left Ctrl. | Confirmed (Decor.cpp:3725/3760: Up or the Jump button climbs it); Galaxy climbed with Up only. Fixed (`VerifyBlupiMovement`). | FIXED_AND_VERIFIED |
| OR-0033 | Right Ctrl backs up; invent backing up for touch devices and gamepads too. | Done: Right Ctrl (on foot and in vehicles, Down keeps its turn-around), a gamepad's RB, an on-screen back-up button over the stick (a Galaxy addition: the original has no backing up). Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0034 | Check that Galaxy is ready for touch devices and supports a gamepad; fix what is not. | Found: no gamepad and no multi-touch at all (the on-screen pad read the mouse only, so stick and Jump could not be held together). Added: `GamepadInput.hpp` (the pad as keys, every phase), Enter/A plays from the main menu; `InputPad::UpdatePlayTouches` (several fingers). Untested on real hardware; the in-world editor stays mouse-driven. Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0035 | Some active cheats should show top-left as in Mobile Eggbert. | Done as the port draws them (InputPad.cpp:1290-1316): the persistent typed cheats (opendoors, megablupi, showsecret, quick, ghost) in a translucent panel top-left during Play. Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0036 | The growing, shrinking, vanishing saucer is probably made of square pieces, in Mobile Eggbert too. | Checked: its scene `Decor/temporaire.rd4` is five nested instances of the master "Cylindre" (round); the sprites show the side only (a stepped outline), which cannot tell round from square. Asked (2026-11-02): the owner keeps it round, as the source. | CONFIRMED_MATCH |
| OR-0037 | The red, green and blue columns (the coloured keys' doors): are they really round from above? | Checked: `Decor/porte-r.rd4`, `porte-v.rd4`, `porte-b.rd4` are each five "Cylindre" and two "Sphère" instances (a turned baluster): round, as drawn. | CONFIRMED_MATCH |
| OR-0038 | The circular saw renders wrong and a stone cube gets into it. | Confirmed: Galaxy drew the saw's floor cell as a rock cube (RockPile) whatever the floor around, with the saw as an upright sprite plate over it. The original's saw stands on the floor in Blupi's cell (IsScie; 134 of 135 cells have solid under them). Fixed: the cell is drawn as the commonest plain floor within 3 cells; the saw is `Decor/scie.rd4` (six frames, IoU 0.986-0.990, tone 1.6/60: its metal is shiny in the original) standing on it, turned with the placement (`kModelOverFloorBases`). Seen live in world 023. Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0040 | In Mobile Eggbert the fan cannot be passed: walking onto it Blupi dies and it explodes. Next to a fan one should be able to place wind tiles turned to a side (the editor). | Confirmed: Galaxy made fans never solid in collision, so he walked through them in 3D. Now a whole block, the mouth killing as before (M3D-0004). The editor: wind tiles are in the Hazards group (110), placed again on their cell they turn west, east, up, down as the original; and, the owner's choice, the O key turns a fan or wind tile to blow along Z (M3D-0005). Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0039 | The saucer does not match Mobile Eggbert: it vanishes only briefly, there for longer; standing on it when it vanishes Blupi should fall. | Measured in the original running (the port's copy, three saucers put under Blupi's start; 60 shots 0.28 s apart, `plans/evidence/orig-saucer-timing.png`): the cycle is ~4 s (the green frame 329 at 9.15 and 13.05 s), the saucer gone 1-2 shots (0.3-0.6 s: table_decor_temp's 2 of 20 steps, 0.4 s), Blupi standing on it falls the moment it goes. Galaxy uses the same table, divisor and passable window and drops him (`VerifyBlupiMovement`; 107's playthrough fell through one). The thin frames (328, one slab) may read as gone; no difference found. | CONFIRMED_MATCH (owner to look again with the 3D saucer) |
| OR-0041 | Left Shift: Blupi crouches and it looks as if he were kick-starting a motorbike. | Confirmed: table_blupi's third column holds Down at its last frame (33, 34, 35, then 35; Decor.cpp:2397) and Air at its fifth (27); Galaxy looped both, the clip too, so he bobbed up and down 7 times a second. Held now, sprite icon and 3D clip (`BlupiController::GetAnimHoldFrame`, `VerifyBlupiMovement`). Owner 2026-11-05 (MR-0008): the point was not the crouch's frames but that the crouched Blupi moved on when he pressed forward; in Mobile Eggbert the crouch ends and he walks. Fixed as the original (BD-0074): a step stands him up; the mines that hung at his height over a way now hang over his head (044, 045, 104, 107). | FIXED_AND_VERIFIED |
| OR-0042 | Blupi licking the lollipop is somehow too fast or jerky. | Its icons are the original's (234-236, each held 2 ticks: a lick every 0.4 s, as the original). The clip keys every tick and moves between them linearly, so each pose holds 50 ms and then jumps in 50 ms: stop-and-go. Now a pose the original shows exactly two ticks keys once and moves over both (`build_clip_library.py` `ease_doubled`; vehicles' clips excepted); 35 clips replaced after a rebuild without it matched the committed ones byte for byte. Analysed 2026-11-05 at the owner's request (MR-0008): the original's lick is table_blupi action 49 (Tables.cpp:326-329), 32 ticks at its 20 Hz (1.6 s, Decor.cpp:3212): three drawings 234, 235, 236, 235, ... each shown 2 ticks, so a lick every 0.4 s, and the picture changes in steps ten times a second (that is how the original looks: stepwise). Galaxy plays the same poses at the same times (BlupiController kSucetteFrames, the clip PickupBusy.Sucette 32 frames) and, since the fix above, moves between them: rendered at 60 frames a second the picture changes every frame (mean pixel change 1.4-7.0 per frame, slowest at the turns, never a hold), so it is smoother than the original, at the original's speed. Nothing to change unless the owner wants the original's steps. | CONFIRMED_MATCH |
| OR-0043 | The opendoors cheat does nothing; it should open every door in the hub, with the door sound. | Confirmed: Galaxy's version opened key and treasure doors (`isDoor()`: 334-336 only) and never the hubs' doors 182/183. The original flips m_bCheatDoors and runs AdaptDoors() again (Decor.cpp:1776-1779, 11534-11622): in a hub every gold bar and mission door slides up (sound 33) and the signs show completed; in a mission nothing opens (the typed word also gives the keys). Now so, kept for the session and applied as each hub loads (`HubDoors::AdaptHubDoors`, `HubDoorsTests`); the invented key/treasure opening removed. Live in hub 10: its three closed doors slide away (`plans/evidence/opendoors-hub10.png`, before above, after below). | FIXED_AND_VERIFIED |
| OR-0044 | The demo world (3D editor) should show really every variant of objects, elements and the rest, the area as large as needed: everything rendered in one place, to see at once what to improve, then send screenshots. | Done as a gallery of its own (world999 stays the tests' world): `worlds3d/world998.vwr` from `tools/worlds/gallery.py`, a 96 x 96 floor with four halls: every block icon 1-439 alone (the touch-triggered ones raised out of reach), the blocks as the worlds use them (saws in the floor, a crusher in its passage, the four fans with their winds, saucers, coloured doors with keys, two teleporter pairs, bars with and without mounts, every water tile, lava, spikes, a drip, a lightning pair, a spring, the clay bridge, the treasure doors, grass, a hub door, the gold bar), every object type (65; the patrolling ones move), every BigDecor the originals use (35). `plans/audit/gallery_map.md` gives the X, Y, Z of each (the editor shows the cell's). The editor puts it in the gamer's first free slot and keeps the copy current while it is unedited (`SeedGallerySlot`). `plans/evidence/gallery.png`. Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0045 | The vanishing saucer: really round discs, not square? Is it so in the rd4? | Answered: `Decor/temporaire.rd4` is five chained objects of one master whose primitive is `cyli` (a Ray Dream cylinder): round, as OR-0036. | CONFIRMED_MATCH |
| OR-0046 | What about the 3ds Max models? The installation and licence were given. | Answered: setup stopped earlier unaccepted (the key file a key-generator serial, the ISO a CRACKS folder; MR-0001); still BLOCKED until a genuine licence (the owner's own serial) is confirmed. | BLOCKED |
| OR-0047 | When Blupi dies, the angel Blupi is a billboard: make it 3D. | The angel is the Clear2 death's soul: element 230-241 rising 300 px (Decor::BlupiDead's VoyageInit, the icons cycled every 2 ticks), half transparent in the sheet (alpha 127), a turn about the vertical as its wings beat. Its scene `Blupi/blupi-ange.rd4` matches all twelve at IoU 0.965-0.982 turned 30 degrees a frame (objects.txt `angel`, `alpha=` option added for half-transparent sprites). Now drawn in 3D at half opacity where he died, rising its 300 px as `VoyageDraw` does, the HUD sprite left out; its icons now advance once every 2 ticks (they advanced every frame of an even tick, about three times too fast). `plans/evidence/death-angel-3d.png` (a fall off a probe world). | FIXED_AND_VERIFIED |
| OR-0048 | The back-up button is too far up; it should be lower. | Moved down to just over the stick's ring (centre 112 px over the stick's at scale 1, was 205); it still wins over the stick's touch area there. `VerifyGEInputPad`. Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0049 | The saucers (three groups of four) all vanish at once: is that playable? Surely one group should vanish while another stays, then swap. | Answered from the source: every saucer is icon 324 and runs on the one clock (drawn by m_time, Decor.cpp:1063-1065; passable at m_time/4%20 >= 18, 6734, 7505), so in the original all vanish together for 0.4 s every 4 s: cross in the 3.6 s after they return. 081, 083 and 107 are played so by the playthroughs. Checked 2026-11-05 on every campaign world that has saucers (block 324: 034 with 3, 081 with 172, 083 with 95, 107 with 21): each playthrough passes on the current build, crossing them in the 3.6 s window (081: 32 timed crossings, 083: 20, 107: 2; 034's row of three, at (17-19, 7, 39), is walked over without a wait). Should the owner mean another spot, its world is still asked. | CONFIRMED_MATCH |
| OR-0050 | Sometime check every rd4 or other model: is it converted into Galaxy's 3D format and into the game, where Galaxy can use it? | Swept: `tools/audit/model_sources.py` lists every scene against the sprites rendered from it (`plans/audit/model_sources.md`). 161 scenes are drawn as models, 8 more otherwise (the parked vehicles from Blupi's clips, the lava's `.a3d` wave). This sweep converted the saws' switch (384/385), the conveyor (311-316, stretched along Z to its cell's depth: its tread is 0.206 of a cell deep) and the slime drip (BD-0033); the column 375's `Decor/colonne1.rds` matched its silhouette but is a cube carrying a painted column (an embedded image and mask): the original was a picture on a box, so 375 keeps its sprite; seen live in a probe world (`plans/evidence/sweep-models.png`). Left, each on a question: `Decor/electro.rd4` (Q-005), the 129 terrain tiles' lumpy blocks (Q-004), the 56 `.max` files (MR-0001); 228 scenes have no game sprite (backdrops, menus, tests). The conveyor lifts (objects 47/48, world 102) are drawn as the same tread on the frame `table_chenille(i)` shows, no longer a textured cube (`plans/evidence/conveyor-lifts.png`; 102's playthrough passes). | FIXED_AND_VERIFIED |
| OR-0051 | The circular saw is wrong: only its upper half should be visible. | Confirmed: the original's sprite holds the blade's upper half only (rows 32-63, its hub on the cell's floor line); the model stands right (hub on the floor's top), but the saw's floor cube was drawn in the animated, semi-transparent pass, which writes no depth, so the blade's lower half showed through its own floor wherever no other floor hid it (a one-wide catwalk of saws, world 023). The saws' floor cubes are now drawn opaque with the static mesh (`m_sawFloorRenderer`). `plans/evidence/saw-half-blade.png`. | FIXED_AND_VERIFIED |
| OR-0052 | A treasure chest under water looks from above completely normal; there should be some effect that it is in water. | Confirmed: the terrain's semi-transparent pass (water, animated tiles; no depth writes) was drawn before Blupi and the object models, so they painted over the water. It is now drawn after them (`TerrainRenderer::DrawOpaque` / `DrawTransparent`): what lies in water shows through it, tinted (`plans/evidence/water-over-objects.png`, world 022's pool). | FIXED_AND_VERIFIED |
| OR-0053 | Think up what to do with the background in the 3D world: should it turn, change or transform as the camera changes? | It was a quad stuck to the camera: the same picture whatever the view. The original tiles its 640 x 480 picture and scrolls it at 2/3 of the world's scroll (Decor.cpp:650-700). Now a distant panorama: the picture wrapped round the horizon a whole number of times (about a screen width each, as before), scrolled by the camera's turn and pitch, and moved up and down by the camera's height at 2/3, as the original's vertical scroll. `plans/evidence/background-panorama.png` (three turns of the camera). Owner 2026-11-05: checked, fine. | FIXED_AND_VERIFIED |
| OR-0054 | It seems the music of 3D worlds can no longer be changed or set in the game editor. | Checked live: the M key (OR-0014) worked but was the only way and nothing showed the choice. The Background group now ends with a row of notes, none and tracks 1-10, the world's own marked; a click sets it (undoable), M still steps it. `VerifyGEEditorPalette`; `plans/evidence/editor-music.png`. | FIXED_AND_VERIFIED |
| OR-0055 | The clay bridge is wrong in 3D: it should be a cuboid of clay lumps taking some 20-30 % of the cube's height and covering its whole top, not only an edge. | Confirmed: `grotte-fragile1.rd4` is one row of five lumps along X, only 0.284 of a cell deep (the side view needed no more), so in 3D it was a strip across the cell; and the scene's teal 'Flat Surface' under the camera's frame was drawn too, cyan balls in the gap. The row is now drawn four times across the cell, every other one turned round, the outer rows' edges on the cell's (`BlockModels.hpp` `kBridgeRows`, `VerifyTileFaces`), the shaking and the crumbling as well; the surface is left out (`drop=2`, IoU 0.858-0.903 from 0.848-0.865). The slab keeps the original's height: the sprite's top 24 of 64 rows, 37 %. Seen live: a slab of lumps across the cell, crossed on foot (`plans/evidence/clay-bridge-slab.png`). | FIXED_AND_VERIFIED |
| OR-0056 | The teleporter booths with the numbers are placed wrong somewhere: the number cannot be seen from outside. | Confirmed: `Decor/teleporteur1-4.rd4` is a blue box the size of the cell with the letter plate and two lights on its front only (the original's sprite shows that front whichever way Blupi comes). The front turns to the side with the most room to stand in, which in every shipped world opens into free space, but not always the way Blupi comes (066's at (12, 33, 46) faces east along its corridor, Blupi comes down the shaft from the west), and the other three sides were blank. The plate and lights are now drawn on all four sides, the box once (`BlockModels.hpp` `IsTeleporterIcon`, `IsBoothBodyPart`, `kBoothSideOffsets`: the box is not centred, so each copy stands out to sit on its side as the front does; `VerifyTileFaces`), as a sign's painting is drawn front and back. Seen live: `plans/evidence/teleporter-four-sides.png`. **Reopened 2026-11-03** (owner: "mostly turned wrong"; I had closed it from the gallery only, without looking in the campaign): every front model (teleporters, the mission and world signs with their numbers, the numbered treasure posts 421-439, trestle, balustrade, lightning emitter) turned to its side with the most room counted only room over a floor two or more cells down, which suits a booth hanging over Blupi's head but gave every model standing on a floor nothing on any side, so its front fell to +X: the hubs' mission signs in their niches faced the niche's side wall (in hub 010 three of four numbers could not be seen from the room), the lower cell of a numbered post showed its number sideways. Now the floor right beside counts too and a hub door's bar counts as open (BlockModels.hpp `FrontModelOpenSide`, `VerifyTileFaces`); a row without room faces across itself. Checked: all 273 signs and numbered posts of the campaign now face an open side (none did not); seen live before/after in hub 010 and world 124 (`plans/evidence/numbered-fronts.png`); the teleporters photographed in the campaign from where the player comes, three cells off on each side with a floor (46 of the 49; 113 views, `build-probe/qa-shots/booths/`): the letter readable in every view. | FIXED_AND_VERIFIED |
| OR-0057 | The conveyor's 3D model is probably wrong: the belt should be on the other side too. | Confirmed: `Decor/chenille.rds` models the belt only on the side the sprite's camera saw (Z -0.02 to 0.08 of its base's -0.10 to 0.10); stretched across the cell (BlockModels.hpp `ModelDepthScale`) it ran along the front half and the back showed the bare base. Now drawn a second time mirrored along Z (`ModelMirroredInDepth`; the terrain conveyor 311-316 and the conveyor lifts 47/48), the belt's run along X unchanged. Seen live in the gallery from both aisles: the rollers on the north and the south side (`plans/evidence/conveyor-both-sides.png`). | FIXED_AND_VERIFIED |
| OR-0058 | Blupi comes to the saws' on/off switch and nothing happens on Space. | The gallery's fault, not the game's: Galaxy lays the switch in the floor (Blupi stands on it and presses Action, as in 102 and 065), and it turns the saws within 20 cells along X at its own height (Decor::ActiveSwitch); the gallery stood its two switches a cell above the saws' row. Moved into the floor (`tools/worlds/gallery.py`); checked by autoplay in world 998: pressed, Blupi walks the saws' row; not pressed, the saws kill him. | FIXED_AND_VERIFIED |
| OR-0059 | Blupi walks under the crusher and nothing happens to him. | Checked: he is squashed as in the original (Decor::IsEcraseur, BD-0032): while its block is down (three steps of its ten) he is flattened for ten seconds at half speed, no life lost; walked under between two strikes he passes untouched, as in the original. In the gallery the crusher stood in a closed one-high passage where the camera could not see him flattened; it now hangs from a beam open at its sides. Seen live: flattened (`blupi-plat.rd4`'s pose) and slowed after passing under. | CONFIRMED_MATCH (gallery made visible) |
| OR-0031 | Everything still a 2D billboard that has an rd4 or other 3D model should become 3D; in particular: the red, green and blue posts and the numbered posts; the vanishing piece of cube (Temp); the crusher (blue spring, red striking block) and Blupi squashed flat under it; the spring; the lightning; at least one grass; the clay bridge (a lump of clay shows above it after Blupi crosses, its top plain brown); the glue and the burn deaths (2D billboards). | Sources found: doors 334-336 `porte-r/v/b.rd4` (0.983), Temp 324-329 `temporaire.rd4` (0.985-0.992), crusher 317-323 `aplatisseur.rd4` (0.948-0.983), spring 211-213 `ressort.rd4` (0.98), bridge 364-367 `grotte-fragile1.rd4` (0.86), flat Blupi `blupi-plat.rd4`, glue `glu.rd4`; the lightning and the charred Blupi have no scene (to model). Found while at it: the original's crusher hangs in Blupi's cell of a one-high passage (IsEcraseur tests his own cell; 13/13 in the original's maps), Galaxy put it in the floor. Mine contact killing by burning is right half the time (BlupiDead(Clear1, Clear2), a coin flip). Done so far: the doors and the vanishing saucer (`e43bfd9`), the saw (OR-0038/0051), the crusher hanging in Blupi's cell of a one-high passage as `aplatisseur.rd4` (BD-0032; 045/053 rebuilt, 199's invented row removed), the spring as `ressort.rd4` bouncing through `table_ressort` as the original draws it (`plans/evidence/spring-model.png`), the signs as `world1.rd4` / `world2a.rd4` / `world2b.rd4` (IoU 0.995) with their digit or world picture as a decal cut from the sprite (MR-0006; `plans/evidence/signs-models.png`), the clay bridge as `grotte-fragile1.rd4` (364-366 in its cell, the crumbling 367-372 drawn by its ObjectType52 in the same cell; the lump over it was that object's fixed icon 365 one cell up; `plans/evidence/clay-bridge.png`; its surface is the scene's clay brown, `Decor/terre.tif` is used by no scene), the lightning's emitter 304 as `eclair.rd4` (`plans/evidence/lightning-emitter.png`; the bolt and the grass: `owner_questions.md` Q-001/Q-002). The burn deaths as his own model crumbling to ash (`39d800a`, `CharredBlupi.hpp`). Finished 2026-11-04 with the owner's answers: the bolt as a generated 3D bolt along its painted frames (Q-002, `707a09d`), the electrode as `electro.rd4` with its sparks (Q-005, `4818a92`), the glue death's strands (Q-006, `3eb4e4b`), the grass as `herbe1.rd4`'s lumpy slabs and 90 more terrain tiles as their scenes' reliefs near the camera (Q-001/Q-004/Q-012; `plans/evidence/terrain-reliefs-7-worlds.png`). No billboard with a 3D source of its own is left (the render ledger's open rows are tiles used in no world). | FIXED_AND_VERIFIED |

## 2026-11-01, second batch (after the first fixes of the day)

| ID | Report | First notes | Status |
|---|---|---|---|
| OR-0001 | The maximum number of eggs (lives) does not match Mobile Eggbert. In Mobile Eggbert Blupi still lives after the last egg is used; in Galaxy that is game over. The eggs at the bottom of the screen probably show the lives remaining, not the total. | Confirmed (Decor.cpp:6379-6396, 10176, 1185-1194): the row shows `m_nbVies`, the lives in reserve; a death with one or more spends one (it flies to him), only a death with none is game over. Galaxy ended the game when the row held one. Fixed: `LoseLife`/`ResolveDeathLock`; `VerifyInteractionSystem`. The maximum (MAX_EGG_COUNT, 10) already matched. | FIXED_AND_VERIFIED |
| OR-0002 | Blupi's farewell (Bye) when going to another world freezes and ends at once. | Reproduced by reading: Blupi stands on the marker the whole farewell, so the world-select check fired every frame and its early `return` skipped the rest of the update, the clip's playback included: the pose froze until the world loaded. Now only checked while not in Bye (Decor.cpp:5476). Live (AUTOPLAY_REALTIME=1, 0.1 s shots): the wave plays while the camera comes round to his front, then the hub loads (`plans/evidence/bye-farewell.png`). | FIXED_AND_VERIFIED |
| OR-0003 | Getting off the skateboard shows two skateboards: one already put down and the one he is putting down. | Confirmed: the board was put down the frame DeposeSkate began; the original starts it at the clip's tick 12 (Decor.cpp:4527-4533) and takes it off the ground at TakeSkate's tick 8 (4510-4518); Galaxy now does both (`PendingSkate`). Seen live 2026-11-05, every frame of 102's take and dismount (autoplay `shots N`, 3 game ticks per original tick): put down, the board stays in his hand to +35 and lies on the ground from +37 (the original's tick 12); taken, it leaves the ground at +24 (tick 8); never two boards (`plans/evidence/skate-take-and-dismount.png`). | FIXED_AND_VERIFIED |
| OR-0004 | The teleporters (alpha, beta, ...) — reported without detail. | Looked at live (world034: delta to the rock cave): the transit matches the original (the 128-tick Teleporte frames, spin and vanishing, sound 71, the arc). Differences, fixed (BD-0017): the original lands him under the partner teleporter, facing as he did, amid two sparkle clusters and without a second sound, and takes him again only once he has left that spot (`m_blupiPosHelico`, Decor.cpp:5594, 6355-6360); Galaxy put him one cell aside, turned about, with the entry sound again. Compared with the original running: `plans/evidence/teleport-arrival.png`. The report gave no detail: anything else about teleporters needs the owner's word. | FIXED (owner to confirm the rest) |
| OR-0005 | Wasps, fish, birds and others move too fast, not like Mobile Eggbert. | Confirmed for every creature type: the original's records (~1,000 in ../mobile-eggbert/worlds) give each type ONE speed along the path's longer axis and ONE rest (bulldozer 30 ticks a cell, moving mine 10, fish 15, bird 8, robot 8, creature 10, wasp 6, big creature 20); 359 of Galaxy's creatures differed, many 2-7x too fast (moving mines up to 14 cells/s against 2). Now every creature gets its type's timing (BD-0016: `worldkit.CREATURE_TIMING`, the registry, the 3D editor); `tools/audit/patrol_envelope.py` reports 0 off. Playthroughs re-timed. | FIXED_AND_VERIFIED |
| OR-0006 | A world with moving platforms that move too fast to jump across. | BD-0006 fixed 31 lifts (024, 061, 062, 073, 113) on 2026-11-01; ask which world if it persists after that build | FIXED (to confirm) |
| OR-0007 | Cheats do not work, e.g. "givecopter" does nothing; probably none work. | Confirmed: Galaxy knew only "ghost" and "quick" of the port's typed words (`InputPad.cpp:686-860`). All 21 now fire their `Decor::CheatAction` effect (vehicles at once, shield, lollipop/charge/drink clips, ten bombs, ten glue shots, a dynamite stick, all keys, and the menu's nine); "killegg" does nothing there either. `VerifyGEInputPad`, `VerifyBlupiMovement`. | FIXED_AND_VERIFIED |
| OR-0008 | The window title says "Galaxy Eggbert (CNA)": " (CNA)" must go; the executable should be GalaxyEggbert, not GalaxyEggbertCNA. | Fixed: the title is "Galaxy Eggbert"; the target `GalaxyEggbertCNA` (kept for build commands) now produces `GalaxyEggbert` (`.exe`, `.html`; the Linux bundle `GalaxyEggbert-linux-*`); scripts and docs follow. | FIXED_AND_VERIFIED |
| OR-0009 | New goal: a completely different loading screen (today it is Mobile Eggbert's). | Done: while the game loads, Galaxy's title scene (`worlds3d/title.vwr`, as behind the menu) plays under the title "Galaxy Eggbert", "Loading" and the original's gauge (`jauge.png`, its waitTable fill over the 5 s) in place of Mobile Eggbert's picture, which stays only as the fallback when the scene cannot load. Evidence `plans/evidence/loading.png`. | FIXED_AND_VERIFIED |
| OR-0010 | The Setup menu: only its first screen works; "Automatic zoom on action" does nothing (not implemented), nor does the accelerometer. | Confirmed: Galaxy's Setup wired Sounds/Reset/Return only; Jump side, Automatic zoom and Accelerometer were drawn but inert. Now all toggle and are saved (GameData data[4..6], the original's defaults: jump right and zoom on, accelerometer off); the Jump side mirrors the stick and the Jump/Action buttons (InputPad.cpp:232-240, 1718-1760); the automatic zoom brings the chase camera to 1.3x while he walks, lingering 30 ticks (Decor::MoveHotSpot; live: `plans/evidence/auto-zoom.png`); the accelerometer steers by sideways tilt with the original's dead zone, hysteresis and level-start (InputPad.cpp:1932-1969) where the device has one (none on this desktop: untested live); the sensitivity slider is not drawn yet (fixed 50). | FIXED (accelerometer untested on hardware) | |
| OR-0011 | Make it playable on mobile devices (Android), with the accelerometer if realistic. | on-screen pad exists (MENU-021..024); the accelerometer steers by tilt with the original's dead zone (OR-0010); touch and gamepad checked (2026-11-02). 2026-11-03: the Android project still described the pre-CNA game (an executable, GLES 2, the 2D worlds, CNA looked for beside the repository, a stale copy of SDL's Java glue). Ported (`c5dcdc5`): the game is `libmain.so` with CNA's `Entrypoint.hpp` (SDL_main); its relative-path files are copied from the APK into internal storage on first start (`AndroidAssets.cpp`, by a Gradle-generated `assets.txt` with a stamp) and the game works there (saves, custom worlds); NDK r29, API 24 (CNA's minimum), GLES 3, SDL's Java glue from CNA's SDL. The x86_64 debug APK (102 MB) installs in the API 35 emulator, copies its 2,562 assets, draws the title scene and reaches the main menu (`plans/evidence/android-emulator-loading.png`, `android-emulator-menu.png`). Loading first stalled in CNA: every texture's missing `.xnb` made CNA list the APK's assets through JNI (UB-006); the models' content root is now absolute on Android (`c48ee08`). A held touch on Play enters the main hub with the on-screen pad and the lives row (`android-emulator-hub.png`; a quick tap was missed there, most likely because a frame takes seconds without KVM). Not drawn on Android: Blupi and every object model -- CNA's skinned shader needs more than the 256 vertex uniform vectors GLES 3.0 guarantees and does not link (logcat, now carrying the game's stdout; UB-007, CNA's to fix: no workaround in the game). The emulator ran without KVM (software emulation), so play, touch and tilt could not be judged (Q-009). | builds, starts, menu and hub reached; models wait on CNA (UB-007); play on a device or a KVM emulator open (Q-009) |
| OR-0012 | The 3D editor's menu looks like a prototype: make it look normal. | The editor's world list (the menu's 3D World Editor button) was flat grey rows on a blue screen. It now stands on the title scene as the menu does, with the menu's translucent panels and the pad's buttons: a numbered button per slot in two columns of five, the trash (a tick asks to confirm), Setup's return arrow; larger text. The in-world editor's palette already has Eggbert 2's green buttons; if its coordinate bar (X-/X+ ... PLACE) is what looked like a prototype, that needs the owner's word. Evidence `plans/evidence/editor-browser.png`. | FIXED_AND_VERIFIED |
| OR-0013 | Nothing is animated in the 3D editor; in blupi_recon every animatable element and object is animated. | Confirmed: the editor never ran the world's animation clocks. It now does (WorldRuntime::Update only advances tile and object animation; patrols stay the game's), so lava, fans, water and every object animate in place while editing. Seen live: with the camera still, the exit arrow and the rows of objects change from frame to frame (`plans/evidence/editor-animates.png`). | FIXED_AND_VERIFIED | |
| OR-0014 | Each world gets one of 10 background music tracks (the Speedy Eggbert MIDI, converted to WAV), or none; set in the 3D editor; the owner leaves the choice for the existing worlds to me. | Done: the ten tracks are Speedy Eggbert's own `SOUND/MUSIC000-009.BLP` (Standard MIDI, from `Speedy_Eggbert_1.7z`), rendered by `tools/music/render_music.py` (libfluidsynth, the GM soundfont, trailing silence cut, loudness normalised) to `Content/music/music000-009.ogg` (OGG Vorbis: CNA decodes it; a WAV is ~64 MB a track). A world's music is stored in its header (sky-region word's second byte, 0 none, 1-10); the existing worlds take the original level's own `music=` (hubs mostly none); the 3D editor's M key steps it (undoable) and the game plays it while editing; it plays in play, pause and setup when sound is on, never in autoplay. On the web since 2026-11-03 (`789ceee`): not packed into the 94 MB `.data` (27 MB more) but served beside the page in `Content/music/`, each track fetched the first time a world plays it (`UpdateMusic`, `emscripten_async_wget`); checked in headless Chrome: the editor's M key fetched `music000.ogg` (one GET) and played it. | FIXED (heard live: to do -- runs here are silent) |
| OR-0015 | The death animations are mostly wrong, e.g. Blupi cheers, and a mine icon flies from him instead of an egg. | The flying icon: fixed (the lives row's small Blupi, icon 48 of the Blupi sheet). The death clips: the original draws Clear1 (lightning, mines, dynamite: charred, crumbling to ash), Clear3 (lava) and Glu from element.png (Decor.cpp:2403-2410), but the 3D clips were built from those numbers on Blupi's sheet, where 46/47 is his raised-arms wave: he cheered as he died. Those deaths now show the original's own element sprite in place of the model (no Ray Dream scene of the charred Blupi exists; `glu.rd4` could give Glu a 3D clip later). Clear2 (invisible), Clear4 (saw) and Drown keep their clips. Seen live in probe worlds (mine: charred, crumbling to ash; lava: charred in its smoke, then rising): `plans/evidence/death-mine.png`, `death-lava.png`. The look found one more fault, fixed: the lava death's rising icon was drawn during its 30-tick hold, a second charred Blupi on the screen beside the first (the original draws it only once it moves, Decor.cpp:10328-10331). 2026-11-02, no billboard of him: the charred frames (40-47, Clear1 and Clear3's first half) are now his own model in his Stop pose, burnt black, shrunk toward his feet frame by frame to each sprite's measured height and spread out as the heap, the sprite's smoke over the heap (`CharredBlupi.hpp`, `VerifyBlupiMovement`; `plans/evidence/death-burn-3d.png`). Glu's sprite was drawn half a cell over the floor and then stood on his feet (`plans/evidence/death-glue-grounded.png`); the glue death is now `Blupi/glu.rd4`'s pose recoloured to the sprites' green at conversion (`texrecolour`), his own model flattened under it as the puddle; the strands are not drawn (Q-006; `plans/evidence/death-glue-3d.png`). | FIXED_AND_VERIFIED |
| OR-0016 | After the invincibility drink, instead of Mobile Eggbert's transparent Blupi there is a small Blupi spinning. | Confirmed (Decor.cpp:828-838): under the drink's Hide the original draws Blupi's own sprite at 30 % opacity, flashing to full every other pair of ticks in the gauge's last 25; Galaxy switched him to the Hide clip (BlupiAction::Hide is the invisible state after a death, a different thing). Now his clips stay and the model is drawn at that opacity, not depth-written. Live: `plans/evidence/hide-drink.png`. | FIXED_AND_VERIFIED | |
| OR-0017 | Walking along a wall close to it, part of Blupi is inside the wall. | The original's box is 36 px across (Decor::BlupiRect, pos.X + 12 to + 48): its sprite barely overlaps a wall; Galaxy's collision is a point, so half his body goes in. Tried twice. 2026-11-01: 18 px in ResolveMove, 55 of 70 playthroughs failed. 2026-11-02 (`plans/audit/or0017-body-width.patch`): the leading face's middle and corners at feet and head, world blocks only (crates keep their push geometry), the step-up gated on a real step at the face, and the harness's `go` taking 'pressed against a wall within its tolerance + 18 px' as arrived: still 31 of the first 42 failed, and not for targets: every step by a wall takes a little longer, so the waits synchronised to lifts and the mission clock drift (011 walks onto a lift's place as it is gone), and walkways along walls put him further out over their edges. Needs the playthroughs re-planned world by world with the patch in place (autoplay_search for the waits). 2026-11-02: the patch on today's tree fails 55 of 78 (missed lifts, timed waits, stuck short of points by walls). Meanwhile his model is drawn out of walls instead (`BlupiController::WallClearanceOffset`: within 18 px of a wall face, measured at his body and head, the model keeps the original's half-width from it; render only, his position, collision and contacts unchanged); `plans/evidence/or0017-wall-clearance.png`. The collision box itself stays open (the patch). 2026-11-03, the patch taken further (`plans/audit/or0017-body-width.patch`, v2): the box's bottom 2 px over his feet as `Decor::BlupiRect` (a lift's top a hair under a deck no longer stops him: 056), ground and ceiling under the box's whole width as `Decor::BlupiIsGround`, a `go` pressed against a wall arriving after 2 still ticks, not 8: 36-37 of 78 playthroughs pass (22 with v1); the ~30 mission scripts left need re-planning one by one (first diagnosed: 012's pull starts only when the `go` before it ends within 0.05 of its point, which the box's new path round the alcove's corner misses). Later the same day, patch v5 (corners read cells' own shapes, ground only over a top, the harness's wall rules only when he is still and not turning) and 16 scripts re-planned to pass with and without it: **60/78** with the box, 78/78 without (`handoff.md` lists what is left). 2026-11-03, evening: patch v6 (the harness's wall rule needs him still vertically too) and the last scripts re-planned (066 along its one-wide tunnels 0.3 off the mines' line, as near the wall as his body goes, no world change): **78/78 with the box**, run as the play pass (`tools/play_pass.sh`), and 78/78 without it; now in the game (BlupiController `kBodyHalfWidth`, `ResolveMove`'s `halfWidth`; VerifyBlupiMovement: his face stops 18 px from a wall, he stands while a corner of the box is over a ledge, falls when none is); the render-only `WallClearanceOffset` is gone, the patch file with it. | FIXED_AND_VERIFIED |
| OR-0018 | On a moving platform a wrong animation plays, as if Blupi were falling. | Confirmed: in `Step()` a lift is no ground, so each frame he fell a frame's gravity, ended the step in the air and took the Air clip; `RideLift()` put him back only afterwards. Now riding (and not rising) counts as standing for his clip only (a first cut that also removed the gravity changed the riding itself: 066's lift boarding failed). `VerifyBlupiMovement`. | FIXED_AND_VERIFIED |
| OR-0019 | Flying the helicopter into a block's ceiling should wreck it; nothing happens. | Confirmed (Decor.cpp:5264-5283): a ceiling within 7 px over his head wrecks the helicopter (explosion, small shake, sound 10) unless the shield, Hide or SuperBlupi. Added (`HelicopterHitsCeiling`, `WreckVehicle`). | FIXED_AND_VERIFIED |
| OR-0020 | In the helicopter in the air (not at the floor), pressing Space leaves the helicopter hanging in the air and Blupi falls; Mobile Eggbert probably does not behave so. | Confirmed (Decor.cpp:3889-3915 and the other vehicles' sites): the action button leaves a vehicle only on the ground and off lifts; the helicopter and the overcraft need both feet's probes on ground; the skateboard less than 8 px a tick. Galaxy left any vehicle anywhere. `CanLeaveVehicle`. | FIXED_AND_VERIFIED |
| OR-0021 | Pulling a crate back sometimes does not work. | The pull needed a whole free cell behind Blupi (and none of it inside another crate's cell); now only his collision point where it puts him (BD-0014). | FIXED_AND_VERIFIED |
| OR-0022 | Pulling a crate, if all keys are released Blupi keeps pulling it by himself for another second. | M3D-0001's committed move removed: it stops the frame he lets go (BD-0014). | FIXED_AND_VERIFIED |
| OR-0023 | In Mobile Eggbert a crate can be pulled onto another crate so that part of it overhangs; in Galaxy it cannot. "You skimped on comparing the crate logic." | Crates rest at any position and need only the original's 20 px corner supports (four in 3D), so they overhang ledges and crates as in the original (BD-0014). | FIXED_AND_VERIFIED |

OR-0021..0023 overturned M3D-0001's first reading: crates are continuous now (BD-0014, M3D-0001
rewritten).

## 2026-11-02

| ID | Report | First notes | Status |
|---|---|---|---|
| OR-0024 | Blupi's hands on the bar are wrong: he should hang a little lower, so his hands are at the bar. | Confirmed: the original hangs him 6 px lower than standing in the bar's cell (BLUPISUSPEND 12, BLUPIOFFY 6); his model now hangs so, palms at the pipe's height. The hands were also across him, off a single pipe: see OR-0026 (M3D-0003). | FIXED_AND_VERIFIED |
| OR-0025 | The teleporter's 3D model has been wrong since before this session; compare with Mobile Eggbert: it takes one cube only, the tip 2/3 of its height, the device the top 1/3. | Confirmed: its tile 330-333 is one cell, the device on top, the tip under it; Galaxy drew a cube with the panel on four sides and the tip as a pyramid in the cell below. Now each letter is its original scene's model (Decor/teleporteur1-4.rd4, IoU 0.997) in its cell, its front toward the room; the converter's flat end caps fixed on the way (the letter panel showed a magnified corner of its letter; the same fix gives the house and the cabin their front textures). | FIXED_AND_VERIFIED |
| OR-0026 | Make the bar 3D, not just a billboard; the 3D game probably needs two bars side by side, one per hand: find out and decide. | Decided two pipes (M3D-0003): the original's hanging poses hold the hands 0.35 cell apart across him. A first reading (one pipe, Blupi turned across it) was wrong: the sprites show him in profile. Both bars are the original model (Decor/barre.rd4; 202 its pipe alone). | FIXED_AND_VERIFIED |
| OR-0027 | The bar with its mounts shows even where no block is above it: probably a bug. | Confirmed: the original's editor gives a bar its mounts (138) only under a tile they hold to, else the plain pipe (202) (Decor::AdaptMidBorder, table_adapt_decor, Decor::IsRightBorder: under nothing, another bar and 84 listed tiles none; 301 of the 303 bars of its levels so). Galaxy's worlds place either anywhere; the cell now shows the variant its tile above calls for (ShownBarIcon; a first cut asked for a solid block and was 296 of 303). | FIXED_AND_VERIFIED |

## OR-0073 — the mission signs' digits looked ugly and damaged (owner 2026-11-06) — FIXED_AND_VERIFIED

- Report: "the numbers on the teleports to the other worlds -- the number sticker looks ugly and
  damaged; compare with Mobile Eggbert".
- Mobile Eggbert: the mission signs 174-181 carry a clean red digit painted on the blue sign.
- Galaxy: the digit is a decal on the sign's Ray Dream model (`avatars3d/objects/sign174_world1`).
  `tools/rd4_to_gltf/sign_decals.py` kept the pixels that differ from the median of the eight icons;
  for eight different digits that median is itself a blend, so the decals (20 x 23) were a mosaic:
  parts of the digit missing, blue sign and other digits' traces in it (seen live in hub 010).
- Fix (Codex CLI to a brief, reviewed): the digits are keyed on their red paint, the edge coverage
  unmixed from the blue face, the contour supersampled 8x (160 x 184, premultiplied alpha for the
  decal pass's blend); region and depth unchanged; the picture and plate modes give byte-identical
  output.
- Validation: mask IoU against the sprites 0.93-1.00, areas 0.99-1.01; in-game before/after in hub 010
  (`plans/evidence/sign-digits-before-after.jpg`; the full set in `build-probe/sign-digits/`).

## OR-0074 — the game stuttered when Blupi stepped on the crumbling clay bridge (owner 2026-11-06) — FIXED_AND_VERIFIED

- Report: "the game stutters when Blupi steps onto the clay bridge that disappears".
- Cause (measured with GALAXY_PERF, which now also times terrain rebuilds): the bridge
  (ObjectType52, Decor.cpp ~8540-8562) writes its `table_bridge` frame into its cell every tick, and
  every changed value made `RefreshTerrainMeshIfChanged()` build a new `TerrainRenderer` of the whole
  world on the main thread: a median of 123 ms (world065) to 189 ms (world094) each, ~30 times per
  crossing. The crumbling frames 367-372 were also drawn as flat sprite plates in the terrain under
  the object's model.
- Fix (Codex CLI to a brief, reviewed): all bridge frames 364-372 are model-drawn (no flat plate);
  `World` keeps a bounded journal of changed cells (256; metadata edits and overflow clear it), and a
  change between air and non-occluding models that touches no baked terrain data (faces, reliefs,
  lava bottoms, saw floors, grass) patches the model-cell list (`TerrainModelCells.hpp`, one per-cell
  rule shared with the constructor); anything else rebuilds as before.
- Validation: `VerifyTileFaces` (the patched list equals a full recomputation through a whole bridge
  sequence beside other model cells; a plain cube refused); full rebuilds in the playthroughs
  world065 211 -> 6 (the saw switches' batches), world094 259 -> 0, a patch 0.02-0.04 ms; ctest
  112/112; golden frames re-baselined only for world999's bridge exhibits (models instead of plates,
  ~650 pixels each), trace unchanged; screenshots `build-probe/bridge-stutter/`.
- Left: a saw switch still rebuilds the whole terrain (~180 ms under xvfb) when it turns its saws.
  (Since fixed: the saw switch's anim swap is patched, FindSawAnimSwaps; and on 2026-11-12 the rest,
  below.)
- **2026-11-12, the rest of the lag (owner's choice after the 0.1.x reports):** every other terrain
  change (dynamite cutting a saw, a drip or a fan; any change PatchModelCells refuses) still built
  the whole terrain on the main thread: 60-70 ms in world065 at the classic look, 150-160 ms with the
  Ultra effects (the light bake), measured under Xvfb. The static terrain is now kept in chunks of
  32^3 cells (TerrainRenderer::StaticChunk, drawn chunk by chunk) and a change rebuilds only the
  chunks its cells reach (TerrainRenderer::ChunksReachedBy: 2 cells every way; along the four
  horizontal lines up to 8 the tiles textured on one side and the still models that look that far;
  Ultra's ray reach every way), 1.4 chunks a change on average: 5-7 ms in world065, 2.6-2.9 in
  world105, 12-13 with Ultra and the reliefs. Checked: VerifyTerrainChunks (every chunk outside the
  reach keeps its fingerprint through 210 changes in four shipped worlds and 120 placed across chunk
  faces; it fails when the reach is cut to 1 or 3, or a line rule is dropped), live (a probe digging
  holes in world065 and world105: vertex, primitive, block, animated, water, model and relief-chunk
  counts and the centroid equal to a full build's after each change, at the classic look and at Ultra
  with the reliefs), the golden frames byte-equal at all three effect levels and the golden trace,
  CTest 131/131. Approximation: at Ultra a change's light bake is refreshed within the rays' reach
  (12); the sun's ray from a face a bounce met (48) is not followed farther.
- **2026-11-12 evening, the rest (the owner: "the rest of the stutter", "a faster editor"):** a chunk
  rebuild cleared the world's lava, lightning and wind frames whenever a rebuilt chunk held any and
  left them to the terrain's next Update, which in play comes before the refresh in the tick: the frame
  after a blast near lava drew no lava (found by comparing a chunk-rebuilt terrain with a full build
  after editor edits: equal but for those meshes). `RebuildChangedChunks` now makes them again at once,
  at the step they showed. The editor no longer built its whole presentation after each edit (the
  background picture decoded again, Ultra's sky light, a new terrain): `RebuildWorldPresentation(true)`
  keeps the picture of the same region and patches or rebuilds the terrain's chunks as play does:
  world065 6-7 ms an edit instead of 76 (Xvfb). `GALAXY_PERF=1` now names an update or a draw over 20 ms
  and the updates CNA catches up before a draw: whole playthroughs of world065 (22 982 ticks) and
  world105 (10 802) have no long update in play, only the world's load (161-176 ms) and the mission's
  end (41-46 ms); their terrain changes were all patched. Left: the draws on a real GPU (not measurable
  under Xvfb), and on the web the audio running on the main thread (CNA's).


## OR-0075 — lightning kills ghost Blupi; he falls and is opaque (owner 2026-11-06) — FIXED_AND_VERIFIED

- Reports: "I turned on ghost Blupi; he should not interact with his surroundings, but the
  lightning killed him"; "In ghost mode Blupi should not have the falling animation but look
  see-through like a ghost."
- Fixed as MODERN Mobile Eggbert: explicit world/contact gate, March moving (including vertically),
  Stop idle, 0.75 opacity for model and sprite fallback. Source guards, preserved reference
  exceptions (awake homing and existing dynamite blasts), tests and screenshots: **BD-0081** in
  `behavior_differences.md`.
- Verified: 112/112 CTest, unchanged golden frames/trace, typed `ghost` probe through world025's
  active lightning and standing in it for a full cycle with three lives. Clear pose screenshots
  in `build-probe/ghost/poses-run/autoplay_shots/`.


## OR-0076 — in the 3D editor "after the click nothing appears" (owner 2026-11-06) — FIXED_AND_VERIFIED

- Report: placing the moving objects seemed not to work; after the click nothing appeared. Cause:
  the editor aimed with the camera's centre ray, so the red preview and every placement landed where
  the screen centre pointed, not under the mouse. Decision: "Ano, mířit kurzorem" (aim with the cursor).
- Fixed (editor UX, no gameplay): `WorldEditor::AimDirection` unprojects the cursor pixel (pinhole
  math from the camera's field of view and aspect, the viewport size) into the aim ray; `AimAt` casts
  it (voxel hit, else the y = 0 ground) and keeps the result, `UpdatePlacementPreview` derives the
  red preview from that kept aim plus the X/Y/Z offsets. So preview, click placement, removal, box
  fill, copy/paste, the moving objects' start/end clicks and BigDecor all use the one ray.
- Decisions: while the right button turns the view the mouse is captured (its x, y are deltas), so the
  centre aims; with the cursor over a palette control (including PLACE and the offset buttons) the
  last aimed cell stays, so PLACE and Enter place there, not under the button (if nothing was aimed
  yet, the centre aims); a touch is an SDL mouse event, so the touch point aims the same way
  (`EditorPalette::IsPointerOverControl` tells the UI hover).
- Verified: `VerifyGEWorldEditor` (cursor right/left/above/below the centre aim there; the right button
  aims at the centre; PLACE under the cursor places at the last aimed cell), the four editor verifiers
  moved to the cursor at the window centre, ctest 112/112; live `tools/qa_steps/editor_cursor_aim.txt`
  (fresh `QA_LIVE_RUN`): the preview follows the cursor (X:46 -> X:53), a click lays the object at
  the cursor, over PLACE the preview stays; shots `build-probe/editor-cursor/`.


## OR-0077 — "the saw on/off machine has no effect" in the gallery / showcase (owner 2026-11-06) — FIXED_AND_VERIFIED

- Report: walking up to the machine in the gallery and pressing Space, nothing happens. Cause: a switch
  (384/385) turns the saws 378/379 of its own row within 20 cells along X (Decor.cpp:7138-7147,
  `WorldRuntime::TryActivateSwitch`), and the exhibit switches stood alone on their pedestals:
  world998 (14, 1, 42) and (16, 1, 42), world999 (3, 1, 19) (world999's 384 at (97, 1, 17) and the
  gallery's z 56 / world999's z 67 floor rows already had saws in their rows).
- Fixed (the owner's choice: saws in the exhibit rows): world998 saws 378 at (7, 1, 42), (9, 1, 42)
  and a stopped 379 at (11, 1, 42), in the gaps of the exhibit row (`tools/worlds/gallery.py`,
  listed in `gallery_map.md`); world999 saws 378 at (4, 1, 19), (6, 1, 19), stopped 379 at (8, 1, 19)
  (`tools/GenerateSampleWorld3D.cpp`). Both regenerated with their generators.
- Found while proving it: from the exhibits' one-wide aisle Blupi's middle stays 0.5 from the
  switch block, out of the switch's reach (0.28 body + 0.125); he reaches it by walking against the
  block (the autoplay probes hold Up for that).
- Verified: `VerifyShippedWorlds` now checks for 998 and 999 that every exhibit switch has a saw in its
  row and that `TryActivateSwitch` (Blupi against it) turns saws (it fails on the previous worlds);
  ctest 112/112, golden frames and trace unchanged; autoplay probes
  `tools/autoplay/probes/exhibit_saws_998.txt` / `_999.txt` (shots in `build-probe/exhibit-saws/`).


## OR-0078 — the saw on/off machine still did nothing: walked into, not pressed (owner 2026-11-06, second report) — FIXED_AND_VERIFIED

- Report: "find out why the machine that turns the saw on/off does nothing; it should turn the saw
  on/off". The same symptom as OR-0077, which I had closed from the data alone (saws in the row, and a
  unit press at a hand-picked spot): the premise was wrong.
- Cause, found by pressing it in the running game (an autoplay probe that walks up and presses Action,
  with the new `switches` / `saws` conditions that count presses and turning saws): the switch block
  is passable (the original's switch is a tile in Blupi's own cell), so walking up to the machine
  takes him INTO its cell (z 41.95 for the switch at z 42), and `WorldRuntime::TryActivateSwitch` looked
  at the block under him and at the eight neighbours at his height, skipping `(dx, dz) = (0, 0)`: his
  own cell. Standing in the machine found no switch. (OR-0067's fix had only added the neighbours.)
- Fixed: the own cell counts (distance 0). Every switch machine at his height now works whether he
  stands against it or in it.
- Verified: `VerifyShippedWorlds` presses every exhibit switch of 998 and 999 against it and standing
  in it, each on a fresh runtime (it fails on the old code for "standing in it"); live probes
  `tools/autoplay/probes/exhibit_saws_998.txt` / `_999.txt` (traces: 998 saws turning 6 -> 4 -> 7, 999
  4 -> 1); autoplay gained the conditions `switches` and `saws` and a trace line on their change.


## OR-0079 — "the zoom cheat does not work, it does nothing" (owner 2026-11-07) — FIXED_AND_VERIFIED

- Report: typing the cheat word `zoom` does nothing. Cause: Galaxy's typed cheats were the original's
  21 words plus `ghost` and `quick`; Mobile Eggbert's MODERN-only `zoom` (view 100 -> 50 -> 25 -> 12 %,
  `SetCheatZoom`, reset on leaving the level; InputPad.cpp:742-811, Decor.cpp:589, 668-677, 11630)
  had been measured (Q-016) but not built, awaiting the owner. The report is taken as the answer:
  option (a).
- Built: `zoom` is a typed cheat (`InputPad::TypedCheat::Zoom`); each typing steps
  `cheatZoomStep_` 0 -> 1 -> 2 -> 3 -> 4 -> 0, the chase camera 1x, 2x, 4x, 8x, 16x as far (the 6 % step
  added at the owner's request, 2026-11-07; Mobile Eggbert's cycle stops at 12 %)
  (`kChaseDistance * 2^step`) and 0.25 rad higher a step toward a bird's-eye view, still stopped by
  walls (`RaymarchWallDistance`). The step shows top-left as "zoom 50%" / "25%" / "12%" like the other
  active cheats ("zoom 6%" at the last step) and is reset when a level loads. The cut-away of walls (option b) is not built: in
  caves the view stays near the walls.
- Verified: `VerifyGEInputPad` (the word fires its cheat), `tools/autoplay/probes/zoom_cheat.txt` in
  hub 070: `plans/evidence/zoom-cheat-implemented.png` (1x, 50 %, 25 %, 12 %, back to 1x); ctest, golden
  frames and trace unchanged (step 0 is the old camera).


## OR-0080 — the vehicles and the swimmer do not lean as in the original (owner 2026-11-07) — FIXED_AND_VERIFIED

- Report: the car should tilt when driven and does not. Found: the jeep's tilt exists (BD-0071: only
  over an edge, the original's `m_blupiRealRotation` 5 degrees a tick to 45; at 20 px a tick the
  window is short, as in the original). What Galaxy lacked were the original's other rotations
  (`Decor::BlupiSearchIcon`, Decor.cpp:2161, 2276-2330, drawn rotated at 774): the helicopter leans
  into its horizontal speed (vx x 2 degrees, max 24), a swimmer turns to his direction (the angle
  from up / sideways / down, + 15, approached 10 degrees a tick, minus 90) bobbing 20 degrees, a
  surfer bobs 10 degrees. The owner chose these (option E of the question).
- Built: `GalaxyEggbertGame::UpdateBodyTilt` (after each Step) turns the body's model about his middle
  (`bodyPitchRadians_`, in `BlupiModelWorld`); render only. In 3D the angle is a pitch (nose down),
  held to a quarter turn either way (the original's 105 degrees head down would tumble a body).
- Verified: a temporary print of the angles in probe world 993 (a pool, a helicopter): the helicopter
  9-19 degrees at its speeds, a swimmer -80 swimming up and 14 level, surf +-10;
  `plans/evidence/body-tilt-helicopter.png` and `body-tilt-swim.png` (from behind, the lean is
  visible on the turns); ctest and golden frames unchanged.


## OR-0081 — the saw switch's animation, and its panel seen edge-on (owner 2026-11-07) — FIXED_AND_VERIFIED

- Report: the original probably animates switching the saw machine on and off; Galaxy toggles it but
  shows no animation. Checked against `Decor.cpp`: `ActiveSwitch` (7134-7147) only swaps the tile 384 <->
  385 (the sprite sheet has just those two: lever up and green lamp for ON, lever down and red lamp for
  OFF) and Blupi plays his `Switch` action; no in-between frames. Galaxy does the same (the model's
  two poses, `switch384`, and Blupi's Switch clip).
- What did hide it: the machine's panel was turned to the open side with the most room, and with a
  free cell on all four sides (a pedestal in a hall, the exhibits) it stood edge-on to the player:
  the lever and lamps were a sliver. Fixed (owner's option A): the switch turns its panel to Blupi, to
  the side he is on where that side is open (else the old side); drawn per frame in the model pass
  (`GalaxyEggbertGame.cpp`, the front-model branch).
- Verified: probe world 992 (a 384 and a 385 on a floor; `plans/evidence/switch-faces-blupi.png`: both
  panels read from the front); ctest and golden frames unchanged. A smooth lever movement between the
  two states would be Galaxy's own addition (not asked).


## OR-0082 — the grass on the main screen has a nonsensical texture from above (owner 2026-11-07) — FIXED_AND_VERIFIED

- Report: the grass on the title scene (and wherever a grass top shows) has a meaningless texture seen
  from above. Cause: `textures3d/grass_top.png`, the texture the grass tops use (`grassEffect_`), was a
  synthetic bright-green noise made by a script that was never committed (commit `1d0d62a4`,
  2026-07-08); nothing in the original looks like it. Owner's choice: option A, that texture.
- Fixed: the texture is now made from the original's own grass (`tools/textures/grass_top.py`, run
  `python3 tools/textures/grass_top.py`): feathered patches of the leaf band of tile 107 in
  `Content/icons/object-m.png` (leaves only: no outline, no earth), laid at random on a wrapping
  64 x 64 canvas so it tiles; deterministic. It is a darker green with the leaves' blade highlights, as
  the leaves on the grass tiles' sprites.
- Verified: the title scene before and after (`plans/evidence/title-grass-leaves.png`); ctest and golden
  frames unchanged (the golden scenes show no grass top). The script also makes the asset reproducible
  (the provenance audit listed it as not).
