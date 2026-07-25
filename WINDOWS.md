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

### Current 3D backend blocker

The staged `GalaxyEggbertCNA.exe` was launched through Wine on 2026-07-25. It created its SDL
window and loaded `worlds3d/world001.vwr` plus its background texture, proving the runtime DLL
bundle works. It then terminated at `CreateVertexBuffer`: CNA's `SDL_RENDERER` backend reports
itself as 2D-only and does not implement 3D vertex buffers. Do not present this build as a runnable
Windows 3D release until a Windows 3D backend is selected and verified.

### Linux / Web / Android

The static-runtime option above is guarded by `if(MINGW)` and has no effect on Linux, Emscripten
(WebAssembly), or Android builds.
