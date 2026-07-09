# Galaxy Eggbert Direct CNA + Easy3D Migration Analysis

**Status:** Analysis only. No C++ code changed. No sibling repository modified. This document
records confirmed facts, recommendations, and open questions as of 2026-07-01.

**Status update (2026-07-05):** the recommendations below were acted on. `GalaxyEggbertCNA`
(`src/GalaxyEggbertCNA/`) now exists as a real, working, opt-in build target (builds via
`-DGALAXY_EGGBERT_BUILD_CNA=ON`, renders real textured/animated 3D terrain), and Easy3D gained a
real renderer (`CubeMesh`/`CubeMeshRenderer`) — see `NEXT.md` for current, up-to-date status. The
facts, rationale, and recommendations captured below are preserved as the historical record of why
those decisions were made; do not read "not yet built"/"does not exist" statements below as
describing the current state — check `NEXT.md` for that.

**Status update (2026-07-08):** `GalaxyEggbertSimple3D` (referred to throughout this document as
"the current Simple3D path") is now historical reference only — it is not built, fixed, or
maintained going forward (see `CLAUDE.md`'s "Current Direction Lock"). By 2026-07-09,
`GalaxyEggbertCNA` reached terrain/object rendering parity with everything terrain/object-related
this document analyzes (all 4 tile render modes, water, `MoveObject`/`BigDecor` billboards,
platform-lift/crate objects, face culling) — the remaining gap to Simple3D is Blupi's visible
sprite, HUD, sound, and gameplay logic (§12's open questions below are mostly resolved by now; see
inline annotations).

---

## 1. Executive Summary

Galaxy Eggbert is a faithful 3D remake of mobile-eggbert (itself a CNA-based port of *Speedy
Blupi*). The project's build direction is changing:

- **Old direction (superseded):** `Galaxy Eggbert → Simple3D → Urho3D / Nova3D → (CNA someday)`
- **New direction (target):** `Galaxy Eggbert → CNA directly`, with `Easy3D` used *beside* CNA
  for small reusable 3D helpers (cameras, billboard/cube batching, texture atlas, debug draw).
  `mobile-eggbert` is used as a **read-only** reference/asset/data source, not as code to modify.

Three facts drive the shape of this migration plan:

1. **Galaxy Eggbert's own `README.md` already describes the target CNA build** (target name
   `GalaxyEggbert`, `CNA_BACKEND_*` CMake flags, Web/Windows cross-build instructions) — but
   **`CMakeLists.txt` does not yet define any such target.** The README was apparently written
   ahead of the code. `CLAUDE.md`, `NEXT.md`, and `plan.md` still describe the Simple3D/U3D/Nova3D
   direction as current. These documents are now inconsistent with each other and need to be
   reconciled (see Phase 0 tasks in `plan.md`).
2. **Easy3D has no renderer.** It has real, working cameras (`Camera3D`, `OrbitCamera`,
   `FollowCamera`) and a genuinely CNA-free `TextureAtlas`, but `BillboardBatch`, `CubeBatch`, and
   `DebugDraw` are confirmed to be pure CPU-side item queues — grepping their implementation for
   any GPU/draw-call symbol (shader, vertex buffer, index buffer, `GraphicsDevice`,
   `SpriteBatch`, `BasicEffect`) returns nothing. Turning queued items into pixels on screen is
   **not yet built**, in Easy3D or anywhere else.
3. **mobile-eggbert cannot currently be linked as a CMake library.** Its top-level
   `CMakeLists.txt` only defines `add_executable(WindowsPhoneSpeedyBlupi ...)` (globbing all of
   `src/*.cpp`); there is no `add_library()` target anywhere, and no sub-`CMakeLists.txt` for
   `include/` or `src/`. Direct-link reuse of mobile-eggbert C++ code is not possible today
   without a mobile-eggbert-side change — which is out of scope for this migration unless the
   user explicitly approves it.

Because of (2) and (3), the realistic first playable milestone for the CNA/Easy3D path is much
smaller than "full gameplay parity." It is: an empty CNA window, Mobile Eggbert's world format
loaded and parsed, terrain drawn as flat-colored or simply-textured geometry, and Blupi rendered
as a 2D billboard using Mobile Eggbert's own sprite frames — with a hand-written (not linked)
terrain/billboard renderer built on top of Easy3D's CPU-side batches, since no such renderer
exists yet anywhere in the stack.

---

## 2. Current Architecture

### 2.1 Current Simple3D Path

Confirmed via inspection of `src/GalaxyEggbertSimple3D/` and `CMakeLists.txt`:

- The only buildable game target today is `GalaxyEggbertSimple3D`
  (`option(GALAXY_EGGBERT_BUILD_SIMPLE3D "Build the Simple3D port target" ON)`), linking
  `simple-3d` (found via `SIMPLE3D_HOME`, default `../simple-3d`), which in turn wraps U3D
  (Urho3D fork). The build hardcodes a U3D binary/data path
  (`/rv/data/library/github.com/u3d-community/U3D/...`) and copies `Data/CoreData` +
  `Content/worlds` next to the binary post-build.
- There is **no `GALAXY_EGGBERT_ENGINE` CMake option** in `CMakeLists.txt` today, despite
  `CLAUDE.md` documenting one (`-DGALAXY_EGGBERT_ENGINE=U3D` / `NOVA3D`). That option is
  aspirational/historical, not currently wired up.
- The old direct-Urho3D tree (`GalaxyEggbertApp`, `GalaxyEggbertGame`, a `GalaxyEggbert` target
  linking Urho3D directly), once kept as a "historical reference until S3D-9" per the (since
  completed and deleted, 2026-07-05) Simple3D migration docs, **no longer exists on disk.**
  `src/GalaxyEggbert/` today contains only the engine-agnostic `Worlds/` subtree.
- `src/GalaxyEggbertSimple3D/` totals 3,251 lines across 10 files:

  | File | Lines | Simple3D-coupled? |
  |---|---:|---|
  | `GalaxyEggbertSimpleGame.cpp` | 768 | Yes |
  | `Game/GEHud.cpp` | 624 | Yes |
  | `Game/GEDecorSystem.cpp` | 305 | Yes |
  | `Game/GEBlupiController.cpp` | 270 | Yes |
  | `Game/GETerrainRenderer.cpp` | 249 | Yes |
  | `Game/GEWorldRuntime.cpp` | 176 | Yes |
  | `Game/GEBridgeSystem.cpp` | 66 | **No** — only includes `GalaxyEggbert/Worlds/World.hpp`, `<vector>`, `<set>` |
  | `Game/GEExploSystem.cpp` | 50 | Yes |
  | `Game/GESound.cpp` | 58 | Yes |
  | `Game/GECameraRig.cpp` | 22 | Yes |

  Every `Game/*.hpp` except `GEBridgeSystem.hpp` includes `<Simple3D/Simple3D.h>`. `GEBridgeSystem`
  is the one class in this tree that is already engine-agnostic pure logic.
- The Simple3D API-gap tracker (deleted 2026-07-05 once the Simple3D migration finished with zero
  outstanding gaps) enumerated 18 Simple3D engine-facing capabilities actually in use: tile-atlas
  UV offset, billboard UV-region crop, fog, ambient light, camera shake, UI image UV crop, UI
  progress bar, full-screen fade panel, particle emitter, looped 3D audio source + listener, save
  data, input action binding, trigger volumes, character controller, orbit camera, camera
  collision avoidance, scene fade transitions, sky dome, and 93-channel indexed audio. This list
  (preserved here since the source doc is gone) is a useful checklist of *engine capabilities* any
  CNA/Easy3D replacement will eventually need to cover — not a checklist to port literally, since
  CNA/Easy3D has a completely different API shape.
- An old Urho3D-class → Simple3D-class mapping table (also deleted 2026-07-05 along with the rest
  of the completed migration's docs) is still recoverable from git history if needed purely as
  **behavioral reference** (what each class was responsible for), independent of which engine ends
  up implementing it.
- `NEXT.md` confirms the Simple3D build is functionally quite far along: world loading, textured
  terrain with animated tiles (lava/crusher/saw/spike/water/fan/marine/temp using mobile-eggbert's
  own frame tables), Blupi billboard with full state machine, mobile-object billboards, a crate
  push mechanic, platform patrol, hazard/kill detection, enemy stomp, respawn invincibility,
  pickups (treasure/keys/shield/egg/drink), exit-gate logic, HUD, save/load, and full 93-channel
  sound. This is real, working, faithful-to-mobile-eggbert gameplay — it is not being discarded
  because it's broken, but because the underlying engine dependency chain
  (Simple3D → Urho3D/Nova3D) is being abandoned in favor of CNA + Easy3D.

### 2.2 Existing Galaxy Eggbert Systems

`include/GalaxyEggbert/` and `src/GalaxyEggbert/Worlds/` are confirmed **fully engine-agnostic**:

- `Worlds/*.hpp` (`Block`, `BlockMetadata`, `Chunk`, `VoxelConfig`, `World`) include only
  `<cstdint>`, `<vector>`, `<filesystem>`, `<array>`, `<stdexcept>` — no graphics/engine headers.
- `def/*.hpp` (`BlupiAction`, `ContinueMissionType`, `DecorAction`, `Direction`, `DoorKeyFlags`,
  `GamePhase`, `GameSpeed`, `KeyPressFlags`, `ObjectType`, `SecretPower`, `SoundChannel`,
  `SpriteChannel`) and `BlockTypes.hpp`, `Def.hpp`, `GameConstants.hpp` are plain enums/constants.
- The world-file parser reads the identical text format used by mobile-eggbert
  (`DescFile: posDecor=...;... dimDecor=100;100 world=... music=... region=... blupiPos=...;...
  blupiDir=...` header line, then a `Decor:` marker, then a 100×100 grid of comma-separated tile
  codes).

This whole tree is portable as-is to the CNA/Easy3D target with no changes required — it does not
depend on Simple3D, Urho3D, or any rendering API at all.

`GalaxyEggbertWorldsTests` (54 gtest cases) links only this engine-agnostic tree and should
continue to build and pass unmodified under the new direction.

### 2.3 Existing Easy3D Capabilities

`../easy-3d` is a small, complete (not sprawling), C++23 static library, CMake target `easy3d`
(alias `easy3d::easy3d`), namespace `Easy3D`, version 0.1.0. Its own docs (`README.md`,
`docs/ARCHITECTURE.md`, `CLAUDE.md`) are explicit and self-consistent about being a **companion
library beside CNA**, not an engine and not a wrapper that hides CNA. It expects `../cna` as a
sibling providing a CMake target literally named `CNA`.

| Header/class | Confirmed state | CNA-coupled? |
|---|---|---|
| `Version.hpp/.cpp` | Fully implemented, trivial | No |
| `TextureAtlas.hpp/.cpp` (95 lines) | Fully implemented: `Add`, `AddGrid` (row-major spritesheet registration), `Contains`, `GetRect`, `GetUv`, `GetUvOrDefault`, `Count`, `Empty`; own `AtlasRect`/`UvRect` PODs | **No** — the most mature and most reusable-as-is class in Easy3D |
| `Camera3D.hpp/.cpp` (33 lines) | Real implementation: position/target/up/fov/aspect/near/far, `GetViewMatrix()`/`GetProjectionMatrix()` via CNA `Matrix::CreateLookAt`/`CreatePerspectiveFieldOfView` | Yes — compiles header-only, but executing view/projection math requires linking CNA |
| `OrbitCamera.hpp/.cpp` | Real implementation (yaw/pitch/distance → position), verified by test | Yes, same reason |
| `FollowCamera.hpp/.cpp` (67 lines) | Real implementation: frame-rate-independent exponential smoothing (`kSmoothingReferenceFps = 60`), verified by test | Yes, same reason |
| `BillboardBatch.hpp/.cpp` | CPU-side item queue only (`Add`, `Items`, `Begin`/`Clear`/`Empty`/`Count`) | No — but also **no rendering** |
| `CubeBatch.hpp/.cpp` | CPU-side item queue only | No — but also **no rendering** |
| `DebugDraw.hpp/.cpp` | CPU-side line/box queue only | No — but also **no rendering** |

Confirmed by direct grep across `include/` and `src/`: **zero occurrences** of shader, vertex
buffer, index buffer, `GraphicsDevice`, draw-primitives, `BasicEffect`, or `SpriteBatch` symbols.
Easy3D issues no GPU draw calls anywhere in the current codebase.

Tests (`test_basics`, `test_texture_atlas`, `test_camera`, `test_batches`, plus two
compile-only `*_compilecheck` targets) genuinely exercise the claimed behavior — nothing appears
mocked or vacuous. `docs/ROADMAP.md`/`NEXT.md` self-report Phase 0–2 (scaffold, cameras,
CPU-side batching/atlas) done, Phase 3 (CPU-side vertex builders) as the next, not-yet-started
task, and Phase 4 (actual CNA `GraphicsDevice` draw calls) as explicitly gated behind a "CNA
draw-path decision" that has not yet been made. `docs/QUESTIONS.md` records that Blupi is
intended to render as a 2D billboard from Mobile Eggbert sprites, and that billboard/cube/tile
rendering only (no 3D models) is the current decision for the near term — this matches the
constraint given for this migration task.

Easy3D's own `CLAUDE.md` explicitly instructs agents working in that repo not to modify `../cna`,
`../sharp-runtime`, `../mobile-eggbert`, or `../galaxy-eggbert`, and not to read/copy from
`../simple-3d`, `nova-3d`, or `mesh-craft` — consistent with the constraints given for this task.

### 2.4 Existing Mobile Eggbert Capabilities

`../mobile-eggbert` is a CNA-based C++ port, namespace `WindowsPhoneSpeedyBlupi`, depending on
`../cna` (via `add_subdirectory(../cna CNA)`), SDL3, SDL3_image, SDL3_mixer.

**Build-target reality check (critical):** the top-level `CMakeLists.txt` globs all of `src/*.cpp`
into a single `add_executable(WindowsPhoneSpeedyBlupi ...)`. There is no `add_library()` anywhere
(the Android build's `add_library(main SHARED ...)` is just SDL activity glue for the same
executable-equivalent code, not a reusable library). There is no sub-`CMakeLists.txt` under
`include/` or `src/`. **Mobile Eggbert cannot be linked as a CMake dependency today without a
mobile-eggbert-side change.**

| Component | Lines | Coupling | Assessment |
|---|---:|---|---|
| `Game1.hpp` | 955 | Owns `GraphicsDeviceManager`, `IPixmap`, `ISound`, `Decor`, `InputPad`, `GameData`, `Jauge`; drives the `Def::Phase` state machine; implements the XNA `Update`/`Draw` loop | Top-level orchestrator, heavily CNA-coupled — architecture reference only |
| `Decor.hpp` / `Decor.cpp` | 2,064 / 11,720 | Includes `GameData`, `IPixmap`, `ISound`, `Tables`, `System::Random`, `Jauge`, `DecorAction`, `DoorKeyFlags`, `ObjectType`, `PixmapChannel`; holds `m_pixmap`/`m_sound` as members | Canonical gameplay simulation — see §5.2 for detailed reuse assessment |
| `Pixmap.hpp` / `.cpp` (238/609 lines incl. `IPixmap.hpp`) | — | Includes CNA `Vector2`, `GraphicsDeviceManager`, `Graphics/SpriteBatch`, `SpriteEffects`, `Texture2D`, `Rectangle` directly | Confirmed 2D `SpriteBatch` wrapper — not reusable for a 3D renderer; useful only as a reference for which `PixmapChannel` maps to which PNG/frame layout |
| `Sound.hpp` / `.cpp` (134/427 lines incl. `ISound.hpp`) | — | Includes CNA `SoundEffect`, `SoundEffectInstance` directly; `PlayImage` takes a `TinyPoint` screen position (2D panning) | Confirmed CNA-audio-coupled; `SoundChannel` enum + index→`sound###.wav` mapping is reusable as data |
| `GameData.hpp` | 369 | Only includes `SharpRuntime/SharpRuntimeHelper.hpp` | **No engine coupling.** Documented flat byte-array layout: 10-byte global header + 3×210-byte gamer records = 640 bytes total. Good direct-reuse candidate for save-format compatibility |
| `Tables.hpp` / `.cpp` | 1,019 / 2,208 | Header only includes `SharpRuntimeHelper.hpp` | **No engine coupling.** Dozens of flat `static const shortcs table_*[N]` arrays (Blupi animation frames, per-enemy-type movement/turn tables, explosion frames, decor rotation data). Strong direct-reuse candidate |
| `Def.hpp` + `def/*.hpp` + `decor/*.hpp` | ~215 + several small files | Only `SharpRuntimeHelper.hpp` | **No engine coupling.** `Phase`, `ButtonGlyph`, layout constants (`LXIMAGE`, `MAXCELX`/`MAXCELY`, `DIMOBJX/Y`, etc.), `ObjectType` (stored as byte, ID layout must not change — matches level file format), `SoundChannel` (0..92, matches 93 `sound*.wav` files). Good direct-reuse/reference candidates |

Assets confirmed present:

- `Content/icons/`: `blupi.png`, `blupi1.png`, `button.png`, `element.png`, `explo.png`,
  `jauge.png`, `object-m.png`, `pad.png`, `text.png` (9 files, ~4.7 MB), plus a parallel
  `Content/icons4x/` with the same 9 filenames.
- `Content/backgrounds/`: `blupiyoupie.png`, `decor000.png`…`decor0XX.png`.
- `Content/sounds/`: 93 files, `sound000.wav`…`sound092.wav`.
- `worlds/`: 78 files, `worldNNN.txt` (non-contiguous numbering — some IDs unused/reserved).

Sample confirmed format (`world001.txt`):
```
DescFile: posDecor=250;5570 dimDecor=100;100 world=0 music=0 region=0 blupiPos=770;5894 blupiDir=2
Decor:
,,,,,,,...,10,10,10,10,...
```
This is identical in shape to galaxy-eggbert's own `worlds/world001.txt`.

---

## 3. Target Architecture

```
Galaxy Eggbert (new CNA/Easy3D source tree)
  -> CNA directly (Microsoft::Xna::Framework::*, GraphicsDevice, Game loop)
  -> Easy3D beside CNA (Camera3D/OrbitCamera/FollowCamera, TextureAtlas,
     BillboardBatch/CubeBatch/DebugDraw item queues)
  -> mobile-eggbert used read-only for:
       - assets (icons, sounds, worlds, backgrounds)
       - header/table knowledge (Tables, Def, ObjectType, SoundChannel, GameData layout)
       - behavioral reference (Decor.cpp as the canonical simulation to port logic *from*,
         not to link *to*, at least in the first phase)
```

Easy3D does not sit between Galaxy Eggbert and CNA. Galaxy Eggbert code is free to call CNA APIs
directly (`GraphicsDevice`, `SpriteBatch` or a custom draw path, `Texture2D`, input, audio) and
call into Easy3D only where a helper genuinely saves repetition (camera math, atlas UV lookup,
batching queues). This mirrors the conceptual example given in the task brief:

```cpp
Microsoft::Xna::Framework::GraphicsDevice* graphics = ...;  // CNA, direct
Easy3D::Camera3D camera;                                     // Easy3D helper
Easy3D::BillboardBatch billboards;                            // Easy3D helper
```

---

## 4. Non-Goals

Explicitly out of scope for this migration (both for this analysis task and, per the brief, for
the first playable milestone):

- No giant Entity/Component framework in Easy3D or Galaxy Eggbert.
- No new full game engine, no Urho3D-style scene graph, no Nova3D abstraction.
- No resource cache, editor, physics engine, networking, or navigation in Easy3D.
- No MeshCraft or Mesh World integration.
- No Lua in the first phase (undecided; may be discussed later as an optional separate module).
- No model importer or PBR renderer in the first phase.
- No 3D Blupi model requirement — Blupi may ship as a 2D billboard using Mobile Eggbert sprite
  frames for the entire first playable version.
- No deletion of `src/GalaxyEggbertSimple3D/` — it remains as historical reference until the
  CNA/Easy3D path reaches parity or the user decides otherwise.
- No modification of `mobile-eggbert`, `../cna`, `../easy-3d` (beyond what the user explicitly
  approves after reading this document), `../simple-3d`, `../nova-3d`, `../mesh-craft`, or
  `../mesh-world`.
- No CMake migration implemented yet in this task, beyond documenting target names as a plan.

---

## 5. Mobile Eggbert Reuse Strategy

### 5.1 Read-only rule

Mobile Eggbert is read-only for this migration. If a Mobile Eggbert change would help, Claude Code
must stop and ask the user. The default migration approach is to adapt Galaxy Eggbert around
Mobile Eggbert, not to reshape Mobile Eggbert for Galaxy Eggbert. This applies even to changes
that look small or "obviously good" (e.g., adding an `add_library()` target) — they still require
explicit user approval before being implemented, in mobile-eggbert, by a separate task.

### 5.2 Direct library reuse candidates

**None are available today**, because mobile-eggbert exposes no CMake library target. If the user
approves adding one to mobile-eggbert (a mobile-eggbert-side change, out of scope here), the
next-best direct-link candidates, ranked by how self-contained they are, would be:

| Candidate | Self-contained? | Notes |
|---|---|---|
| `Tables` | Yes | Pure static data, zero engine coupling in the header |
| `Def` + `def/*` + `decor/*` enums | Yes | Pure enums/constants |
| `GameData` | Yes | Only depends on `SharpRuntimeHelper.hpp` |
| `Decor` | No | Requires concrete `IPixmap`/`ISound` implementations to instantiate; also depends on `Tables`, `GameData`, `System::Random`, `Worlds.hpp` |
| `Pixmap`/`Sound` | No | Directly coupled to CNA `SpriteBatch`/`SoundEffect` — a 3D remake would not want these anyway |

### 5.3 Asset reuse candidates

All confirmed present in `../mobile-eggbert/Content/` and `../mobile-eggbert/worlds/`:

| Mobile Eggbert thing | Direct reuse? | Why / why not | Proposed Galaxy strategy |
|---|---:|---|---|
| `Content/icons/blupi.png`, `blupi1.png` | Yes | Plain asset file | Use directly for Blupi billboard frames |
| `Content/icons/element.png` | Yes | Plain asset file | Use directly for pickups/enemies/mobile objects as billboards |
| `Content/icons/object-m.png` | Yes | Plain asset file | Use directly for terrain tile textures |
| `Content/icons/pad.png`, `jauge.png`, `button.png`, `text.png` | Yes | Plain asset files | Use directly for HUD/menu once that phase is reached |
| `Content/icons4x/*` | Yes (optional) | Higher-res variant set | Consider for higher-DPI displays later; not required for first playable |
| `Content/backgrounds/*.png` | Yes | Plain asset files | Use for level-specific backgrounds/skybox-equivalent |
| `Content/sounds/sound000.wav`…`sound092.wav` (93 files) | Yes | Plain WAV files, index matches `SoundChannel` enum | Load by same index, same filenames |
| `worlds/worldNNN.txt` (78 files) | Yes | Identical text format to galaxy-eggbert's own world files | Load with the existing engine-agnostic `Worlds` parser |

### 5.4 Animation/table reuse candidates

| Mobile Eggbert thing | Direct reuse? | Why / why not | Proposed Galaxy strategy |
|---|---:|---|---|
| `Tables::table_blupi[2911]` | Maybe | Pure static data, no engine coupling — but reuse requires either linking mobile-eggbert (not possible today) or copying the array | Prefer direct link if the user later approves adding a library target to mobile-eggbert; otherwise this is a "copy/adapt after user approval" item, not something to do in this task |
| `Tables::table_mirror`, `table_vitesse_*`, `table_decor_quart`, per-enemy movement/turn tables, `table_explo1..6` | Maybe | Same reasoning as above | Same — direct link preferred, copy requires approval |
| `Def::Phase`, `ButtonGlyph`, layout constants (`MAXCELX`, `DIMOBJX/Y`, etc.) | Maybe | Tiny, stable, no engine coupling | Low-risk copy/adapt candidate, but still requires explicit user approval per the task rules before any code is copied |
| `ObjectType` enum | Maybe | No engine coupling, but IDs are load-bearing (stored in level files) and must match exactly | If copied, must be copied verbatim, byte-for-byte identical to preserve level-file compatibility — flag as approval-required |
| `SoundChannel` enum | Maybe | No engine coupling, IDs match WAV filenames | Same as above |

### 5.5 Sound reuse candidates

| Mobile Eggbert thing | Direct reuse? | Why / why not | Proposed Galaxy strategy |
|---|---:|---|---|
| `sound*.wav` files | Yes | Plain assets | Load directly by path/index |
| `SoundChannel` enum / index mapping | Maybe | Pure data, but see §5.4 | Copy/adapt after approval, or re-derive independently since galaxy-eggbert already has its own `include/GalaxyEggbert/def/SoundChannel.hpp` (needs cross-check for exact parity — see open questions) |
| `Sound`/`ISound` classes | Probably no | Confirmed CNA-`SoundEffect`-coupled and 2D-panning-coupled (`TinyPoint pos`) | Not reusable as-is; Galaxy Eggbert needs its own thin CNA audio wrapper informed by, not copied from, this code |

### 5.6 World/save-data reuse candidates

| Mobile Eggbert thing | Direct reuse? | Why / why not | Proposed Galaxy strategy |
|---|---:|---|---|
| `worlds/worldNNN.txt` | Yes | Format already confirmed identical to galaxy-eggbert's own world files | Load directly with the existing `GalaxyEggbert::Worlds` parser |
| `GameData` byte layout (640-byte flat array, 10-byte header + 3×210-byte gamer records) | Maybe | No engine coupling, but reuse means either linking the class (not possible today) or reimplementing the same byte layout | If save-file compatibility with mobile-eggbert is desired, document the layout and reimplement it in galaxy-eggbert; treat as a "copy/adapt after approval" item since it involves transcribing byte offsets from mobile-eggbert source |

### 5.7 Things that should not be copied without user approval

- Any `Tables.cpp` array contents (large, load-bearing, easy to introduce subtle transcription
  bugs).
- `ObjectType` and `SoundChannel` enum values (must stay byte-identical to level/asset data; a
  transcription mistake would silently corrupt level parsing).
- `GameData` byte-offset layout (same risk).
- Any `Decor.cpp` method bodies, even partial (huge, subtly coupled to `IPixmap`/`ISound`
  side-effects — see §5.2/§6.4 for why this is reference-only in the first phase).

---

## 6. Simple3D Code Fate

### 6.1 Keep as historical reference

`src/GalaxyEggbertSimple3D/` in its entirety. It documents real, working, faithful gameplay
behavior (per §2.1) and remains the best available reference for "what does correct
galaxy-eggbert behavior look like" even after the engine underneath changes.

### 6.2 Systems to rewrite

Everything in `Game/*.cpp` except `GEBridgeSystem` will need a CNA/Easy3D-native rewrite, since
their current implementations call Simple3D APIs (`Entity::SetTileTexture`, `Camera::Shake`,
`UI::ProgressBar`, etc.) that have no CNA/Easy3D equivalent. The *behavior* they implement (per
§6.1's capability checklist above and `NEXT.md`) is the porting target; the *code* is not reusable
verbatim.

### 6.3 Systems to discard

None outright — even Simple3D-coupled code remains useful as line-by-line behavioral reference
until the CNA/Easy3D path independently reaches the same feature. No deletion is proposed in this
task.

### 6.4 Systems whose logic can inspire the new CNA/Easy3D path

- `GEBridgeSystem` — already engine-agnostic; likely portable with zero or near-zero changes.
- `GEWorldRuntime` — the *shape* of "load a `World`, walk its `Chunk`s, emit renderable tile
  requests" is a good pattern to replicate against Easy3D's `CubeBatch`, even though the concrete
  Simple3D calls must be replaced.
- `GEBlupiController`, `GEDecorSystem`, `GEExploSystem` — their state-machine structure (matching
  mobile-eggbert's `BlupiAction`/`ObjectType` behavior) is worth mirroring; their rendering calls
  are not.
- `GEHud`, `GESound`, `GECameraRig` — smallest/most isolated pieces; useful as a checklist of what
  needs a CNA/Easy3D equivalent (input-bound camera, 93-channel audio, HUD text/gauges), not as
  code to port.

---

## 7. Easy3D Role

### 7.1 What belongs in Easy3D

Per Easy3D's own `CLAUDE.md`/`ARCHITECTURE.md` (confirmed, not just assumed) and consistent with
this task's brief:

- Cameras (`Camera3D`, `OrbitCamera`, `FollowCamera`) — already present.
- `TextureAtlas` — already present, already CNA-free and mature.
- CPU-side batch queues (`BillboardBatch`, `CubeBatch`, `DebugDraw`) — already present.
- CPU-side vertex builders that turn queued items into vertex/index arrays — **missing, needed**.
- CNA renderer adapters (turning vertex arrays into actual `GraphicsDevice` draw calls) —
  **missing, needed**, and per Easy3D's own roadmap this requires a "CNA draw-path decision" that
  has not yet been made.

### 7.2 What does not belong in Easy3D

Per the task brief and confirmed by Easy3D's own non-goals list in `docs/ARCHITECTURE.md`: ECS,
full game engine, Urho3D-style scene graph, Nova3D-style abstraction, resource cache, editor,
physics, networking, navigation, MeshCraft import, Mesh World integration, Lua (first phase),
model importer (first phase), PBR renderer (first phase).

### 7.3 Easy3D gaps needed by Galaxy Eggbert

Confirmed missing today, all required before Galaxy Eggbert can render anything through Easy3D:

- CPU-side vertex builders for billboard quads (turn `BillboardBatch::Items()` into vertex data).
- CPU-side vertex builders for cube/tile terrain (turn `CubeBatch::Items()` into vertex data).
- CPU-side debug line/box geometry generation (turn `DebugDraw` queues into line-list vertex data).
- A CNA renderer adapter for billboard batches (issue actual `GraphicsDevice` draw calls).
- A CNA renderer adapter for cube batches.
- A CNA renderer adapter for debug draw.
- A texture loading/ownership strategy — likely CNA-side (`Texture2D`), not Easy3D-side, since
  Easy3D's `TextureAtlas` only stores UV rectangles by name, not actual texture data.

These gaps can be filled either inside Easy3D itself (if the user wants Easy3D to grow a minimal
renderer) or as a thin adapter layer living in Galaxy Eggbert's own new source tree. This is an
open question (see §12) — Easy3D's own roadmap treats it as an open decision too, not something
this task should decide unilaterally.

---

## 8. CNA Role

### 8.1 What Galaxy Eggbert should use directly from CNA

- `Game` / fixed-timestep loop (mobile-eggbert's own `Game1` and its `CNA/Game.cpp`
  fixed-timestep accumulator are useful reference for matching gameplay speed across platforms).
- `GraphicsDeviceManager`, `GraphicsDevice`.
- `Texture2D` loading and ownership.
- Input (keyboard/gamepad — mirroring mobile-eggbert's `InputPad` usage pattern, not its code).
- Audio playback primitives (`SoundEffect`/`SoundEffectInstance`), informed by mobile-eggbert's
  `Sound`/`ISound` design but not copied from it.
- `Vector2`/`Vector3`/`Matrix`/`Rectangle` math types (already used throughout Easy3D).

### 8.2 What should remain in CNA rather than Easy3D

Anything that is a direct 1:1 wrapper of a single CNA type or call (texture loading, sound
playback, raw input polling) should stay as Galaxy-Eggbert-side CNA usage, not be pulled into
Easy3D as a new abstraction — consistent with "Easy3D must not become Simple3D again."

---

## 9. Proposed New Galaxy Eggbert Source Layout

Two naming options were considered, as requested by the task brief:

**Option A: `src/GalaxyEggbertCNA/`, target `GalaxyEggbertCNA`**
**Option B: `src/GalaxyEggbertEasy3D/`, target `GalaxyEggbertEasy3D`**

**Recommendation: Option A (`GalaxyEggbertCNA`).**

Reasoning: the target architecture is "CNA directly, with Easy3D as one of several helpers beside
it" (§3) — CNA is the foundational dependency (game loop, graphics device, audio, input); Easy3D
is one helper library among what could later be several. Naming the tree/target after CNA
matches the actual dependency relationship and avoids implying Easy3D is the primary engine layer
(which would risk recreating the "Simple3D" framing the project is explicitly moving away from).
The precedent of `GalaxyEggbertSimple3D` being named after its primary engine dependency supports
using the same convention: name the target after what it fundamentally runs on (CNA), not after
one of its helper libraries (Easy3D).

This recommendation should be confirmed with the user before implementation (see §12).

Proposed layout, mirroring the existing `GalaxyEggbertSimple3D` structure:

```
src/GalaxyEggbertCNA/
  GalaxyEggbertCNAGame.hpp / .cpp     — top-level CNA Game subclass, owns the loop
  Game/
    GEWorldRuntime.hpp / .cpp         — loads World via existing engine-agnostic parser
    GETerrainRenderer.hpp / .cpp      — CubeBatch-based terrain, CNA draw adapter
    GEBlupiController.hpp / .cpp      — Blupi state machine + billboard rendering
    GEDecorSystem.hpp / .cpp          — object/pickup/enemy billboards
    GEHud.hpp / .cpp
    GESound.hpp / .cpp                — thin CNA audio wrapper
    GECameraRig.hpp / .cpp            — wraps Easy3D::OrbitCamera/FollowCamera
    GEExploSystem.hpp / .cpp
    GEBridgeSystem.hpp / .cpp         — likely near-verbatim port from Simple3D version
```

`include/GalaxyEggbert/` (Worlds, def/, BlockTypes, Def, GameConstants) is reused unchanged by
both `GalaxyEggbertSimple3D` and the new `GalaxyEggbertCNA` target.

### CMake integration notes

- Confirmed: Easy3D auto-detects an existing `CNA` CMake target from a parent project, so the
  intended integration order is `add_subdirectory(../cna)` then `add_subdirectory(../easy-3d)`
  inside galaxy-eggbert's own `CMakeLists.txt`, then `add_executable(GalaxyEggbertCNA ...)`
  linking both `CNA` and `easy3d`.
- Confirmed: mobile-eggbert cannot be added via `add_subdirectory` as a *library* dependency today
  (§5, §2.4) — only as an executable, which is not useful for reuse. Non-invasive options for
  asset reuse specifically (not code reuse):
  - Read mobile-eggbert assets directly from a sibling path at runtime
    (`../mobile-eggbert/Content/...`, `../mobile-eggbert/worlds/...`) — simplest, but couples the
    galaxy-eggbert runtime to a specific sibling checkout existing on disk.
  - Copy assets into galaxy-eggbert's own `Content`/`worlds` directories at build time via a
    CMake custom command (`file(COPY ...)` or `add_custom_command`) — more robust for
    distribution, but duplicates ~5 MB+ of binary assets in the galaxy-eggbert tree/build output.
  - Symlink assets — avoids duplication but is fragile on Windows and in CI without extra setup.
  - This is called out explicitly as an open decision in §12, not decided here.
- `GalaxyEggbertSimple3D` target and its `GALAXY_EGGBERT_BUILD_SIMPLE3D` option remain untouched
  and default `ON`.

---

## 10. Phased Migration Plan

See `plan.md`, new section **"Direct CNA + Easy3D Migration"**, for the full checkbox task list
with `E3D-MIG-*` IDs. Summary of phases:

0. Documentation and direction lock (this document + `plan.md` update — done by this task).
1. Build integration investigation (CNA/Easy3D/mobile-eggbert CMake requirements — mostly done by
   this analysis, formalized as tasks).
2. New target skeleton (`GalaxyEggbertCNA`, empty window, clear color, no gameplay).
3. Asset path strategy and first asset reuse.
4. World loading in the new target.
5. Easy3D terrain path (requires filling the vertex-builder/CNA-adapter gap from §7.3).
6. Blupi first version (2D billboard, no 3D model).
7. Object/decor first version (billboards from `element.png`).
8. Sound.
9. HUD.
10. Gameplay parity pass.
11. Retire Simple3D path (later, not now).
12. Optional future work (3D Blupi model, camera switching, Lua discussion, renderer polish).

---

## 11. Risks

- **Easy3D rendering gap is the critical path.** Nothing in the current stack (Easy3D, CNA usage
  in mobile-eggbert, or galaxy-eggbert) demonstrates cube/billboard rendering today. This is new
  work, not integration work, and is likely the single largest risk to schedule.
- **mobile-eggbert asset/table reuse is blocked on either a mobile-eggbert change (needs
  approval) or manual transcription (error-prone, especially for `ObjectType`/`SoundChannel`
  IDs and `Tables` arrays, which are load-bearing against level file data).**
- **Decor.cpp is not a reusable component.** At 11,720 lines with `IPixmap`/`ISound` members
  threaded through simulation methods, it can only be used as a *behavioral reference* to reimplement
  from, not linked or lifted. Porting gameplay parity (Phase 10) will be a large, incremental
  effort regardless of which engine is underneath — this was already true for the Simple3D port
  and will be true again for the CNA/Easy3D port.
- **Two competing sets of project docs.** `README.md` (CNA-oriented) vs. `CLAUDE.md`/`NEXT.md`/
  `plan.md` (Simple3D-oriented) currently disagree about what the current/target build is. Until
  reconciled, new contributors (human or agent) reading only one of these files will get a wrong
  picture.
- **Asset duplication vs. sibling-path coupling** (§9) is an unresolved tradeoff with real
  consequences for distribution and CI; picking wrong now could mean redoing asset pipeline work
  later.

---

## 12. Open Questions for the User

1. **Target/source-tree naming:** confirm or override the recommendation in §9
   (`GalaxyEggbertCNA` / `src/GalaxyEggbertCNA/`) vs. the `Easy3D`-named alternative.

   > **Resolved (2026-07-01):** `GalaxyEggbertCNA` / `src/GalaxyEggbertCNA/`, exactly as
   > recommended — this is what actually got built.
2. **Mobile Eggbert library target:** would you like to separately approve a future
   mobile-eggbert-side task to add an `add_library()` target (e.g. for `Tables`, `Def`,
   `GameData`, `ObjectType`/`SoundChannel` enums only — not `Decor`/`Pixmap`/`Sound`) so those
   pieces can be linked instead of copied? This would need to be its own approved task against
   `../mobile-eggbert`, not part of this migration.

   > **Still open as of 2026-07-09** (`plan.md` `E3D-MIG-015`) — mobile-eggbert has no
   > `add_library()` target today; nothing has changed here. Not blocking current work, since
   > `ObjectType`/`SoundChannel` turned out to already be numerically identical (see Q4 below) —
   > no reuse-via-linking is actually needed yet.
3. **Asset strategy:** sibling-path read at runtime, build-time copy, or symlink, for reusing
   `../mobile-eggbert/Content/*` and `../mobile-eggbert/worlds/*` (§9, CMake integration notes)?

   > **Resolved (2026-07-01, implemented `E3D-MIG-030`):** build-time copy, via a `POST_BUILD`
   > `copy_directory` step in `CMakeLists.txt` — exactly the recommended default. Symlinks were
   > rejected as fragile on Windows/CI, matching this document's own §9 leaning.
4. **Copy-with-approval items:** do you want to proceed (in a future task) with copying
   `ObjectType`, `SoundChannel`, `Def` constants, and/or `GameData`'s byte layout into
   `include/GalaxyEggbert/` verbatim, given galaxy-eggbert already has its own
   `include/GalaxyEggbert/def/ObjectType.hpp` and `SoundChannel.hpp`? These need a parity check
   against mobile-eggbert's versions — are they already identical, or do they need to be
   reconciled?

   > **`ObjectType`/`SoundChannel` resolved (2026-07-01, `E3D-MIG-039`):** already identical —
   > both sides declare the same 204/93 numeric IDs, verified programmatically. No copying needed;
   > nothing was changed in either repository. **`GameData`'s byte layout is still open** — see Q7
   > below, unchanged.
5. **Where should the Easy3D rendering gap (§7.3) be filled** — inside `../easy-3d` itself (its
   own roadmap Phase 3/4), or as a thin adapter living only in `GalaxyEggbertCNA`? This affects
   whether `../easy-3d` needs a follow-up task of its own.

   > **Decision (2026-07-02, recorded as `plan.md` `E3D-MIG-050`):** inside `../easy-3d` itself.
   > Cross-checking easy-3d's own `docs/ROADMAP.md` (Phase 3 "CPU-side vertex builders", Phase 4
   > "CNA renderer adapters") and `NEXT.md` §8 item 8 showed that repo's own plan already scopes
   > this work as its own, independent of Galaxy Eggbert — it is generic billboard/cube/debug
   > geometry-and-draw-call plumbing with no Eggbert-specific knowledge, matching
   > `docs/ARCHITECTURE.md`'s stated helper role. Galaxy Eggbert stays the owner of all
   > Eggbert-specific meaning (tile IDs, `ObjectType`, animated-tile frames) and only calls the
   > new Easy3D functions. This is a documentation decision only — implementing it still requires
   > separate, explicit user approval to modify `../easy-3d` (a sibling repo).
6. **Documentation reconciliation:** should `README.md`'s aspirational CNA build instructions be
   rolled back to match current Simple3D reality until the new target exists, or should
   `CLAUDE.md`/`NEXT.md`/`plan.md` be updated now to describe the CNA/Easy3D target as the
   near-term direction (while noting it isn't buildable yet)? This task updates `plan.md` per its
   instructions but leaves `README.md`/`CLAUDE.md`/`NEXT.md` untouched pending your decision.

   > **Resolved, long since overtaken by events:** the second option, and then some —
   > `CLAUDE.md`/`NEXT.md`/`plan.md`/`README.md` all now consistently describe `GalaxyEggbertCNA`
   > as the actively developed target (not just "near-term direction") and
   > `GalaxyEggbertSimple3D` as historical reference only (locked 2026-07-05, Simple3D frozen
   > 2026-07-08) — a full documentation pass across all of these plus `ANDROID.md`/`WINDOWS.md`
   > confirmed this consistency 2026-07-09.
7. **Save-data compatibility:** is byte-level save compatibility with mobile-eggbert's
   `GameData` format (640-byte layout) actually a goal, or is a fresh save format acceptable for
   galaxy-eggbert? This affects whether §5.6's reuse strategy is worth pursuing at all.

   > **Still open as of 2026-07-09** — no save system exists yet in `GalaxyEggbertCNA` at all
   > (Phase 9/10 in `plan.md`, not started), so this hasn't needed answering yet.
8. **Lua:** confirmed out of scope for the first migration per the task brief — no action needed
   now, but flagging that `docs/QUESTIONS.md` in `../easy-3d` treats it as open too, so the two
   repos' timelines should probably be discussed together later.
