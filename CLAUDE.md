# Galaxy Eggbert — Claude Code Guidelines

## Project Overview

**Galaxy Eggbert** is a faithful 3D remake of **mobile-eggbert** (itself a faithful C++ port of the original *Speedy Blupi*, a Windows Phone XNA game from 2013).

`GalaxyEggbertSimple3D` is written in C++ using the **Simple3D** API (`../simple-3d`), which
currently wraps **U3D** (`u3d-community/U3D`, a Urho3D fork). It was the working, playable target
through 2026-07-08 — see `NEXT.md` for its status. **As of 2026-07-08, it is historical reference
only: do not build, fix, or troubleshoot it (see "Current Direction Lock" below).** Galaxy Eggbert
will run only on CNA long-term, and Simple3D will be gradually removed as `GalaxyEggbertCNA`
matures.

The **long-term implementation**, `GalaxyEggbertCNA`, now exists as an early-stage, opt-in build
target (`src/GalaxyEggbertCNA/`) — it builds, opens a window, and renders real, textured,
animated 3D terrain from a hand-authored world, but has no Blupi/object rendering, HUD, sound, or
gameplay yet, and is far from feature parity with `GalaxyEggbertSimple3D`. See `NEXT.md` §2 for
its current status and "Current Direction Lock" below for the rules governing it.

## Current Direction Lock

**The long-term target is Direct CNA + Easy3D.**

```text
Galaxy Eggbert
  -> CNA directly
  -> Easy3D beside CNA (small helpers only — cameras, texture atlas, billboard/cube batching)
  -> mobile-eggbert used read-only as reference / asset / data source
```

This supersedes the old long-term direction:

```text
Galaxy Eggbert -> Simple3D -> U3D / Urho3D / Nova3D -> (CNA someday)
```

**`GalaxyEggbertCNA` is the sole intended end state (updated 2026-07-05).** Galaxy Eggbert will
run only on CNA — `GalaxyEggbertSimple3D` is a transitional target, not a permanent one. It will be
**gradually removed** from galaxy-eggbert as `GalaxyEggbertCNA` gains equivalent functionality,
piece by piece, not kept indefinitely "as reference." Its code stays intact for now — do not delete
any of it without an explicit removal task from the user — but **as of 2026-07-08, do not build,
fix, or otherwise maintain it: it is treated as historical/behavioral reference only, not an
actively working target.** (Its build is currently broken in this environment — missing/
incompatible U3D prebuilt — and that is intentionally left unfixed.) Reading its source for
reference (e.g. porting logic to `GalaxyEggbertCNA`) remains fine.

**Before doing any migration work, read `easy3d.md` (full analysis) and `plan.md` (section
"Direct CNA + Easy3D Migration", task IDs `E3D-MIG-*`).**

Rules for this direction:

- Do not build, fix, or invest in the Simple3D/U3D/Nova3D direction at all (updated 2026-07-08,
  per user) — `GalaxyEggbertSimple3D` is historical/behavioral reference only now, not a
  maintained target. No bug fixes, no build-environment troubleshooting.
- Do not modify `../mobile-eggbert` without explicit user approval.
- Do not refactor mobile-eggbert for Galaxy Eggbert, and do not split it into a `core` library.
- Do not copy mobile-eggbert code or data (tables, enums, save-format byte layout, sprite/frame
  logic, etc.) into Galaxy Eggbert without explicit user approval, even if it looks like "just
  data." See `easy3d.md` §5.7 for specific examples of load-bearing data that must not be
  transcribed casually.
- Reusing mobile-eggbert *assets* (PNG sprite sheets, sounds, world files) by sibling path or
  build-time copy is allowed only once the asset strategy task is approved/implemented — do not
  wire up ad hoc asset paths outside that plan.
- Prefer new CNA/Easy3D code under `src/GalaxyEggbertCNA/` (see "Source layout" below).
- Do not mutate `src/GalaxyEggbertSimple3D/` into the CNA implementation. Keep it intact for now
  (historical reference, not built/maintained as of 2026-07-08); the CNA path is new code in a new
  tree. Its long-term fate is gradual removal as CNA gains equivalent capability, not permanent
  retention — but don't remove any of it without an explicit removal task.
- Easy3D is a helper library beside CNA — do not hide CNA behind Easy3D, and do not let Easy3D
  grow into a scene graph / ECS / engine. See `easy3d.md` §7 for what does and does not belong in
  Easy3D.
- Do not add Lua unless explicitly requested by the user.
- Do not add MeshCraft, Mesh World, Nova3D, or further Simple3D features back into the active
  target path.
- `../simple-3d` may be **read** freely when actually needed (e.g. understanding a behavioral
  reference in `GalaxyEggbertSimple3D`, which is itself historical reference only as of
  2026-07-08 — not built/fixed) — do not read it reflexively/speculatively, only when the task
  at hand genuinely requires it. **Never modify `../simple-3d`.**

## CRITICAL RULE — Faithful Remake

**Galaxy Eggbert is a faithful 3D remake of mobile-eggbert. Nothing more.**

Before suggesting or implementing ANY feature, verify it exists in mobile-eggbert first.
If it is not in mobile-eggbert, do NOT implement it and do NOT suggest it.

Examples of things that must NOT be added (do not exist in mobile-eggbert):
- Coins / mince
- Time bonuses / star ratings
- Coyote time / wall jump / double jump
- Combo multipliers
- Any mechanic invented by Claude

The 3D dimension allows only natural technical adaptations: perspective camera, billboard sprites,
shadow, step-up traversal. Game logic, objects, enemies, and pickups must mirror mobile-eggbert exactly.

When suggesting next tasks after completing work, only suggest tasks that port features
already verified to exist in mobile-eggbert (`/rv/data/development/github.com/openeggbert/mobile-eggbert`).

Build target selection (actual, matches `CMakeLists.txt`; `GALAXY_EGGBERT_BUILD_SIMPLE3D` still
defaults `ON` in `CMakeLists.txt`, but per the historical-reference-only status above, do not build
this target — shown here only for completeness):
```
cmake -S . -B cmake-build-debug -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON   # default ON — historical reference only, do not build/fix (2026-07-08)
cmake --build cmake-build-debug --target GalaxyEggbertSimple3D
```

Early-stage, opt-in (default OFF — not at feature parity with Simple3D yet, see "Current
Direction Lock" above and `NEXT.md` §2 for current status):
```
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA   # must run from its own build dir (relative asset paths)
```

## Relationship to mobile-eggbert

**mobile-eggbert** (`/rv/data/development/github.com/openeggbert/mobile-eggbert`) is the primary
reference and inspiration. It is a C++ port of the original Windows Phone Speedy Blupi, ported
path: C# (ILSpy decompile) → MonoGame → C++ with CNA (XNA-compatible SDL3 framework).

**mobile-eggbert is read-only for this migration.** Per "Current Direction Lock" above, do not
copy code or data from it into Galaxy Eggbert without explicit user approval — this applies to
every row below. Where reuse is possible without copying (assets, or a future approved library
link), that is called out explicitly.

| Category | mobile-eggbert source | Reuse plan for galaxy-eggbert |
|---|---|---|
| Sprite sheets | `Content/icons/` and `Content/icons4x/` | Direct asset reuse (no copying of code) — texture source for tile/billboard rendering: `blupi.png`, `element.png`, `explo.png`, `object-m.png`, `button.png` |
| Level backgrounds | `Content/backgrounds/decor000.png` … | Direct asset reuse — level-specific background/skybox |
| Sounds | `Content/sounds/sound000.wav` … `sound092.wav` | Direct asset reuse — same WAV files, same indices |
| World files | `worlds/world001.txt` … | Direct asset reuse — same text format, read by the existing engine-agnostic `GalaxyEggbert::Worlds` parser |
| Game enums (`BlupiAction`, `Direction`, `SecretPower`, `GameSpeed`, `KeyPressFlags`, `ObjectType`, `SoundChannel`, decor actions) | `include/WindowsPhoneSpeedyBlupi/def/`, `decor/` | Behavioral/ID reference only — galaxy-eggbert has its own `include/GalaxyEggbert/def/*.hpp` equivalents; copying/reconciling IDs requires explicit user approval (see `easy3d.md` §12 Q4) |
| Level constants (`MAXCELX=100`, `MAXCELY=100`, etc.) | `Def.hpp` | Reference only — galaxy-eggbert's own `GameConstants.hpp` already tracks the grid dimensions |
| Gameplay logic | `Decor.cpp` / `Decor.hpp` | **Canonical behavioral reference only for actual GalaxyEggbert game code** (`src/GalaxyEggbertSimple3D/`, `src/GalaxyEggbertCNA/`) — at ~11,700 lines with `IPixmap`/`ISound` members threaded through simulation methods, it is not a reusable component, see `easy3d.md` §5.2/§6.4; study it, do not link or copy it into those trees. **Exception, approved 2026-07-05:** a written, prose behavioral specification (with key numeric constants — speeds, timers, ranges; not pseudocode, not a verbatim/line-by-line transcription) of mobile-eggbert's gameplay logic may be added to `mobile-eggbert-reference/`, covering: `ObjectType` Category A+B (real behavior, ~70 IDs) and core Blupi mechanics (movement/jump/gravity, secret powers, gauge/lives, doors+keys, save behavior). This is documentation of behavior, not a license to copy this logic into actual game code — implementing any of it in `GalaxyEggbertCNA`/`GalaxyEggbertSimple3D` is a separate decision each time. |
| Animation/movement tables | `Tables.cpp` | Reference only unless a future approved mobile-eggbert library target makes direct linking possible (`easy3d.md` §12 Q2); do not transcribe array contents without approval |
| Save data | `GameData.cpp` | Reference only — reusing the byte-level layout for save compatibility is an open question (`easy3d.md` §12 Q7), not a default |

**mobile-eggbert is never modified, including for analysis purposes** (clarified 2026-07-05): if
a task genuinely needs a working copy to mark up/annotate/experiment with, copy the file into
`galaxy-eggbert` first (e.g. a scratch location, clearly not part of the build) and work on that
copy — never edit anything under `../mobile-eggbert` itself, even temporarily.

### mobile-eggbert build-target reality

mobile-eggbert's `CMakeLists.txt` currently defines only `add_executable(WindowsPhoneSpeedyBlupi
...)` — there is no `add_library()` target. It **cannot be linked as a CMake dependency today**
without a mobile-eggbert-side change, which would itself require explicit user approval as a
separate task. Do not assume or claim otherwise.

## Engine rules

- **No `#ifdef` guards for engine differences** in `GalaxyEggbertSimple3D` or
  `GalaxyEggbertCNA` — each target speaks its own API directly (Simple3D, or CNA+Easy3D
  respectively). If a backend beneath Simple3D is missing a feature, that is a `simple-3d`-repo
  problem, not something to work around in galaxy-eggbert.
- Public headers (used by tests) go in `include/`; private implementation headers go in `src/`.
- `include/GalaxyEggbert/Worlds/`, `def/*.hpp`, `BlockTypes.hpp`, `Def.hpp`, `GameConstants.hpp`
  are engine-agnostic and shared by both the Simple3D and CNA targets. Do not add engine-specific
  dependencies to this tree.

## Source layout

```
include/GalaxyEggbert/
  Worlds/          — data model: Block, Chunk, World (engine-agnostic, tested)
  def/, BlockTypes.hpp, Def.hpp, GameConstants.hpp — engine-agnostic enums/constants

src/GalaxyEggbert/Worlds/       — implementation of the engine-agnostic world/voxel model

src/GalaxyEggbertSimple3D/      — historical reference only as of 2026-07-08 (not built/maintained),
                                    built on Simple3D/U3D
  GalaxyEggbertSimpleGame.hpp / .cpp
  Game/
    GEWorldRuntime, GETerrainRenderer, GEBlupiController, GEDecorSystem,
    GEHud, GESound, GECameraRig, GEExploSystem, GEBridgeSystem

src/GalaxyEggbertCNA/            — early-stage long-term target (GalaxyEggbertCNA), built directly
                                    on CNA + Easy3D (see "Current Direction Lock"); opt-in via
                                    -DGALAXY_EGGBERT_BUILD_CNA=ON, not at feature parity yet

tests/
  GalaxyEggbert/Worlds/          — unit tests (54 tests, engine-independent)
```

## Build

`GalaxyEggbertSimple3D` build (requires `simple-3d` and a pre-built U3D) — **historical reference
only as of 2026-07-08, do not build/fix it**; shown only for completeness:

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --target GalaxyEggbertSimple3D -j2
./cmake-build-debug/GalaxyEggbertSimple3D
```

Use `-j2` maximum to protect RAM (32 GB limit; crashes occurred with more parallel jobs + multiple sessions).

`GalaxyEggbertCNA` — the actively built/maintained target — build instructions are above under
"Build target selection". See `NEXT.md` §2/§7 for its current status and the full set of
verification/tooling commands.

## Code rules

- No comments explaining *what* code does — only *why* when non-obvious.
- No abstractions beyond what the task requires.
- No `#ifdef` guards for engine differences.
- Prefer editing existing files over creating new ones.
- Public headers (used by tests) go in `include/`; private implementation headers go in `src/`.
