# U3D Support Plan for galaxy-eggbert

**Date:** 2026-06-11  
**Author:** Analysis by Robert Vokac + Claude  
**Status:** Analysis / Pre-implementation

---

## 1. Context

**galaxy-eggbert** is a future 3D remake of Speedy Blupi.  
**mobile-eggbert** is the 2D Speedy Blupi port (XNA 4.0 origin); galaxy-eggbert will take heavy inspiration and reuse code from it.

Currently galaxy-eggbert links against **Nova3D** — a fork of Urho3D that Robert Vokac is building, which internally calls his C++ XNA 4.0 re-implementation (CNA/easy-gl). Nova3D is not yet finished.

**U3D** (`github.com/u3d-community/U3D`, local mirror at `/rv/data/library/github.com/u3d-community/U3D`) is the community fork of Urho3D created after the original Urho3D repository was taken over. U3D is stable and mature.

**Goal:** galaxy-eggbert must be able to run on both U3D and Nova3D via a CMake switch.  
**Immediate priority:** Start active galaxy-eggbert development now using U3D (since Nova3D is unfinished).  
**Long-term:** When Nova3D reaches feature parity, switch back to Nova3D as the primary engine with zero or near-zero game-code changes.

---

## 2. Engine Comparison

### 2.1 Shared foundations

Both engines share:

| Property | U3D | Nova3D |
|---|---|---|
| C++ namespace | `Urho3D` | `Urho3D` |
| Header path pattern | `<Urho3D/...>` | `<Urho3D/...>` |
| Core math types | `Vector2/3`, `Color`, `Quaternion`, `Matrix3x4` | Same (same headers) |
| Container smart pointers | `SharedPtr<T>`, `WeakPtr<T>` | Available |
| `URHO3D_OBJECT` macro | Yes | Yes |
| `Application` base class | Yes (`Setup/Start/Stop`) | Yes (`Setup/Start/Stop`) |
| Scene graph types (planned) | `Scene`, `Node`, `Component`, `StaticModel`, ... | Declared in `Urho3D.h` (planned) |
| Resource system (planned) | `ResourceCache`, `GetResource<T>()` | Declared in `Urho3D.h` (planned) |
| Physics (planned) | Full Bullet integration | Declared in `Urho3D.h` (planned) |

Nova3D's master include (`include/Urho3D/Urho3D.h`) already forward-declares **all the same subsystems** as U3D: `Scene`, `Node`, `StaticModel`, `Material`, `Camera`, `Viewport`, `Light`, `Octree`, `ResourceCache`, `AnimatedModel`, `Physics`, etc. This means Nova3D's **target API is identical to U3D's API** — it is just not yet fully implemented.

### 2.2 Current differences that must be bridged

#### A) Application entry point

| | U3D | Nova3D |
|---|---|---|
| Macro | `URHO3D_DEFINE_APPLICATION_MAIN(ClassName)` | Not used |
| Actual `main()` | Defined by the macro (uses `SharedPtr<Context>`) | Written by hand: `int main() { Context ctx; App app(&ctx); return app.Run(); }` |

#### B) Per-frame update mechanism

| | U3D | Nova3D |
|---|---|---|
| Method | Event subscription: `SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(...))` | Virtual method: `void Update(float timeStep) override` |
| Source | Standard Urho3D event pump | Nova3D-specific addition in `Application` |

#### C) Current Nova3D rendering API (temporary)

Nova3D's **current** simplified renderer (`Program.cpp` demo) uses:
```cpp
GetContext()->GetRenderer()->AddBox(position, size, color);
GetContext()->GetRenderer()->AddGround(size, color);
GetContext()->GetRenderer()->SetCameraMatrices(pos, target, up, fovY, aspect, near, far);
```
This is an **early/temporary API** that exists while the full scene-graph backend is being built. It will be replaced by the standard `Node`/`StaticModel`/`Camera`/`Viewport` API identical to U3D's. **Game code should NOT target this simplified API.**

#### D) Context subsystem accessor style

| | U3D | Nova3D |
|---|---|---|
| Input | `GetSubsystem<Input>()` | `GetContext()->GetInput()` or `GetSubsystem<Input>()` |
| ResourceCache | `GetSubsystem<ResourceCache>()` | `GetSubsystem<ResourceCache>()` |
| Engine | `GetSubsystem<Engine>()` | `GetContext()->GetEngine()` or `GetSubsystem<Engine>()` |

`GetSubsystem<T>()` comes from `Object` and is available on both. Using it is the portable choice.

#### E) `Data/` and `CoreData/` resource directories

U3D needs a `Data/` and `CoreData/` directory next to the executable (containing built-in models, shaders, techniques, fonts). Nova3D (when complete) will have a similar requirement. For the U3D phase, these directories must be copied or symlinked as a post-build step.

---

## 3. Feasibility Assessment

**Adding U3D support is very feasible.** The engines are deeply API-compatible by design (Nova3D targets the same API). The total amount of `#ifdef` guard code needed in game logic is **minimal — two locations only**:

1. The application entry point (`Program.cpp`): one `#ifdef` block.
2. The per-frame update hookup inside the `Application` subclass: one `#ifdef` block.

All actual 3D game logic (scene building, physics, input, UI) written against the common `Urho3D::` scene-graph API will compile and run on **both** engines without guards.

---

## 4. Implementation Plan

### Phase 1 — U3D primary, Nova3D stub (start now)

**Goal:** Start development immediately on U3D. Nova3D support compiles but is not the active engine.

#### 4.1 New CMake option

Add to `CMakeLists.txt`:

```cmake
# Engine backend selection
# Default is U3D while Nova3D is not yet complete.
set(GALAXY_EGGBERT_ENGINE "U3D" CACHE STRING "Engine backend: U3D or NOVA3D")
set_property(CACHE GALAXY_EGGBERT_ENGINE PROPERTY STRINGS U3D NOVA3D)
string(TOUPPER "${GALAXY_EGGBERT_ENGINE}" GALAXY_EGGBERT_ENGINE_UPPER)
```

U3D path in CMake:
```cmake
if(GALAXY_EGGBERT_ENGINE_UPPER STREQUAL "U3D")
    # Path to U3D build/SDK — user sets this via -DU3D_HOME=...
    # or environment variable URHO3D_HOME pointing at the U3D build directory.
    set(U3D_HOME "" CACHE PATH "Path to U3D build directory or installed SDK")
    if(U3D_HOME)
        list(APPEND CMAKE_MODULE_PATH "${U3D_HOME}/cmake/Modules")
    elseif(DEFINED ENV{URHO3D_HOME})
        list(APPEND CMAKE_MODULE_PATH "$ENV{URHO3D_HOME}/cmake/Modules")
    else()
        # Fallback: use local mirror
        list(APPEND CMAKE_MODULE_PATH
            "/rv/data/library/github.com/u3d-community/U3D/cmake/Modules")
    endif()

    find_package(Urho3D REQUIRED)

    target_include_directories(${_game_target} PRIVATE ${URHO3D_INCLUDE_DIRS})
    target_compile_definitions(${_game_target} PRIVATE GE_ENGINE_U3D)
    target_link_libraries(${_game_target} PRIVATE ${URHO3D_LIBRARIES})
```

Nova3D path stays as current code, with added `GE_ENGINE_NOVA3D` define:
```cmake
elseif(GALAXY_EGGBERT_ENGINE_UPPER STREQUAL "NOVA3D")
    add_subdirectory(../nova-3d Nova3D_dep)
    # ... existing backend setup ...
    target_compile_definitions(${_game_target} PRIVATE GE_ENGINE_NOVA3D)
    target_link_libraries(${_game_target} PRIVATE Nova3D)
endif()
```

For the U3D engine, we also need to copy `Data/` and `CoreData/` next to the binary:
```cmake
if(GALAXY_EGGBERT_ENGINE_UPPER STREQUAL "U3D" AND NOT ANDROID AND NOT EMSCRIPTEN)
    add_custom_command(TARGET ${_game_target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${URHO3D_HOME}/bin/Data"
            "$<TARGET_FILE_DIR:${_game_target}>/Data"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${URHO3D_HOME}/bin/CoreData"
            "$<TARGET_FILE_DIR:${_game_target}>/CoreData"
        COMMENT "Copying U3D runtime data directories"
    )
endif()
```

#### 4.2 Restructure Program.cpp

`Program.cpp` currently contains a Nova3D-specific temporary demo. It must be split into:

```
src/GalaxyEggbert/
  Program.cpp           ← entry point only (engine-specific #ifdef)
  GalaxyEggbertApp.hpp  ← Application subclass (small #ifdef for update hookup)
  GalaxyEggbertApp.cpp  ← Setup(), Start(), game bootstrap
  Game/
    GalaxyEggbertGame.hpp  ← engine-agnostic game class
    GalaxyEggbertGame.cpp  ← all actual game logic goes here
```

**Program.cpp** — only the entry point:
```cpp
#include "GalaxyEggbertApp.hpp"

#ifdef GE_ENGINE_U3D
URHO3D_DEFINE_APPLICATION_MAIN(GalaxyEggbertApp)
#else
// Nova3D entry point
int main() {
    Urho3D::Context context;
    GalaxyEggbertApp app(&context);
    return app.Run();
}
#endif
```

**GalaxyEggbertApp.hpp** — the Application subclass:
```cpp
#pragma once
#include <Urho3D/Urho3D.h>
using namespace Urho3D;

class GalaxyEggbertApp : public Application {
    URHO3D_OBJECT(GalaxyEggbertApp, Application);
public:
    explicit GalaxyEggbertApp(Context* context) : Application(context) {}

    void Setup() override;
    void Start() override;
    void Stop() override;

    // Common per-frame update — called by both engine adapters
    void GameUpdate(float dt);

#ifdef GE_ENGINE_U3D
    // U3D uses event subscription; forward to GameUpdate
    void HandleUpdate(StringHash, VariantMap& eventData);
#else
    // Nova3D calls this virtual method directly
    void Update(float dt) override { GameUpdate(dt); }
#endif
};
```

**GalaxyEggbertApp.cpp** — Setup/Start/Stop:
```cpp
#include "GalaxyEggbertApp.hpp"

void GalaxyEggbertApp::Setup() {
    engineParameters_[EP_WINDOW_TITLE] = "Galaxy Eggbert";
    engineParameters_[EP_FULL_SCREEN]  = false;
    engineParameters_[EP_WINDOW_WIDTH] = 1280;
    engineParameters_[EP_WINDOW_HEIGHT] = 720;
#ifdef GE_ENGINE_U3D
    engineParameters_[EP_RESOURCE_PATHS] = "Data;CoreData";
    engineParameters_[EP_LOG_NAME]       = "GalaxyEggbert.log";
#endif
}

void GalaxyEggbertApp::Start() {
    // Build the scene, set up camera, etc. (engine-agnostic Urho3D API)
    // ...

#ifdef GE_ENGINE_U3D
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(GalaxyEggbertApp, HandleUpdate));
#endif
}

#ifdef GE_ENGINE_U3D
void GalaxyEggbertApp::HandleUpdate(StringHash, VariantMap& ed) {
    GameUpdate(ed[Update::P_TIMESTEP].GetFloat());
}
#endif

void GalaxyEggbertApp::GameUpdate(float dt) {
    // delegate to game logic
}

void GalaxyEggbertApp::Stop() {}
```

**GalaxyEggbertGame** is entirely free of engine `#ifdef` — it uses the standard `Urho3D::` scene graph API that both engines implement.

---

### Phase 2 — Nova3D reaches feature parity

When Nova3D implements its full scene graph backend (as declared in its `Urho3D.h` master include), the only changes needed to switch back are:

1. Set `GALAXY_EGGBERT_ENGINE=NOVA3D` in CMake.
2. Verify that Nova3D's `ResourceCache`, `StaticModel`, etc. behave identically to U3D's.
3. Test that `EP_RESOURCE_PATHS` and `Data/CoreData` work under Nova3D.

Game code itself should require **zero changes** if Nova3D's API parity is complete.

---

## 5. Risks and Notes

| Risk | Severity | Mitigation |
|---|---|---|
| Nova3D scene-graph API diverges from U3D in subtle ways | Medium | Write integration tests that run against both engines; keep game code to documented `Urho3D::` API only |
| U3D `Data/CoreData` resource path requirement adds setup friction | Low | Post-build copy step in CMake (see §4.1) |
| U3D uses `SharedPtr<Context>` (ref-counted); Nova3D uses plain `Context` on stack | Low | Handled transparently in the `#ifdef` entry-point wrapper |
| `EP_LOG_NAME` and other engine parameters may differ between U3D and Nova3D | Low | Wrap in `#ifdef GE_ENGINE_U3D` guards inside `Setup()` |
| Nova3D currently uses `Update(float)` virtual; U3D does not declare it | Low | Adapter pattern in `GalaxyEggbertApp` (see §4.2) covers this cleanly |
| Building U3D from source is slow (large codebase) | Medium | Use pre-built SDK (`cmake-build-debug` in the local mirror) or CI cache |

---

## 6. Build Instructions (U3D path)

Point `URHO3D_HOME` at the U3D build directory (the local mirror already has a build):

```bash
export URHO3D_HOME=/rv/data/library/github.com/u3d-community/U3D/cmake-build-debug

cmake -S . -B build-u3d \
    -DGALAXY_EGGBERT_ENGINE=U3D \
    -DURHO3D_HOME=$URHO3D_HOME

cmake --build build-u3d -j$(nproc)
./build-u3d/GalaxyEggbert
```

Nova3D path (unchanged from current setup):

```bash
cmake -S . -B build-nova3d -DGALAXY_EGGBERT_ENGINE=NOVA3D
cmake --build build-nova3d -j$(nproc)
./build-nova3d/GalaxyEggbert
```

---

## 7. Summary

- **Feasibility: HIGH.** Both engines share the same C++ namespace, header paths, and target API. The code bridge needed is tiny (~20 lines of `#ifdef` in two files).
- **Start with U3D immediately.** Nova3D is not ready; U3D provides a stable, complete platform.
- **Write all game logic against the common `Urho3D::` scene-graph API.** Do not use Nova3D's current `AddBox()`/`AddGround()` simplified API — that is a temporary implementation detail that will disappear.
- **When Nova3D matures** and implements its full scene-graph backend, switching is a one-line CMake change with no game-code modifications required.
- **The current `Program.cpp`** (Nova3D simplified demo) must be deleted and replaced with the structure described in §4.2 before game development begins.
