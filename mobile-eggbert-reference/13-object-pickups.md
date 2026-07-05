# Object Pickups & Power-Ups — Gameplay Behavior

**Status:** `DOC-304` (2026-07-05), written under explicit user approval recorded in `CLAUDE.md`
(2026-07-05) to go beyond `03-objects.md`'s one-line-per-`ObjectType` catalog into actual pickup
mechanics: trigger condition, state mutation, numeric constants, sound. Prose + verified numeric
constants only — no pseudocode, no verbatim transcription of `Decor.cpp`.

**Scope:** collectibles/power-ups only — `ObjectType` 5, 6, 7, 13, 19, 21, 24, 25, 26, 28, 29, 30,
31, 39, 40, 46, 49, 50, 51, 55 (pickup only). Enemies are `04-enemy-behavior.md`. Crates, platform
lifts, and bridges are `14-crates-lifts-bridges-effects.md` (a sibling doc, not part of this file).
Door/key mechanics beyond the raw key pickup are already fully documented in `06-doors.md` and are
only cross-referenced here, not repeated. All line numbers are `../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/Decor.cpp`, current as of this pass — re-verify if the file changes.

## Shared mechanics

### Contact detection (`Decor::MoveObjectDetect`, ~9689-9771)

Every pickup in this doc is detected the same way, once per tick, inside `Decor::BlupiStep`
(~5492: `int num26 = MoveObjectDetect(m_blupiPos, bNear)`, then `icon = num26` reused through
~6199). `MoveObjectDetect` intersects Blupi's inner-body rectangle against a per-object rectangle.
Left/right are narrowed inside `MoveObjectDetect` itself to `pos.X+16`/`pos.X+60-16`, but top/bottom
are **not** overridden there — they come straight from `Decor::BlupiRect(pos)` (~2471-2532), so the
vertical extent is state-dependent, not the tile's full 0..60 height: `pos.Y+11..pos.Y+58` while
walking/idle (the common case), `pos.Y+10..pos.Y+58` on jeep/tank, `pos.Y+2..pos.Y+58` riding the
balloon vehicle (`m_blupiOver`), and narrower bands while swimming/surfing/inflated/squashed. None
of the 20 pickup types has its own special-case rectangle in that function (unlike enemies/crates,
which do) — they all use the default: `left = posCurrent.X+16, right = posCurrent.X+44, top =
posCurrent.Y+36, bottom = posCurrent.Y+60` (the bottom 24px band of the tile, inset 16px
horizontally). A real rectangle
overlap sets `bNear = true` and returns that object's index; this is a genuine contact test, not a
proximity/distance check (the one exception in the function, `ObjectType2`'s wider `bNear=false`
detection, is an enemy "notice" radius, unrelated to pickups). **Every pickup block described below
is gated on `icon != -1 && bNear`** (confirmed structurally: the whole ~5646-6199 range is one
`if (icon != -1 && bNear) { … } else { m_goalPhase = 0; }`), i.e. all of them require this rectangle
contact — none are pure proximity pickups.

`Decor::MoveChargeDetect` (~9803) and `Decor::MovePersoDetect` (~9835) are separate, narrower
rectangle tests used elsewhere — not on the pickup path. `MoveChargeDetect` is called once (~4834)
as an obstruction check so Blupi can't drop a persona decoy (`ObjectType200`) on a tile currently
occupied by a Charge/Cloud pickup (`ObjectType31`). `MovePersoDetect` is called from
`Decor::MoveObjectStep` (~7959) so the bulldozer/`blupih`/`blupit` enemies detect collision with a
placed persona decoy. Neither is itself a pickup-trigger path for the player.

### The "Voyage" pattern — HUD fly-then-apply (`Decor::VoyageInit`/`VoyageStep`, ~10158-10308)

Several pickups (treasure, extra-life egg, all 3 keys, dynamite) do not mutate game state at the
moment of contact. Instead the world object is deleted immediately, and a "voyage" animation is
started: an icon flies from the pickup's screen position to a fixed HUD slot over
`Config::ScaleTime((|Δx|+|Δy|)/10)` ticks (real elapsed time depends on `Config::ScaleTime`'s scale
factor, per the same open point noted in `06-doors.md`/`08-animations.md`). **The actual reward
(treasure++, life++, key-flag OR'd in, dynamite++) is only applied when the flight completes**, in
`VoyageStep` (~10254-10305), not at the moment of contact. Only one voyage can be in flight at a
time — starting a new one force-completes (and applies the reward of) any voyage already in
progress (~10160-10164). Some pickups (bullet pack, ammo) instead mutate state immediately at
contact and use the voyage purely as a cosmetic fly-and-chime with no follow-up effect — called out
per-type below.

### Buff timer sharing and mutual exclusion

Shield, Power (suction-cup), Cloud (charge), Hide (drink), and Invert (mirror) all share one
countdown field, `m_blupiTimeShield`, initialized to **100** on activation. Each buff decrements it
at its own real-time rate inside `Decor::BlupiStep` (~5071-5164): shield every `ScaleTime(5)` ticks,
power/invert every `ScaleTime(3)`, cloud/hide every `ScaleTime(4)` — so although every buff starts
at the same count of 100, their real durations differ (shield lasts longest, power/invert shortest).
An "about to expire" warning sound plays once at a fixed remaining-count threshold for four of the
five (shield at 10, power at 20, cloud at 25, hide at 20 — channels 43/45/56/63 respectively,
already in `07-sounds.md`); invert has no warning stage, it just ends. Because they share one
field, only one of these five can be active at a time in practice — but the pickup guard conditions
that enforce this are **not symmetric** (verified per-type below): Shield's guard excludes Hide/Power
but *not* Cloud, and picking it up silently cancels an active Cloud. Suction-cup and Drink guard
against Shield/vehicles and (for Drink) Cloud, but **not against their own buff** — grabbing a
second Suction-cup while Power is already active just restarts its 100-tick timer, same for
Drink/Hide. Charge/Cloud is the one exception that **does** guard against itself (and against all
four other buffs) — it cannot be picked up again until the current Cloud expires. Mirror/Invert's
guard is the loosest of all: only `!m_blupiHide` — it can be picked up while Shield, Power, or Cloud
are active, or even while riding a vehicle.

## Treasure — `ObjectType5` (~5948-5961, 10183-10192, 10270-10275)

On contact: the world object is deleted and 4 sparkle particles (`ObjectType39`, see below) burst
outward from the pickup point. A voyage starts toward the fixed HUD treasure-counter slot
`(430,430)`, with icon `6`/channel `Element`. `m_nbTresor` is only incremented when that voyage
completes (~10272), which also re-scans for now-unlockable treasure-gated doors
(`OpenDoorsTresor()` — full mechanic in `06-doors.md`). The pickup sound is chosen at voyage
*start*, not completion: channel 19 (final-treasure fanfare) if this is the last treasure needed
(`m_nbTresor == m_totalTresor - 1` at pickup time), else channel 11 (generic fanfare) — both already
documented in `07-sounds.md`.

## Extra-life egg — `ObjectType6` (~6007-6013, 10262-10269)

Gated on `m_nbVies < MAX_EGG_COUNT` (`MAX_EGG_COUNT = 10`, `Decor.cpp` line 96) — if lives are
already at the cap of 10, touching an egg does nothing at all (the object is not even deleted).
Otherwise it's deleted and a voyage starts toward a HUD slot that **shifts with life count**:
`VoyageGetPosVie(m_nbVies+1)` returns `(210 + 16*n, 417)`, so successive eggs fly to progressively
further-right HUD positions. `m_nbVies++` happens only on voyage completion (capped again at 10 as
a safety net), with channel 3's generic pickup chime.

## Level-exit goal / secret-level exit — `ObjectType7` / `ObjectType21` (~6158-6199)

Both types share identical contact logic, debounced by `m_goalPhase`: on first contact
(`m_goalPhase == 0`), if `m_nbTresor >= m_totalTresor`, all 4 vehicle-motor loop sounds are stopped,
channel 14 (level-complete fanfare) plays, and Blupi enters `BlupiAction::Win` (focus disabled, Y
snapped to the goal's Y) — this is the actual level-completion trigger. If not enough treasure yet,
channel 13 (rejection sound) plays instead and Blupi simply keeps standing there. Either way
`m_goalPhase` is set to 50 and decrements each subsequent tick of continued contact, so the
check/sound only re-fires every 50 ticks while still touching, not every tick. The **only**
difference between the two types: touching `ObjectType21` additionally sets `m_bFoundCle = true`
(the "found the hidden exit" flag `06-doors.md` describes as gating `OpenGoldsWin` vs. `OpenDoorsWin`
on level completion) — `ObjectType7` does not set it.

## Vehicle mounts — helicopter (`13`), jeep (`19`), tank (`28`), skateboard (`24`), balloon (`46`)

All five share the same shape of guard: blocked only while already riding *any* other vehicle
(helicopter/over/balloon/ecrase/jeep/tank/skate) or while swimming/surfing/suspended — **none of them
check Shield or Power**, so a vehicle can be mounted while those buffs are active. All (except the
floating-object case below) require the action button to be held/pressed at the moment of contact —
this is *not* an automatic walk-over pickup like treasure or the ammo pack. On mount: the world
object is deleted, the corresponding ride flag (`m_blupiHelico`/`m_blupiJeep`/`m_blupiTank`/
`m_blupiSkate`/`m_blupiOver` for balloon) is set true, horizontal velocity is zeroed, and if Cloud or
Hide was active it is silently cancelled (`m_jauges[1].SetHide(true)`) — Shield/Power are left
untouched. Helicopter (`13`) has one extra auto-mount path: `Decor::IsFloatingObject(icon)`
(~10350) checks whether the tile directly beneath the helicopter pickup is passable (i.e. it's
sitting over open space/water rather than resting on solid ground); if so it can be grabbed by
mere contact, no button press required. None of the five has a fixed duration — all persist until a
voluntary dismount (button press while riding, not detailed here) or until destroyed: landing on a
spring/bounce hazard (`Decor::IsRessort`, ~2835-2893) forcibly dismounts whichever vehicle is
active, spawning a small explosion and playing channel 10 (already documented in `07-sounds.md`) —
but only when `!m_blupiShield && !m_blupiHide && !m_bSuperBlupi`; while Shield or Hide is active, or
in SuperBlupi mode, the vehicle survives the spring hit instead of being destroyed. The
ride's own movement/firing mechanics are vehicle-behavior, out of this pickup doc's scope.

## Bullet pack — `ObjectType29` (~5731-5744)

Automatic on contact, no button needed. Gated on `m_blupiBullet < 10` — if ammo is already at the
cap, walking over the pack does nothing (object stays in the world). Otherwise the object is
deleted, `m_blupiBullet` is topped up to exactly 10 (`+= 10` then clamped), immediately at contact —
**not** deferred to voyage completion. A voyage still plays (icon 177 to the fixed HUD ammo slot
`(570,430)`, channel 54 fanfare from `07-sounds.md`) but it's purely cosmetic; `VoyageStep`'s icon-177
branch (~10301-10304) only replays channel 3, it doesn't touch `m_blupiBullet` again.

## Shield — `ObjectType25` (~6014-6024)

Guard: `!m_blupiShield && !m_blupiHide && !m_blupiPower` — notably **not** `!m_blupiCloud`. Automatic
on contact, no button. Sets `m_blupiShield = true`, and unconditionally clears `m_blupiPower` and
`m_blupiCloud` (so picking up a Shield while Cloud is active silently cancels the Cloud, since
Cloud isn't excluded by the guard). `m_blupiTimeShield = 100` (decrements every `ScaleTime(5)` ticks
— the slowest of the five shared-timer buffs, so Shield's real-time duration is the longest).
Channel 42 plays on activation, channel 43 at 10 ticks remaining (warning), per `07-sounds.md`.

## Suction-cup ("Sucette") — `ObjectType26` (~6025-6039, 3212-3223)

Requires the action button held at contact (not automatic). Guard excludes Shield and all vehicle
rides, but **not** Power/Hide/Cloud — grabbing one while Power is already active just restarts its
timer. On button-press: object deleted, `BlupiAction::Sucette` starts (a 32-tick, `ScaleTime(32)`,
channeling animation; player control/`m_blupiFocus` disabled during it), channel 50 plays
immediately. When the animation reaches phase 32, `m_blupiPower = true`, `m_blupiTimeShield = 100`
(decrements every `ScaleTime(3)` tick), channel 44 plays, and focus returns. So there are two
distinct sounds: 50 at grab, 44 when the buff actually activates ~32 ticks later.

## Drink — `ObjectType30` (~6053-6068, 3224-3235)

Same two-stage shape as Suction-cup. Requires the action button. Guard excludes Shield and Cloud
(and vehicles), but **not** Power/Hide — a second Drink while Hide is already active just restarts
the timer. Button-press starts `BlupiAction::Drink` (36-tick, `ScaleTime(36)`, channeling animation,
focus disabled), channel 57 plays at grab. At phase 36, `m_blupiHide = true`, `m_blupiTimeShield =
100` (decrements every `ScaleTime(4)` tick), channel 62 plays, focus returns.

## Charge/Cloud — `ObjectType31` (~6069-6087, 3048-3057)

The one pickup among this trio that is **automatic on contact** (no button) — and the only one whose
guard excludes *all four* other buff states including its own (`!shield && !hide && !power &&
!cloud`), so it cannot be re-grabbed until the current Cloud expires. Contact immediately starts
`BlupiAction::Charge` (a 64-tick, `ScaleTime(64)`, channeling animation, focus disabled) **and**
immediately sets `m_blupiCloud = true`, `m_blupiTimeShield = 100`, playing channel 58 — so the buff
is live from the moment of contact, not deferred like Suction/Drink. When the 64-tick animation
completes (~3048), `m_blupiCloud`/`m_blupiTimeShield = 100` are set again and channel 55 plays — in
effect re-arming the same 100-tick countdown a second time once the visible channeling animation
finishes. Decrements every `ScaleTime(4)` tick (same rate as Hide).

## Mirror/Invert — `ObjectType40` (~6040-6052)

The most permissive guard of any pickup here: only `!m_blupiHide` — Shield, Power, Cloud, and even
active vehicle rides do **not** block it. Automatic on contact, no button, no channeling animation:
`m_blupiInvert = true` and `m_blupiTimeShield = 100` (decrements every `ScaleTime(3)` tick, tying
with Power/invert for the shared-timer buffs' fastest decrement) take effect the instant Blupi
touches it. Spawns 4 `ObjectType41` burst particles at Blupi's position (±60/±10 offsets — the same
4-direction burst shape used for the pickup-sparkle described below, just a different sprite), plays
channel 66. Mechanically, Invert negates Blupi's horizontal input speed (`Decor::SetSpeedX`, ~1400-
1407: `if (m_blupiInvert) speed = -speed`) and mirrors his rendered facing direction — left/right
controls and sprite orientation are swapped for the buff's duration. On expiry (`m_blupiTimeShield`
reaches 0, no warning stage unlike the other four buffs) it spawns 4 `ObjectType42` particles (at
`±100` X/Y offsets from Blupi, same `-60/60/10/-10` direction params as the activation burst) in a
different spread and plays channel 67.

## Pickup sparkle — `ObjectType39` (~5957-5960, 5972-5975, 5987-5990, 6002-6005, 8382-8390)

Not itself a placed/collectible pickup — it is the sparkle-burst effect spawned *by* other pickups.
Confirmed by direct search: only two pickup families spawn it — Treasure (`ObjectType5`) and all
3 keys (`ObjectType49`/`50`/`51`), each spawning exactly 4 at the pickup point with offset parameters
`-60, 60, 10, -10`. Those offsets are directions/distances passed to `Decor::ObjectStart` (~7805-
7873): magnitudes over 50 mean vertical (up for negative, down for positive, after subtracting 50),
otherwise horizontal — so the 4 sparkles glide outward up/down/left/right until blocked by terrain,
then vanish. Each cycles an 11-frame icon table (`Tables::table_tresortrack`) over
`Config::ScaleTime(11)` ticks before expiring to `ObjectType0`. Purely cosmetic — no state mutation
of its own.

## Keys — `ObjectType49`/`50`/`51` (Key1/Key2/Key3)

Fully documented already in `06-doors.md`'s "Key pickup mechanic" section — not repeated here to
avoid drift between two copies of the same description. Summary only: contact (same `MoveObjectDetect`
rectangle test as every other pickup in this doc) deletes the world object and spawns 4
`ObjectType39` sparkles (see above), then a voyage flies the key's icon to its fixed HUD slot; the
`DoorKeyFlags` bit is only OR'd into `m_blupiCle` when that voyage completes, not at the moment of
contact. Each pickup block additionally guards `m_voyageIcon != <this key's icon> ||
m_voyageChannel != Element`, preventing the same key from being re-collected while its own
HUD-flight animation is still in progress.

## Dynamite (pickup only) — `ObjectType55` (~6116-6129)

Covers only the pickup step; placing/using dynamite (`BlupiAction::PutDynamite`, spawning
`ObjectType56`'s fuse) is a different mechanic, out of this doc's scope. Requires the action button
held at contact, and is gated on `m_blupiDynamite == 0` — Blupi can only ever be carrying one
dynamite at a time; a second cannot be picked up until the first is placed (decrementing
`m_blupiDynamite` back to 0 elsewhere). Also guarded by `m_voyageIcon != 252 || m_voyageChannel !=
Element`, the same "don't re-trigger while my own voyage is still flying" pattern as the keys. On
button-press: object deleted, a voyage starts (icon 252 to a fixed HUD slot `(505,414)`, channel 60
pickup chime from `07-sounds.md`), and `BlupiAction::TakeDynamite` plays (focus disabled). Only on
voyage completion does `m_blupiDynamite++` (~10298), going from 0 to 1.

## Not confidently characterized

Nothing in the required list above was left unresolved — every trigger condition, state mutation,
and constant cited was confirmed directly against `Decor.cpp`, not inferred. The one adjacent item
intentionally left alone is the **vehicle dismount/riding mechanics** (voluntary dismount via the
action button while mounted, and each vehicle's own movement/firing behavior) — real logic exists
for it in `Decor.cpp` but it describes what a vehicle *does*, not how it's *picked up*, so it belongs
in a vehicle-behavior doc, not here.
