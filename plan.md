# Galaxy Eggbert — Feature Plan

3D faithful remake of **mobile-eggbert** (C++ port of *Speedy Blupi*, Windows Phone XNA 2013).

**Rule:** Only implement what exists in mobile-eggbert. 3D-specific adaptations
(camera, blob shadows, billboard sprites, auto step-up) are allowed.

---

## Engine & Build

- [x] Urho3D API build (U3D backend, Linux)
- [x] Windows cross-compile (MinGW-w64)
- [x] Web build (Emscripten / WebAssembly)
- [ ] Android build

---

## World & Terrain

- [x] Load world from mobile-eggbert `.txt` format
- [x] 100×100 decor grid rendered as 3D terrain (1 cube per tile)
- [x] Tile textures from `object-m.png` (icon ID = block type)
- [x] 400+ distinct tile IDs render correct textures
- [x] Correct tile passability (decorative tiles → Air, via `table_decor_quart`)
- [x] 5 worlds (Grassland, Forest, Ice Caves, Lava Fields, Space Station)
- [x] Level progression: win → next world, wraps at 5
- [x] Terrain depth fill (cliff edges extrude 3 dark fill blocks downward)
- [x] Sky dome per world (`backgrounds/decorNNN.png`, `region=` from header)
- [x] Per-world sky palette (ambient + fog colours)
- [x] `blupiPos=` header parsed → Blupi spawns at correct world position

---

## Tile Types & Hazards

- [x] Animated tiles: Lava, Crusher, Spike, Saw, Water1, Water2 (cycle at 6 fps)
- [x] Lava (icon 68) — kills on contact
- [x] Spike (icon 373) — kills on contact
- [x] Crusher (icon 317) — kills only during frames 5–9 (fully extended)
- [x] Saw (icon 378–383) — kills on contact
- [x] Shield bypasses all tile hazards and enemy hits

---

## Game Phases & UI

- [x] Init (gamer select: slots 1/2/3)
- [x] Play
- [x] Pause
- [x] Win
- [x] Lost (game over)
- [x] Settings (sound toggle, accessible from Init and Pause)
- [ ] Ranking screen (high-score table, accessible from Init)
- [x] Overlay text renderer (PhaseManager)
- [x] Level intro title card (world name, 3 s fade-in/hold/fade-out)
- [x] Controls hint (auto-fades after 8 s)
- [x] Mouse cursor hidden during Play

---

## HUD

- [x] Life icons (Blupi head sprite, up to 5 + overflow text)
- [x] Gauge sprite (`jauge.png`, bottom-left)
- [x] Treasure counter ("N/total")
- [x] Key icons — red / green / blue, one slot per type
- [x] Shield timer
- [x] World name + elapsed time
- [x] Score
- [x] Game speed indicator (FAST / SLOW label)
- [x] Hit flash (red full-screen overlay, 0.4 s fade)
- [x] Camera shake on hit

---

## Blupi Character

- [x] Physics: gravity, jump, AABB voxel collision
- [x] Controls: arrow keys (forward/turn), Left Ctrl (jump), Left Shift (crouch), Right Shift (look-up / glide)
- [x] Animation: Stop, March, Turn, Jump, Air, Down (crouch), Up (look-up / glide)
- [x] Frame tables ported from `Tables.cpp`
- [x] Glide / suspend: Right Shift in air → reduced gravity, capped fall speed
- [x] Auto step-up: 1-tile ledges climbed automatically *(3D adaptation)*
- [x] Respawn invincibility: 2 s grace period, Blupi flashes
- [x] Death freeze: 1 s input lock after death (explosion plays before regaining control)
- [x] Blob shadow (scans downward, scales with height) *(3D adaptation)*
- [x] Shield tint: cyan sprite when active; blinks at < 1.5 s remaining
- [x] Footstep sound (ch3 per march stride)
- [x] Landing sound (ch4)
- [x] Stomp kill (velY < −1.0 on contact with enemy)
- [x] Bounce after stomp
- [ ] Push mechanic (push ObjectType12 crates)
- [ ] Helicopter boarding (`m_blupiHelico`): ObjectType13 → fly mode
- [ ] Jeep boarding (`m_blupiJeep`): ObjectType19 → drive mode
- [ ] Tank boarding (`m_blupiTank`): ObjectType28 → tank mode
- [ ] Skateboard (`m_blupiSkate`): ObjectType24 → skate mode
- [ ] Balloon / overhead (`m_blupiOver`): ObjectType46 → float mode
- [ ] Swim (`m_blupiNage`) and Surf (`m_blupiSurf`)
- [ ] Suction-cup power-up: ObjectType26 → BlupiAction::Sucette

---

## Enemies

- [x] ObjectType2 — patrol enemy A (table_robot_left)
- [x] ObjectType3 — patrol enemy B
- [x] ObjectType4 — bulldozer (table_bulldozer_left)
- [x] ObjectType16 — spider (vertical oscillation: hang ↔ drop)
- [x] ObjectType17 — fish (patrol, table_poisson_left)
- [x] ObjectType20 — bird (aerial patrol, y = 3.0)
- [x] ObjectType33 — blupit tank (table_blupit_left)
- [x] Patrol movement: posStart ↔ posEnd; stationary gets ±2 tile default range
- [x] Stomp kills all enemy types; enemy respawns at posStart after 5 s
- [x] Directional sprites (flipX when moving right)
- [x] Blob shadow under enemies (disabled for birds) *(3D adaptation)*
- [x] Y-proximity check: aerial enemies don't hit ground-level Blupi
- [ ] ObjectType23 — fired projectile (enemies shoot at Blupi)
- [ ] ObjectType96 / 97 — follow enemies (chase Blupi)

---

## Pickups & Collectibles

- [x] ObjectType1 — platform lift (moves posStart ↔ posEnd, carries Blupi)
- [x] ObjectType47 / 48 — platform lift variants (rightward / leftward carry)  *(not yet, minor)*
- [x] ObjectType5 — treasure (+10 score; all required to open exit)
- [x] ObjectType6 — egg (+1 life, cap 9)
- [x] ObjectType7 — exit (triggers Win when all treasures collected)
- [x] ObjectType12 — crate (static decoration)
- [x] ObjectType13 — helicopter pickup (placeholder: grants shield; full boarding `[ ]` above)
- [x] ObjectType25 — shield orb (5 s invincibility)
- [x] ObjectType30 — drink (+1 life, cap 9)
- [x] ObjectType49 / 50 / 51 — red / green / blue keys (+50 score; shown in HUD)
- [x] Exit locked until all ObjectType5 collected; "EXIT OPEN!" popup + sparkle when unlocked
- [x] Bonus life when all treasures collected (once per level)
- [x] Pickup bobbing (sine-wave Y offset) *(3D visual)*
- [x] Score popups (rising text, 1 s fade) *(3D visual)*
- [ ] ObjectType19 — jeep pickup (full boarding above)
- [ ] ObjectType21 — secret exit
- [ ] ObjectType24 — skateboard pickup
- [ ] ObjectType26 — suction-cup power-up
- [ ] ObjectType28 — tank pickup
- [ ] ObjectType29 — bullet ammo
- [ ] ObjectType31 — cloud power-up
- [ ] ObjectType40 — invert power-up
- [ ] ObjectType46 — balloon pickup
- [ ] ObjectType55 — dynamite

---

## Score & Progression

- [x] Score: +10 treasure, +25 stomp, +50 key / egg / drink, +100 all-treasures bonus
- [x] High score per gamer slot (persisted in GameData bytes 2–5)
- [x] Level elapsed timer (HUD + Win overlay)
- [x] Game speed selector: G key cycles Slow (0.6×) → Normal (1.0×) → Fast (1.5×)

---

## Sound

- [x] SoundManager: 93 WAV files from `Content/sounds/`
- [x] Per-channel volume from `tableVolumePitch`
- [x] Sound on/off toggle (persisted in GameData)
- [x] Channels wired: jump ch1, footstep ch3, landing ch4, stomp ch5, death ch8, collect ch10, key ch11, life ch42, shield-off ch44, exit ch57

---

## Save Data

- [x] GameData: 640-byte binary format, binary-compatible with mobile-eggbert
- [x] 3 gamer slots: lives, last world, door states
- [x] High score extension (gamer-header bytes 2–5, was reserved)
- [x] Auto-save on win / lost / quit / reset / gamer-select

---

## Camera *(3D-specific)*

- [x] 3rd-person orbit (RMB pitch, scroll zoom, auto-yaw follow)
- [x] Smooth zoom (lerp)
- [x] Pitch auto-reset to 20° when RMB released
- [x] Wall collision (DDA ray march from Blupi to desired position)
- [x] FOV 65°

---

## Engine backend

- [x] Simple3D (via U3D/Urho3D) — active, primary target `GalaxyEggbertSimple3D`
- [ ] Nova3D (Urho3D fork in progress; Simple3D selects it via `-DSIMPLE3D_ENGINE=NOVA3D`)

---

## Simple3D Migration

Galaxy Eggbert runs entirely on the `simple-3d` framework (target: `GalaxyEggbertSimple3D`).
The legacy direct-Urho3D target has been removed (S3D-9).
Build with: `cmake -S . -B cmake-build-simple3d -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON`

- [x] S3D-1 — Simple3D port skeleton: app entry, world loading, placeholder terrain, Blupi CharacterController, basic HUD labels, orbit camera, minimal sound, CMake target, gap documentation
- [x] S3D-2 — Terrain visual fidelity: tile atlas material + UV offset per block type
- [x] S3D-3 — Blupi sprite/billboard animation from `blupi.png`
- [x] S3D-4 — Decor object visuals: enemy + pickup billboard sprites from `element.png`
- [x] S3D-5 — HUD images: gauge sprite, life icons, key icons, hit flash panel
- [x] S3D-6 — Phase/menu port: Init gamer select with per-slot data, Settings screen (sound toggle), SaveData persistence
- [x] S3D-7 — Sound channel parity: 93 channels via SoundChannel enum, per-channel volume from tableVolumePitch, no-restart policy, key/life/shield-off events wired
- [x] S3D-8 — Web build verified: GalaxyEggbertSimple3D.html builds with Emscripten; NetworkManager stub for web; Android blocked (U3D no Android support)
- [x] S3D-9 — Remove legacy Urho3D path after Simple3D version reaches playable parity
