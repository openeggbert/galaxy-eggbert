# Galaxy Eggbert

Galaxy Eggbert is a faithful 3D remake of **mobile-eggbert** (a C++ port of the original
*Speedy Blupi*, a Windows Phone XNA game from 2013).

## Current status

**`GalaxyEggbertCNA` is the actively developed target.** Built directly on **CNA** (a C++
reimplementation of the XNA 4.0 API), with **Easy3D** used as a small helper library beside CNA
(cameras, texture atlas, billboard/cube batching) — Easy3D does not hide CNA, Galaxy Eggbert code
is free to call CNA directly at any time. It is an opt-in build target
(`src/GalaxyEggbertCNA/`, `-DGALAXY_EGGBERT_BUILD_CNA=ON`, default OFF) that builds, opens a
window, and renders real, textured, animated 3D terrain from a hand-authored world — all 4
confirmed tile render modes plus water, `MoveObject`/`BigDecor` billboards and platform-lift/crate
objects (embeddable directly in the 3D `.vwr` format itself), and face-culled terrain. It has **no
visible Blupi (an invisible collision point only), no HUD, no sound, and no real gameplay logic
yet**. See `NEXT.md` for full current status, `easy3d.md` for the original migration analysis (now
a dated snapshot — see its own status-update banner), and `plan.md` (section "Direct CNA + Easy3D
Migration") for the task list.

`GalaxyEggbertSimple3D`, built on the `simple-3d` library (which wraps U3D/Urho3D), was the
working, playable target through 2026-07-08. **As of 2026-07-08 it is historical reference
only — it is not built, fixed, or maintained going forward** (its build is currently broken in
this environment, a missing/incompatible U3D prebuilt, and that is intentionally left unfixed).
Its code stays in the repository — it is not being deleted — and will be gradually removed only as
`GalaxyEggbertCNA` reaches equivalent functionality, piece by piece, not kept indefinitely.

## Target direction

The former long-term direction — Simple3D → U3D/Urho3D → Nova3D — is superseded by direct
CNA + Easy3D (locked 2026-07-05). Galaxy Eggbert will run only on CNA long-term.

## mobile-eggbert

`mobile-eggbert` (sibling repository, `../mobile-eggbert` relative to this one) is the read-only
reference implementation this remake is based on. Its assets (PNG sprite sheets, sounds, world
files) and its data/behavior (animation tables, gameplay logic) are the source of truth wherever
Galaxy Eggbert needs to match Speedy Blupi behavior exactly. **No changes are made to
`mobile-eggbert` as part of this migration without explicit user approval.**

Lua is not part of the first CNA/Easy3D migration phase.

## Development

### Init submodules

```bash
git submodule init --recursive
git submodule update --recursive
```

### `GalaxyEggbertCNA` build — the active target, opt-in (default OFF)

Not at gameplay parity with Simple3D yet (no Blupi/HUD/sound/gameplay logic) — see `NEXT.md` §2
for current status. Requires sibling checkouts of `../cna` and `../easy-3d` (or
`-DCNA_HOME=`/`-DEASY3D_HOME=` overrides).

#### Linux native build (confirmed working)

```bash
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA   # must run from its own build dir (relative asset paths)
```

Unit tests (engine-independent world-data model):

```bash
cmake --build build-cna --target GalaxyEggbertWorldsTests -j2
./build-cna/GalaxyEggbertWorldsTests
```

The graphics backend (EasyGL on Linux by default, SDL_Renderer elsewhere) is CNA's own
`CNA_GRAPHICS_BACKEND` cache option (`SDL_RENDERER` / `EASYGL` / `BGFX` / `VULKAN`), not a
galaxy-eggbert-specific flag — pass `-DCNA_GRAPHICS_BACKEND=<value>` to override it.

#### Windows native build (not verified in this session)

Same commands as above, run from a Windows toolchain (e.g. CLion's bundled MinGW — see
`WINDOWS.md` for known gaps, notably that runtime DLLs are not yet auto-copied next to the
executable).

```powershell
cmake -S . -B build-windows -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-windows --target GalaxyEggbertCNA
```

#### Windows cross-build from Linux (MinGW-w64) (not verified in this session)

**Important: Always use a clean build directory when switching toolchains (e.g., `rm -rf build-windows`).**

1. Ensure you have `mingw-w64` installed (e.g., `sudo apt install mingw-w64`).
2. Run the build:
```bash
# Ensure you are in galaxy-eggbert directory
rm -rf build-windows
cmake -S . -B build-windows \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
  -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-windows --target GalaxyEggbertCNA
```

*Note: this requires Windows-target SDL3 package configs (`SDL3`, `SDL3_image`, etc.) discoverable
via `CMAKE_PREFIX_PATH` or a similar override — the exact mechanism has not been re-verified
against the current `CMakeLists.txt` in this session.*

#### Web / Emscripten build (not verified in this session — CNA's CMake path has no Emscripten-specific handling confirmed yet)

##### Prerequisites

1. Install and activate the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):
   ```bash
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```
2. Ensure all submodules are initialised:
   ```bash
   git submodule update --init --recursive
   ```

##### Configure

```bash
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -S . -B cmake-build-web -DGALAXY_EGGBERT_BUILD_CNA=ON -DCMAKE_BUILD_TYPE=Debug
```

##### Build

```bash
cmake --build cmake-build-web -j
```

##### Run

```bash
emrun cmake-build-web/GalaxyEggbertCNA.html
```

##### Generated files

| File | Description |
|------|-------------|
| `GalaxyEggbertCNA.html` | Main entry point — open in browser |
| `GalaxyEggbertCNA.js`   | Emscripten JS glue |
| `GalaxyEggbertCNA.wasm` | WebAssembly binary |
| `GalaxyEggbertCNA.data` | Preloaded asset bundle |

##### Virtual filesystem layout (planned)

| Path | Source directory | Notes |
|------|-----------------|-------|
| `/Content/backgrounds` | `Content/backgrounds/` | Read-only; preloaded |
| `/Content/icons` | `Content/icons/` | Read-only; preloaded |
| `/Content/sounds` | `Content/sounds/` | Read-only; preloaded |
| `/worlds` | `worlds/` | Read-only; preloaded |
| `/save` | IndexedDB (IDBFS) | Writable; persists save file |

##### Notes (planned — not yet confirmed working for `GalaxyEggbertCNA`'s Emscripten build)

- Save data would be stored via IndexedDB (IDBFS), flushed on every write and on page unload.
- Audio uses SDL_mixer; the browser may require a user gesture before audio starts.
- CPU usage bounded via `emscripten_set_main_loop` (backed by `requestAnimationFrame`) instead of
  a busy loop.
- **Game speed**: intended to use a fixed-timestep accumulator in `CNA/Game.cpp` to match native
  desktop timing, the same approach CNA already uses elsewhere.

### Backend status for `GalaxyEggbertCNA`

- Linux: confirmed working, EasyGL backend by default (`CNA_GRAPHICS_BACKEND=EASYGL`).
- Windows: SDL_Renderer is the intended supported backend; not verified in this session (see
  `WINDOWS.md` for known gaps).
- Web (Emscripten): SDL_Renderer backend, experimental — the CMake plumbing for it has not been
  confirmed to exist for `GalaxyEggbertCNA` specifically (no Emscripten-specific handling found in
  its part of `CMakeLists.txt` as of 2026-07-05).
- Android: intended, see `ANDROID.md` — but **the Android Gradle build does not currently pass
  `-DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF`** to CMake, so with the
  defaults in `CMakeLists.txt` (`GALAXY_EGGBERT_BUILD_SIMPLE3D` still `ON` by default,
  `GALAXY_EGGBERT_BUILD_CNA` still `OFF`) it would currently attempt to build the historical-only
  `GalaxyEggbertSimple3D` target, not `GalaxyEggbertCNA` — found 2026-07-09, not yet fixed; see
  `NEXT.md`.

`GalaxyEggbertCNA` builds and renders real terrain, objects, and `BigDecor` on Linux today, but is
not yet at gameplay parity (no Blupi/HUD/sound/gameplay logic) and its non-Linux backends are
unverified. `GalaxyEggbertSimple3D` is not built/verified going forward (historical reference
only, see "Current status" above).

### `GalaxyEggbertSimple3D` build — historical reference only, do not build (as of 2026-07-08)

Shown here only for completeness; per the "Current status" section above, this target is not
built, fixed, or maintained going forward, and its build is currently broken in this environment
(missing/incompatible U3D prebuilt) — that is intentionally left unfixed.

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --target GalaxyEggbertSimple3D -j2
./cmake-build-debug/GalaxyEggbertSimple3D
```

## Progress

See `NEXT.md` for current build status, recent changes, known bugs, and the next planned tasks.
