# Galaxy Eggbert

Galaxy Eggbert is a faithful 3D remake of **mobile-eggbert** (a C++ port of the original
*Speedy Blupi*, a Windows Phone XNA game from 2013).

> **Current project status:** read [CURRENT.md](CURRENT.md) first. This README retains project
> background and onboarding material; `CURRENT.md` is the concise authority for active targets,
> verification, limitations, and next work.

## Current status

**`GalaxyEggbertCNA` is the actively developed target.** It is playable and includes 3D terrain,
objects, gameplay, HUD, sound, menus, save/progress, and a complete in-game 3D world editor. It is
built directly on **CNA** (a C++ reimplementation of the XNA 4.0 API), with **Easy3D** used as a
small helper library beside CNA (cameras, texture atlas, billboard/cube batching); Easy3D does not
hide CNA and game code can call CNA directly. See [CURRENT.md](CURRENT.md) for verified current
status, [NEXT.md](NEXT.md) for commands and detailed history, and [plan.md](plan.md) for research
and the full historical task inventory.

The former `GalaxyEggbertSimple3D` implementation was retired after CNA reached playable parity
and removed from the live tree on 2026-07-25. It remains recoverable in git history at `4afd53e`.

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

### `GalaxyEggbertCNA` build — the active target (default ON)

For the verified capability and test baseline, see [CURRENT.md](CURRENT.md). Building requires
sibling checkouts of `../cna` and `../easy-3d` (or
`-DCNA_HOME=`/`-DEASY3D_HOME=` overrides).

#### Linux native build (confirmed working)

```bash
cmake -S . -B build-cna -DGALAXY_EGGBERT_BUILD_CNA=ON
cmake --build build-cna --target GalaxyEggbertCNA -j2
cd build-cna && ./GalaxyEggbertCNA   # must run from its own build dir (relative asset paths)
```

#### Linux runtime bundle (`.tar.gz`)

The `package` target creates a relocatable x86-64 Linux archive. It includes the executable,
source-tracked game data, the SDL3 runtime libraries, licenses, and a launcher that keeps the
required asset-relative working directory.

```bash
cmake -S . -B build-package -DCMAKE_BUILD_TYPE=Release \
  -DGALAXY_EGGBERT_BUILD_CNA=ON
cmake --build build-package --target package -j2
tar -xzf build-package/GalaxyEggbertCNA-linux-x86_64.tar.gz
./GalaxyEggbertCNA-linux-x86_64/run-galaxy-eggbert
```

The archive is deliberately a runtime bundle, not an AppImage: its host still supplies standard
Linux libraries such as the graphics stack, FFmpeg, and the C++ runtime. See the bundled
`README-Linux.md` for details. Set `-DGALAXY_EGGBERT_ENABLE_LINUX_BUNDLE=OFF` to omit the package
target in a developer-only Linux build.

Unit tests (engine-independent world-data model):

```bash
cmake --build build-cna --target GalaxyEggbertWorldsTests -j2
./build-cna/GalaxyEggbertWorldsTests
```

The graphics renderer is CNA's own `CNA_GRAPHICS_RENDERER` cache option, not a
galaxy-eggbert-specific flag — pass `-DCNA_GRAPHICS_RENDERER=<value>` to override it. This
project defaults it to `OPENGLES3` natively and `WEBGL2` under Emscripten; both select CNA's
EasyGL implementation family. `EASYGL` is no longer a renderer name upstream — it became an
internal family reached through the five GL-profile identities `OPENGLES2` / `OPENGLES3` /
`OPENGL33` / `WEBGL1` / `WEBGL2`. See `../cna/cmake/RendererSelection.cmake` for every accepted
value (`SDL_RENDERER`, `VULKAN`, `BGFX`, and ~45 more).

#### Windows native build (not verified in this session)

Same commands as above, run from a Windows toolchain (e.g. CLion's bundled MinGW — see
`WINDOWS.md` for known gaps, notably that runtime DLLs are not yet auto-copied next to the
executable).

```powershell
cmake -S . -B build-windows -DGALAXY_EGGBERT_BUILD_CNA=ON
cmake --build build-windows --target GalaxyEggbertCNA
```

#### Windows cross-build from Linux (MinGW-w64) (compile verified 2026-07-25)

**Important: Always use a clean build directory when switching toolchains (e.g., `rm -rf build-windows`).**

1. Ensure you have `mingw-w64` installed (e.g., `sudo apt install mingw-w64`).
2. Run the build:
```bash
# Ensure you are in galaxy-eggbert directory
rm -rf build-windows
cmake -S . -B build-windows \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
  -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_GRAPHICS_RENDERER=SDL_RENDERER \
  -DBUILD_TESTING=OFF
cmake --build build-windows --target GalaxyEggbertCNA -j2
```

The verified Linux environment used the repository's Windows x86-64 SDL prebuilt configuration
and passed `-DZLIB_INCLUDE_DIR=/usr/x86_64-w64-mingw32/include` plus
`-DZLIB_LIBRARY=/usr/x86_64-w64-mingw32/lib/libz.a`. MinGW builds now place `SDL3.dll`,
`SDL3_image.dll`, `SDL3_mixer.dll`, and `libwinpthread-1.dll` next to the executable while linking
the GCC/C++ runtime statically. A Wine launch confirmed this bundle reaches window creation,
asset loading, and world loading, but then fails because CNA's current `SDL_RENDERER` backend is
2D-only and throws on 3D vertex-buffer creation. It is therefore a valid packaging build, not yet
a runnable Windows 3D release.

#### Web / Emscripten build (manually verified; not in CI or a release pipeline)

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

- Linux: confirmed working, EasyGL family by default (`CNA_GRAPHICS_RENDERER=OPENGLES3`).
- Windows: SDL_Renderer cross-compiles with MinGW-w64 and stages its required SDL/MinGW runtime
  DLLs, but cannot yet run the 3D game because that CNA backend is 2D-only (see `WINDOWS.md`).
- Web (Emscripten): manually verified WebGL2 build; not exercised by CI or a publishing pipeline.
- Android: intended, see `ANDROID.md`. `GalaxyEggbertCNA` is the sole game target and is enabled
  by default, so Gradle needs no target-selection argument.

`GalaxyEggbertCNA` is the maintained, playable target. Its full verified state and backend status
live in [CURRENT.md](CURRENT.md).

## Progress

See [CURRENT.md](CURRENT.md) for current build status, limitations, and active work; use
[NEXT.md](NEXT.md) for commands and detailed recent history.
