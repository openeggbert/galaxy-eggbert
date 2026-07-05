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
`Wall`) is the one exception to "all terrain is `UniformCube`" — see §4's updated text.
**Superseded the same day by §10: a systematic re-check found ~100 exceptions, not 1** — the
`GoldPillar`/`Saw` catches were the first two instances of a much larger, systemic gap. §4/§9.1's
"closed door = `UniformCube`" call is reversed. **This whole document's render-mode
categorization is now PARTIALLY INVALIDATED pending §10's triage** — treat §4/§5's "no other
exceptions" language as historical, not current, and §10 as the actual current state of
knowledge. Not yet independently re-verified (see §10's own methodology note) or re-approved by
the user in its expanded form.

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

**Default: `UniformCube`.** Correct for genuine bulk/structural material (dirt, rock, brick,
grass, generic wall/floor variants) — no rendering code changes needed for these.

**Status (2026-07-06): the earlier claim here — "one exception found (icon 183), no other terrain
icon warrants the same exception" — was wrong and has been superseded.** A systematic re-check (8
parallel agents, every one of the 314 named tiles individually crop-inspected) found the true
exception count is much larger — on the order of 100 icons, not 1. See **§10** for the full
findings and the revised categorization framework (a third render mode, `ThinMechanical`/decal,
in addition to `UniformCube` and `Billboard`). §4's role now is just: **UniformCube remains the
correct default for tiles NOT listed in §10** (the majority — dirt/rock/grass/generic-wall
variants are still fine as-is). `Bridge` (icon 364) is covered in §10 too (thin walkway, not
`UniformCube`) — superseding this section's earlier claim that it was merely a "construction-
sequence detail, not a render-mode question."

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

### 9.1 Doors: ~~closed door stays `UniformCube`~~ **correction (2026-07-06): closed door is also `Billboard`**

Checked `Decor::OpenDoor()` (`06-doors.md`'s `DOC-303` research): a closed door is a solid terrain
tile (`Door1`/`Door2`/`Door3`, icons 334-336); opening it sets the tile to Air and spawns
`ObjectType22`, a transient `MoveObject` that slides up using **whichever door icon spawned it**
(no fixed icon of its own — see `03-objects.md`'s row for ID 22). The door-open animation half of
this was already right: **door-open animation → `Billboard`** (§5's `ObjectType` default already
covers ID 22, dynamically textured from the closed door's icon at spawn time).

**But the "closed door → `UniformCube`" half was wrong**, per the §10 large-scale finding below:
direct crop inspection of icon 334 (`Door1`) shows a red pillar/bollard shape, not a flat door
panel or wall material — the same "rare-and-special, not bulk material" pattern as `GoldPillar`.
**Corrected: closed door → `Billboard` too**, matching mobile-eggbert's own apparent convention of
drawing interactive/special tiles (doors, teleporters, secret powers, signs) as vertical post/
pillar sprites rather than tileable wall material. See §10 for the full scope of this correction.

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

## 10. Large-scale finding (2026-07-06): `UniformCube` is wrong for ~100 tiles, not 1

**Trigger:** after seeing the first billboard-rendered `MoveObject`s in a live run, the user
noticed a "circular saw" (icon 378, `BlockTypes::Saw`) rendering as a `UniformCube` looked visibly
wrong — a spinning saw blade is a **thin, flat, circular mechanical object**, not a chunk of solid
bulk material tileable on all 6 cube faces (including the sides, which would show a full blade
face instead of a thin edge). This is a different failure mode from the earlier `GoldPillar` case
(§4's original text) — `GoldPillar` was **misidentified** (wrong name for what it showed); `Saw`
is **correctly identified** but still wrong as a render-mode assumption, because being a genuine,
common hazard tile doesn't make it bulk material.

**Method:** 8 parallel agents each visually inspected one ~40-icon slice of `02-tiles.md`'s 314
named tiles (every crop, compared against its current name/description and against the "is this
genuinely bulk material?" question), specifically watching for the `Saw` pattern in
hazard/animated/interactive tiles and for the `GoldPillar` pattern (misidentified rare icons)
elsewhere. This is a **first-pass finding, not yet independently adversarially verified** the way
the `DOC-1xx`/`DOC-3xx` documentation passes were — treat specific icon identities below as
probable, not certain, until spot-checked further; the scale and pattern of the finding itself
(this is a large, systemic gap, not 1-2 isolated misses) is solid.

### 10.1 Revised render-mode framework — a third mode is needed

§2's `renderMode` enum needs a third value: `ThinMechanical` (name provisional) — a thin,
flat/circular/mechanical hazard or interactive element that is common enough (and tied precisely
enough to its grid position, for gameplay reasons) that pure `Billboard` framing feels
under-specified, but `UniformCube` is definitely wrong. Concretely this probably renders as a thin
horizontal or vertical plane/decal at the block's position (not a full 6-face cube), similar in
spirit to `Billboard` but not necessarily camera-facing (a saw blade lies flat in a fixed
orientation; a switch panel is wall-mounted, not free-facing). **The exact geometry for
`ThinMechanical` is not decided here** — this section documents *what needs it*, not *how it's
built*; that's follow-up design work.

### 10.2 Confirmed vertical post/pillar/marker family → `Billboard`

**Pattern:** mobile-eggbert draws many interactive/special tiles as a **vertical post/pillar/
pedestal** sprite (matching `GoldPillar`'s shape) rather than tileable wall/floor material — this
turns out to be a recurring house style for "special" tiles, not a one-off. All of these are
recommended `Billboard`:

- **`Door1`/`Door2`/`Door3` (334-336)** — **correction to §9.1**: confirmed by direct crop
  inspection to be a red pillar/bollard, not a door panel. Reverses the earlier "closed door stays
  `UniformCube`" call.
- **`Teleport1..4` (330-333)** — cone/pyramid "beacon" post with colored indicator dots + an
  emblem letter per pair. The design doc's own table already called these "solid pillar" without
  drawing the render-mode conclusion.
- **`Sp0`/`Sp1` (158-159) and `Sp2`-`Sp7` (160-165)** — gold pedestal + distinct icon per value;
  almost certainly mobile-eggbert's 8-value `SecretPower` enum. All rare (1/78 each).
- **Two numbered-marker families**: icons **174-181** (red disc + digit 1-8 on a post, usage
  12,12,12,12,9,4,2,1) and icons **421-437** (gold ball + digit 1-15,17 on a post, usage up to
  15/78 for "2") — likely level-order or counter markers of some kind; not yet identified
  precisely, but unambiguously post-mounted, not bulk.
- **`Marker` (309)** — small gold trophy/cup on a pedestal, 1/78.
- **`Crusher` base (317)** — teal segmented piston shaft on a small base, 3/78 — thin mechanical
  rod, not a solid block (matches the `Saw` pattern directly, on a different hazard type).
- Icon **182** — plain post/pillar; `02-tiles.md`'s own text already notes 182 is "the real door
  tile" per `06-doors.md`'s `SearchDoor`, but the catalog row itself was never reconciled with
  that identity.
- Icons **76** (stone pedestal/column, 22/78 — common), **77** (yellow "Y" signpost/antenna,
  11/78), **191** (thin red post, 6/78), **399** (fluted classical column, 5/78), **404** (green
  vase/bulb on a neck, 3/78), **410** (small green knob/dome, mostly cropped, 5/78).
- Decorative prop clusters: **218-222** (colored balls/marbles, candy-striped poles), **230/231**
  (star badges), **233/234** (mushroom/tree silhouettes), **235/236** (same candy-pole art as
  218-222 but `passable:yes`, more common at 14-15/78), **245** (arched window/doorway pair,
  3/78), **215-217** (plain spheres/orbs, 10-14/78), **304** (gold ball on a red spring/screw
  bumper, 15/78 — common), **375-377** (twisted rope/banded decorative posts, 1-5/78), **398**
  (fence rail on two posts), **411-413** (gold picture-frame-on-pedestal display stands, currently
  named `Tile411`/`412`/`413`, 1/78 each).
- Signs: **30/31** (yellow sign with painted numeral "1"/"2", 10/78 and 4/78), **48** (yellow
  warning-triangle sign, already a good name candidate `WarningSign`, 11/78), **214** (dashed
  red/yellow boundary-marker outline, 7/78).

### 10.3 Thin mechanical/hazard tiles tied to gameplay position → `ThinMechanical`

These are functionally important (hazards, switches, mechanisms) and often common — not rare
edge cases like §10.2 — but are still visually thin/flat/mechanical, not bulk material:

- **`Saw`/`SawStopped` (378/379)** — the finding that started this whole pass. Circular blade;
  379 (stopped state) inherits the same shape.
- **`Spring` (211)** — coiled spring, already flagged as a direct `Saw`-precedent parallel.
- **`Blitz` (305)** — thin jagged lightning-bolt/arc line, not bulk, 15/78 (common hazard).
- **`Switch`/`SwitchOff` (384/385)** — flat wall-mounted control box with ON/OFF lights; tiling
  this on 6 faces (including top/bottom) is nonsensical for a wall-mounted panel. 8/78.
- **`FanLeft`/`Right`/`Up`/`Down` (126/129/132/135)** — ventilator with thin propeller blades on a
  hub protruding from a wall panel, confirming the task's explicit fan/ventilator concern.
- **`Temp` base (324)** — stepped triangular/pyramid wedge; the *silhouette itself* is non-cuboid
  (distinct from, in addition to, the usual hazard-flicker concern).
- **`Bridge` (364)** — confirmed independently against `Tables::table_decor_quart`: only the top
  16px band is solid, the rest of the cell is empty — a thin walkway/log structure, not a solid
  volume. (Supersedes §4's older "construction-sequence detail, not a render-mode question" text.)
- **`Spike` base (373)** — lower confidence: crop looks like the same glossy wall-texture family
  as its neighbors, not an obviously pointed spike shape; name may be code-derived (`Decor::
  IsPiege`, French "trap") rather than a literal visual match. Flagged for a closer look, not a
  confirmed `ThinMechanical` case yet.
- Pipe/conduit systems: icon **138** (single blue pipe segment with flanged joints, 19/78 —
  common) and icons **250-260** (an 11-icon blue pipe/valve/gauge-fitting family — straight runs,
  elbows, T-junctions, valves; usage 1-9/78 per icon).
- Grates/vents: **201** (metal cross-braced grate, `passable:yes`, **36/78 — the single most
  common icon flagged in this whole pass**), **187-189** (green ventilation-grille bars, 9-10/78).
- **86-90** — gate/portcullis-with-counterweight mechanism (metal bar racks + hinged/chained
  balls), 5-18/78.
- **110/114/118/122** — thin dashed wire/spark/cable line segments, 1-8/78, exact identity unclear.
- **264-273** — sparse yellow drip/goo/liquid overlay family (10 icons, mostly-transparent
  coverage), 6-8/78 each; marked `animated:no` in the catalog but the near-identical shapes across
  10 consecutive icons suggest an undetected animation cycle.
- **199** — yellow crossed-bar A-frame/brace, `passable:yes`, 9/78 — thin scaffold/ladder-brace
  structure.
- **192** — a real painted door-with-hinges graphic, currently miscatalogued as generic
  "unnamed variant" filler; 11/78.
- **66** — vertical rung-segmented column, reads as a `Ladder`; 15/78, `passable:yes`.

### 10.4 Special surface treatment needed (neither `UniformCube` nor `Billboard`)

- **`Water1`/`Water2` (91/92/96)** — a flat liquid surface with a wavy top-edge silhouette;
  common (25-38/78). Tiling the surface-ripple texture onto all 6 cube faces (including the
  sides/bottom, which should show depth, not another ripple surface) is wrong the same way a
  swimming pool isn't a solid turquoise brick. Needs a dedicated flat/top-surface liquid treatment
  — this is a real design gap `DirectionalCube` (§2) doesn't quite cover either (a top-only
  texture with fallback color on the other 5 faces is close, but "fallback color" for water's
  sides should probably still read as water, e.g. a darker/duller tint, not an arbitrary flat
  color).
- **`Marine` base (203)** — a green seaweed/kelp frond, 18/78. Correctly named (fits the aquatic
  theme) but is a thin plant frond, not bulk — recommend foliage-style billboard/cross-plane
  treatment (like grass in many voxel games), not `UniformCube`.

### 10.5 Architectural frame kit (391-395, 397, 400) — likely an assembly, not per-tile icons

Icons 391 (twin arch/window niches), 392/393 (left/right door-jamb edge pieces), 394/395/397
(lintel + corner-post fragments), and 400 (the archway opening itself) look like fragments of one
modular door/archway sprite kit meant to compose together, not independent tileable materials
(5-7/78 each). Flagged together since they likely need a coordinated design decision (a multi-
block archway assembly) rather than individual per-icon treatment.

### 10.6 Needs identification before a render-mode decision (uncertain visual content)

These are confirmed *not* to be plain bulk fill, but their exact identity/purpose wasn't resolved
in this pass — worth a follow-up look (higher-res crop, or cross-referencing `Decor.cpp` usage)
before committing to a specific treatment:

- **1, 7** — flat bezel/panel with a waveform line + colored status dots; reads like a
  control-panel or gauge display (8-10/78).
- **61, 62, 65, 67** — orange/brown wood-toned shapes, possibly plank/beam family related to the
  `Ladder` (66) finding above (6-14/78).
- **73** — a ring of small spheres around a gold center, circular/rotating arrangement (12/78).
- **78-85** — dark metallic panels with bolt/lever/connector shapes; reads like a switch/circuit-
  board tileset (2-14/78); the grate-like background alone might be fine as `UniformCube`, but the
  raised connector details likely aren't.
- **139-143** — a wooden bookcase/cabinet furniture set (closed/open-door variants); lower-severity
  flag since it's genuine bulk furniture, but the art is strongly front-facing only — a
  front-face-only texture might suit it better than full 6-face `UniformCube` (9-10/78 each).
- **198** — white rounded arch/dome shape, possibly a tunnel or igloo-style opening (10/78).
- **200 (`Platform`)** — possible misidentification: the name implies a flat walkable tread, but
  the crop shows two thin vertical support-leg posts, no visible flat surface (27/78 — common,
  worth resolving).
- **202** — almost entirely blank/transparent except one thin horizontal line; identity unclear,
  clearly not bulk regardless (11/78).
- **246-249** — a continuous embossed bubble/foam pattern filling the tile edge-to-edge; plausibly
  genuine bulk material (unlike the sparse 264-273 drip family) but visually unusual enough to
  double-check (8/78 each).
- **386-389** — thin-looking vertical shapes (plate/flag posts, cylindrical post, disc-on-pedestal)
  but marked non-passable/solid, unlike the confirmed-post family in §10.2 — could genuinely be
  intended solid pillars rather than thin decorative posts (4-6/78).
- **401-403** — extremely faint gray branching/radiating line art, barely visible without
  upscaling; **the most heavily used unidentified icons in this whole pass (15-16/78 files each)**
  — high-priority to resolve given the usage count.

### 10.7 What this means for §7's "what would actually need building"

§7's summary ("no terrain-rendering changes needed, `UniformCube` already correct for all 441
icons") is **no longer accurate** and needs revisiting once the icons above are triaged into
firm categories. Practical next steps, roughly in priority order:
1. Resolve §10.6's identification questions (especially **401-403**, the most-used unidentified
   icons, and **200**/`Platform`, since a misnamed common structural tile affects gameplay-relevant
   assumptions elsewhere, not just rendering).
2. Decide the `ThinMechanical` render mode's actual geometry (§10.1) — this blocks a firm
   recommendation for all of §10.3's ~25 icons.
3. Decide the water/liquid surface treatment (§10.4) — affects some of the most commonly-placed
   tiles in the whole catalog (up to 38/78 files for `Water1`).
4. ~~Update `02-tiles.md` itself with corrected names/categories for everything in §10.2-§10.4~~
   **Done (2026-07-06)** — all 171 flagged icons across §10.2-§10.5 now carry an inline
   render-mode note in `02-tiles.md`'s Category column (Billboard/ThinMechanical/special-surface/
   architectural-kit/needs-identification); see that file's own "Render-mode labeling pass" status
   note. This is a documentation label, not a rename of any `BlockTypes.hpp` constant and not an
   implementation of any new render mode.
5. An independent adversarial verification pass over this section's specific icon-by-icon claims,
   matching the rigor the `DOC-3xx` behavior-spec docs got, before treating any single icon's
   identity here as final.
