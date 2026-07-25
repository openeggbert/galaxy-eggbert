# renderers.md — Giving Galaxy Eggbert both a lightweight renderer and a "2026" high-fidelity renderer at once

_A design/direction document, not a change to game code. It describes how to architect two
interchangeable renderers behind a single interface. Everything here respects the existing
constraints (faithful remake, `../mobile-eggbert` never modified, no copied code/data, no `#ifdef`
guards for engine differences). Choosing this direction is the project owner's call — this document
commits to nothing; it only proposes._

---

## Short answer

It is realistic, and **half of the work that makes it possible is already done.** The simulation
(`BlupiController`, `InteractionSystem`, `WorldRuntime`) is entirely graphics-free — that is
precisely the purpose of the decoupling / `*ThisFrame()` bus. All rendering lives in
`GalaxyEggbertGame::Draw()` (~817 lines from line 3100) plus `TerrainRenderer` / `Hud` /
`ObjectIcons` / the tile helpers / `TileAtlas`.

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
     the quality ceiling is set by CNA. **← CHOSEN by the owner, 2026-07-20.**
   - **B) A separate modern engine backend** behind the same `IGameRenderer` — a higher "2026
     amazing" ceiling, but more work and a second dependency tree.

   The phased plan below is written for **path A**.

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

Decision recorded: item 4 is **A (same CNA base)** (owner, 2026-07-20). The phased plan below is
written for path A.

---

## Phased plan (path A — same CNA/Easy3D base)

Path A means: **one game, one CNA base, one `SceneFrame`; the hi-fi look is added by upgrading the
render layer (shaders, materials, lighting, effects), not by bolting on a second engine.** Lite and
Hi share geometry (cubes, quads, the tile atlas) and the same `SceneFrame`; they differ only in how
they shade and post-process it. The plan is ordered so each phase has standalone value and never
blocks on the still-missing 3D assets.

**Ground rules for every phase (non-negotiable):**
- **Presentation only.** No phase may change physics, timing, collision, object behavior, or any
  gameplay value. If a render change would require touching the sim, stop and re-scope.
- **No `#ifdef` for the Lite/Hi split.** The choice is a runtime selection behind `IGameRenderer`.
- **Lite output must stay byte-for-byte identical** through Phases 1–3 (that is what the golden
  harness in Phase 3 proves). Hi is the only place new visuals appear.
- **`../mobile-eggbert` / `../simple-3d` untouched; no copied code/data.** Sibling repos
  (`../easy-3d`, `../cna`) are modified only via their own approved tasks, never silently from here.
- Each phase ends with the existing sign-off ritual: build → `Verify*` → full `ctest` → live
  headless check → commit → push.

### Phase 1 — Introduce `SceneFrame` (the seam)

Define an engine-neutral per-frame scene description and make the current `Draw()` consume **only**
it. Do it incrementally so the game stays runnable the whole time:

1. `SceneFrame` struct: camera; terrain draw list `(cell, blockType, renderMode, animFrame)`;
   billboard list `(worldPos, objectType, icon/animState, facing)`; Blupi `(worldPos, animState,
   direction, vehicleMode, secretPower)`; HUD elements (640×480 ref space); environment
   (`skyRegion`, background id).
2. A `SceneFrameBuilder` that reads sim/world state into a `SceneFrame` once per frame (this is where
   the logic currently inlined in `Draw()`/`GalaxyEggbertGame` moves to).
3. Repoint `Draw()` to read the `SceneFrame` only — first camera + terrain, then billboards, then
   HUD, verifying the frame is visually unchanged after each slice.

_Value on its own:_ breaks up the ~2000-line god methods (RC-4 in `REMAKE-ANALYSIS.md`) and produces
the artifact Phase 3 needs. _Verify:_ live headless screenshots identical to pre-refactor at each
slice; full `ctest`.

### Phase 2 — Define `IGameRenderer`; wrap today's code as `GERendererLite`

1. `IGameRenderer` interface: `BeginFrame(camera)`, `SubmitTerrain(...)`, `SubmitBillboard(...)`,
   `SubmitHud(...)`, `EndFrame()`.
2. Move the Phase-1 `Draw()` body into `GERendererLite : IGameRenderer` (verbatim — no visual
   change).
3. Runtime selection scaffold (a config value / launch flag) that constructs the active renderer;
   default **Lite**. Only `GERendererLite` exists yet, so behavior is unchanged.

_Verify:_ Lite still pixel-identical; `ctest`; the runtime flag switches an (as-yet only Lite)
renderer without touching sim.

### Phase 3 — Golden-image / behavioral harness on `SceneFrame` (locks the refactor)

This is P0-1 from `REMAKE-ANALYSIS.md`, now cheap because the seam exists.

1. A deterministic headless capture target: fixed world + fixed camera + scripted input, N
   deterministic ticks, screenshots at known frames (promote the existing throwaway `xvfb-run`
   instrumentation into a permanent, committed target).
2. Golden images for a few representative worlds/objects, diffed in `ctest`.
3. Optionally a per-tick behavioral trace (position/velocity/anim-state) diffed as text.

_Value on its own:_ turns "a human catches render regressions by eye" into "CI catches them." From
here on, **any accidental change to Lite fails CI** — which is exactly what makes Phase 4 safe.

### Phase 4 — `GERendererHi` on the same CNA base (the hi-fi look)

Add a second `IGameRenderer` that consumes the same `SceneFrame` but shades it richly. Because
path A shares CNA, this reuses the existing cube/quad geometry and atlas; the new work is the
shading/effects pipeline. Order by value-per-effort, each independently visible:

1. **Lighting + real-time shadows** on the existing cube/billboard geometry (directional sun +
   shadow map). Biggest visual jump for least asset work.
2. **Materials / PBR-ish shading** — normal/roughness where textures allow; richer terrain and water
   shading. (Water is the single most common tile set — high payoff.)
3. **Post-processing stack** — bloom, SSAO, tonemapping, colour grading.
4. **Particle systems** — explosions, splashes, smoke, sparks driven off existing effect
   `ObjectType`s in the `SceneFrame` (no new gameplay — same events, richer visuals).

Caveat honestly recorded: **the quality ceiling on path A is CNA's own effect system.** CNA ships
XNA-style `BasicEffect`/`SkinnedEffect`; a genuinely "2026" look will likely need **custom
shaders/effects inside CNA** (a `../cna`-side task, owner-approved) rather than only tuning the
built-in effects. Also note the known Vulkan-backend `BasicEffect` alpha bug (see `NEXT.md` §5) —
the hi-fi path must be validated on both backends, or explicitly scoped to one.

_Verify:_ Hi rendered under the Phase-3 harness with its **own** goldens; Lite goldens must stay
untouched (proving Hi changes nothing for Lite). Runtime flag switches Lite⇄Hi.

### Phase 5 — Optional 3D-model / PBR asset track (asset-gated)

The deepest "amazing" step and the one that also erases the billboard walk-cycle problem (RC-5):
replace flat billboards with **real 3D models** (Blupi, enemies, key objects) in `GERendererHi`
only. Strictly gated on assets existing.

1. Decide the model/rig format (owner input — this is the long-standing "no visible 3D Blupi model"
   blocker; do not commit to a format unilaterally).
2. An optional asset set (models + PBR textures) loaded only by Hi; Lite keeps the PNG sprites.
3. Hi swaps billboards → models where a model exists, falls back to the billboard where it does not
   — so the track can land incrementally, one model at a time.

Until assets arrive, **Phase 4 already delivers a visibly upgraded Hi renderer** (better lighting,
materials, post, particles) on the existing sprite/cube content — Phase 5 is purely additive.

### Suggested order to actually start

**Phases 1 → 2 → 3 first.** They commit to nothing about hi-fi, are pure refactor + test
infrastructure, keep Lite provably identical, and directly pay down the "piles of problems." Only
then start Phase 4, one effect at a time, each behind its own Hi golden.
