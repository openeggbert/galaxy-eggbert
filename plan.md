# Galaxy Eggbert — Engine Plan

**Date:** 2026-06-13  
**Status:** Active

---

## Current state

Galaxy Eggbert builds against the **Urho3D API** using **U3D** (`u3d-community/U3D`) as the engine backend.

**Nova3D** is Robert Vokac's in-progress fork of Urho3D. It targets the same Urho3D API (same namespace, same headers, same scene-graph types). When Nova3D reaches feature parity with U3D, switching will be a one-line CMake change — game code requires no modifications.

The engine backend is selected at CMake configure time:

```bash
cmake -S . -B cmake-build-u3d -DGALAXY_EGGBERT_ENGINE=U3D     # current default
cmake -S . -B build-nova3d    -DGALAXY_EGGBERT_ENGINE=NOVA3D   # future
```

---

## Rules

- All game code targets the standard `Urho3D::` scene-graph API — no `#ifdef` engine guards anywhere.
- Nova3D must implement the same Urho3D API. If it doesn't compile galaxy-eggbert, Nova3D must fix itself.
- The one known API gap: Nova3D does not implement AngelScript. Galaxy Eggbert does not use AngelScript, so this is not a concern.

---

## Build (U3D, Linux)

U3D must be pre-built at:
```
/rv/data/library/github.com/u3d-community/U3D/cmake-build-debug/lib/libUrho3D.a
```

```bash
cmake --build cmake-build-u3d --target GalaxyEggbert -j2
./cmake-build-u3d/GalaxyEggbert
```

Use `-j2` maximum to protect RAM.

---

## Risks and notes

| Risk | Severity | Mitigation |
|---|---|---|
| Nova3D scene-graph API diverges from U3D in subtle ways | Medium | Keep all game code to documented `Urho3D::` API; Nova3D must fix divergences |
| U3D `Data/CoreData` resource path requirement | Low | Post-build CMake copy step already in place |
| Building U3D from source is slow | Medium | Use pre-built SDK at the path above |
