# Blupi Core Movement & Physics

**Status:** new for this initiative (`DOC-300`, 2026-07-05) — first prose behavioral-specification
document in this reference tree, per the user's explicit scoped approval (`CLAUDE.md`, 2026-07-05)
to describe mobile-eggbert's actual gameplay logic in prose, with key numeric constants, but **not**
pseudocode and **not** verbatim/line-by-line code transcription. This document covers Blupi's core
per-frame movement/physics/state machine, driven almost entirely by one function,
`Decor::BlupiStep()` (`Decor.cpp:2734-~6546`, ~3,800 lines) plus its direct helpers. It does not
re-catalog sprite frame tables (see `08-animations.md` §2), tile icons (`02-tiles.md`), object
pickups (`03-objects.md`), or sound triggers (`07-sounds.md`) — those are cross-referenced, not
duplicated. Per-type enemy/object movement is `04-enemy-behavior.md`'s scope, not this file's.

All tick-count and per-tick pixel values below are the game's **original 20 Hz logical tick rate**
(`Config::FPS = Fps::Fps20`, `Config.hpp:57`). At higher real frame rates the codebase rescales
every timer via `Config::ScaleTime()`/`ScaleDiv()` and every per-tick speed via `Config::SPEED_SCALE`
so real-world durations and speeds are unchanged (`Config.hpp:53-158`) — the numbers here are the
tick-domain values actually written in the source, not seconds or pixels-per-second.

## 1. Input model

Each frame, before `BlupiStep()` runs, the input layer (`InputPad.cpp`) reduces touch/keyboard/
accelerometer state to two channels and hands them to `Decor`:

- **Directional intent**, `-1.0`/`0.0`/`1.0` (or an analog accelerometer value on some platforms)
  per axis, pushed via `Decor::SetSpeedX()`/`SetSpeedY()` (`Decor.cpp:1400-1412`;
  `InputPad.cpp:1068-1105`). These become `m_blupiSpeedX`/`m_blupiSpeedY` — **intent**, not a pixel
  speed. `SetSpeedX()` negates the value first if `m_blupiInvert` is set (a temporary
  reversed-controls state; not investigated further here as it is outside movement/physics proper).
- **Discrete buttons**, a `KeyPressFlags` bitmask (`Jump=1`, `Fire=2`, `Down=4`,
  `KeyPressFlags.hpp:30-36`) pushed via `Decor::KeyChange()` into `m_keyPress`.

Every movement mode inside `BlupiStep()` converts `m_blupiSpeedX`/`Y` into an actual per-tick pixel
velocity (`m_blupiVitesseX`/`Y`, French "vitesse" = speed) using its own acceleration rate and its
own maximum-speed multiplier — there is no single shared "max speed" constant; see §7's per-mode
table. `Direction` (`Direction.hpp:26-31`) is just `None`/`Left`/`Right`, used for facing and
sprite mirroring, not a movement vector by itself.

## 2. Collision rectangle (`BlupiRect`) and ground/ceiling detection

`Decor::BlupiRect(pos)` (`Decor.cpp:2471-2532`) returns the axis-aligned box used for essentially
all of Blupi's collision — tile collision, ground detection, hazard detection — and its dimensions
change per active mode. Blupi's logical footprint lives inside a 60×60 unit box (not the 64×64 tile
size — `BLUPIOFFY = 4 + BLUPIFLOOR = 6`, `BLUPIFLOOR = 2`, `Decor.hpp:195-197`, giving a small
built-in margin against tile edges):

| Mode | X range (from `pos.X`) | Y range (from `pos.Y`) | Width × Height |
|---|---|---|---|
| Normal (default/else case) | `+12` .. `+48` | `+11` .. `+58` | 36 × 47 |
| Swim/Surf, standing (`Stop`) | `+12` .. `+48` | `+5` .. `+50` | 36 × 45 |
| Swim/Surf, moving | `+12` .. `+48` | `+15` .. `+50` | 36 × 35 |
| Jeep | `+2` .. `+58` | `+10` .. `+58` | 56 × 48 |
| Tank | `+2` .. `+58` | `+10` .. `+58` | 56 × 48 |
| Overcraft (`m_blupiOver`) | `+2` .. `+58` | `+2` .. `+58` | 56 × 56 |
| Balloon | `+10` .. `+50` | `+5` .. `+58` | 40 × 53 |
| Ecrase (crushed) | `+5` .. `+55` | `+39` .. `+58` | 50 × 19 |

The crushed ("Ecrase") box is a deliberate 19-unit-tall pancake, matching its animation; the
overcraft box is nearly the full 60-unit tile in both axes.

`Decor::BlupiIsGround()` (`Decor.cpp:2459-2469`) tests a 1-pixel-tall band 2 units below the
collision box's bottom edge (`Top = pos.Y+58`, `Bottom = pos.Y+59`) against the tile grid — but only
when Blupi isn't riding a moving platform (`m_blupiTransport == -1`); while riding one it always
returns `false` (ground state is meaningless — the platform IS the ground reference).

Inside `BlupiStep()` the same idea recurs as two per-tick flags computed against the proposed
destination `end` (`Decor.cpp:2806-2822`): **`flag2`** ("ground gone") probes a 1px band at the
box's bottom edge and is `true` when nothing solid is there (about to fall) — again forced `false`
while on a transport; **`flag3`** ("ceiling hit") probes a 10px band near the top of the box
(`Top = end.Y+10`, `Bottom = end.Y+20`) and is `true` when something solid is overhead. These two
flags are what every fall/jump/landing decision below is keyed on.

Before ground/ceiling are even checked, if Blupi is standing on an air-vent tile (icon 110/114/118/
122, or generally 110-125) the vent nudges `end` sideways or vertically by a fixed 9 or 20 pixels
(`Decor.cpp:2771-2805`) — left-vent 110 push left 9px, right-vent 114 push right 9px, up-vent 118
push up 20px, down-vent 122 push down 20px — before the rest of the frame's physics runs.

## 3. Penetration recovery (`BlupiAdjust`) and horizontal blocking (`BlupiBloque`)

`Decor::BlupiAdjust()` (`Decor.cpp:2544-2617`) runs at the very start of every `BlupiStep()` call
(and again after the frame's move is committed) to push Blupi back out of any tile he's currently
overlapping. If the current box is already clear it returns immediately. Otherwise it resolves the
overlap in up to five bounded phases, each a loop of at most 50 two-pixel steps (so worst-case ~100px
of correction per phase): push down first (probing a thin top edge), then push right, then push
left, then two more horizontal-only passes probing just the leading vertical edge. Each phase stops
the moment its probed edge clears. Very deep penetration (deeper than the 50-step cap allows) is not
guaranteed to fully resolve — this cap is original tuning, not a bug to "fix" in a port.

`Decor::BlupiBloque(pos, dir)` (`Decor.cpp:2619-2637`) is the narrower per-tick horizontal-blocking
check most movement modes use to zero out `m_blupiVitesseX` when they'd walk into a wall: it tests
only a 2-pixel-wide column on the leading edge (right edge if `dir > 0`, left edge if `dir < 0`),
restricted to the lower 20 pixels of the box (foot-level), not the whole body height.

## 4. Jump, gravity, and landing (ground `Air`/`Jump` states)

Three distinct sources put Blupi into the air, each with its own initial vertical speed
(`m_blupiVitesseY`, negative = upward):

- **Walking off a ledge** (`flag2` true while grounded and not in any vehicle/swim/suspend mode,
  `Decor.cpp:2823-2834`): starts a plain fall at `m_blupiVitesseY = 1.0`, no boost.
- **Spring tile** (`Decor::IsRessort`, tile icon 211 — `Decor.cpp:7312-7321`, cross-ref
  `02-tiles.md`'s `Spring` entry): stepping onto one while grounded (not swimming/surfing/suspended/
  already airborne) launches Blupi upward at `-25` (Jump held + Power) / `-19` (Jump held, no Power)
  / `-16` (Power, Jump not held) / `-10` (neither) (`Decor.cpp:2900-2907`). Landing on a spring
  while riding the **helicopter, overcraft, jeep, tank, or skateboard** ejects Blupi from that
  vehicle outright (spawns a small "poof" effect, `ObjectType9`, and does **not** re-drop the vehicle
  pickup into the world the way a deliberate dismount does — `Decor.cpp:2837-2892`). Balloon and
  Ecrase are not cancelled by this check; whether a spring bounce is actually visible while
  ballooning/crushed (given those modes recompute `m_blupiVitesseY` themselves later in the same
  frame) was not traced further — flagged as uncertain, not asserted.
- **Direct jump input**: pressing Jump while grounded starts a short `Jump` wind-up animation; once
  its phase reaches tick 3 it launches into `Air` at `-17`/`-13` (Power/no Power) if skateboarding,
  else `-26`/`-16` (Power/no Power) if `Decor::IsNormalJump()` is true, else `-16`/`-12` (restricted
  jump) (`Decor.cpp:2913-2947`). `IsNormalJump()` (`Decor.cpp:7432-7460`) checks for two full
  tiles of clear headroom (offset slightly toward the facing direction) — a low ceiling silently
  caps jump height rather than blocking the jump entirely.

`m_blupiPower` is the Lollipop power-up (`BlupiAction::Sucette` completing after a 32-tick drink
animation, `Decor.cpp:3212-3223`) — while active it raises both the direct-jump and the
spring-bounce initial speed, as shown above.

**Gravity**, applied only while `m_blupiAir` is set (`Decor.cpp:2948-3041`): `m_blupiVitesseY`
increases by `2.0` every tick while it is still below `20.0` (`Decor.cpp:2968`/`2976`), while the
actual per-tick vertical displacement is **twice** that velocity value (`end.Y += vitesseY * 2.0`).
**Correction (2026-07-11, re-verified directly against source while researching real fall-death
timing)**: the check is `if (vitesseY < 20.0) vitesseY += 2.0`, tested BEFORE incrementing — since
`m_blupiVitesseY` starts at `1.0` and only ever takes odd values (`1, 3, 5, ..., 19`), the last
qualifying increment happens at `19.0` (still `< 20.0`), landing on `21.0`, which then fails the
`< 20.0` test and stops. **Real terminal velocity is therefore `21.0`, not `20.0`** — the velocity
never actually lands on the number the check compares against, an easy-to-miss off-by-one in the
increment logic. At the real 20Hz reference rate this is `21.0 * 2.0 * 20 = 840` real px/sec
(13.125 tiles/sec). Hitting a ceiling while still ascending (`flag3` true and
`vitesseY < 0.0`) stops the ascent (`vitesseY` reset to `1.0`); if the upward speed exceeded `14.0`
in magnitude, it additionally sets a "hurt" flag (`m_blupiJumpAie`) and plays a head-bump sound
instead of the plain one (`Decor.cpp:2950-2963`). Landing (`vitesseY >= 0.0` and ground detected)
clears `m_blupiAir`, transitions to `BlupiAction::StopJump`, and — if the hurt flag was set on the
way up — instead plays a 30-tick `JumpAie` hurt-landing animation before returning to `Stop`
(`Decor.cpp:3002-3047`). Note: the landing code branches on `vitesseY > 20.0` vs not, but both
branches assign the identical `StopJump` action (`Decor.cpp:2990-2997` and `3022-3029`) — no
observable behavioral difference was found; likely a vestige of a removed hard-landing variant, not
something to reproduce as a real distinction.

While falling, a moving-elevator platform can also catch Blupi mid-air: `AscenseurDetect()` is
probed near the box's feet, and if found while still moving downward, Blupi attaches to it
(`m_blupiTransport = icon`) instead of landing on static ground (`Decor.cpp:3009-3040`,
`4209-4222` for the Jeep/Tank equivalent).

## 5. Turning

Reversing horizontal direction (`BlupiAction::Turn`) plays a brief pivot animation whose duration
depends on the active mode before movement resumes (`Decor.cpp:3472-3601`):

| Mode | Turn duration (ticks) |
|---|---|
| Normal ground | 6 |
| Mid-air (`TurnAir`) | 6 |
| Helicopter | 10 |
| Overcraft | 7 |
| Jeep | 7 |
| Tank | 12 |
| Skateboard | 14 |
| Swim / Surf | 10 |
| Suspended (hanging) | 10 |

## 6. Vehicle and special movement modes

Each mode below is its own gated block in `BlupiStep()`, active only while its own boolean flag
(`m_blupiHelico`, `m_blupiOver`, etc.) is set. All convert `m_blupiSpeedX`/`Y` into
`m_blupiVitesseX`/`Y` using a per-mode target-speed multiplier and acceleration rate; a neutral
input (`speedX == 0`) always decelerates back toward zero, never instantly.

| Mode | Horizontal max speed | Horizontal accel/decel | Vertical behavior | Source |
|---|---|---|---|---|
| Helicopter, "easy move" setting | ±12 px/tick | 0.5 accel, 2.0 decel | ascend to −7, descend to 8, accel 0.5; idle sinks toward +1 at 1.0/tick | `Decor.cpp:3694-3917` |
| Helicopter, normal setting | ±16 px/tick | 1.0 accel, 2.0 decel | ascend to −10, descend to 12, accel 0.5 | `Decor.cpp:3694-3917` |
| Overcraft | ±12 px/tick | 1.0 accel/decel — **inverted from intuition**: accelerates toward the target speed only while `flag2` ("ground gone") is true, i.e. while hovering over a gap/no solid ground within reach; whenever solid ground is immediately underfoot (`flag2` false) or input is neutral, it decelerates to 0 at 1.0/tick instead | ascend to −5, gated on **not** having 80px of clear space below (`OVERHEIGHT = 80`, probed in a band *below* the box, not above — acts as a hover-altitude cap: ascend is only allowed while still close to solid ground/a platform underneath; once 80px of clearance opens up below, further ascend input is ignored), descend to 12 at +5/tick burst, else +1 every 2 ticks | `Decor.cpp:3918-4038` |
| Balloon | ±10 px/tick | 1.0 accel, 2.0 decel | every 6 ticks: ascend to −5, descend to 0, else idle **buoyancy** drifts upward to −3 (a balloon left alone slowly rises) | `Decor.cpp:4039-4108` |
| Ecrase (crushed) | grounded ×4, airborne ×7 of `m_blupiSpeedX` | 1.0 accel, 2.0 decel | airborne: +1.0/tick capped at 2.0 (slow, heavy fall); grounded: pinned to 0 | `Decor.cpp:4109-4176` |
| Jeep | ±20 px/tick, ±20 forced while cresting a slope edge | 1.0 accel, 2.0 decel | airborne +5.0/tick capped 50 (heavy); grounded pinned to 0; body tilt eases toward ±45° at 5°/tick when one side is over an edge (`Misc::Approach`) | `Decor.cpp:4177-4308` |
| Tank | ±12 px/tick | 1.0 accel, 3.0 decel | same as Jeep (+5/tick capped 50 airborne, 0 grounded) | `Decor.cpp:4309-4451` |
| Skateboard | ±15 px/tick | 1.0 accel, 1.0 decel (holds momentum longer than other modes) | none of its own — uses the shared ground gravity/`Air` path | `Decor.cpp:4452-4509` |
| Swim (Nage) | ±8 px/tick (further scaled per stroke phase via `Tables::table_vitesse_nage`, role only — see `08-animations.md`) | 1.0 accel, 2.0 decel | ascend/descend to ∓5 at 1.0/tick; idle drifts to −1 (`Stop` action) or 0 | `Decor.cpp:4542-4654` |
| Surf | ±8 px/tick (scaled per phase via `Tables::table_vitesse_surf`) | 1.0 accel, 2.0 decel | ascend/descend to ∓5 at 1.0/tick; idle centers on −2 (rides slightly above the water line); Y is additionally snapped toward the 64px tile grid using a `BLUPISURF = 12` offset | `Decor.cpp:4656-4730` |
| Suspended (hanging on a bar) | ±5 px/tick, applied directly (no ramp) | — | vertical is bar-relative, not physics — see below | `Decor.cpp:4732-4790` |

Both Helicopter rows above live in the same single `if (m_blupiHelico...)` block (`Decor.cpp:3694-3917`) —
`Def::getEasyMoveProperty()` branches select easy-move vs. normal values inline for both the vertical and
horizontal sub-sections; they are not two separate contiguous code regions.

Dismounting a vehicle via the action button re-spawns its pickup object into the world at Blupi's
position (helicopter → `ObjectType13`, overcraft → `ObjectType46`, jeep → `ObjectType19`, tank →
`ObjectType28`) so it can be picked up again; the skateboard instead plays a short `DeposeSkate`
animation (12-tick delay before the object reappears, 20-tick total) and only allows dismounting
below `m_blupiVitesseX < 8.0` (`Decor.cpp:4499-4509`, `4527-4540`). Note that one of these IDs
(`ObjectType46`) contradicts `03-objects.md`'s current classification of that same ID (listed there
as a balloon pickup) — confirmed via direct code reading to be a real discrepancy, see "Open /
uncertain points" below for detail; flagged as a cross-file fix for `03-objects.md`, not made here.

**Suspended/hanging mode** moves differently from every other mode: horizontal input is applied
directly as `speedX * 5` with no acceleration ramp (immediate), gated on the `March` action
(`Decor.cpp:4734-4743`). `Decor::GetTypeBarre()` classifies what kind of hand-hold is under Blupi:
type `2` means the end of the bar/rope, ending the hang if the landing spot below is clear; type `0`
(nothing) or moving down for more than 5 ticks drops Blupi into free-fall (`Air`), plays a relief
("Ouf5") reaction, and sets a 5-tick grace timer (`m_blupiNoBarre`) preventing an immediate re-grab
(`Decor.cpp:4744-4768`). Pressing Jump while hanging starts a `Jump`-action wind-up; once its phase
reaches a 10-tick timer, Blupi lets go and hops off the bar — transitions to `Air` with
`m_blupiVitesseY = -11.0` (a fixed upward launch, not further climbing up the bar), sets the same
5-tick `m_blupiNoBarre` re-grab grace timer, and plays a sound (`Decor.cpp:4769-4787`). Grabbing a
bar from the ground requires a clear landing spot at the snapped position (`BLUPISUSPEND = 12`
offset, `Decor.cpp:5437-5453`).

## 7. Ghost mode (`BlupiGhostStep`) — debug/dev-only, not original gameplay

`Decor::BlupiGhostStep()` (`Decor.cpp:2640-2707`) is compiled only under `#ifdef MODERN` and only
runs when `m_blupiGhost` is set, which is itself only reachable via the `Ghost` cheat code
(`Tables::CheatCodes::Ghost`, also `#ifdef MODERN`, `Tables.hpp:107`). It is free-flight with no
gravity and no collision response: input directly sets a velocity (scaled ×4 from
`m_blupiSpeedX`/`Y`), the only constraint applied is clamping the resulting position to the level's
outer bounds (`0` to `6400 - 64`, i.e. the 100×100-tile, 64px-per-tile world), and the camera-scroll
easing is a simplified copy of the normal scroll logic. This is a debug/dev overlay feature added in
the mobile-eggbert C++ port, not a mechanic present in the original 2013 XNA game — do not treat it
as a faithful-remake target for galaxy-eggbert without separately confirming that with the user, per
`CLAUDE.md`'s "faithful remake" rule.

## 8. Hazard contact and death (`BlupiDead`)

Most hazards call `Decor::BlupiDead(action1, action2)` (`Decor.cpp:6547-6614`) directly and
unconditionally kill Blupi (subject to `m_blupiShield`/`m_blupiHide`/Super-Blupi immunity checks at
each call site) rather than modifying his state first:

| Hazard check | Death action(s) | Notes |
|---|---|---|
| `IsLave` (lava) | `Clear3` | Launches Blupi 2000px straight up over 40 ticks as the death "voyage" (`Decor.cpp:6598-6606`) |
| `IsScie` (saw) | `Clear4` | Spawns 3 flying debris objects (`ObjectType41`) at offsets — Blupi is visibly cut apart (`Decor.cpp:6607-6613`) |
| `IsBlitz` (lightning/electric hazard) | `Clear1` | Distinct from `BlupiElectro` below — this is the hazard that hurts Blupi, not his own power |
| `IsVentillo` (fan/blower) | `Clear1` or `Clear2` (random 50/50) | |
| `IsPiege` (trap) / `IsGoutte` (drip) | `Glu` ("stuck/glue") | |
| Walking off the world's bottom row (`(end.Y+30)/64 >= 99`, i.e. `MAXCELY = 100` tiles, `Def.hpp:150`) | `Clear2` | Checked before the rest of the frame runs at all (`Decor.cpp:2754-2762`) |

When `BlupiDead()` is given two candidate actions, it picks between them with a coin flip
(`m_random`, `Decor.cpp:6555-6557`) purely for visual variety — not a difference in cause. It also
unconditionally cancels every mode/power-up flag (helicopter, overcraft, jeep, tank, skateboard,
swim, surf, suspend, shield, power, cloud, hide, balloon, ecrase, and ghost) and stops all vehicle
motor sounds, regardless of which hazard triggered it.

**`IsEcraseur`** (crusher) is the one hazard-adjacent check that does **not** call `BlupiDead` — it
instead forces Blupi into the crushed `Ecrase` state (§6 above), a survivable state transformation,
not an instant death (`Decor.cpp:5549-5592`). Drowning (swim oxygen reaching 0, §6's Swim row)
similarly routes through a dedicated `Drown` action rather than the `Clear*` family, but is otherwise
handled like any other death for respawn purposes.

Each death animation plays for a fixed number of ticks before respawn logic runs — `Clear1`: 70,
`Clear2`: 100, `Clear3`: 70, `Clear4`: 110, `Clear5-Clear8`: 90 each, `Drown`: 90, `Glu`: 100,
`Electro`: 90 (`Decor.cpp:6374-6377`). At that point, if lives remain (`m_nbVies > 0`), Blupi is
hidden, repositioned to `m_blupiValidPos` (only if the specific death set `m_blupiRestart`), and a
brief camera pan to the lives-counter HUD icon plays (`Decor.cpp:6379-6392`). If no lives remain,
the level/mission is terminated outright (`m_term = -1`, `DoorsLost()`, `Decor.cpp:6393-6398`).
Falling more than 1000px past the level's bottom edge is fatal to the whole attempt unconditionally,
bypassing the lives system entirely (`Decor.cpp:6406-6410`).

**Respawn position** (`m_blupiValidPos`) is not simply "last position" — it is refreshed from a
10-slot position history FIFO (`Decor::BlupiAddFifo`, `Decor.cpp:6654-6673`), and only while Blupi
is in a broad "safe" state: focused, not airborne (unless helicopter/overcraft and grounded), not
ballooning/crushed/shielded/hidden, not teetering at a ledge, not riding a transport, and not
currently touching any hazard tile (lava, trap, drip, saw, bridge gap, teleporter, lightning, a
blinking/temporary tile (`IsTemp`, icon 324), or in the path of a projectile/moving object)
(`Decor.cpp:6467-6478`). The FIFO's **oldest** entry is
used as the safe position, giving roughly a half-second buffer so Blupi doesn't respawn exactly where
he died.

## 9. Electric aura (`BlupiElectro`) — Blupi's own power, not a hazard to Blupi

Despite the name's similarity to the lightning hazard (`IsBlitz`, §8), `Decor::BlupiElectro(pos)`
(`Decor.cpp:9610-9638`) is unrelated to Blupi taking damage. It only returns `true` when
`m_blupiCloud` (the Power-Charge aura state) is active and a given position — expanded by 40px
around Blupi's own box — overlaps that position. Its only caller (`Decor.cpp:7976-7984`) uses it
to instantly destroy small enemies (`ObjectType4`/`32`/`33`) that wander into Blupi's aura while
`m_blupiCloud` is active — it is an offensive aura Blupi carries, not damage Blupi can take.
`m_blupiCloud` is granted by the `PowerCharge` pickup (`ObjectType31`) via a `Charge` drink animation
(`Decor.cpp:6069-6087`, `3048-3057`).

## 10. Power-up timers

`m_blupiShield`, `m_blupiPower` (Lollipop), `m_blupiCloud` (Power-Charge), and `m_blupiHide` share one
countdown, `m_blupiTimeShield`, always initialized to `100` on pickup but decremented at different
rates so each power effectively lasts a different real-world duration: Shield every 5 ticks (500
ticks total), Power every 3 ticks (300 ticks), Cloud and Hide every 4 ticks (400 ticks each)
(`Decor.cpp:5071-5134`). Each plays a distinct "about to expire" warning sound a fixed number of
units before running out (Shield at 10 remaining, Power at 20, Cloud at 25, Hide at 20).

## 11. Sprite/animation selection (`BlupiSearchIcon`) — role only

`Decor::BlupiSearchIcon()` (`Decor.cpp:2123-~2459`) is called at the end of every `BlupiStep()`
pass to pick the sprite frame and channel for the current state. Per the function's own header
comment, it works in three stages: (1) remap the base `BlupiAction` into a mode-specific variant for
whichever vehicle/state flag is active (helicopter/overcraft/jeep/tank/skate/swim/surf/suspend/
balloon/ecrase/vent), also computing rotation values for the tilting modes; (2) scan
`Tables::table_blupi` (a flat `{actionId, frameCount, holdFrame, icon...}` record list) for the
matching action and index into it by the phase counter; (3) mirror left-facing icons through
`Tables::table_mirror` and swap to the `Element` sprite channel for certain effect actions. It also
triggers ambient idle sounds and surf-splash particles at specific phase values as a side effect.
The full 87-state `BlupiAction` catalog and every animation's frame table are already documented in
`08-animations.md` §2 — this section exists only to explain *what selects* those frames, not to
re-list them.

## Open / uncertain points

- The spring-bounce interaction with Balloon/Ecrase mode (§4) was not traced to a definitive
  behavioral conclusion — flagged, not resolved.
- The apparent duplicate `m_blupiCloud = true` assignment (immediate on pickup vs. again after the
  64-tick `Charge` animation completes, §9) reads as redundant but was not investigated further —
  documented as observed, not explained.
- `ObjectType46`'s in-code behavior (grants Overcraft, confirmed at `Decor.cpp:5668-5675` — picking
  it up sets `m_blupiOver = true`, the same flag the `CheatCodes::Overcraft` cheat sets directly at
  `Decor.cpp:2021-2022`) versus `03-objects.md`'s existing classification of that ID (listed as a
  balloon pickup) is a confirmed real discrepancy — `03-objects.md`'s "balloon" label traces back to
  mobile-eggbert's own `ObjectType.hpp:126` doc comment, which is itself self-contradictory ("Balloon
  vehicle pick-up... boarding sets `m_blupiOver`"); the runtime behavior is unambiguously Overcraft,
  not Balloon. Left for `03-objects.md` to reconcile, not fixed here.
- `Tables::table_vitesse_march`/`table_vitesse_nage`/`table_vitesse_surf`'s actual array contents are
  intentionally not transcribed here (per `CLAUDE.md`'s restriction on copying `Tables.cpp` data
  verbatim) — only their role (a per-animation-phase cadence multiplier) is described.
