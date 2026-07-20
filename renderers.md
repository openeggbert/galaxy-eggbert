# renderers.md — Giving Galaxy Eggbert both a lightweight renderer and a "2026" high-fidelity renderer at once

_A design/direction document, not a change to game code. It describes how to architect two
interchangeable renderers behind a single interface. Everything here respects the existing
constraints (faithful remake, `../mobile-eggbert` never modified, no copied code/data, no `#ifdef`
guards for engine differences). Choosing this direction is the project owner's call — this document
commits to nothing; it only proposes._

---

## Short answer

It is realistic, and **half of the work that makes it possible is already done.** The simulation
(`GEBlupiController`, `GEInteractionSystem`, `GEWorldRuntime`) is entirely graphics-free — that is
precisely the purpose of the decoupling / `*ThisFrame()` bus. All rendering lives in
`GalaxyEggbertCnaGame::Draw()` (~817 lines from line 3100) plus `GETerrainRenderer` / `GEHud` /
`GEObjectIcons` / the tile helpers / `GETileAtlas`.

So the simulation already produces state and `Draw()` merely visualizes it — the seam effectively
**already exists**, it just is not named yet. (Verified: the sim has no dependency on
`Easy3D::` / `GraphicsDevice` / `Camera3D`; only the render layer and the editor call graphics.)

---

## The principle: one "scene" interface, two renderers behind it

The key is not "two renderers side by side with `#ifdef`", but **one stable interface between the
simulation and the renderer**. Every frame the simulation produces an engine-neutral *scene
description*, and the renderers are interchangeable implementations of a single interface:

```
        Engine-agnostic core (World/Block/def)   ← already exists, shared
                     │
        Simulation (Blupi, Interaction, WorldRuntime)   ← already graphics-free ✔
                     │
                     ▼
          SceneFrame  (camera, tiles+animFrame, billboards+icons,
                       Blupi state, HUD elements, skyRegion/background)   ← NEW, this is the seam
                     │
         ┌───────────┴────────────┐
         ▼                        ▼
   GERendererLite            GERendererHi ("2026")
   Easy3D billboards+cubes   3D models, PBR, shadows/GI,
   2D HUD, also runs on web  particles, bloom/SSAO/tonemap
```

Both implementations consume the **same `SceneFrame`**. The game does not change — only who draws
the scene changes.

---

## Concrete steps given today's code

1. **Extract a `SceneFrame` out of `Draw()`.** Today `Draw()` reaches directly into sim/world
   state. The first move is to make `Draw()` read **only** a `SceneFrame`:
   - camera (position, orientation, FOV),
   - terrain: a list of `(cell, blockType, renderMode, animFrame)`,
   - dynamic objects: a list of `(worldPos, objectType, icon/animState, facing)`,
   - Blupi: `(worldPos, animState, direction, vehicleMode, secretPower)`,
   - HUD: elements in the 640×480 reference space,
   - environment: `skyRegion`, background id.

   This is the hardest and most valuable step — and it also breaks up the "god" methods (RC-4 in
   `REMAKE-ANALYSIS.md`).
2. **Define `IGameRenderer`** — `BeginFrame(camera)`, `SubmitTerrain(...)`, `SubmitBillboard(...)`,
   `SubmitHud(...)`, `EndFrame()`. Today's code becomes `GERendererLite : IGameRenderer`.
3. **Add `GERendererHi`** as a second implementation, selected **at runtime** (config/flag), not via
   `#ifdef` — which respects the rule in `CLAUDE.md`. Lite stays the default for web/WASM and
   low-end machines; Hi is for desktop.

---

## What the "2026" renderer does differently

- Instead of flat billboards → **real 3D models** for tiles/objects (normal maps, PBR materials),
  real-time **shadows + GI**, particle systems (explosions, water, smoke), a post-processing stack
  (bloom, SSAO, tonemapping), better camera/depth.
- **A useful side effect:** the hi-fi path actually **solves RC-5 and the "billboard walk-cycle"
  problem** from the analysis — once characters/tiles are real 3D models, the guessing of "how to
  show a 2D sprite in 3D" disappears and enemies no longer "walk sideways" when viewed at an angle.
  The cost is that it needs **new 3D assets that do not exist today** (this is also the long-standing
  "no visible 3D Blupi model" blocker).

---

## Decisions that are the owner's to make (with hard consequences)

This is a change in the project's direction, so these decisions must come from the top — do not
start implementing until they are confirmed:

1. **Direction lock.** Today `CLAUDE.md`/`NEXT.md` lock "Direct CNA + Easy3D is the sole target." A
   dual renderer widens that — a deliberate change to the lock.
2. **Faithful-remake line.** It must hold: **only presentation changes, not mechanics.** Both
   versions play identically (same physics, timing, objects). "Amazing graphics" is a presentation
   choice, not a new gameplay mechanic — that is fine, but the gameplay must not move.
3. **Assets.** The hi-fi renderer needs an additional (optional) set of 3D models + PBR textures.
   Lite keeps using the mobile-eggbert PNG sprites. Who supplies those models, and how?
4. **Engine for hi-fi** — two paths:
   - **A) The same CNA/Easy3D base with beefed-up shaders/materials** — less work, shares code, but
     the quality ceiling is set by CNA.
   - **B) A separate modern engine backend** behind the same `IGameRenderer` — a higher "2026
     amazing" ceiling, but more work and a second dependency tree.

---

## Bonus: synergy with the earlier analysis

That `SceneFrame` is **exactly the artifact** the P0 parity/golden harness in `REMAKE-ANALYSIS.md`
needs — a deterministic per-frame description that can be diffed. So this refactor and the fix for
the "piles of problems" pull in the same direction: a clean sim↔render seam enables both a dual
renderer *and* automated correctness checking.

---

## Recommended sequence

Do **steps 1–2 (`SceneFrame` + `IGameRenderer`) first**, with today's renderer as `GERendererLite`.
That has value on its own (it breaks up the god objects and enables the golden harness) and
**commits to nothing regarding hi-fi**. The hi-fi renderer is then purely an addition behind a
finished interface.

Open for decision: item 4 — **A (same CNA base)** vs **B (separate modern engine)** — this
fundamentally changes the scope and plan of the hi-fi path.
