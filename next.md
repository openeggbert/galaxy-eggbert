# Galaxy Eggbert — Phase Log

3D faithful remake of **mobile-eggbert** (C++ port of *Speedy Blupi*, Windows Phone XNA 2013).

**Faithful remake rule:** Only implement features that exist in mobile-eggbert.
3D-specific adaptations (camera, blob shadows, billboard sprites, auto step-up) are allowed.

See `plan.md` for the feature checklist.

---

## Phase log

| Phase | Summary |
|-------|---------|
| 79 | Remove non-mobile-eggbert features (time bonus, star rating, best time, stomp combo, danger pulse, coyote time, jump buffer, variable jump height) |
| 78 | Best time per world — **removed in 79** |
| 77 | Danger pulse (lives == 1) — **removed in 79** |
| 76 | Exit open sparkle + "EXIT OPEN!" popup |
| 75 | Treasure lock on exit ("Need all treasures!") |
| 74 | Star rating on win screen — **removed in 79** |
| 73 | Time bonus on level complete — **removed in 79** |
| 72 | Stomp camera shake (combo multiplier removed in 79) |
| 71 | Game speed selector (G key: Slow / Normal / Fast) |
| 70 | High score per gamer slot (GameData bytes 2–5) |
| 69 | Enemy respawn sparkle (poof on reappearance) |
| 68 | Blob shadows under enemies; disabled for birds |
| 67 | Level intro title card (world name, 3 s fade) |
| 66 | Blob shadow under Blupi (scans down, scales with height) |
| 65 | Glide (Right Shift in air): reduced gravity, capped fall speed |
| 64 | Auto step-up: 1-tile ledges climbed without jumping |
| 63 | Variable jump height — **removed in 79** |
| 62 | Coyote time + jump buffer — **removed in 79** |
| 61 | Multiple simultaneous explosions; pickup sparkle (scale 0.4) |
| 60 | Crusher safe timing (frames 0–4); ObjectNode vertical offset fix |
| 59 | Death respawn freeze (1 s input lock); FC_ROTATE_Y sprite fix |
| 58 | Shield warning blink (< 1.5 s); HUD colour turns orange |
| 57 | Moving platform landing (SnapToSurface); StepMovement Y-snap fix |
| 56 | Minecraft-style demo world (hill, staircase, lava pit, crusher corridor…) |
| 55 | Shield expiry feedback (sound + popup); "+1 LIFE!" popup on egg/drink |
| 54 | Enemy facing direction (flipX when moving right) |
| 53 | Score popup system (rising Text3D, 1 s fade) |
| 52 | Footstep sound (ch3 per stride); stomp explosion at enemy position |
| 51 | Death explosion animation (explo.png, BillboardSet, 12 fps) |
| 50 | Camera shake + red screen flash on any hit |
| 49 | Pickup bobbing (sine-wave Y offset) |
| 48 | Blupi proportions corrected (kHalfH = 23/64) |
| 47 | Animated tiles: Lava, Crusher, Spike, Saw, Water1, Water2 |
| 46 | Web / Emscripten build (GalaxyEggbert.html + .wasm) |
| 45 | Control scheme redesign: pure arrow keys, Left Ctrl jump, Left/Right Shift |
| 44 | Enemy respawn: stomped enemies hidden and return at posStart after 5 s |
| 43 | Score system: +10 treasure, +25 stomp, +50 key/egg/drink, +100 bonus-life |
| 42 | Level elapsed timer (HUD + Win overlay) |
| 41 | Pause overlay: score + time + key names |
| 40 | SelectGamer reset bug; fall-death sound; camera FOV 65° |
| 39 | Refactor: WorldName() helper; keysRed/Green/Blue rename; persistent vs per-level split |
| 38 | Mouse cursor hidden in Play; camera pitch fix; pause key-status line |
| 37 | Per-type key icons (red/green/blue); smooth camera zoom (lerp) |
| 36 | Treasure-only counter (eggs/drinks excluded); camera pitch auto-reset |
| 35 | Shield tint (cyan); lives overflow indicator (xN) |
| 34 | Spider vertical oscillation; drink +1 life; F1 world-jump fix |
| 33 | Egg +1 life; pause shows treasures; "ALL WORLDS COMPLETE!" on world 5 |
| 32 | Win/Lost overlays with world name + stats; aerial bird hit fix |
| 31 | Enemy stomp mechanic (velY < −1.0); Blupi::Bounce() |
| 30 | Terrain depth fill: 3 dark fill blocks below cliff edges |
| 29 | Camera wall collision (DDA ray march); pause info; F1–F5 world jump |
| 28 | Bonus life on all-treasures; controls hint auto-fade |
| 27 | Invincibility flash (Blupi blinks); landing sound; ObjectType12 crate |
| 26 | Fall-death life deduction; respawn invincibility (2 s); kMaxObjects 50→100 |
| 25 | Sky dome per world; blupit tank (ObjectType33) |
| 24 | Correct tile passability (table_decor_quart → 203 IDs become Air) |
| 23 | Helicopter/drink pickups; per-world sky palette |
| 22 | BlockTypes identity redesign (block type = icon ID; all 400+ tiles render) |
| 21 | Fish enemy (ObjectType17); moving platform lift (ObjectType1) |
| 20 | Blupi spawns at correct world position (blupiPos= header) |
| 19 | Bulldozer (type4) + bird (type20) enemies; ObjectNode sprite UV fix |
| 18 | Real mobile-eggbert world loading (LoadMobileEggbertTerrain) |
| 17 | HUD treasure total; world names |
| 16 | HUD sprite icons (life icons + key icons, BorderImage) |
| 15 | Windows build (MinGW-w64 cross-compile) |

---

## Android build

Status: **Not yet implemented.**

Steps: build U3D AAR → set `BUILD_STAGING_DIR` → remove `FATAL_ERROR` guard for ANDROID in CMakeLists.txt.

---

## Nova3D backend

When Nova3D implements the full Urho3D API:

1. Set `GALAXY_EGGBERT_ENGINE=NOVA3D` in CMake.
2. Verify all calls compile and behave identically.
3. Expected game-code changes: zero.
