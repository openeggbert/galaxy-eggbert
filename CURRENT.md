# Galaxy Eggbert — Current Truth

_Last verified: 2026-11-08 (native build, 113/113 CTest, golden frames and trace, controls/HUD/water
checks and mission 051 with the resume self-test). The all-78-playthrough baseline remains
2026-11-06 (play pass 33: 77 replays equal). The Web/Emscripten build was verified
2026-11-06 in headless Chrome; the whole-campaign run
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
- Blupi's 3D model (from the Ray Dream originals) has clips for every live table_blupi action,
  including Win, Turn, Vertigo, Recede, the reactions (JumpAie, Ouf*) and the poses between moves.

## Shipped, working state

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
  slow typing still works. On non-touch Play, ordered CNA text events also retain short presses
  between frames: rapid givecopter now works in the owner's second editor world. Touch/autoplay
  keep the snapshot path. All28 words and fairplay are covered; a live text-input resume matches.
  CMake/macro `GALAXY_EGGBERT_ENABLE_CHEATS=OFF` disables typed and hidden
  cheats. Both builds were tested; the current local build is **ON**. Resume format6 stores compass and the fairplay lock.
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
  showsecret, compass, quick and zoom; only fairplay can then be typed to unlock again. F12 and
  the hidden menu/gesture are blocked too. Previously acquired lives, bombs, ammo, keys, vehicles,
  powers and completed world changes remain. Unlocking permits new cheats without restoring old
  toggles. Normal F1 help remains available. Resume format6 retains the lock.
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
  Amapi pool's animated height field; being over its voxel is safe. A saw's supporting cube is
  harmless; walking into or jumping onto its exposed disk kills, including level-height contact.
  Its rendered support cube is solid horizontally as well as vertically, including stopped blades;
  the original empty 2D collision mask no longer allows walking through that cube.
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
  Resume format v6 records ordered cheat letters, touch capability, compass and the cheat-input lock so reconstruction does not depend on the current device;
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
  deleted on clean exit. Format v6 retains the mission path and FNV-1a hash of the exact world
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

- Native F9 context recovery loses terrain mip contents above level0 (UB-009): close texture
  detail returns, but minified/distant floor faces become black. Reproduced and traced to CNA
  EasyGL's recovery allocation without higher-level upload. Owner requested analysis only;
  no recovery/rendering workaround or upstream fix was applied. Evidence: build-probe/f9-analysis/.
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
- The current Windows `SDL_GPU` path has not yet had a fresh cross-build and live 3D run; do not
  call Windows supported until both pass.
- Editor text is deliberately limited to functional readouts/notices (XYZ coordinates, background
  ids, and the temporary not-implemented message); the main tool palette remains icon-driven.
- Intermittent window-focus/input loss has occurred under Xvfb/live verification. It has not been
  reproduced as a Galaxy Eggbert code defect.

## Open work, in priority classes

### Active and explicitly approved

- **The fidelity audit (milestone G, since 2026-11-01)**: `plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md`
  and its ledgers in `plans/audit/`. Fixed so far: crate push/pull (D60), lifts over the
  original's speeds (BD-0006: 31 lifts; 061's shuttle ladder), crate support (BD-0003). Open: the
  editor's variant cycling (ED-0001/0006), follow stars and shots vs decor (BD-0004), lava as a pool
  from the original Amapi sources, fans and pillars 76/77 as their original 3D scenes (all 2026-11-01;
  110/110, all 78 playthroughs, the whole campaign 761140 ticks). Open: pillars 375-377/399, the wind
  tiles, the 56 `.max` sources (BLOCKED, MR-0001), a human play pass, Waves C and E.
- The completion campaign (`plans/GALAXY_EGGBERT_COMPLETION.md`): owner-reported defects D1-D12
  (§5), then milestones B-F (representative mission, model/animation parity, hubs and missions,
  editor slots and title screen, stabilisation).

### Needs user direction or visual judgement

- `AscenseurVertigo` platform geometry, `ThinMechanical` geometry, water treatment, architectural
  kit assembly, and residual seam-line transparency.
- The real Blupi model/rig and its target presentation.
- `TILE-005`: author the remaining full 3D worlds. This is valid but sizeable content work.

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
