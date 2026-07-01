# Simple3D Migration — Galaxy Eggbert

> **Direction note:** this document describes the Simple3D path. That path is now superseded as
> the long-term target by Direct CNA + Easy3D (see `easy3d.md`, `plan.md` §"Direct CNA + Easy3D
> Migration"). The Simple3D implementation (`GalaxyEggbertSimple3D`) remains a working
> historical/reference implementation until the CNA/Easy3D target reaches parity.

## Architecture

### Current (legacy Urho3D)

```
galaxy-eggbert game code (src/GalaxyEggbert/)
  -> Urho3D API directly (Context, Scene, Node, Material, BillboardSet, …)
    -> libUrho3D.a (u3d-community/U3D)
```

### Target (Simple3D)

```
galaxy-eggbert game code (src/GalaxyEggbertSimple3D/)
  -> Simple3D API (Simple3D/Simple3D.h — no Urho3D types in game code)
    -> Simple3D library (../simple-3d)
      -> Urho3D / Nova3D
        -> CNA / backend
```

---

## S3D-1 Pass — What Was Migrated (Phase 80)

| Component | Old (Urho3D) | New (Simple3D) | Status |
|---|---|---|---|
| App entry point | `GalaxyEggbertApp` (Urho3D::Application) | `GalaxyEggbertSimpleGame` (Simple3D::Game) | Done (skeleton) |
| Game coordinator | `GalaxyEggbertGame` | `GalaxyEggbertSimpleGame` (merged) | Done (skeleton) |
| World data loader | `LoadMobileEggbertTerrain()` (Urho3D FS) | `GEWorldRuntime::LoadFromMobileEggbertFile()` (std::ifstream) | Done |
| Voxel world model | `GalaxyEggbert::Worlds::World` | Same (reused verbatim) | Done (shared) |
| Terrain renderer | `SpawnTerrainNodes()` (Urho3D nodes + Material UV) | `GETerrainRenderer::Build()` (Simple3D::Entity grey boxes) | Done (no texture) |
| Player controller | `Blupi` (Urho3D BillboardSet + manual physics) | `GEBlupiController` (CharacterController) | Done (no sprite) |
| Object system | `Decor` (Urho3D ObjectNode) | `GEDecorSystem` (Simple3D Entity + trigger spheres) | Done (basic) |
| HUD | `HUD` (Urho3D UI Text + BorderImage) | `GEHud` (Simple3D::Label only) | Done (text-only) |
| Sound | `SoundManager` (93 Urho3D SoundSource channels) | `GESound` (Game::PlaySound, 5 sounds mapped) | Done (minimal) |
| Camera | `CameraController` (manual Urho3D orbit + DDA) | `GECameraRig` (Camera::SetOrbitMode) | Done |
| Input | Urho3D key enums, direct polling | `BindAction / BindAxis2D / IsActionPressed` | Done |
| CMake target | `GalaxyEggbert` | `GalaxyEggbertSimple3D` (new, separate) | Done |

---

## What Still Uses Legacy Urho3D

The entire `src/GalaxyEggbert/` tree is unchanged and still builds as the `GalaxyEggbert` target. It is kept as the reference implementation until the Simple3D port reaches playable state.

Do not delete or modify `src/GalaxyEggbert/` until task S3D-9 is reached.

---

## What Does Not Compile Yet (S3D-1)

The `GalaxyEggbertSimple3D` target will only compile when:
1. `GALAXY_EGGBERT_BUILD_SIMPLE3D=ON` is passed to CMake.
2. A valid `simple-3d` source tree exists at `../simple-3d` (or `SIMPLE3D_HOME`).
3. `simple-3d` itself compiles against U3D or Nova3D.

If `simple-3d` is missing or incomplete, the new target simply won't be configured. The existing `GalaxyEggbert` target is unaffected.

### Known compile blockers in the Simple3D port (to be resolved by simple-3d):
- `Entity::AddBillboard` exists but no UV-crop API yet (needed for sprite sheets).
- No `Game::SetFogColor` or ambient zone API.
- No camera-shake API on `Camera`.

See `docs/SIMPLE3D_GAPS.md` for the full gap table.

---

## How to Build the Simple3D Target

```bash
cmake -S . -B cmake-build-simple3d \
  -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON \
  -DGALAXY_EGGBERT_ENGINE=U3D

cmake --build cmake-build-simple3d --target GalaxyEggbertSimple3D -j2

./cmake-build-simple3d/GalaxyEggbertSimple3D
```

Or override the simple-3d path:

```bash
cmake -S . -B cmake-build-simple3d \
  -DGALAXY_EGGBERT_BUILD_SIMPLE3D=ON \
  -DSIMPLE3D_HOME=/path/to/simple-3d
```

---

## Next Steps

| Task | Description |
|---|---|
| S3D-2 | Terrain visual fidelity: tile atlas material + UV offset per block type |
| S3D-3 | Blupi sprite/billboard animation from `blupi.png` |
| S3D-4 | Decor object visuals: enemy and pickup billboard sprites from `element.png` |
| S3D-5 | HUD images: gauge sprite, life icons, key icons, hit flash |
| S3D-6 | Phase/menu port: Init gamer select, Settings, Ranking screen |
| S3D-7 | Sound channel parity: 93 channels, per-channel volume, loop control |
| S3D-8 | Web/Android build verification with Simple3D backend |
| S3D-9 | Remove legacy Urho3D path after Simple3D version reaches playable parity |
