# Open Questions for the 3D Mapping Design

**Status (2026-07-05): mostly resolved.** Of the 9 questions originally flagged here, 7 are now
resolved (see `15-3d-render-mapping-design.md` for the design and its §9 addendum) — only the
billboard walk-cycle mismatch (narrowed to enemies, not Blupi) and the 7 partial-support
`ObjectType`s' implementation priority remain genuinely open, both deliberately, since neither is
a rendering/research question. This reference existed to give a factual basis for answering these
questions — that work is now largely done.

- ~~How should mobile-eggbert's objects/elements (all `ObjectType`s, `03-objects.md`) be rendered
  in 3D at all?~~ **Resolved — approved 2026-07-05** — see `15-3d-render-mapping-design.md`:
  default `Billboard` for ~68 of ~70 real `ObjectType`s, with `UniformCube` for platform lifts and
  crates; terrain tiles stay `UniformCube` (already correct for all 441 icons, no facing/rotation
  actually needed by any current tile). Implementation is separate, scoped work (`NEXT.md` §8).
  Original framing below, for context:
  - **Billboard** — a flat, always-camera-facing sprite (already the plan for Blupi/objects per
    `plan.md`'s `E3D-MIG-061`–`063`).
  - **Textured cube** — apply the 2D texture to one or more cube faces (which face(s), and whether
    the same texture repeats on all 6 or only a subset, is itself undecided), with the *remaining*
    untextured faces filled with a single flat fallback color chosen to suit that specific texture
    (e.g. sampled/averaged from the texture itself, or hand-picked) rather than left blank or reusing
    an unrelated default.
  - Whichever object types end up on the "textured cube" path need **per-block metadata** to carry:
    which rendering mode applies, which fallback color to use on the untextured faces, and — for
    objects/tiles whose 2D appearance is directionally meaningful (e.g. fans blowing a specific way,
    wall-mounted textures, anything with a "facing") — a **facing/rotation value, likely 4 cardinal
    directions**, so the 3D placement can reproduce the 2D original's orientation. This is a
    generalization of (and should be resolved together with) the doors-specific billboard question
    below, and ties into `BlockMetadata`'s existing 4-bit field (`World Format.md`) as a plausible
    (but unconfirmed) place to store it.
- **Billboard walk-cycle direction mismatch, confirmed by direct observation (2026-07-03) in the
  already-shipped `GalaxyEggbertSimple3D` billboard implementation.** A properly-implemented
  billboard always rotates to face the camera (so there is no "edge-on" viewing angle — that specific
  concern is a non-issue by construction). The real problem is different: mobile-eggbert's Blupi
  sprite only has a left/right side-view walk cycle (matching its original fixed 2D side-scroller
  camera, `Direction::Left`/`Right`) — no front- or back-facing frames exist in the source assets.
  When Blupi moves toward or away from the camera in true 3D (not purely left/right on screen), the
  camera-facing billboard still shows the side-view leg-cycling walk animation, which visibly doesn't
  match the apparent movement direction (legs appear to move sideways while the character travels
  into/out of the screen) — described by the user as looking clearly wrong in actual play, not a
  theoretical concern. Two candidate mitigations, neither requiring new/invented sprite assets (which
  `CLAUDE.md` prohibits):
  - Constrain the camera rig to a proper chase/follow camera that stays closely behind/aligned with
    Blupi's movement direction (not a free orbit camera), so the worst case (moving straight toward or
    away from the camera) becomes rare in practice rather than the common case.
  - Accept this as a documented, known v1 limitation of the billboard approach, to be properly
    resolved later by an actual 3D Blupi model with a real walk animation that matches movement
    direction from any angle — already planned as future work independent of this reference.
  This is a strong point in favor of constraining the camera design early (see the billboard/cube
  question above) rather than assuming a fully free 3rd-person orbit camera is compatible with
  billboard-rendered characters using only mobile-eggbert's original 2-directional sprite content.
  **Partially addressed for Blupi himself (2026-07-05):** `GalaxyEggbertCNA`'s camera is now
  first-person (see `NEXT.md`) — the player never sees their own character's billboard at all, so
  this specific concern doesn't apply to Blupi in the current camera mode. It's still a real,
  unresolved concern for **other** billboard-rendered characters (enemies) once those exist and are
  viewed from an angle — not fully closed, just narrower than originally framed.
- ~~Should `BigDecor` become a second parallel render layer in the 3D `World`, be folded into the
  main grid, or be dropped? Its actual collision behavior in 2D was not confirmed in this pass.~~
  **Resolved — 2026-07-05** — see `15-3d-render-mapping-design.md` §9.2: confirmed by exhaustive
  source check that `BigDecor` is purely decorative in mobile-eggbert (never referenced by any
  collision function). Recommendation: render as `Billboard`s (not folded into the main solid-block
  grid, and not a second parallel `World` layer).
- ~~Should hazard/animated tiles carry their animation phase as `BlockMetadata` instead of the
  current CPU-side "rebuild the animated subset's mesh" approach?~~ **Resolved — 2026-07-05** —
  see `15-3d-render-mapping-design.md` §9.3: no change recommended. Animation phase is a shared,
  group-wide counter in mobile-eggbert (all tiles in a group advance in lockstep), not per-instance
  state, so per-block metadata would be redundant with what `GETerrainRenderer::Update()` already
  does correctly.
- ~~Should doors be a distinct `BlockMetadata`-tagged variant, or a billboard object layered over
  `Air`, matching mobile-eggbert's own door-open-is-a-`MoveObject` model?~~ **Resolved — 2026-07-05**
  — see `15-3d-render-mapping-design.md` §9.1: no new design needed. A closed door is already a
  terrain block (`UniformCube`, §4's default); the door-open animation is already `ObjectType22`,
  which already falls under §5's `Billboard` default — the two existing categories already cover
  both halves of this correctly.
- ~~Should `MoveObject` records (pickups, enemies, effects) become `World`-embedded per-block
  metadata, or stay a separate object list alongside the `World`~~ **Resolved — approved
  2026-07-05** — see `15-3d-render-mapping-design.md` §6: they stay a separate list (as
  galaxy-eggbert's own `MobileObjSpec`/`GEDecorSystem` already model them for the 2D-sourced
  Simple3D target), not embedded blocks — the block-type-id space is designed for
  static terrain, not per-instance dynamic state (position, phase, patrol range).
- ~~Teleporter pairing is implicit (scan-the-map) in mobile-eggbert — worth deciding whether to
  keep that convention or make pairing explicit via `BlockMetadata`.~~ **Resolved — 2026-07-05** —
  see `15-3d-render-mapping-design.md` §9.6: keep the implicit scan-based approach (already how
  `BlockTypes.hpp`'s `isTeleporter()` works); revisit only if a future world wants more pairs than
  the icon-based scheme comfortably supports.
- ~~The `region=` → background-image mapping is still unresolved~~ **Resolved 2026-07-04**
  (`DOC-247`/`DOC-248`, `05-backgrounds.md`) — it's a direct formula, no lookup table.
  ~~The remaining open question is a mapping-*design* one: should the eventual 3D target load real
  parallax background images at all, and if so, how does a flat 2D background become a 3D
  skybox/backdrop?~~ **Design question resolved — 2026-07-05** — see
  `15-3d-render-mapping-design.md` §9.5: recommend NOT attempting real skybox/parallax rendering
  (the source art is flat 2D, a poor fit for 3D skybox geometry — no skybox capability exists in
  CNA/`../easy-3d` today either); keep `GalaxyEggbertSimple3D`'s existing flat sky-clear-color
  approach, with a suggested follow-up to derive each region's color from its real background PNG
  instead of the current 5 hand-picked approximations.
- ~~What should the ~175 still-unresearched `ObjectType` IDs actually turn out to be?~~ **Resolved
  2026-07-04** — `03-objects.md`'s classification is complete for all 204 IDs (`DOC-003`): 133 are
  confirmed genuinely vestigial (zero references anywhere in source), 41 have real behavior but are
  never placed in a shipped level, 1 is ambiguous, 29 are real and used. The remaining open question
  is about the 7 partial-support types below, not about unresearched ones.
- The 7 partial-support `ObjectType`s (jeep/secret-exit/skateboard/suction-cup/mirror/balloon/
  dynamite — `03-objects.md`) spawn but have no gameplay effect — whether/when to implement their
  real pickup behavior is a separate decision from the 3D mapping question, but affects how much of
  their behavior needs documenting here first. **Still genuinely open (2026-07-05) — deliberately
  not resolved as part of the 3D-mapping design work**, since it's a gameplay-implementation
  priority question, not a rendering or research question — the behavior itself is already fully
  documented in `13-object-pickups.md`/`04-enemy-behavior.md` regardless of when/whether it gets
  implemented. Revisit when scoping actual gameplay work for the CNA target.
