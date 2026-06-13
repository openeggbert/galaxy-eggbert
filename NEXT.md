# Galaxy Eggbert — Development Roadmap

Galaxy Eggbert is a faithful 3D remake of Speedy Blupi, using mobile-eggbert
(`/rv/data/development/github.com/openeggbert/mobile-eggbert`) as the primary reference.
All gameplay logic, level data, enums, sounds, and textures originate from mobile-eggbert
or the original Windows Phone game.

---

## Current state (as of Phase 63)

**Working:**
- World loaded from `worlds/world001.vwr` at runtime; demo world saved on first run
- Terrain rendered from `World` data model; tiles UV-mapped from `object-m.png`
- Blupi: sprite billboard from `blupi.png` (10-col × 34-row sheet, 60×60 tiles)
  - Full animation state machine: Stop, March, Turn, Jump, Air
  - Frame tables ported from mobile-eggbert `Tables.cpp`
  - Physics: gravity, jump, AABB voxel collision (derived from `Decor.cpp`)
  - Controls: WASD move/strafe, Q/E or L/R arrows turn, SPACE jump (arrow keys also work)
- `Decor` object pool (up to 100 objects), all animated via `GetIcon()` phase tables:
  - ObjectType2/3 (patrol enemies A/B), ObjectType16 (spider)
  - ObjectType5 (treasure), ObjectType6 (egg), ObjectType7 (exit)
  - ObjectType49/50/51 (red/green/blue keys) — key collection tracked, shown in HUD
  - ObjectType25 (shield orb) — grants 5 s invincibility, shown in HUD as countdown
  - Tile hazards: `Lava` (icon 68), `Spike` (icon 373), `Crusher` (icon 317) block types
    — kill Blupi on contact unless shield is active; shield bypasses enemy hits too
- `GamePhase` state machine: Init → Play → Pause / Win / Lost
  - Init, Pause, Win, Lost phases each show a fullscreen background overlay
- Camera: smooth 3rd-person orbit; RMB pitch, scroll zoom, auto-yaw follow
- HUD: lives, treasures, keys, shield timer, `jauge.png` gauge sprite (bottom-left)
- `SoundManager`: loads all 93 WAV files; per-channel volume from `tableVolumePitch`
  - Wired: jump (ch1), collect/treasure (ch10), key (ch11), shield (ch42), death (ch8), exit (ch57)
- `GameData`: 640-byte save file, binary-compatible with mobile-eggbert
  - Stores lives, last world reached, door states (3 gamer slots)
  - Loaded at startup, written on win/lost/quit/reset
- Level progression: `LoadWorld(N)` loads `worlds/worldNNN.vwr`; `AdvanceToNextWorld()` transitions
  win → next world without full reset; wraps at world 5; terrain cleared via `terrainRoot_` node
  - Per-world difficulty: +1 Crusher tile and +1 patrol enemy per world level
  - HUD shows current world number: `"World N | Lives: N  Treasures: N  Keys: N"`
- Gamer-select screen at Init phase: keys 1/2/3 choose one of 3 save slots; shows
  lives/world/doors per slot; text rendered on overlay via `PhaseManager::SetOverlayText()`
- Settings screen (`GamePhase::MainSetup` / `PlaySetup`, `backgrounds/setup.png`):
  sound on/off toggle (S key), persisted to save, applied at startup; ESC returns to caller
  - Accessible via S key from Init (main menu) or Pause
  - `SoundManager::SetEnabled(bool)`: mutes all channels; `Play()` is no-op when disabled
- 54 unit tests pass for `Worlds/` data model (engine-independent)
- Windows build: `GalaxyEggbert.exe` (PE32+, x86-64) via MinGW-w64 cross-compile
  - `build-windows/` configured with `cmake/toolchains/mingw-w64.cmake` + U3D Windows build
  - Statically linked: `-static-libgcc -static-libstdc++`; Windows system libs include `iphlpapi`
- HUD sprite icons: life icons (Blupi head, `blupi.png` icon 48) and key icons (`element.png` icon 215)
  replace "Lives: N / Keys: N" text; both shown as `BorderImage` sprites inside/near the gauge
- HUD treasure total: shows "Treasures: N/total" — total counted by `Decor::GetTotalTreasures()`
- HUD world names: `WorldName(N)` lookup — "Grassland", "Forest", "Ice Caves", "Lava Fields", "Space Station"
- Real levels: `LoadMobileEggbertTerrain()` parses mobile-eggbert `.txt` world files
  - `worlds/world00N.txt` (from mobile-eggbert world01N.txt) loaded automatically when present
  - Terrain built from 100×100 Decor grid; tile IDs mapped via `BlockTypes::fromMobileIconId()`
  - MoveObjects parsed: types 1–7,12,13,16,17,20,25,30,33,49–51 placed as Decor objects
  - Patrol enemies with posStart==posEnd get default ±2 tile X patrol range
  - 64px tile size for all position conversions (pixel → tile = px/64)
  - `blupiPos=` header parsed → stored as `blupiSpawn_`; Blupi spawns at correct world position
  - Fall-through-floor respawn uses `spawn_` (no longer hardcoded to origin)
- ObjectType4 (bulldozer) and ObjectType20 (bird) added as patrol enemies
- ObjectType17 (fish): patrol enemy using `table_poisson_left` icons 81–83
- ObjectType1 (platform lift): moves between posStart↔posEnd carrying Blupi
  - `Decor::GetPlatformDelta()` returns XZ carry delta; applied via `Blupi::ApplyExternalDelta`
  - Carry activated when Blupi is within 0.85 units horizontally and 1.5 units vertically
- `ObjectNode` tile dimensions fixed: 60×60 px tiles, 10 cols (was wrong 64×9)
- `BlockTypes` redesigned: block type = icon ID; every mobile-eggbert tile now renders
  its actual texture from `object-m.png` instead of falling back to grass
  - `fromMobileIconId` is now trivial: `icon > 0 ? icon : Air`
  - `toIconIndex` is trivial: `t == Air ? -1 : t`
  - All 400+ distinct tile IDs across the 5 world files render correctly
- ObjectType13 (helicopter pickup): static icon 68; collecting grants shield (boarding placeholder)
- ObjectType30 (drink pickup): static icon 178; collecting counts as treasure
- Per-world sky palette: ambient + fog colours change per world (Grassland green →
  Forest dark → Ice Caves blue-white → Lava Fields red → Space Station near-black)
- Correct tile passability: 203 decorative tile IDs (from `table_decor_quart`) become
  Air instead of solid blocks, removing invisible walls in real levels
  - `BlockTypes::isMobileTransparent(icon)` precomputed bool[441] lookup
  - Icons 68 (Lava) and 317 (Crusher) kept solid despite being quart-passable
- Sky dome: `DiffSkydome.xml` sphere (500 units) loaded from `backgrounds/decorNNN.png`
  per world; region parsed from `region=` in .txt header (`postopaque`, no depth write)
- Strafe movement: A/D keys strafe Blupi left/right without rotating
- ObjectType33 (blupit tank): `table_blupit_left` icons 248-250; patrol enemy, kills on contact
- Fall-death life deduction: falling off map now deducts a life (previously free respawn)
- Respawn invincibility: 2 s grace period after any respawn; tile hazards and enemy hits
  are skipped while `respawnInvincibleTimer_ > 0`
- `kMaxObjects` increased from 50 → 100 (worlds 3–5 have up to 57 objects)
- Blupi flashes (billboard toggles every 0.1 s) during 2 s post-respawn invincibility window
- Landing sound: `SoundChannel4` plays when Blupi transitions from airborne → ground
- ObjectType12 (crate): static decoration, element.png icon 32; now placed in world 4 (2 instances)
- Full WASD+QE controls: W/S move forward/back (same as UP/DN), Q/E turn left/right (same as L/R)
- Controls hint auto-fades after 8 s; `controlsHintTimer_` reset on every level load/select
- Bonus life when all treasures collected (once per level, capped at 9 lives); plays SoundChannel42
- Camera wall collision: DDA ray march from Blupi to desired camera pos; clamps to first solid voxel
  - `SetCollisionWorld(World*, wcx, wcz)` wired in Start() and LoadWorld()
- Pause screen shows world name, lives, and key hints via `phases_->SetOverlayText()`
- F1–F5 debug world jump: instantly teleports to world 1–5 during play; full state reset
- Terrain depth fill: edge tiles (any horizontal air neighbour) get 3 dark fill blocks extruded
  below them (`Color(0.22, 0.19, 0.17)`); cliffs now appear solid from any camera angle
- Enemy stomp: falling Blupi (velY < -1.0) kills enemy on contact; plays SoundChannel5 (bounce);
  `Blupi::Bounce()` gives upward impulse (0.6 × kJumpSpeed); enemy removed from pool
  - All 7 enemy types stompable: ObjectType2/3/4/16/17/20/33
  - `Decor::WasStompKill()` / `stompKill_` flag; cleared in ClearEvents()
- Win overlay shows completed world name, treasures collected/total, and lives remaining;
  Lost overlay shows world name where Blupi died; both set via `phases_->SetOverlayText()`
- ObjectType20 (bird): placed at y=3.0 in real levels so birds fly visually above terrain
- `Decor::TouchesBlupi()` checks Y proximity (`std::abs(dy) < 1.5f`) so aerial birds
  do not hit Blupi on the ground; all ground enemies (y=1.0) remain unaffected
- ObjectType6 (egg) grants +1 life (cap 9) when collected; plays SoundChannel42;
  `Decor::eggCollected_` / `WasEggCollected()` flag distinct from generic `collected_`
- Pause overlay now shows treasure progress: `Lives: N   Treasures: X/Y`
- Win overlay shows `"ALL WORLDS COMPLETE!"` header when completing world 5 (kMaxWorld);
  all other worlds show `"LEVEL COMPLETE!"`
- ObjectType30 (drink) grants +1 life (cap 9) when collected; `Decor::drinkCollected_` flag,
  same award logic as egg; drink still increments HUD `collected_` counter
- ObjectType16 (spider) oscillates vertically (y=4 hang → y=1 drop) in real levels;
  `StepMovement` stationary check now requires all 3 axes equal (Y clamp removed);
  spider is dangerous only when descended (Y proximity check prevents aerial ghost hits)
- Debug octree toggle moved from F1 → F12; F1–F5 world-jump now fires correctly in Play phase
- Blupi billboard tinted cyan (`Color(0.5, 0.85, 1.0)`) when shield is active; resets to white
  when timer expires; `Blupi::SetShieldActive(bool)` wired from `GalaxyEggbertGame::UpdatePlay()`
  after each timer decrement
- HUD lives overflow: `livesOverflow_` Text element shows `"xN"` in amber when lives exceed the
  5 icon slots; hidden when lives ≤ 5; cleared in `SetVisible(false)`
- Treasure counter: `Decor::collected_` now counts only ObjectType5; eggs (type6) and drinks
  (type30) no longer inflate it — they award lives via separate flags; HUD "X/Y" display and
  bonus-life check both now reflect true treasure-only progress; double-sound on egg/drink fixed
- Camera pitch auto-reset: when RMB is not held, pitch exponentially decays toward
  `kDefaultPitch=20°` at rate `2×dt`; players no longer get stuck looking up/down
- Per-type key icons: `Decor` tracks `keysType49_/50_/51_` separately; `ShowPlay` takes
  `keys49,keys50,keys51`; HUD slot 0=red(icon 209), slot 1=green(icon 220), slot 2=blue(icon 229);
  each slot visible only when that specific key type was collected
- Camera zoom smoothed: `targetDist_` accumulates scroll input; `dist_` lerps toward it at
  rate `8×dt`; zoom feels responsive but not jumpy
- Mouse cursor hidden in Play phase, restored in all other phases (Pause/Win/Lost/Init/Settings)
  and on Stop(); `EnterPhase` calls `input->SetMouseVisible(bool)` per phase
- Camera initial pitch changed from 25° to `kDefaultPitch` (20°) so there is no auto-reset
  drift at game start
- Pause overlay shows collected key types ("Keys: Red Green Blue") when any are held;
  line omitted entirely when no keys collected
- `SelectGamer()` now resets all per-level state (`prevCollected_`, `keysRed/Green/Blue_`,
  `shieldTimer_`, `respawnInvincibleTimer_`) matching `AdvanceToNextWorld`; previously these
  persisted when switching gamer slots mid-session (bug)
- Fall death plays `SoundChannel8` (same as tile hazard / enemy hit); previously silent
- Camera FOV set to 65° (was default 45°); wider view suits 3rd-person platformer
- Web (Emscripten/WebAssembly) build (Phase 46): `build-web/GalaxyEggbert.html` + `.wasm` (3.4 MB) + `.data` (48 MB preloaded content);
  emsdk 3.1.60 at `/rv/data/library/emsdk`; U3D built for web at `/rv/data/library/github.com/u3d-community/U3D/build-web/lib/libUrho3D.a`;
  CMakeLists.txt: removed FATAL_ERROR for Emscripten in U3D branch, added `build-web` as default URHO3D_HOME for EMSCRIPTEN,
  added `-lembind` + content `--preload-file` link options; GEEngine.hpp: pre-includes Bullet headers to complete types before
  Urho3DAll.h; GalaxyEggbertApp: explicit `~GalaxyEggbertApp()` dtor defined in .cpp to avoid incomplete `unique_ptr` type;
  U3D Ptr.h `CheckedDelete` sizeof check removed (upstream Clang 19 compat fix); Linux + Windows builds unaffected
  - Web build command: `. /rv/data/library/emsdk/emsdk_env.sh && emcmake cmake -S . -B build-web -DGALAXY_EGGBERT_ENGINE=U3D -DBUILD_TESTING=OFF -DWEB=1 && cmake --build build-web --target GalaxyEggbert -j2`
- Control scheme redesign (Phase 45): WASD/QE/strafe removed; pure arrow-key movement/turn;
  Left Ctrl = jump; Space = action (placeholder); Left Shift = crouch (BlupiAction::Down,
  freezes at icon 35); Right Shift = look up (BlupiAction::Up, icon 44); crouching/looking-up
  locks horizontal movement; Tables::GetBlupiIcon extended with Down(3-frame, maxPhase=2)
  and Up(1-frame) entries from mobile-eggbert table_blupi; Pause overlay now shows score+time;
  HUD hint updated to reflect new bindings
- Enemy respawn: stomped enemies (types 2/3/4/16/17/20/33) are hidden and respawn at posStart
  after 5 s (`kRespawnDelay`); `Decor::Object::respawnTimer` counts down while `active=false`;
  `ObjectNode::SetVisible(bool)` toggles `node_->SetEnabled()` without destroying the scene node;
  pickup types (treasure, egg, key, shield, drink) are still permanently removed on collection
- Score system: `score_` (persistent across level transitions, reset on full restart/slot select);
  awards: +10 treasure, +25 stomp kill, +50 key, +50 egg/drink, +100 all-treasures bonus life;
  shown in HUD text (`"Score: N"`), Win overlay, and Lost overlay (Game Over screen)
- Level elapsed timer: `levelTime_` (per-level, resets in AdvanceToNextWorld/ResetLevel/SelectGamer/F-key jump);
  shown in HUD as `"M:SS"` alongside treasures; also displayed in Win overlay (`"Time: M:SS"`);
  timer stops accumulating the moment the Play phase exits (frozen in Win overlay)
- Animated tiles (Phase 47): Lava, Crusher, Spike, Saw, Water1, Water2 all cycle through
  their mobile-eggbert icon sequences (6–16 frames at 6 fps); driven by `totalTime_`;
  `BlockTypes::tileAnimBase()` maps all icons in an animation group to one shared material;
  UV offsets updated per frame via `SetShaderParameter("UOffset"/"VOffset")`; replaces
  old color-pulsing; Saw (icon 378–383) added as 4th hazard tile; `BlockTypes::isHazard()`
  centralises the hazard check (Lava/Spike/Crusher/Saw)
- Blupi proportions corrected (Phase 48): `kHalfH = 23/64 ≈ 0.359` (block=64px, Blupi=46px);
  billboard visual size `60/64 = 0.9375`; camera look-at Y offset 0.35; ObjectNode same scale
- Pickup bobbing (Phase 49): pickups (treasure/egg/exit/keys/shield/drink) oscillate on a
  sine-wave Y offset `0.12·sin(totalTime·2.5 + i·1.5)`; enemies/platforms stay flat
- Camera shake + red screen flash (Phase 50): `CameraController::StartShake(intensity,duration)`
  applies decaying oscillating XY offset to camera position; `HUD::ShowHitFlash()` shows
  a full-screen red `BorderImage` at 50% alpha fading to 0 over 0.4 s; both triggered on
  fall death, tile hazard hit, and enemy contact; `HUD::Update(dt)` ticks the flash
- Death explosion animation (Phase 51): `Explosion` class plays `table_explo1` (39 frames,
  icons 0–11) from `explo.png` (1440×1440, 24 cols × 24 rows of 60×60 tiles) at 12 fps
  as a 1.5-unit BillboardSet at the death position; spawned on all 3 death paths; auto-
  destroys when done; reset on `LoadWorld()` and `Stop()`
- Footstep sound (Phase 52): `SoundChannel3` plays once per march stride (`animTick_ % 18 == 1`
  at 60 fps, matching 6-frame × 3-tick cycle); stomp kill also spawns explosion at enemy position
- Score popup system (Phase 53): `ScorePopup` class — `Text3D` node with `FC_ROTATE_XYZ`,
  font 24 pt; rises 1.5 units/s and fades alpha over 1 s; `popups_` vector in game auto-erased
  when done; `score_` persists across level transitions; awards: +10 treasure, +25 stomp,
  +50 key/egg/drink, +100 all-treasures bonus
- Enemy facing direction (Phase 54): `Object::facingLeft` updated each frame from XZ delta;
  sprites face left by default (mobile-eggbert convention); `ObjectNode::UpdateIcon(icon, flipX)`
  swaps U0↔U1 when moving right; directional types: 2/3/4/17/20/33
- Shield expiry feedback (Phase 55): `SoundChannel44` plays on shield expiry + "SHIELD OFF" cyan
  `ScorePopup`; shield pickup shows "SHIELD!" cyan popup; egg/drink show "+1 LIFE!" green popup
- Blupi billboard visual offset (fix): `bb->position_ = (0, kVisHalf−kHalfH, 0)` ≈ +0.11 units
  upward; aligns sprite bottom with physics feet — was clipping into block surface
- Minecraft-style demo world (Phase 56): flat ground plane (R=18), raised hill (h=1–3 east),
  staircase up from west, elevated platform across north (h=2), stone tower NW (h=4),
  lava pit with stepping stones SW, spike strip south, crusher corridor east, water channel west,
  border walls; extra Crusher tiles added per world difficulty
- Moving platform vertical landing (Phase 57): `Decor::GetPlatformLandY()` exposes platform top
  surface Y; `Blupi::SnapToSurface(float)` snaps feet when within [−0.4, +0.3] of surface and
  not jumping up; wired in `UpdatePlay()` after platform carry delta; `StepMovement` Y-snap fix:
  `obj.pos = target` (was only snapping X/Z, leaving Y behind for spider/vertical movers)
- Shield warning (Phase 58): when `shieldTimer_ < 1.5f` Blupi tint blinks rapidly between cyan
  and white (0.15 s intervals via `shieldBlinkPhase_`); HUD shield text colour turns orange
  (`Color(1.0, 0.55, 0.1)`) at < 1.5 s remaining; restores blue-white when shield expires
- Sprite embedding fix: `FC_ROTATE_XYZ` → `FC_ROTATE_Y` for Blupi and ObjectNode billboards;
  with XYZ the billboard tilts with camera pitch so vertical size.y is applied in camera-up
  direction, causing the sprite bottom to clip into terrain at the default 20° pitch; `FC_ROTATE_Y`
  keeps the sprite upright so size.y is a pure world-Y offset — bottom aligns at block top ✓
- Death respawn freeze (Phase 59): `Blupi::inputFrozen_` / `SetInputFrozen(bool)` suppresses
  all movement for 1 s after any death (`deathFreezeTimer_` in game); animation still ticks;
  lets the death explosion play before the player regains control; reset on level transitions
- Crusher timing (Phase 60): `BlockTypes::Crusher` is only lethal during frames 5–9 of its
  10-frame 6-fps cycle (icons 321–323, crusher fully down); frames 0–4 (317–320, retracted/
  rising) are safe to stand on — `crusherSafe` bool computed from `totalTime_` before kill check
- ObjectNode vertical offset (Phase 60): `bb->position_ = (0, kVisHalf−0.5, 0)` ≈ −0.031 units;
  aligns sprite bottom with block top surface (node Y=1.0, block top Y=0.5, visHalf=60/128)
- Variable jump height (Phase 63): `kMinJumpSpeed = kJumpSpeed × 0.30` — releasing Left Ctrl while
  still ascending cuts `vel_.y_` to `kMinJumpSpeed`; full hold gives maximum arc, tap gives a
  short hop ~30% as high; `jumpHeld_` flag cleared on landing or on button-release; reset in
  `SpawnAt()`; pairs with coyote/buffer from Phase 62 for complete platformer-feel jump system
- Coyote time + jump buffer (Phase 62): `kCoyoteTime=0.12s` grace period after walking off an edge
  where jump still fires; `kJumpBuffer=0.12s` queued jump fires on the landing frame if pressed
  just before touching ground; both implemented entirely in `Blupi::Update()` — no game-coordinator
  changes needed; `coyoteTimer_/jumpBuffer_` reset in `SpawnAt()`; improves platformer feel
  significantly, especially on narrow platforms and moving-platform hops
- Multiple simultaneous explosions (Phase 61): `explosions_` vector replaces single `explosion_`
  unique_ptr; all 3 death paths + stomp push_back; ticked with `remove_if(!e->Update(dt))`
- Pickup sparkle (Phase 61): `Explosion` constructor takes optional `float scale = 1.0f`;
  `bb->size_ = Vector2(1.5*scale, 1.5*scale)`; treasure/key/egg/drink collection spawns
  a scale-0.4 mini-explosion alongside the score popup; death explosions remain scale-1.0

- **Refactor (Phase 39):** `WorldName()` file-scoped helper in GalaxyEggbertGame.cpp replaces
  3× duplicated `kWorldNames[]` array in UpdatePause/Win/Lost; `keys49_/50_/51_` renamed to
  `keysRed_/keysGreen_/keysBlue_`; Decor.hpp comment updated to list all 15 supported types;
  GalaxyEggbertGame.hpp state section split into "persistent" vs "per-level" groups

**Architecture (subsystem classes):**
```
src/GalaxyEggbert/Game/
  Blupi.hpp/.cpp          — physics, animation, sprite billboard       ✅
  Camera.hpp/.cpp         — 3rd-person orbit camera                    ✅
  Decor.hpp/.cpp          — object pool, patrol movement, collision     ✅ (partial)
  GameData.hpp/.cpp       — 640-byte save format (mobile-eggbert compat)✅
  HUD.hpp/.cpp            — lives, keys, shield, gauge sprite           ✅ (partial)
  ObjectNode.hpp/.cpp     — single object billboard in Urho3D scene     ✅
  PhaseManager.hpp/.cpp   — GamePhase state + overlay transitions       ✅
  SoundManager.hpp/.cpp   — 93-channel WAV audio with volume table      ✅
  Tables.hpp/.cpp         — animation frame tables (from Tables.cpp)    ✅
```

**Not yet done:**
- Vehicles and advanced object types (helicopter, jeep, skateboard, bulldozer — player-mode state machine)
- Animated 3D model for Blupi (currently billboard placeholder)
- Android and Web (Emscripten) platform builds

---

## Architecture plan — class and file structure

Inspired by mobile-eggbert's layering (`Game1` → `Decor` → `Pixmap`, `Tables`, `GameData`).
Galaxy-eggbert maps the same concerns to Urho3D 3D classes:

```
src/GalaxyEggbert/
  GalaxyEggbertApp.hpp/.cpp     — Urho3D Application subclass; wires lifecycle + event loop
  GalaxyEggbertGame.hpp/.cpp    — top-level coordinator; owns scene, world, subsystems
  Game/
    Blupi.hpp/.cpp              — Blupi character: physics, input, collision      ✅
    Camera.hpp/.cpp             — 3rd-person camera logic                         ✅
    Decor.hpp/.cpp              — object pool, enemies, events                    ✅ (partial)
    GameData.hpp/.cpp           — save data persistence                           ✅
    HUD.hpp/.cpp                — 2D overlay: lives, keys, shield, gauge          ✅ (partial)
    ObjectNode.hpp/.cpp         — Urho3D scene node for one moving object/enemy   ✅
    PhaseManager.hpp/.cpp       — GamePhase state machine + overlay transitions   ✅
    SoundManager.hpp/.cpp       — wraps Urho3D audio; maps SoundChannel → WAV     ✅
    Tables.hpp/.cpp             — animation frame tables (port Tables.cpp)        ✅
  World/
    (= current include/GalaxyEggbert/Worlds/ — keep engine-agnostic)             ✅
```

---

## Phase 15 — Windows build (MinGW cross-compile)

Status: **DONE** — `build-windows/GalaxyEggbert.exe` (PE32+, x86-64, 22 MB).

Build commands (for reference):
```bash
# U3D Windows library (already built at build-windows/lib/libUrho3D.a):
make -C /rv/data/library/github.com/u3d-community/U3D/build-windows -j2 Urho3D

# Galaxy Eggbert Windows executable:
cmake -S . -B build-windows \
      -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
      -DGALAXY_EGGBERT_ENGINE=U3D \
      -DU3D_HOME=/rv/data/library/github.com/u3d-community/U3D/build-windows \
      -DBUILD_TESTING=OFF
cmake --build build-windows --target GalaxyEggbert -j2
```

---

## Phase 63 — Variable jump height

Status: **DONE**

- `Blupi::kMinJumpSpeed = kJumpSpeed * 0.30f = 3.0f`: minimum arc when jump released early
- `jumpHeld_`: set to true when jump fires; cleared when either `onGround_` again or
  jump button released (at which point `vel_.y_` is clamped to `kMinJumpSpeed` if still positive)
- Holding Left Ctrl for the full press gives full `kJumpSpeed` arc; tapping gives a short hop
- Complementary to Phase 62 coyote/buffer: together they form the standard 3-feature
  responsive jump system (coyote + buffer + variable height)

---

## Phase 62 — Coyote time + jump buffer

Status: **DONE**

- `Blupi::kCoyoteTime = 0.12f` / `kJumpBuffer = 0.12f`: constants for the two grace windows
- `coyoteTimer_`: refreshed to `kCoyoteTime` every frame while `onGround_`; counts down when
  airborne; set to 0 when jump fires; reset in `SpawnAt()`
- `jumpBuffer_`: set to `kJumpBuffer` on `GetKeyPress(LCTRL)`; counts down each frame;
  consumed (set to 0) when jump fires; reset in `SpawnAt()`
- Jump now fires when `jumpBuffer_ > 0 && coyoteTimer_ > 0` — covers on-ground, coyote, and
  buffer-on-landing cases in one unified condition; replaces the old single-line
  `if (onGround_ && KeyPress)` check
- No changes outside `Blupi.hpp/.cpp` — self-contained platformer-feel improvement

---

## Phase 60 — Crusher timing + ObjectNode vertical offset

Status: **DONE**

- `GalaxyEggbertGame::UpdatePlay()`: `crusherSafe = (bt == Crusher && (int)(totalTime_*6)%10 < 5)`;
  `isHazard(bt) && !crusherSafe` — crusher kills only during frames 5–9 (extended position);
  frames 0–4 (retracted/rising, icons 317–320) are passable; matches mobile-eggbert behavior
  where the timing window lets Blupi run through the corridor without dying
- `ObjectNode::ObjectNode()`: `bb->position_ = Vector3(0, kVisHalf - 0.5f, 0)` ≈ −0.031 downward;
  objects placed at node Y=1.0 previously had sprite bottom at Y=0.531 (floating above block top
  0.5); offset corrects this so feet land exactly on the surface; consistent with Blupi's offset
  formula `kVisHalf - kHalfH` (same math, different height above block top)

---

## Phase 59 — Death respawn freeze + FC_ROTATE_Y sprite fix

Status: **DONE**

- `Blupi::inputFrozen_` / `SetInputFrozen(bool)`: when true, `Update()` skips all movement and
  physics, runs only `++animTick_` and `UpdateSprite()` + shield blink; prevents re-death during
  the 1 s freeze window (respawnInvincibleTimer_ also active)
- `GalaxyEggbertGame::deathFreezeTimer_` (per-level): set to 1.0 s after all 3 death paths
  (fall, tile hazard, enemy hit); counted down at top of `UpdatePlay()`; `SetInputFrozen(false)`
  on expiry; reset in `AdvanceToNextWorld / ResetLevel / SelectGamer`
- Sprite embedding root cause: `FC_ROTATE_XYZ` tilts the billboard to fully face the camera
  including pitch; at default 20° pitch the billboard bottom swings forward in Z and clips into
  terrain block front faces; fixed by switching Blupi and ObjectNode to `FC_ROTATE_Y` (keeps
  sprite upright, size.y = pure world-Y offset, bottom lands at block top regardless of pitch)
- `Decor::StepMovement` Y-snap bug also fixed (Phase 58 commit): `obj.pos = target` replaces
  `obj.pos.x_=target.x_; obj.pos.z_=target.z_` — spider vertical oscillation now correct

---

## Phase 58 — Shield warning blink + HUD colour change

Status: **DONE**

- `Blupi`: added `shieldWarning_` bool + `shieldBlinkPhase_` float + `SetShieldWarning(bool)`;
  `shieldBlinkPhase_` accumulates dt while shield is active; `UpdateSprite()` alternates tint
  between `Color(0.5, 0.85, 1.0)` and `Color::WHITE` every 0.15 s when `shieldWarning_` is true
- `HUD::ShowPlay()`: shield text `SetColor(Color(1.0, 0.55, 0.1))` when `shieldSecs < 1.5f`;
  `Color(0.85, 0.90, 1.0)` otherwise; colour restored to normal after shield expires
- `GalaxyEggbertGame`: `blupi_->SetShieldWarning(shieldTimer_ > 0 && shieldTimer_ < 1.5f)`
  called alongside `SetShieldActive()` after each frame's timer decrement
- `Decor::StepMovement` Y-snap bug fixed: `obj.pos = target` (was `obj.pos.x_ = target.x_;
  obj.pos.z_ = target.z_;` — left Y unchanged when reaching a waypoint, breaking spider vertical
  oscillation and any future vertical-moving platform)

---

## Phase 57 — Moving platform landing (SnapToSurface) + improved demo objects

Status: **DONE**

- `Decor::platformLandY_`: set to `obj.pos.y_ + 0.5f` each frame when platform is under Blupi;
  exposed via `GetPlatformLandY()`; reset to −999 at start of each Update
- `Blupi::SnapToSurface(float surfaceY)`: if `vel_.y_ ≤ 0` and feet within [surfaceY−0.4,
  surfaceY+0.3], snaps `pos.y_ = surfaceY + kHalfH`, zeroes `vel_.y_`, sets `onGround_ = true`
- `UpdatePlay()`: `float landY = decor_->GetPlatformLandY(); if (landY > −900) blupi_->SnapToSurface(landY)`
  called after platform carry delta; keeps Blupi standing on moving platforms
- `CreateDemoObjects()`: updated object placements to match Phase 56 terrain heights;
  moving platform E-W on flat area (Y=1), treasures at h=0/2/3 heights, bird over hill (Y=4),
  spider vertical drop near tower, exit at hilltop (Y=4)

---

## Phase 56 — Minecraft-style demo world

Status: **DONE**

- `BuildDemoWorld()` fully rewritten: flat ground plane R=18, raised hill (h=1–3 east at dx 6–12),
  staircase of 4 steps (dx 5–8), elevated platform across north (dx −8..0, dz 8..12, h=2),
  stone tower NW (dx −14..−12, dz 12..14, h=1..4), lava pit with 4 stepping stones (dz=−6, SW),
  spike strip (dz=−10), crusher corridor (dx 2..5, dz=−7), water channel (dx −8/−9, dz −4..4),
  border walls (h=1..3 along R=18 perimeter); extra Crusher tiles per world number
- Provides terrain with varied heights (h=0..4) for testing jump, platform, and hazard mechanics

---

## Phase 55 — Shield expiry feedback + life pickup popups

Status: **DONE**

- Shield expiry: `hadShield` bool captured before timer decrement; when `hadShield && shieldTimer_ ≤ 0`,
  plays `SoundChannel44` and spawns "SHIELD OFF" cyan `ScorePopup` at Blupi's position
- Shield pickup: spawns "SHIELD!" cyan popup at collection point
- Egg/drink pickup: additional "+1 LIFE!" green popup spawned above score popup when life awarded
- Popup colour constants: `kYellow(1.0,0.95,0.2)`, `kGreen(0.4,1.0,0.4)`, `kCyan(0.3,0.8,1.0)`;
  all passed explicitly to `spawnPopup` lambda (lambda default args cannot reference local vars)

---

## Phase 54 — Enemy facing direction

Status: **DONE**

- `Decor::Object::facingLeft` bool: updated each frame from `dx = pos.x_ − oldPos.x_`;
  directional types 2/3/4/17/20/33 checked; `facingLeft = (dx < 0)` (sprites default face left)
- `ObjectNode::UpdateIcon(int icon, bool flipX)`: when `flipX`, UV rect swaps `u0`↔`u1`
  so the billboard mirror-flips horizontally; no geometry change needed
- Called as `obj.node->UpdateIcon(GetIcon(obj), !obj.facingLeft)` each frame

---

## Phase 53 — Score popup system

Status: **DONE**

- `ScorePopup` class: `Text3D` node with `SetFaceCameraMode(FC_ROTATE_XYZ)`, font 24 pt,
  rises `1.5×t` units and fades alpha `1.0 − t` over `kDuration = 1.0 s`; returns false from
  `Update()` when done; `~ScorePopup()` removes node from scene
- `popups_` (`std::vector<std::unique_ptr<ScorePopup>>`) in game; erased per frame with
  `remove_if(…!p->Update(dt))`; cleared in `LoadWorld()` and `Stop()`
- `score_` persistent int; reset only on full restart (`ResetLevel`) or slot select;
  awards: +10 treasure, +25 stomp kill, +50 key/egg/drink, +100 all-treasures bonus

---

## Phase 52 — Footstep sound + stomp explosion

Status: **DONE**

- `Blupi::stepThisFrame_` / `WasStepThisFrame()`: set when `action_ == March && onGround_
  && animTick_ % 18 == 1`; one footstep sound per stride (6 anim frames × 3 ticks/frame = 18)
- `GalaxyEggbertGame::UpdatePlay()`: plays `SoundChannel3` when `WasStepThisFrame()`
- Stomp kill: `Explosion` spawned at `GetLastStompPos()` in addition to bounce/sound

---

## Phase 40 — SelectGamer reset bug, fall-death sound, FOV

Status: **DONE**

- `SelectGamer()`: added missing per-level resets (`prevCollected_`, `keysRed/Green/Blue_`,
  `shieldTimer_`, `respawnInvincibleTimer_`); all per-level fields now match what
  `AdvanceToNextWorld` and `ResetLevel` already reset — switching gamer slots no longer
  carries over stale shield/key state from a previous play session
- Fall death path: `sound_->Play(SoundChannel8)` added before life deduction;
  previously falling off the map was the only death path without an audio cue
- `CameraController`: `cam->SetFov(65.0f)` in constructor; Urho3D default 45° was too
  narrow for a 3rd-person view — 65° gives a natural platformer field of view

---

## Phase 39 — Maintainability refactor (no behaviour change)

Status: **DONE**

- `GalaxyEggbertGame.cpp`: added file-scoped `static const char* WorldName(int)` helper;
  removed 3× duplicated local `kWorldNames[]` in `UpdatePause`, `UpdateWin`, `UpdateLost`
- `GalaxyEggbertGame.hpp/.cpp`: `keys49_/50_/51_` renamed to `keysRed_/keysGreen_/keysBlue_`
  throughout (mechanical sed rename, zero logic change)
- `GalaxyEggbertGame.hpp`: state block split into two clearly labelled groups:
  "persistent game state" (`lives_`, `currentWorld_`, `gameData_`, …) and
  "per-level state" (`prevCollected_`, `keysRed_/Green_/Blue_`, timers, `bonusLifeAwarded_`)
  — makes "what needs resetting on level load" immediately visible
- `Decor.hpp`: header comment replaced with accurate list of all 15 supported object types,
  movement capabilities, and what remains unported

---

## Phase 38 — Mouse cursor management + camera pitch fix + pause key status

Status: **DONE**

- `EnterPhase(Play)`: `input->SetMouseVisible(false)` — cursor hidden during gameplay so it
  doesn't drift off-screen during RMB camera control; all other phases call `SetMouseVisible(true)`;
  `Stop()` also restores visibility for clean shutdown
- `Camera.hpp`: initial `pitch_ = 20.0f` (was 25°) matches `kDefaultPitch`; no longer drifts
  toward default at startup
- `UpdatePause()`: builds `keyLine` only when `keys49_+keys50_+keys51_ > 0`; shows
  "Keys: Red / Green / Blue" for collected types; line absent when no keys held

---

## Phase 37 — Per-type key icons + smooth camera zoom

Status: **DONE**

- `Decor`: replaced single `keysCollected_` with `keysType49_/50_/51_`; `GetKeys49/50/51()`
  added; `GetKeysCollected()` returns their sum; each key case increments its own counter
- `GalaxyEggbertGame`: `keysCollected_` replaced by `keys49_/50_/51_`; key detection
  compares all 3 per-type; reset sites updated; `ShowPlay` call passes all 3 values
- `HUD`: `ShowPlay` signature takes `keys49,keys50,keys51`; constructor assigns per-slot
  icon rects (`kKeyRects[3]`): icon 209 red, icon 220 green, icon 229 blue; each slot
  shown only when its respective key type has been collected
- `CameraController`: `targetDist_` field receives scroll input; `dist_` lerps toward it
  with `min(1, 8×dt)` each frame — zoom is smooth even with fast wheel moves

---

## Phase 36 — Treasure-only counter + camera pitch auto-reset

Status: **DONE**

- `Decor`: ObjectType6 (egg) and ObjectType30 (drink) no longer increment `collected_`;
  only ObjectType5 (treasure) does — `collected_`/`GetCollected()` is now purely type5;
  HUD "Treasures: X/Y" display is now correct; bonus life fires only on all type5 collected;
  egg/drink collect sound was SoundChannel10+42 (double), now just SoundChannel42 (life sound)
- `CameraController`: `kDefaultPitch = 20.0f`; when RMB is not held, pitch decays toward
  default via `pitch_ += (kDefaultPitch - pitch_) * min(1, 2*dt)`; players who tilt the camera
  see it automatically return to a playable overhead angle when they let go

---

## Phase 35 — Blupi shield tint + lives overflow indicator

Status: **DONE**

- `Blupi::SetShieldActive(bool)`: sets `shieldActive_` field; `UpdateSprite()` applies
  `bb->color_ = shieldActive_ ? Color(0.5, 0.85, 1.0) : Color::WHITE` each frame;
  Urho3D multiplies billboard color with texture giving a visible blue-cyan tint
- `GalaxyEggbertGame::UpdatePlay()`: calls `blupi_->SetShieldActive(shieldTimer_ > 0.0f)`
  immediately after decrementing timers so tint is always in sync with the actual shield state
- `HUD`: added `livesOverflow_` (Text, amber color) positioned right of the 5 life icons;
  `ShowPlay()` shows `"xN"` when lives > `kMaxDisplayedLives`; `SetVisible(false)` hides it

---

## Phase 34 — Spider vertical drop + drink extra-life + F1 world-jump fix

Status: **DONE**

- `Decor::StepMovement()`: stationary check now includes Y axis (`posStart.y_ == posEnd.y_`);
  `delta.y_ = 0.0f` clamp removed — movement can be along any axis; all existing horizontal
  patrol enemies unaffected (their posStart.y_ == posEnd.y_ == 1.0f)
- `LoadMobileEggbertTerrain`: ObjectType16 (spider) gets `posStart.y_=4.0f, posEnd.y_=1.0f`
  so it oscillates hanging↔dropped; XZ unchanged; Y proximity check (Phase 32) prevents
  hits while spider is high up; stomp possible when it drops to ground level
- `Decor`: added `drinkCollected_` bool flag + `WasDrinkCollected()` + ClearEvents reset;
  ObjectType30 collection sets it (also still increments `collected_`)
- `GalaxyEggbertGame::UpdatePlay()`: `WasDrinkCollected()` → +1 life (cap 9), SoundChannel42,
  `gameData_.Write()` — matching egg behaviour
- Debug toggle moved from KEY_F1 to KEY_F12 in `Update()`; F1–F5 world-jump now reaches
  the handler in `UpdatePlay()` without being consumed first

---

## Phase 33 — Egg extra-life + pause treasures + all-worlds message

Status: **DONE**

- `Decor`: ObjectType6 (egg) now sets `eggCollected_` flag on collection (still increments
  `collected_` for HUD); `WasEggCollected()` / `ClearEvents()` wired; reset in `Update()` start
- `GalaxyEggbertGame::UpdatePlay()`: checks `WasEggCollected()` → +1 life (cap 9),
  `gameData_.Write()`, `SoundChannel42`; behaviour mirrors the original mobile-eggbert egg pickup
- `UpdatePause()`: overlay text now shows `"Lives: N   Treasures: X/Y"` — pausing
  mid-level shows collected/total treasure count alongside lives
- `UpdateWin()`: uses `header = (completedWorld >= kMaxWorld) ? "ALL WORLDS COMPLETE!" : "LEVEL COMPLETE!"`;
  completing world 5 shows the distinct all-worlds congratulation text

---

## Phase 32 — Win/Lost result overlays + aerial birds

Status: **DONE**

- Win overlay: `UpdateWin()` calls `phases_->SetOverlayText()` each frame with "LEVEL COMPLETE!",
  completed world name (currentWorld_-1), treasures collected/total, and lives remaining;
  `decor_` is still alive during Win phase so stats remain queryable until `AdvanceToNextWorld()`
- Lost overlay: `UpdateLost()` shows "GAME OVER", world name, and "press any key to restart"
- `ObjectType20` (birds) placed at y=3.0f in `LoadMobileEggbertTerrain` so they fly visually
  above the terrain instead of skimming the ground
- `Decor::TouchesBlupi()` now checks `std::abs(dy) < 1.5f` alongside XZ radius;
  aerial birds (dy ≈ 1.8 from ground Blupi) no longer ghost-hit while walking below them;
  ground enemies and fish (dy < 0.8) unaffected

---

## Phase 31 — Enemy stomp mechanic

Status: **DONE**

- `Decor::Update()` now takes `float blupiVelY`; if `blupiVelY < -1.0` when touching an
  enemy (types 2,3,4,16,17,20,33) → stomp: enemy marked inactive + removed, `stompKill_` set
  instead of `blupiHit_`; otherwise normal hit applies
- `Blupi::Bounce()`: sets `vel_.y_ = kJumpSpeed * 0.6f`, clears `onGround_`; gives a natural
  bounce off the enemy's head
- `Blupi::GetVelY()`: exposes current Y velocity for Decor/GalaxyEggbertGame queries
- GalaxyEggbertGame: passes `blupi_->GetVelY()` to `decor_->Update()`; handles `WasStompKill()`
  → plays SoundChannel5 (bounce/boing, ported channel ID from mobile-eggbert) + calls Bounce()

---

## Phase 30 — Terrain depth fill (cliff extrusion)

Status: **DONE**

- `SpawnTerrainNodes()`: for each solid block, checks all 4 horizontal neighbours; if any
  is air or out-of-bounds the block is an "edge" tile → 3 extra `BlockFill` nodes placed
  at y-1, y-2, y-3 with a dark earthy material (`Color(0.22, 0.19, 0.17)`)
- `kFillDepth = 3` (constant at top of function); reuses the same `fillMat` SharedPtr across
  all fill blocks to avoid per-block material allocation
- Net effect: terrain cliffs appear as solid 3-unit-deep walls rather than thin 1-unit slabs;
  camera can now look sideways at platforms and see proper depth
- Also fixed two stale lines in NEXT.md (controls description, MoveObject type list)

---

## Phase 29 — Camera wall collision + pause info + F1-F5 world jump

Status: **DONE**

- Camera collision: `CameraController` stores `World*` + offsets via `SetCollisionWorld()`;
  `Update()` steps the ray from Blupi (0.5 unit margin) toward ideal position in 0.3-unit steps;
  first solid voxel clamps camera distance to `max(1.5, t - step)`; avoids camera clipping into walls
- Pause overlay text: `UpdatePause()` calls `phases_->SetOverlayText()` each frame with world
  name (same table as HUD), lives, and key hints (ESC/S)
- F1-F5 world jump: during Play, pressing F1-F5 performs a full state reset and loads world 1-5;
  same reset sequence as AdvanceToNextWorld (resets all timers, flags, decor, blupi)

---

## Phase 28 — WASD/QE controls + bonus life + controls hint fade

Status: **DONE**

- Full WASD+QE control scheme: W/S as alias for UP/DN (forward/back), Q/E as alias for L/R arrow
  (turn); A/D strafe unchanged; all keys checked via `||` in Blupi::Update()
- Controls hint line in HUD fades after 8 s (`controlsHintTimer_`); hint updated to show new keys;
  reset to 8.0f on every world load, SelectGamer, AdvanceToNextWorld, ResetLevel
- Bonus life on all-treasures: when `collected >= totalTreasures > 0` and `!bonusLifeAwarded_`,
  award +1 life (cap 9), play SoundChannel42, persist via GameData; flag reset per level

---

## Phase 27 — Invincibility flash + landing sound + ObjectType12 crate

Status: **DONE**

- Blupi invincibility flash: `StartFlash(float)` / `flashTimer_` / `flashTickTimer_` in Blupi;
  billboard enabled_ toggled every 0.1 s while timer > 0; resets to visible when timer expires
  and on `SpawnAt()`; called with `2.0f` after each respawn path in GalaxyEggbertGame
- Landing sound: `landedThisFrame_` flag set in `Blupi::Update()` when onGround_ transitions
  false→true after `ResolveY()`; `SoundChannel4` played by GalaxyEggbertGame each landing
- ObjectType12 (crate/box — pushable in original): added as static decoration;
  `GetIcon()` returns 32 (element.png); no collision action; type 12 added to supported list
  in `LoadMobileEggbertTerrain()`

---

## Phase 26 — Fall-death life deduction + respawn invincibility + kMaxObjects fix

Status: **DONE**

- Fall death: `Blupi::Update()` sets `fallDeath_ = true` before `SpawnAt()` when `pos.y_ < -10`;
  `GalaxyEggbertGame::UpdatePlay()` reads `WasFallDeath()`, deducts a life, handles Lost phase,
  clears flag via `ClearFallDeath()`
- Respawn invincibility: `respawnInvincibleTimer_` (2 s) set after every respawn path (fall death,
  tile hazard, enemy hit); tile hazard check and `WasBlupiHit()` both guarded with `<= 0.0f`;
  reset to 0 in `ResetLevel()` and `AdvanceToNextWorld()`
- `kMaxObjects` in `Decor.hpp` increased from 50 → 100; worlds 3/4/5 need 52/57/50 slots

---

## Phase 25 — Sky dome + A/D strafe + blupit tank enemy

Status: **DONE**

- Sky dome: large sphere (500 units) using `DiffSkydome.xml` (renders at far plane, no depth
  write, no fog); background texture `backgrounds/decorNNN.png` from `region=` in world header
  - World 1 (region 0) → decor000.png, World 3 (region 16) → decor016.png, etc.
- Strafe: A/D keys move Blupi left/right perpendicular to facing direction
  (LEFT/RIGHT still rotates; A/D strafes without turning)
- ObjectType33 (blupit tank): `table_blupit_left` icons {249,249,250,250,249,249,248,248};
  patrol enemy like type 2/3; gets ±2 tile range if posStart==posEnd; kills on contact
- `skyRegion_` field in GalaxyEggbertGame; reset per `LoadWorld`, parsed from `.txt` header

---

## Phase 24 — Correct tile passability from table_decor_quart

Status: **DONE**

- `BlockTypes::isMobileTransparent(icon)`: precomputed `bool[441]` from mobile-eggbert
  `Tables::table_decor_quart` — true for icons with all-zero 4×4 sub-cells (fully decorative)
- `fromMobileIconId`: passable tiles → `Air` instead of solid block
  - 203 icon IDs become Air (sky tiles, clouds, backgrounds, decorative patterns)
  - Icons 68 (Lava) and 317 (Crusher) excluded — kept solid for hazard gameplay
  - Spike (373) is NOT passable in the quart table — already solid and correct
- Removes invisible walls that blocked Blupi in all 5 real world files

---

## Phase 23 — Helicopter/drink pickups + per-world sky colours

Status: **DONE**

- `ObjectType13` (helicopter): static element.png icon 68; collecting grants shield as vehicle-boarding placeholder
- `ObjectType30` (drink): static element.png icon 178; collecting increments treasure counter
- Both added to `LoadMobileEggbertTerrain` supported list (21 helicopters in world 4)
- `ApplyWorldSky()`: Urho3D Zone ambient + fog colour swapped per world number
  - 1 Grassland: warm green sky
  - 2 Forest: dark green
  - 3 Ice Caves: pale blue-white
  - 4 Lava Fields: deep red
  - 5 Space Station: near-black

---

## Phase 22 — Full tile texture variety (BlockTypes identity redesign)

Status: **DONE**

- `BlockTypes`: block type = icon ID directly; named constants equal their icon values
- `fromMobileIconId`: trivial identity — `icon > 0 → icon`, `0 → Air`
- `toIconIndex`: trivial — `t == Air ? -1 : t`
- Result: all 400+ distinct tile IDs in the 5 world files now render their actual
  object-m.png texture (ice, rock, forest, lava field tiles etc.) instead of defaulting to grass
- Named gameplay constants (Ground=10, Wall=183, Lava=68, Spike=373, Crusher=317, …) unchanged in semantics

---

## Phase 21 — Fish enemy + moving platform lift

Status: **DONE**

- `ObjectType17` (fish): patrol enemy; `table_poisson_left` icons {82,82,81,81,82,82,83,83}; damages Blupi on contact
- `ObjectType1` (platform lift): moves between posStart↔posEnd; `Decor::GetPlatformDelta()` returns the XZ delta carried by platforms Blupi is riding; applied in `UpdatePlay` via `Blupi::ApplyExternalDelta`
- Both added to `LoadMobileEggbertTerrain` supported-types list
- 18 platform and 10 fish objects now active in real world files

---

## Phase 20 — Correct Blupi spawn from real world files

Status: **DONE**

- `LoadMobileEggbertTerrain` uses 64px tile size (matches mobile-eggbert pixel coordinates)
- `blupiPos=` header parsed → `blupiSpawn_` stored in `GalaxyEggbertGame`
- `EnterPhase(Play)`: calls `blupi_->SetSpawnPoint(blupiSpawn_)` then `Respawn()`
- Fall-through-floor respawn in `Blupi::Update` uses `spawn_` field (fixed hardcoded origin)
- Blupi now starts at correct tile position in all 5 real world files

---

## Phase 19 — New enemy types + ObjectNode sprite fix

Status: **DONE**

- `ObjectNode` sprite UV fixed: 60×60 px tiles, 10 cols (was erroneously 64 px/9 cols)
- `ObjectType4` (bulldozer): patrol enemy using `table_bulldozer_left` icons 65–67
- `ObjectType20` (bird): patrol enemy using `table_oiseau_left` icons 98–105
- Both damage Blupi on contact (same as ObjectType2/3)
- `LoadMobileEggbertTerrain` now parses types 4 and 20; stationary patrol enemies
  get a default ±2 tile X patrol range

---

## Phase 18 — Real mobile-eggbert world loading

Status: **DONE** — `LoadMobileEggbertTerrain()` in GalaxyEggbertGame.cpp.

- `worlds/world00N.txt` files (from mobile-eggbert world01N.txt) shipped in the repo
- `LoadWorld()` tries .vwr first, then .txt, then generates demo world
- `BlockTypes::fromMobileIconId()` maps mobile-eggbert tile IDs → galaxy-eggbert block types
- MoveObject parser handles types 2,3,5,6,7,16,25,49,50,51; speed from `stepAdvance`
- Blupi always spawns at 3D (0,y,0); world is centered on the `blupiPos` from the file

---

## Phase 17 — HUD treasure total + world names

Status: **DONE** — `Decor::GetTotalTreasures()` + `WorldName()` lookup in HUD.

- `Decor::PlaceObject` counts ObjectType5 placements into `totalTreasures_`
- `HUD::ShowPlay` now shows `"Treasures: N/total"` and `"World N: <Name>"`
- World name table: 1→Grassland, 2→Forest, 3→Ice Caves, 4→Lava Fields, 5→Space Station

---

## Phase 16 — HUD sprite icons (life icons + key icons)

Status: **DONE** — `HUD.cpp` rewritten with `BorderImage` sprite icons.

- Life icons: `blupi.png` icon 48 (col 8, row 4 of 60×60 sheet) — up to 5 Blupi heads
- Key icons: `element.png` icon 215 (col 5, row 21) — one icon per collected key (max 3)
- Text simplified: removed position/facing debug; shows `World N | Treasures: N [SHIELD Xs]`
- `ShowPlay` signature drops `pos`/`facingYaw` (no longer needed for display)

---

## Phase 19 — Android build (U3D)

Status: **Not yet implemented for U3D.**

U3D uses its own Gradle + CMake Android integration.
Steps: build U3D AAR → set `BUILD_STAGING_DIR` → remove `FATAL_ERROR` guard for ANDROID in CMakeLists.txt.

---

## Phase 20 — Web build (Emscripten)

Status: **Not yet implemented for U3D.**

Steps: Emscripten U3D build (`emcmake cmake`) → add `--preload-file Data/ CoreData/` → remove `FATAL_ERROR` guard.

---

## Nova3D backend

When Nova3D implements the full Urho3D API (same headers, same namespace, same scene graph):

1. Set `GALAXY_EGGBERT_ENGINE=NOVA3D` in CMake.
2. Verify all Urho3D API calls compile and behave identically.
3. Expected C++ source change: zero lines (no `#ifdef` guards exist).
