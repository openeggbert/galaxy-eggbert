# Hazards & Interactive Tiles — Gameplay Behavior

**Status:** new document (`DOC-302`), first pass at cataloging what happens when Blupi *touches or
uses* each hazard/interactive tile. `02-tiles.md` already catalogs which icon/animation each tile
is and its coarse passability — this document does not repeat that identity work, only the
gameplay consequence. Everything below is confirmed directly from
`mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp` (read-only reference; line numbers are
approximate, re-verified against a 2026-07-05 read of the file). Most hazard checks are called from
`Decor::BlupiStep()` (`Decor.cpp` ~2734), the per-frame Blupi update.

## Shared death/respawn mechanism

Several hazards below kill Blupi the same way: `Decor::BlupiDead(action1, action2)`
(`Decor.cpp` ~6547) sets `m_blupiAction` to `action1`, or randomly (50/50) to `action1` or
`action2` if both are supplied, clears every ride/power/focus flag (helico, over, jeep, tank,
skate, nage, surf, suspend, shield, power, hide, balloon, ecrase, cloud, jump-aie, invert), stops the
looping engine/vehicle sound channels (16/18/29/31), and hides both HUD gauges. Each death
`BlupiAction` (`Clear1`…`Clear8`, `Glu`, `Drown`, `Electro`) then plays out its own animation for a
fixed tick count before the game checks lives: `Clear1`/`Clear3` = 70 ticks, `Clear2` = 100,
`Clear4` = 110, `Clear5`–`Clear8`/`Drown`/`Electro` = 90, `Glu` = 100 (all `Config::ScaleTime()`
ticks, i.e. expressed at the normalized 20 FPS baseline — divide by 20 for seconds, e.g. `Clear1`
≈ 3.5 s, `Clear4` ≈ 5.5 s). When the animation completes, if `m_nbVies > 0` (lives remain) Blupi is
revived at `m_blupiValidPos` — but **only if `m_blupiRestart` was set true** by the hazard that
killed him; otherwise he revives in place. `m_blupiValidPos` is a rolling last-known-safe position,
updated every frame Blupi is grounded, focused, and not standing in/on lava, spikes, a drip, a saw,
a bridge tile, a teleporter tile, a Blitz tile (any phase), a vanishing (`Temp`) tile, a bullet
path, or a moving-object path (`Decor.cpp` ~6467–6471) — so a respawn can never place Blupi back
inside a hazard's danger zone, even on a currently-inactive frame of that hazard's cycle.

## Lava (`Decor::IsLave`, `Decor.cpp` ~7195)

Detects icon 68 (the `Lava` base icon) directly under Blupi's feet-offset position (`pos.X+30`).
Touching it kills Blupi via `BlupiDead(Clear3, nullopt)` (deterministic, not randomized), sets
`m_blupiRestart = true`, snaps his Y to the tile row (`BLUPIOFFY` offset), and plays `SoundChannel8`
(`Decor.cpp` ~5497). **No vehicle immunity** — being in a jeep/tank/"over" transport does not
protect against lava, and this check does not require `m_blupiFocus` either, so lava can kill Blupi
even during some non-player-controlled action. The only protections are `m_blupiShield`,
`m_blupiHide`, or the `m_bSuperBlupi` debug/cheat flag.

## Spikes (`Decor::IsPiege`, `Decor.cpp` ~7205)

Detects icon 373 at Blupi's feet line (`pos.Y+60`), restricted to a narrow central x-band of the
tile (`pos.X%64` in roughly 15–49; touching the tile's edges doesn't count). Kills Blupi via
`BlupiDead(Glu, nullopt)`, sets `m_blupiRestart = true` and `m_blupiAir = true`, spawns an
`ObjectType53` effect at his position, and plays `SoundChannel51` (`Decor.cpp` ~5504). **Vehicle
immunity**: riding `Over`/`Jeep`/`Tank` protects against spikes (unlike lava/Blitz below), and this
check additionally requires `m_blupiFocus` (only lethal while the player has normal control).
Shield/Hide/SuperBlupi also protect.

## Drip / droplet hazard (`Decor::IsGoutte`, `Decor.cpp` ~7220)

Two modes via the `bAlways` parameter. The real-time lethal check (`bAlways=false`, called at
`Decor.cpp` ~5513) matches only icon 404 — the drip's actual "impact" frame — at Blupi's raw
position (no foot/head offset, since a falling drip can hit any part of his body), same narrow
central x-band as spikes. The broader check (`bAlways=true`, used only to gate the safe-checkpoint
system) also matches icon 410, a wider "danger zone" that covers the drip's non-impact animation
phase too, so Blupi is never checkpointed anywhere in the drip's cycle, not just its exact kill
frame. On hit: `BlupiDead(Glu, nullopt)`, `m_blupiRestart = true`, `m_blupiAir = true`,
`SoundChannel51`. Same immunity set as spikes (vehicle modes + Shield/Hide/SuperBlupi protect;
requires focus).

## Saw (`Decor::IsScie`, `Decor.cpp` ~7243)

Detects icon 378 — the **active/spinning** saw icon — in a wide central x-band (`pos.X%64` 4–60).
Icon 379 (`SawStopped`, a real named constant per `02-tiles.md`) is the same tile in its inert
state and is never lethal; see "Switches" below for how a level flips a saw between the two. On
hit: `BlupiDead(Clear4, nullopt)`, `m_blupiFront = true`, `m_blupiRestart = true`,
`m_blupiAir = true` (`Decor.cpp` ~5521); `BlupiDead`'s own `Clear4` branch additionally spawns three
`ObjectType41` debris bursts and plays `SoundChannel75`. Same immunity set as spikes/drip (vehicle
modes protect; requires focus; Shield/Hide/SuperBlupi protect).

## Switches (`Decor::IsSwitch` ~7257, `Decor::ActiveSwitch` ~7131)

`IsSwitch` detects icon 384 (switch, "on"/open) or 385 ("off"/closed) under Blupi in a wide central
x-band (same band as the saw check). The trigger is the player's action button: pressing it while
standing on a switch (and not riding a vehicle/balloon/skate, not shielded/hidden/super, and not
already interacting with a nearby `MoveObject`) calls
`ActiveSwitch(currentIcon == 385, cel)` — i.e. the switch always toggles to the opposite of its
current state — and puts Blupi into a fixed ~10-tick `Switch` pose animation with zero velocity
(`Decor.cpp` ~5529–5540). `ActiveSwitch` itself is instantaneous, no separate delay: it flips the
switch tile's own icon (384↔385) and plays a click sound (`SoundChannel77` for "on", `76` for
"off"), then scans a **fixed window of 41 cells** (the switch's column ±20 tiles) and toggles every
matching saw tile it finds between 378 (active) and 379 (stopped) to match the new switch state.
Saws farther than 20 tiles from a switch can never be linked to it — the link is purely
positional/implicit, there is no separate wiring table.

## Crusher (`Decor::IsEcraseur`, `Decor.cpp` ~7277)

Detects icon 317 (the `Crusher` base icon), but only during a **3-out-of-10 window** of a raw
`m_time`-driven cycle (`m_time / 3 % 10 <= 2`; note, unlike `BlitzActif` below, this divisor is
*not* passed through `Config::ScaleDiv`, so it is not FPS-normalized the way most other timers
are) — i.e. the crusher is only dangerous for roughly the down-stroke portion of its animation, not
the whole cycle. Touching it during that window does **not** kill Blupi outright: it sets
`m_blupiEcrase = true`, zeroes his velocity, switches his pose to a flattened `StopEcrase`/
`MarchEcrase` animation, plays `SoundChannel70`, spawns four `ObjectType41` debris bursts plus a
screen-shake. While squashed, Blupi can still move (at reduced effective speed) but cannot jump,
swim, ride vehicles, etc. Recovery is on a timer reusing the shield-timer field
(`m_blupiTimeShield`, set to 100 and decremented once every `Config::ScaleTime(2)` ticks — roughly
200 normalized ticks / ~10 s total): when it reaches 0, `m_blupiEcrase` clears automatically, Blupi
becomes briefly airborne, and another four-debris-burst "pop" effect plays (`Decor.cpp` ~5182–5197).
Unlike the spike/drip/saw group, there is **no vehicle immunity** for the crusher — only
`!m_blupiEcrase` (can't retrigger while already squashed), Shield, Hide, and SuperBlupi prevent it;
it requires focus.

## Blitz — electric hazard (`Decor::IsBlitz` ~7291, `Decor::BlitzActif` ~620)

Icon 305 is the hazard cell itself; icon 304 in the cell directly above it is a purely cosmetic
"emitter" used only to time a zap sound cue, not part of the collision/lethality check. Like
`IsGoutte`, `IsBlitz` has a `bAlways` bypass: `bAlways=true` (used only for checkpoint-gating) treats
any icon-305 cell as always dangerous regardless of its flicker phase; `bAlways=false` (the real
per-frame lethal check) additionally requires `BlitzActif()` to be true. `BlitzActif` runs a
100-tick cycle (`Config::ScaleDiv(1)`-normalized, so a fixed ~5 real seconds regardless of FPS
setting): the hazard is active (lethal) only on even ticks within the first half of the cycle
(`num % 2 == 0 && num < 50`) — a rapid on/off flicker for the first ~2.5 s, giving a 25% duty cycle
over the full 5 s period — then fully inactive for the second half. Whenever the emitter tile
(304) sits above, a zap sound (`SoundChannel69`) additionally plays at six specific ticks in the
cycle (0, 7, 18, 25, 33, 44) to line the audio up with the visible strikes. On a lethal hit:
`BlupiDead(Clear1, nullopt)` (deterministic), `m_blupiRestart = true`, `m_blupiAir = true`, Y
snapped to the tile row, `SoundChannel8` (`Decor.cpp` ~5541). **No vehicle immunity** — same
lethality profile as lava; only Shield/Hide/SuperBlupi protect, and there is no focus requirement.

## Spring / bounce tile (`Decor::IsRessort`, `Decor.cpp` ~7312)

Detects icon 211 at Blupi's feet line (`pos.Y+60`). Only triggers when Blupi is grounded and in a
"normal" state (not swimming, surfing, suspended on a bar, or already airborne). If Blupi is
currently riding `Helico`/`Over`, `Jeep`, `Tank`, or on `Skate`, landing on a spring **forcibly
dismounts him first** (each mode individually cleared, with its own small screen-shake, an
`ObjectType9` particle burst, and `SoundChannel10`) before the bounce itself applies
(`Decor.cpp` ~2835–2911). The bounce sets `m_blupiAction = Air` and launches Blupi upward with a
velocity chosen from whether the Jump key is currently held and whether the `Power` state is
active: held+Power = -25, held+no-Power = -19, not-held+Power = -16, not-held+no-Power = -10
(negative = upward), then plays `SoundChannel41`. Landing specifically on a spring also suppresses
the normal generic landing-thud sound (`Decor.cpp` ~2984), since the bounce sound already covers it.

## Vanishing tile / `Temp` (`Decor::IsTemp`, `Decor.cpp` ~7323)

`IsTemp` itself (icon 324 at Blupi's feet line) is only consulted by the safe-checkpoint gate — it
is **not** a direct kill check. The actual gameplay consequence runs through the generic
passable/blocking classification (`IsPassIcon`/`IsBlocIcon` below): icon 324 is treated as solid
90% of the time and passable the other 10%, oscillating on a raw, un-scaled `m_time`-driven 20-value
cycle (`m_time / 4 % 20`) — passable only when the bucket is 18 or 19 (the last two of twenty).
This lines up exactly with `08-animations.md`'s `Temp` animation table (20 frames, including
"2 fully-transparent vanish frames") — the two transparent visual frames are precisely the two
frames during which the tile is walk-through, so Blupi simply **falls through** when it visually
vanishes rather than taking any damage. The cycle is driven purely by global `m_time` with no
per-cell phase offset, so every `Temp` tile in a level blinks in perfect lockstep.

## Bridge (`Decor::IsBridge`, `Decor.cpp` ~7334)

Checks two candidate cells for icon 364 — Blupi's feet line (`pos.Y+60`) first, then one tile
higher (`pos.Y`) — and returns whichever matches, along with its cell coordinates. It performs no
state change to Blupi himself: when it matches (and Blupi is focused), the caller spawns an
`ObjectType52` "bridge construction" object at that cell (`Decor.cpp` ~5608). `IsBridge` is a
trigger-tile detector only; the actual multi-step construction/degradation sequence that object
drives is out of scope for this document and would belong in a dedicated bridge-mechanics doc (not
yet written).

## Teleporter (`Decor::IsTeleporte` ~7378, `Decor::SearchTeleporte` ~7406)

`IsTeleporte` checks a cell one row above Blupi's feet (`pos.Y-60`) for one of four teleporter
icons, 330–333 (each icon value is effectively an independent color/network), but only in a narrow
7px-wide band at the left edge of the tile (`pos.X%64 <= 6`) — unlike most other hazard checks
here, this is not a wide or centered band, so a teleporter only triggers when Blupi is closely
aligned to the tile's left edge. Entering one (while
not already transported and focused) sets `m_blupiAction = Teleporte`, zeroes velocity, drops
focus, X-aligns to the tile, plays `SoundChannel71`, and spawns an `ObjectType92` entry-particle
effect (`Decor.cpp` ~5593–5607). Pairing is **implicit by shared icon value, not an explicit table**:
after a fixed 128-tick delay (`Config::ScaleTime(128)`, ≈6.4 s) of the `Teleporte` animation,
`SearchTeleporte` linearly scans the entire 100×100 map for the first other cell sharing the same
icon value that is more than 40 px away on either axis (to skip the entry cell itself) and relocates
Blupi there, spawning two `ObjectType27` arrival-particle bursts and restoring focus
(`Decor.cpp` ~6350–6361). If no matching second teleporter exists anywhere on the map, Blupi simply
regains control in place — a silent no-op. Because the scan is first-match, row-major order, a
third tile sharing the same icon would break the pairing; level data must keep teleporter icons in
matched pairs (matches the existing open-question note in `09-open-questions.md`).

## Jump physics — normal vs. reduced (`Decor::IsNormalJump`, `Decor.cpp` ~7432)

Not a hazard — this determines jump *strength*. From a point one tile above Blupi's center, offset
15 px toward his current facing direction, it probes two stacked tiles overhead: if both are
passable (`IsPassIcon`), the jump is "normal" and Blupi gets the higher boost (-16, or -26 with
`Power` active); if either probe tile is blocked (a low ceiling within two tiles above, to the
facing side), he gets a reduced "bumped head" boost instead (-12, or -16 with `Power`) — preventing
a full-height jump from clipping through a nearby ceiling (`Decor.cpp` ~2913–2946). Skating has its
own separate fixed values (-13/-17 with `Power`) that bypass this check entirely.

## Water depth state machine (`IsSurfWater` ~7462, `IsDeepWater` ~7477, `IsOutWater` ~7493)

`IsSurfWater` detects the water surface tile (icon 92) specifically at Blupi's lower body band
(bottom `BLUPISURF` = 12 px of his hitbox) with dry tile above; `IsDeepWater` detects either water
icon (91 or 92) fully at his position; `IsOutWater` detects a non-water, passable tile at his
center. These drive three body states: `Surf` (standing/floating at the surface), `Nage`
(fully submerged swimming), and neither (dry). Entering `Surf` or `Nage` from dry ground resets
velocity and plays a splash (`MoveObjectPlouf`); transitioning `Surf`→`Nage` (going under) starts a
breath gauge at 100 (`m_blupiLevel`, HUD gauge 0, blue) that ticks down by 1 every
`Config::ScaleTime(5)` ticks — a maximum submersion time of ~25 s (100×5 ticks at the normalized 20
FPS baseline) — turning the gauge red at level 25 and killing Blupi via `BlupiAction::Drown` at
level 0 (`Decor.cpp` ~4609–4629), unless Shield/Hide/SuperBlupi is active. Resurfacing (`Nage`→`Surf`)
plays `SoundChannel25` and hides the gauge. Pressing Jump while positioned at specific sub-tile
depths near the surface launches Blupi up and clear of the water (-16 with `Power`, else -12,
`SoundChannel22`, a "Tiplouf" splash-out effect). Riding any vehicle (`Helico`/`Over`/`Jeep`/`Tank`/
`Skate`) into `Surf` or `Deep` water forcibly dismounts it the same way a spring does (shake +
`ObjectType9` + `SoundChannel10`) — vehicles cannot enter water at all (`Decor.cpp` ~5415–5435).

## Fan tiles (`Decor::IsVentillo`, `Decor.cpp` ~7667)

Icons 126–137 form a fan: only the four **head** icons (126 left, 129 right, 132 up, 135 down) can
ever be lethal; the remaining icons in that range (127/128/130/131/133/134/136/137) are just the 4
head icons' own idle animation frames, always harmless (real `IsVentillo`'s `switch` falls through
to a no-op `break` for all of them) — **NOT the fan's air-column/trail tiles**, correcting this
doc's own earlier characterization (2026-07-11, re-verified directly against `Decor.cpp:7667-7752`
while implementing `plan.md` `E3D-MIG-149`, not just this doc's prior summary). Detection is
checked at Blupi's OWN current tile (not a tile in front of him), restricted to a narrow sub-tile
band matching the head's blow direction (e.g. icon 126/FanLeft: only the tile's left ~1/4,
`pos.X%64<=16`). When triggered, the effect is unconditional regardless of Blupi's invulnerability
state: it **consumes the fan** — clears the head tile itself immediately
(`ModifDecor(pos, -1)`), then walks further tiles one full 64px step at a time in the blow
direction, continuing ONLY while each next tile's icon exactly matches a specific real
trail-continuation value (110 for FanLeft, 114 for FanRight, 118 for FanUp, 122 for FanDown) —
these 4 values belong to a wholly separate, not-yet-render-decided "wind-vent particle stream" tile
family (icons 110-125, see `08-animations.md` §6's own "deferred" list), with no galaxy-eggbert
`BlockTypes` constant and never placed in any level this project has inspected yet. Passing through
a fan's blast removes that fan's visual column from the map for good — and always triggers an
`ObjectType11` particle burst plus a `BigShake` screen effect and `SoundChannel10`, even if Blupi
survives. Only if `m_blupiFocus` is true and Blupi is not Shielded/Hidden/SuperBlupi does it
additionally kill him via `BlupiDead(Clear1, Clear2)` (a 50/50 random pick between the two)
(`Decor.cpp` ~5459–5475). A shielded Blupi walking through a fan thus still pops it but survives.

## Passable vs. blocking classification (`IsPassIcon` ~7503, `IsBlocIcon` ~7522)

Brief note only — the per-icon passability table itself is `02-tiles.md`'s "Passable" column, not
re-derived here. Both functions test `Tables::table_decor_quart`, a 16-entry (4×4 sub-tile) solidity
mask per icon: `IsPassIcon` is true only if *all* 16 sub-cells are empty; `IsBlocIcon` is true only
if *all* 16 are solid. A tile with a mixed mask (e.g. a slope/partial shape) is neither purely
passable nor purely blocking under these two coarse helpers — the actual fine-grained collision
(`DecorDetect`, not covered here) tests sub-cells directly. Both functions special-case icon 324
(`Temp`) to route through the vanishing-tile timing described above instead of the static table.
