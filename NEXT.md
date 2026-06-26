# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a Windows Phone XNA game from 2013).

- Implemented in C++ using the **Urho3D API**
- Two build targets exist:
  - `GalaxyEggbert` — original, fully working, built against U3D (u3d-community/U3D backend)
  - `GalaxyEggbertSimple3D` — new port in progress, built against `simple-3d` (a high-level C++ framework wrapping Urho3D)
- **Faithful remake rule:** Only implement what exists in mobile-eggbert. No new mechanics.
- 3D-specific adaptations (camera, blob shadows, auto step-up, billboard sprites) are allowed.
- Feature tracking: `plan.md` (checklist)

Architecture target:
```
galaxy-eggbert game code
  -> simple-3d API
    -> Urho3D / Nova3D
      -> CNA / backend
```

The Simple3D migration (prefix: S3D-*) is the current development focus.

---

## 2. Current status

### GalaxyEggbert (Urho3D target) — FULLY WORKING
- Builds and runs: `cmake --build cmake-build-u3d --target GalaxyEggbert -j2`
- 80 phases complete, all committed and pushed to `develop`
- Playable: 5 worlds, enemies, pickups, shield, stomp, score, HUD, camera, sound
- Phase 79 removed non-mobile-eggbert features: time bonus, star rating, best time, stomp combo, danger pulse, coyote time, jump buffer, variable jump

### GalaxyEggbertSimple3D (Simple3D target) — SKELETON ONLY, NOT BUILT YET
- Phase 80 (S3D-1): skeleton created, all source files exist under `src/GalaxyEggbertSimple3D/`
- Target is OFF by default (`-DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON` to enable)
- Has NOT been compiled yet — likely has build issues due to Simple3D API gaps
- Terrain: grey placeholder cubes, no tile textures
- Blupi: `CharacterController`-based, basic movement, no sprites
- HUD: text-only labels
- Sound: 5 key sounds wired
- Camera: orbit mode

### Tests
- 54 engine-independent unit tests for `GalaxyEggbert::Worlds::*` — pass

---

## 3. Recent changes

**Phase 80 (commit `381b6e2`):**
- Created `src/GalaxyEggbertSimple3D/` — 14 new source files (full skeleton)
- Created `docs/SIMPLE3D_GAPS.md` — table of missing Simple3D APIs
- Created `docs/simple3d_migration.md` — architecture and migration progress
- Created `docs/simple3d_migration_task.md` — original task instructions preserved
- Updated `CMakeLists.txt` — added `GalaxyEggbertSimple3D` target (off by default)
- Updated `plan.md` — Simple3D Migration section with S3D-1 through S3D-9

---

## 4. Current blocker / main problem

**`GalaxyEggbertSimple3D` has never been compiled — build errors likely exist.**

Most previously-missing Simple3D APIs are now implemented (tile textures, billboard UV crop,
fog, ambient, camera shake, UI image rect, progress bar, fade transitions).

Two gaps remain — see `docs/SIMPLE3D_GAPS.md` for the full current list:
- Sky dome (per-world background image)
- Per-channel audio (93 indexed channels)

---

## 5. Known bugs and limitations

| Status | Issue |
|--------|-------|
| incomplete | `GalaxyEggbertSimple3D` never compiled — unknown build errors |
| incomplete | S3D-2 blocked: `Entity::SetTileTexture` missing from simple-3d |
| incomplete | S3D-3: Blupi billboard animation needs `AddBillboard(texPath, uvRect, size)` (UV crop not yet in simple-3d) |
| incomplete | S3D-5: HUD images (gauge, life icons, key icons, hit flash) — `UI::Image::SetImageRect` exists in simple-3d UI but not exposed in simple-3d public API |
| incomplete | S3D-6: Phase/menu system (Init gamer select, Ranking screen) not ported |
| incomplete | S3D-7: Sound channel parity — only 5 sounds wired |
| incomplete | GECameraRig::StartShake is a no-op stub (Camera::Shake missing from simple-3d) |
| incomplete | GEDecorSystem: enemy AI logic is placeholder, not ported from Decor.cpp |
| incomplete | Save data not wired in Simple3D path |
| incomplete | Ranking screen not implemented in Urho3D version either |
| incomplete | Push mechanic (ObjectType12 crates) not implemented |
| incomplete | ObjectType23 (projectile), ObjectType96/97 (follow enemies) not implemented |
| incomplete | Vehicles: helicopter, jeep, tank, skateboard, balloon, swim, surf |
| incomplete | Android build not implemented |
| incomplete | Nova3D backend not available yet |
| needs verification | `GEBlupiController` step-up: currently uses `CharacterController::SetStepHeight`; actual Urho3D CC behaviour vs. old manual AABB step-up may differ |

---

## 6. Architecture notes

### Two independent build targets

| Target | Sources | Engine API | Status |
|--------|---------|------------|--------|
| `GalaxyEggbert` | `src/GalaxyEggbert/` | Direct Urho3D (`#include <Urho3D/Urho3DAll.h>`) | Fully working |
| `GalaxyEggbertSimple3D` | `src/GalaxyEggbertSimple3D/` | `#include <Simple3D/Simple3D.h>` only | Skeleton, untested |

Shared (engine-agnostic) code compiled into both targets:
- `include/GalaxyEggbert/Worlds/` — Block, Chunk, World data model
- `include/GalaxyEggbert/BlockTypes.hpp` — tile type constants + `tileUV()` UV math
- `src/GalaxyEggbert/Game/Tables.cpp` — animation frame tables (Urho3D target only for now)

### Key invariants
- `BlockTypes::tileUV(icon, uOff, vOff, uScale, vScale)` maps icon index → UV rect in `object-m.png` (1301×1431 px, 64×64 tiles, 20 columns)
- Block type = icon index = `object-m.png` atlas position (except Air=0)
- `MAXCELX = MAXCELY = 100` — level grid is always 100×100
- World files: `worlds/world001.txt` … `world005.txt` — mobile-eggbert format
- Save data: `GameData` 640-byte binary — compatible with mobile-eggbert (Urho3D path only)
- No `#ifdef` for engine differences in C++ source — Simple3D port is a separate source tree

### Simple3D gaps documented in
- `docs/SIMPLE3D_GAPS.md` — full table of missing APIs with proposed solutions
- `src/GalaxyEggbertSimple3D/Support/Simple3DMissingFeatures.hpp` — inline stub docs

---

## 7. Useful commands

```bash
# Build Urho3D version (default, working)
cmake --build cmake-build-u3d --target GalaxyEggbert -j2
./cmake-build-u3d/GalaxyEggbert

# Run unit tests
cmake --build cmake-build-u3d --target RunTests -j2
# or directly:
./cmake-build-u3d/tests/GalaxyEggbertWorlds

# Configure Simple3D build (first time)
cmake -S . -B cmake-build-simple3d \
  -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON \
  -DSIMPLE3D_HOME=../simple-3d
cmake --build cmake-build-simple3d --target GalaxyEggbertSimple3D -j2
```

---

## 8. Next smallest tasks

**Ordered by priority:**

### Task 1 — First compile of GalaxyEggbertSimple3D
**Goal:** Get the Simple3D target to compile (even if it doesn't run correctly).
**Files:** `CMakeLists.txt`, `src/GalaxyEggbertSimple3D/**`
**Steps:**
1. Ensure `../simple-3d` is built
2. Run: `cmake -S . -B cmake-build-simple3d -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON`
3. Run: `cmake --build cmake-build-simple3d --target GalaxyEggbertSimple3D -j2`
4. Fix each compile error one by one; note any missing Simple3D API as gaps
**Verify:** Binary `cmake-build-simple3d/GalaxyEggbertSimple3D` exists and launches

### Task 2 — S3D-2: Tile textures in GETerrainRenderer
**Goal:** Replace grey Box.mdl cubes with textured tiles from `object-m.png`.
`Entity::SetTileTexture` and `Entity::SetMaterialColor` are now in simple-3d — no prerequisite task needed.
**Files:** `src/GalaxyEggbertSimple3D/Game/GETerrainRenderer.cpp/.hpp`
**Steps:**
1. Call `BlockTypes::tileUV(icon, uOff, vOff, uS, vS)` per block
2. Call `e->SetTileTexture("icons/object-m.png", uOff, vOff, uS, vS)`
3. For fill/edge blocks, call `e->SetMaterialColor(Color(0.22f, 0.19f, 0.17f))`
4. Remove `// TODO(S3D-2)` comments
5. Mark S3D-2 as `[x]` in `plan.md`
**Verify:** Launch GalaxyEggbertSimple3D — terrain shows mobile-eggbert tile graphics

### Task 4 — Ranking screen (Urho3D version)
**Goal:** Implement `Phase::Ranking` — high-score table accessible from Init screen.
**Files:** `src/GalaxyEggbert/Game/GalaxyEggbertGame.hpp/.cpp`
**Reference:** `../mobile-eggbert` — `Phase::Ranking` in `Def.hpp`, score display in `Game1.cpp`
**Verify:** Press R on Init screen → ranking shown; F key returns to gamer select

### Task 5 — Push mechanic for ObjectType12 crates (Urho3D version)
**Goal:** Blupi can push crates horizontally when walking into them.
**Files:** `src/GalaxyEggbert/Game/Blupi.cpp`, `src/GalaxyEggbert/Game/Decor.cpp`
**Reference:** `../mobile-eggbert` — crate push logic in `Decor.cpp`
**Verify:** Walk into a crate → it slides; crate stops at wall

---

## 9. Do not do yet

- Do not attempt full visual parity of Simple3D version in one session
- Do not port Vehicles (helicopter, jeep, tank) — complex multi-state; tackle after core port is solid
- Do not touch Android build — needs separate CMake toolchain work
- Do not edit simple-3d and galaxy-eggbert simultaneously in parallel agents — file conflicts
- Do not delete `src/GalaxyEggbert/` (Urho3D version) — keep as reference until Simple3D version is playable
- Do not add features not present in mobile-eggbert (faithful remake rule)
- Do not use `-j` more than `-j2` — RAM constraint on this machine (crashes with more parallel jobs)

---

## 10. Resume prompt

```
Read NEXT.md first to understand the current state of galaxy-eggbert.
Then inspect only the files relevant to the first task listed in section 8.
Do not refactor unrelated code.
Make one small, verified improvement.
Run the relevant build command from section 7 to confirm success.
Update NEXT.md after finishing — update sections 2, 3, 4, and 8 to reflect what changed.
```
