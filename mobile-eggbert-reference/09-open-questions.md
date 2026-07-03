# Open Questions for the 3D Mapping Design

These are flagged, not answered, here — this whole reference exists to give a factual basis for
answering them later, in a separate mapping-design task.

- **How should mobile-eggbert's objects/elements (all `ObjectType`s, `03-objects.md`) be rendered in
  3D at all?** Raised by the user 2026-07-03, not decided here. Two candidate approaches, not
  mutually exclusive across different object types:
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
- Should `BigDecor` become a second parallel render layer in the 3D `World` (e.g. a background
  chunk offset behind the main terrain), or be folded into the main grid, or be dropped? Its actual
  collision behavior in 2D (does Blupi ever collide with `BigDecor` cells, or is it purely visual?)
  was not confirmed in this pass and should be checked before deciding.
- Should hazard/animated tiles (lava, saws, etc.) carry their animation phase as `BlockMetadata`
  instead of being handled by the current CPU-side "rebuild the animated subset's mesh" approach
  (`GETerrainRenderer::Update()`)? The 4-bit metadata field is a plausible fit for a handful of
  animation-phase states, but 12-bit type IDs already fully separate animated groups by base icon.
- Should doors be a distinct `BlockMetadata`-tagged variant of a normal block, or something else
  entirely (a billboard object type layered over an `Air` cell, matching how mobile-eggbert's own
  door-open animation is itself a `MoveObject`, not a tile mutation with an attached animation)?
- Should `MoveObject` records (pickups, enemies, effects) become `World`-embedded per-block
  metadata, or stay a separate object list alongside the `World` (as galaxy-eggbert's own
  `MobileObjSpec`/`GEDecorSystem` already model them for the 2D-sourced Simple3D target)? The
  latter already works and doesn't obviously need the block-metadata system at all.
- Teleporter pairing is implicit (scan-the-map) in mobile-eggbert — worth deciding whether to keep
  that convention or make pairing explicit via `BlockMetadata` now that the format supports it.
- The `region=` → background-image mapping is still unresolved (`05-backgrounds.md`) — needed
  before any real background/skybox work, 2D or 3D.
- What should the ~175 still-unresearched `ObjectType` IDs (`03-objects.md`) actually turn out to
  be? Some may be genuinely vestigial/unused; the mapping design shouldn't have to account for
  types that never appear in any real level.
- The 8 partial-support `ObjectType`s (jeep/secret-exit/skateboard/suction-cup/mirror/balloon/
  dynamite — `03-objects.md`) spawn but have no gameplay effect — whether/when to implement their
  real pickup behavior is a separate decision from the 3D mapping question, but affects how much of
  their behavior needs documenting here first.
