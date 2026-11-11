# Windows Build Notes

## Building with MinGW (CLion default toolchain)

The project is configured to build on Windows using the bundled CLion MinGW
toolchain (GCC/G++ targeting `x86_64-w64-mingw32`).

### Static C++ runtime

`GalaxyEggbertCNA` links the GCC/C++ runtime statically on MinGW, guarded by
`if(MINGW)` in `CMakeLists.txt`. This avoids a runtime dependency on
`libgcc_s_seh-1.dll`/`libstdc++-6.dll` outside the CLion/MSYS2 environment,
plus CNA's established `--allow-multiple-definition` workaround for the MinGW PE/COFF linker.

### Known gap: no runtime DLL copying

`GalaxyEggbertCNA` calls CNA's `cna_copy_mingw_runtime(target)` and
`cna_copy_sdl_runtime(target)` helpers after every MinGW build. The repository's Windows SDL
prebuilt resolves its import libraries through `SDL3_DIR` without exposing the helpers'
compatibility aliases, so the target also copies the known `bin/` DLLs from that package prefix as
a fallback. Its output directory therefore contains `libwinpthread-1.dll`, `SDL3.dll`,
`SDL3_image.dll`, and `SDL3_mixer.dll` next to the executable.

### Current verification gap

The previous staged `GalaxyEggbertCNA.exe` packaging run proved that the runtime DLL bundle could
reach window and asset creation, but it used a CNA renderer that has since been retired. Current
CNA offers `SDL_GPU` and `DIRECTX9` for Windows builds. The documented cross-build selects
`SDL_GPU`; do not present Windows as supported until that configuration builds and completes a
live 3D run.

### The cross-build from Linux, with multiplayer (2026-11-11, Q-022)

The cross-build is done on Linux, in the shared probe directory (no build tree of its own; Q-018
names none). Since the multiplayer merge (2026-11-11) the tree is the main worktree's
`build-probe/windows`:

```bash
export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv
CNA=/rv/data/development/github.com/libcna/cna
CURL=$HOME/deps/curl-mingw-8.22.0/curl-8.22.0_3-win64-mingw   # the gamer services' libcurl, shared
cmake -S . -B build-probe/windows -G Ninja -DCMAKE_TOOLCHAIN_FILE=$CNA/cmake/toolchains/mingw-w64.cmake \
      -DCMAKE_BUILD_TYPE=Release -DCNA_HOME=$CNA -DCNA_ENGINE_HOME=$CNA/../cna-engine \
      -DCNA_GRAPHICS_RENDERER=OPENGL33 \
      -DSDL3_DIR=$CNA/.sdl-prebuilt-Windows-x86_64/install/lib/cmake/SDL3 \
      -DSDL3_mixer_DIR=$CNA/.sdl-prebuilt-Windows-x86_64/install/lib/cmake/SDL3_mixer \
      -DCURL_INCLUDE_DIR=$CURL/include -DCURL_LIBRARY=$CURL/lib/libcurl.dll.a \
      -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache -DCNA_USE_CCACHE=ON
cmake --build build-probe/windows --target GalaxyEggbertCNA --parallel 6
```

- **What it makes:** `GalaxyEggbert.exe` with `SDL3.dll`, `SDL3_mixer.dll`, `libcurl-x64.dll` and
  `libwinpthread-1.dll` beside it. It runs from a folder with `Content`, `avatars3d`, `textures3d`
  and `worlds3d`.
- **The renderer:** OPENGL33, Linux's (EasyGL), so Galaxy's own GLSL works as it is. A Windows
  machine needs OpenGL 3.3.
- **Verified under wine 10 (Xvfb, Mesa llvmpipe):**
  - A mission plays and draws as on Linux (`SDL_VIDEODRIVER=windows`, which overrides the
    environment's `x11`).
  - Network games between a wine instance and Linux ones on the same machine find each other and
    are played to the end: `NET_WINDOWS=<this tree> NET_WINDOWS_PLAYERS="Ann ..."
    tools/net_players.sh`.
- **Not yet tried:** a real Windows machine, a firewall's question, the DirectX 9 and SDL_GPU
  renderers.
- **The package:** `tools/release/make_windows_zip.sh build-probe/windows build-probe/release` makes
  `GalaxyEggbert-<version>-windows-x86_64.zip` (the exe, its DLLs, the data, LICENSE,
  THIRD_PARTY.txt, RELEASE_NOTES.md, `packaging/README-windows-bundle.md` as README.md; some 160 MB).
  The first one (2026-11-11, 1.0.0-rc1) was unpacked and played under wine.

### Linux / Web / Android

The static-runtime option above is guarded by `if(MINGW)` and has no effect on Linux, Emscripten
(WebAssembly), or Android builds.
