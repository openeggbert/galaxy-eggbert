# Simple3D Migration Task

Source: user instructions, 2026-06-23.

## Goal

Migrate `galaxy-eggbert` from direct Urho3D/Nova3D API usage to the new high-level `simple-3d` API.

## Context

* `galaxy-eggbert` currently runs directly on Urho3D / Nova3D style API.
* `simple-3d` is a high-level C++ framework over Urho3D/Nova3D.
* `simple-3d` is not fully complete yet.
* This migration is allowed to produce a non-compiling intermediate state, but the repository must become structurally ready for a Simple3D-based version.
* Do not throw away working Urho3D code yet. Keep it as legacy/reference until the Simple3D port reaches playable state.

## Architecture target

```
galaxy-eggbert game code
  -> simple-3d API
    -> Urho3D / Nova3D
      -> CNA / backend
```

## Files to read before starting

### galaxy-eggbert

* `README.md`
* `plan.md`
* `next.md`
* `CMakeLists.txt`
* `src/GalaxyEggbert/Program.cpp`
* `src/GalaxyEggbert/GalaxyEggbertApp.hpp`
* `src/GalaxyEggbert/GalaxyEggbertApp.cpp`
* `src/GalaxyEggbert/Game/GalaxyEggbertGame.hpp`
* `src/GalaxyEggbert/Game/GalaxyEggbertGame.cpp`
* `src/GalaxyEggbert/Game/Blupi.hpp`
* `src/GalaxyEggbert/Game/Blupi.cpp`
* `src/GalaxyEggbert/Game/Decor.hpp`
* `src/GalaxyEggbert/Game/Decor.cpp`
* `src/GalaxyEggbert/Game/ObjectNode.hpp`
* `src/GalaxyEggbert/Game/ObjectNode.cpp`
* `src/GalaxyEggbert/Game/HUD.hpp`
* `src/GalaxyEggbert/Game/HUD.cpp`
* `src/GalaxyEggbert/Game/SoundManager.hpp`
* `src/GalaxyEggbert/Game/SoundManager.cpp`
* `src/GalaxyEggbert/Game/Camera.hpp`
* `src/GalaxyEggbert/Game/Camera.cpp`
* `src/GalaxyEggbert/Game/Explosion.hpp`
* `src/GalaxyEggbert/Game/Explosion.cpp`
* `src/GalaxyEggbert/Game/ScorePopup.hpp`
* `src/GalaxyEggbert/Game/ScorePopup.cpp`
* `include/GalaxyEggbert/Worlds/*`
* `include/GalaxyEggbert/BlockTypes.hpp`
* `worlds/world001.txt`

### simple-3d (`../simple-3d`)

* `README.md`
* `API.md`
* `plan.md`
* `CLAUDE.md`
* `include/Simple3D/Simple3D.h`
* `include/Simple3D/Core/Game.h`
* `include/Simple3D/Core/Keys.h`
* `include/Simple3D/Core/Collision.h`
* `include/Simple3D/Entity/Entity.h`
* `include/Simple3D/Camera/Camera.h`
* `include/Simple3D/CharacterController/CharacterController.h`
* `include/Simple3D/UI/Label.h`
* `docs/tutorials/09_blupi_proto.md`
* `docs/tutorials/13_character_controller.md`
* `docs/tutorials/16_gamepad.md`
* `samples/blupi_proto/main.cpp`

## Rules

* New Simple3D-facing code must include `<Simple3D/Simple3D.h>`, not `Urho3D/Urho3DAll.h`.
* Do not use `Urho3D::Context`, `Urho3D::Scene`, `Urho3D::Node`, `Urho3D::Material`, `Urho3D::Texture2D`, `Urho3D::BillboardSet`, `Urho3D::Text`, or `Urho3D::SoundSource` in the new Simple3D port code.
* Keep old Urho3D code temporarily under the existing files or move it under `src/GalaxyEggbert/LegacyUrho/`.
* Do not delete gameplay logic just because Simple3D lacks an API today.
* If Simple3D lacks an API, create a clearly documented TODO/stub/wrapper in the GalaxyEggbert Simple3D migration layer.
* Do not edit `simple-3d` in this pass unless absolutely necessary. Prefer documenting missing Simple3D features in `docs/SIMPLE3D_GAPS.md`.

## New file structure to create

```
src/GalaxyEggbertSimple3D/
  Program.cpp
  GalaxyEggbertSimpleGame.hpp
  GalaxyEggbertSimpleGame.cpp

  Game/
    GEWorldRuntime.hpp
    GEWorldRuntime.cpp
    GETerrainRenderer.hpp
    GETerrainRenderer.cpp
    GEBlupiController.hpp
    GEBlupiController.cpp
    GEDecorSystem.hpp
    GEDecorSystem.cpp
    GEHud.hpp
    GEHud.cpp
    GESound.hpp
    GESound.cpp
    GECameraRig.hpp
    GECameraRig.cpp

  Support/
    Simple3DMissingFeatures.hpp
    Simple3DMigrationNotes.hpp

docs/simple3d_migration.md
docs/SIMPLE3D_GAPS.md
```

## CMake requirements

* Add a new target `GalaxyEggbertSimple3D` (do not replace the existing `GalaxyEggbert` target yet).
* Add cache path `SIMPLE3D_HOME=/path/to/simple-3d`; default to `../simple-3d` if it exists.
* Use `add_subdirectory(../simple-3d Simple3D_dep)` when appropriate.
* Link the new target against `Simple3D`.
* Compile shared non-engine data/model sources:
  * `src/GalaxyEggbert/Worlds/*.cpp`
  * `include/GalaxyEggbert/Worlds/*.hpp`
  * `include/GalaxyEggbert/BlockTypes.hpp`
  * `include/GalaxyEggbert/def/*.hpp`
  * `src/GalaxyEggbert/Game/Tables.cpp` if needed
* Do not compile old Urho3D-specific sources into the Simple3D target.

## Phase 1 — Simple3D app skeleton

Implement:

```cpp
class GalaxyEggbertSimpleGame final : public Simple3D::Game
{
public:
    void Start() override;
    void Update(float dt) override;
    void Stop() override;
};
```

Startup should:
* set window title: `Galaxy Eggbert`
* set window size: `1280x720`
* set clear color
* set resource prefix paths if needed
* create camera
* create minimal HUD labels
* load `worlds/world001.txt` or fallback demo world
* create placeholder terrain
* create placeholder Blupi player

Use Simple3D API:
* `CreateEntity`
* `CreateCamera`
* `CreateLabel`
* `PlaySound`
* `PlayMusic`
* `IsKeyDown`
* `IsKeyPressed`
* `IsGamepadButtonPressed`
* `GetGamepadAxis`
* `CharacterController`
* `CollisionLayer`
* `MakeCollisionMask`

If Simple3D Input Actions exist, use them.
If they do not exist yet, use direct Simple3D key/gamepad calls and write a TODO saying this should move to Task #24B Input Actions later.

## Phase 2 — Port world loading (not Urho rendering)

Keep the existing world data code:
* `GalaxyEggbert::Worlds::World`
* `.txt` mobile-eggbert loader logic
* `BlockTypes`

Move or copy only engine-independent logic from `GalaxyEggbertGame::LoadMobileEggbertTerrain()` into the new Simple3D world runtime.

Do not depend on `Urho3D::Vector3`. Use `Simple3D::Vector3`.

Required runtime state:
* current world number
* world name
* `World` instance
* parsed `blupiPos`
* parsed `region`
* parsed `MoveObject` list
* treasure count
* basic level timer

## Phase 3 — Placeholder Simple3D terrain

Because Simple3D currently does not expose dynamic material/atlas/UV APIs equivalent to the old Urho3D `Material + Texture2D + shader parameter` code, do not try to fully reproduce the old textured terrain in this pass.

Instead:
* build a simple block terrain using `CreateEntity`
* each solid block gets:
  * `AddModel("Models/Box.mdl")`
  * `SetPosition`
  * `SetScale`
  * `AddRigidBody(0.0f)`
  * `AddBoxCollider(Vector3(1,1,1))`
  * `SetCollisionLayer(CollisionLayer::StaticGeometry)`
* use a small world area if needed for performance, or the whole 100x100 if acceptable.
* add comments where tile-specific material/UV support is missing.

Do not use direct Urho3D materials in the new Simple3D renderer.

Document missing Simple3D features in `docs/SIMPLE3D_GAPS.md`:
* tile atlas material / UV offset API
* billboard sprite API
* sky dome / fog / zone API
* UI image/gauge/icon API
* particle/explosion API
* richer audio channels if needed

## Phase 4 — Blupi as Simple3D character

Create `GEBlupiController`.

Use Simple3D:
* player entity root
* `AddCharacterController`
* `CollisionLayer::Actor`
* collision mask: `StaticGeometry | Trigger`
* optional visual child entity
* camera follows visual or root

Controls:
* arrow left/right = turn
* arrow up/down = forward/back
* LCtrl or Space = jump
* LShift = crouch placeholder
* RShift = glide/look-up placeholder
* gamepad left stick = movement/turn if simple enough
* gamepad A = jump

Port foundation:
* position
* facing yaw
* movement
* jump
* gravity handled through CharacterController where possible
* respawn if falling below limit
* shield timer placeholder
* death/freeze placeholder if easy

Keep original `Blupi.cpp` as reference, but do not use Urho3D APIs in the new controller.

## Phase 5 — Decor as Simple3D object system foundation

Create `GEDecorSystem`.

Reuse logical concepts from current `Decor`:
* ObjectType
* posStart / posEnd
* movement speed
* active flag
* treasure count
* keys
* exit
* enemies
* pickups

For rendering:
* use placeholder `Simple3D::Entity` objects.
* pickups can be small boxes/spheres/cubes for now.
* enemies can be placeholder boxes.
* triggers should use Simple3D trigger volumes where practical:
  * treasure / egg / shield / keys / exit use `AddTriggerSphere`
  * set trigger layer/mask correctly
  * callbacks should check player entity before collecting

If trigger logic is not enough yet, keep existing manual distance checks temporarily, but make the rendering Simple3D-based.

Do not use `ObjectNode`, `BillboardSet`, or Urho3D sprite APIs in new Simple3D code.

## Phase 6 — HUD minimally

Create `GEHud`. Use only `Simple3D::Label` / `Simple3D::UI::Label` for now.

Show:
* world name
* lives
* treasures collected / total
* score
* keys
* shield timer
* controls hint

Do not attempt to port:
* gauge image
* life icons
* key icons
* hit flash image
* complex overlay UI

Add these to `docs/SIMPLE3D_GAPS.md` as missing Simple3D UI features.

## Phase 7 — Sound minimally

Create `GESound`.

Use:
* `Game::PlaySound(path)`
* `Game::PlayMusic(path)`
* `Game::SetMasterVolume(volume)`

Do not port the full channel system yet. Map the most important sounds:
* jump
* collect
* hit/death
* win
* landing if easy

If old `SoundManager` channel behavior cannot be represented, document it as a gap.

## Phase 8 — Camera minimally

Create `GECameraRig`.

Use Simple3D camera:
* `CreateCamera`
* `SetOrbitMode` or `Follow`
* `SetCollisionEnabled(true)`
* `SetFOV`
* `SetFarClip`

Do not keep old direct `Urho3D::Camera` code in the Simple3D path.

## Phase 9 — Documentation and migration notes

Create or update `docs/simple3d_migration.md` with:
* current old architecture
* target Simple3D architecture
* what was migrated
* what still uses legacy Urho3D
* what does not compile yet, if anything
* next steps

Create or update `docs/SIMPLE3D_GAPS.md` with a table:

```
Feature needed by Galaxy Eggbert | Currently in Simple3D? | Temporary workaround | Proposed Simple3D API
```

At minimum include:
* billboard sprites / atlas cells for Blupi and objects
* tile atlas material UVs
* terrain batching / chunk mesh generation
* sky dome / fog / zone
* UI images / gauges / icons / panels
* particles / explosion sprites
* input actions if missing
* save/load integration if needed

## plan.md additions

Add a new migration section `Simple3D Migration` with:
* `[ ] S3D-1 — Simple3D port skeleton` (this first pass)
* `[ ] S3D-2 terrain visual fidelity`
* `[ ] S3D-3 Blupi sprite/billboard animation`
* `[ ] S3D-4 Decor object visuals`
* `[ ] S3D-5 HUD images/gauges/icons`
* `[ ] S3D-6 phase/menu port`
* `[ ] S3D-7 sound channel parity`
* `[ ] S3D-8 web/android verification`
* `[ ] S3D-9 remove legacy Urho path after parity`

## Acceptance criteria for this pass (S3D-1)

* A Simple3D-based target or source tree exists.
* New Simple3D code does not include `Urho3D/Urho3DAll.h`.
* Legacy Urho3D code is preserved as reference.
* The project has a clear migration document.
* World loading logic is reused in the Simple3D path.
* Basic placeholder terrain exists in the Simple3D path.
* Basic placeholder player exists in the Simple3D path.
* Basic camera and HUD exist in the Simple3D path.
* Missing Simple3D APIs are documented instead of being hacked with direct Urho3D calls.
* If the Simple3D target does not compile yet, document exact compile blockers and missing APIs in `docs/simple3d_migration.md`.
* Do not attempt full visual parity in this pass.
* Do not port every gameplay feature in one pass.
* Do not delete the working Urho3D version until the Simple3D version is playable.
