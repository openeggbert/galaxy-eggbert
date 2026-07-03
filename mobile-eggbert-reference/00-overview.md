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

## Status: partially complete, in progress

The first pass (2026-07-03) covered a **representative sample**, not the full space — 29 of
roughly 441 addressable tile icons, 18 of 204 `ObjectType`s, 31 of an unknown-but-larger total
animation count. The user flagged this as insufficient: the goal is **complete** coverage of every
tile icon, every `ObjectType`, and every animation mobile-eggbert has, built up incrementally.
Tracked as `DOC-001`..`DOC-006` in `plan.md` §15 ("Documentation — mobile-eggbert 2D reference,
complete"). Each file below notes its own completeness status at the top.

**Progress:** `DOC-001` (file split), `DOC-002` (complete tile catalog, all 441 icons), and
`DOC-003` (complete `ObjectType` classification, all 204 IDs) done 2026-07-03. `DOC-003` also
surfaced a real rendering bug (wrong sprite-sheet channel assumed for 5 `ObjectType`s in
`GEDecorSystem.cpp`) — see `03-objects.md`'s bug callout. `DOC-004`..`DOC-006` (animations, sounds,
backgrounds) still pending.

## Files in this reference

| File | Covers | Status |
|---|---|---|
| `00-overview.md` | This file — scope, index, status | — |
| `01-world-file-format.md` | The `.txt` level file format (header, `Decor:`, `BigDecor:`, `MoveObject:`) | Believed complete — all sections of the format have been identified |
| `02-tiles.md` | Every tile/block icon in `object-m.png` | **Complete** — all 441 addressable icons accounted for (`DOC-002`, done 2026-07-03) |
| `03-objects.md` | Every `ObjectType` (204 IDs) | **Complete for classification** (`DOC-003`, done 2026-07-03) — 29 real+used, 41 real+unused-in-shipped-levels, 1 ambiguous, 133 vestigial; 30 of 204 have a cropped icon |
| `04-enemy-behavior.md` | The enemy/object movement & collision model | Architectural overview only, not per-type exhaustive |
| `05-backgrounds.md` | Background/sky region images | Partial — 3 of 38 backgrounds shown; `region=` mapping unresolved (`DOC-006`) |
| `06-doors.md` | Door tile behavior (2D) | Believed complete for the 2D behavior itself |
| `07-sounds.md` | The 93 sound channels | Not yet expanded — pointer only (`DOC-005`) |
| `08-animations.md` | Every animated sequence (tiles, Blupi, objects, explosions, doors) | **Partial — 31 animations documented, explosions and 12 newer ObjectTypes not yet covered** (`DOC-004`) |
| `09-open-questions.md` | Open questions for the eventual 3D mapping design | Living list, updated as facts are found |

## How images were generated

See `02-tiles.md`'s and `08-animations.md`'s own "How these images were generated" notes — the
grid math and tooling are documented next to where they're used, not centralized here, since
different files use different sprite sheets. All images live in `images/` (relative to this
folder), generated from `../mobile-eggbert/Content/` (read-only source, never modified).
