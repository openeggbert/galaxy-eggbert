# Open Questions for the 3D Mapping Design

These are flagged, not answered, here — this whole reference exists to give a factual basis for
answering them later, in a separate mapping-design task.

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
