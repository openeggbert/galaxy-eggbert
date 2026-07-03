# Animations

**Status: COMPLETE for known animated sequences** — 71 animated sequences documented (12 tiles, 5
Blupi states, 14 original objects, 8 explosions, 1 door slide, 9 new-object-type animations across
10 GIFs — `ObjectType96` has two states, dormant and awake). A full sweep of every `table_*` array
in mobile-eggbert's `Tables.cpp` (§4 below) confirms nothing else animates that isn't either already
covered here or explicitly listed as deferred/out-of-scope with a reason. Tracked as `DOC-004` in
`plan.md`.

Every animated sequence mobile-eggbert's tile/character/object system can produce, as looping
GIFs assembled (with ImageMagick) from per-sheet grid crops — this is meant as a full overview of
what the animation system does, to inform what galaxy-eggbert eventually needs to replicate. Frame
data is taken from galaxy-eggbert's own already-approved, verified-against-source ports
(`GETerrainRenderer.cpp`'s `kAnim*` tables, `GEBlupiController.cpp`'s state tables,
`GEDecorSystem::GetObjIcon`'s per-type tables), not re-derived here. All GIFs loop (last frame
connects back to the first).

**Correction (2026-07-03, found while completing this pass): the "6 fps" tick rate claimed below
for object/explosion animations is not confirmed and is likely wrong.** Tile animations
(`GEWorldRuntime::Update()`) and Blupi's state animations (`GEBlupiController::AdvanceAnim()`) both
use an explicit, throttled timer (6 fps and 8 fps respectively — these ARE confirmed, real, in the
code). Object animations (`GEDecorSystem::Update()`) have **no such throttle** —
`st.animPhase = (st.animPhase + 1) % 10000;` runs unconditionally every call, and
`GEDecorSystem::Update()` is called once per game frame with no rate cap found anywhere in
`GalaxyEggbertSimple3D`. This means object-animation phase advances at the *display frame rate*
(commonly ~60 fps, but not a guaranteed constant), not a fixed 6 fps — the per-type "duration/frame"
and "loop length" values in §3 below (inherited from the original animation pass, not re-verified in
this one) are likely off by roughly a factor of 10 (e.g. a `p/6` divisor at 60 fps is 100 ms/frame,
not the ~1.0 s/frame the existing table claims). **Not corrected in this pass** — re-deriving the
real frame rate needs either instrumenting a real run or finding an explicit rate constant this
search didn't locate; flagging it here rather than leaving the wrong assumption undocumented, per
`feedback_ask_before_scoping_completeness`-style discipline (don't silently carry forward an
unverified number as if confirmed). New animation entries added in this pass (§2–§4) use the same
"N ticks, assumed frame rate" phrasing with the same caveat, not a confirmed duration.

## 1. Animated tiles (`object-m.png`, 6 fps base tick ≈ 167 ms/frame — confirmed, throttled timer)

| Animation | Frames | Duration/frame | Loop length | GIF |
|---|---|---|---|---|
| Lava | 8 | 167 ms | 1.33 s | ![Lava anim](images/tile-anim-lava.gif) |
| Spike | 16 | 167 ms | 2.67 s | ![Spike anim](images/tile-anim-spike.gif) |
| Crusher | 10 | 167 ms | 1.67 s | ![Crusher anim](images/tile-anim-crusher.gif) |
| Saw | 6 | 167 ms | 1.0 s | ![Saw anim](images/tile-anim-saw.gif) |
| Water1 | 6 | 167 ms | 1.0 s | ![Water1 anim](images/tile-anim-water1.gif) |
| Water2 | 6 | 167 ms | 1.0 s | ![Water2 anim](images/tile-anim-water2.gif) |
| Temp | 20 (incl. 2 fully-transparent "vanish" frames) | 167 ms | 3.33 s | ![Temp anim](images/tile-anim-temp.gif) |
| Marine | 11 | 167 ms | 1.83 s | ![Marine anim](images/tile-anim-marine.gif) |
| FanLeft | 3 | 167 ms | 0.5 s | ![FanLeft anim](images/tile-anim-fanleft.gif) |
| FanRight | 3 | 167 ms | 0.5 s | ![FanRight anim](images/tile-anim-fanright.gif) |
| FanUp | 3 | 167 ms | 0.5 s | ![FanUp anim](images/tile-anim-fanup.gif) |
| FanDown | 3 | 167 ms | 0.5 s | ![FanDown anim](images/tile-anim-fandown.gif) |

The two `-1` ("invisible") frames in `Temp`'s table are rendered as fully-transparent frames here
(verified: alpha channel mean 0 on both), not skipped — matching the real vanish-then-reappear
behavior (Blupi falls through while invisible, per `02-tiles.md`).

## 2. Blupi character states (`blupi.png`, 8 fps base tick = 125 ms/frame — confirmed, throttled timer)

`Stop`={0}, `Down`={33}, `Up`={44} are single static frames (no animation) — see `03-objects.md`
for those.

| State | Frames | Duration/frame | Loop length | GIF |
|---|---|---|---|---|
| March (walk) | 6 | 125 ms | 0.75 s | ![March anim](images/blupi-anim-march.gif) |
| Jump | 3 | 125 ms | 0.375 s | ![Jump anim](images/blupi-anim-jump.gif) |
| Air (falling) | 5 | 125 ms | 0.625 s | ![Air anim](images/blupi-anim-air.gif) |
| SwimIdle | 10 | 125 ms | 1.25 s | ![SwimIdle anim](images/blupi-anim-swimidle.gif) |
| SwimMove | 14 | 125 ms | 1.75 s | ![SwimMove anim](images/blupi-anim-swimmove.gif) |

## 3. Object/pickup/enemy animations (`element.png` unless noted — see per-row sheet)

Durations below use the originally-claimed "6 fps ÷ divisor" figure inherited from the first
animation pass — **see the frame-rate correction note at the top of this file, these numbers are
not confirmed** (object animation is not throttled to a fixed rate in the code). Treat "duration"
columns here as illustrative ordering, not confirmed real-world timing.

### 3.1 Original 14 (from the first pass; `element.png`)

| ObjectType | Frames | Duration/frame (unconfirmed) | GIF |
|---|---|---|---|
| 2 (patrol enemy A) | 9 | 1.0 s | ![ObjectType2 anim](images/object-anim-type02-patrolA.gif) |
| 3 (patrol enemy B) | 9 | 1.0 s | ![ObjectType3 anim](images/object-anim-type03-patrolB.gif) |
| 4 (bulldozer) | 8 | 1.5 s | ![ObjectType4 anim](images/object-anim-type04-bulldozer.gif) |
| 16 (spider) | 9 | 0.5 s | ![ObjectType16 anim](images/object-anim-type16-spider.gif) |
| 17 (fish) | 8 | 1.0 s | ![ObjectType17 anim](images/object-anim-type17-fish.gif) |
| 20 (bird) | 8 | 1.0 s | ![ObjectType20 anim](images/object-anim-type20-bird.gif) |
| 33 (`blupit`) | 8 | 1.0 s | ![ObjectType33 anim](images/object-anim-type33-blupit.gif) |
| 5 (treasure sparkle) | 22 (11-icon ping-pong) | 1.5 s | ![ObjectType5 anim](images/object-anim-type05-treasure.gif) |
| 6 (extra-life egg) | 8 | 2.0 s | ![ObjectType6 anim](images/object-anim-type06-egg.gif) |
| 7 (level-exit goal) | 8 | 1.5 s | ![ObjectType7 anim](images/object-anim-type07-exit.gif) |
| 25 (shield) | 8 | 1.0 s | ![ObjectType25 anim](images/object-anim-type25-shield.gif) |
| 49 (key, `Key1`) | 12 | 1.5 s | ![ObjectType49 anim](images/object-anim-type49-key1.gif) |
| 50 (key, `Key2`) | 12 | 1.5 s | ![ObjectType50 anim](images/object-anim-type50-key2.gif) |
| 51 (key, `Key3`) | 12 | 1.5 s | ![ObjectType51 anim](images/object-anim-type51-key3.gif) |

`ObjectType25`'s `kShield[8]` is a known-incomplete port of mobile-eggbert's real 16-frame
`table_shield` (flagged in `plan.md`'s `DOC-002` entry) — the GIF above reflects galaxy-eggbert's
current (partial) table, not mobile-eggbert's full one.

### 3.2 New this pass — the 12 newly-supported `ObjectType`s (`03-objects.md` Category A)

Frame data from `src/GalaxyEggbertSimple3D/Game/GEDecorSystem.cpp`'s `GetObjIcon()`, transcribed
from mobile-eggbert's `table_blupih_left`/`table_guepe_left`/`table_creature_left`/`table_chenille`/
`table_follow1`/`table_follow2`. **Sheet correction applied per `DOC-003`'s finding**: `32` (blupih)
crops from `blupi1.png` (not `element.png`); `47` (platform-lift chenille track) crops from
`object-m.png` (not `element.png` — its icon indices 311–316 are out of `element.png`'s bounds
entirely). `19` (jeep), `46` (balloon), `55` (dynamite) are single static icons, confirmed
channel=10 (`element.png`) — no animation, no GIF needed (already in `03-objects.md`'s icon table).

| ObjectType | Frames | Sheet | GIF |
|---|---|---|---|
| 21 (secret-level exit) | 12 | `element.png` | ![ObjectType21 anim](images/object-anim-type21-secretexit.gif) |
| 24 (skateboard) | 34 | `element.png` | ![ObjectType24 anim](images/object-anim-type24-skateboard.gif) |
| 26 (suction-cup) | 8 | `element.png` | ![ObjectType26 anim](images/object-anim-type26-suctioncup.gif) |
| 32 (blupih, patrol+fires projectiles) | 8 | `blupi1.png` (corrected) | ![ObjectType32 anim](images/object-anim-type32-blupih.gif) |
| 40 (mirror/invert) | 20 | `element.png` | ![ObjectType40 anim](images/object-anim-type40-mirrorinvert.gif) |
| 44 (wasp/bee) | 6 | `element.png` | ![ObjectType44 anim](images/object-anim-type44-wasp.gif) |
| 47 (platform lift track) | 6 | `object-m.png` (corrected) | ![ObjectType47 anim](images/object-anim-type47-chenille.gif) |
| 54 (large creature) | 8 | `element.png` | ![ObjectType54 anim](images/object-anim-type54-largecreature.gif) |
| 96 (follower, dormant) | 26 | `element.png` | ![ObjectType96 dormant anim](images/object-anim-type96-follower-dormant.gif) |
| 96 (follower, awake/homing) | 5 | `element.png` | ![ObjectType96 awake anim](images/object-anim-type96-follower-awake.gif) |

All 9 remaining channels (`21,24,26,40,44,54,96`, plus the confirmed-`element.png` ones) were
cross-checked against real level files' `MoveObject: ... channel=` field (all `channel=10` =
`Element`, confirming `element.png` is correct for these — only `32` and `47` needed correction).

## 4. Explosions (`explo.png`, 144×144 px grid per `Pixmap.cpp`'s `PixmapChannel::Explosion`, 10 cols)

Frame data transcribed directly from mobile-eggbert's `Tables.cpp` `table_explo1`..`table_explo8`
(doc comments there label each as a distinct "channel" — channel numbering here is the table
index, 1–8, not to be confused with the `MoveObject:` `channel=` sprite-sheet field used elsewhere
in this doc). `-1` frames render as fully transparent (same technique as `Temp`'s vanish frames in
§1). Real per-icon bounding-box sizing (`Tables::table_explo_size`, keyed by a separate 0–99
"explosion channel" index, not cleanly 1:1 with these 8 tables) was not resolved in this pass — all
frames below use a flat 144×144 crop instead of each icon's real half-width/half-height box; treat
these GIFs as icon-sequence-correct but not pixel-perfect on sizing.

| Table | Frames | GIF | Notes |
|---|---|---|---|
| `table_explo1` | 39 | ![explo1](images/explosion-anim-explo1.gif) | primary blast sequence, no blank frames |
| `table_explo2` | 20 | ![explo2](images/explosion-anim-explo2.gif) | scattered debris, several `-1` blank frames |
| `table_explo3` | 20 | ![explo3](images/explosion-anim-explo3.gif) | smoke puff, no blank frames |
| `table_explo4` | 9 | ![explo4](images/explosion-anim-explo4.gif) | short impact flash |
| `table_explo5` | 12 | ![explo5](images/explosion-anim-explo5.gif) | strobing fragments (alternating visible/blank) |
| `table_explo6` | 6 | ![explo6](images/explosion-anim-explo6.gif) | dense burst |
| `table_explo7` | 128 | ![explo7](images/explosion-anim-explo7.gif) | large multi-particle scatter, staggered blanks, fades out |
| `table_explo8` | 5 | ![explo8](images/explosion-anim-explo8.gif) | dying-ember tail |

Which `ObjectType` triggers which `explo1`..`8` table was not resolved in this pass (the
`ObjectType8`–`12`/`36`–`38`/`41`/`42`/`53`/`90`–`93`/`98`–`100` "explosions/visual effects"
category in `03-objects.md` covers ~20 IDs for only 8 tables, so it's a many-to-one or
context-dependent mapping) — flagged as an open item, not guessed here.

## 5. Door opening (positional slide, not a frame-cycle — see `06-doors.md`)

Mobile-eggbert's door-open animation is a **position change**, not an icon-frame cycle: the door's
own static icon (334/335/336) slides upward by one tile over `Config::ScaleTime(50)` ticks (real
elapsed time depends on `Config::ScaleTime`'s scale factor, not resolved in this pass). Represented
here as a 10-frame linear slide of `Door1` (icon 334) from its resting position up and out of a
64×128 window — an illustrative approximation of the motion, not a frame-accurate port of the real
50-tick timing:

![Door slide anim](images/door-slide.gif)

## 6. `Tables.cpp` completeness sweep

Every `table_*` identifier found in mobile-eggbert's `Tables.cpp` (`grep`, exhaustive), categorized:

**Covered above:** `table_blupi` (Blupi states, §2), `table_decor_lave`/`_eau1`/`_eau2`/`_ecraseur`/
`_scie`/`_temp`/`_piege1`/`_piege2` (tiles, §1 — Spike's two `_piege` tables both map into the
single `Spike` sequence already documented), `table_marine` (§1), `table_decor_ventb`/`_ventd`/
`_ventg`/`_venth` (the 4 fan tiles, §1), `table_cle1`/`_cle2`/`_cle3` (keys, §3.1), `table_tresortrack`
(treasure, §3.1), `table_blupih_left` (§3.2), `table_guepe_left` (§3.2), `table_creature_left`
(§3.2, "same for both directions in source" per `GEDecorSystem.cpp`'s comment), `table_chenille`
(§3.2), `table_follow1`/`table_follow2` (§3.2), `table_skate` (§3.2 as `24`'s table), `table_power`
(§3.2 as `26`'s table), `table_invert` (§3.2 as `40`'s table), `table_cle` (generic key table, used
as `21`'s secret-exit animation), `table_explo1`–`table_explo8` (§4).

**Deferred — real animation tables that exist but were not built into a GIF this pass** (mostly
directional-variant tables for enemies whose "left" cycle is already shown, or effects for
`ObjectType`s confirmed in `03-objects.md` Category B as never placed in any real level, so lower
value to visualize): `table_blupih_right`/`_turn2l`/`_turn2r`, `table_blupit_left`/`_right`/
`_turn2l`/`_turn2r` (blupit's own directional tables — the existing `kBlupit[8]` in `GetObjIcon` is
a simplification, not sourced 1:1 from these), `table_bulldozer_left`/`_right`/`_turn2l`/`_turn2r`,
`table_guepe_right`/`_turn2l`/`_turn2r`, `table_creature_turn2`, `table_oiseau_left`/`_right`/
`_turn2l`/`_turn2r`, `table_poisson_left`/`_right`/`_turn2l`/`_turn2r`, `table_bridge` (`ObjectType52`),
`table_charge` (`ObjectType31`), `table_chenillei` (inverse/return chenille direction), `table_clear`
(`ObjectType37`), `table_decor_goutte` (unidentified — droplet decoration?), `table_decor_ventillob`/
`_ventillod`/`_ventillog`/`_ventilloh` (unidentified — possibly the fan *device* sprite distinct from
the wind-effect tiles already covered; not resolved), `table_drinkeffect`/`_drinkoffset`/
`_drinkoffsetLength` (`ObjectType30`'s drink effect, beyond its already-documented static icon 178),
`table_dynamitef` (`ObjectType56`, dynamite fuse), `table_electro` (`ObjectType38`), `table_glu`
(`ObjectType34`), `table_invertpanel`/`_invertstart`/`_invertstop` (`ObjectType41`/`42` particle
bursts, related to but distinct from `table_invert` already covered), `table_magicloop`/
`_magictrack` (`ObjectType27`), `table_mirror` (unidentified — possibly redundant with
`table_invert`, not resolved), `table_plouf` (`ObjectType14`), `table_pollution` (`ObjectType36`),
`table_ressort` (Spring tile bounce visual, if any — `Spring`/211 is documented as non-animated in
`02-tiles.md`, this table's exact role wasn't resolved), `table_shield_blupi`/`_shieldloop`/
`_shieldtrack` (`ObjectType57`/`58` shield effects beyond the base `kShield` already covered),
`table_sploutch1`/`_2`/`_3` (`ObjectType98`/`99`/`100` splash variants), `table_tentacule`
(`ObjectType53`), `table_tiplouf` (`ObjectType35`).

**Out of scope — not a visual frame-cycle animation:** `table_decor_quart` (7056-entry passability
lookup, the basis of `isMobileTransparent` — already documented in `02-tiles.md`), `table_decor_action`
(519-entry tile-motion sub-pixel displacement scripts, not a sprite sequence), `table_adapt_decor`/
`_adapt_fromage` (tile corner-blending replacement lookups, not animation), `table_explo_size`
(per-channel bounding-box size parameter, referenced in §4 above, not a sequence itself),
`table_training1`–`4` (tutorial-overlay placement records — column/row/action/text-ID, not sprite
frames), `table_vitesse_march`/`_nage`/`_surf` (movement speed-per-tick parameters, not visuals).

## 7. Not yet covered / open items

- Real timing (frame duration) for object animations is unconfirmed — see the correction note at
  the top of this file.
- ~35 tables listed as "deferred" in §6 have real source data but no GIF yet.
- The `explo1`–`8` → `ObjectType` trigger mapping (§4) and `table_explo_size`'s real per-icon
  bounding boxes are unresolved.
- `06-doors.md`'s exact `Config::ScaleTime(50)` real-time duration for the door slide (§5) wasn't
  resolved (depends on a scale factor not looked up in this pass).
