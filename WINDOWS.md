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

### Linux / Web / Android

The static-runtime option above is guarded by `if(MINGW)` and has no effect on Linux, Emscripten
(WebAssembly), or Android builds.
