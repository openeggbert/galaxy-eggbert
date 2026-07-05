# Galaxy Eggbert

Galaxy Eggbert is a faithful 3D remake of **mobile-eggbert** (a C++ port of the original
*Speedy Blupi*, a Windows Phone XNA game from 2013).

## Current status

The current buildable implementation is **`GalaxyEggbertSimple3D`**, built on the `simple-3d`
library (which currently wraps U3D/Urho3D). This is a real, working, playable build — see
`NEXT.md` for its current feature status.

## Target direction

The long-term implementation is **`GalaxyEggbertCNA`**, built directly on **CNA** (a C++
reimplementation of the XNA 4.0 API), with **Easy3D** used as a small helper library beside CNA
(cameras, texture atlas, billboard/cube batching). Easy3D does not hide CNA — Galaxy Eggbert code
is free to call CNA directly at any time.

**`GalaxyEggbertCNA` now exists** as an early-stage, opt-in build target (`src/GalaxyEggbertCNA/`,
`-DGALAXY_EGGBERT_BUILD_CNA=ON`) — it builds, opens a window, and renders real, textured, animated
3D terrain from a hand-authored world, but has no Blupi/object rendering, HUD, sound, or gameplay
yet, and is far from feature parity with `GalaxyEggbertSimple3D`. See `NEXT.md` for its current
status, `easy3d.md` for the original migration analysis (now a dated snapshot — see its own
status-update banner), and `plan.md` (section "Direct CNA + Easy3D Migration") for the task list.

The former long-term direction — Simple3D → U3D/Urho3D → Nova3D — is now superseded by direct
CNA + Easy3D. `GalaxyEggbertSimple3D` remains the current working implementation and stays in the
repository as a historical/reference implementation until the CNA/Easy3D target reaches feature
parity with it. It is not being deleted.

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

### Current build — `GalaxyEggbertSimple3D`

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug --target GalaxyEggbertSimple3D -j2
./cmake-build-debug/GalaxyEggbertSimple3D
```

Unit tests (engine-independent world-data model):

```bash
cmake --build cmake-build-debug --target GalaxyEggbertWorldsTests -j2
./cmake-build-debug/GalaxyEggbertWorldsTests
```

### `GalaxyEggbertCNA` build — early-stage, opt-in (default OFF)

Not at feature parity with Simple3D yet — see `NEXT.md` §2 for current status. Requires sibling
checkouts of `../cna` and `../easy-3d` (or `-DCNA_HOME=`/`-DEASY3D_HOME=` overrides).

#### Linux native build (confirmed working)

```bash
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON -DGALAXY_EGGBERT_BUILD_SIMPLE3D=OFF
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA   # must run from its own build dir (relative asset paths)
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
- Android: intended, see `ANDROID.md` — cross-check against `NEXT.md` for current status.

`GalaxyEggbertSimple3D` remains the only target with a fully verified, playable, cross-platform
build; `GalaxyEggbertCNA` builds and renders real terrain on Linux today but is not yet at feature
parity and its non-Linux backends are unverified.

## Progress

See `NEXT.md` for current build status, recent changes, known bugs, and the next planned tasks.
