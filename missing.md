# missing.md — known ports/fixes not carried over between targets

Tracks cases where `GalaxyEggbertSimple3D` (historical reference, not built/maintained as of
2026-07-08) already solved a problem that `GalaxyEggbertCNA` (the active long-term target)
re-encountered independently, because the fix was never ported over. Kept separate from
`NEXT.md` (which already documents each individual fix in detail) as a quick-scan list of this
specific *class* of bug, in case it recurs.

## CNA tile-atlas UV bleeding (seam-line artifact) — fixed 2026-07-09

**What was missing:** `GalaxyEggbertSimple3D`'s `GETerrainRenderer.cpp` already used
`BlockTypes::tileUV()` (`include/GalaxyEggbert/BlockTypes.hpp`), which applies a documented
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

**Why it matters as a pattern:** `GalaxyEggbertCNA` was largely written by re-deriving logic from
mobile-eggbert/Simple3D behavior rather than literally reusing existing galaxy-eggbert code where
it already existed and was already correct. `BlockTypes.hpp` is engine-agnostic and shared by both
targets already — `tileUV()` was sitting right there, callable from CNA with zero porting effort,
and simply wasn't. Worth a quick check whenever a CNA rendering bug turns out to have a plausible
2D/pixel-math root cause: search whether `GalaxyEggbertSimple3D` or the engine-agnostic
`include/GalaxyEggbert/` tree already solved the same problem before re-deriving a fix from
scratch.
