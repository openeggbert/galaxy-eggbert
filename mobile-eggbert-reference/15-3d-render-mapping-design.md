# 3D Render Mapping Design — Block Types → Render Treatment

**Status: APPROVED (2026-07-05), not yet implemented.** User confirmed the §6 recommendation
(`MoveObject`s stay a separate list, not embedded `World` blocks) explicitly; the rest of the
proposal (§3-§5 render-mode categorization) stands as written. This resolves the render-mode
question raised in `09-open-questions.md`'s first bullet, grounded in what `GalaxyEggbertCNA`'s
renderer actually supports today and in the full tile/object catalogs (`02-tiles.md`,
`03-objects.md`) plus the gameplay-behavior spec (`10`-`14-*.md`). Approved as a design — actual
implementation (billboard renderer, etc.) is separate, scoped work, tracked in `NEXT.md` §8.
**§9 addendum (same day):** resolves 5 more `09-open-questions.md` items (doors, `BigDecor`,
hazard-animation metadata, background/skybox rendering, teleporter pairing) — same approval
status, not yet implemented.
**§4 correction (2026-07-06, user-caught):** icon 183 (renamed `GoldPillar`, was misclassified as
`Wall`) is the one exception to "all terrain is `UniformCube`" — see §4's updated text. This is a
factual correction to the original §4 claim, not a new open decision.

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

**Default: `UniformCube`.** Matches current behavior for essentially all 441 tile icons — no
rendering code changes needed for ordinary terrain. (`Bridge`, icon 364, keeps its existing
special-cased animated/collision handling from `GETerrainRenderer` — that's a construction-sequence
detail, not a render-mode question; see `14-crates-lifts-bridges-effects.md`.)

**One exception found (2026-07-06, user-caught):** icon 183 (`BlockTypes::GoldPillar`, renamed
from the wrong `Wall` name/description — see `02-tiles.md`'s corrected row) is visually a golden
pillar/post, not brick-wall texture, confirmed by direct crop inspection. It's rare (1/78 files,
only 12 cells, forming a two-column gate/portal-frame shape immediately after icon 182 — the real
door tile, per `06-doors.md`'s `SearchDoor`) and carries a separate special meaning in
`Decor::AdaptDoors`'s hub/world-select-screen logic (marks an uncollected world's gold). This is
the same "rare and special, not mass-repeated structural material" pattern that puts objects on
the `Billboard` path in §5, not the `UniformCube` path — **recommend `Billboard` for icon 183**,
overriding the terrain default for this one icon. No other terrain icon was found to warrant the
same exception (all others are either genuinely common structural/hazard tiles or unnamed/unused).

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

## 8. Open items this proposal did NOT originally resolve

- ~~Doors-as-billboard-vs-something-else~~ — resolved in §9 below (2026-07-05).
- ~~`BigDecor` (second tile layer)~~ — resolved in §9 below (2026-07-05).
- ~~Hazard/animated-tile metadata-vs-CPU-rebuild question~~ — resolved in §9 below (2026-07-05).

## 9. Addendum (2026-07-05): doors, `BigDecor`, hazard-animation metadata, teleporter pairing

Resolving five more `09-open-questions.md` items with concrete research + recommendations.

### 9.1 Doors: closed door stays `UniformCube`; door-open animation is already `Billboard`

Checked `Decor::OpenDoor()` (`06-doors.md`'s `DOC-303` research): a closed door is a solid terrain
tile (`Door1`/`Door2`/`Door3`, icons 334-336); opening it sets the tile to Air and spawns
`ObjectType22`, a transient `MoveObject` that slides up using **whichever door icon spawned it**
(no fixed icon of its own — see `03-objects.md`'s row for ID 22). This already matches §4/§5's
categories exactly, with no new design needed:
- **Closed door → `UniformCube`** (§4's terrain default, unchanged).
- **Door-open animation → `Billboard`** (§5's `ObjectType` default, already covers ID 22) — the
  billboard's texture is chosen dynamically at spawn time from the door tile it replaces, which is
  an implementation detail (look up the closed door's icon before removing it), not a new render
  mode or metadata field.

No `BlockMetadata` tagging is needed for doors beyond what §2/§3 already define.

### 9.2 `BigDecor`: confirmed non-colliding — recommend `Billboard`, not folded into the main grid

**New fact, confirmed by direct source check:** `m_bigDecor` is referenced in exactly two places in
`Decor.cpp` — the draw pass (`Build()`, ~line 716) and save/load (`Read`/`Write`, ~lines 11088-11339).
It is **never** referenced by any collision function (`DecorDetect`, `IsBlocIcon`, `IsPassIcon`,
`TestPath`, or any of the `Is*` hazard checks) — confirmed by an exhaustive grep across the whole
file. `BigDecor` is purely decorative in mobile-eggbert; Blupi never collides with it. This resolves
the open question's stated prerequisite ("was not confirmed in this pass").

The draw code also shows `BigDecor` can itself be animated (icon `203` remaps through
`Tables::table_marine`, same as a main-grid Marine tile) and applies a small per-icon Y offset
(e.g. `-13` for icons 66-68) — it's a real background layer, not a trivial afterthought.

**Recommendation: render `BigDecor` cells as `Billboard`s (§5's category), not folded into the main
solid-block grid, and not a second parallel `World` layer.** Reasoning:
- It's confirmed non-colliding — putting it in the main grid as a normal solid block would be
  wrong (it would incorrectly block movement).
- A true second parallel voxel layer (a whole extra `World` alongside the main one) is a lot of
  new plumbing for content that's small in practice (`01-world-file-format.md`: single digits to
  low tens of cells per level) and never interacted with.
- `Billboard`s already have no collision in this design (§5) and already support per-frame texture
  swaps (needed for the animated case above) — reusing that path is the smallest change that's
  still faithful to "purely decorative, sometimes animated."

### 9.3 Hazard/animated-tile phase: keep the current `BlockMetadata`-free CPU-rebuild approach

Animation phase in mobile-eggbert is a **shared, group-wide counter** — all `Lava` tiles (for
example) advance through their 8-frame loop in lockstep, driven by one global tick, not
independent per-instance state (confirmed by `Tables.cpp`'s single shared frame-index formula per
group, already used in `GETerrainRenderer`/`GEWorldRuntime::GetAnimPhase()`). Storing phase in each
block's 4-bit metadata would duplicate the same value across every instance of a group for no
benefit. **Recommendation: no change** — keep the existing "rebuild the animated subset's mesh on
global phase change" approach (`GETerrainRenderer::Update()`), which is already implemented,
tested (NEXT.md: ~48 rebuild cycles over 8s, no crash), and correctly matches how mobile-eggbert
itself models tile animation.

### 9.5 Backgrounds: keep flat sky-color, don't attempt real skybox/parallax rendering

Checked: neither CNA nor `../easy-3d` has any skybox/skydome capability today (confirmed by
grepping both include trees for "Skybox"/"SkyDome"/"SkyGradient" — zero matches). Building one from
scratch is possible but the source material is a poor fit: mobile-eggbert's 28 region backgrounds
(`05-backgrounds.md`) are flat 2D parallax images (240×180 px), designed to scroll behind a fixed
side-on 2D camera — not a 360° panorama or cubemap. Projecting one onto real 3D skybox geometry
would only look correct from one angle and would visibly distort or seam elsewhere.

**Recommendation: keep the current approach — a flat sky clear-color per region — rather than
attempting real background-image skybox rendering.** `GalaxyEggbertSimple3D` already does this
(`GalaxyEggbertSimpleGame.cpp`, 5 hardcoded `Color` values indexed by world number, under a
`TODO(S3D-sky)` marker). A reasonable improvement (not a new capability, just better sourcing):
derive each region's clear-color by sampling/averaging its actual background PNG instead of using
5 hand-picked colors for all ~28 regions — still a flat color, just faithfully sourced from the
real per-region art instead of approximated for a handful of regions. This is a small, low-risk
follow-up, not a blocker.

### 9.6 Teleporter pairing: keep implicit scan-based pairing

Mobile-eggbert pairs teleporters by scanning the map for a matching icon at use-time
(`Decor::SearchTeleporte`, per `12-hazards-and-interactables.md`), not by an explicit stored pair
ID. `BlockTypes.hpp` already mirrors this with 4 named icons (`Teleport1..4`) and a scan-based
`isTeleporter()` helper. **Recommendation: keep the implicit scan approach** — it already works,
needs no new metadata, and matches the source faithfully. Revisit only if a future hand-authored
3D world wants more simultaneous teleporter pairs than the icon-based scheme comfortably supports
(not a current need).
