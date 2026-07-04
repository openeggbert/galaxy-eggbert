# Sounds

**Status: IN PROGRESS — channels 0-79 of 93 documented (`DOC-235`-`DOC-242`).** Tracked as `DOC-005` in
`plan.md`, broken into 10-channel batches (`DOC-235`-`DOC-244`). `SoundChannel` (93 channels) is
already ported 1:1 in `include/GalaxyEggbert/def/SoundChannel.hpp`, confirmed numerically identical
to mobile-eggbert's version; **correction: that header has no names or comments per channel, only
the bare numeric enum** (the previous version of this line said "name-only list", which overstated
what's there — every channel is just `SoundChannelN = N`). 93 real `.wav` files exist in
`../mobile-eggbert/Content/sounds/`.

## Methodology

For each channel: `grep -rn "SoundChannel::SoundChannelN\b" ../mobile-eggbert/src/` (all channel
references live in `Decor.cpp` and `InputPad.cpp`; `Tables.cpp` doesn't reference `SoundChannel` at
all), then read the surrounding code to identify the real trigger. Some channels are indirected
through `Decor::SoundEnviron()`, which remaps a "generic" channel to a terrain-specific variant
based on the tile icon Blupi is standing on/hitting — those variants are documented in their own
batch (channels 78-91) with a cross-reference back here.

## Channels 0-9

| Channel | `.wav` | Real trigger (from `Decor.cpp`) |
|---|---|---|
| 0 | `sound000.wav` | **Reserved — means "no sound"**, not a real effect. Used as a sentinel value for "no motor loop currently playing" in `m_blupiMotorSound`/`AdaptMotorVehicleSound()`, and by `InputPad::PlayImage` similarly. Matches the header's own comment ("Channel 0 is reserved"). |
| 1 | `sound001.wav` | Jump takeoff (both the `BlupiAction::Jump` liftoff and the skateboard-jump variant — same channel either way). |
| 2 | `sound002.wav` | **Unused.** Zero references anywhere in `Decor.cpp`/`Tables.cpp`/`InputPad.cpp` — confirmed by grep, not assumed. The `.wav` file exists but nothing in the game ever plays it. |
| 3 | `sound003.wav` | Generic **footstep/landing** sound — remapped by `Decor::SoundEnviron()` to a terrain-specific variant (channels 78/80/82/84/86/88/90) based on the tile icon underfoot. Also reused as a generic **pickup/reward confirmation chime** at the end of several collectible "voyage" arcs (treasure, the 3 keys, dynamite, and others in `Decor::VoyageStep()`) — one channel serves both roles. |
| 4 | `sound004.wav` | Generic **head-bump/ceiling-hit** sound (triggered when Blupi's upward jump arc hits an obstacle above) — remapped by `Decor::SoundEnviron()` to a terrain-specific variant (channels 79/81/83/85/87/89/91), paired with channel 3's landing variants. |
| 5 | `sound005.wav` | Turn-around sound — plays whenever Blupi reverses direction, both on the ground (`BlupiAction::Turn`) and mid-air (`BlupiAction::TurnAir`). |
| 6 | `sound006.wav` | Vertigo/ledge-edge sound — plays when Blupi is standing at the edge of a drop (`BlupiAction::Vertigo`, teetering) and when starting to cautiously advance toward one (`BlupiAction::Advance`). |
| 7 | `sound007.wav` | Climbing/descending start sound for `BlupiAction::Down` (paired with channel 21 for `Up`) — plays when Blupi starts moving down (e.g. a rope/ladder-like vertical move). |
| 8 | `sound008.wav` | **Blupi death/danger** sound — plays for every fatal hazard: falling into the void below the level, touching lava, touching an electric hazard (`Blitz`), and falling out of bounds past the level's vertical limit. One shared "you died" sound across all death causes. |
| 9 | `sound009.wav` | Life-lost sound — plays at the start of the "life" voyage-arc animation (`Decor::VoyageInit`, `icon==48` in the Blupi sprite channel) at the exact moment `m_nbVies` (lives) is decremented. |

Channel 2 being provably unused is a real finding, not a gap in this pass — same pattern as the
unused `ObjectType`s and tile icons found elsewhere in this rework.

## Channels 10-19

| Channel | `.wav` | Real trigger (from `Decor.cpp`) |
|---|---|---|
| 10 | `sound010.wav` | **Small explosion/kill-impact** sound — plays whenever an enemy (bird/wasp/creature/`blupih`/`blupit`) is destroyed (converted to `ObjectType8`, the primary explosion) and whenever a ridden vehicle (helicopter/jeep/tank/skateboard) is destroyed by a spring/bounce hazard (`IsRessort`). **Special-cased in `Sound::PlayImage`**: it is the one channel excluded from the "don't restart if already playing" dedup check, so it can overlap with itself (needed since multiple kills/impacts can happen in quick succession). |
| 11 | `sound011.wav` | Generic **treasure/key pickup fanfare** — plays at the end of the collectible "voyage" arc for treasure (`ObjectType5`) and all 3 keys (`Decor::VoyageStep`/`VoyageInit`), and by the `AllTreasure` cheat. Superseded by channel 19 for the *last* treasure specifically. |
| 12 | `sound012.wav` | Secret-level-exit collectible found chime — plays when picking up the `ObjectType21` marker (distinct from actually reaching the goal tile, which is channels 13/14). |
| 13 | `sound013.wav` | "Not enough treasure yet" rejection sound — plays when Blupi reaches the level-exit goal (`ObjectType7`/`21`) but `m_nbTresor < m_totalTresor`. Real logic confirmed at both the actual goal-touch site and the `EndGoal` cheat path. |
| 14 | `sound014.wav` | **Level-complete/Win fanfare** — plays when Blupi reaches the goal tile with all treasure already collected (`BlupiAction::Win`). Also stops all 4 vehicle-motor-loop channels (16/18/29/31) first, since winning ends any ride. |
| 15 | `sound015.wav` | Helicopter engine **start** one-shot (`AdaptMotorVehicleSound`, played once when transitioning from no motor to the helicopter loop). |
| 16 | `sound016.wav` | Helicopter engine **loop**, high-pitch variant (`m_blupiMotorHigh`) — looped via `PlayImage(..., bLoop=true)`; defensively `StopSound`'d at dozens of state-transition points (death, mode switch, level end) to guarantee it never keeps looping past its vehicle's lifetime. |
| 17 | `sound017.wav` | Helicopter engine **stop** one-shot (played once when transitioning from the helicopter loop back to no motor). |
| 18 | `sound018.wav` | Helicopter engine **loop**, low-pitch variant — the other half of channel 16's high/low pair, same loop/stop pattern. |
| 19 | `sound019.wav` | **Final-treasure** pickup fanfare — upgrades channel 11 specifically when `m_nbTresor == m_totalTresor - 1` at pickup time (i.e. this is the treasure that completes the set). |

Channels 15-18 are the helicopter half of a motor-sound quartet; the jeep/tank/overcraft half
(channels 28-31) is documented below (`DOC-237`) since it follows the identical
start/loop-high/stop/loop-low pattern one batch later in the numbering.

## Channels 20-29

| Channel | `.wav` | Real trigger (from `Decor.cpp`) |
|---|---|---|
| 20 | `sound020.wav` | Descend-**end** sound — plays when `BlupiAction::Down` finishes and Blupi returns to `Stop`. Pairs with channel 7 (descend-*start*, see channels 0-9); there is no equivalent "ascend-end" sound — the `Up`→`Stop` transition is silent. |
| 21 | `sound021.wav` | Ascend-**start** sound for `BlupiAction::Up` (documented already in channels 0-9 as channel 7's pair). |
| 22 | `sound022.wav` | Water entry/exit splash — plays on every transition between swimming/surfing and open air (jumping out of water, being pushed out, surfacing at the water's edge). |
| 23 | `sound023.wav` | "Plouf" splash effect sound — plays in `Decor::MoveObjectPlouf()` when spawning `ObjectType14` (the water splash effect documented in `03-objects.md`). |
| 24 | `sound024.wav` | "Blup" rising-bubble sound — plays in `Decor::MoveObjectBlup()` when spawning `ObjectType15` (the water bubble effect documented in `03-objects.md`). |
| 25 | `sound025.wav` | Start-surfing transition sound — plays when Blupi transitions from swimming (`m_blupiNage`) to surfing on the surface (`m_blupiSurf`). |
| 26 | `sound026.wav` | **Drowning death** sound — plays when the underwater oxygen meter (`m_blupiLevel`) reaches 0 and Blupi enters `BlupiAction::Drown`. **A real finding**: drowning has its own dedicated death sound, distinct from channel 8's generic lava/electric/fall death sound — not the same "you died" cue reused. |
| 27 | `sound027.wav` | Blupi "sigh"/reaction sound — plays for the `Ouf3` reaction action and also when the action button is pressed while Blupi is already idle-standing (`BlupiAction::Non`, a "nothing to do here" no-op reaction). |
| 28 | `sound028.wav` | Jeep/tank/overcraft engine **start** one-shot — the ground-vehicle equivalent of channel 15 (see channels 0-9), same `AdaptMotorVehicleSound()` mechanism. |
| 29 | `sound029.wav` | Jeep/tank/overcraft engine **loop**, high-pitch variant — the ground-vehicle equivalent of channel 16; defensively `StopSound`'d at the same dozens of state-transition points as the helicopter loop channels. |

Channels 30/31 (the jeep/tank/overcraft loop's stop one-shot and low-pitch variant, completing the
28-31 quartet) are documented below (`DOC-238`).

## Channels 30-39

| Channel | `.wav` | Real trigger (from `Decor.cpp`) |
|---|---|---|
| 30 | `sound030.wav` | Jeep/tank engine **stop** one-shot — plays when voluntarily dismounting (action button), completing the 28-31 ground-vehicle quartet (parallel to channel 17 for the helicopter). |
| 31 | `sound031.wav` | Jeep/tank/overcraft engine **loop**, low-pitch variant — the ground-vehicle equivalent of channel 18; also `StopSound`'d defensively alongside 16/18/29 at every major state-transition point (death, world-exit, level-end, vehicle destruction). |
| 32 | `sound032.wav` | **World-exit** sound — plays when Blupi reaches a special "world portal" tile (`IsWorld()`) and enters `BlupiAction::Bye`, leaving the level entirely (e.g. returning to the world map). **A real finding**: this is a distinct sound from channel 14's level-complete/Win fanfare — reaching a world-exit tile is not the same event as winning via the goal object. |
| 33 | `sound033.wav` | **Door opening** sound — already confirmed in `06-doors.md` (`Decor::OpenDoor`, `Decor.cpp` ~11667): plays when a door tile is unlocked and its slide-up animation starts. |
| 34 | `sound034.wav` | **Grab suspend-bar** sound — plays when Blupi catches hold of a hanging bar/rope (`m_blupiSuspend` becomes true). |
| 35 | `sound035.wav` | **Release suspend-bar** sound — plays when Blupi jumps off a suspend bar (the `BlupiAction::Jump` case while `m_blupiSuspend` is active); pairs with channel 34. |
| 36 | `sound036.wav` | Idle ambient tick while hanging on a suspend bar (`BlupiAction::StopSuspend`) — a periodic sway/creak cue on specific animation-phase ticks, not a one-shot event. |
| 37 | `sound037.wav` | Idle ambient tick while standing still (`BlupiAction::Stop`) — a periodic fidget/blink cue on specific animation-phase ticks, the standing equivalent of channel 36. |
| 38 | `sound038.wav` | **Crate-push** sound — starts when Blupi begins pushing a crate (`ObjectType12`, `BlupiAction::Push`) and is explicitly stopped when the push ends; effectively a push-in-progress loop/cue. |
| 39 | `sound039.wav` | **Crate-bump** sound — plays when Blupi collides with a crate while airborne without the jump key held (`BlupiAction::Pop`/`StopPop`), i.e. bumping into a box rather than pushing it; stopped when leaving the Pop/StopPop states. |

Channels 36/37 are genuinely periodic idle-animation cues (triggered on specific `m_blupiPhase %
N` ticks), not simple state-transition one-shots like most other channels documented so far — worth
flagging since a naive port might only wire up one-shot triggers and miss these.

## Channels 40-49

| Channel | `.wav` | Real trigger (from `Decor.cpp`) |
|---|---|---|
| 40 | `sound040.wav` | **Wasp/bee sting** effect sound (`ObjectType44` contact) — inflates Blupi into a temporary "puffed up" debuff (`m_blupiBalloon = true`, ~100-tick timer, `ElectricShake` decor action) that pops on its own via channel 41. **Correction (found while researching this batch, `DOC-240`): earlier documentation of this channel as the `ObjectType31` Charge/Cloud pickup was wrong — that's channel 58 (this batch). Also a real finding in its own right: `m_blupiBalloon`, despite its name, is unrelated to the `ObjectType46` "balloon" vehicle pickup** (which sets `m_blupiOver`, not `m_blupiBalloon`, and plays no sound at all) **— a genuine naming collision in the original source.** |
| 41 | `sound041.wav` | Generic **pop/burst effect** sound — plays when the wasp-sting balloon debuff (channel 40) expires (`ObjectType91` spawn), when the "squashed" (`m_blupiEcrase`) effect ends (spawning 4 `ObjectType41` burst sprites in a cross pattern), and at one auto-launch site with the same burst-like framing. |
| 42 | `sound042.wav` | **Shield power-up** activation sound (`ObjectType25`, and the `RoundShield` cheat) — plays when the shield becomes active. |
| 43 | `sound043.wav` | Shield **about-to-expire** warning — plays once, exactly when the shield countdown timer (`m_blupiTimeShield`) reaches 10. |
| 44 | `sound044.wav` | **Power-mode** activation sound — plays when the `Sucette` pickup animation completes and `m_blupiPower` becomes true. |
| 45 | `sound045.wav` | Power-mode **about-to-expire** warning — plays once when `m_blupiTimeShield` reaches 20 while `m_blupiPower` is active; the power-mode equivalent of channel 43. |
| 46 | `sound046.wav` | Idle "fidget" reaction sound for the `Ouf1a`/`Ouf1b` idle-boredom animations (`BlupiActionOuf`, triggered probabilistically the longer Blupi stands still). |
| 47 | `sound047.wav` | Idle fidget sound for the `Ouf5` and `Mockeryp` idle-animation variants. |
| 48 | `sound048.wav` | Idle fidget sound for the `Ouf3` idle-animation variant. |
| 49 | `sound049.wav` | Idle fidget sound for the `Ouf4` idle-animation variant. |

Channels 46-49 (plus channel 65, documented in a later batch, for the `Mockery`/`Mockeryi` variants)
are all part of the same family: Blupi has several distinct "bored idle" reaction animations
(`Ouf1a`, `Ouf1b`, `Ouf3`, `Ouf4`, `Ouf5`, `Mockery`, `Mockeryi`, `Mockeryp`) chosen based on how
long (`m_blupiTimeOuf`) Blupi has been standing still, each with its own sound. Channels 43/45
(shield/power expiry warnings) are a matched pair with the same "about to run out" role for their
respective power-ups.

## Channels 50-59

| Channel | `.wav` | Real trigger (from `Decor.cpp`) |
|---|---|---|
| 50 | `sound050.wav` | **Suction-cup power-up** (`ObjectType26`, "Sucette") pickup **start** sound — plays when the action button is pressed on the pickup, before the `Sucette` animation completes into the actual `Power` buff (channel 44). |
| 51 | `sound051.wav` | **Glue-trap death** sound — plays when Blupi dies from stepping in glue/goo (`IsPiege`/`IsGoutte`, `BlupiDead(BlupiAction::Glu)`). Another dedicated death sound distinct from channel 8's generic one (see channel 26/drowning for the first such case). |
| 52 | `sound052.wav` | **Bullet-fired** sound — plays when a projectile (`ObjectType23`) is successfully spawned, in both the helicopter (`HelicoGlu`) and tank (`FireTank`) firing sequences. |
| 53 | `sound053.wav` | **Out-of-ammo** sound — plays when the fire button is pressed but `m_blupiBullet == 0`, in both the helicopter and tank firing paths (the "click" when trying to fire with no bullets left). |
| 54 | `sound054.wav` | **Bullet-pack pickup** fanfare — plays at the end of the `ObjectType29` (bullet pack, icon 177) collectible "voyage" arc, refilling `m_blupiBullet`. |
| 55 | `sound055.wav` | Charge/Cloud action **complete** sound — plays when the `BlupiAction::Charge` animation finishes and `m_blupiCloud` becomes true (the buff is now actually active); pairs with channel 58 (pickup start). |
| 56 | `sound056.wav` | Cloud (Charge) **about-to-expire** warning — plays once when `m_blupiTimeShield` reaches 25 while `m_blupiCloud` is active; the third member of the expiry-warning family alongside channels 43 (shield) and 45 (power). |
| 57 | `sound057.wav` | **Drink power-up** (`ObjectType30`) pickup **start** sound — plays when the action button is pressed on the pickup, before the `Drink` animation completes into the `Hide` buff (channel 62, below — not `Power`; `Drink` and `Sucette` grant two different buffs). |
| 58 | `sound058.wav` | **Charge/Cloud power-up** (`ObjectType31`) pickup **start** sound — the real trigger for picking up this item (corrects the earlier mis-attribution of this event to channel 40, above). |
| 59 | `sound059.wav` | Effect sound for an in-flight object destroyed by an electric hazard (`BlupiElectro`, spawns `ObjectType38`). |

Channels 50/44, 57/62, and 58/55 are three matching pickup-start/effect-complete pairs — each of
the three "drink-like" power-up items (`Sucette`/suction-cup, `Drink`, `Charge`/cloud) plays a sound
when first grabbed and a second, different sound when its buff actually activates a short animation
later. This is the same start/complete pattern as the vehicle motor quartets, just for buffs instead
of engines.

## Channels 60-69

| Channel | `.wav` | Real trigger (from `Decor.cpp`) |
|---|---|---|
| 60 | `sound060.wav` | Generic pickup-**start** chime for the dynamite (`ObjectType55`, icon 252) and character/persona (icon 108, `Button` channel) "voyage" pickups, plus the `Bombs` cheat. Parallels channel 11's role for treasure/keys, but for these two items specifically. |
| 61 | `sound061.wav` | Blupi **skin/persona swap** sound — plays when the action button spawns an `ObjectType200` (Blupi skin variant) and `m_blupiPerso` is decremented (spending a persona point to change skin). |
| 62 | `sound062.wav` | `Drink` buff-**complete** sound — plays when the `Drink` animation finishes and `m_blupiHide` becomes true (the actual buff activates); pairs with channel 57 (pickup start). |
| 63 | `sound063.wav` | `Hide` (Drink buff) **about-to-expire** warning — plays once when `m_blupiTimeShield` reaches 20 while `m_blupiHide` is active; the fourth and last member of the expiry-warning family alongside channels 43 (shield), 45 (power), and 56 (cloud). |
| 64 | `sound064.wav` | "Tiplouf" small-splash sound — plays in `Decor::MoveObjectTiplouf()` when spawning `ObjectType35` (documented in `03-objects.md`), the smaller counterpart to channel 23's "plouf". |
| 65 | `sound065.wav` | Idle taunt sound for the `Mockery`/`Mockeryi` idle-animation variants (part of the same "bored idle" family as channels 46-49, cross-referenced there). |
| 66 | `sound066.wav` | **Mirror/invert power-up** (`ObjectType40`) pickup activation sound — plays when picked up, spawning 4 `ObjectType41` burst sprites (same burst pattern as the squash-effect end, channel 41). |
| 67 | `sound067.wav` | Mirror/invert buff **expiry** burst sound — plays when `m_blupiInvert` wears off (`m_blupiTimeShield` reaches 0), spawning 3 `ObjectType42` sprites in a spread pattern. Unlike channels 43/45/56/63, this buff has no separate "about to expire" warning stage — just a direct end-of-buff pop. |
| 68 | `sound068.wav` | **Unused.** Zero references anywhere in `Decor.cpp`/`Tables.cpp`/`InputPad.cpp`/`Sound.cpp` — confirmed by grep, the second such gap after channel 2. |
| 69 | `sound069.wav` | Electric **"Blitz" zap** ambient sound — plays in `Decor::BlitzActif()` on specific ticks of a 100-tick lightning-flicker cycle, synced to the visible strikes near a blitz-emitter tile (icon 304). |

Channel 68 being provably unused (like channel 2) reinforces that not every reserved `.wav` file in
the sheet actually gets played — worth keeping in mind for `DOC-246`'s full 93-channel accounting.

## Channels 70-79

| Channel | `.wav` | Real trigger (from `Decor.cpp`) |
|---|---|---|
| 70 | `sound070.wav` | **Crusher-trap** (`Ecraseur`/`IsEcraseur`) onset sound — plays when Blupi gets squashed (`m_blupiEcrase = true`); pairs with channel 41 for the recovery pop when the effect ends. |
| 71 | `sound071.wav` | **Teleporter** use sound — plays when Blupi steps onto a teleporter tile (`IsTeleporte`, `BlupiAction::Teleporte`). |
| 72 | `sound072.wav` | **Bridge construction start** sound — plays at phase 0 of the 157-frame `ObjectType52` bridge-building sequence (documented in `03-objects.md`). |
| 73 | `sound073.wav` | Bridge construction **progress** sound — plays partway through the same sequence, at phase 137 (of 157), as a secondary construction cue before completion. |
| 74 | `sound074.wav` | Death-sequence **"angel ascent"** cue — plays for the fall-into-void (`Clear2`) and lava (`Clear3`) death recovery arcs, alongside the `VoyageInit` that spawns the angel-ascent animation (icons 230-241, per the cross-reference in the channels 0-9 section's `VoyageStep` note). |
| 75 | `sound075.wav` | Death-sequence sound for the saw/`Scie` death (`Clear4`) — spawns 3 `ObjectType41` burst sprites instead of the angel-ascent animation; a distinct "cut apart" cue rather than channel 74's ascent. |
| 76 | `sound076.wav` | Switch **click (off)** sound — plays in `Decor::ActiveSwitch()` when a switch is toggled to its closed/inactive state (icon 385). |
| 77 | `sound077.wav` | Switch **click (on)** sound — plays in the same function when toggled to open/active (icon 384); pairs with channel 76, one call site picks between them with `bState ? 77 : 76`. |
| 78 | `sound078.wav` | `SoundEnviron()` **landing** variant (remapped from generic channel 3) for one specific terrain-icon range (obstacle icons 32-34, 41-47, 139-143 — see the channels 0-9 section for the remapping mechanism). |
| 79 | `sound079.wav` | `SoundEnviron()` **head-bump** variant (remapped from generic channel 4) for the same terrain-icon range as channel 78 — its paired opposite. |

Channels 78/79 are the first of the 7 terrain-specific footstep/head-bump pairs promised back in the
channels 0-9 section; the remaining 6 pairs (80-91) are documented in the next batch (`DOC-243`).
