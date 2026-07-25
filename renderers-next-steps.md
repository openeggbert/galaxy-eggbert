# renderers-next-steps.md — Dual-renderer work: current status and what is needed next

_Companion to `renderers.md` (the design + phased plan, path A). This file records what has actually
been done so far on the branch, the environment blocker that stops further verified work here, and
the concrete remaining steps with what each one needs. Keep it factual; update it as steps land._

---

## 1. Done so far (committed on this branch)

- **`REMAKE-ANALYSIS.md`** — root-cause analysis of the recurring "piles of problems".
- **`renderers.md`** — dual-renderer design, owner's decision (**path A**: same CNA/Easy3D base,
  hi-fi via upgraded shaders/materials behind one `IGameRenderer`), and the 5-phase plan.
- **`src/GalaxyEggbert/Game/SceneFrame.hpp`** — the `SceneFrame` data contract (Phase 1,
  step 1). Engine-neutral (no Easy3D/CNA graphics types), grounded in the real enums
  (`ObjectType`, `AnimState`, `VehicleMode`, `SecretPower`) and the real dynamic-object shape
  (`MobileObjSpec`). Isolated and non-breaking: nothing includes it yet, it holds no logic, `Draw()`
  is not repointed, and `IGameRenderer` does not exist yet. HUD is deliberately left out of this
  first cut (modelling `Hud`'s 640×480 space blind would be a guess). Enum references were verified
  against their definitions by inspection.

## 2. The environment blocker (why further steps are not done here)

`GalaxyEggbertCNA` **cannot be built or run in the current session**: the sibling repositories it
depends on are absent — `../cna` (the CNA framework), `../easy-3d` (Easy3D helpers), `../easy-gl`
(GL backend). Only `galaxy-eggbert` itself is checked out. `../mobile-eggbert` and `../simple-3d`
are also absent (not needed to build CNA, but noted).

Consequence: the `SceneFrame` **data contract** was safe to write blind (pure data, no logic,
included by nothing). Everything remaining touches the render layer and **must be compiled and
verified** — doing it without a build would reintroduce exactly the unverified churn that
`REMAKE-ANALYSIS.md` identifies as the core problem. So no further code was written until the build
question is decided.

The sibling repos exist and are addable to a session (`openeggbert/easy-3d`, `openeggbert/cna`,
`openeggbert/easy-gl`).

## 3. How to continue — three options (owner's call)

1. **Add the sibling repos to the session and build (recommended).** Clone `openeggbert/easy-3d`,
   `openeggbert/cna`, `openeggbert/easy-gl` alongside `galaxy-eggbert`, build `GalaxyEggbertCNA`,
   then do every remaining step verified (build → `Verify*` → `ctest` → headless screenshot).
   Aligned with the whole goal of ending unverified churn. Downside: heavier setup, and the CNA
   build has been finicky in this environment before (see `NEXT.md` §2/§5).
2. **Continue writing code unverified; owner builds.** Write the builder, `IGameRenderer`, and the
   `Draw()` repoint without compiling here; the owner builds and verifies on their own machine.
   Faster now, but risks compile errors and is precisely the unverified pattern being fought.
3. **Stop at `SceneFrame` for now.** Leave the data contract in place and do no further code until a
   buildable environment is available.

## 4. Remaining steps (path A), with what each needs

Ordered as in `renderers.md`. Each ends with the standard sign-off: build → `Verify*` →
full `ctest` → live headless check → commit → push.

### Phase 1 (finish the seam) — needs a build
- **1b. `SceneFrameBuilder`.** A function/class that reads the live sim/world state
  (`BlupiController`, `WorldRuntime`'s `MobileObjSpec` list, camera, `world.skyRegion()`) into a
  `SceneFrame` once per frame. This is where logic currently inlined in
  `GalaxyEggbertGame::Update`/`Draw` moves to.
- **1c. Repoint `Draw()` to consume only the `SceneFrame`.** Do it in slices — camera + terrain
  first, then billboards/objects, then Blupi — verifying the frame is visually unchanged after each
  slice (this is where the golden harness, Phase 3, pays for itself).
- **1d. Add HUD to `SceneFrame`.** Deferred from the first cut; model it against the real `Hud`
  640×480 elements once a build can verify the mapping.

### Phase 2 (renderer interface) — needs a build
- Define `IGameRenderer` (`BeginFrame(camera)`, `SubmitTerrain(...)`, `SubmitBillboard(...)`,
  `SubmitHud(...)`, `EndFrame()`).
- Move the Phase-1 `Draw()` body into `GERendererLite : IGameRenderer` (verbatim, no visual change).
- Add a runtime renderer-selection scaffold (config/flag), default **Lite**. No `#ifdef`.

### Phase 3 (golden/behavioral harness) — needs a build; unlocks safe hi-fi work
- Promote the existing throwaway `xvfb-run` instrumentation into a permanent, committed,
  deterministic headless-capture target (fixed world + camera + scripted input, screenshots at known
  frames).
- Add golden-image diffing in `ctest`; optionally a per-tick behavioral trace diff. From here, any
  accidental change to Lite fails CI.

### Phase 4 (the hi-fi look, `GERendererHi`) — needs a build; each effect behind its own Hi golden
- Add a second `IGameRenderer` consuming the same `SceneFrame`, in value-per-effort order:
  lighting + real-time shadows → materials/PBR-ish shading (water first, most common tiles) →
  post-processing (bloom, SSAO, tonemapping) → particle systems (off existing effect `ObjectType`s).
- **Ceiling caveat:** path A's quality is bounded by CNA's effect system (XNA-style
  `BasicEffect`/`SkinnedEffect`); a genuinely "2026" look will likely need **custom shaders inside
  CNA** — a `../cna`-side, owner-approved task, not a change made silently from here. Validate on
  both graphics backends, or scope explicitly to one (note the known Vulkan `BasicEffect` alpha bug,
  `NEXT.md` §5).

### Phase 5 (optional 3D-model / PBR asset track) — asset-gated, owner input required
- Choose the model/rig format (the long-standing "no visible 3D Blupi model" blocker — do not commit
  to a format unilaterally).
- Load an optional asset set (models + PBR textures) in `GERendererHi` only; Lite keeps the sprites.
- Swap billboards → models where a model exists, fall back to the billboard otherwise, so the track
  lands incrementally. This also erases the billboard walk-cycle problem (RC-5 in
  `REMAKE-ANALYSIS.md`).

## 5. Guardrails (unchanged, apply to every step)

- **Presentation only** — no phase changes physics/timing/collision/gameplay values.
- **No `#ifdef`** for the Lite/Hi split — runtime selection behind `IGameRenderer`.
- **Lite stays pixel-identical through Phases 1–3** (Phase 3's goldens prove it).
- **`../mobile-eggbert` / `../simple-3d` untouched; no copied code/data.** Sibling repos
  (`../easy-3d`, `../cna`) change only via their own approved tasks.
- Each step is small and independently verified before commit.
