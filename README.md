# Galaxy Eggbert

Galaxy Eggbert is the 3D remake of the game Speedy Blupi.

## Development

### Init submodules

git submodule init --recursive
git submodule update --recursive

### Linux native build

```bash
cmake -S . -B build-linux \
  -DCNA_BACKEND_SDL_RENDERER=OFF \
  -DCNA_BACKEND_EASY_GL=ON \
  -DCNA_BACKEND_BGFX=OFF
cmake --build build-linux --target GalaxyEggbert
```

### Windows native build

```powershell
cmake -S . -B build-windows \
  -DCNA_BACKEND_SDL_RENDERER=ON \
  -DCNA_BACKEND_EASY_GL=OFF \
  -DCNA_BACKEND_BGFX=OFF
cmake --build build-windows --target GalaxyEggbert
```

### Windows cross-build from Linux (MinGW-w64)

**Important: Always use a clean build directory when switching toolchains (e.g., `rm -rf build-windows`).**

1. Ensure you have `mingw-w64` installed (e.g., `sudo apt install mingw-w64`).
2. Run the build:
```bash
# Ensure you are in galaxy-eggbert directory
rm -rf build-windows
cmake -S . -B build-windows \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
  -DCNA_BACKEND_SDL_RENDERER=ON \
  -DCNA_WINDOWS_DEPENDENCIES_ROOT=/path/to/windows/sdl3/libs
cmake --build build-windows --target GalaxyEggbert
```

*Note: You must provide Windows-target SDL3 package configs (`SDL3`, `SDL3_image`, etc.) through `CNA_WINDOWS_DEPENDENCIES_ROOT` or `CMAKE_PREFIX_PATH`.*

### Web / Emscripten build

#### Prerequisites

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

#### Configure

```bash
source /path/to/emsdk/emsdk_env.sh
emcmake cmake -S . -B cmake-build-web -DCMAKE_BUILD_TYPE=Debug
```

#### Build

```bash
cmake --build cmake-build-web -j
```

#### Run

```bash
emrun cmake-build-web/GalaxyEggbert.html
```

#### Generated files

| File | Description |
|------|-------------|
| `GalaxyEggbert.html` | Main entry point — open in browser |
| `GalaxyEggbert.js`   | Emscripten JS glue |
| `GalaxyEggbert.wasm` | WebAssembly binary |
| `GalaxyEggbert.data` | Preloaded asset bundle |

#### Virtual filesystem layout

| Path | Source directory | Notes |
|------|-----------------|-------|
| `/Content/backgrounds` | `Content/backgrounds/` | Read-only; preloaded |
| `/Content/icons` | `Content/icons/` | Read-only; preloaded |
| `/Content/sounds` | `Content/sounds/` | Read-only; preloaded |
| `/worlds` | `worlds/` | Read-only; preloaded |
| `/save` | IndexedDB (IDBFS) | Writable; persists `SpeedyBlupi` save file |

#### Notes

- Save data (`SpeedyBlupi`) is stored in `/save/.cna_isolated_storage/SpeedyBlupi`
  backed by the browser's IndexedDB. It is flushed to IndexedDB on every write
  and on page unload.
- Audio uses SDL_mixer; the browser may require a user gesture before audio
  starts. If no sound is heard, click the canvas once.
- CPU usage is bounded — the game uses `emscripten_set_main_loop` (backed by
  `requestAnimationFrame`) instead of a busy loop.
- **Game speed**: the Web build uses a fixed-timestep accumulator in
  `CNA/Game.cpp` to match native desktop timing. The browser calls the RAF
  callback at ~60 Hz; real inter-frame wall-clock time is measured and
  accumulated, and `Update()` fires only when one full `TargetElapsedTime`
  slice has accumulated. This ensures gameplay speed is identical to
  Linux/Windows regardless of the browser's actual RAF cadence. A 250 ms
  spike cap prevents runaway catch-up after the tab is backgrounded.

### Backend status

- Windows: SDL_Renderer is the supported backend.
- Linux: SDL_Renderer is supported; easy-gl can be enabled explicitly when needed.
- Web (Emscripten): SDL_Renderer backend, experimental.
- Android: planned.

## Progress



