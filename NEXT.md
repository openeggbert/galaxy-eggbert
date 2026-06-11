# Galaxy Eggbert — Next Steps

## Platform support for U3D engine

Phase 1 implements U3D support for **Linux desktop** and **Windows desktop**.
The following platforms need additional work before they can build with `GALAXY_EGGBERT_ENGINE=U3D`.

---

### Windows (MinGW cross-compile)

Status: **CMake ready, needs U3D Windows build.**

1. Build U3D for Windows (MinGW or MSVC):
   ```bash
   # From a MinGW environment or using the existing toolchain:
   cmake -S /rv/data/library/github.com/u3d-community/U3D \
         -B /rv/data/library/github.com/u3d-community/U3D/build-windows \
         -DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/MinGW.cmake
   ninja -C build-windows Urho3D
   ```
2. Configure galaxy-eggbert:
   ```bash
   cmake -S . -B build-windows \
         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64.cmake \
         -DGALAXY_EGGBERT_ENGINE=U3D \
         -DU3D_HOME=/rv/data/library/github.com/u3d-community/U3D/build-windows
   ```
3. Add `cna_copy_sdl_runtime` equivalent for U3D (copy SDL3.dll, OpenAL, etc. from U3D's bin/).

---

### Android

Status: **Not yet implemented for U3D.**

U3D's Android build differs fundamentally from the current Nova3D/CNA approach:
- U3D uses its own Gradle + CMake integration (`android/` directory in U3D source).
- The AAR/JNI approach from `cmake/Modules/FindUrho3D.cmake` (lines for `ANDROID`) needs `BUILD_STAGING_DIR` or a Maven AAR.
- Requires building U3D as an Android library first (ARM64-v8a, armeabi-v7a).

Steps:
1. Build U3D for Android: `./gradlew assembleRelease` inside U3D's `android/` directory.
2. Set `BUILD_STAGING_DIR` or publish U3D AAR to a local Maven repository.
3. Update galaxy-eggbert's `CMakeLists.txt` U3D section to handle `ANDROID=ON`.
4. Remove the current `FATAL_ERROR` guard for `ANDROID + U3D`.

Reference: `U3D/cmake/Modules/FindUrho3D.cmake` lines 89–118 (Android discovery logic).

---

### Web (Emscripten)

Status: **Not yet implemented for U3D.**

U3D supports Emscripten but requires:
- A separate Emscripten U3D build with `-DEMSCRIPTEN=1`.
- Different resource embedding: `Data/` and `CoreData/` must be preloaded into the WASM virtual FS.
- U3D's HTML5 template and SDL2 Emscripten port (U3D bundles its own SDL).

Steps:
1. Source Emscripten SDK, then build U3D:
   ```bash
   emcmake cmake -S /rv/.../U3D -B /rv/.../U3D/build-em
   emmake ninja -C build-em Urho3D
   ```
2. Update galaxy-eggbert CMakeLists.txt U3D section for EMSCRIPTEN: add `--preload-file` for `Data/` and `CoreData/`.
3. Remove the current `FATAL_ERROR` guard for `EMSCRIPTEN + U3D`.

---

## Nova3D scene-graph parity

When Nova3D implements its full scene-graph backend (same API as U3D):
1. Set `GALAXY_EGGBERT_ENGINE=NOVA3D` in CMake.
2. Remove `#ifdef GE_ENGINE_U3D` / `#ifndef GE_ENGINE_U3D` guards from `GalaxyEggbertGame.cpp`.
3. Drop the Nova3D stub `Start()`/`Update()` stubs.
4. Verify `EP_RESOURCE_PATHS`, `ResourceCache`, `StaticModel`, etc. behave identically.

Expected code change: zero lines in game logic, a few lines in `GalaxyEggbertApp.cpp` Setup().

---

## Game development (post-Phase-1)

- Integrate `GalaxyEggbert::Worlds::World` / `Chunk` data model with the U3D renderer:
  spawn one `StaticModel` node per non-air block, use chunk dirty flags to batch updates.
- Speedy Blupi player character: load an actual mesh instead of the Box.mdl placeholder.
- Game world loading from `worlds/` directory using `World::load()`.
- Basic gameplay mechanics ported from mobile-eggbert.
