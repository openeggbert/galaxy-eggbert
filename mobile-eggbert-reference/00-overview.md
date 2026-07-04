# mobile-eggbert 2D World Reference — Overview

## Purpose and scope

This reference catalogs everything a mobile-eggbert (2D *Speedy Blupi*) world/level can contain —
the `.txt` level file format, tile/block types, moving objects (pickups, enemies, effects),
backgrounds, doors, sounds, and animations — as the factual, source-grounded basis for a future
deliberate mapping design onto galaxy-eggbert's richer 3D `World`/`.vwr` format (see
`World Format.md`).

**This is a catalog, not a design document.** It does not decide how any 2D concept should be
represented in 3D (e.g. whether doors become billboards, whether `BigDecor` becomes a second
render layer or per-block metadata) — those are open questions listed in `09-open-questions.md`,
to be resolved in a separate mapping-design task. All facts below were read directly from
`../mobile-eggbert` (read-only reference) and cross-checked against galaxy-eggbert's own
already-approved partial ports (`include/GalaxyEggbert/BlockTypes.hpp`,
`include/GalaxyEggbert/def/*.hpp`, `src/GalaxyEggbertSimple3D/Game/GEWorldRuntime.cpp`,
`src/GalaxyEggbertCNA/Game/GEWorldRuntime.cpp`).

## Status: complete for cataloging; one real engine bug (`DOC-007`) still open

The first pass (2026-07-03) covered a representative sample; the user flagged this as insufficient
and the goal became **complete** coverage of every tile icon, every `ObjectType`, every animation,
every sound channel, and every background — built up incrementally, one small task at a time.
Tracked as `DOC-001`-`DOC-006` in `plan.md` §15, then `DOC-100`-`DOC-267` in §16 after a second
audit (2026-07-03/04) found the GIFs themselves had a real ghosting bug and several sprite crops had
a real 1px-grid-offset bug. **All of `DOC-001`-`DOC-006` are now done** — see each file's own
status note below for specifics. Each file notes its own completeness status at the top; this
section only summarizes.

**What's still open:** `DOC-007` — a real, unfixed engine bug in `GalaxyEggbertSimple3D`'s
`GEDecorSystem.cpp`: it hardcodes `element.png` for every `ObjectType`, but real level data shows
types `1`/`12` need `object-m.png` and types `32`/`33` (`blupih`/`blupit`) need `blupi1.png` (type
`47`'s equivalent bug was fixed separately as `S3D-4`, Chenille — see `plan.md` §13 — but `1`/`12`/
`32`/`33` are not). This is a code fix, not a documentation task, and is out of scope for this
reference tree.

**Two real engine bugs were found and fixed during the `DOC-100`-`DOC-267` audit** (not just
documentation corrections — see `plan.md` §13 "Simple3D Migration Milestones" for full detail):
`S3D-2` (`BlockTypes::tileUV()` assumed a flat 64px grid in `object-m.png`; the real sheet has a 1px
gap *and* a 1px leading margin, confirmed by the sheet's exact pixel dimensions) and `S3D-4`
(`ObjectType47`/Chenille's real sprite sheet is `object-m.png`, not `element.png`, and its icon
values were out of bounds for `element.png`). `S3D-2`'s fix meant every crop sourced from
`object-m.png` across this whole reference tree needed regenerating with the corrected formula —
tracked file-by-file in `plan.md` §16.3 (`DOC-230`-`DOC-234`).

## Files in this reference

| File | Covers | Status |
|---|---|---|
| `00-overview.md` | This file — scope, index, status | — |
| `01-world-file-format.md` | The `.txt` level file format (header, `Decor:`, `BigDecor:`, `MoveObject:`) | Believed complete — all sections of the format have been identified |
| `02-tiles.md` | Every tile/block icon in `object-m.png` | **Complete** — all 441 addressable icons accounted for (`DOC-002`); all 313 full crops regenerated with the corrected grid formula after the `S3D-2` bug was found (`DOC-230`) |
| `03-objects.md` | Every `ObjectType` (204 IDs) | **Complete for classification** (`DOC-003`) — 29 real+used, 41 real+unused-in-shipped-levels, 1 ambiguous, 133 vestigial; all 67 cropped icons re-verified, 7 regenerated for the `S3D-2` grid bug (`DOC-231`) |
| `04-enemy-behavior.md` | The enemy/object movement & collision model | Architectural overview only, not per-type exhaustive |
| `05-backgrounds.md` | Background/sky region images | **Complete** — `region=` → filename mapping found and verified against all 78 levels, all 28 level backgrounds thumbnailed, all 10 non-level UI-screen backgrounds documented (`DOC-006`/`DOC-247`-`DOC-257`) |
| `06-doors.md` | Door tile behavior (2D) | Believed complete for the 2D behavior itself; all 3 door icon crops regenerated for the `S3D-2` grid bug (`DOC-234`) |
| `07-sounds.md` | The 93 sound channels | **Complete** — all 93 channels documented with their real in-game trigger, cross-checked against `SoundChannel.hpp`, all 93 `.wav` files verified accounted for (`DOC-005`/`DOC-235`-`DOC-246`) |
| `08-animations.md` | Every animated sequence (tiles, Blupi, objects, explosions, doors) | **Complete** — 155 sequences documented (12 tiles, all 87 real `BlupiAction` states, 23 objects, 8 explosions, 1 door slide); all 129 animated GIFs regenerated after a ghosting/translucency bug in the GIF-assembly tooling was found and fixed (`DOC-004`/`DOC-100`-`DOC-229`) |
| `09-open-questions.md` | Open questions for the eventual 3D mapping design | Living list, updated as facts are found |

## How images were generated

See `02-tiles.md`'s and `08-animations.md`'s own "How these images were generated" notes — the
grid math and tooling are documented next to where they're used, not centralized here, since
different files use different sprite sheets. All images live in `images/` (relative to this
folder), generated from `../mobile-eggbert/Content/` (read-only source, never modified).
