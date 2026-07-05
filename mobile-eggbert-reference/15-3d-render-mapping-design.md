# 3D Render Mapping Design — Block Types → Render Treatment

**Status: APPROVED (2026-07-05), not yet implemented.** User confirmed the §6 recommendation
(`MoveObject`s stay a separate list, not embedded `World` blocks) explicitly; the rest of the
proposal (§3-§5 render-mode categorization) stands as written. This resolves the render-mode
question raised in `09-open-questions.md`'s first bullet, grounded in what `GalaxyEggbertCNA`'s
renderer actually supports today and in the full tile/object catalogs (`02-tiles.md`,
`03-objects.md`) plus the gameplay-behavior spec (`10`-`14-*.md`). Approved as a design — actual
implementation (billboard renderer, etc.) is separate, scoped work, tracked in `NEXT.md` §8.

## 1. What the renderer actually supports today (confirmed by reading source)

- `include/GalaxyEggbert/BlockTypes.hpp`'s block-type-id space (12 bits, `.vwr` format) currently
  equals mobile-eggbert's **tile icon space only** (0–440, via `fromMobileIconId()`). `ObjectType`
  pickups/enemies are a wholly separate `MoveObject` list, never embedded as World blocks, in
  either target.
- `Easy3D::CubeItem`/`CubeMesh` (`../easy-3d`) currently support **exactly one UV region per cube,
  applied to all 6 faces** (confirmed directly in `CubeMesh.hpp`'s doc comment: "one texture region
  per cube, not one per face"). There is no per-face texture-or-color capability today, and no
  facing/rotation concept anywhere in the CNA rendering path.
- Every currently-used tile already renders as a uniform-textured cube via `GETerrainRenderer` —
  this is real, working, and correct for the vast majority of tiles (see §3).

## 2. Proposed per-block-type render descriptor

A new per-block-**type** (not per-instance) lookup, indexed by block-type-id, describing how that
type renders:

```text
renderMode: UniformCube | DirectionalCube | Billboard
fallbackColor: RGB   (only meaningful for DirectionalCube — untextured faces use this)
```

- **`UniformCube`** — today's behavior: the type's one icon texture tiles across all 6 faces.
  Cheapest, and correct wherever the source art has no inherent "facing."
- **`DirectionalCube`** — the icon texture is applied to one face (or a specific subset — e.g.
  top+sides vs. bottom), chosen by the block **instance's** facing metadata (§3 below); all other
  faces get `fallbackColor` (a flat color, sampled/averaged from the icon texture at build time —
  not left blank, not an unrelated default, matching `09-open-questions.md`'s original framing).
- **`Billboard`** — no cube at all: a flat, always-camera-facing sprite, matching the already-planned
  Blupi/object approach (`E3D-MIG-061..063`, `Easy3D::BillboardBatch`, still not implemented).

## 3. Per-instance metadata: facing/rotation

The `.vwr` format's existing 4-bit per-block metadata field (`World Format.md`) is used as:

```text
bits 0-1: facing (4 values — cardinal direction in the X/Z ground plane: +X/-X/+Z/-Z)
bits 2-3: reserved (unused for now — candidate for a future animation-variant or state flag)
```

**Finding: no currently-used tile actually needs this.** Checked every tile mobile-eggbert's own
2D art might imply a "facing" for (fans, doors, switches, springs) — in every case, mobile-eggbert
already encodes the direction as a **separate icon ID**, not as a rotation of one icon:
- Fans: `FanLeft`(126)/`FanRight`(129)/`FanUp`(132)/`FanDown`(135) are 4 distinct icons, already
  distinct block types. No rotation metadata needed — the block-type-id itself already picks the
  right art and, by extension, the right blow direction.
- Doors: `Door1`/`Door2`/`Door3` (334/335/336) differ by **key color**, not facing — mobile-eggbert
  levels only ever show doors from one fixed 2D angle anyway.
- Switches, springs, teleporters: all single-icon, non-directional in the source art.

**Recommendation: reserve the facing bits (cheap, forward-looking) but don't build `DirectionalCube`
support yet.** Every real, currently-cataloged tile (all 441 icons in `02-tiles.md`) is correctly
served by `UniformCube` alone. `DirectionalCube` becomes useful only for **new, hand-authored 3D
content** that isn't sourced from a mobile-eggbert icon (e.g. a future wall-mounted decoration with
a real "front"), which is speculative future work, not a blocker for faithful-remake parity. This
is a meaningful simplification: the `Easy3D::CubeItem`/`CubeMesh` per-face extension this would
require (an `../easy-3d` change, needing its own separate approval per `CLAUDE.md`) does not need
to happen for mobile-eggbert parity — only if/when denser hand-authored worlds want directional
decoration.

## 4. Terrain tiles (block-type-id space): categorization

**All 441 tile icons → `UniformCube`.** No exceptions found. This matches current behavior
exactly — no rendering code changes needed for terrain. (`Bridge`, icon 364, keeps its existing
special-cased animated/collision handling from `GETerrainRenderer` — that's a construction-sequence
detail, not a render-mode question; see `14-crates-lifts-bridges-effects.md`.)

## 5. Objects/enemies/pickups (`ObjectType`, not currently in the block-type-id space)

**Default: `Billboard` for everything** — matches the already-planned architecture
(`E3D-MIG-061..063`) and the fact that every pickup/enemy/effect sprite is a single flat 2D frame
with no inherent 3D structure. Applies to all ~70 Category A+B real-behavior `ObjectType`s: every
pickup (`13-object-pickups.md`), every enemy (`04-enemy-behavior.md`), dynamite, bridge-construction
visuals, helicopter-destruction/death-voyage effects (`14-crates-lifts-bridges-effects.md`).

**Two exceptions — recommend `UniformCube` instead of `Billboard`:**
- **Platform lifts** (`ObjectType 1`/`47`/`48`, "Chenille" track lifts) — Blupi physically stands
  and rides on top of these; a flat billboard would look wrong for something load-bearing. Render
  as a solid cube with the existing track texture (`ObjectType47`'s crop is already sourced from
  `object-m.png`, not `element.png` — see `03-objects.md`'s `S3D-4` fix).
- **Crates** (`ObjectType 1`/`12`, the pushable crate mechanic — note `1` is shared with platform
  lifts above; `Decor.cpp`'s dispatch distinguishes them by context, not by a different
  `ObjectType`) — a pushable box is naturally a cube in every direction; a billboard would look
  wrong when pushed sideways or viewed from behind. `UniformCube` (same crate texture on all 6
  faces) is the simplest correct treatment.

Everything else stays `Billboard`.

## 6. Resolves an existing open question: objects stay a separate list, not embedded blocks

`09-open-questions.md`'s last bullet asked whether `MoveObject` records (pickups/enemies/effects)
should become `World`-embedded per-block metadata, or stay a separate object list alongside
`World`. **Recommendation: keep them separate** (as today in `GalaxyEggbertSimple3D`/
`GEDecorSystem`), for three reasons:
1. The block-type-id space + 4-bit metadata is designed for **static** terrain description;
   `MoveObject`s need dynamic per-instance state (position, phase, patrol range, active/inactive)
   that doesn't fit a voxel grid model without real awkwardness.
2. The separate-list model already works today and needs no new capability to keep working.
3. Simpler mental model going forward: **world blocks are terrain; `MoveObject`s are everything
   that moves, reacts, or gets picked up** — objects get their own small per-`ObjectType` render
   descriptor (§5's table), looked up by `ObjectType`, not by block-type-id.

## 7. Summary — what would actually need building

If this proposal is approved:
- No terrain-rendering changes needed (`UniformCube` already correct for all 441 icons).
- `Easy3D::BillboardBatch`'s vertex builder + CNA renderer adapter (already-planned
  `E3D-MIG-061..063`) becomes the rendering path for ~68 of ~70 real `ObjectType`s.
- A new, small `Easy3D::CubeBatch`-based renderer path (reusing the existing terrain cube
  machinery, not a new one) for platform lifts + crates (2 exceptions).
- `DirectionalCube` / per-face-texture support is **not needed** for faithful-remake parity —
  defer it until/unless new hand-authored 3D content actually wants it.
- The facing/rotation metadata bits can be reserved in the format now (cheap) without being used
  by anything yet.

## 8. Open items this proposal does NOT resolve

- Doors-as-billboard-vs-something-else (still open, see `09-open-questions.md`) — doors are
  currently a terrain block type (`Door1/2/3`), not an `ObjectType`, so they fall under §4's
  `UniformCube` default; whether that's the right long-term treatment for something that
  animates/opens is a separate question from this doc's scope.
- `BigDecor` (second tile layer) — still unresolved, unaffected by this proposal.
- Hazard/animated-tile metadata-vs-CPU-rebuild question — still open, unaffected.
