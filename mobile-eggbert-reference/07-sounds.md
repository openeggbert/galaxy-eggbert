# Sounds

**Status: IN PROGRESS — channels 0-9 of 93 documented (`DOC-235`).** Tracked as `DOC-005` in
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
