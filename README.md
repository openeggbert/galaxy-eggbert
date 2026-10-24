# Galaxy Eggbert

Galaxy Eggbert is a faithful 3D remake of **mobile-eggbert** (a C++ port of the original
*Speedy Blupi*, a Windows Phone XNA game from 2013).

> **Current project status:** read [CURRENT.md](CURRENT.md) first. This README retains project
> background and onboarding material; `CURRENT.md` is the concise authority for active targets,
> verification, limitations, and next work.

## Current status

**`GalaxyEggbertCNA` is the actively developed target.** It is playable and includes 3D terrain,
objects, gameplay, HUD, sound, menus, save/progress, and a complete in-game 3D world editor. It is
built directly on **CNA** (a C++ reimplementation of the XNA 4.0 API), with the strict modules of
**cna-engine** used as helpers beside CNA (camera, mesh builder for cubes/plates/billboards, GPU
meshes, texture regions); they do not hide CNA and game code calls CNA directly. (Until 2026-10-20
the helper library was easy-3d, which cna-engine replaced.) See [CURRENT.md](CURRENT.md) for verified current
status, [handoff.md](handoff.md) for commands and where work stands, and [plan.md](plan.md) for research
and the full historical task inventory.

The former `GalaxyEggbertSimple3D` implementation was retired after CNA reached playable parity
and removed from the live tree on 2026-07-25. It remains recoverable in git history at `4afd53e`.

## Target direction

The former long-term direction — Simple3D → U3D/Urho3D → Nova3D — is superseded by direct
CNA (locked 2026-07-05, with easy-3d as its helper library until cna-engine replaced it on
2026-10-20). Galaxy Eggbert will run only on CNA long-term.

## mobile-eggbert

`mobile-eggbert` (sibling repository, `../mobile-eggbert` relative to this one) is the read-only
reference implementation this remake is based on. Its assets (PNG sprite sheets, sounds, world
files) and its data/behavior (animation tables, gameplay logic) are the source of truth wherever
Galaxy Eggbert needs to match Speedy Blupi behavior exactly. **No changes are made to
`mobile-eggbert` as part of this migration without explicit user approval.**

Lua is not part of the first CNA migration phase.

## Development

### Init submodules

```bash
git submodule init --recursive
git submodule update --recursive
```

### `GalaxyEggbertCNA` build — the active target (default ON)

For the verified capability and test baseline, see [CURRENT.md](CURRENT.md). Building requires
sibling checkouts of `../cna` and `../cna-engine` (or
`-DCNA_HOME=`/`-DCNA_ENGINE_HOME=` overrides).

#### Linux native build (confirmed working)

```bash
cmake -S . -B build-cna-opengl3 -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_GRAPHICS_RENDERER=OPENGL33
cmake --build build-cna-opengl3 --target GalaxyEggbertCNA -j2
cd build-cna-opengl3 && ./GalaxyEggbertCNA   # run from its build dir (relative asset paths)
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
cmake --build build-cna-opengl3 --target GalaxyEggbertWorldsTests -j2
./build-cna-opengl3/GalaxyEggbertWorldsTests
```

The graphics renderer is CNA's own `CNA_GRAPHICS_RENDERER` cache option, not a
galaxy-eggbert-specific flag — pass `-DCNA_GRAPHICS_RENDERER=<value>` to override it. This
project defaults it to `OPENGLES3` natively and `WEBGL2` under Emscripten; both select CNA's
EasyGL implementation family. `EASYGL` is no longer a renderer name upstream — it became an
internal family reached through the public identities `OPENGLES3`, `OPENGL33`, and `WEBGL2`.
Current CNA also exposes `WEBGPU`, `HEADLESS`, `SOFTWARE`, `STUB`, `DIRECTX9`, `SDL_GPU`, and
`FNA3D`; its renderer identity registry is the source of truth.

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
  -DCNA_GRAPHICS_RENDERER=SDL_GPU \
  -DBUILD_TESTING=OFF
cmake --build build-windows --target GalaxyEggbertCNA -j2
```

The earlier Windows packaging work used the repository's x86-64 SDL prebuilt configuration and
passed `-DZLIB_INCLUDE_DIR=/usr/x86_64-w64-mingw32/include` plus
`-DZLIB_LIBRARY=/usr/x86_64-w64-mingw32/lib/libz.a`. MinGW builds stage `SDL3.dll`,
`SDL3_image.dll`, `SDL3_mixer.dll`, and `libwinpthread-1.dll` beside the executable while linking
the GCC/C++ runtime statically. The current `SDL_GPU` configuration has not yet been reverified;
do not present Windows as supported until a build and live 3D run pass.

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
emcmake cmake -S . -B build-cna-webgl2 \
  -DGALAXY_EGGBERT_BUILD_CNA=ON \
  -DCNA_GRAPHICS_RENDERER=WEBGL2 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=OFF
```

##### Build

```bash
cmake --build build-cna-webgl2 --target GalaxyEggbertCNA -j2
```

##### Run

```bash
emrun build-cna-webgl2/GalaxyEggbertCNA.html
```

##### Generated files

| File | Description |
|------|-------------|
| `GalaxyEggbertCNA.html` | Main entry point — open in browser |
| `GalaxyEggbertCNA.js`   | Emscripten JS glue |
| `GalaxyEggbertCNA.wasm` | WebAssembly binary |
| `GalaxyEggbertCNA.data` | Preloaded asset bundle |

##### Virtual filesystem layout

| Path | Source directory | Notes |
|------|-----------------|-------|
| `/Content/backgrounds` | `Content/backgrounds/` | Read-only; preloaded |
| `/Content/icons` | `Content/icons/` | Read-only; preloaded |
| `/Content/sounds` | `Content/sounds/` | Read-only; preloaded |
| `/worlds3d` | `worlds3d/` | Read-only; preloaded |
| `/textures3d` | `textures3d/` | Read-only; preloaded |
| `/avatars3d` | `avatars3d/` | Read-only; preloaded |
| `/save` | IndexedDB (IDBFS) | Writable; persists save file |

##### Notes

- CNA's storage preRun hook mounts `/save` through IndexedDB (IDBFS) with automatic persistence.
- Audio uses SDL_mixer; the browser may require a user gesture before audio starts.
- CNA's blocking `Game::Run()` yields between frames through `requestAnimationFrame` and Asyncify
  instead of using a busy loop.
- **Game speed**: intended to use a fixed-timestep accumulator in `CNA/Game.cpp` to match native
  desktop timing, the same approach CNA already uses elsewhere.

### Backend status for `GalaxyEggbertCNA`

- Linux: confirmed working, EasyGL family by default (`CNA_GRAPHICS_RENDERER=OPENGLES3`).
- Windows: the previous cross-build packaging path exists, but the current `SDL_GPU` renderer
  still needs a fresh build and live 3D verification (see `WINDOWS.md`).
- Web (Emscripten): WebGL2 release build and a 12-second final-artifact browser runtime verified on
  2026-10-17; the target is not covered by CI or a publishing pipeline.
- Android: intended, see `ANDROID.md`. `GalaxyEggbertCNA` is the sole game target and is enabled
  by default, so Gradle needs no target-selection argument.

`GalaxyEggbertCNA` is the maintained, playable target. Its full verified state and backend status
live in [CURRENT.md](CURRENT.md).

## Progress

See [CURRENT.md](CURRENT.md) for current build status, limitations, and active work; use
[handoff.md](handoff.md) for commands and where work stands
([NEXT.md](NEXT.md) is the frozen history up to 2026-10-24).
