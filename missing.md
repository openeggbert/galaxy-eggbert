# missing.md — known reuse misses

Tracks cases where `GalaxyEggbertCNA` re-derived a problem that engine-independent code in this
repository had already solved. Kept separate from `NEXT.md` as a quick-scan list of this recurring
pattern. The retired pre-CNA implementation mentioned by the original incident remains available
at git commit `4afd53e`.

## CNA tile-atlas UV bleeding (seam-line artifact) — fixed 2026-07-09

**What was missing:** `BlockTypes::tileUV()` (`include/GalaxyEggbert/BlockTypes.hpp`) already
applied a documented
half-texel UV inset to avoid bilinear-filtering bleed at atlas tile edges. When
`GalaxyEggbertCNA`'s `GETileAtlas` was written, it independently reimplemented tile UV lookup via
a plain `Easy3D::TextureAtlas` grid registration — correct on tile *pitch* (no cross-tile pixel
drift, since it did account for `kSheetGap`), but it never applied the additional half-texel
inset. `GETileAtlas.cpp`'s own comment even cited `tileUV()`'s doc comment as the source of the
pitch-gap fact, without noticing the inset it also applies.

**Symptom:** thin blue (sky-clear-color) or dark seam lines along block edges at oblique/close
camera angles, reported by the user 2026-07-08 via a live screenshot of the `RockPile` staircase.

**Fix:** `GETileAtlas::GetTileUv()` now calls `BlockTypes::tileUV()` directly instead of its own
`Easy3D::TextureAtlas`-based lookup — reusing the already-proven math rather than re-deriving it.
See `NEXT.md` §3 (2026-07-09 entry) for the full root-cause writeup and measured before/after
pixel-level results (60% reduction in fully-transparent seam pixels, not fully eliminated).

**Why it matters as a pattern:** `GalaxyEggbertCNA` re-derived logic despite an existing,
engine-independent implementation. `BlockTypes.hpp` was already callable from CNA with zero
porting effort. Whenever a rendering bug has a plausible 2D/pixel-math root cause, search
`include/GalaxyEggbert/`, the mobile reference notes, and current verification tests before
re-deriving a fix from scratch.
