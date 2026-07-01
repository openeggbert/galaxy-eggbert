# Galaxy Eggbert

Galaxy Eggbert is a faithful 3D remake of **mobile-eggbert** (a C++ port of the original
*Speedy Blupi*, a Windows Phone XNA game from 2013).

## Current status

The current buildable implementation is **`GalaxyEggbertSimple3D`**, built on the `simple-3d`
library (which currently wraps U3D/Urho3D). This is a real, working, playable build — see
`NEXT.md` for its current feature status.

## Target direction

The planned long-term implementation is **`GalaxyEggbertCNA`**, built directly on **CNA** (a C++
reimplementation of the XNA 4.0 API), with **Easy3D** used as a small helper library beside CNA
(cameras, texture atlas, billboard/cube batching). Easy3D does not hide CNA — Galaxy Eggbert code
is free to call CNA directly at any time.

**`GalaxyEggbertCNA` does not exist yet.** It is planned, not implemented — there is no CMake
target for it today. See `easy3d.md` for the full migration analysis and `plan.md` (section
"Direct CNA + Easy3D Migration") for the task list.

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

### Planned build — `GalaxyEggbertCNA` (not implemented yet)

The commands below describe the intended CNA build once the `GalaxyEggbertCNA` target and the
`GALAXY_EGGBERT_BUILD_CNA` CMake option exist. **They do not work today** — there is no such
target in `CMakeLists.txt` yet. They are recorded here as the agreed target shape for when that
work starts (see `plan.md`, "Next implementation batch — CNA target skeleton").

#### Linux native build (planned)

```bash
cmake -S . -B build-linux \
  -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_BACKEND_SDL_RENDERER=OFF \
  -DCNA_BACKEND_EASY_GL=ON \
  -DCNA_BACKEND_BGFX=OFF
cmake --build build-linux --target GalaxyEggbertCNA
```

#### Windows native build (planned)

```powershell
cmake -S . -B build-windows \
  -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_BACKEND_SDL_RENDERER=ON \
  -DCNA_BACKEND_EASY_GL=OFF \
  -DCNA_BACKEND_BGFX=OFF
cmake --build build-windows --target GalaxyEggbertCNA
```

#### Windows cross-build from Linux (MinGW-w64) (planned)

**Important: Always use a clean build directory when switching toolchains (e.g., `rm -rf build-windows`).**

1. Ensure you have `mingw-w64` installed (e.g., `sudo apt install mingw-w64`).
2. Run the build:
```bash
# Ensure you are in galaxy-eggbert directory
rm -rf build-windows
cmake -S . -B build-windows \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
  -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_BACKEND_SDL_RENDERER=ON \
  -DCNA_WINDOWS_DEPENDENCIES_ROOT=/path/to/windows/sdl3/libs
cmake --build build-windows --target GalaxyEggbertCNA
```

*Note: You must provide Windows-target SDL3 package configs (`SDL3`, `SDL3_image`, etc.) through `CNA_WINDOWS_DEPENDENCIES_ROOT` or `CMAKE_PREFIX_PATH`.*

#### Web / Emscripten build (planned)

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

##### Notes (planned, once `GalaxyEggbertCNA` exists)

- Save data would be stored via IndexedDB (IDBFS), flushed on every write and on page unload.
- Audio uses SDL_mixer; the browser may require a user gesture before audio starts.
- CPU usage bounded via `emscripten_set_main_loop` (backed by `requestAnimationFrame`) instead of
  a busy loop.
- **Game speed**: intended to use a fixed-timestep accumulator in `CNA/Game.cpp` to match native
  desktop timing, the same approach CNA already uses elsewhere.

### Backend status (planned, for `GalaxyEggbertCNA`)

- Windows: SDL_Renderer is the intended supported backend.
- Linux: SDL_Renderer is intended to be supported; easy-gl can be enabled explicitly when needed.
- Web (Emscripten): SDL_Renderer backend, experimental.
- Android: planned, further out.

None of the above is implemented yet — `GalaxyEggbertSimple3D` is the only backend that currently
builds and runs.

## Progress
