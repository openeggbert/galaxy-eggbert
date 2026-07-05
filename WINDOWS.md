# Windows Build Notes

## Building with MinGW (CLion default toolchain)

The project is configured to build on Windows using the bundled CLion MinGW
toolchain (GCC/G++ targeting `x86_64-w64-mingw32`).

### Static C++ runtime

`GalaxyEggbertSimple3D` links the GCC/C++ runtime statically on MinGW, guarded by
`if(MINGW)` in `CMakeLists.txt`:

```cmake
target_link_options(GalaxyEggbertSimple3D PRIVATE -static-libgcc -static-libstdc++)
```

This avoids a runtime dependency on `libgcc_s_seh-1.dll`/`libstdc++-6.dll` outside the
CLion/MSYS2 environment. `GalaxyEggbertCNA` does not currently have this option applied — if you
need a portable `GalaxyEggbertCNA.exe`, either build it inside the CLion/MSYS2 environment (so the
MinGW runtime DLLs are already on `PATH`), or add the same `-static-libgcc -static-libstdc++`
link options to its CMake target.

### Known gap: no runtime DLL copying

Neither `GalaxyEggbertSimple3D` nor `GalaxyEggbertCNA` currently has a `POST_BUILD` step that
copies runtime DLLs (`libwinpthread-1.dll`, or `SDL3.dll`/`SDL3_image.dll`/`SDL3_mixer.dll` for
`GalaxyEggbertCNA`) next to the built executable — running either `.exe` outside the build
environment on a machine without those DLLs on `PATH` will fail to launch. The sibling `../cna`
repository's `cmake/ThirdPartySDL.cmake` already provides reusable helper functions for exactly
this (`cna_copy_mingw_runtime(target)`, `cna_copy_sdl_runtime(target)`), but this repo's
`CMakeLists.txt` does not currently call them for either target. Wiring these up is a real,
open task, not yet scheduled — see `NEXT.md` for the current task list.

### Linux / Web / Android

The static-runtime option above is guarded by `if(MINGW)` and has no effect on Linux, Emscripten
(WebAssembly), or Android builds.
