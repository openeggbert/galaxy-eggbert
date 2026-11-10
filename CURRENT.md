# Galaxy Eggbert — Current Truth

_Last verified: 2026-11-09 (native build, 117/117 CTest including the ten main-menu demo replays, live
and web checks of galaxy and demo sharing and of xnjttjpo's kept edits on the web,
golden frames equal to tests/golden (2026-11-09; the trace 2026-11-08), live checks of demo recording, xnjttjpo
and the players' galaxies (with delete/restore and Continue in a galaxy, 2026-11-09), controls/HUD/water checks and mission 051 with the resume self-test). The all-78-playthrough baseline remains
2026-11-06 (play pass 33: 77 replays equal). The Web/Emscripten build was verified
2026-11-09 in headless Chrome (demos, recording, galaxy, xnjttjpo, persistence over a reload); the whole-campaign run
remains the 2026-10-27 verification. Update this file
whenever a change alters the current state, a known limitation, a supported build, or the actionable
backlog. Keep historical detail in `plan.md` (and the frozen `NEXT.md`), where work stands in
`handoff.md`, the active plan in `plans/GALAXY_EGGBERT_COMPLETION.md`._

## What this project is

Galaxy Eggbert is a faithful 3D remake of `mobile-eggbert` / *Speedy Blupi*. The sole game target
is **`GalaxyEggbertCNA`**, built directly on CNA with cna-engine's strict modules as helpers (easy-3d
until 2026-10-20). The retired pre-CNA implementation was removed from the live tree on 2026-07-25
and remains available in git history at commit `4afd53e`.

The in-game 3D world editor is the one approved exception to the faithful-remake rule: it may add
content-authoring UX, but not new gameplay mechanics.

## Content state (2026-10-27)

- Real content: `world999` (showcase), `title.vwr`, the **main hub** `world001`, **all twelve
  worlds** (hub plus missions each: `world010`-`014` ... `world120`-`125`, 64 missions) and the
  **bonus world 199**: briefs in `plans/briefs/`, scripts in `tools/worlds/`, every lesson checked
  with `levelcheck.py`/`flightcheck.py` stages and live at its key moments. The campaign's structure:
  `tools/campaigncheck.py`; water Blupi cannot leave: `tools/watercheck.py`; inventories
  `plans/inventory/worlds.md` and `plans/inventory/assets.md`. Playthroughs by the QA autoplay
  (`tools/autoplay.sh`, scripts in `tools/autoplay/`, all replayed by `tools/autoplay_all.sh`):
  all 64 missions, each world also as one chain from its hub through its signs to the main hub
  (`world010.txt` ... `world120.txt`), world 199 to the win screen, and the whole campaign in one
  run from a new game (`campaign.txt`, `tools/autoplay_campaign.py`).
- Hubs work as the original's (`HubDoors.hpp`): mission bars slide up once the previous mission is
  won, gold bars in the main hub per completed world, the golden key ends a world. Crates are solid,
  pushed along X or Z and pulled (crouch + Down) at the original's speed with Blupi's hands on them
  (`CratePush.hpp`, D60, 2026-11-01: a move once started finishes its cell); the dynamite blast and
  mine contacts are 3D.
- Swimming as the original's (float at the surface, dive with LeftShift, jump out onto a bank);
  vehicles are wrecked by water; the helicopter drops glue; teleporter arrivals face on.
- Terrain, its grass and the cube objects are lit (one directional light + ambient,
  `TerrainLighting.hpp`); sprite tiles are cut out (`TileCoverage.hpp`), faint ones blended; lifts
  are drawn as their tile's bar and stem; the chase camera rises over obstacles behind Blupi.
- Since 2026-11-04 (owner, Q-001/Q-004/Q-012) 158 terrain tiles (every one with a scene of its own) show their Ray Dream scene's relief on
  their exposed faces within 8 blocks of the camera's target (`TerrainRelief.hpp`,
  `avatars3d/terrain/`, `tools/terrain_relief/`): rocks, caves, cheese, slime, toys, pipes, machine
  panels, the palace, windows, bookshelves; the grass as its scene's slabs back to back. Farther off,
  and for the tiles whose scene the archive lacks (bricks, grey metal plates, some panels), textured
  cubes.
- The brick wall's edge pieces 262/263/283 (owner 2026-11-09, Q-011: ragged only towards the open):
  at a wall's open end (air on the piece's clear side and across it, the wall going on behind) the
  cell is its four courses of bricks, each ending where its bricks end (`BrickEdges.hpp`,
  `TerrainRenderer` `AppendRaggedBrick`, faces only where seen, a whole neighbour's face drawn over
  the toothing it would hide); everywhere else a whole cube whose clear texels are the whole brick
  261's. Visual only (collision stays the cell). 13, 14, 14, 33 and 1 cells in 124, 123, 121, 034
  and 199 (the worlds scatter the pieces in walls, where they stay whole).
- Blupi's 3D model (from the Ray Dream originals) has clips for every live table_blupi action,
  including Win, Turn, Vertigo, Recede, the reactions (JumpAie, Ouf*) and the poses between moves.

## Shipped, working state

- The network game (2026-11-10, owner Q-019; **branch `multiplayer` only, not in `develop`** until
  the owner accepts it, Q-020): Speedy Blupi 2's multiplayer over CNA SystemLink on a LAN, 2-4
  players, Linux and Android (not the web): the menu's Multiplayer, the games on the network, the
  lobby (colour, ready, the host's mission and Start, chat), SB2's rules (ten lives, team starts
  and bombs, the crates/goals/enemy Blupis out, restart at the last safe place, the last present
  wins, a pause for everyone), the others drawn in their colours, world changes replicated, worlds
  201-205. Guide with ZeroTier: `docs/handbook/09-multiplayer.md`; differences: BD-0120; tests:
  `VerifyNetProtocol`, `tools/net_two_players.sh` (bomb, laid, shot, vehicle: PASS). Not yet run
  between two real machines.
- The player's files in the system's user folder (2026-11-10, the owner's request before a
  release; `UserFolder.hpp`): savedata.txt, resume.txt, demos/ and customworlds/ in
  `~/.local/share/Galaxy Eggbert` (`$XDG_DATA_HOME`), `%LOCALAPPDATA%\Galaxy Eggbert` on Windows
  (CNA's StorageDevice root), or `--user DIR` / `GALAXY_USER_DIR` (the test scripts: their run
  folder). An earlier build's files in the working folder are copied over on the first start.
  The web keeps /save, Android its private storage.
- The arrows and a gamepad on the demo positions, the galaxy screen and the world screen
  (2026-11-10, `FocusNavigator.hpp`): arrows or the d-pad choose a button (drawn lit), A or Enter
  presses it, the mouse or a touch ends the choice; B leaves the positions in Pause. With a gamepad
  plugged in the mouse still works in the menu and the editor's screens. A keyboard on the screen
  (`ScreenKeyboard.hpp`) names a galaxy or a demo without a keyboard. Demos have names (Name, then
  a recorded position; shown on the position and in Import's list, exported as `<name>.gdemo`).
  The galaxy screen's Delete galaxy (two clicks) keeps the galaxy as a pack Import brings back.
  Live: keyboard, arrows and a virtual gamepad under Xvfb; no physical gamepad or touch screen.
- A goodbye on quitting from the menu (2026-11-10, as Speedy Blupi's edition): thanks and credits
  over the title scene, a key, click, button or touch closes the game. Galaxy's own loading screen
  shows from the first frame (the logo until the title scene is up), not Mobile Eggbert's.
- Release packages (2026-11-10): `tools/release/make_release.sh` makes the Linux archive (CPack,
  versioned from `VERSION.txt`, with LICENSE, THIRD_PARTY.txt, RELEASE_NOTES.md; it needs glibc
  2.38 and the system's FFmpeg) and the release-signed Android APK (arm64), with SHA256SUMS.txt.
  1.0.0-rc1 built and the archive run; not published. Windows and a glibc 2.35 archive are open
  (Q-018).
- 2x textures (2026-11-09/10, session ray, `../ray.md`; owner-approved): `Content/icons2x/` and
  `Content/backgrounds2x/` load in place of the 1x sheets when 2x is wanted (`TextureScale.hpp`:
  only a file exactly twice the 1x one, else 1x; every pixel-to-UV step divides by the logical 1x
  size; the terrain's padded atlas 3200x3520 at 2x). The Setup's "Textures" (Auto / 1x / 2x, at
  the next start, `textureScale=`) or `GALAXY_TEXTURES=1x|2x|auto`; Auto picks 2x with Ultra on a
  display at least 1000 px tall. Web and Android always 1x; the APK leaves both directories out.
  The sheets are in (2026-11-10, ray's final set from `../speedyblupi-hd/galaxy/`, 9 icon sheets and
  39 backgrounds, each exactly 2x its 1x file; per cell provenance in
  `../speedyblupi-hd/manifest/build_galaxy.csv`: reduced from 8x Ray Dream renders where they
  passed verification, else an edge-padded per-cell Lanczos of the 1x pixels). Checked live at
  Ultra in a 1360x840 window (`textures 2x` in the log; no light band at the cells' edges, which an
  earlier whole-sheet stand-in had). Opaque tiles are made wholly opaque at load whatever the sheet.

- The world is drawn from a `SceneFrame` by `WorldRenderer` (2026-11-09, the owner's request; the seam
  of `renderers.md`, one renderer, no second one): `GalaxyEggbertGameScene.cpp` reads the simulation
  into a draw list (cubes, models and poses, decals, sparks, billboard batches, in the order the
  world has always been drawn), `src/GalaxyEggbert/Game/WorldRenderer.cpp` draws it with the effects
  the game loads; `GalaxyEggbertGameDraw.cpp` holds the frame (world, HUD/menus/editor, captures).
  A refactor only: golden frames, 1728 autoplay frames of 11 missions and 28 editor frames
  byte-equal to the build before it.
- 3D effects (2026-11-09, the owner's requests; the Setup's "3D effects" steps Off, Low, Medium,
  High, Ultra, saved as `effectsLevel=0..4`, off by default; `GALAXY_EFFECTS=0-4|off|low|medium|high|ultra`
  overrides it). Ultra (desktop): the world at the window's own pixels in HDR (ACES, auto exposure,
  HDR bloom, a grade per world), physically based light with the sky's light from the background,
  SSAO, contact shadows, screen-space reflections on the water and on metal and polished floors
  (from the atlas's surface map), a voxel GI baked into the terrain and read by the reliefs too
  (per relief chunk) and by the sprites and particles (by cell), the sprites and particles lit as
  the rest with fire, sparks and stars above white into the bloom, volumetric fog with god rays,
  TAA, depth of field, motion blur, grain, the atlas finer with normal and surface maps, the models
  subdivided (PN triangles); handoff.md has its parts. The water (medium and up) is no longer its
  2D tile: its colour and clearness from the tiles, a height field of waves in world space, Fresnel,
  a glint, Snell's window from inside (Ultra: rippling reflections). Low: distance fog in the colour of the world's background, soft round shadows,
  the explosions, splashes and sparks as particles cut from their own sprite frames and Blupi's
  moments (dust landing and running, a splash into water, bubbles under it, stars as he teleports,
  sparks off the lava; `EffectParticles`, stepped by ticks). Medium: + ambient occlusion baked into
  the terrain's corners, the water (ripples, caustics, sky at a glancing view, sun glints) and lava
  (pulse, crawling hot spots) shaders, FXAA. High: + real sun shadows (`SceneShaders`: two
  cascades, 44 blocks round the camera's target and 160 ahead, soft 16-sample PCF) thrown by the
  terrain, the cubes, every model and every sprite onto everything; point lights from explosions,
  the lightning, the nearest lava and the electrodes; the models' Ray Dream procedural materials
  (wood, marble, spots and bump, from `avatars3d/objects/*/materials.txt`,
  `tools/rd4_to_gltf/procedural_materials.py`); a glow on what shines with a light grade
  (`PostProcess`). `GALAXY_EFFECTS_DEBUG=shadow|occlusion` draws the terrain as that factor alone.
  Presentation only: the golden trace and the autoplay traces are equal with them on; off, every
  frame is byte-equal to the build without them. Golden frames per level: `tests/golden`,
  `tests/golden/effects-medium`, `tests/golden/effects-high` (`tools/verify_golden_frames.sh`).
  Measured draw time (GALAXY_PERF, Xvfb llvmpipe, i.e. a CPU rasteriser: relative only), mission
  105: off 23 ms, low 24, medium 25, high 68. Desktop OPENGL33 checked under Xvfb (11 missions at
  each level, the editor, the Setup live); the WebGL 2 build checked in headless Chrome (high set in
  the main menu's Setup, every shader compiled, played).
- Main-menu demos (owner request 2026-11-08, Speedy Blupi's `CEvent::Demo*`, blupi_recon
  `src/event.cpp:3712-3825`; BD-0115): the menu's Demo button (under Exit), or 40 s without input
  in the settled main menu of the active window, plays the ten positions in turn
  (`Content/demos/demo001-010.gdemo`: missions 011, 074, 105, 023, 033, 055, 065, 081, 112, 123;
  45-50 s each; BD-0117), the first again after the last. Recorded input drives
  the normal simulation at 60 ticks a second, drawn and heard ("Demo" top left). A new key, mouse
  button, wheel, gamepad button or touch stops it; mouse movement does not; the stopping input
  never reaches the menu (held until released). Focus loss stops it. The player's profile,
  settings, cheats and resume file are frozen during a demo and restored after it. Files carry
  their own header, a simulation revision and their world's hash (`DemoReplay.hpp`); damaged,
  incompatible or stale demos are skipped. Not in Play, Pause, Setup, the editor or menu fades.
  Desktop verified live under Xvfb (button, idle start 40.02 s after the last input, reset by
  activity, every stop input, the cycle and its repeat, the 800x480 window and 1280x720
  fullscreen, 45 s of demo in 44.9 s, sound, focus loss, files untouched, Resume after). Web
  (2026-11-09, headless Chrome over DevTools): the shipped demos replay as recorded, a recording
  made there is kept in `/save/demos/` over a reload and replays as recorded (demos 1-5 in turn, the fifth a galaxy recording made on the web). Android was not run, nor a physical
  gamepad or touch screen. `tools/record_demo.sh`, `tools/verify_demos.sh`, CTest
  `VerifyDemoReplay`/`VerifyDemoFiles`/`VerifyDemoReplays`. Evidence: `build-probe/demo-live/`.
- The player records a demo (BD-0116; how Speedy Blupi did it by a trick:
  `plans/audit/speedy_blupi_demos.md`): Pause's Record button shows the ten demo positions; the one
  chosen restarts the mission and records it ("REC") until Play or the mission is left (or 10
  minutes), into `demos/demo%03d.gdemo` beside savedata.txt; that position then plays it instead
  of its shipped demo ("Original" gives the shipped one back). Campaign and galaxy missions; the
  game is played and saved as always. Live: recorded, replayed equal, played by the menu, restored.
  Shared (2026-11-09): the positions' Export copies a position's demo to `demos/shared/A-demo-N.gdemo`
  (a download on the web), Import copies a shared `.gdemo` (the web's file picker) into a position;
  a galaxy recording finds its world by hash in any player's galaxy, on either platform. Import's
  list pages by seven, a row's bin deletes the shared file (two clicks); the galaxy list the same.
  The main menu's Demos button opens the positions without a mission (export, import, original);
  a second export or a file of a name taken is numbered, never replaced. Live on the
  desktop and the web (a desktop galaxy recording imported on the web and into another player's
  galaxy on the desktop, replayed equal).
- xnjttjpo (BD-0118, Speedy Blupi's XMISSION): typed in play, the 3D editor opens on the world being
  played; saving writes the shipped world file itself (the version before kept in
  `customworlds/campaign-backups/`), Stop returns to the mission from its start. Live verified on a
  copy of the worlds. On the web (2026-11-09) the save is also kept in the browser's storage
  (`/save/customworlds/campaign-edits/`) and laid over the packed worlds at every start: an edit
  outlasts a reload (verified in headless Chrome). Pause then shows "Original" (left of Record) for
  an edited world, on the web and the desktop: "Sure?", then the world as shipped again (kept before
  the first edit: on the web in memory, on the desktop in `customworlds/campaign-originals/`), the
  kept edit dropped, the mission restarted.
- Each player's own galaxy (BD-0119): the menu's Galaxy button plays the selected player's world
  system, a main hub, world hubs 1-14 and their missions 1-8 (and 199), built in the editor's
  galaxy screen (world screen's Galaxy button; `customworlds/gamerN/galaxy/`), with the campaign's
  hub rules and its own saved progress (`galaxyN.*`). Places are deleted from the galaxy screen
  (bin, two clicks, into `galaxy/backups/`) and brought back with Backups. A galaxy mission writes
  the resume file like the campaign; Continue resumes it in the galaxy. Live verified (made, played
  hub to hub to mission, progress kept apart, `--galaxy` autoplay, delete/restore, focus lost ->
  Continue in the galaxy, `GALAXY_RESUME_SELFTEST` equal in mission 11). Shared (2026-11-09): the
  galaxy screen's Export writes the whole galaxy as one file (`customworlds/shared/A-galaxy.galaxy`,
  a download on the web), Import replaces it with a listed one (two clicks; the old places in their
  backups, its progress started over). The menu's player panel shows "Galaxy : N gates" (or the
  galaxy's name: it is named by clicking the galaxy screen's title and typing; the name travels in
  its pack and names its export file). Live on the
  desktop and the web (a desktop pack imported there, exported back byte for byte).
- All four editor Blupi starts participate in G/Tools Pick and the shared cursor/NumPad
  edit target, with red selection outline/model tint. O/Tools Turn rotates each independently
  by 90 degrees; moves, deletion, Undo/Redo and reload preserve its heading. Shared-cell starts
  are disambiguated by the chosen colour; overlapping objects remain selectable in object mode.
  Models are fully opaque with normal depth writes. Heading metadata8 is additive; old worlds
  default to the unchanged direction. Level entry uses the yellow start heading, matching the
  original start-direction rule. All114 tests and native selection/turn/opacity/reload checks pass
  (build-probe/player-start-selection/).
- Editor route cubes1/2 now cover fish, moving mines, birds, wasps, creatures, enemy Blupis
  and every lift type, using registry motion capabilities. Optional patrols show a route when
  endpoints differ; ordinary stationary mines, treasure and chasing hazards do not. Coincident
  endpoints show one cube. Existing double-sided/depth-tested rendering is preserved. Pure
  route tests and native fish/mine placement/save checks pass (build-probe/editor-fixes/).
- Editor Blupi starts are independent: one yellow, orange, blue and green start, each
  drawn in its own palette colour. Moving/removing a colour leaves the others intact, including
  starts sharing one cell; Undo/Redo and save/load preserve each separately. Yellow remains the
  single-player spawn. Non-yellow starts use additive metadata7; existing worlds need no migration.
  This supersedes the rejected initial shared-yellow mapping. Personal mines remain separate.
  All 114 tests and native place/move/delete/Undo/reload/Play checks pass
  (build-probe/player-start-colors/). Neighbouring palette slots remain posts182/signs158/key21.
- Editor placement keeps a single exit: placing a red arrow or final golden key replaces
  the previous exit, as blupi_recon PlaceDecor10 does. Clipboard stamps obey the same rule,
  including removal outside the pasted box. One Undo restores removed/overwritten objects;
  Redo reapplies the relocation. Pure tests and native arrow/Undo/Redo/save checks pass,
  with all 114 CTest tests (build-probe/editor-fixes/).
- Saw correction: only the upper semicircle is drawn, with its flat cut on the cell bottom.
  All three original animation poses are clipped/capped before export; the lower half cannot
  rotate back into view. The lethal volume is the same upper half. Diameter/texture/IDs stay
  unchanged; fallback sprite UVs are cropped too. Native front/below checks, asset validation,
  all114 CTest and all15 saw missions pass (build-probe/saw-half/).
- Debug diagnostics have a fixed-width panel and fixed label/value columns, including three
  separate XYZ/velocity columns and secondary fields. Numeric glyphs use a fixed advance;
  changing digits, modes or device states no longer shifts the panel or neighbouring values.
  Native ground/walking/helicopter and touch captures pass (build-probe/debug-layout/).
- NumPad-selected editor coordinates stay fixed when flying the camera or placing terrain;
  NumPad5/Tools Reset resumes cursor aiming. Selection and Place share one outline when they
  occupy the same cell. Delete clears an extended region with one Undo. Unit coverage and
  normal-render camera/turn/delete/Undo captures pass (build-probe/editor-turn/anchored-shots/).
- A saw is an independent blade in its own voxel: no generated support cube, floor texture
  selection or solid invisible cell. Its original mesh's upper half rests on the cell bottom; disk/body
  contact follows it, stopped blades are harmless and the block/air below is independent.
  New v2 worlds set flag bit0. Runtime upgrades old implicit-floor files in memory to explicit
  floor + blade cells, retaining orientation and switch links. Saving records the new flag;
  occupied cells above are never overwritten. Copy/paste/Undo retain migrated switch links.
  World062 already authors saws at walking21 above floor20: only its header flag changes,
  and regenerated output matches. Native standalone/brick/panel checks,114/114 CTest and all15
  affected mission scripts pass (build-probe/saw-independent/). Scripts use normal simulation
  with a temporary GPU-draw stub; visual checks use normal rendering. The full campaign stops
  at world021's egg expectation at26658 ticks identically in the saved pre-change binary;
  that standalone mission passes. Whole-campaign success is not claimed.
- Editor selection follow-up: red outline/tint marks the edit target, cyan the Place cell.
  Selection coordinates appear in the status. Delete/Backspace, middle button and Tools Remove
  share the same target as O and V, including NumPad coordinates and objects outside object
  palette mode. Undo restores blocks/objects/decor. Shift alone lowers the camera (Z remains);
  Shift+NumPad and device Select 3D do not lower it. Native Delete/Undo/Shift captures and
  all113 tests pass (build-probe/editor-turn/selection-*).
- Editor O/Tools Turn uses the explicit NumPad coordinate when offset, the G-selected object,
  otherwise the preview's object/decor or aimed terrain. A house window (one textured side)
  now honors all four saved turns in both flat and relief rendering. The same target is tinted
  translucent red; converted models use their actual posed mesh with depth testing. Native
  tree/window/NumPad/save captures and113/113 CTest pass (build-probe/editor-turn/).
- Editor V retains short native presses between updates, including several in one event batch.
  Each changes the aimed voxel once and can be undone separately. All multi-variant families
  are covered; a real saved-world before/after check and all113 tests pass (build-probe/editor-v/).
- `debug` toggles the MODERN runtime diagnostics: simulation time/world/region/lives, raw XYZ
  position/cells, actual step velocity in blocks/s, heading, animation/icon, movement modes,
  speed and input devices. Desktop panel is upper-right; touch panel is upper-centre to clear
  Pause/Action. It appears only in Play, is listed by help/cheats and among active cheats, is
  cleared/blocked by fairplay and compiled out by the cheat option. Resume format7 retains it;
  a moving-state reconstruction matches. Parser29 words, OFF verifier,113/113 CTest and live
  toggle/heli/fairplay/help/touch checks pass. Evidence: build-probe/debug-cheat/.
- The large green column/vase models (BigDecor66-69, owner screenshot13-22-15) are solid,
  using the existing collision shapes fitted to their 3D geometry. Registry-driven extraction
  applies in campaign and custom/editor play worlds. The level checker now reads the same registry.
  A before/after live walk stops at the model instead of passing through; world199 still reaches Win.
- Native F9 context recovery (UB-009) is fixed in owner-authorized CNA `ee3235984`: EasyGL
  replays every authored raw/compressed mip level rather than only level0. GPU tests sample
  actual Color/DXT1 mip contents across two context losses, including a partial update; all15
  shader-lifecycle cases pass. Galaxy's distant floor retains textures after F9. No game-side
  mip regeneration/workaround was added. Evidence: build-probe/ub009/.
- Editor/input update (2026-11-08): Ctrl raises the 3D camera, Z lowers it, Space places. Shift
  with NumPad extends a 3D region; Place fills it and Undo restores it in one command. Touch Move,
  Up/Down, world drag and Tools cover editing without a keyboard, including a live 3×2×8 fill.
  Gamepad left/right sticks fly/look, RT/LT change height, A places, X removes, B cancels, D-pad and
  shoulders offset cells, Y switches to a menu cursor, Start saves and Back stops with protection.
  Lifts show original numbered start/end cubes (icons30/31), whose painted faces render from both
  sides with alpha-test depth writes. These same cubes in terrain now show their rear faces.
  Endpoint markers explicitly re-enable depth testing after HUD setup, so opaque terrain hides
  them. An enclosed-marker capture matches the same scene without markers pixel for pixel.
  Editor lifts also preview their XYZ patrol, dwell and return using Play's same movement routine;
  the authored endpoints and marker positions remain unchanged.
- Main menu: **Help** opens F1's panel and [Close] dismisses it by mouse/touch without a click
  reaching buttons underneath. **Fullscreen / Window** always offers the other mode, remembers
  `fullscreen=0/1` in savedata.txt, and starts windowed when no preference exists; F11 also persists.
- `compass` toggles the optional lower-right aid (default off): nearest active chest, then nearest
  red exit arrow/golden key. Three unlabelled rows show integer-rounded straight-line distance,
  a left/right turn relative to Blupi's facing, and up/down height. Correct facing shows `|`;
  the target's same height shows `-` (within3 degrees and0.1 block respectively).
  Touch layouts leave it clear of controls. `help`/`cheats` explain it. Recognized cheat prefixes
  consume letter controls through release (`weelkeys` no longer turns Blupi with its final S);
  slow typing still works. Typed cheats now use CNA Keyboard::KeyDownEXT's ordered non-repeat
  physical presses on every device, including keyboards used with touch screens. No IME/text-input
  mode is needed; autoplay keeps its snapshot path. Fast givecopter is verified while standing in
  the palace hub with180ms frames and no text events. Resume format7 records those letter events.
  CMake/macro `GALAXY_EGGBERT_ENABLE_CHEATS=OFF` disables typed and hidden
  cheats. Both builds were tested; the current local build is **ON**. Resume format7 stores compass and the fairplay lock.
- Vehicle glue fire: **F**, gamepad **RT**, or held touch **Fire**. The tank also accepts Left Ctrl. Tank shots
  follow the tank's continuous heading, including angles between the former eight directions;
  helicopter glue falls downward while airborne/off a lift. Ammo and original timings
  are unchanged. Space/B/X keep enter/leave separate; A/touch Jump and Right Shift still climb the
  helicopter; physical Left Ctrl now also climbs it (owner follow-up), Left Shift descends.
  Live keyboard, virtual gamepad and direct-touch captures show green projectiles.
- Vehicle motor channels stop on leaving Play and when replacing an editor play-test world.
  Continuing Play starts the current vehicle's loop again; main menu and a new on-foot world
  retain no old engine sound. Verified against actual audio-instance states with the dummy driver.
- `fairplay` toggles the runtime cheat-input lock. Enabling clears megablupi, ghost, opendoors,
  showsecret, compass, debug, quick and zoom; only fairplay can then be typed to unlock again. F12 and
  the hidden menu/gesture are blocked too. Previously acquired lives, bombs, ammo, keys, vehicles,
  powers and completed world changes remain. Unlocking permits new cheats without restoring old
  toggles. Normal F1 help remains available. Resume format7 retains the lock.
- Vehicle cheats now replace Ghost instead of leaving GhostStep active under a helicopter's
  camera. A blocked ghost pose returns to the last valid position when leaving that mode. The
  owner later reported givecopter working again; the separate ghost→givecopter failure was
  reproduced and fixed, with an actual climb verified.
- Rare Jeep/palm floor trap: 3D model penetration recovery now tries horizontal escape along both
  X and Z at the same height; it cannot push Blupi down through a solid floor. An already partly
  buried model overlap can recover onto that floor. Controller tests cover foot/Jeep/Tank/helicopter
  head overlap and a half-buried Jeep. The owner's exact rare impact was not reproduced.
- Reusable drink interruption: an enemy glue hit during drinking returns the bottle at its original
  position, without granting unfinished Hide. Normal completion also returns it exactly once;
  the same return handling covers the other reusable action pickups.

- Visible hazard contact (2026-11-08): lava collision/support and lethal contact share the
  Amapi pool's animated height field; being over its voxel is safe. A saw has no implicit support
  cube: walking into or jumping onto its visible disk kills. An actual block below retains its
  own collision independently. The blade's empty mask prevents an invisible cube.
  Fan collision follows the recovered
  motor/base/shaft/hub, and only the swept blade annulus is lethal (all six directions, including
  rotated side fans). Existing invincibility/vehicle gates, death effects and wind-column removal
  remain. Verified by pure contact/shape tests, live safe motor landing and lethal blade landing,
  a lava fall with no premature death, and playthroughs 021/034/051/055/066/084/102/113/114/115/124/199.
- Every exit now finishes the original forty-tick Win cheer before either loading the next world
  or showing Win. The ending, showcase and editor play-tests previously bypassed it. Model clips
  advance on the same simulation clock as action timers. Verified by the 999 exit staying in Play
  through ninety ticks of cheering and then reaching Win, and the full 199 ending playthrough.

- Moving-platform looks (2026-11-08): the 52 authored campaign worlds with ordinary lifts now
  choose their tile looks from the same-numbered original mission's confirmed lift palette (or
  its world's palette when that 2D mission had no lifts). `tools/worldkit.py ORIGINAL_LIFT_ICONS`
  cites the small source data table; 829 lift records have explicit looks, covering 45 tile ids.
  Paths, timing, objects and other world data are unchanged. World021's gate 6 retains a solid
  lintel instead of a jumpable hole; world055's three-cell-high treasure gates now seal their
  whole corridor. The remaining campaign treasure gates were audited; open display exhibits in
  worlds998/999 are intentional. Verified: world metadata comparison, CTest, playthroughs 021/055,
  and a live attempt to jump gate 6 before collecting chests, then pass it after six chests.


- The ten additional retro music previews were cancelled by the owner (2026-11-08): their
  OGG/MIDI files, playlist and dedicated generator were removed. They never had world assignments;
  the original ten runtime tracks are unchanged.

- Controls/menu update (2026-11-08): Play's touch buttons and their mouse hit areas are enabled
  only when CNA reports a touch screen. F1 toggles black controls help on a translucent white panel
  and freezes the current screen; F1/Escape closes it. WASD aliases the arrows (S turns around),
  X/Right Ctrl backs up. The first connected gamepad in XNA slots 1-4 drives play; A also confirms
  Resume, Pause and Win/Lost. The main menu has Exit, and Setup uses the new Galaxy Eggbert logo.
  Resume format v7 records ordered cheat letters, touch capability, compass and the cheat-input lock so reconstruction does not depend on the current device;
  older logs fall back to the checkpoint. Native build and 112/112 CTest pass; live Xvfb screenshots,
  an SDL virtual gamepad and mission 012 with the resume equality self-test verified the changes.
  The opt-in compass HUD aid is described above.
  Typing `help` or `cheats` opens a
  paused overlay listing all supported cheat words and their explanations, including the original's
  deliberately ineffective `killegg`.
- Underwater visibility (2026-11-08): adjacent water cells now render their exterior faces only
  (`WaterSurface.hpp`), removing the stacked translucent internal walls that obscured swimmers,
  fish and routes. The animated exterior and above-water coverage remain. Verified by face-mask
  tests and before/after third-/first-person captures in mission 051.
  Follow-up: the submerged camera now receives mild warm-colour absorption and a blue haze;
  it fades over the last 0.35 block below the surface. HUD/menu colours remain untouched.
  The chest-distance number has its own lower-right position above the HUD/buttons, clear of lives.

- Ghost cheat matches MODERN Mobile Eggbert (BD-0081 / OR-0075, 2026-11-06): skips tile reactions
  and object contacts, uses March/Stop even in mid-air, and draws at 0.75 opacity (model and sprite
  fallback). Existing active followers still home, and existing dynamite blasts remain lethal,
  as in the reference. Verified by 112/112 CTest, unchanged golden frames/trace, and typed-cheat
  lightning/pose probes in `build-probe/ghost/`.
  Ghost/lava depth corrected (BD-0114, 2026-11-08): the recovered opaque lava pools now draw
  with depth writes before models, so a pool behind translucent Blupi no longer covers him;
  foreground lava still hides him. The real-render regression `tools/verify_lava_depth.py`
  fails on the old binary and passes on the fix; 114/114 CTest pass. Three ordinary golden
  captures are identical before/after this fix, but both binaries differ from the stored
  references in this environment; those references were not refreshed. Evidence:
  `build-probe/ghost-lava/`.

- Lit dynamite follows the original 100-tick `table_dynamitef` at 20 Hz (icons 252-255).
  Its three painted spark shapes are camera-facing decals anchored to the 3D fuse tip;
  icon 252 draws no spark (`tools/rd4_to_gltf/dynamite_sparks.py`). Verified by the icon/model
  verifiers, 112/112 CTest, a mission 014 playthrough, live showcase captures in
  `build-probe/dynamite-spark/`, and unchanged golden frames/trace (2026-11-06).
- Mission starts and respawns establish ground support before the first input tick
  (`BlupiController::SettleOnGround`, 2026-11-06): walking starts at the original's 2 px per
  20 Hz tick, while a mid-air spawn still falls. Mission replay uses the same placement.
  `SetPosition` alone retains its previous semantics. The movement verifier covers first-tick
  walking, respawn, falling, footprint/crate support and temporary platforms.
- BD-0077 / Q-015 (2026-11-06): interrupted missions resume from their recorded moment via
  `ResumeLog` (mission start plus deterministic input replay), saved on Play deactivation and
  deleted on clean exit. Format v7 retains the mission path and FNV-1a hash of the exact world
  bytes loaded, touch capability, compass and cheat-input lock (2026-11-08); old versions and changed worlds are refused.
  Desktop build identity includes a lazily
  cached executable hash (Linux verified; Windows/macOS paths implemented); unreadable executables,
  Android and Emscripten retain the compile-time identifier. The existing checkpoint remains the fallback. Native verification:
  112/112 CTest, equality self-tests and passing playthroughs for 012/025/045/051/199, plus desktop
  focus-loss/restart/Continue/clean-close checks. Details and platform limits:
  `plans/audit/behavior_differences.md` BD-0077.
- Playable 3D worlds: terrain, animated tiles, objects, enemies, hazards, pickups, vehicles,
  doors, switches, bridges, teleporters, HUD, sound, menus, save/progress, and hub progression.
- Clay bridge changes now patch the terrain's model-cell list instead of rebuilding the world
  mesh (2026-11-06). All frames 364-372 use models, including gallery exhibits; the crumble no
  longer draws a flat plate underneath. The bounded revision journal falls back to a full rebuild
  for unknown history or edits affecting baked terrain. `VerifyTileFaces` checks incremental/full
  equality; 112/112 CTest and both golden gates pass (image references updated only for the six
  world999 exhibits). Measured rebuilds: world065 211 -> 6 (saw switches), world094 259 -> 0
  (OR-0074). Evidence and frozen comparison scripts: `build-probe/bridge-stutter/`.
- The canonical game implementation lives in `src/GalaxyEggbert/Game/` under
  `GalaxyEggbert::Game`, without legacy `GE` prefixes on game-owned components; the CNA-specific
  source tree now contains only the `GalaxyEggbertGame` application host/platform wiring.
- Block and MoveObject type metadata are centralized in backend-independent
  `BlockDefinitionRegistry` and `ObjectDefinitionRegistry` tables. Rendering, collision, imported
  world support, editor validation, animation, texture-sheet selection, and shared declarative
  MoveObject interaction capabilities consume these definitions instead of maintaining parallel
  icon/type classification lists.
- All approved world-editor milestones and follow-ups through `EDITOR-130` are complete: per-gamer
  custom worlds, Eggbert-ordered palette, raycast editing, object editing, undo/redo, box fill,
  functional touch deletion, verified Inverter/crate/Overcraft entries, menu-based sky-region
  thumbnails, save and play-test loop, unsaved-change protection, correctly sourced BigDecor
  scenery, a visible/Escape return from the editor browser to the main menu, and a backend-neutral
  `GalaxyEggbert::Editor` namespace/source tree without legacy `GE` prefixes on editor components.
- Linux native uses CNA's current OpenGL-family renderer identities. Windows packaging support
  stages SDL3 and MinGW thread runtime DLLs while statically linking GCC/C++, but the current
  `SDL_GPU` path still needs a fresh build and live 3D verification. The Web/Emscripten build uses
  `WEBGL2`; it is manually verified rather than part of CI or a release pipeline.
- Linux provides a reproducible `GalaxyEggbert-linux-x86_64.tar.gz` runtime bundle through
  CMake's `package` target. It contains source-tracked assets and bundled SDL libraries; actual
  desktop launch remains a host-environment check, not a CI/release gate.
- GitHub Actions CI builds and tests the CNA target from a clean sibling-checkout layout. It excludes
  one known, unrelated `easy-gl` test failure.

## Verification baseline

The current source and CI use published CNA `594eb28dc` (optional KeyDownEXT), including
`ee3235984` (UB-009 mip recovery). The native local checks used that exact checkout.

As of the date above, rebuilding, testing, and live-running the current source succeeds for the
verified native and WebGL 2 configurations:

| Build | Result |
|---|---|
| `build-cna-opengl3` (`OPENGL33`) | Build succeeds; 93/93 CTest cases pass with the then-quarantined `easy-gl-resource-smoke-tests` excluded; normal startup and the three-frame golden capture run successfully under Xvfb (2026-10-17, before cna-engine; on 2026-10-20 an `OPENGL33` build on cna-engine ran the menus and the editor under Xvfb) |
| `OPENGLES3` on cna-engine (2026-10-20) | 93/93 CTest cases pass; golden frames, behavioral trace and startup screenshots byte-identical to the easy-3d build under Xvfb and on the GPU (`NEXT.md` §3, "Moved from easy-3d to cna-engine") |
| `build-cna-webgl2` (`WEBGL2`) | Emscripten Release build of 2026-11-02 late night (`GalaxyEggbert.{html,js,wasm,data}`, the data pack with the gallery world998, the drip, switch, conveyor and glue models, the re-exported clay bridge, the teleporters' four-sided plates and the Wave E worlds of that night); headless Chrome under Xvfb (SwiftShader WebGL 2, driven over DevTools) loads `world001`, the object models and 93/93 sound channels and draws the gamer-select menu over the title scene with no exception in the console (CNA + cna-engine of that day) |

The deterministic golden-frame and behavioral-trace harnesses are manual `xvfb-run` checks, not
default CTest cases. Full commands and sibling-repository pins live in `handoff.md`.

## Known limitations

- Controls since 2026-10-25: third person is the default view, F3 toggles first person (was C),
  turning is 135°/s, and Blupi no longer walks up one-block steps — he jumps, as in
  `Decor::TestPath`. Menus fill the bars beside their 640×480 images and text uses the
  original proportional glyph widths.
- Controls added 2026-11-02 and extended 2026-11-08 (owner's requests): X or Right Ctrl backs Blupi up (on foot or in a
  vehicle; Down still turns him round); Left Ctrl fires in the tank and climbs the helicopter; Right Shift also climbs
  (gamepad A and touch Jump also climb); a gamepad (`GamepadInput.hpp`): left stick/D-pad walk and turn, A jump (and Enter:
  Play in the main menu), B/X action, RB back up, LB crouch, Y look up, Start pause; touch
  screens: the on-screen pad takes several fingers at once (`InputPad::UpdatePlayTouches`: stick
  and Jump together) and has a back-up button over the stick. Not tried on real hardware yet
  (no physical gamepad or touch screen here); an SDL virtual gamepad was verified on 2026-11-08.
- Third person shows the real Blupi converted from the original Ray Dream scenes
  (`avatars3d/blupi/`, `tools/rd4_to_gltf/`) with a clip, expression, props and vehicle per
  animation state, and parked vehicles as 3D models (re-exported 2026-10-24: the overcraft now
  matches its sprites at 0.94-0.98). The game's objects are 3D models converted the same way (`avatars3d/objects/`,
  `plan.md` Phase 7B): pickups (treasure, egg, exit, keys, secret exit, drink, ammo, dynamite,
  invert sign, lollipop), enemies (mines, moving and hanging mines, bird, fish, wasp, bulldozer,
  creature, follow star, decoys, enemy projectile), the tentacle and the big decor (palms,
  trees, grass, houses, machines, toys, pillars, plant, column, fountain, statue; checked live
  2026-10-24). Effects, the charge and the shield (sparks) stay billboards; procedural
  textures (bark, metal noise, wood grain) are flat colours. First person still uses an invisible collision
  point. The fox placeholder remains as a fallback when `avatars3d/blupi/` is absent.
- 3D effects: Ultra's first frame makes the finer atlas and its maps (~2 s); Ultra not run on
  Android or the web. The Setup's frame rate above 60 draws the camera (and its shake), Blupi, every
  moving object (projectiles too) and the particles between the 60 steps a second
  (`GALAXY_INTERP_TRACE` shows it per frame). The sun's shadow reaches 160 blocks ahead of the camera (the far
  cascade drawn every third frame); the procedural materials are Ray Dream's colours and parameters over a value noise,
  not its own noise; skinned models above 56 bones (none today) would be drawn by the stock effect. The shaders, the shadow map and the glow need GLSL source (`ShaderEffect`) and
  render targets: OPENGL33 and WEBGL2 checked, Android (OPENGLES3) not run; where they do not
  compile they are left out by themselves (no ambient occlusion either) and the fog, soft shadows
  and particles remain.
- The current Windows `SDL_GPU` path has not yet had a fresh cross-build and live 3D run; do not
  call Windows supported until both pass.
- Editor text is deliberately limited to functional readouts/notices (XYZ coordinates, background
  ids, and the temporary not-implemented message); the main tool palette remains icon-driven.
- Intermittent window-focus/input loss has occurred under Xvfb/live verification. It has not been
  reproduced as a Galaxy Eggbert code defect.

## Open work, in priority classes

_Brought in line with the release audit (`plans/audit/release_audit.md`, 2026-11-06) and the
fidelity audit's waves (`plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md` §3) on 2026-11-09. The fidelity
audit (milestone G, waves A-F) is done and every ledger is settled (0 open rows); the completion
plan's milestones A-F are done (`plans/GALAXY_EGGBERT_COMPLETION.md` §13). The owner's later
requests (main-menu demos, recordings, xnjttjpo, the players' galaxies, sharing them, BD-0115 to
BD-0119) are done too. What is left:_

### Waiting on the owner

- **Publishing release 1.0.0-rc1** (Q-018): the Linux archive and the Android APK are built
  (`tools/release/make_release.sh`, `build-probe/release/`); open are the Windows package, an
  archive for glibc 2.35, the version number and where to publish (the web build too?).
- **Multiplayer**: analysed in `plans/plan_gemultiplayer.md` (nothing built); its questions Q1-Q11
  are the owner's.
- Optional: a human play pass (MR-0004 lists what to try); the owner judged the automated
  playthroughs sufficient (MR-0007), so it does not hold anything up.

### Outside the code's reach here

- Android: played by the owner on his phone (2026-11-10, 1.0.0-rc1): everything worked but the
  accelerometer, fixed the same evening (geultra, 86017658); his word on the fix is still to come.
- A physical gamepad on the desktop and a desktop touch screen: verified with an SDL virtual
  gamepad only (the phone's touch screen works).
- Windows: the `SDL_GPU` path has had no fresh cross-build or live 3D run; not supported until both
  pass.

### Decided, kept as they are

- The 56 `.max` sources are not converted (MR-0001, closed by the owner as MR-0010): the shields
  and the charge stay the original's sprites, Blupi comes from the Ray Dream scenes.
- Procedural textures (bark, metal noise, wood grain) stay flat colours: a bake measured worse
  (`plans/GALAXY_EGGBERT_COMPLETION.md` §7.3); the wind tiles stay 2D effects (no 3D scene exists).
- BB3D-032, the fish's head: cosmetic, every lead tested (`plan.md`).

### Ready for an explicitly chosen engineering session

- None. `INFRA-006`'s final ObjectType audit is complete: pure type membership and parameter rules
  are registry-backed, while the retained branches are distinct state machines or explicitly scoped
  cheat commands. Choose a new feature or content task before changing gameplay architecture.

## Non-negotiable boundaries

- Never modify `../mobile-eggbert`; treat it as read-only reference and test data.
- Do not transcribe its code, tables, enum values, or byte formats without explicit user approval.
- Do not invent gameplay mechanics; confirm every gameplay feature against the reference first.
- Keep `InteractionSystem` independent of `BlupiController`, camera, and graphics. Use its
  established event/request boundary for cross-system behavior.
- Keep the engine-agnostic `include/GalaxyEggbert/` world/data model free of CNA/cna-engine dependencies.

## Document ownership

| Document | Use it for |
|---|---|
| `CURRENT.md` | Current status, tested baseline, active limitations, and actionable work |
| `handoff.md` | Where work stands, what to run next, commands, sibling pins, gotchas; updated after every task |
| `NEXT.md` | Obsolete since 2026-10-24: frozen chronological write-ups and architecture notes up to then |
| `plan.md` | Historical implementation log, research evidence, and the complete task inventory |
| `README.md` | Project introduction and onboarding; it links here for current status |

When an entry here conflicts with old prose elsewhere, treat this file as the status source and
correct the stale document in the same change when practical.
