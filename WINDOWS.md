# Windows Build Notes

## Building with MinGW (CLion default toolchain)

The project is configured to build on Windows using the bundled CLion MinGW
toolchain (GCC/G++ targeting `x86_64-w64-mingw32`).

### Running the executable outside CLion

When built with MinGW the executable normally depends on MinGW runtime DLLs that
are only available inside the CLion/MSYS2 environment:

| DLL | Handled by |
|-----|-----------|
| `libgcc_s_seh-1.dll` | Statically linked (`-static-libgcc`) |
| `libstdc++-6.dll` | Statically linked (`-static-libstdc++`) |
| `libwinpthread-1.dll` | Copied next to the executable at build time |
| `SDL3.dll` | Copied next to the executable at build time |
| `SDL3_image.dll` | Copied next to the executable at build time |
| `SDL3_mixer.dll` | Copied next to the executable at build time |

After a successful build the output directory
(`cmake-build-debug/` or your chosen build dir) should contain:

```
GalaxyEggbert.exe
libwinpthread-1.dll
SDL3.dll
SDL3_image.dll
SDL3_mixer.dll
Content/   (game assets)
```

You can copy this entire directory to any Windows machine and run the game
without installing MinGW, CLion, or any other runtime.

### How it is implemented (CMake)

**Static GCC/C++ runtime** — in `CMakeLists.txt`, guarded by `if(MINGW)`:

```cmake
target_link_options(${_game_target} PRIVATE -static-libgcc -static-libstdc++)
```

**Copying `libwinpthread-1.dll`** — the helper function `cna_copy_mingw_runtime()`
defined in `../../cna/cmake/ThirdPartySDL.cmake`:
1. Calls `gcc -print-file-name=libwinpthread-1.dll` at configure time to locate
   the DLL inside the active MinGW installation.
2. Falls back to the directory that contains the compiler binary.
3. Adds a `POST_BUILD` command that copies the DLL next to the target executable.

**SDL runtime DLLs** — copied by the existing `cna_copy_sdl_runtime()` helper
(also in `ThirdPartySDL.cmake`), called unconditionally for all `WIN32` builds.

### Test executable (`CnaTests.exe`)

The same steps are applied to `CnaTests.exe`:
- `-static-libgcc -static-libstdc++` is added via `target_link_options`.
- `cna_copy_mingw_runtime(CnaTests)` copies `libwinpthread-1.dll`.
- `cna_copy_sdl_runtime(CnaTests)` copies the SDL DLLs.
- `gtest`/`gmock` shared libraries are also copied at POST_BUILD.

### Linux / Web / Android

These changes are fully guarded by `if(MINGW)` / `if(WIN32)` and have no effect
on Linux, Emscripten (WebAssembly), or Android builds.
