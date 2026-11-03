# handoff.md — where work stands and how to continue

For the AI agent that continues this work in a fresh context. Read this file first, then the
documents it points to. Update it after every committed task (what was done, where it stopped,
what to run next). `NEXT.md` is frozen history (do not add to it); the backlog with checkboxes is
`plan.md`; verified current state is `CURRENT.md`; the active plan is
`plans/GALAXY_EGGBERT_COMPLETION.md`.

_Last updated: 2026-11-03 evening (OR-0056 reopened: front models on a floor face their room; OR-0057..0059)._

## 0. Start here (fidelity audit, since 2026-11-01)

The owner's campaign brief of 2026-11-01 is milestone G: `plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md`
(plan, waves, next steps) and its ledgers in `plans/audit/` (regenerate the CSVs with
`tools/audit/*.py`; hand-written parts in the `*.notes.csv` sidecars and the `.md` ledgers). Read the
ledgers before repeating any discovery. Where it stands:

- **Owner reports OR-0001..0023** (`plans/audit/owner_reports.md`, the owner's spot checks of
  2026-11-01; the owner expects the audit to go deeper than the list). Done: OR-0001/0015 lives
  row and life-loss icon; **OR-0021/0022/0023 continuous crates** (BD-0014, M3D-0001 rewritten,
  M3D-0002 lane slide): crates are boxes at any X/Z, move by `CaisseGetMove` while all of the
  group fits, stop the frame he lets go, overhang by the original's 20 px corner supports; a pull
  needs room only for his collision point. Autoplay: `crate X Y Z [tol T]`; a push in a script
  now stops Blupi exactly 57 px (0.89) behind the crate's goal (`go ... tol 0.05`). All 24
  crate playthroughs pass. OR-0008: the executable is `GalaxyEggbert` (target still
  `GalaxyEggbertCNA`), the window "Galaxy Eggbert".
- Done 2026-11-02: OR-0002 (Bye froze: the world-select check returned early every frame of
  the farewell), OR-0003 (the skateboard is put down at DeposeSkate's tick 12, taken at
  TakeSkate's 8; live look still to do), OR-0007 (the port's 21 typed cheat words,
  `InputPad::TypedCheat`, `ApplyTypedCheat`), OR-0018 (a lift rider's clip: Stop, not Air; the
  riding itself unchanged -- a first cut that removed his per-frame fall broke 066's lift
  boarding), OR-0019 (the helicopter's ceiling crash, `HelicopterHitsCeiling`, sub-cell masks),
  OR-0020 (vehicles left only on the ground and off lifts, the helicopter/overcraft on both foot
  probes, the skateboard below 8 px a tick: `CanLeaveVehicle`), BD-0015 (a spring wrecks the
  vehicle). **OR-0005 / BD-0016: every creature at its type's original speed and rest**
  (`tools/worldkit.py` `CREATURE_TIMING`, the registry's `patrolTicksPerCell`, the 3D editor;
  `python3 tools/audit/patrol_envelope.py` must report 0). 46 worlds rebuilt; 046/051's mines
  given the original's three-cell beat; 043's chimney opened into the dome (LV-0021).
  Playthroughs re-timed with `tools/autoplay_search.py --jobs 3` (middle of each passing
  window); autoplay logs `vehicle ...` changes and has `expect/until vehicle`;
  `AUTOPLAY_REALTIME=1` paces a run at 60 Hz for `qa_live.sh` screenshots. All 78 pass.
- Done 2026-11-02 (later): OR-0016 (the drink's Hide: his own clips at 30 % opacity, as
  Decor.cpp:828-838; `plans/evidence/hide-drink.png`), OR-0013 (the editor runs the world's
  animation clocks; live look to do), OR-0010 (Setup: Jump side mirrors the stick and buttons,
  automatic zoom 1.3x while walking -- `plans/evidence/auto-zoom.png`, the accelerometer
  steers by tilt with the original's dead zone; settings saved as `jumpRight`, `autoZoom`,
  `accelActive`; the sensitivity slider is not drawn yet; tilt untested on hardware).
- Done: OR-0014 world music -- the original tracks (`SOUND/MUSIC000-009.BLP` in
  `Speedy_Eggbert_1.7z`) rendered by `tools/music/render_music.py --ogg` to `Content/music/`;
  a world's `musicTrack()` (header sky word, second byte), the originals' `music=` for every
  world (`worldkit.ORIGINAL_MUSIC`), the editor's M key; not on the web yet. OR-0015's death
  clips: Clear1/Clear3/Glu show the original element.png sprite (live look to do).
- Done: OR-0009 (Galaxy's loading screen: the title scene with "Loading" and the original's gauge
  on a menu panel, `InputPad::DrawGalaxyLoading`; Mobile Eggbert's picture only as the fallback),
  OR-0012 (the editor's world list stands on the title scene: a 5 x 2 grid of pad buttons under
  the tile letters, trash/tick to delete, Setup's return arrow; `EditorBrowserScreen`). The host
  loads `title.vwr` for Wait and for the list (`titleSceneUnavailable_` stops retrying when it
  fails). Evidence `plans/evidence/loading.png`, `editor-browser.png`.
- Seen live 2026-11-02: OR-0013 (editor animation, `plans/evidence/editor-animates.png`) and
  OR-0015's deaths in probe worlds (`build-probe/probe-worlds/world996.vwr` mines,
  `world997.vwr` lava, made by a throwaway script; run with `QA_LIVE_WORLDS=build-probe/probe-worlds
  tools/qa_live.sh ... --mission 996`): `plans/evidence/death-mine.png`, `death-lava.png`. Fixed
  on the way: the lava death's rising icon was drawn during its 30-tick hold (a second charred
  Blupi); the HUD now gets `VoyageActive() && VoyageIconVisible()`.
- OR-0017 (Blupi partly in walls): his model is drawn out of walls now (render only,
  `WallClearanceOffset`, 2026-11-02). The collision box (18 px each side) tried three times: the
  patch fails 55 of 78 playthroughs on today's tree, mostly timing and points by walls.
  The second try became the patch the playthroughs were re-planned for; in the game since
  2026-11-03 (`a45e785`, see below).
- Done: OR-0004 as far as it can go without detail -- teleporters looked at live (world034) and
  one difference fixed, BD-0017: Blupi lands under the partner pillar (was one cell +Z, turned
  about) and a teleporter takes him only once the contact grace (`m_blupiPosHelico`) is over;
  106's well pillars moved over the wells; 066/102 scripts adjusted. Then compared with the
  original running: the arrival's two sparkle clusters added (`SpawnTeleportArrival`), the second
  sound on arriving removed (`plans/evidence/teleport-arrival.png`).
- Done: BD-0018, six Blupi sounds the original plays and Galaxy never did (turn 5, bar grab 34,
  bar jump-off 35, hanging fidget 36, idle taps 37, crate grip 39), as `BlupiController`
  events. Hanging Blupi has his clips now (StopSuspend/MarchSuspend from `suspendu*.rd4`, only
  those two exported into `avatars3d/blupi/`; a full re-export would also change Hide and
  StopTank -- converter drift to look at first; checked 2026-11-02: the converters of 048c4c0
  and of today build a byte-identical `blupi.glb`, so the day's converter changes (planar caps,
  baked texture repeats) do not touch Blupi; the shipped library differs from any full export in
  ~10 clips (Stop, Mockery(i), Ouf4, Charge, Switch, Teleporting, Vertigo, Hide, StopTank) and
  holds one face more (`face_18abc13655.png`), being assembled clip by clip from builds of
  different days: do not export the whole library blind, compare clip by clip), and the 10-tick JumpSuspend wind-up before he
  lets go of a bar (bars in 064 065 066 071 074 075 083 084).
- Done: BD-0019, the head shake (Non) when the action button has nothing to do: the host's
  `actionUsed` collects every use of the press; BD-0020, Advance: the void behind him, he steps
  forward 0.31 of a cell; with the vehicles' ledge rules (helicopter/overcraft recede at once,
  a parked jeep/tank/skateboard with a wheel over the void rolls on 10 ticks, `m_rollTimer`). New clips are exported one at a time
  (`build_clip_library.py --skip-check` into `build-probe/rd4-blupi/library-susp/`, then copy
  only the new clip, `avatar.skinnedmodel.json` and `clips.txt`).
- Done: BD-0021, hazard contact by the original's boxes (`MoveObjectTouches`: 28 px across, by
  type's vertical band) instead of a 0.7-cell sphere; the contact grace 30 px; a teleporter waits
  until he leaves the cell he arrived in; pickups by the boxes too, the playthroughs re-aimed
  (detours onto items, lick points, exits, dives; found with a temporary probe that logged every
  item the old sphere would have taken -- the method is in BD-0021). BD-0022: a pickup's reward
  was lost when its HUD flight started off screen (projection now clamped).
- Done: BD-0023, keys held as the original's flags (a second key of a held colour stays put;
  Galaxy took it and lost it at the first door). Egg and treasure reviewed: match.
- Done: BD-0024, vehicles boarded by the original's box contact (was a 1.1-cell sphere), and a
  helicopter floating over nothing boarded on contact without the button.
- Done: BD-0025 (an invert sign is taken again while inverted, Decor.cpp:6040). **The bar in 3D**
  (OR-0024/0026/0027, M3D-0003, `BarRows.hpp`): a bar cell is two parallel pipes of the original
  model (`barre_barre`; 202 = its pipe alone, `barreplain_barre`, built with the new
  `objects.txt` option `drop=` that leaves scene parts out) along its row, one under each of
  Blupi's hands; the cell shows its mounts only under a tile they hold to (`ShownBarIcon`, the
  original editor's rule: `Decor::IsRightBorder` compiled from a copy and run over every icon in
  `build-probe/irb/`; 301 of the 303 bars of its levels), whichever bar icon the world holds. On a
  row Blupi hangs on its centre line facing along it and moves along it only; fields (071, 074,
  075) stay free. His model hangs 6 px lower (BLUPISUSPEND) with his right palm on his pipe per
  icon (`HangingModelShift`, measured on the clip library). Evidence
  `plans/evidence/bar-two-pipes.png`; probe world 995 (bar row along Z under a two-cell beam).
  **The teleporters** (OR-0025): each letter its scene's model in its one cell (330 alpha
  `teleporteur1`, 331 beta `2`, 332 delta `4`, 333 chi `3`; IoU 0.997), the device its top third,
  the tip under it; the front (lights, letter) turned to the cell's side with the most room to
  stand in (`ModelCell::openSide`, `kFrontModelIcons`); the old cube and hanging pyramid are
  only the fallback. The converter's flat end caps of prisms now take the whole image
  (`freeform_mesh.planar_caps`, kept out of `split_seam`): the letter panels were a magnified
  corner. Evidence `plans/evidence/teleporter-model.png`; probe world 998 (four teleporters).
  Also: texture repeats (`tile 1`, `cnth`/`cntv`) baked into the image for every mapping (the
  house's flowers at their size; the trees `plante2a-c` not re-exported: +0.0015 for 8 MB); the
  yellow trestle 199 as `Decor/support3.rd4` (front to the open side like the teleporters). The
  numbered world signs 174-181 (`Decor/world1.rd4`, outline 0.995) wait: the scene has no digits
  (painted on the renders) and its face converts as a disc; a digit decal needs the owner's word.
  All 78 playthroughs pass after the bar row change (2026-11-02). The posts 182/183 are their
  models too (`porte1/2.rd4`). Waiting on instanced drawing: the cave rocks 285-303
  (`grotte01-19.rd4`, 0.81-0.91; ~6,000 cells). The switch 384/385 (`interrupteur.rd4`, outline
  1.000): its scene does not swap the lamps (385's sprite has them recoloured: `recolour=`), and
  most switches sit in walls, where the pillar box's four faces still suit.
- Done 2026-11-02 (Wave E): BD-0029, an updraught held a falling Blupi in the air (the fall cap
  was 10 blocks/s, the original's is 40 px a tick = 12.5; the wind's 20 px a tick only halves it):
  `kFallLimit` -12.5, `kWindVerticalSpeed` 6.25; found in world 102, whose plate works go down a
  fan's updraught shaft. `tools/audit/lesson_inventory.py` lists per world what the original has
  and Galaxy leaves out (`plans/audit/lesson_inventory.md`).
- Done 2026-11-02 (Wave E, LV-0060): world 102 gets the original's lessons it lacked: two
  lightning curtains across the machine yard (x 23, 31; passed in one quiet half on the board),
  the plate works' way out (a fan's updraught shaft at (86, 17) down to a gallery, the switch
  stopping a saw on its row, a wind channel with a bar over it to a fourth teleporter 333, the
  atrium's (61, 82)), the red key on a crumbling bridge over lava. Playthroughs 102 and 100 pass;
  `plans/evidence/world102-lessons.png`. The lightning (305) seen live: drawn while deadly.
  `lesson_inventory.py` now counts the bars too: 22 worlds still leave out a mechanic of their
  original (bars in 024, 034, 046, 052, 061, 106, 113, 114, 125, 199); next by weight: 114, 113,
  115, 107, 105, 199, then the bars and the small ones (lava pits, single fans, crushers in 084).
- Done 2026-11-02 (Wave E, LV-0070): world 114 gets a bar over a lava pit on the way south
  (z 60-64, crossed hanging both ways), a one-wide passage over three updraught fans under a
  sweeping mine on the way west (an updraught jump peaks at y 24.8, the mine sweeps at 25), and
  lightning columns on the north and east ways. Playthroughs 114 and 110 pass;
  `plans/evidence/world114-lessons.png`. Open there: the original's sideways wind rooms.
- Done 2026-11-02 (Wave E, LV-0069): world 113 gets the original's bar over lava between two
  hanging mines (x 35-39, one wide; the lift pits moved to x 15, 20, 25, 30). Learned: hanging,
  Blupi touches anything in the bar's row within 0.44 (the hanging mine's box is the top 24 px of
  its cell); a jump off a bar under a ceiling regrabs it, Down held 5 ticks drops him.
- 2026-11-02 evening, the owner's live reports (OR-0028..0035, `plans/audit/owner_reports.md`;
  the queue with sources in `plans/GALAXY_EGGBERT_FIDELITY_AUDIT.md` §6 and the task list):
  BD-0031 (every effect at the table's pitch, the crate's effort once, not looped), the Setup's
  volume sliders (effects, music; Erase progress a row lower), the fans at divisor 2, the
  helicopter climbing with Jump, Right Ctrl backing up, a gamepad (`GamepadInput.hpp`), multi-touch
  with a back-up button (`InputPad::UpdatePlayTouches`), the active cheats top-left, the coloured
  doors 334-336 and the vanishing saucer 324-329 as their Ray Dream models
  (`kAnimatedModelBases` draws an animated tile's model per frame). BD-0030 (jump headroom as
  IsPassIcon) changed jumps under wind and bars: 107's saucer ledges moved to 46 and 54 with
  straight hops, 124's wind tunnel fills its height. Still queued from the owner: the crusher (it
  hangs in Blupi's cell in the original; Galaxy put it in the floor) and the flattened Blupi
  (`blupi-plat.rd4`), the clay bridge (`grotte-fragile1.rd4`), the spring (`ressort.rd4`), the
  lightning and the charred Blupi (no scene: to model), grass, the glue death (`glu.rd4`; its
  icons are element.png's, the clip library matches blupi.png's only), the world signs.
- Done 2026-11-02 late (OR-0038..0054): the saw as `Decor/scie.rd4` on the floor around it, its
  floor cubes drawn opaque (`m_sawFloorRenderer`: in the depth-read animated pass the blade's lower
  half showed through, OR-0051); a fan a whole block (M3D-0004); Down and Air hold their last frame
  (`GetAnimHoldFrame`); opendoors as `Decor::AdaptDoors` with `m_bCheatDoors`
  (`HubDoors::AdaptHubDoors`); the back-up button just over the stick's ring;
  `build_clip_library.py` `ease_doubled` (35 clips replaced, vehicles excepted; compare clip by
  clip with `--no-ease` before replacing anything); **BD-0032 the crusher** hangs in Blupi's own
  cell of a one-high passage, passable, `aplatisseur.rd4` via `kAnimatedModelBases`, the squash
  ends vehicles and powers (045/053 rebuilt, 199's invented row removed; levelcheck: a way, not a
  floor); the death angel (element 230-241) as `blupi-ange.rd4` in 3D at half opacity
  (`build_object_library` `alpha=`); water drawn after the models (`DrawOpaque`/`DrawTransparent`,
  OR-0052); the region background a panorama turning with the camera (OR-0053); the world's music
  in the editor palette's Background group (OR-0054). `GenerateSampleWorld3D` writes only world999
  (it overwrote every world). All 112 tests pass; playthroughs: all 78 after the saw/fan, then
  045/053/199 after the crusher (render-only changes since).
- Done after that: the gallery `worlds3d/world998.vwr` (`tools/worlds/gallery.py`, its map
  `plans/audit/gallery_map.md`; the editor seeds it into the first free slot and refreshes it while
  unedited, `SeedGallerySlot`); the web build of 2026-11-02 with it (checked in headless Chrome
  under Xvfb over DevTools: `--use-angle=swiftshader --enable-unsafe-swiftshader
  --remote-debugging-port=9333 --remote-allow-origins=...`, serve the build dir over HTTP first).
- Done 2026-11-02 night (after a full 78-world regression run, all passing): **BD-0033 the slime
  drip** (`a0d31de`: passable, kills in his own cell as `IsGoutte`, animated by
  `table_decor_goutte`, drawn as `goutte01-04.rds`; 062/063/066 replayed); OR-0050's sweep
  (`04c66c0`: the switch 384/385 and the conveyor 311-316 as models, the conveyor stretched along Z
  by `ModelDepthScale`; `tools/audit/model_sources.py` lists what is left: `electro.rd4` Q-005,
  the terrain tiles Q-004, `colonne1`); **OR-0055 the clay bridge** covers its cell: its one row of
  lumps drawn four times across, every other turned round (`kBridgeRows`), the scene's teal
  surface dropped (`drop=2`); the charred deaths (Clear1, Clear3's first half) as his own model
  burnt black and shrinking to the heap (`39d800a`, `CharredBlupi.hpp`), the glue sprite now on
  his feet (it hung half a cell up); the column 375 back to its sprite (`b887345`:
  `Decor/colonne1.rds` is a cube with a painted column, no turned shaft exists). Probe world for these: `build-probe/wave-e/probe_burn.py`
  (mission 995: lightning spawn 50,1,42,90; spikes 50,1,44,90; drip/conveyor/switch 50,1,48,90;
  bridge 44,1,56,90). Note for qa_live: a mine kills by a coin flip (Clear1 or the angel); the
  lightning is always Clear1; only spike 373 kills (347/374 are its animation). Measure a
  model's size at its frame-0 pose (`tools/rd4_to_gltf/posed_bounds.py`), not from the accessor
  bounds: the bridge's bind pose sat 0.109 off.
- Done after: the conveyor lifts 47/48 as the chenille model (`bda333e`, world 102); Wave E:
  084's three gates of crushers (`0cf9a4f`, LV-0051), 024's rods over the lava hall's last gap
  (`7fc63b9`, LV-0011), 034's lava pit across the gallery crossed on rods (`1e99936`, LV-0017),
  each with its playthrough; **OR-0056** the teleporters' letter plate and lights on all four sides
  (`3be74e6`); then 106's rods before the west teleporter (`74184d8`, LV-0064), 061's clay
  bridge to treasure 5 in a lava pool (`9894f64`, LV-0035: the bridge grows back after 156 ticks
  of 20, so a script waits ~520 frames on the far side), 066's crumbling bridge cell over lava in
  the upper corridor (`f79dacb`). 052's rods under its corner drop were tried and reverted: the
  helicopter parked at the well's foot then could not be boarded again (LV-0027, open). Web build
  refreshed (`CURRENT.md`). Rods are `ROD = 202` at the walk over a drop
  (`go ... max 600` crosses them in a script); levelcheck stages put a floor under them.
- Done after (2026-11-02, late night): a full 78-world regression run passed after all of the
  above; the inventory's spikes are icon 373 alone (`f27de41`: 347 is slime); **the glue death in
  3D** (`433b01d`): `glu.rd4` holds only a yellow Blupi, the sprites' green was added afterwards,
  so the converter's new `texrecolour` turns its yellow (texture skin too) green, and the game
  lays his flattened model under it as the puddle (strands not drawn; Q-006 updated).
- Wave E after that, each world replayed: 111's second pit over lava (`1fdcace`), 124's lava
  strip across the start passage (`4ddeab0`; not at x 8-9: teleporter 332's partner stands
  there), 122's pocket over lava on a lift (`8478ed7`; wait for the lift's next arrival before
  boarding back), 014's water channel in the key room (`62bca18`; climb out with `with lookup jump
  1.2`; levelcheck's surface stand-in), 112's two floor saws (`c03bd14`). 105's lava is its
  bottomless hall (LV-0063, reviewed earlier). Left in `lesson_inventory.md`: 013/124 thinned
  counts, the bars of 046/052/061/125, 113's fan (which way its wind runs against the route is
  not clear from the map), 125's lava, 199 (UNREVIEWED, many).
- Then: the render ledger counts blocks drawn as models (`2eeee31`: 97 were misfiled as plates
  or cubes); **the treasure doors 421-439** as the golden post 183's model with their number plate
  a decal (`5cd6968`, `sign_decals.py --plate`, listed with the signs in `kSignModelIcons`). The
  flagged cubes left in use (render ledger: toy balls/badges 215-236, pipes 250-260, gates 86-90,
  187-192) match `Decor/enfant*`, `tuyau*`, `meca*` scenes: they wait on Q-004 (terrain tiles as
  models).
- Then (2026-11-03): 052's rods (`35f8b4a`, `f3c8ec2`: the first try's failure was the script's
  re-boarding at the edge of the helicopter's box, found with a trace of parked helicopters; the
  pit is two deep, no lava: 052 has none). The use case ledger worked through by kind: every sound
  channel's call sites compared with Decor.cpp (`$CLAUDE_JOB_DIR`-free method: grep both sides),
  every action's frame table compared with table_blupi. Found and fixed: the ceiling tap (BD-0034,
  channel 4 + remaps), the mockery's timing and cut (BD-0035), the glue shot's sound (BD-0036),
  invented march footsteps removed (BD-0037), Ouf5 after dropping off a bar (BD-0038, its clip
  built with `build_clip_library.py --skip-check` in 38 s and only the new clip copied: a full
  rebuild changes Hide and StopTank, which were assembled selectively), Clear4's two missing frames
  (BD-0039). Ledger: 331 UNREVIEWED left (objects, functions).
- Then (2026-11-03, later): **the use case ledger finished** -- every function, action and object
  type of Decor.cpp compared (`plans/audit/gameplay_use_cases.csv`: 0 UNREVIEWED; 207 match, 96
  fixed, 53 3D adaptations, 181 not applicable). Found and fixed, each with a verifier check:
  the Cloud ends on the water's surface (BD-0040), the jump out of the water splashes at the jump
  (BD-0041), a fan touched with the shield still explodes (BD-0042), every door key flies from
  x 520 (BD-0043), MockeryDetect ported whole -- height, air, lava/crusher, the 20-tick window,
  Mockeryp without cooldown (BD-0044), no switch with the shield/Hide/SuperBlupi (BD-0045), the
  dynamite cuts floor saws and drips and kills within the original's box (BD-0046), the vertigo
  at a lift's edge (BD-0047), the respawn FIFO's whole gate incl. the objects' and shots'
  trajectories (BD-0048), the pieces flying away (ByeBye: enemies, crates, the invert potion,
  fans, the lost helicopter; BD-0049), cleanall's explosions (BD-0050), dynamite/decoy only with
  both feet down, no decoy by a charger or with dynamite (BD-0051), the overcraft over a lift in
  reach (BD-0052), the crusher/balloon/trap/creature effects
  (BD-0054), the decoy trap's dissolve that never went and the Cloud kill's flash (BD-0055),
  Hide's trail (BD-0056). All 78 playthroughs pass; the web build rebuilt and checked over
  DevTools under Xvfb (`$CLAUDE_JOB_DIR/tmp/scripts/cdp_shot.py`, see its handoff note above:
  world001, 118 models, 93/93 sounds, no exception). Wave E: 061's bars (`836147ae`, the lava
  hall's first trench three wide under two bars along Z; hub 060 replayed). Looked at without a
  change: 046's single bar (over a one-cell gap in the original's key room; its role in the
  route is not clear from the map) and 125's bars under girders and lava strip (a helicopter
  world; the original's on-foot route there needs the video). The follower star's wake (BD-0053, Q-007) was later
  (2026-11-03) brought to the original's box, 052/053 re-laid for it (78/78). Probe world `build-probe/wave-e/probe_burn.py` (995) has a fan, a pool,
  a lava cell and a bulldozer for live looks; typed cheats in `qa_live` steps need
  `keydown`/`keyup` with a sleep (a bare `key` is too short to be seen by the game).
- Then (2026-11-03, latest): **Wave E, world 199's lessons all in** (LV-0078; `lesson_inventory.md`
  lists nothing missing for 199): a lightning curtain in the machine's passage (`d1b3222`), the
  cheese's lava trench on a clay bridge (`8f33b24`), teleporters 330/331 floor-ring (`56661d7`),
  two spike traps and two drips (`4251c6e`), the wasp's well with a moving mine across it, its
  balloon the way out (`ec46f4a`), the invented fish removed (`a190777`), bars over the machine's
  pit (`3912fe2`), a saucer climb to the south arm (`7f1c3c6`), a fan in the machine's east wall
  (`f6ac155`). Learned on the way: a creature's shot passes a closed door (`IsBlocIcon`: a door's
  mask is not full), so 199's script crosses the village's line of fire (z 58) with `clear`; the
  egg cap (10) stops pickups, so no extra egg on a route; an updraught (118) does not lift Blupi on
  foot, it halves his fall (6.25 of 12.5 u/s; use it as world 102 does). The spare enclaves (toys,
  palace, rock) are still not played through.
- Then (2026-11-03, latest): **BD-0057** -- a bar over wind (any tile with an empty collision mask)
  is hung from, as the original's `GetTypeBarre`/`IsPassIcon` (`b07711f`; Galaxy's `IsSolidAt` had
  read wind as a floor). 113's high way got the original's windy stretch: a fan in its upper row,
  bars over two mines in a one-wide lane (`31c4733`); 105 the teleporter pair between the exit's hut
  and a ledge under the roof with two helicopters (`1f72bbc`); 125 a tube of four mines crossed
  under bars off the pool's south bank (an egg at its end) and a lava strip across the corridor's
  mouth (`7b652c9`). 046's single bar recorded as the catch of a high jump through a floor hole
  that Galaxy's deck does not have (LV-0024). 105's lava floor: a full one made the run ~6x slower
  (the animated mesh is rebuilt every tick), so the void stays -- Q-008 asks about a cheaper lava.
  `lesson_inventory.md` now lists only 105's lava, 112's water pipes (scenery, LV-0068), 046's bar,
  013/124's thinned counts.
- Then (2026-11-03, latest): **ED-0007, the original editor's auto-tiling** (`fb76c3f`,
  `src/GalaxyEggbert/Editor/AutoTile.hpp`: `IsRightBorder` rule by rule, `table_adapt_decor`, the
  random twins, the lightning column; run after every placement, erase and box-fill cell as the PC
  editor's `decdesign.cpp` does; right/left from X or Z as the owner chose in MR-0005; one change per
  cell so one undo restores all; the cheese/cave edge plates in empty cells are not laid). Ledgers:
  the render ledger reviewed by the model library's render-back IoU (`cf221a1`: 13 rows left, D11's
  candidates), the asset sources classified from the survey's evidence (`6961504`: 60 left, a3d files
  without a Ray Dream twin and the root test files). Run the editor verifiers from the build
  directory (`ctest`): run from the repository root they write `customworlds/` there.
  Also: ED-0003's moving mechanisms (`cdd8503`: the four lifts are type 1 at the PC editor's speeds
  per cell, scaled when the end is set; the conveyor lift 47; the slime creature 54, not 18); the
  balustrade 398 as its Ray Dream model (`e60f61f`, `Decor/palais/mur06.rds`, a front model);
  **world 199's spare enclaves played to the win** by `tools/autoplay/spare/world199.txt`
  (`051fd54`; not in the playthrough set, run it alone); **the 57 unreviewed world lessons reviewed**
  (`042dafd`: 53 CONFIRMED_MATCH by brief + lesson_inventory + the regression + a spawn look, 013 an
  adaptation, 998/999 not applicable; at six spawns the camera starts overhead). **Regression of
  2026-11-03 after all of it: 78/78 playthroughs pass; ctest 112/112;** the web build rebuilt and
  loaded in headless Chrome over DevTools (119 models, 93/93 sounds, the menu drawn; Chrome 155 needs
  a fresh `--user-data-dir` and a few seconds before port 9333 answers).
- 2026-11-03, after wave F: BD-0053 fixed -- the follow star wakes in the original's box
  (`InteractionSystem`, `VerifyInteractionSystem` 1.555); 053's crossing star, gallery approach,
  hall columns (y 21-24) and cage choreography, 052's S2 drop timed to its lift and its hangar and
  well stars re-laid from a traced flight (`tools/autoplay` `trace`; the star-free probe world
  trick: generate the world with `star` stubbed into `build-probe/probe-worlds-052/`). 78/78.
- OR-0014 on the web (`789ceee`): the music served beside the page and fetched per track (see the
  web build section; checked in headless Chrome).
- **OR-0017 done (`a45e785`)**: Blupi's body is the original's 36 px box (BlupiController
  `kBodyHalfWidth`, `ResolveMove`'s `halfWidth`): his face stops 18 px from a wall, he stands while a
  corner of the box is over a top, ceilings stop its whole width; on foot and swimming, vehicles keep
  their point. `WallClearanceOffset` is gone. The harness takes a `go` pressed against a wall short
  of its point as arrived only when he is still in x, z, yaw and y past its first ticks. 78/78 with
  it (066 last: `e5111ab`, its one-wide tunnels walked 0.3 off the mines' line). When a script now
  fails by a wall, the usual causes: a target within a half-width of a wall (go along the middle),
  a ledge left with the box's corner still on it (go further), a jump under an overhang (start
  further back), an `until x >=` the body's face never reaches.
- **The play pass (`06f1323`)**: `AUTOPLAY_SHOTS=N` saves a frame every N ticks of an autoplay run;
  `tools/play_pass.sh <build> <out> [worlds]` plays every script in lanes with it (4 lanes:
  `PLAY_PASS_JOBS=4`, ~75 min) and writes contact sheets `<out>/<world>-pN.jpg` (tick and Blupi's
  place on each frame) plus `results.txt` -- a full regression and a look at the whole campaign as
  the player sees it. Never run two passes at once: they share `build-probe/play-pass-run-<n>/`.
  The first pass (2026-11-03, `build-probe/play-pass-1/`) found: (1) **single-textured-side tiles
  always textured on +Z** (the machine panel 10 and kin, house walls 186-192, palace 390-397: 27 %
  of their visible sides textured; the owner's questionnaire answer has the side per block) --
  fixed `0b401be` (`SingleTexturedSide`: the open side facing the most room; 88 %); (2) **the
  camera cut Blupi at the picture's bottom** whenever a wall pulled it within ~2 cells (the look
  point a block over his feet, 45-degree picture) -- `ChaseLookHeight` (ChaseCameraYaw.hpp) lowers
  the look point toward his middle as it comes in; it changes the pickups' flight time to the HUD
  (Voyage, by screen distance, as the original), so it waits on a full pass before its commit;
  (3) 045's cheese floor shows clusters of black dots near (59.6, 52.6, 15.4) and (49.4, 55.1, 12.1),
  not looked into yet; (4) frames with Blupi out of view while he walks down a stairwell (024
  t3600) or flies the helicopter in a shaft (043 t6120) -- to recheck with (2).
- Q-003 done (`22295e0`): the clay bridge wears `Decor/terre.tif` from above
  (`tools/rd4_to_gltf/top_texture.py`, run after the bridge's export; evidence
  `plans/evidence/clay-bridge-terre.png`). Q-001 (grass leaves) stays the owner's: 8,997 grass
  cells in the campaign (3,020 in 021) would need the leaves merged and simplified into the
  terrain mesh, not a model a cell; Q-002 (a bolt without a source scene) likewise.
- Then (2026-11-03, last): the editor ledger has no unreviewed row (ED-0002 by the storage verifier,
  ED-0004 driven live by `tools/qa_steps/editor_playtest.txt`, ED-0005 a mixed undo history in
  `VerifyGEWorldEditor`); the asset ledger has none either (`af4a487`: the survey's root folder was
  missed; a3d files without a twin are Amapi geometry of objects drawn from their scenes); world999's
  creatures at their types' timing (`491cdd2`, patrol_envelope 0; regenerated, it lost 19 stray
  trigger tiles the gallery shows on pillars). D11: the house's flower paper was fixed in `9582fc9`
  (the candidates sheet was made from a stale library); the fountain bowl and column are white in
  every channel of their shaders -- the sprites' dark bowl is Ray Dream's single light without
  ambient, a lighting difference. OR-0011 (later the same day): the Android port brought to the CNA
  game (`c5dcdc5`, `c48ee08`, `37ca3b3`; `ANDROID.md`): `libmain.so` with CNA's `Entrypoint.hpp`,
  the assets copied into internal storage on first start (`AndroidAssets.cpp`, Gradle's
  `assets.txt`), NDK r29 / API 24 / GLES 3, SDL's Java glue from CNA's SDL, stdout in logcat (tag
  `GalaxyEggbert`). The x86_64 APK runs in the API 35 emulator (`-read-only`; no KVM here, so
  software emulation, minutes per screen) through the menu into the main hub
  (`plans/evidence/android-emulator-*.png`), but draws no 3D model: CNA's skinned shader needs
  more than 256 vertex uniform vectors (UB-007, CNA's to fix). UB-006 (CNA lists the APK per
  missing `.xnb`) is worked around by an absolute content root. Q-009: KVM or a phone. The platform lift (type 1) is drawn as its Ray
  Dream model `Decor/techno29.rd4` (`7fc68ac`, IoU 0.94; the survey had filed it with the terrain
  for its folder; no other misfiled piece left there but the cave rocks, which need instancing).
  D11's candidates regenerated from a fresh library (`570aa48`, `c34dc2b`: 119 models; the known
  reasons now written in the list: decals, the fountain's lighting, the saw's specular). Web build
  rebuilt after all of it: 120 models, 93/93 sounds, no error in headless Chrome.
- 2026-11-03, evening (owner's reports OR-0056..0059): **OR-0056 reopened** -- the owner found the
  numbers "mostly turned wrong" after I had closed it from the gallery alone. Cause, found only by
  photographing the campaign: a front model's open side counted only room over a floor two or more
  cells down (written for the teleporters over Blupi's head), so every model standing on a floor
  -- the hubs' mission signs in their niches, the numbered posts' lower cells, 025's balustrades --
  fell to +X. Fixed in `9d363db` (`BlockModels.hpp FrontModelOpenSide`, `VerifyTileFaces`): all 273
  signs and numbered posts of the campaign face an open side. The booths themselves were photographed
  from the player's approach in every campaign world (`build-probe/qa-shots/booths/`, spawns three
  cells off each side with a floor). Lesson: check an owner's report in the
  campaign worlds as the player meets it, not only in 998. OR-0057 the conveyor's belt mirrored onto
  its back (`2e9e055`); OR-0058/0059 were the gallery's layout (`d8233a7`: the saw switches in the
  floor, the crusher on an open beam). The full regression with the committed build: 78/78.
  Noticed on the way, not yet looked into: in a dead-end one cell wide (102's booth at (70,3,12))
  and in some low passages (095 at (30,31,20), 105 at (37,18,48)) the chase camera ends up over
  Blupi's head or inside it.
- Next: the glue death's strands if the owner wants them (Q-006), the lightning bolt and grass (Q-001/Q-002)
  wait for the owner (`plans/audit/owner_questions.md`, Q-001..Q-008); D11's model list (the
  candidates regenerated from a fresh library); Android on a device once Q-009 is answered and CNA
  fixes UB-007; a human play pass of every world. Wave F's release audit: `plans/audit/release_audit.md`.
- Owner's decisions 2026-11-02 (asked interactively, `plans/audit/manual_review.md` MR-0005/0006):
  the editor's auto-tiling (ED-0007) **fully**, right/left read from both X and Z; the world signs
  174-181 as the `world1.rd4` model **with the digit** as a decal from the sign sprite; OR-0006 not
  tried yet by the owner; next priority **Wave E** (per-world reviews).
- Done 2026-11-02 (Wave C): BD-0026 (an exit checked every 51 ticks while he stands on it, the
  original's `m_goalPhase`: the last treasure landing while he is on it wins), BD-0027 (no
  dynamite or decoy put down on a lift, `m_blupiTransport`); matching after review: the
  lollipop, the drink, taking dynamite, the shooters 32/33, the overcraft (46: boarding ends
  Cloud and Hide, as every vehicle); BD-0028 (a decoy meets the bulldozer and the shooters by
  the original's boxes); the fish and the bird's contact. All 29 types the original's worlds use
  are reviewed; left: the secret exit's `m_bFoundCle` (no consumer) and a close look at the
  shooters' and the big creature's height (the original draws them 6 px lower).
- **The original can be watched running**: `tools/qa_original.sh` drives the Linux build of the
  mobile-eggbert port (a copy in `build-probe/orig-port/` of the owner's
  `~/Downloads/SpeedyBlupi2013Linux`; never touch that one) like `qa_live.sh`; its Play always
  starts mission 1, so copy the map to watch over the copy's `worlds/world001.txt`. Use it
  whenever the source leaves a behaviour open.
- Open owner reports: OR-0004 (anything else about teleporters: the owner's word), OR-0011
  (Android: SDK, NDK 29 and emulator images (android-35 x86_64) are in ~/Android/Sdk; gradle
  wants NDK 30.0.14904198, CMake 4.1.2 is there; an APK build compiles everything for two ABIs.
  `android/app/src/main/assets/` links only `Content` and the old 2D `worlds` (May): the 3D game
  also needs `worlds3d`, `avatars3d`, `textures3d`, and CNA's Android file access for them is
  untested), OR-0017 (his
  collision is a point: a body
  half-width of 18 px, the original's BlupiRect, for horizontal moves only, then all
  playthroughs).
- Script lessons from the vehicle rules: wait ~40 frames for a skateboard to slow before Action
  (and never let it glide onto a lift); settle the overcraft (`hold 40 crouch`, `until ground`)
  before Action; fly the helicopter well clear of ceilings (descend before entering a lower
  passage; it drifts a cell after a stop).
- Done: baseline (70d2b99, 110/110); ledgers (`f4d67c8`); silent runs (`b33fac3`); **D60 crate
  push/pull** (`1c59eae`; BD-0001/0002, M3D-0001):
  `CratePush.hpp`, `InteractionSystem::UpdateCratePush()`, crates as boxes in Blupi's collision,
  `BlupiController::FollowCrate()/SetCrateDriven()/CanStandAt()/IsBlockSolidAt()`; tests and the 18
  crate playthroughs pass; evidence `plans/evidence/crate-push-012.png`.
- Game runs are silent now (owner's request): `tools/autoplay.sh`/`qa_live.sh` default to
  `SDL_AUDIODRIVER=dummy`; `QA_SOUND=1` for an explicit sound check.
- Done: **BD-0006** — every shipped lift within the original's speeds (`7e0d1cf` world061: the
  shuttle ladder's jump window ~5 frames -> ~0.9 s with the original's speed and a 30-tick dwell;
  `46a663b` 113, 024, 062, 073). `python3 tools/audit/lift_envelope.py` must report 0. Method for
  a lift problem: the original counterpart's speed in `../mobile-eggbert/worlds/worldNNN.txt`;
  probe with a copy of the builder (`--stage -1 out.vwr` reproduces 061 byte for byte) in a
  worlds dir of symlinks (`AUTOPLAY_WORLDS=<dir>`); measure windows with
  `tools/autoplay_search.py`. Never `pkill -f` a pattern that also matches a parent shell.
- Done: **BD-0003** (a crate could be pushed out over water: support needs a solid mask; the 18
  crate playthroughs unchanged).
- Done: ED-0001 editor variant cycling (`e8a86ae`) and ED-0006 for the BigDecor families; BD-0004
  follow stars and shot ranges vs decor (`92a1a6f`). After all of today's gameplay fixes: 110/110,
  all 78 playthroughs, the whole campaign 761140 ticks (2026-11-01).
- `.a3d`: decoded and read (`tools/a3d/a3d.py`, all 105, `plans/audit/a3d_format.md`). The lava
  frames are a height-field slab, the columns profiled shafts: the next step is a 3D lava surface
  and pillar models from them or their `.rds` twins.
- `.max` (MR-0001): the owner offered a 3D Studio MAX 3.1 ISO; setup was stopped unaccepted (the
  key file holds the generic key-generator serial, the ISO a `CRACKS` folder); still BLOCKED until
  a genuine licence is confirmed. The XP VM was shut down, its ISO detached, its network still
  `null`. Do not change the VM's security settings (a registry change for blank-password guest
  logons was refused by the permission classifier).
- Fan/lava/pillar live views: first captures in `build-probe/fa-live/vis*/` are too imprecise;
  take them in world999 from the exhibition rows' documented positions, first person (F3), close.
- Done (visual, Wave B): lava as a pool with the original wave from `Decor/lave/lave-1..5.a3d`
  (`90acade`, `LavaSurface.hpp`, data `tools/a3d/lava_surface.py`); fans as the original model
  `Decor/ventillo-h.rd4` turned per direction (`6daa06b`); pillars 76/77 as `support1/2.rd4`
  (`801c232`). Mechanism for more: `BlockModels.hpp` — animated families by base
  (`SetModelDrawnBases`) and still icons (`kStillModelIcons`, the terrain constructor's list), drawn
  by the host's model pass. A new block model: build it with `build_object_library.py` (probe in
  `build-probe/`), export with `export_cna_avatar.py`, copy `icons.txt`, add to
  `avatars3d/objects/index.txt` and `tools/rd4_to_gltf/objects.txt`, list its icon in
  `BlockModels.hpp`, regenerate `model_parity.md`/`assets.md`, look live (before/after).
- Done (Wave E): 066's blupit walks across room B1 so its line of fire must be crossed (`63e37fe`,
  LV-0040; crossing window measured with `autoplay_search.py`); 094's mine rows checked.
- Next: pillars 375/376 (no source found: `Decor/colonne1.rds` is a plain textured cube, not the
  turned column); the wind tiles
  (`Decor/vent-norm/` holds only rendered frames: a 2D effect candidate); a low-angle owner look at
  lava/fan/pillars; Wave E for the other mechanic-heavy worlds (199 next); Wave C type by type (an
  enemy review of types 4/16/17/20/44/54 was started 2026-11-01).

## 1. The project and the goal in one paragraph

Galaxy Eggbert is a faithful, true-3D tile/voxel remake of Mobile Eggbert / Speedy Blupi, written
in C++ on CNA (an XNA 4.0 reimplementation) and cna-engine. The owner's standing goal: finish it as
a faithful, fully playable, polished remake, keeping the existing architecture (voxel worlds,
`.vwr` format, the 3D editor, controls, camera, registries, saves), with gameplay taken from
`../mobile-eggbert` (C++, the authoritative reference) and models/animations converted from the
original Ray Dream scenes in `../speedyblupi-data`. Read, in this order:
`CLAUDE.md` (repository rules), `../CLAUDE.md` (build rules for every repository),
`plans/GALAXY_EGGBERT_COMPLETION.md` (direction, defects D1-D59, milestones in §13), then this file.
`plans/SESSION_SUMMARY_2026-10.md` summarises the whole session that brought the project here
(what was done, the evidence, the honest list of weaknesses).

## 2. State (verified)

| Milestone | State |
|---|---|
| A, B, D, E | done (plan §13) |
| F, stabilisation and completion audit | **done**: suite 110/110; `tools/qa_sweep.sh` loads all 79 worlds live, 79/79; `campaigncheck.py`, `watercheck.py`, `VerifyShippedWorlds` pass; every mission has a playthrough, every world a chain from its hub, and the whole campaign plays in one autoplay run (755171 ticks, ends on the win screen) |
| C, model/animation parity | **open only for D11 and BB3D-032** (below); everything else done: parity matrix `plans/inventory/model_parity.md`, every turning object matched every 5 degrees, objects and vehicles toned to their sprites, Blupi's 45 clips at 0.94-0.99 silhouette IoU |

Engine defects found by the playthroughs and fixed with tests: D55-D59 (penetration recovery, a
lift's reach through ledges, a lift taking him sideways off a ledge, a key opening one door cell,
dynamite taken two at a time / without Action). World defects fixed: 013, 025, 062, 066, 082, 094,
103, 104, 105, 113, 114, 121-124, 199 (details in the plan's §13 row F and the briefs in
`plans/briefs/`).

The Web/Emscripten Release build was freshly rebuilt and linked on 2026-10-31 against the current
CNA and cna-engine checkouts. Headless Chrome created a real WebGL 2 context, loaded `world001`,
82 object models and 93/93 sound channels, uploaded the terrain, and visibly rendered the
gamer-select menu over its 3D scene. The build exposed and fixed one portability defect:
`ChaseCameraYaw.hpp` now includes the standard header required by its initializer list.

## 3. What is open, and exactly how to continue

### D11 — the owner's list of models that look wrong (the only real blocker)

The owner said they will send the list later. Until then nothing in D11 can be closed. To help
them, `plans/inventory/d11_candidates.md` lists the models that differ most from their sprites
(worst silhouette IoU, largest remaining colour error) with the numbers; the contact sheet of
sprite vs model for those (sent to the owner) is regenerated by
`tools/rd4_to_gltf/d11_candidates.py` (its docstring has the commands; the full library it reads
is `build-probe/rd4-objects-current/`). Seen on that sheet, worth looking at first even before
the owner's list arrives:

- `bd_maison` (the house): the sprite's walls carry a pink texture with daisies; the model shows
  pink with yellow and white blotches. Looks like a texture placement fault of the converter
  (a real candidate).
- `bd_big02` (fountain bowl) and `bd_big0108` (column): grey in the sprites, near white in the
  model; a tone fit made the bowl worse, so it got none. Look at their shaders (highlight /
  reflection channels) rather than toning.
- `bd_cabane` (cabin): its logs are a procedural wood grain, flat here (decided, see below).
- `dynamite`: the sprite's lit fuse spark is an effect, not part of the model (expected).
- `bird`, plants 1 and 3: thin wings, legs and leaves a pixel off; faithful (recorded).

When the list arrives, for each named model:

1. Find its entry in `tools/rd4_to_gltf/objects.txt` (name, sprite sheet, icons, Ray Dream
   scenes, options) and its directory in `avatars3d/objects/<name>_<scene>/`.
2. Rebuild it alone: `python3 tools/rd4_to_gltf/build_object_library.py --originals
   ../speedyblupi-data/originals --sheets ../mobile-eggbert/Content/icons --objects
   tools/rd4_to_gltf/objects.txt --out build-probe/rd4-objects-probe --only <name>`; look at
   `build-probe/rd4-objects-probe/<dir>/compare.png` (rows: sprite, model, overlap) and the IoU in
   its `icons.txt`.
3. Find the cause in the converter (`tools/rd4_to_gltf/`, every verified Ray Dream convention is
   in its `README.md`), never by hand-editing the model. Probe hypotheses with temporary
   environment switches, measure IoU, revert what does not help, record the experiment in
   `plan.md` (see BB3D-032 for the style).
4. Export only that object: `export_cna_avatar.main(["--glb", <probe>/model.glb, "--cna-home",
   "../../libcna/cna", "--out", "avatars3d/objects/<dir>"])` and copy its `icons.txt` beside it.
   Do NOT run the full `export_objects.py` against a partial library: it deletes every directory
   the library lacks.
5. Check live (`tools/qa_live.sh`, below; world999's exhibition rows z 27-54, x 78-96 hold every
   object), regenerate `plans/inventory/model_parity.md` with `python3 tools/model_parity.py >
   plans/inventory/model_parity.md`, commit.

### BB3D-032 — the fish's head (~15 % large), cosmetic

Every lead has been tested and recorded in `plan.md` under BB3D-032 (jaws' deformer about origin
vs hinge, envelopes, interpolation, jaw scale, the hidden free-form, the body cylinders). No
remaining hypothesis. Leave it unless the owner's D11 list names the fish.

### Left as decided (do not reopen without the owner)

- Procedural textures (palm bark, wood grain, metal noise) stay flat colours: an approximate bake
  was measured worse against the sprite (plan §7.3).
- Objects and vehicles are toned to their sprites (owner's choice): `tone=A,B` per object in
  `objects.txt`, fitted by `tools/rd4_to_gltf/calibrate_tone.py <library> --objects
  tools/rd4_to_gltf/objects.txt`; vehicles by `calibrate_vehicle_tone.py`, applied with
  `export_cna_avatar.py --tone` (commands in `avatars3d/blupi/README.md`). A new object: build
  without a tone, calibrate, add the suggested `tone=`, rebuild, compare. The owner asked for an
  opinion on reverting; the recommendation given was to keep them and soften only an object that
  looks washed out in play (before/after sheets were sent). Revert = `git revert ad46640 a25e8cb`.
- levelcheck reports traps in 052 and 054 that the wasp's balloon actually leaves (not modelled).
- 066's blupit can be walked round (room B1 wider than its line of fire): design weakness, left.
- Questions still waiting for the owner from earlier: the charge (31) and shield as billboards;
  BigDecor at the original's size (2-2.25 blocks) vs the old 1 block.

## 4. Workflows

### Build and test (rules: `../CLAUDE.md`, `CLAUDE.md`)

```bash
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv      # the one shared ccache; never another
cmake --build build-cna-opengl3 --parallel 2
ctest --test-dir build-cna-opengl3 -j2                  # 110 tests, all pass
```

### Web build

Use the existing build tree and the workstation's actual CNA/cna-engine locations:

```bash
source /home/robertvokac/emsdk/emsdk_env.sh
emcmake cmake -S . -B build-cna-webgl2 \
  -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_HOME=/rv/data/development/github.com/libcna/cna \
  -DCNA_ENGINE_HOME=/rv/data/development/github.com/libcna/cna-engine \
  -DCNA_GRAPHICS_RENDERER=WEBGL2 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
cmake --build build-cna-webgl2 --target GalaxyEggbertCNA -j2
```

The deployable files are `build-cna-webgl2/GalaxyEggbert.{html,js,wasm,data}` and the folder
`build-cna-webgl2/Content/music/` beside them (the music is fetched a track at a time, not packed;
OR-0014). Serve them over HTTP; opening the HTML directly as a `file:` URL is not a valid runtime
check.

Only existing build directories (`build-cna-opengl3/`, `build-cna-webgl2/`; `build-probe/` for
probes and converter output). Never build in `/tmp` or the scratchpad. Reconfigure only when broken:
`cmake -S . -B build-cna-opengl3 -DGALAXY_EGGBERT_BUILD_CNA=ON -DCNA_GRAPHICS_RENDERER=OPENGL33
-DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache`.

### Playthroughs (QA autoplay) — the regression net for any gameplay or world change

- `GalaxyEggbert --mission N --autoplay <script>` plays a script deterministically (fixed 1/60
  ticks, draws one frame in 30) and writes `autoplay_trace.txt`; exit 0 = passed. Commands and
  fields are documented in `src/GalaxyEggbert/Game/Autoplay.hpp` (go/jump/tol/until/with, lift,
  crate, clear/near with lane, hold, face, expect, trace, include; fields incl. blitz, temp, crush,
  mtick, keys, eggs, dynamite, phase).
- `tools/autoplay.sh build-cna-opengl3 <script> <mission> [--spawn x,y,z,yaw]` runs one under its
  own Xvfb with a fresh save in `$AUTOPLAY_RUN` (default `build-probe/autoplay-run/`; give each
  parallel run its own directory).
- Scripts: `tools/autoplay/worldNNN.txt` (a mission; a hub's `worldN0.txt` is its world's chain:
  each sign, the mission's script included, back past the bar it opened, the golden key to the main
  hub); `tools/autoplay/campaign.txt` (a new game through all twelve worlds, the gold bars and
  199; regenerate with `python3 tools/autoplay_campaign.py`). `tools/autoplay_all.sh
  build-cna-opengl3 [worldNNN ...]` replays mission scripts; the campaign run is the full replay
  (about two hours of wall time: run it in the background).
- After any change that can touch gameplay or a world: replay the affected missions, and before
  calling a batch done, the campaign.
- Writing or fixing a script (hard-won rules):
  - Write a section, run, read the trace, repeat; `trace N` logs every tick.
  - Lifts: wait on the leading cube's unique end cell (other cubes pass through intermediate
    cells); board at the start of a dwell (wait for the far end, then the near end).
  - Jumps need a run-up and take off at an edge; jumps two up take off close to the wall.
  - Pickups need Blupi within ~0.6. Sticks of dynamite need `hold 2 action` (D59). Keys are
    spent by a door (the whole count clears); eggs stop at ten per mission (MAX_EGG_COUNT).
  - Patrols: `near X Y Z R lane x|z` for the enemy at its far end, then passing heading away.
  - Cycles: sync on the fields (`until temp = 19` then `until temp = 0` for vanishing saucers,
    `blitz` for lightning, `crush` for crushers); never on `tick` (it breaks in chains; use
    `mtick`, ticks since the mission loaded).
  - To find a delay, template it as `wait {D}` and run `python3 tools/autoplay_search.py
    <template> "<mission> [--spawn ...]" <from> <to> <step>`; pick the middle of the passing run.
  - Water: reach the edge, `until y >= surface`, face the bank, `hold 8 up jump`, `hold 30 up`.
- A script that fails after an engine change: check whether the mechanic changed against
  `../mobile-eggbert` before "fixing" the script; a real engine defect gets a test first.

### World changes

Worlds are built by `tools/worlds/worldNNN.py` (writes `worlds3d/worldNNN.vwr`; staged variants
`--stage K out.vwr` for checks). After a change: `python3 tools/levelcheck.py` on the stages the
world's brief (`plans/briefs/worldNNN.md`) names, `ctest -R VerifyShippedWorlds`, `python3
tools/campaigncheck.py`, `python3 tools/watercheck.py worlds3d/world*.vwr`, the mission's
playthrough and a live look; update the brief. Never convert or extrude the 2D maps.

### Live checks

`tools/qa_live.sh build-cna-opengl3 <out> <steps> [--mission N --spawn x,y,z,yaw]` runs under
Xvfb from `build-probe/qa-run/` (never touches the owner's `build-cna-opengl3/savedata.txt`);
steps: `sleep`, `shot NAME`, `key`, `keydown/keyup`, `focus` (Space is Action). `QA_LIVE_AVATARS=`
and `QA_LIVE_WORLDS=` point at alternative asset directories (A/B comparisons with
`git archive <rev> avatars3d`). `tools/qa_sweep.sh build-cna-opengl3` loads every world (79/79).

### Models

Converter: `tools/rd4_to_gltf/` (`README.md` holds every verified Ray Dream convention).
Objects: `build_object_library.py` + `objects.txt` → `export_objects.py` / `export_cna_avatar.py`
→ `avatars3d/objects/` (`avatars3d/objects/README.md`). Blupi and the vehicles:
`build_clip_library.py` → `avatars3d/blupi/` (`avatars3d/blupi/README.md`; rebuilding Blupi is
expensive and risky: verify byte-identical output before replacing anything). Parity:
`tools/model_parity.py`. Converter outputs and probes go to `build-probe/` (`rd4-objects-current/`
is a full library built 2026-10-27 with the current settings).

## 5. Rules that bind every change

- CNA (`../../libcna/cna`) is absolutely read-only: no edits, temporary changes, commits or
  issues. A CNA defect goes to `upstream_bugs.md` (English, UB-NNN). It is a live working tree
  another session edits: rebuild after its header changes before suspecting this repository.
- cna-engine: only justified, policy-compliant fixes, upstream with a test and CHANGELOG, no
  workarounds in Galaxy (owner's rule).
- Never modify `../mobile-eggbert`, `../speedyblupi-data`, `blupi_recon`, `free-eggbert`.
- Every gameplay feature traces to confirmed mobile-eggbert behaviour; fixes only for a reproduced
  bug, focused, with a regression test.
- Commit each discrete task; no AI attribution of any kind in commits or PRs; push only when the
  owner asks. Never `git reset --hard`, `git clean -fd`, forced checkout, destructive rebase.
- At most 2 concurrent subagents; give each its own autoplay run directory and file prefixes.
- The VirtualBox VM `xp` (Ray Dream Studio) must never get network access.

## 6. Sibling repositories (last verified together)

| Repo | Verified commit | Date |
|---|---|---|
| `../../libcna/cna` (live tree; what `build-cna-opengl3` compiles) | `5a754159d` | 2026-11-03 (112/112, wave F's playthroughs, Android APK) |
| `../../libcna/cna` | `090c998a1` | 2026-11-03 (112/112, 78/78 playthroughs, web build) |
| `../../libcna/cna-engine` | `2a93b7e` | 2026-11-03 (same) |
| `../../libcna/easy-gl` (via `CNA_EASYGL_ROOT`) | `b582bd8` (v0.1.1+2) | 2026-11-03 (same) |
| `../../libcna/meta-gl` | `795bcdc` (v0.4.1+2) | 2026-11-03 |
| `../../libcna/cna` | `dda46ec4d` | 2026-11-01 (110/110) |
| `../../libcna/cna-engine` | `d86c62da` | 2026-10-20 |
| `~/deps/cna-baseline` | `900d0db3b` | 2026-10-20 |
| `../../libcna/easy-gl` | `f4d6afaf` (v0.1.1) | 2026-10-20 |
| `~/deps/sharp-runtime-baseline` | `f2aecfef` | 2026-10-20 |
| `../../libcna/meta-gl` | `dd3b3005` (v0.4.1) | 2026-10-20 |

## 7. Gotchas

- `tests/golden/` frames are stale since Phase 7B (CURRENT.md says why).
- The chase camera orbits to Blupi's side after 3 s idle; `--spawn` yaw 0 faces -Z, 90 +X.
- Types 32 and 33: the code's names ("blupih"/"blupit") do not match their icons; check against
  mobile-eggbert before converting.
- Effects (explosions, splashes, sparkles, goo) stay billboards (decision 2026-10-24).
- Part files of exported models are named after their colours (`<name>_rd_rrggbb`); toning
  renames them, and no code refers to those names.
