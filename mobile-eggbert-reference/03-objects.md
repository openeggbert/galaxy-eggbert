# Objects/Elements Catalog (`ObjectType`, 204 IDs)

**Status: COMPLETE for classification and icons.** Every ID 0–203 is placed in exactly one of four
categories (real behavior + used in shipped levels / real behavior but never placed in a shipped
level / ambiguous boundary-only reference / vestigial with zero references), verified
programmatically against real mobile-eggbert source and all 78 world files. **Follow-up
(2026-07-03): icon crops added for Category B too, per user request — "no icon, lower value" is
not an acceptable silent scope cut.** Of Category B's 41 IDs, 37 got a real crop (sourced from a
direct `Decor.cpp`/`Tables.cpp` case, not guessed); the remaining 4 (`0`, `18`, `22`, `58`) have no
crop because no icon-assignment logic for them exists anywhere in `Decor.cpp` (`0`/`18`/`58`) or
because the icon is inherently variable, not fixed (`22` — inherits whichever door tile spawned it;
see `06-doors.md`) — documented explicitly per-ID in the table below, not silently skipped. Along
the way, corrected 3 more sprite-channel assumptions inherited from the original (wrong)
"everything is `element.png`" premise: `14`/`15`/`31`/`35`/`48`/`52` actually use `object-m.png`
(`PixmapChannel::Object`), and `38`'s first 30 ticks use `blupi1.png` before switching to
`element.png`. Tracked as `DOC-003` in `plan.md`.

**`DOC-231` (2026-07-04):** re-verified all 67 icon crops' sheet/channel and grid math. The 7 crops
sourced from `object-m.png` (`14`, `15`, `31`, `35`, `47`, `48`, `52`) had the same leading-margin
bug as `DOC-230`'s tile crops (missing the 1px margin before the first row/column) and were
regenerated with the corrected formula from `02-tiles.md`. The other 60 crops (`element.png`/
`blupi.png`/`blupi1.png`/`explo.png`) are unaffected — those sheets divide evenly into their tile
grid with no gap or leading margin (`600×1740`/`600×2040`/`1440×1440`, all exact multiples of their
tile size), confirmed by direct `identify` measurement, not assumed.

`ObjectType` (`include/GalaxyEggbert/def/ObjectType.hpp`, mirrors mobile-eggbert's own enum
1:1 in numeric value, confirmed 0–203 with no gaps/duplicates) is already fully declared in
galaxy-eggbert with categorized comments. Summary by category (not re-listing all 204 IDs — see
the header for the full, already-commented list):

- **Platform lifts**: 1, 47, 48
- **Horizontal patrol enemies**: 2, 3, 96, 97
- **Bulldozer**: 4
- **Collectibles**: 5 (treasure), 6 (extra-life egg), 7 (level-exit goal), 21 (secret-level exit), 39 (pickup sparkle)
- **Key collectibles**: 49, 50, 51 (correspond to `DoorKeyFlags::Key1/2/3`)
- **Vehicle/power-up pickups**: 13 (helicopter), 19 (jeep), 24 (skateboard), 25 (shield), 26 (suction-cup), 28 (tank), 29 (bullet pack), 30 (drink), 31 (charge/cloud), 40 (mirror/invert), 46 (balloon), 55 (dynamite)
- **Explosions/visual effects** (transient, auto-expire): 8–12, 36–38, 41, 42, 53, 90–93, 98–100
- **Water/goo effects**: 14, 15, 34, 35
- **Projectiles**: 23 (fired by `blupih`/`blupit` enemies)
- **Patrol walker enemies**: 16 (spider), 17 (fish), 18 (variant), 20 (bird), 32 (`blupih`, fires projectiles on turn), 33 (`blupit`, fires two), 44 (wasp/bee), 54 (large creature, destroys helicopter on contact)
- **Moving level objects**: 22 (door-opening animation, see `06-doors.md`), 27 (magic sparkle), 52 (bridge construction, 157 frames), 56 (dynamite fuse), 57/58 (shield effects)
- **Blupi skin variants**: 200–203
- **Unidentified/reserved**: 43, 45, 59–89, 94, 101–199 (declared only to keep the enum contiguous
  for level-file round-trips — **now confirmed by direct source search, not assumed**: zero
  references to any of these IDs exist anywhere in `Decor.cpp` or `Tables.cpp`. `95` is a partial
  exception — see Category C below.)

## Classification methodology (verified programmatically)

For every ID 0–203: (1) searched `../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp`'s
`MoveObjectStepIcon()` function (the per-type icon/animation dispatch, lines ~8192–9057) for a
direct `ObjectType::ObjectTypeN` comparison; (2) for IDs not found there, searched the rest of
`Decor.cpp` and `Tables.cpp` for any reference at all; (3) cross-referenced against a real scan of
every `MoveObject: type=N` value across all 78 world files (`grep`, not a manual sample). Verified
by script: the four categories below partition all 204 IDs exactly — no ID in two categories, no ID
missing (`29 + 41 + 1 + 133 = 204`).

## Category A — real behavior AND placed in a shipped level (29 IDs)

`1,2,3,4,5,6,7,12,13,16,17,19,20,21,24,26,30,32,33,40,44,46,47,49,50,51,54,55,96`

**Fixed 2026-07-03**: 12 of these (`19,21,24,26,32,40,44,46,47,54,55,96`) previously never spawned
at all in `GalaxyEggbertSimple3D` (silently dropped by a hardcoded allowlist). All 12 now spawn,
verified against real level files (`tools/VerifyMoveObjectTypes.cpp`, 12/12 pass):

| Type | Name | Status |
|---|---|---|
| 47 | platform lift | Full — reuses `ObjectType1`'s lift movement, animated `table_chenille` track texture |
| 32 | blupih (patrol, fires projectiles) | Full — patrol movement, real `table_blupih_left` icon table |
| 44 | wasp/bee | Full — patrol movement, real `table_guepe_left` icon table |
| 54 | large creature | Full — patrol movement, real `table_creature_left` icon table |
| 96 | follower | Full — dormant→awake→chase behavior approximating mobile-eggbert's `Decor::MoveObjectFollow` |
| 19 | jeep | Partial — spawns with correct icon/animation, no pickup effect (vehicle mode not implemented) |
| 21 | secret-level exit | Partial — spawns with correct icon/animation, no pickup effect |
| 24 | skateboard | Partial — spawns with correct icon/animation, no pickup effect |
| 26 | suction-cup | Partial — spawns with correct icon/animation, no pickup effect |
| 40 | mirror/invert | Partial — spawns with correct icon/animation, no pickup effect |
| 46 | balloon | Partial — spawns with correct icon/animation, no pickup effect |
| 55 | dynamite | Partial — spawns with idle icon only, no fuse/explosion trigger (`ObjectType56`) wired up |

(The "partial" ones match an existing precedent: `ObjectType13`, helicopter, has had "spawns, no
collection effect" status all along — this isn't a new kind of gap.)

### ⚠️ Real bug found this pass: wrong sprite-sheet channel for at least 5 types

`GEDecorSystem.cpp`'s file header says "Sprites from `element.png`" and draws **every** `ObjectType`
via that one sheet unconditionally. Checking real level files' `MoveObject: ... channel=N` field
(the field that tells mobile-eggbert which actual sprite sheet to sample — see
`01-world-file-format.md`) shows this is **not universally true**:

| Type | Real `channel=` value in shipped levels | Real sheet | galaxy-eggbert currently assumes |
|---|---|---|---|
| 1 | `channel=1` | `PixmapChannel::Object` (`object-m.png`) | `element.png` (wrong) |
| 12 | `channel=1` | `PixmapChannel::Object` (`object-m.png`) | `element.png` (wrong) |
| 32 (blupih) | `channel=12` or `13` | `PixmapChannel::Blupi1_12`/`Blupi1_13` (`blupi1.png`) | `element.png` (wrong) |
| 33 (blupit) | `channel=11`, `12`, or `13` | `PixmapChannel::Blupi1_11/12/13` (`blupi1.png`) | `element.png` (wrong) — **this is one of the *original* 18 "confirmed" types**, not a new one |
| 47 (platform lift, track texture) | `channel=1` | `PixmapChannel::Object` (`object-m.png`) | `element.png` (wrong — and `table_chenille`'s icon values 311–316 are **out of bounds** for `element.png`, which only holds icons 0–289; cropping icon 311 from `element.png` throws an ImageMagick range error, confirmed) |

This means galaxy-eggbert's Simple3D port is very likely drawing the *wrong texture* (or, for type
47/48, a texture-sheet-bounds violation) for these 5 `ObjectType`s today. This is a real rendering
bug, not just a documentation gap — **not fixed in this pass** (out of scope for a cataloging task;
needs its own careful fix, likely reading `.channel` per-`MoveObjectSpec` from the level file
instead of hardcoding `element.png`). Recorded as a new item in `plan.md`.

`object-type047-icon311-objectchannel.png` (correct sheet) replaces the earlier, wrong-sheet crop
for type 47 in the icon table below.

## Category B — real behavior confirmed, but never placed in any of the 78 shipped levels (41 IDs)

These have genuine logic in `Decor.cpp` (confirmed by direct search, not assumed), but no level
author ever placed one — they may still be reachable through non-`MoveObject` paths (e.g. spawned
dynamically by other game logic, like explosion/effect types typically are) rather than being
level-authored placements.

| ID | Name (from `ObjectType.hpp`) | Image | Sheet / source |
|---|---|---|---|
| 0 | null / inactive slot | *(no icon — this is a slot-recycling marker, never assigned in `MoveObjectStepIcon()`; every other type sets `type = ObjectType0` on expiry, it has no visual of its own)* | — |
| 8 | primary explosion (Explosion channel, 39 frames) | ![8](images/object-type008-icon000-explo1.png) | `explo.png`, `table_explo1[0]`=0 |
| 9 | secondary small explosion (20 frames) | ![9](images/object-type009-icon012-explo2.png) | `explo.png`, `table_explo2[0]`=12 |
| 10 | tertiary explosion (20 frames) | ![10](images/object-type010-icon032-explo3.png) | `explo.png`, `table_explo3[0]`=32 |
| 11 | fan-hit shockwave (9 frames, triggers BigShake) | ![11](images/object-type011-icon012-explo4.png) | `explo.png`, `table_explo4[0]`=12 |
| 14 | water plouf splash (7 frames) | ![14](images/object-type014-icon099-plouf.png) | `object-m.png` (**not `element.png`**), `table_plouf[0]`=99 |
| 15 | water bubble rising (20 frames) | ![15](images/object-type015-icon103-blup.png) | `object-m.png` (**not `element.png`**), `table_blup[0]`=103 |
| 18 | patrol variant | *(no icon — no `MoveObjectStepIcon()` case found anywhere for `ObjectType18`; only referenced by ID inside a generic "has a sprite" type-list, never given its own animation logic)* | — |
| 22 | door opening animation (dynamically spawned when a door opens — see `06-doors.md`) | *(no fixed icon — inherits whichever door tile spawned it, sliding that exact icon; see `02-tiles.md`'s Door1/2/3 crops, icons 334/335/336)* | `object-m.png`, variable |
| 23 | fired projectile (from blupih/blupit enemies — dynamically spawned, not level-placed) | ![23](images/object-type023-icon176-projectile.png) | `element.png`, static icon 176 |
| 25 | shield (100 ticks invincibility) — confirmed real 16-frame `table_shield`; apparently never a placed `MoveObject`, likely granted as an effect not an authored pickup | ![25](images/object-type025-icon144-shield16.png) | `element.png`, `table_shield[0]`=144 |
| 27 | magic track sparkle (24 frames) | ![27](images/object-type027-icon152-magictrack.png) | `element.png`, `table_magictrack[0]`=152 |
| 28 | tank | ![28](images/object-type028-icon167-tank.png) | `element.png`, static icon 167 |
| 29 | bullet ammo pack (+10 bullets) | ![29](images/object-type029-icon177-bulletpack.png) | `element.png`, static icon 177 |
| 31 | charge/cloud power-up (6 frames, 100 ticks) | ![31](images/object-type031-icon238-charge.png) | `object-m.png` (**not `element.png`**), `table_charge[0]`=238 |
| 34 | goo/glue particle (25-frame loop) | ![34](images/object-type034-icon168-glu.png) | `element.png`, `table_glu[0]`=168 |
| 35 | small plouf splash (3 frames) | ![35](images/object-type035-icon244-tiplouf.png) | `object-m.png` (**not `element.png`**), `table_tiplouf[0]`=244 |
| 36 | pollution/cloud puff (8 frames) | ![36](images/object-type036-icon179-pollution.png) | `element.png`, `table_pollution[0]`=179 |
| 37 | clear/dissipate effect (70 frames) | ![37](images/object-type037-icon040-clear.png) | `element.png`, `table_clear[0]`=40 |
| 38 | electric arc (90 frames) — **two-channel animation**: first 30 ticks draw from `blupi1.png`, ticks 30–89 from `element.png` | ![38](images/object-type038-icon266-electro-blupi1channel.png) | `blupi1.png` (ticks 0–29), then `element.png`; `table_electro[0]`=266 |
| 39 | sparkle trail (spawned on pickup, not level-placed; `table_tresortrack` is an 11-frame sparkle sequence related to `ObjectType5`'s treasure sparkle in `08-animations.md`, not identical) | ![39](images/object-type039-icon166-tresortrack.png) | `element.png`, `table_tresortrack[0]`=166 |
| 41 | invert-start particle burst (8 frames) | ![41](images/object-type041-icon179-invertstart.png) | `element.png`, `table_invertstart[0]`=179 |
| 42 | invert-stop particle burst (8 frames) | ![42](images/object-type042-icon186-invertstop.png) | `element.png`, `table_invertstop[0]`=186 |
| 48 | platform lift, leftward carry bonus — mirrors `47`'s `table_chenille` with `table_chenillei` (6 frames); channel not independently confirmed in source (no explicit `channel=` assignment found for `48`, inferred from `47`'s identical pattern, corrected the same way) | ![48](images/object-type048-icon316-chenillei-objectchannel.png) | `object-m.png` (inferred), `table_chenillei[0]`=316 |
| 52 | bridge construction (157 frames) | ![52](images/object-type052-icon365-bridge.png) | `object-m.png` (**not `element.png`**), `table_bridge[0]`=365 |
| 53 | tentacle hazard (45 frames, Explosion channel) | ![53](images/object-type053-icon086-tentacule.png) | `explo.png`, `table_tentacule[0]`=86 |
| 56 | dynamite fuse (100 frames, triggers blasts 50–69 — dynamically spawned by `55`, not level-placed directly) | ![56](images/object-type056-icon253-dynamitefuse.png) | `element.png`, `table_dynamitef[0]`=253 |
| 57 | shield trail sparkle (20 frames) | ![57](images/object-type057-icon274-shieldtrack.png) | `element.png`, `table_shieldtrack[0]`=274 |
| 58 | shield disappear effect | *(no icon — the only `ObjectType58` logic found is a phase≥20-ticks removal; no icon is ever assigned to it)* | — |
| 90 | electric spark (12 frames, triggers ElectricShake) | ![90](images/object-type090-icon054-explo5.png) | `explo.png`, `table_explo5[0]`=54 |
| 91 | small flash (6 frames) | ![91](images/object-type091-icon054-explo6.png) | `explo.png`, `table_explo6[0]`=54 |
| 92 | long energy arc (128 frames) | ![92](images/object-type092-icon060-explo7.png) | `explo.png`, `table_explo7[0]`=60 |
| 93 | tiny flash (5 frames) | ![93](images/object-type093-icon007-explo8.png) | `explo.png`, `table_explo8[0]`=7 |
| 97 | follow enemy variant 2 (5 frames; tracks Blupi exactly — the "awake" promotion of `96`, dynamically switched-to, not level-placed as `97` directly) | ![97](images/object-type097-icon256-follow2.png) | `element.png`, `table_follow2[0]`=256 |
| 98 | water splash variant 1 (10 frames) | ![98](images/object-type098-icon090-sploutch1.png) | `explo.png`, `table_sploutch1[0]`=90 |
| 99 | water splash variant 2 (13 frames; first 3 frames are invisible — `-1` in the table, a fall-height delay before the splash appears) | ![99](images/object-type099-icon090-sploutch2.png) | `explo.png`, `table_sploutch2[3]`=90 (first real frame) |
| 100 | water splash variant 3 (18 frames; first 8 frames are invisible, same delay pattern as `99` but for a greater fall height) | ![100](images/object-type100-icon090-sploutch3.png) | `explo.png`, `table_sploutch3[8]`=90 (first real frame) |
| 200 | Blupi default skin (6-frame idle cycle, icons 257–262) | ![200](images/object-type200-icon257-blupiskin0.png) | `blupi.png`, icon 257 |
| 201 | Blupi skin variant 1 (damages Blupi on contact) — same base frames as `200`; the `Blupi1_11`/`_12`/`_13` channels all read the identical `blupi1.png` pixels (confirmed: `Pixmap::GetBitmap()` returns the same bitmap for all three), so any color/tint difference is applied at render time, not visible in a raw crop | ![201](images/object-type201-icon257-blupiskin1.png) | `blupi1.png` (untinted), icon 257 |
| 202 | Blupi skin variant 2 (same caveat as `201`) | ![202](images/object-type202-icon257-blupiskin2.png) | `blupi1.png` (untinted), icon 257 |
| 203 | Blupi skin variant 3 (same caveat as `201`) | ![203](images/object-type203-icon257-blupiskin3.png) | `blupi1.png` (untinted), icon 257 |

## Category C — ambiguous: boundary-only reference (1 ID)

**`95`** appears in `Decor.cpp` only as a numeric bound inside range comparisons (e.g.
`type > ObjectType::ObjectType95`, `type <= ObjectType::ObjectType95`) used to test whether some
*other* object's type falls inside a broad "effect-ish" numeric range — it is never itself the
subject of a direct `type == ObjectType95` check. Unclear whether `95` denotes a real, distinct
object or is purely a range-fence value chosen because `94`/`95` happen to sit between confirmed
ranges (`90–93` and `96–100`). Left unresolved rather than guessed.

## Category D — vestigial: zero references anywhere in `Decor.cpp`/`Tables.cpp` (133 IDs)

`43`, `45`, `48`(see Category B, not here), `59`–`89`, `94`, `101`–`199`. Exact ranges (verified,
no overlap with categories A/B/C, no gaps against 0–203): `43`, `45`, `59-89`, `94`, `101-199`.
These are genuinely inert — the enum only declares them to keep level-file byte offsets/round-trips
stable, not because they do anything in the original game.

## Known icons (phase-0 frame)

Images are 60×60 px crops from `element.png` (60×60 px tiles, no gap, 10 columns — see
`02-tiles.md`'s image-generation note for the general grid-math method), except `47` which is
64×64 from `object-m.png` (see the channel-bug note above). 30 of the 70 Category-A/confirmed IDs
have a crop: the original 18 (from `GEDecorSystem::GetObjIcon`) plus the 12 added in the
2026-07-03 `MoveObject` fix.

| Image | ObjectType | Category (from above) |
|---|---|---|
| ![ObjectType1](images/object-type001-icon029.png) | 1 | Platform lift |
| ![ObjectType2](images/object-type002-icon012.png) | 2 | Horizontal patrol enemy |
| ![ObjectType3](images/object-type003-icon048.png) | 3 | Horizontal patrol enemy |
| ![ObjectType4](images/object-type004-icon066.png) | 4 | Bulldozer |
| ![ObjectType5](images/object-type005-icon000.png) | 5 | Collectible (treasure) |
| ![ObjectType6](images/object-type006-icon021.png) | 6 | Collectible (extra-life egg) |
| ![ObjectType7](images/object-type007-icon029.png) | 7 | Collectible (level-exit goal) |
| ![ObjectType12](images/object-type012-icon032.png) | 12 | Explosion/visual effect (icon shown is via `element.png` — real channel is `Object`, see bug note above) |
| ![ObjectType13](images/object-type013-icon068.png) | 13 | Vehicle/power-up pickup (helicopter) |
| ![ObjectType16](images/object-type016-icon069.png) | 16 | Patrol walker enemy (spider) |
| ![ObjectType17](images/object-type017-icon082.png) | 17 | Patrol walker enemy (fish) |
| ![ObjectType19](images/object-type019-icon089.png) | 19 | Vehicle/power-up pickup (jeep) |
| ![ObjectType20](images/object-type020-icon098.png) | 20 | Patrol walker enemy (bird) |
| ![ObjectType21](images/object-type021-icon122.png) | 21 | Collectible (secret-level exit) |
| ![ObjectType24](images/object-type024-icon129.png) | 24 | Vehicle/power-up pickup (skateboard) |
| ![ObjectType25](images/object-type025-icon144.png) | 25 | Vehicle/power-up pickup (shield) |
| ![ObjectType26](images/object-type026-icon136.png) | 26 | Vehicle/power-up pickup (suction-cup) |
| ![ObjectType30](images/object-type030-icon178.png) | 30 | Vehicle/power-up pickup (drink) |
| ![ObjectType32](images/object-type032-icon066.png) | 32 | Patrol walker enemy (`blupih`) — icon shown is via `element.png`; real channel is `blupi1.png`, see bug note above |
| ![ObjectType33](images/object-type033-icon249.png) | 33 | Patrol walker enemy (`blupit`) — icon shown is via `element.png`; real channel is `blupi1.png`, see bug note above |
| ![ObjectType40](images/object-type040-icon187.png) | 40 | Vehicle/power-up pickup (mirror/invert) |
| ![ObjectType44](images/object-type044-icon195.png) | 44 | Patrol walker enemy (wasp/bee) |
| ![ObjectType46](images/object-type046-icon208.png) | 46 | Vehicle/power-up pickup (balloon) |
| ![ObjectType47](images/object-type047-icon311-objectchannel.png) | 47 | Platform lift (track texture) — correct-channel crop (`object-m.png`) |
| ![ObjectType49](images/object-type049-icon209.png) | 49 | Key collectible (`Key1`) |
| ![ObjectType50](images/object-type050-icon220.png) | 50 | Key collectible (`Key2`) |
| ![ObjectType51](images/object-type051-icon229.png) | 51 | Key collectible (`Key3`) |
| ![ObjectType54](images/object-type054-icon247.png) | 54 | Patrol walker enemy (large creature) |
| ![ObjectType55](images/object-type055-icon252.png) | 55 | Vehicle/power-up pickup (dynamite, idle icon) |
| ![ObjectType96](images/object-type096-icon256.png) | 96 | Horizontal patrol enemy (follower, dormant) |

## Blupi (not an `ObjectType` — the player character, `blupi.png`/`blupi1.png`)

A few representative 60×60 px frames (of 340 total across the sheet's 10×34 grid) — not a full
state/animation catalog (see `08-animations.md` for animated GIFs of Blupi's actual states):

![Blupi frame 0](images/blupi-icon000.png)
![Blupi frame 1](images/blupi-icon001.png)
![Blupi frame 5](images/blupi-icon005.png)
![Blupi frame 10](images/blupi-icon010.png)

**`DOC-232` (2026-07-04):** re-verified these 4 against the now-complete `table_blupi` parse
(`extract-blupi-action.py`) and the fixed sheet-grid formula. Pixel-exact match (`compare -metric
AE`=0) against the flat 60×60/10-col grid with no gap — `blupi.png` is 600×2040 (exact multiple of
60 in both dimensions), so it never had the `object-m.png` leading-margin bug. The 4 frames turned
out to already be meaningful, not arbitrary: icon 0 is `BlupiAction::Stop`'s frame (`ACTION_STOP`,
idle), icon 1 is `Turn`'s first frame (`ACTION_TURN`), and icons 5/10 are `March`'s first/last frame
(`ACTION_MARCH`, a 6-frame walk cycle 5–10) — confirmed by reading `table_blupi` live. No
regeneration needed.
