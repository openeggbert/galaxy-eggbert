# NEXT.md — Galaxy Eggbert

## 1. Project summary

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (a C++ port of *Speedy Blupi*, a Windows Phone XNA game from 2013).

- Two build targets:
  - `GalaxyEggbert` — original, fully working, built against U3D
  - `GalaxyEggbertSimple3D` — new port in progress, built against `simple-3d`
- **Faithful remake rule:** Only implement what exists in mobile-eggbert. No new mechanics.
- 3D-specific adaptations (camera, blob shadows, auto step-up, billboard sprites) are allowed.
- Feature tracking: `plan.md` (checklist)

Architecture target:
```
galaxy-eggbert game code -> simple-3d API -> Urho3D / Nova3D -> CNA / backend
```

---

## 2. Current status

### GalaxyEggbert (Urho3D target) — FULLY WORKING
- Builds and runs: `cmake --build cmake-build-u3d --target GalaxyEggbert -j2`
- 80 phases complete; playable: 5 worlds, enemies, pickups, shield, stomp, score, HUD, camera, sound

### GalaxyEggbertSimple3D (Simple3D target) — BUILDS, S3D-3 DONE
- Compiles: `cmake-build-simple3d/GalaxyEggbertSimple3D` (89 MB)
- Terrain: tile-atlas textures from `object-m.png` per block type (S3D-2 ✅)
- Blupi: billboard sprite from `blupi.png`, walk animation frames 0–9 (S3D-3 ✅)
- HUD: text-only labels
- Sound: 5 key sounds wired
- Camera: orbit mode

### Tests
- 54 engine-independent unit tests for `GalaxyEggbert::Worlds::*` — pass

---

## 3. Recent changes

- **S3D-3** — Blupi: box placeholder replaced with billboard from `blupi.png`; child entity at vertical offset `kVisHalf - kHalfH`; walk animation cycles frames 0–9 row 0; idle shows frame 0
- **S3D-2** — `GETerrainRenderer`: `BlockTypes::tileUV` + `SetTileTexture("icons/object-m.png", ...)` per block; fill/edge blocks use `SetMaterialColor(dark brown)`
- **S3D-1 compile** — fixed `Label::SetScale` in simple-3d; fixed `SetShieldActive` → `SetShieldTimer`

---

## 4. Current focus

**S3D-4 — Decor object visuals (enemies + pickups as billboard sprites from `element.png`)**

---

## 5. Known incomplete items

| Status | Issue |
|--------|-------|
| incomplete | S3D-4: Decor object visuals (enemies, pickups as billboards from `element.png`) — current task |
| incomplete | S3D-5: HUD images (gauge, life icons, key icons) via `UI::Image::SetImageRect` |
| incomplete | S3D-6: Phase/menu system (Init gamer select, Ranking screen) |
| incomplete | S3D-7: Sound channel parity — 93 channels; API available, not wired |
| incomplete | GECameraRig::StartShake — stub, needs `Camera::Shake(intensity, duration)` call |
| incomplete | GEDecorSystem: enemy AI placeholder, not ported from Decor.cpp |
| incomplete | Save data not wired in Simple3D path |
| incomplete | Ranking screen not implemented (Urho3D version either) |
| incomplete | Push mechanic (ObjectType12 crates) not implemented |
| incomplete | ObjectType23 (projectile), ObjectType96/97 (follow enemies) not implemented |
| incomplete | Vehicles: helicopter, jeep, tank, skateboard, balloon, swim, surf |
| incomplete | Android build not implemented |
| incomplete | Nova3D backend not available yet |
| needs verification | `GEBlupiController` step-up via `CharacterController::SetStepHeight` — behaviour vs. old AABB step-up may differ |

---

## 6. Architecture notes

### Two independent build targets

| Target | Sources | Engine API | Status |
|--------|---------|------------|--------|
| `GalaxyEggbert` | `src/GalaxyEggbert/` | Direct Urho3D | Fully working |
| `GalaxyEggbertSimple3D` | `src/GalaxyEggbertSimple3D/` | `Simple3D` only | Builds, S3D-2 done |

Shared (engine-agnostic) code compiled into both targets:
- `include/GalaxyEggbert/Worlds/` — Block, Chunk, World data model
- `include/GalaxyEggbert/BlockTypes.hpp` — tile type constants + `tileUV()` UV math

### Key invariants
- Block type = icon index = `object-m.png` atlas position (except Air=0)
- `BlockTypes::tileUV(icon, uOff, vOff, uScale, vScale)` → UV rect in `object-m.png` (1301×1431 px, 64×64 tiles, 20 cols)
- World files: `worlds/world001.txt` … `world005.txt` — mobile-eggbert format, all blocks at y=0
- No `#ifdef` for engine differences — Simple3D port is a separate source tree
- Simple3D API gaps: `docs/SIMPLE3D_GAPS.md` (currently none outstanding)

---

## 7. Useful commands

```bash
# Build Urho3D version
cmake --build cmake-build-u3d --target GalaxyEggbert -j2
./cmake-build-u3d/GalaxyEggbert

# Run unit tests
./cmake-build-u3d/tests/GalaxyEggbertWorlds

# Build Simple3D version (first time: add -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON -DSIMPLE3D_HOME=../simple-3d)
cmake --build cmake-build-simple3d --target GalaxyEggbertSimple3D -j2
./cmake-build-simple3d/GalaxyEggbertSimple3D
```

---

## 8. Next tasks

### Task — S3D-4: Decor object visuals
**Goal:** Enemies and pickups show as billboard sprites from `element.png`.
**Files:** `src/GalaxyEggbertSimple3D/Game/GEDecorSystem.cpp/.hpp`
**Reference:** `src/GalaxyEggbert/Game/Decor.cpp` — ObjectNode sprite UV per ObjectType

### Task — Ranking screen (Urho3D version)
**Goal:** `Phase::Ranking` — high-score table from Init screen.
**Files:** `src/GalaxyEggbert/Game/GalaxyEggbertGame.hpp/.cpp`
**Reference:** `../mobile-eggbert` — `Phase::Ranking` in `Def.hpp`

### Task — Push mechanic (Urho3D version)
**Goal:** Blupi pushes ObjectType12 crates horizontally.
**Files:** `src/GalaxyEggbert/Game/Blupi.cpp`, `Decor.cpp`
**Reference:** `../mobile-eggbert` — crate push logic in `Decor.cpp`

---

## 9. Do not do yet

- Do not port Vehicles (helicopter, jeep, tank) — complex multi-state
- Do not touch Android build
- Do not edit simple-3d and galaxy-eggbert simultaneously in parallel agents — file conflicts
- Do not delete `src/GalaxyEggbert/` — keep as reference until Simple3D version is playable
- Do not add features not in mobile-eggbert (faithful remake rule)
- Do not use `-j` more than `-j2` — RAM constraint

---

## 10. Resume prompt

```
Read NEXT.md first. Then inspect only files relevant to the first task in section 8.
Do not refactor unrelated code. Make one small, verified change.
Build with the command from section 7 to confirm success.
Update NEXT.md sections 2, 3, 4, 8 after finishing.
```
