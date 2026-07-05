# Crates, Lifts, Bridges & Effects — Gameplay Behavior

**Status:** `DOC-306` (2026-07-05) — prose behavioral spec of crate-push mechanics, platform lifts,
bridge construction, dynamite, and two visual-effect systems (helicopter destruction, death
"voyage"), under the explicit scoped approval recorded in `CLAUDE.md` (2026-07-05). Line numbers
are approximate (`~`) against the `Decor.cpp` revision current as of this pass.

## Crate push (`ObjectType12`, "Object" pushable crates)

- **Trigger** (`Decor.cpp` ~6130-6157): while Blupi is walking (`BlupiAction::March`, not in any
  vehicle/mode — helicopter, "over", balloon, "ecrase"/squash, jeep, tank, skate, swim, surf, or
  suspended-on-a-bar) and his leading edge overlaps a crate object, he switches into
  `BlupiAction::Push`, plays a grunt sound (`SoundChannel38`) and an "oof" reaction animation, and
  every subsequent tick attempts `Decor::TestPushCaisse` against the crate one step further in his
  facing direction. A second, distance-based variant exists: landing from a fall while moving
  horizontally into a crate (rather than walking into it) enters a separate `BlupiAction::Pop`
  state (`Decor.cpp` ~3300-3320) that drives the same push machinery with a "pop" flag set (see
  "Linked crates" below) and a shorter push distance; if the pop-push is obstructed, Blupi's action
  reverts to `Stop` instead of merely clamping his position the way a blocked walking-push does.
- **Speed** (`Decor::CaisseGetMove(int max)`, ~9487): the base per-tick pixel distance is 5 for a
  walking push and 3 for the pop variant (the two call sites at `Decor.cpp` ~3388/3446 vs
  ~3410/3468). That base is reduced by `(linkedCrateCount - 1) / 2` — a heavier linked stack moves
  slower — clamped to a minimum of 1px so a push always makes some progress; the "Power" bonus
  doubles the result; and for the first `ScaleTime(20)` ticks of a push the speed ramps up linearly
  from near zero, so a push visibly starts gently rather than snapping to full speed.
- **What stops a push** (`Decor::TestPushOneCaisse`, ~9354): a candidate destination is rejected
  outright if the shifted crate box collides with solid decor geometry (`DecorDetect` with
  `bCaisse=false` — other crates are deliberately ignored here, since a linked stack is validated as
  a group, not pairwise against itself). **Floor-support is checked only for the crate at the same
  row as the one Blupi is directly pushing** (the seed of the push): its two lower corners (20px
  wide edge strips just below its bottom edge) must both be solid, so a pushed ground-row crate
  cannot be shoved out over a gap. Crates that are part of the same linked group but sit **above**
  that row (i.e. stacked on top) skip the floor-support test entirely and only need the general
  non-collision check — mobile-eggbert does not re-verify support under every level of a stack, only
  under the row being actively pushed.
- **Linked crates** (`Decor::SearchLinkCaisse`, ~9397, and `Decor::AddLinkCaisse`, ~9442): pushing a
  crate is a two-pass, all-or-nothing group operation. `SearchLinkCaisse` flood-fills outward from
  the pushed crate, adding any other crate whose 1px-inflated box touches an already-linked crate,
  repeating until a full pass adds nothing — this naturally picks up a whole physically-touching
  stack, not just the one crate Blupi touched. Only crates at or above the seed's row are considered
  (so you push a stack, not the floor it rests on). `TestPushCaisse` (~9321) then tests every member
  of that linked set against the proposed move and only commits the shift to all of them if every
  member is clear — a stack always moves atomically, never partially. The `bPop` flag (used by the
  landing/pop push variant) additionally restricts linking to crates within &plusmn;32px
  horizontally of the seed, so a falling/popping crate doesn't drag in a wide unrelated row.
  `Decor::UpdateCaisse` (~9302) simply rebuilds the flat list of every live `ObjectType12` instance
  each frame (`m_rankCaisse`), which `SearchLinkCaisse`/`CaisseInFront` scan. `Decor::CaisseInFront`
  (~9456) finds the crate immediately in front of Blupi (probing a point 32px beyond his leading
  edge) to know which crate a push/pop should target.
- **Stacked-crate support — confirms a real, if simplified, mechanic**: `TestPushOneCaisse`'s
  design shows mobile-eggbert genuinely supports multi-crate stacks and pushes them together
  (`SearchLinkCaisse` links crates vertically as well as horizontally), but the floor-support gap
  check is intentionally shallow — it only applies to the pushed row, not to whatever a stacked
  crate rests on above that row. `NEXT.md` notes galaxy-eggbert's own Simple3D port currently only
  tests floor-support at y=0 and leaves stacked crates (y=1) untested; that gap is narrower than it
  might sound, since even the reference implementation does not re-check support for crates above
  the pushed row — but it has not verified that stacked crates *link and move together* at all,
  which mobile-eggbert's `SearchLinkCaisse` does do. This is the concrete behavior to port/verify
  next if stacked-crate parity is prioritized.

## Platform lifts (Chenille / Ascenseur — `ObjectType1`/`47`/`48`)

- **Boarding** (`Decor::AscenseurDetect`, ~9184): used when Blupi is falling/landing to test whether
  he is coming down onto a lift rather than the ground. It returns `-1` immediately during a short
  post-slide-off cooldown (`m_blupiTimeNoAsc != 0`). It tests only the thin top strip (16px) of each
  lift object (type 1, 47, or 48) against Blupi's box; to avoid tunnelling through a fast-moving
  lift during a high-speed fall, the query box is swept in 30px vertical increments between his old
  and new position and each intermediate step is tested, so a fast descent still lands on the lift
  instead of passing through it. On a hit, the caller sets `m_blupiTransport` to that lift's index.
- **Continuous ride / position sync** (`Decor::MoveObjectStepLine`, ~8005-8174): every tick, for
  each live lift object, the function independently tests whether Blupi's feet strip (a 1px band
  just above his feet — `Y+58` to `Y+59` — inset 20px from each side) overlaps the lift's top strip. If so, it computes
  `m_blupiVector` as the lift's own displacement this tick (`posCurrent` before/after its own
  line-movement step) for X, but snaps Y by direct subtraction against Blupi's exact expected foot
  position (`posCurrent.Y - (blupiPos.Y + 60 - BLUPIFLOOR)`) rather than a pure delta — so vertical
  drift is corrected every frame instead of accumulating. Types 47/48 (the animated "Chenille"
  conveyor variants) add a constant &plusmn;2px/tick horizontal nudge on top of the platform's own
  motion (47 pushes right, 48 pushes left), matching their scrolling conveyor-track texture. This
  overlap re-test, not `AscenseurDetect`, is what keeps Blupi glued to a lift he is already standing
  on; `AscenseurDetect` only matters for the initial catch while airborne.
- **Vertigo** (`Decor::AscenseurVertigo`, ~9238): called every tick while `m_blupiTransport != -1`
  to report whether Blupi's box hangs off the left or right edge of his current lift (compared
  against the lift's 64px width), driving a teetering/edge animation. For "shiftable" lifts (see
  below), detecting vertigo additionally **flips** which side is reported and starts a short
  `m_blupiTimeNoAsc = 10`-tick no-lift cooldown — i.e. rather than let Blupi balance on the very
  edge indefinitely, the wide lifts are designed to slide him off.
- **Shiftable lifts** (`Decor::AscenseurShift`, ~9271): a lift is "shiftable" simply if its current
  icon is one of the wide horizontal-platform frames (311-316, from `table_chenille`); narrow lift
  icons are not shiftable and let Blupi stand anywhere on them without the edge-flip/cooldown
  behavior above.
- **Movement pattern**: lifts are ordinary `MoveObject`s and use the same generic linear
  posStart&harr;posEnd oscillation (`stepAdvance`/`stepRecede`, dwell timers) documented
  architecturally in `04-enemy-behavior.md` — nothing bespoke beyond the Blupi-carrying logic above.
- **`Decor::AscenseurSynchro`** (~9291): despite taking an index parameter, the loop inside
  immediately overwrites it and rewinds **every** object in the move-object pool to its start state
  (`posCurrent = posStart`, `step = 1`, `time = 0`, `phase = 0`) — in practice a whole-level "reset
  all lift/animation timers to a common origin" call, not a per-lift operation; the parameter name is
  misleading.

## Dynamite (`ObjectType55` pickup / `ObjectType56` fuse / `Decor::DynamiteStart`)

- **Placement** (`Decor.cpp` ~4792-4812): pressing the action button while carrying at least one
  dynamite (`m_blupiDynamite > 0`), not in any vehicle mode, and not currently lift-transported,
  requires solid ground under **both** feet (checked via the same edge-probe pattern used
  elsewhere) before it will arm; if grounded, it spawns an `ObjectType56` fuse object at Blupi's feet
  and switches him into `BlupiAction::PutDynamite`.
- **Fuse timing** (`Decor.cpp` ~8252-8296): the fuse object runs a 100-tick animation
  (`Tables::table_dynamitef`) but is scripted to fire, not a single explosion, but **nine** separate
  `Decor::DynamiteStart` blast calls at fixed ticks — 50 (the center blast, offset `(0,0)`), then 53,
  55, 56, 59, 62, 64, 67, and 69, each with its own hand-authored `(dx, dy)` pixel offset (ranging up
  to roughly &plusmn;200px horizontally and &plusmn;100px vertically) — an asymmetric scattered
  pattern, not a uniform radius. The fuse object self-destructs at tick 70, well before its own
  100-tick animation table would otherwise loop.
- **Per-blast effect** (`Decor::DynamiteStart(int i, int dx, int dy)`, ~9058): each blast spawns an
  explosion sprite (`ObjectType8`) at its offset position; only the center blast (`dx==dy==0`) also
  plays the boom sound (`SoundChannel10`) and triggers a screen shake (`DecorAction::SmallShake`).
  Each blast then, within its own 128&times;128px (2&times;2-tile) area: clears destructible hazard
  tiles by icon (saws 378/379, drip hazards 404/410, via `ModifDecor`); destroys every enemy, crate,
  or object whose box overlaps that area, from an explicit type list (thrown objects, followers 96/97,
  most patrol enemies, several vehicle pickups, the large creature 54, bridge-construction objects
  52, and more) — crates (`ObjectType12`) are destroyed as a full linked group via
  `SearchLinkCaisse` so an entire touched stack detonates together, each destroyed crate spawning a
  debris fragment via `ByeByeAdd` (see below); and finally kills Blupi (`BlupiAction::Clear1`) if he
  is inside the blast rect and not shielded, hidden, or in "super Blupi" mode.

## Bridge construction (`ObjectType52`)

- **Trigger** (`Decor::IsBridge`, ~7334, called from `Decor.cpp` ~5608-5612): while Blupi has focus,
  each tick checks whether his standing cell (at two candidate foot-height offsets, to cover both
  possible stance heights) is icon `364` (`Bridge`, per `02-tiles.md`). If so, an `ObjectType52`
  move-object is spawned at that cell, kicking off the 157-frame construction animation
  (`table_bridge`, `object-m.png`, noted in `03-objects.md`).
- **During the animation** (`Decor.cpp` ~8540-8562): each tick's `table_bridge` icon is written both
  to the move-object's own sprite **and directly into the terrain `Decor` grid cell** at the spawn
  position — so the ground tile itself visibly cycles through the sequence rather than the effect
  being purely an overlay sprite. The table itself: 28 ticks building up through icons 365-372
  (alternating/advancing), then exactly 112 ticks holding icon `-1` (mobile-eggbert's "empty/no
  tile" sentinel), then a mirrored 17-tick descent back down through 372..365 to a final icon of
  **364 again** — the same icon the cell started with. A mid-sequence sound (`SoundChannel73`) fires
  at tick 137, inside the `-1` hold window, purely as a scripted cue unrelated to the icon at that
  moment. The object self-deletes once its phase reaches 157.
- **Does it become passable terrain? Re-verified — the "purely cosmetic" framing below was wrong
  and has been corrected.** Checking `Tables::table_decor_quart` directly (`Tables.cpp` ~469, a
  4&times;4-per-tile solid/non-solid bitmap indexed `[icon*16 + subcell]` that `Decor::DecorDetect`
  — the function `BlupiStep`'s actual ground-contact check calls, not `IsPassIcon`/`02-tiles.md`'s
  classification — reads live every tick) shows icon `364` is **not** fully passable: its top
  16px quart-row is solid (`[1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0]`), which is exactly what gives Blupi
  footing to stand on it. Icons `365`/`366` (the first two build-up frames) share that same
  solid-top profile, but icons `367`-`372` are **fully non-solid** (all 16 sub-cells zero), and the
  `-1` sentinel is likewise always non-solid. Because `table_bridge` overwrites the live terrain
  cell every tick (confirmed above) and `DecorDetect` always reads that live icon, the cell's real
  floor support genuinely toggles across the sequence: solid for ticks 0-15 (365/366) and 152-156
  (366/365/364), but **fully unsupported for ticks 16-151 — 136 of the 157 ticks** — covering the
  367-372 ramp, the entire 112-tick `-1` hold, and the mirrored ramp back down. `IsPassIcon(364)`
  itself actually returns `false` (its 16-cell scan hits the solid top row immediately), the
  opposite of the "already classified as passable" claim this section used to make — and
  galaxy-eggbert's own `BlockTypes.hpp::kPassable[364]` independently agrees (`false`, while
  `kPassable[367..372]` are `true`), corroborating this reading of the mobile-eggbert table.
  (`02-tiles.md` labeling 364 "passable in 2D" looks like a separate pre-existing error in that
  file — out of scope to fix here.) **Net effect: the construction animation is not a no-op
  overlay — it removes and restores real ground collision at that exact cell for the bulk of its
  duration.** Whether this is ever visible in play as an actual fall-through was not fully traced
  here (during continuous walking Blupi likely clears the cell within the ~16-tick solid window
  before it goes hollow), but do not port this as "cosmetic only" — the ground-truth collision data
  says otherwise for most of the sequence.

## Helicopter destruction effect (`Decor::ByeByeHelico`/`ByeByeAdd`/`ByeByeStep`/`ByeByeDraw`)

- **Trigger**: `Decor::BlupiDead` (~6547) calls `ByeByeHelico()` unconditionally at the start of
  *every* death, for *any* cause — it is a no-op unless `m_blupiHelico` is true (Blupi was actually
  flying the helicopter at the time), in which case one debris fragment (`PixmapChannel::Element`
  icon 68 — a different icon namespace than the `object-m.png`/`Decor`-grid icon 68 that is `Lava`'s
  base icon per `02-tiles.md`) is spawned at Blupi's position. **Resolved:** cropping
  `element.png` directly (600&times;1740px, 60px cells, no gap, 10 columns per
  `Pixmap::GetSrcRectangle`'s `column = icon % (width/gridX)` formula &rarr; icon 68 = column 8,
  row 6 = pixel rect (480,360)-(540,420)) shows a small teal helicopter fuselage/chassis graphic,
  sitting in the sprite sheet between a bulldozer turn-animation run (icons 60-67) and a red
  spiky burst icon (69) — clearly a helicopter debris chunk, consistent with `ByeByeHelico`'s use
  as a helicopter-destruction fragment and unrelated to `object-m.png` icon 68 (`Lava`). Contact
  with the large creature (`ObjectType54`, per `03-objects.md`/`04-enemy-behavior.md`) calls
  `ByeByeHelico()` directly and separately (`Decor.cpp` ~5867-5884) as part of grabbing Blupi into a
  "Glu"/stuck state (spawning a tentacle effect, `ObjectType53`) — this does not itself kill Blupi,
  but it does destroy the helicopter if he was in one at the moment of contact, matching the "large
  creature destroys helicopter on contact" note.
- **Effect** (`Decor::ByeByeAdd`, ~10056, and `Decor::ByeByeStep`, ~10087): each call adds one
  independent debris fragment to a small pool (separate from the main `MoveObject` array), given a
  random horizontal launch speed (10-19px/tick — `Next(0,10)+10` with a .NET-style exclusive upper
  bound, not 10-20 — random left/right) and caller-chosen rotation/
  animation speeds. Each fragment then follows a ballistic pop-up-then-drop arc — its vertical
  position rises for the first ~10 phase-ticks (accelerating with `(10-phase)^1.5`) then falls
  the same way beyond that — while horizontal speed decays linearly toward zero and its rotation
  angle accumulates continuously, giving a spinning "blown outward and dropping" look. Fragments are
  culled once their phase exceeds 30 ticks. The same `ByeByeAdd`/`ByeByeStep`/`ByeByeDraw` pool is
  reused for other debris — e.g. destroyed crates in a dynamite blast (`Decor.cpp` ~9155) and other
  explosion-adjacent fragments — not exclusively for the helicopter.

## Death "voyage" effect (trigger/behavior only — see `08-animations.md` for frame data)

`Decor::VoyageInit`/`VoyageStep` (~10158/~10237) drive a generic "fly an icon from A to B, then
apply a reward/effect" arc reused for all collectibles (life eggs, treasure, keys, dynamite,
follower, persona) as well as certain death outcomes. `Decor::BlupiDead` itself (~6547-6614)
directly queues this arc for two of its "clear" variants once the death action is chosen: `Clear2`
(triggered by falling out of the level's bottom bound, `Decor.cpp` ~2754-2756) starts a voyage with
icon 230 from Blupi's position to a point 300 world-units straight above it — the "angel ascent"
that then cycles icons 230-241 while in flight, per `07-sounds.md` channel 74; `Clear3` (lava
contact) instead starts a voyage with icon 40 to a point a full 2000 world-units above Blupi — a
much longer, non-cycling arc (fixed 50-tick duration override, no frame animation). Both play
`SoundChannel74` at the moment the arc is queued. `Clear4` (saw/`Scie` death) does not use the
voyage system at all — it spawns three `ObjectType41` burst sprites and plays `SoundChannel75`
instead. `Decor::VoyageGetPosVie` (~10141) is a small helper computing the fixed HUD slot position
for the Nth remaining life-egg icon (`x = 210 + 16*n, y = 417`), used when a life-loss voyage arc
needs a HUD destination point. The animation frame content itself (which Blupi sprite
`Clear1`-`Clear8` actions correspond to which cause) is documented in `08-animations.md` and is not
repeated here.
