# Galaxy Eggbert — completion plan

_Started 2026-10-25 on the restored `develop` baseline (`00ba088`). This is the active execution
plan; keep it truthful and update it with every milestone. `CURRENT.md` holds the verified
state, `handoff.md` where work stands, `plan.md` the historical log and the detailed task
inventory (its Phase 7B `BB3D-*` items stay the per-object record)._

Status legend: **done** (in the tree, tested, checked live when visual) · **partial** ·
**open** · **blocked** (named blocker) · **not convertible** (proven).

## 1. Direction (owner, 2026-10-25)

The owner reverted a misguided campaign (a 2.5D side-view conversion of the 2D maps, then a
rewrite toward camera-relative controls; branches `preserve/2.5d-side-view-2026-10-25` and
`true-3d-correction`, kept only as notes, never merged). The brief that replaced it:

- **Finish the existing game.** Galaxy Eggbert is a genuinely 3D, tile/voxel-based remake on
  CNA + cna-engine. Keep its voxel world, `.vwr` format lineage, 3D editor, yaw ("tank")
  controls, camera, physics, registries and save data. Fix a system only for a specific
  reproduced defect, with a focused change and a regression test.
- **Worlds are original 3D tile designs.** Mobile Eggbert's 78 worlds give each world's
  identity, theme, mechanics, difficulty and progression — never its 2D geometry. No extrusion,
  no lane lock, no row-to-row mapping.
- **Gameplay follows Mobile Eggbert's C++ code** (`../mobile-eggbert`, read only), adapted to
  3D only where a 2D rule cannot apply, as a small documented decision.
- **Models and animations come from the original sources** (`../speedyblupi-data`, Ray Dream
  `.rd4`/`.rds` first), see §2 and §7.
- **Repository boundaries.** Writable: this repository; `cna-engine` only for a demonstrated
  defect or a broadly reusable helper, separately tested and committed. Read only: `cna`
  (defects go to `upstream_bugs.md`, `UB-NNN`), `mobile-eggbert`, `blupi_recon`,
  `free-eggbert`, `speedyblupi-data`. Push only when asked.
- **Data tables.** Since 2026-10-25 the owner's standing approval covers porting small
  Mobile Eggbert data tables needed for fidelity (glyph widths, animation and timing tables),
  citing the source; larger code still needs asking.

## 2. Creative vision and Original Art Fidelity (permanent requirement)

_Imagine Daniel Roux had made Speedy Blupi as a fully 3D game in 1998._ Galaxy Eggbert should
feel like that hypothetical game, completed with the existing Galaxy technology — not a modern
reinterpretation. A longtime player should recognise Blupi, the vehicles, objects, enemies and
mechanisms at once and feel the world has come alive in three dimensions.

**Worlds versus models.** Worlds are creatively re-designed as 3D tile levels. Object designs
stay faithful to the originals: original geometry, silhouettes, proportions, colours, textures,
expressions, animation poses, the low-polygon style. Higher polygon counts, PBR materials or
extra detail are not fidelity.

**Asset policy (source first).** For every game object: (1) find its original 3D source in
`speedyblupi-data`; (2) check for an existing conversion; (3) keep a faithful one; (4) fix the
converter when a conversion is missing or defective; (5) convert to the runtime format (CNA
`SkinnedModelEXT`, with `.glb` as the intermediate); (6) preserve geometry, proportions,
materials, UVs, axes, scale, animation; (7) render and compare against the sprites, the
original game and the gameplay video — a successful conversion is not a visual check; (8)
integrate, replacing billboards only after verification. Where no source can be recovered,
build a modest reconstruction from sprites/footage, never an extravagant replacement. Effects
(explosions, splashes, sparks, glows) may stay billboards (decision of 2026-10-24).

**Validation criteria for a model:** silhouette IoU against its sprites (`build_object_library`
reports, `compare.png`); consistent outward winding (no face lost to back-face culling — check
added 2026-10-25, §7.2); correct scale against Blupi and a 64-pixel block; checked live from
several angles in the running game; animation frames against the sprite tables. For Blupi's
clips: every frame against its icon (`clips.md`), no hidden parts in a normal pose, and a live
check of the state.

## 3. Protected baseline (do not regress)

- `GalaxyEggbertCNA` on CNA + cna-engine; no other engine or renderer.
- Voxel world: 100³ cells in 10³ chunks, block = 12-bit type + 4-bit metadata, numeric block
  ids (object-m.png icons), `BlockDefinitionRegistry`, `ObjectDefinitionRegistry`,
  `InteractionSystem` free of controller/camera/graphics.
- `.vwr` v2 (`VWR1` magic, header v2, chunk payload `VCH1` v2, sparse `BMD1` records:
  MoveObject type 1, plate rotation type 2, BigDecor type 3). See §6.
- 3D editor (raycast editing, palette in Eggbert order, undo/redo, box fill, object editing,
  sky region, save/play-test loop) and per-gamer custom worlds.
- Controls: yaw turning with Left/Right, Up/Down forward/back, Ctrl jump, Space action,
  Shift crouch/look-up; first/third-person views (third person default, F3 since 2026-10-25).
- Real Blupi model (`avatars3d/blupi/`, 45 clips, 5 vehicles) and 82 object models
  (`avatars3d/objects/`), converter `tools/rd4_to_gltf/`.
- Sound channels, ObjectType ids, save file `savedata.txt` (3 gamers), menus (Wait, Init,
  Setup, Pause, Resume, Win/Lost), hub progression and door unlocks.

## 4. Audit of the restored baseline (2026-10-25)

| Area | Found |
|---|---|
| Repository | `develop` = `origin/develop` = `00ba088`, clean. Native build `build-cna-opengl3/` (OPENGL33, Release, Ninja, ccache). Siblings: cna `e4e721e69` (live tree, another session edits it), cna-engine `f68e7c3`, easy-gl `f4d6afa`, meta-gl `dd3b300`, sharp-runtime `4e146b90`. |
| Build/tests | Builds; **96/96** CTest pass. Golden frames stale since Phase 7B (known). |
| Live run | Wait → Init (copied Mobile Eggbert `init.png`, an X-icon editor button) → hub → missions works under Xvfb. Default view was first person. |
| Worlds | 79 `.vwr`: `world001` (flat 18×18 plaza, 12 world portals + demo portal), 12 theme hubs (flat plazas with numbered portals and gated doors, 373-437 blocks, ≤3 heights), **65 placeholders** (every mission and 199: an identical 11×11 floor with one exit, generated by `tools/GenerateSampleWorld3D.cpp`), `world999` (the mechanics showcase, 6 843 blocks, 14 heights, 100 objects). Inventory: `plans/inventory/worlds.md` (`tools/world_inventory.py`). |
| Format | `.vwr` v2 as documented in `World Format.md`; v1 worlds no longer load (deliberate 2026-07-09 cutover). |
| Assets | Blupi: 58 bones, 45 clips, 5 vehicles; Bye had no clip. Objects: 82 models drawing 256 icons; effects, charge, shield remain billboards. 27 of 83 converted models had inward-facing caps (§7.2). |
| Editor | Browser lists `customworlds/gamerN/custom_NNN.vwr` (up to 999), "New World", play-test loop; entered from the main menu's X icon. |
| Saves | `savedata.txt` and `customworlds/` relative to the working directory. **The owner plays from `build-cna-opengl3/`**: its files there are real user data (they include `slot01/02.vwr` written by the reverted build). Live checks run from `build-probe/qa-run/` (`tools/qa_live.sh`). |
| References | `../mobile-eggbert` (C++ port; `worlds/` 78 `.txt`), runnable original `~/Downloads/SpeedyBlupi2013Linux`, gameplay video (§9.3), `speedyblupi-data/originals` (528 Ray Dream scenes, 44 `.max`, 6 `.a3d`). |

## 5. Defects reported by the owner (2026-10-25)

| # | Report | Cause found | Status |
|---|---|---|---|
| D1 | Standing Blupi shows only his right half | `Stop` (and Mockery, Mockeryi, Switch, Teleporting, Charge) took poses from `blupi-scie.rds`, the saw death's half Blupi | **done**: tools `d660085`, Blupi rebuilt `78757b1` |
| D2 | Exit arrow has transparent/missing sides | free-form end caps wound inwards, culled by the game (27 models) | **done** `94ce688`, objects re-exported `b1b0669` |
| D3 | Hole on top of walking Blupi's head | same as D2: his body's pole caps | **done** `78757b1` |
| D4 | Blupi walks up a one-block step | engine-invented step-up (`kStepLimit` 1 block); `Decor::TestPath` has none | **done** `c317f94` |
| D5 | Third person should be default; C is a cheat letter | — | **done** `aabc4f8` (F3 toggles) |
| D6 | Turning a bit slower | — | **done** `c2cb216` (135°/s) |
| D7 | 3D world visible beside Pause/Setup | 640×480 images drawn centred only; original stretches a corner patch | **done** `8ba5e34` |
| D8 | Game aborted after Play ("Unknown animation clip") | regression of D5: model drawn before a clip was chosen | **done** `8260eaf` |
| D9 | Gaps between letters in menu text | fixed 17 px advance instead of `Text::table_width` | **done** `21cccb3` |
| D10 | Bye freezes on its first frame at a world portal, leaves too early | no Bye icon table/clip, so Stop's first frame showed | **done**: icons `4572099`, clip `78757b1` |
| D11 | Many exhibition models in world999 look wrong compared with Mobile Eggbert | partly D2; the rest needs per-object comparison | open (§7.3) |
| D12 | "Player A: Erase progress" runs off the Setup screen | one line where the original has two | **done** `9d88278` |
| D13 | Blupi on the skateboard loses a life on a mine; in Mobile Eggbert the skateboard destroys the mine ("just an example" of vehicle/enemy interactions to audit) | `Decor.cpp:5782-5824`: hazards 2/3/4/16/17/20/96/97 always destroyed, Blupi dies unless in jeep, tank or on the skateboard | **done** `149ec32` |

Found while building mission 011 (live, 2026-10-25):

| # | Defect | Cause | Status |
|---|---|---|---|
| D14 | Small machine parts (icons 53-60, 63/64) show as black cards | cross-billboards (87-94 % transparent) drawn in the opaque terrain pass | **done** `70be115` |
| D15 | Every platform lift is a black box | lift = one cube of icon 29, a bar over a stem with its lower 44 % transparent | **done** `7f02216` (bar + stem, `LiftVisual.hpp`) |
| D16 | Walls, steps and edges merge into one flat colour | terrain drawn unlit: all faces equally bright | **done** `eeb9624` (one directional light + ambient, `TerrainLighting.hpp`) |
| D17 | Partly transparent machine cubes (15-31) would show black holes in 3D | their 2D tiles let the backdrop through | **done** `2b7c60d` (cut out like the other sprite tiles, D31); walls still built of the opaque 1-14 (`tools/worlds/kit.py`) |
| D18 | Chase camera pulled tight behind Blupi next to walls loses the view | wall collision brings the eye to Blupi's back | **done** `fe7d351` (the camera rises over what blocks it, ChaseCameraElevation); levels still keep open sides |
| D19 | Hubs: world pictures as mission signs, sign icons as doors, worlds 9-12 shown as copies of 1-4, no door slide, no gold bars, no golden-key ending | Galaxy's 2026-07-17 scheme | **done** `2f57bbc` (original's signs/doors/arrivals, `HubDoors.hpp`) |
| D20 | Opened doors and collapsed bridges stay drawn | terrain mesh never rebuilt in play | **done** `2f57bbc` (`World::blockRevision()`) |
| D21 | Crates 2D-era: X-only push, floor test at y 0, Blupi walks through them, no pull | — | **done** `1598ebc`, `fd67b3f`, clips `d098a0b` (solid, X/Z push with support, StopPop/Pop pull) |
| D22 | The game crashes when Blupi dives in deep water | camera raymarch counted water as a wall, eye on target, LookAt threw | **done** `f42d58a` |
| D23 | The dynamite blast misses what is in front of or behind the stick | 2D blast, half a cell in Z | **done** `d6e9bce` |
| D24 | A fan kills only when right above Blupi's head, whatever way it blows; its wind stays | simplified IsVentillo | **done** `39d3f16` (contact at the mouth, wind column cleared) |
| D25 | A creature patrolling along Z shoots sideways | shots along X only | **done** `393a2a3` |
| D26 | Enemies patrolling along Z move sideways | sprite yaw faces ±X | **done** `1e139d2` |
| D27 | A follower at another depth never reaches Blupi | homing in X/Y only | **done** `4f0f062` |
| D28 | Walking along Z leaves no Shield/Power trail | distance in X/Y only | **done** `3cb27fa` |
| D29 | A win screen after every mission; no Win cheer, no Turn | — | **done** `018c0a6`, `65d2f05` (cheer then straight on; the screen only after 199; clips with the next model rebuild) |
| D30 | After Lost Blupi restarts at (0, 1, 0), not the world's spawn | — | **done** `018c0a6` |
| D31 | Sprite tiles drawn as opaque cubes/plates: black where the sprite is clear (doors 334-336, treasure doors 421+, pillars 399/76/375, girder 201, bars 51/138, balustrade 398, plate decor, cobwebs) | — | **done** (TileCoverage: alpha-test cutout pass, faint tiles blended; stone walls under a quarter clear stay opaque) |
| D32 | Blupi cannot leave deep water onto a bank (bobs at the surface, sinks back) | full gravity while Surf, a kick while Nage | **done** `762d254` (swimming as Decor.cpp: no gravity, float band, dive, drift up, the weak jump out) |
| D33 | The tank's own glue shot killed Blupi | contact is a sphere round him; the shot left from his position (the original starts it 35 px ahead) | **done** `85bc1ae` (his own shots never hit him) |
| D34 | After a teleport Blupi faces the partner pillar (a step takes him back) and the camera eases over from the departure | arrival kept his yaw; the smoothed camera was never reset (also on a world load) | **done** `fe7d351` |
| D35 | Flat-colour faces of DirectionalCube tiles show stripes; the facades 186-192 dark or red edges | an 8 x 8-texel swatch stretched over a face; facades sampled at their window | **done** `db0816f` |
| D36 | The horizontal fans 126/129 show their base on the side they blow toward | base and open face swapped in the face table | **done** `6101598` |
| D37 | Vehicles pass over and through water (013's "it dislikes water!") | water detection skipped while riding | **done** `557a694` (wrecked as Decor.cpp:5415-5435) |

## 6. Native world format: evolution policy

Current: `.vwr` v2 (§3). The format is not frozen, but grows only for a demonstrated need.

Rules: additive, versioned, deterministic; new readers load every committed world and every
user world; old executables may refuse newer files only if documented; no silent
reinterpretation of bytes; malformed input is rejected; fixtures and round-trip tests for
legacy load, upgraded load, load→save→load equality, editor edit→save→play→reopen, unknown
optional records, multi-chunk boundaries, and every profile's slots. Backups of user worlds
before any migration; migrations opt-in.

Candidates (investigate when a world or the editor needs them; none implemented yet):

| Need | Today | Possible extension |
|---|---|---|
| Portal destinations beyond "marker icon → mission arithmetic" | hub markers encode targets implicitly | optional per-marker metadata record (target mission, arrival cell) |
| Arrival point next to the door last used (video: Blupi arrives beside the door he opened) | single spawn | per-door arrival cells (sparse record) |
| Music per world | Mobile Eggbert header `music=`; Galaxy has none | the v2 header has no free field (all 40 bytes used): an optional world-metadata record |
| Object orientation / yaw for 3D placement | MoveObject has none | optional yaw in a new MoveObject payload size (decoder already accepts 45/47 bytes) |
| Larger or non-cubic worlds | 100³ fixed | `chunksX/Y/Z` header v3 (`World Format.md` "Recommended Future World Header") |

## 7. Original Art Fidelity and Asset Conversion

### 7.1 Inventory

- Source survey: `build-probe/rd4-survey/scenes.csv` (all 528 Ray Dream scenes read; masters not
  built: `tx` 3D text (menus), `COFA` stars, `fogg` fog). `.max` (44: menus, Blupi variants,
  shields) and `.a3d` (6: creatures, drill) are not readable without proprietary tools; the
  creatures also exist as `.rds`. **To produce:** `plans/inventory/assets.md`, one row per source
  (path, format, use, convert status, `.glb`, runtime directory, game object, checks, limits).
- In the game: Blupi (`avatars3d/blupi/`), 5 vehicles, 82 object models (pickups, enemies,
  decoys, tentacle, big decor). Billboards kept by decision: effects, charge (31), shield (25).

### 7.2 Corrections in progress (2026-10-25)

- Inward caps on free-forms (D2/D3), now fixed in the converter; consistency check:
  every welded edge must be traversed once in each direction (to be added to the converters'
  own checks). All 83 models are being re-exported and re-checked.
- Half-body poses (D1): `--whole-icons` rule; Bye clip (D10): end-key frames, `kByeFrames`.
  Blupi rebuilt from the full re-match and replaced (`78757b1`), checked offline and live.

### 7.3 Open items

- Per-object live comparison with Mobile Eggbert (video/original game) for every exhibition
  object of `world999` (D11), recorded per object: source, IoU, live view, verdict. 2026-10-26:
  the silhouette match against each object's own sprites (`avatars3d/objects/*/icons.txt`, the
  library's IoU per icon) is 0.88-1.00 for 76 of the 82 models. The weakest, to look at first:
  the bird (min 0.60, mean 0.88: some wing frames), the plants 3a-c (0.65-0.69), the follow
  star (min 0.77), the plants 1a-d (0.83-0.86), the fish (min 0.85, BB3D-032) and the drink
  (0.88). Silhouettes do not measure colour: the flat procedural textures remain. The live
  exhibition (rows z 27-54, x 78-96; stand at x 76.5) shows no model gross enough to name
  without the owner's list.
- `BB3D-032` fish head ~15 % large; `BB3D-050` grass blades wide; procedural textures flat.
- Push/Pull: done (D21: push pose while walking into a crate, pull by crouching + Down,
  Recede/Advance clips; checked live in 012).
- Win cheer: done (D29, Win clip `3415d3f`).
- Saw death's half body (`Corps et Cube`, a CSG prop in Clear4 only) still has 119 + 76
  inconsistently wound triangles after the cap fix: a second converter case (CSG), to find.

## 8. Blupi animation coverage

Original actions (`BlupiAction.hpp`, 88) vs Galaxy (`BlupiController::AnimState`, clips in
`avatars3d/blupi/clips.txt`). Built and wired: Stop, March, Jump, Air, Down, Up, Push, the
squashed pair, Balloon, Teleporte, Hide, Switch, the vehicles (helicopter, jeep, tank incl.
fire, skateboard incl. take/put, overcraft), swimming and surface, Mockery ×3, dynamite
take/put, deaths Clear1/3/4, Glu, Drown, pickups Sucette/Drink/Charge, Bye, Win (13), Turn
(3), Vertigo (8), Recede/Advance (9/10, pull), Pop/StopPop (28/29) (2026-10-25/26).
HelicoGlu (58, the helicopter's glue drop, `85bc1ae`), JumpAie (36) and Ouf1a/1b/3/4 (44, 45,
47, 48, `ec84d42`; clips with the next library rebuild). Checked against `Decor.cpp` and not
added, being dead code there: the Suspend family (31-34; `m_blupiSuspend` is never set), Ouf5
(65, its only cause is Suspend), Electro (57) and Clear5-8 (78-81; never assigned), Set (12;
never referenced). The vehicle turns are the host turning the 3D model for the length of
each Turn action (`TurnAroundSeconds`). Ouf2 (46, the decoy, `99f215a`) and the
StopMarch/StopJump poses (60/61, `6ffded6`, clips `729063e`) are in too. StopJumph (62) is never
set. **Open:** SlowdownSkate (85).

## 9. Campaign content

### 9.1 Structure

78 identities: `world001` main hub, 12 theme hubs (`world010`…`world120`), 64 missions
(4, 5, 4, 6, 8, 6, 5, 4, 5, 7, 5, 5 per world) and the bonus `world199`; plus `world999`, the
Galaxy showcase (kept; seeds editor slot 1, §10). Table: `plans/inventory/worlds.md`.

### 9.2 Authoring process

Per world, before building: a short **3D design brief** (identity, signature mechanics and
objects from the reference and the video, teaching goal, 3D layout idea using several X/Z
directions and real heights, route to the exit, secrets), stored under `plans/briefs/`.
Build with deliberate authoring tooling inside Galaxy (reusable architecture kits allowed;
cloned terrain with renamed spawns not allowed), then: loads, edits, saves; critical objects
present; a reachable route to the exit (scripted reachability check); no softlock; portals
and progression work; live screenshots. Order: one representative mission as the quality
reference → main hub and one theme hub with its missions → the rest in batches.

### 9.3 Gameplay video

`/rv/big/my-blupi-videos-on-youtube/Speedy_Blupi_for_Windows_Phone/20241215 Speedy_Blupi_for_Windows_Phone_gameplay_all_levels.mp4`
(806×516, 20 fps, 6 h 54 min). `plans/evidence/video-timeline.csv` is a per-mission timeline
recovered from the reverted branch; spot-checked 2026-10-25 against frames extracted here (win
of 011 at 1:43.9, hub at 1:46.5, Bye at sign 2 at ~1:48, level 012 at 1:49.8: all match).
Use it as an index; verify each claim against extracted frames before relying on it.

## 10. Editor and UI

- **10 slots × 3 gamers** (done `7eac7d5`): slot N = `customworlds/gamerG/custom_00N.vwr` for N = 1-10
  (existing files keep their slot). Files beyond 10 (and the reverted build's `slotNN.vwr`) stay
  untouched and are listed as "older worlds" to open or copy into a free slot — never deleted
  or hidden. Slot 1 of a gamer is seeded once from `worlds3d/world999.vwr` only when empty
  (copy, never a link). Create/edit/save/play-test/delete with confirmation; tests per gamer.
- **Title screen** (done `6917fc9`, `worlds3d/title.vwr`): a Galaxy Eggbert scene (CNA-rendered or composed from Galaxy's own
  assets), not Mobile Eggbert's `init.png`; legible original UI.
- **Editor entry** (done `e3a1042`): a labelled "3D World Editor" button instead of the X icon.
- **Transparency** (done `063b272`): the panels' alpha was applied twice (BasicEffect premultiplies, the overlays blend straight alpha); now as the original's light veil (video 0:10).
- Done 2026-10-25: proportional text (D9), full-screen backgrounds (D7).

## 11. Mechanic parity (to fill per mechanic)

Columns: mechanic · reference (`Decor.cpp` location) · Galaxy location · difference · class
(A missing gameplay, B missing/incorrect 3D interpretation, C visual/animation, D intended 3D
adaptation) · evidence/test · priority. Known rows so far:

| Mechanic | Class | State |
|---|---|---|
| Step onto a one-tile ledge | A (was wrong) | done D4 |
| Bye at world select | C | done (`78757b1`) |
| Win cheer at the exit, then straight to the hub (video §d) | A/C | done D29 |
| Arrival beside the last door in a hub (video §a) | A | done D19 (ArrivalPastDoor / ArrivalBesideSign) |
| Hub door slides up after a win (video §c) | C | done (every opened door of a hub slides up on arrival: `OpenDoorAt`, ObjectType22, as `Decor::AdaptDoors`/`OpenDoor`) |
| Crate pull (Recede/Advance) | A | done D21 |
| Vehicle × enemy contact (skateboard vs. mine, D13; then every vehicle and enemy) | A | mines done D13; every other vehicle × enemy pair open |
| Helicopter glue drop (HelicoGlu, Decor.cpp:3694-3708, 3058-3080) | A | done `85bc1ae` |
| Swimming, surface float, the jump out of the water (Decor.cpp:4542-4692, 5336-5357) | A | done D32 |
| Reactions: JumpAie, Ouf1a/1b/3/4 | C | done `ec84d42` (clips pending) |

## 12. QA gates and tools

- Build `-j2` in `build-cna-opengl3/`; `ctest --test-dir build-cna-opengl3 -j2`.
- Live: `tools/qa_live.sh <build> <out> <steps> [--mission N] [--spawn x,y,z,yaw]` — isolated
  saves, window-cropped screenshots, fails on an error in the log.
- Worlds: `tools/vwr_inspect.py`, `tools/world_inventory.py`.
- Models: converter reports, winding consistency check (§7.2), live views.
- No claim of visual parity from compilation alone.

## 13. Milestones

| | Milestone | State |
|---|---|---|
| A | Protect and understand: audit, captures, this plan, inventories | done (this document; asset manifest still to write) |
| B | Representative multi-height tile mission on existing systems | done: mission 011 `4986391` (`plans/briefs/world011.md`) |
| C | Model/animation integration and parity (D1-D3, D10, Push/Bye live, parity matrix) | in progress: D1-D3, D10 done; parity matrix open |
| D | Hub network and the 64 missions + 199 | in progress: main hub and worlds 1-3 complete (001; 010-014; 020-025; 030-034), worlds 4-5 being authored; worlds 6-12 and 199 open |
| E | Editor slots, demo seed, title screen, editor entry, transparency | done: `7eac7d5`, `6917fc9`, `e3a1042`, `fb48872` |
| F | Stabilisation and completion audit | open |
