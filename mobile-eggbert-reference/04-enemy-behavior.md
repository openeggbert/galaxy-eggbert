# Enemy/Object Behavior Model

**Status:** architectural overview only — describes the general movement/collision *system*, not a
per-type exhaustive behavior catalog (that level of per-type detail, where it exists, is in
`03-objects.md`, complete for classification under `DOC-003`).

**`DOC-305` (2026-07-05):** the user gave explicit, scoped approval to add prose behavioral specs
of gameplay logic (previously off-limits per the faithful-remake/no-transcription rule). `## Per-type
enemy behavior` below extends this file with real per-`ObjectType` patrol/attack/contact/death
mechanics for spider (16), fish (17), the vestigial "variant" (18), bird (20), blupih (32), blupit
(33), wasp (44), large creature (54), and the generic 2/3 patrol hazards, plus a verification pass
on the existing follower (96/97) section below. `03-objects.md` remains the source of truth for
classification/icons; this file is now the source of truth for per-type *behavior*.

From `Decor.cpp`'s own architectural documentation (top-of-file comment, paraphrased): most
enemies/effects are **table-driven** — a `MoveObject` moves linearly between `posStart` and
`posEnd` at `stepAdvance`/`stepRecede` speed with `timeStopStart`/`timeStopEnd` dwell timers
(`MoveObjectStepLine`), while its visual animation/lifetime is a phase counter indexed into a
`Tables.cpp` animation array (`MoveObjectStepIcon`). A **minority** of object types are
**hand-coded** with bespoke per-type logic in `MoveObjectStepIcon` and its helpers — named examples
in the source comment: dynamite, "charging" enemies, followers (`ObjectType96`/`97`, which track
Blupi's position directly rather than patrolling a fixed line), cloud-nets, and crates.

Patrol-style enemies with `posStart == posEnd` in the raw file data (a common authoring pattern —
confirmed by galaxy-eggbert's own Simple3D loader, which detects this case and synthesizes a small
patrol range: `spec.posStart.x_ -= 2.0f; spec.posEnd.x_ += 2.0f`) patrol back and forth over a
short fixed range around their placed position rather than staying stationary.

Collision for both Blupi and objects is tile-based (not swept): an axis-aligned rectangle is
tested against the tile cells it overlaps via `IsBlocIcon()`/`IsPassIcon()`; a single blocking cell
makes the whole rectangle "occupied". This is the source-of-truth collision model mobile-eggbert
uses — distinct from, and not necessarily reusable for, galaxy-eggbert's own 3D grid-collision code
(e.g. `GEBlupiController` in the CNA target implements its own simplified 3D grid collision with
step-up traversal, not a transcription of this 2D system).

## The follower pattern (`ObjectType96`/`97`), as implemented 2026-07-03

mobile-eggbert's real follower behavior (`Decor::MoveObjectFollow`) promotes a dormant `96` to an
awake `97` once Blupi is near, then homes toward Blupi at roughly 1 px/tick.
`GEDecorSystem`'s port (Simple3D) approximates this with a distance-triggered wake (≈2.5 world
units) followed by continuous speed-based motion toward Blupi's live position every frame, using an
idle animation (`table_follow1`) while dormant and a chase animation (`table_follow2`) while awake
— a reasonable approximation of the real tick-stepped homing, not a byte-exact port.

## Per-type enemy behavior (`DOC-305`, 2026-07-05)

Sourced from `Decor::MoveObjectStepIcon` (`Decor.cpp:8192`–`8970` for the types below) for
per-type animation/attack dispatch, plus the shared helpers `Decor::MoveObjectDetect`
(`Decor.cpp:9689`–9771, Blupi-contact hitboxes), `Decor::MockeryDetect` (`Decor.cpp:9518`–9602,
idle taunt triggers), `Decor::MoveObjectFollow` (`Decor.cpp:9646`–9678), `Decor::DynamiteStart`
(`Decor.cpp:9058`–9175, blast destruction) and `Decor::BlupiElectro`/`MovePersoDetect`
(`Decor.cpp:9610`–9638, `9835`–9863). Patrol range/speed (`posStart`/`posEnd`,
`stepAdvance`/`stepRecede`, `timeStopStart`/`timeStopEnd`) is level-authored per placed instance,
read straight from the world file's `MoveObject` record (`Decor.cpp:11269`-11271) — not a
per-type constant. What *is* hardcoded per type, and documented below, is the animation table,
attack behavior, contact hitbox, and death/destruction handling.

**Shared patrol-turn mechanic.** Every type below (except spider) cycles the same 4-phase `step`
driven by `MoveObjectStepLine` (doc comment `Decor.cpp:7989`-8004): step 1 = dwell at `posStart`
for `timeStopStart` frames, step 2 = walk to `posEnd` over `stepAdvance` frames, step 3 = dwell at
`posEnd` for `timeStopEnd` frames, step 4 = walk back over `stepRecede` frames. Steps 1/3 (the
dwells) play a "turn" animation table, steps 2/4 play the straight walk-cycle table; which literal
table (`_left`/`_right` vs `_turn2l`/`_turn2r`) is picked is mirrored by whether
`posStart.X > posEnd.X`, so a level author can flip patrol direction just by swapping the two
points.

### ObjectType2 / ObjectType3 — generic patrol hazards
Icon-only cycling with **no turn table** — type2: `icon = 12 + phase/2 % 9` (`Decor.cpp:8202`-8206),
type3: `icon = 48 + phase/2 % 9` (`Decor.cpp:8207`-8211) — neither actually visually turns, unlike
the named animals below. Both are in the shared kill list: contact with either (while Blupi is
vulnerable) triggers `BlupiDead(Clear1, Clear2)` and destroys the hazard (ObjectType8 explosion,
`Decor.cpp:5782`-5816). **Type3 has a duck-immunity quirk**: `MoveObjectDetect` remaps its contact
box to the tile's top half only, and skips it entirely while Blupi's action is `Down` (ducking)
(`Decor.cpp:9720`-9728) — type2 uses the plain lower-half default box. Source comments
(`Decor.cpp:9515`, `9686`) call type2 specifically a "thrown object": it gets a wider secondary
"anticipation" box in `MoveObjectDetect` so code can react before contact (`Decor.cpp:9763`-9767),
`MockeryDetect` suppresses the idle taunt when it approaches head-on (`Decor.cpp:9583`-9598), and a
separate check plays an `Ouf4` flinch reaction whenever a type2 hazard is near-but-not-touching
(`Decor.cpp:5638`-5645) — flagging as uncertain, but this suggests ObjectType2 may be a
thrown/rolling hazard rather than a walking creature despite the "patrol enemy" catalog label. Both
are destroyed by dynamite blast (`Decor.cpp:9103`-9104).

### ObjectType16 — spider
Animation: a single continuous 9-frame crawl loop (`icon = 69 + phase/1 % 9`, `Decor.cpp:8212`-8216)
with no left/right/turn tables at all — unlike every named enemy below, its sprite doesn't change
with movement direction. Contact: in the shared kill list (`Decor.cpp:5782`) — kills Blupi via
`BlupiDead(Clear1, Clear2)` (gated by jeep/tank/skate immunity and focus/air/jumpAie, same as 2/3),
always destroyed itself. Uses the default lower-half `MoveObjectDetect` hitbox (no special-casing)
and is in the taunt-capable set (`Decor.cpp:9559`-9562). Destroyed by dynamite blast
(`Decor.cpp:9111`).

### ObjectType17 — fish
Animation: 48-frame turn (`table_poisson_turn2l/r`) at step 1/3, 8-frame straight swim at step 2/4
(`Decor.cpp:8670`-8711). Contact: in the shared kill list, but its destruction explosion is
upgraded to ObjectType10 (bigger "tertiary" explosion) with `BigShake`, instead of the small
explosion/shake used by 2/3/16/20 (`Decor.cpp:5797`-5806). Hitbox: `MoveObjectDetect` narrows its
contact band to the sprite's vertical middle (Y+16..Y+44) instead of the default lower half
(`Decor.cpp:9744`-9749). **Not** in the taunt-capable set (excluded at `Decor.cpp:9559`-9562) —
fish never trigger Blupi's idle mockery, unlike spider/bird/wasp/creature. Destroyed by dynamite
blast (`Decor.cpp:9112`).

### ObjectType18 — patrol variant (vestigial)
No `MoveObjectStepIcon` case exists for this type (no icon/animation), and it is never placed in
any of the 78 shipped world files (`03-objects.md`). Its **only** reference anywhere in `Decor.cpp`
is a single membership check inside `DynamiteStart`'s blast-destruction list (`Decor.cpp:9113`) —
if an instance existed, a dynamite blast would delete it, but nothing spawns one, and it has no
contact-kill, taunt, or detection special-casing. Independently re-verified 2026-07-05: confirmed by
direct grep, `Decor.cpp:9113` is the sole reference to `ObjectType18` anywhere in `Decor.cpp`/
`Tables.cpp`, and no shipped world file contains `type=18`. Flagging as fully unobservable in
practice; documented only because it was explicitly in scope to check.

**Needs reconciliation with `03-objects.md` (not fixed here — flagging only):** `03-objects.md`
files `18` under Category B ("real behavior confirmed, but never placed") and its own top summary
table groups `18` ("variant") alongside spider/fish/bird/blupih/blupit/wasp/creature under "Patrol
walker enemies" — implying comparable real patrol/animation logic. Per this section, `18` has none
of that (no icon, no animation, no patrol/attack logic at all — just one incidental
blast-destruction membership check), which sits uneasily with "real behavior confirmed" and
especially with being grouped as a "patrol walker" next to types that actually walk, turn, and
attack. `03-objects.md`'s own Category B entry for `18` (no icon, no `MoveObjectStepIcon` case) is
consistent with this file; it's the categorization label and the summary-table grouping that
overstate it.

### ObjectType20 — bird
Animation: 10-frame turn (`table_oiseau_turn2l/r`), 8-frame straight flight, same direction-mirror
as fish (`Decor.cpp:8712`-8753). Contact/hitbox: identical treatment to fish (17) — shared kill
list, same ObjectType10/`BigShake` destruction, same narrowed mid-band hitbox (`Decor.cpp:9744`-9749).
Unlike fish, bird **is** in the taunt-capable set (`Decor.cpp:9560`). Destroyed by dynamite blast
(`Decor.cpp:9115`).

### ObjectType32 — blupih (stationary shooter, drops projectile downward)
Animation: 26-frame turn (`table_blupih_turn2l/r`), 8-frame straight walk (`Decor.cpp:8838`-8877).
Attack: during each turn-dwell (step 1 or 3), at dwell-frame 21 exactly, it spawns one
`ObjectType23` projectile at `(posCurrent.X, posCurrent.Y + 40)` via
`ObjectStart(pos, ObjectType23, 55)` (`Decor.cpp:8878`-8886). Decoding `ObjectStart`'s speed
encoding (`Decor.cpp:7805`-7869): a magnitude over 50 always means straight down (the `55` here is
`50 + 5`, with `5` used only to scale travel time) — so blupih drops its shot straight down rather
than aiming at Blupi; it plays `SoundChannel52` only if the drop found a floor to land on. Contact:
blupih's own body is **not** in the Blupi-kill list (2/3/96/97/16/4/17/20) or the wasp/creature
blocks — walking into its body is not a documented damage path; only its projectile
(`ObjectType23`) harms Blupi, always fatal via the `Glu` (glue-death) action
(`Decor.cpp:5914`-5947) unless shielded/hidden/superblupi. Alongside bulldozer (4) and blupit (33),
it mutually destroys itself and any `ObjectType200`-`203` "Perso" object it walks into
(`Decor.cpp:7957`-7975, `MovePersoDetect` at `9835`) — **correction 2026-07-05**: the original
draft called these "rescuable NPCs"; that label is unverified and likely wrong. `ObjectType.hpp`'s
own comments describe `200` as a costume-select pickup and `201`-`203` as doppelganger hazards that
kill Blupi on contact (`BlupiDead`, `Decor.cpp:6088`-6109), not rescue targets — and per
`03-objects.md`'s Category B, `200`-`203` are never placed as a `MoveObject` in any of the 78
shipped world files, so this mutual-destruction path (like `ObjectType18`, above) is real code that
never actually fires in a shipped level. It is one of only three enemy types (4/32/33)
vulnerable to the Cloud power-up's lightning (`BlupiElectro`, `Decor.cpp:7976`-7987) — spider,
fish, bird, wasp, and the creature are not. Destroyed by dynamite blast (`Decor.cpp:9121`).

### ObjectType33 — blupit (stationary shooter, fires two horizontal shots)
Animation: 24-frame turn (`table_blupit_turn2l/r`), 8-frame straight walk (`Decor.cpp:8888`-8927).
Attack: fires **two** `ObjectType23` projectiles per turn-dwell, both horizontal (magnitude `±5`
per the same `ObjectStart` speed encoding, unlike blupih's downward drop): one at dwell-frame 3,
aimed toward the side it's about to walk, spawned 30px to that side (`Decor.cpp:8928`-8948), a
second at dwell-frame 21 aimed the opposite way (`Decor.cpp:8949`-8969) — the two shots bracket the
turn, one before and one after, in opposite directions. Contact/vehicle-crush/electro/dynamite:
same treatment as blupih — one of the 3 Cloud-vulnerable and 3 mutually-self-destructing-with-Perso
types (`Decor.cpp:7957`, `7976`; see the `ObjectType200`-`203` correction under blupih above —
same never-placed-in-a-shipped-level caveat applies here), destroyed by dynamite blast
(`Decor.cpp:9122`); body contact is likewise not a documented Blupi-kill path, only the shared
projectile block is.

### ObjectType44 — wasp/bee
Animation: 5-frame turn (`table_guepe_turn2l/r`), 6-frame straight flight (`Decor.cpp:8754`-8794).
Contact: **does not kill Blupi.** It transforms him instead: `ByeByeHelico()` strips any current
vehicle, then `m_blupiBalloon = true` is set (other movement-mode flags cleared, 100-tick shield
timer), with `SoundChannel40` and an ObjectType90 puff (`Decor.cpp:5826`-5866). This is the
"balloon" status effect — Blupi floats rather than dying; later touching a type 3/16/96/97 hazard
while ballooned pops the balloon into a falling/"Air" state instead of killing him outright
(`Decor.cpp:5766`-5781). The wasp itself is not destroyed by this contact (no `ObjectDelete` in the
branch). Taunt-capable (`Decor.cpp:9560`); not Cloud-vulnerable; destroyed by dynamite blast
(`Decor.cpp:9125`).

### ObjectType54 — large creature
Animation: 8-frame straight walk each direction, but its turn animation (`table_creature_turn2`,
152 frames) is **the same table for both step 1 and step 3 regardless of patrol direction**
(`Decor.cpp:8796`-8836) — unlike every directional type above, it has no turn2l/turn2r pair.
Contact: only registers while the creature is in a turn-dwell (`step != 2 && step != 4`, i.e. NOT
mid-walk) and Blupi is vulnerable (`Decor.cpp:5867`-5913). If Blupi is currently riding any vehicle
(helico/over/balloon/ecrase/jeep/tank/skate), that vehicle is destroyed (ObjectType10 explosion,
`SmallShake` — corrected 2026-07-05, verification found this was miswritten as `BigShake`; the
actual call at `Decor.cpp:5903` is `m_decorAction = DecorAction::SmallShake`, unlike fish/bird's
real `BigShake` upgrade above — vehicle flags cleared); otherwise it just plays `SoundChannel51`. Either way Blupi's
action becomes `Glu` (glue-style death). This **confirms** the `03-objects.md` catalog note
("destroys helicopter on contact") but clarifies the actual mechanic: the creature grabs Blupi
(fatal unless the grab only consumes a current vehicle), and this only happens while it's paused
mid-turn, not while walking. The creature itself is never destroyed by this contact (no
`ObjectDelete` on it in this branch) — unlike spider/fish/bird, touching it can't kill it. Taunt:
unconditionally returns mockery icon `83` regardless of facing (`Decor.cpp:9575`-9578), unlike
every other type's facing-dependent `63`/`64`. Not Cloud-vulnerable; destroyed by dynamite blast
(`Decor.cpp:9128`).

### ObjectType96 / ObjectType97 — follower (re-verified, no correction needed)
Re-checked against `MoveObjectFollow` (`Decor.cpp:9646`-9678) and the type-97 branch of
`MoveObjectStepLine` (`Decor.cpp:8025`-8064): the "as implemented 2026-07-03" section above remains
accurate. Precise numbers not previously recorded here: the wake box is the follower's tile padded
±100px on all sides, tested against Blupi's inset box (`Decor.cpp:9658`-9671); once awake it steps
exactly 1px/frame toward Blupi's current position (`Decor.cpp:8029`-8044) and self-destructs into
an ObjectType9 explosion if `TestPath` finds its next step blocked (`Decor.cpp:8049`-8064), rather
than continuing to patrol. Contact/kill/taunt/dynamite handling for both 96 and 97 is otherwise
identical to spider (shared kill list, default hitbox, taunt-capable, dynamite-destructible).
