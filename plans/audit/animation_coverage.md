# Animation coverage: Blupi and the 3D models against Mobile Eggbert (2026-11-07)

Owner's question (2026-11-07): does Galaxy Eggbert really use all of Blupi's Ray Dream animations and
the other 3D models' animations that make sense to animate and that the original animates? This is a
read-only audit from code and files: nothing was built or run, `../speedyblupi-data` was not read, and
every "Galaxy" cell cites the source line it was read from. `tools/audit/animation_coverage.py` prints
the parsed `Tables::table_blupi` (frames, hold, set sites) beside `clips.txt`'s frame counts
(`python3 -I tools/audit/animation_coverage.py`) and, with `--objects`, how many of each object type's
icons have a model pose.

## Summary

**Verdict key.** MATCH: the original animates it and Galaxy plays the same animation (or the original
is static and so is Galaxy). DIFFERENT: both animate, not the same way ("by design" where the 3D turn
replaces a sprite turn, GP-0040). MISSING: the original animates it, Galaxy never does. STATIC_BY_DESIGN:
a decided 2D or fixed rendering. UNKNOWN: not verifiable from code and files. NOT_APPLICABLE: the
original never sets it. Counts are table rows (an object row can stand for several models).


| Category | Rows | Verdicts |
|---|---|---|
| Blupi actions (ids 0-87 of `BlupiAction`) + slow-walk | 89 | MATCH 65, DIFFERENT 11, MISSING 1, STATIC_BY_DESIGN 1, UNKNOWN 1, NOT_APPLICABLE 10 |
| Vehicles and riders | 16 | MATCH 13, DIFFERENT 3, MISSING 0, STATIC_BY_DESIGN 0, UNKNOWN 0, NOT_APPLICABLE 0 |
| Objects (121 model directories, effect families, overlays) | 36 | MATCH 33, DIFFERENT 0, MISSING 0, STATIC_BY_DESIGN 2, UNKNOWN 1, NOT_APPLICABLE 0 |
| Tiles (animated groups of `Decor::Build`) | 14 | MATCH 14, DIFFERENT 0, MISSING 0, STATIC_BY_DESIGN 0, UNKNOWN 0, NOT_APPLICABLE 0 |
| **Total** | 155 | MATCH 125, DIFFERENT 14, MISSING 1, STATIC_BY_DESIGN 3, UNKNOWN 2, NOT_APPLICABLE 10 |

Of the 14 DIFFERENT rows, 10 are the by-design 3D turns (nine `Turn*` actions and the
helicopter turn row); the other 4 are three real findings: Jump (one row), JumpSkate (a row in the Blupi table and one in the vehicle
table) and the skateboard pickup.

**Headline.** All 68 of Blupi's library clips have the original's frame count (checked by the helper: the clip is built
from the action's icon table at 20 frames a second) and 66 of them are selected somewhere; the exceptions are
below. The existing ledger line "every animation the original plays has its 3D clip"
(`plans/audit/blupi_animations.md`) is right about clips but silent about use: two clips are never played (`TakeDynamite`, `Hide`),
the jump clips play longer than the original's, and one pickup that spins in the original is frozen. Object models are
not "animated" by their own clip: all 121 are posed from a single clip `Frames` at the instant the icon
sequence of the original picks (`ObjectModelPose.seconds`, GalaxyEggbertGame.cpp:7138), so they animate
exactly as the original's tables do, wherever a pose exists for every icon of the sequence.

**Gaps ranked by visibility to a player**

1. **Jump and JumpSkate loop while rising** (DIFFERENT, small). The original plays Jump for 3 ticks, then
   Air (Decor.cpp:2922). Galaxy plays Jump while `m_velocityY > 0` (BlupiController.cpp:2861) and the host
   loops the 3-frame clip (`loop=true`, GalaxyEggbertGame.cpp:6897-6900); with kJumpSpeed 12 and kGravity 25 the rise
   lasts 0.48 s (about 10 ticks), so the 3 frames cycle about three times before Air. By arithmetic, not
   seen live. Every jump.
2. **Skateboard pickup (ObjectType 24) is frozen** (DIFFERENT, small). The original spins it about its long
   axis (`table_skate`, 34 ticks, icons 129-135, Decor.cpp:8339); Galaxy resolves the icons but draws the parked
   `skate` model at clip time 0.0 (GalaxyEggbertGame.cpp:7028-7040). `campaign_count` 63 in gameplay_use_cases.csv (GP-0108).
3. **TakeDynamite is never played and never freezes Blupi** (MISSING, small). The clip and AnimState exist and
   nothing calls `TriggerOneShotAnim(TakeDynamite...)`; the original shows 18 ticks and holds him still
   (Decor.cpp:6116-6128, 6329). `campaign_count` 39 (GP-0117). Fixing it moves golden traces by 18 ticks per pickup.
4. **Turns in vehicles, swimming, hanging and the air have no clip** (DIFFERENT, by design). The model yaws over the
   original's own turn length (TurnAroundSeconds). Only if the owner wants the original's front-view swing
   would this need new clips (large).
5. **Clip `Hide` is never selected** (UNKNOWN): the model is hidden through the life-loss window instead; not
   traced whether the original draws the Hide table there.
6. **ObjectType 31 (charge pickup) shows the electrode's model** with no pose for its sixth frame, 243
   (UNKNOWN, small): never placed in a shipped level.
7. Cosmetic: lightning frames in turn not at random, spring phase offset, fish head rounder (MATCH with notes).
8. Explosions, splashes, the shield orb, the pickup sparkles and trails stay animated billboards by decision
   (no Ray Dream scene): STATIC_BY_DESIGN.
9. In first-person camera no Blupi model is drawn, so none of the clips shows (design).

**The switch machine 384/385 (owner's report): nothing is missing.** See the last section: the original
swaps the two icons instantly and so does Galaxy; Blupi's Switch action is the only other animation.

## 1. Blupi's actions

All 88 `BlupiAction` ids, with `Tables::table_blupi` frame count and hold frame (parsed by
`tools/audit/animation_coverage.py`; the Clear3 block's header says 70 but 100 entries follow, and the
original's own scan steps over the spare 30 `-1`s). Galaxy's clip lengths equal the original's in every
row where a clip exists (the helper prints both). Timing: one frame per 1/20 s in both
(`kAnimFps`, BlupiController.hpp:103); one-shot lengths are the original's tick counts /20.

| Original action (id, frames, where set) | Original | Galaxy implementation (file:line) | Verdict | Proposed fix (size) |
|---|---|---|---|---|
| 0 None | 0 f; Decor.cpp 133,171,351... | no state (no pose to show) | NOT_APPLICABLE |  |
| 1 Stop | 330 f; Decor.cpp 324,397,2063... | AnimState::Stop, clip `Stop` 330 f; idle taps at ticks 125.. (BlupiController.cpp:2910-2940) | MATCH |  |
| 2 March | 6 f; Decor.cpp 2678,3379,3437... | AnimState::March, clip `March` 6 f (BlupiClipName, GalaxyEggbertGame.cpp:332) | MATCH |  |
| 3 Turn | 6 f; Decor.cpp 3372,3430 | AnimState::Turn only on foot, standing (`m_turning && standsForClip && !moving`, BlupiController.cpp:2855; host SetTurning, GalaxyEggbertGame.cpp:4587); clip `Turn` 6 f over TurnAroundSeconds 6 ticks. Reversal while walking shows no Turn clip (UNKNOWN whether the original plays it then: Decor.cpp:3372, 3430) | MATCH |  |
| 4 Jump | 3 f; Decor.cpp 2919,4772 | AnimState::Jump while `m_velocityY > 0`, else Air (BlupiController.cpp:2861); clip `Jump` 3 f, host plays it with loop=true, so a jump that rises longer than 3 ticks cycles the 3 frames again (no hold for Jump, BlupiController.cpp:2949). Original: Jump for 3 ticks, then Air (Decor.cpp:2922) | DIFFERENT | Make Jump end after 3 ticks into Air (or hold its last frame, as GetAnimHoldFrame does for Air/Down); small |
| 5 Air | 5 f, hold 4; Decor.cpp 2828,2897,2924... | AnimState::Air, clip `Air` 5 f held at frame 4 (GetAnimHoldFrame BlupiController.cpp:2949; host clamp GalaxyEggbertGame.cpp:6451-6455) | MATCH |  |
| 6 Down | 3 f, hold 2; Decor.cpp 3258,3267 | AnimState::Down, clip `Down` 3 f held at frame 2 (same hold code) | MATCH |  |
| 7 Up | 1 f; Decor.cpp 3241 | AnimState::Up, clip `Up` 1 f | MATCH |  |
| 8 Vertigo | 8 f; Decor.cpp 4917,4931 | AnimState::Vertigo 16 ticks (BlupiController.cpp:2630), clip `Vertigo` 8 f looped twice as the original; also in wind (`Stop` becomes Vertigo, BlupiController.cpp:2866-2872) | MATCH |  |
| 9 Recede | 6 f; Decor.cpp 3117,3128,4911... | AnimState::Recede 3 ticks (BlupiController.cpp:2619), clip `Recede` 6 f cut at 3 as the original (Decor.cpp:5045) | MATCH |  |
| 10 Advance | 6 f; Decor.cpp 4941,4967,4972... | AnimState::Advance 5 ticks (BlupiController.cpp:2636), clip `Advance` 6 f | MATCH |  |
| 11 Clear1 | 70 f; Decor.cpp 5463,5543,5795... | DeathLocked + DeathCause::Clear1, clip `DeathLocked.Clear1` 70 f, lock 70 ticks (BlupiController.cpp:1270); charred shape CharredBlupi.hpp | MATCH |  |
| 12 Set | 0 f; never set | none (the original never sets it, only tests it) | NOT_APPLICABLE |  |
| 13 Win | 6 f; Decor.cpp 1902,6174 | AnimState::Win, clip `Win` 6 f looped, 40 ticks (kWinDuration, GalaxyEggbertGame.cpp:5959) | MATCH |  |
| 14 Push | 6 f; Decor.cpp 6141,6150 | AnimState::Push (crate pushing, and the original's wind remap March->Push, BlupiController.cpp:2866-2872), clip `Push` 6 f | MATCH |  |
| 15 StopHelico | 1 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopHelico, clip `StopHelico` 1 f + vehicle model `helico` clip of the same name | MATCH |  |
| 16 MarchHelico | 8 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchHelico, clip 8 f + `helico` vehicle clip; lean by UpdateBodyTilt (GalaxyEggbertGame.cpp:1649, OR-0080) | MATCH |  |
| 17 TurnHelico | 10 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | no clip: TurnAroundSeconds 10 ticks yaws the model (GalaxyEggbertGame.cpp:277), as the 8 turn actions below. GP-0040 INTENTIONAL_3D_ADAPTATION | DIFFERENT | none wanted: a yaw turn is the 3D equivalent of the sprite's front-view turn; only if the owner wants the original's swing, build a turn clip from the TurnHelico icons (large) |
| 18 StopNage | 10 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopNage, clip `StopNage` 10 f; body turned and bobbing by UpdateBodyTilt (OR-0080) | MATCH |  |
| 19 MarchNage | 14 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchNage, clip `MarchNage` 14 f | MATCH |  |
| 20 TurnNage | 10 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | no clip: yaw over 10 ticks; swim direction rotation done (OR-0080) | DIFFERENT | as TurnHelico |
| 21 StopSurf | 12 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopSurf, clip 12 f; surf bob (OR-0080) | MATCH |  |
| 22 MarchSurf | 12 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchSurf, clip 12 f | MATCH |  |
| 23 TurnSurf | 10 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | no clip: yaw over 10 ticks | DIFFERENT | as TurnHelico |
| 24 Drown | 90 f; Decor.cpp 4628 | DeathLocked + Drown, clip `DeathLocked.Drown` 90 f, lock 90 ticks (BlupiController.cpp:1275) | MATCH |  |
| 25 StopJeep | 8 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopJeep, clip 8 f + `jeep` vehicle clip | MATCH |  |
| 26 MarchJeep | 8 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchJeep, clip 8 f + `jeep` vehicle clip; tips over an edge (OR-0080 text) | MATCH |  |
| 27 TurnJeep | 7 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | no clip: yaw over 7 ticks | DIFFERENT | as TurnHelico |
| 28 StopPop | 6 f; Decor.cpp 3295,3312 | AnimState::StopPop, clip `StopPop` 6 f (holding a crate) | MATCH |  |
| 29 Pop | 6 f; Decor.cpp 3303,3320 | AnimState::Pop, clip `Pop` 6 f (pulling it) | MATCH |  |
| 30 Bye | 12 f; Decor.cpp 5486 | AnimState::Bye, clip `Bye` 12 f looped, kByeDuration 1.5 s = 30 ticks (Decor.cpp:6436); the header comment saying it has no table is stale | MATCH |  |
| 31 StopSuspend | 328 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopSuspend, clip 328 f, taps 118/230/278 (BlupiController.cpp:2910-2940) | MATCH |  |
| 32 MarchSuspend | 12 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchSuspend, clip 12 f | MATCH |  |
| 33 TurnSuspend | 10 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | no clip: hanging model turns (m_hangFacingYaw, BlupiController.cpp:935) | DIFFERENT | as TurnHelico |
| 34 JumpSuspend | 10 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::JumpSuspend, clip 10 f (wind-up kSuspendJumpWindUpTicks, BlupiController.cpp:1773) | MATCH |  |
| 35 Hide | 9 f; Decor.cpp 6381 | AnimState::Hide exists and clip `Hide` 9 f is built, but nothing ever selects it (BlupiController.cpp:2839 chain; 0 assignments): after a death the model is hidden (IsDeathHidden). The original sets Hide with `m_blupiIcon = -1` (Decor.cpp:6381-6382); whether its table icons 276-284 are ever drawn during the 40-tick life-loss window was not traced | UNKNOWN | Trace Decor.cpp BlupiStep during Hide; if the table is drawn, play the clip in the IsDeathHidden window (small) |
| 36 JumpAie | 32 f; Decor.cpp 3005,3037 | reaction JumpAie, clip 32 f, 30 ticks (kJumpAieDuration; Decor.cpp:3042) | MATCH |  |
| 37 StopSkate | 140 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopSkate, clip 140 f + `skate` vehicle clip | MATCH |  |
| 38 MarchSkate | 96 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchSkate, clip 96 f + `skate` | MATCH |  |
| 39 TurnSkate | 14 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | no clip: yaw over 14 ticks | DIFFERENT | as TurnHelico |
| 40 JumpSkate | 3 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::JumpSkate while rising (BlupiController.cpp:2812-2816), clip 3 f + `skate` clip, looped; original JumpSkate lasts 3 ticks then AirSkate (Decor.cpp:2922) | DIFFERENT | as Jump |
| 41 AirSkate | 8 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::AirSkate, clip 8 f looped (the original's table has no hold either) | MATCH |  |
| 42 TakeSkate | 20 f; Decor.cpp 5752 | TriggerOneShotAnim at mount (GalaxyEggbertGame.cpp:5605), clip 20 f, 1.0 s (Decor.cpp:5752, ends 4512-4535) | MATCH |  |
| 43 DeposeSkate | 20 f; Decor.cpp 4504 | TriggerOneShotAnim at dismount (GalaxyEggbertGame.cpp:2700), clip 20 f, 1.0 s | MATCH |  |
| 44 Ouf1a | 29 f; Decor.cpp 5411 | reaction Ouf1a (after a swim, ArmOuf BlupiController.cpp:2679), clip 29 f, 29 ticks | MATCH |  |
| 45 Ouf1b | 29 f; Decor.cpp 6139,6148 | reaction Ouf1b (after a long push, BlupiController.cpp:2683), clip 29 f | MATCH |  |
| 46 Ouf2 | 32 f; Decor.cpp 4837 | TriggerOneShotAnim putting a decoy down (GalaxyEggbertGame.cpp:5540), clip 32 f, 32 ticks | MATCH |  |
| 47 Ouf3 | 34 f; Decor.cpp 3327,4846 | reaction Ouf3 (after a long pull BlupiController.cpp:2688; decoy shrug GalaxyEggbertGame.cpp:5547), clip 34 f | MATCH |  |
| 48 Ouf4 | 40 f; Decor.cpp 5643 | reaction Ouf4 (enemy passed close, ArmOuf GalaxyEggbertGame.cpp:5886), clip 40 f | MATCH |  |
| 49 Sucette | 32 f; Decor.cpp 1944,6033 | PickupBusy + Sucette, clip `PickupBusy.Sucette` 32 f, 32 ticks (BlupiController.cpp:1328) | MATCH |  |
| 50 StopTank | 64 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopTank, clip 64 f + `tank` vehicle clip | MATCH |  |
| 51 MarchTank | 8 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchTank, clip 8 f + `tank` | MATCH |  |
| 52 TurnTank | 12 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | no clip: yaw over 12 ticks | DIFFERENT | as TurnHelico |
| 53 FireTank | 6 f; Decor.cpp 4340 | TriggerOneShotAnim on a shot (GalaxyEggbertGame.cpp:5865), clip 6 f, 6 ticks, + `tank` clip | MATCH |  |
| 54 Glu | 25 f; Decor.cpp 5507,5516,5874... | DeathLocked + Glu, clip `DeathLocked.Glu` 25 f looped, lock 100 ticks as the original (Decor.cpp:6377); puddle model BlupiGlueModel | MATCH |  |
| 55 Drink | 4 f; Decor.cpp 2006,6061 | PickupBusy + Drink, clip 4 f looped, 36 ticks (BlupiController.cpp:1329) | MATCH |  |
| 56 Charge | 64 f; Decor.cpp 1989,6073 | PickupBusy + Charge, clip 64 f, 64 ticks | MATCH |  |
| 57 Electro | 90 f; never set | none (never set by the original, only tested at Decor.cpp:2412) | NOT_APPLICABLE |  |
| 58 HelicoGlu | 14 f; Decor.cpp 3705 | TriggerOneShotAnim on a glue drop (GalaxyEggbertGame.cpp:5873), clip 14 f, 14 ticks, + `helico` clip | MATCH |  |
| 59 TurnAir | 6 f; Decor.cpp 3365,3423 | no clip: yaw over 6 ticks while airborne (default of TurnAroundSeconds) | DIFFERENT | as TurnHelico |
| 60 StopMarch | 3 f; Decor.cpp 3624 | reaction StopMarch, clip 3 f, 3 ticks (BlupiController.cpp:2664) | MATCH |  |
| 61 StopJump | 5 f; Decor.cpp 2992,2996,3024... | reaction StopJump, clip 5 f, 5 ticks (BlupiController.cpp:2659) | MATCH |  |
| 62 StopJumph | 2 f; never set | none (never set by the original, only tested at Decor.cpp:3204) | NOT_APPLICABLE |  |
| 63 Mockery | 92 f; m_blupiActionOuf = ToBlupiAction(icon), Decor.cpp:5633 (MockeryDetect) | mockery (ArmOuf/TriggerMockery GalaxyEggbertGame.cpp:5178), clip 92 f, 92 ticks | MATCH |  |
| 64 Mockeryi | 104 f; m_blupiActionOuf = ToBlupiAction(icon), Decor.cpp:5633 (MockeryDetect) | as Mockery, clip 104 f | MATCH |  |
| 65 Ouf5 | 44 f; Decor.cpp 4766 | reaction Ouf5 (ArmOuf BlupiController.cpp:1794/1804), clip 44 f | MATCH |  |
| 66 Balloon | 16 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::Balloon, clip 16 f, duration 10 s of the original's 100 levels | MATCH |  |
| 67 StopOver | 1 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopOver, clip 1 f + `over` vehicle clip | MATCH |  |
| 68 MarchOver | 12 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchOver, clip 12 f + `over` | MATCH |  |
| 69 TurnOver | 7 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | no clip: yaw over 7 ticks | DIFFERENT | as TurnHelico |
| 70 Recedeq | 0 f; never set | none (never set) | NOT_APPLICABLE |  |
| 71 Advanceq | 0 f; never set | none (never set) | NOT_APPLICABLE |  |
| 72 StopEcrase | 1 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::StopEcrase, clip 1 f (flattened by the crusher) | MATCH |  |
| 73 MarchEcrase | 24 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::MarchEcrase, clip 24 f | MATCH |  |
| 74 Teleporte | 128 f; Decor.cpp 5597 | AnimState::Teleporting, clip `Teleporting` 128 f, kTeleportDuration 6.4 s = 128 ticks (Decor.cpp:6350) | MATCH |  |
| 75 Clear2 | 1 f; Decor.cpp 2756 | DeathCause::Clear2: BlupiClipName returns `DeathLocked.Clear2`, which has no clip (the frame is -1 in the original too); IsAnimIconInvisible hides the model and the soul is drawn as `angel` (OR-0047) | MATCH |  |
| 76 Clear3 | 70 f; Decor.cpp 5499 | DeathLocked + Clear3, clip 70 f, 70 ticks (header says 70; the original table carries 30 spare -1s that its own scan steps over) | MATCH |  |
| 77 Clear4 | 110 f; Decor.cpp 5524 | DeathLocked + Clear4, clip 110 f, 110 ticks (saw) | MATCH |  |
| 78 Clear5 | 1 f; never set | none (never set) | NOT_APPLICABLE |  |
| 79 Clear6 | 1 f; never set | none (never set) | NOT_APPLICABLE |  |
| 80 Clear7 | 1 f; never set | none (never set) | NOT_APPLICABLE |  |
| 81 Clear8 | 1 f; never set | none (never set) | NOT_APPLICABLE |  |
| 82 Switch | 10 f; Decor.cpp 5535 | TriggerOneShotAnim on Action at a switch (GalaxyEggbertGame.cpp:5486), clip `Switch` 10 f, 0.5 s | MATCH |  |
| 83 Mockeryp | 60 f; m_blupiActionOuf = ToBlupiAction(icon), Decor.cpp:5633 (MockeryDetect) | as Mockery, clip 60 f (the large creature) | MATCH |  |
| 84 Non | 18 f; Decor.cpp 6370 | TriggerReaction Non (GalaxyEggbertGame.cpp:5807), clip 18 f, 18 ticks | MATCH |  |
| 85 SlowdownSkate | 1 f; a BlupiSearchIcon remap of the base action (Decor.cpp:2123-2330) | AnimState::SlowdownSkate (BlupiController.cpp:2821), clip 1 f + `skate` clip | MATCH |  |
| 86 TakeDynamite | 18 f; Decor.cpp 6126 | AnimState/clip exist (18 f, kTakeDynamiteDuration 0.9 s) but nothing calls TriggerOneShotAnim(TakeDynamite): the dynamite pickup (InteractionSystem.cpp:2229) neither plays it nor freezes Blupi 18 ticks as the original does (Decor.cpp:6116-6128, 6329) | MISSING | On the ObjectType55 pickup event call TriggerOneShotAnim(TakeDynamite, kTakeDynamiteDuration); small (the 0.9 s freeze shifts golden traces: refresh them) |
| 87 PutDynamite | 26 f; Decor.cpp 4811 | TriggerOneShotAnim at placement (GalaxyEggbertGame.cpp:5515), clip 26 f, 1.3 s | MATCH |  |
| (March) phase halved when the analog speed is 0.1-0.75 and not in the helicopter | `num6 /= 2` (Decor.cpp:2388-2392) | clip time always runs at 1x (GalaxyEggbertGame.cpp:6449); the pad's stick counts past 0.5 as -1/0/+1 like the keys (GamepadInput.hpp:24), so a slow analog walk does not arise | STATIC_BY_DESIGN | |

Clips never played: `Hide` (no AnimState sets it), `TakeDynamite` (no trigger). Mapped but absent:
`DeathLocked.Clear2`. All other 66 clips are selected somewhere. Library frame count against the table: equal for all 68 clips (script output).

### 1b. Vehicles, riders and pickups

| Original action (id, frames, where set) | Original | Galaxy implementation (file:line) | Verdict | Proposed fix (size) |
|---|---|---|---|---|
| Helicopter in flight (StopHelico 15 / MarchHelico 16) | table_blupi 1 f / 8 f, remapped from Stop/March when `m_blupiHelico` (Decor.cpp:2139-2162) | `helico` vehicle model, clips StopHelico / MarchHelico drawn with Blupi's transform and clip time (GalaxyEggbertGame.cpp:6965-6980) | MATCH | |
| Helicopter lean `m_blupiRealRotation = speedX * 2` (Decor.cpp:2161, drawn rotated 774-782) | body rotated by his horizontal speed | UpdateBodyTilt (GalaxyEggbertGame.cpp:1649, OR-0080; angles 9-19 degrees checked by the main session, not re-measured here) | MATCH | |
| Helicopter glue drop (HelicoGlu 58, 14 f) | Decor.cpp:3058-3080, 3694-3708 | `helico` clip HelicoGlu + Blupi clip, 14 ticks (GalaxyEggbertGame.cpp:5873) | MATCH | |
| Helicopter turn (TurnHelico 17, 10 f) | sprite turn through the front view | no clip; model yaws over 10 ticks (TurnAroundSeconds, :277) | DIFFERENT (by design) | see the Blupi table, TurnHelico |
| Jeep Stop/March (25/26, 8 f each) | Decor.cpp remap 2186-2204 | `jeep` vehicle model, clips StopJeep / MarchJeep | MATCH | |
| Jeep tilt over an edge (`m_blupiRealRotation` approaches -45/+45 by 5 a tick, `m_blupiOffsetY = abs(rot/2)`, Decor.cpp:4281-4292) | the jeep tips nose down at an edge | tipping over an edge, nose down (GalaxyEggbertGame.cpp:1751, BD-0071); the 5-degree approach and the Y offset were not compared | MATCH | |
| Tank Stop 64 f / March 8 f / FireTank 6 f | 50/51/53 | `tank` vehicle model clips StopTank / MarchTank / FireTank (FireTank on a shot, :5865) | MATCH | |
| Skateboard Stop 140 / March 96 / Air 8 / Slowdown 1 / Take 20 / Depose 20 | 37/38/41/85/42/43 | `skate` vehicle model, all seven clips present and selected (BlupiController.cpp:2812-2822, host :2700, :5605) | MATCH | |
| Skateboard jump (JumpSkate 40, 3 f) | 3 ticks then AirSkate (Decor.cpp:2922) | looped while rising | DIFFERENT | see Jump; small |
| Overcraft Stop 1 f / March 12 f | 67/68 | `over` vehicle model, clips StopOver / MarchOver | MATCH | |
| Swimming (StopNage 10 / MarchNage 14) and surfing (StopSurf 12 / MarchSurf 12) | 18/19/21/22; body rotated by swim direction with a 20 degree bob, surf 10 degree bob (Decor.cpp:2259-2331) | Blupi clips of the same names, no vehicle model (the original has none); UpdateBodyTilt (OR-0080) | MATCH | |
| Parked pickups of the helicopter (13: icon 68), jeep (19: 89), tank (28: 167), overcraft (46: 208) | one fixed icon each, never animated (Decor.cpp:9009-9024) | the vehicle's model in the first frame of its Stop clip (kParkedVehicles, GalaxyEggbertGame.cpp:264, draw :7015-7050) | MATCH (original static) | |
| Parked skateboard pickup (24) | `table_skate` 34 ticks, icons 129-135: the board spins about its long axis, edge-on to flat and back (Decor.cpp:8339-8343; sheet crop checked by eye) | ObjectIcons.cpp resolves the icons (kSkate) but the parked-vehicle pass draws the `skate` model at clip time 0.0 for every phase (GalaxyEggbertGame.cpp:7028-7040) and skips the billboard (ObjectSprite, :3221) | DIFFERENT | Roll or yaw the parked board by the kSkate frame (angle from the sprite's width); small. 63 campaign placements (gameplay_use_cases.csv GP-0108) |
| Enemy Blupis (32 helicopter, 33 tank) | blupi1.png icons: MarchHelico, TurnHelico, HelicoGlu / MarchTank, TurnTank, FireTank (Tables.cpp:1314-1360) | Blupi's real model and vehicle in MarchHelico / HelicoGlu / MarchTank / FireTank, recoloured, turn as a yaw swing timed on the original's turn tables (EnemyBlupi.hpp, DrawEnemyBlupis :3329) | MATCH (turns DIFFERENT by design) | |
| The lost helicopter flying away (Decor::ByeByeAdd, BD-0049) | a spinning sprite | parked helico model spun about its middle (:7054-7090) | MATCH | |
| Vehicle shown in a clip that has no vehicle clip (Win, Switch, PickupBusy, DeathLocked, Teleporting...) | those icons are Blupi-only | the vehicle is drawn only when the vehicle model has the playing clip (:6968) | MATCH (by construction) | |

## 2. Object models

How the host draws them: `objectModelTable_` maps (sheet, icon) to a model, the pose time in its
`Frames` clip, a mirror and a yaw (`avatars3d/objects/<model>/icons.txt`); the game chooses each
object's icon exactly as the original (ObjectIcons.cpp holds the transcribed tables), and
`drawModel` samples the clip at that exact time with `loop=false` (GalaxyEggbertGame.cpp:7138):
a stepped pose per icon, like the sprite, no blending between poses. A sequence whose icons all have
a pose is therefore animated as in the original. `--objects` of the helper confirms it per type: every
animated type with a model has all its table icons covered (types 2-7, 13, 16, 17, 20, 21, 26, 29, 30,
34, 40, 44, 47-56, 96, 97, 200, 201). The ones without a pose are 2D effects (25, 36, 37, 39, 41, 42, 57),
the parked vehicles (19, 24, 28, 46, drawn from Blupi's vehicle models), the enemy Blupis (32, 33, drawn
from Blupi's clips) and ObjectType 31 (frame 243), all in the tables below.

Of the 121 models: 23 hold several distinct poses, 14 have several icons on one pose (turned by
yaw/mirror, or a decal), 84 are single-icon statics.

### 2a. Models with several poses

| Model / family | Original animation (source) | Galaxy implementation | Verdict | Proposed fix / note |
|---|---|---|---|---|
| treasure_coffre (ObjectType 5, element 0-11) | `icon = q<11 ? q : 11-q%11`, q = phase/3 % 22 (Decor.cpp:8297-8307): the chest opens and closes | 12 icons, 12 poses (0.00-2.35 s) at the icon the table gives (ObjectIcons.cpp:337) via drawModel (GalaxyEggbertGame.cpp:7119) | MATCH | |
| egg_oeuf (6, element 21-28) | 21 + phase/4 % 8 | 8 icons on 7 distinct yaw/mirror, one pose: the egg turns | MATCH | |
| exit_fleche (7, element 29-36) | 29 + phase/3 % 8 | 8 icons, 8 yaws, one pose: the arrow turns | MATCH | |
| mine_bombe (2: 12-20, 3: 48-56) | 12 + phase/2 % 9 (the hanging one 48 + ...) | 18 icons, yaw 0-80 and mirror/upside-down, one pose: the mine turns | MATCH | |
| movingmine_bombe-move (16, element 69-77) | 69 + phase % 9 | 9 icons, 9 poses | MATCH | |
| follow_bombe-follow (96/97, element 256-265) | table_follow1 (26) / table_follow2 (5) | 10 icons, 10 yaws, one pose | MATCH | |
| bulldozer_tracks-profil (4, element 57-67) | table_bulldozer left/right/turn2l/turn2r by step (Decor.cpp:8628-8668) | 11 icons, 11 yaws; GetBulldozerIcon (ObjectIcons.cpp:481) keeps the 4-step machine | MATCH | |
| fish_poisson (17, element 78-88) | table_poisson tables (turn 48 f) | 11 icons, 6 poses; GetFishIcon | MATCH | head rounder than the sprite (BB3D-032, open, cosmetic) |
| bird_oiseau (20, element 90-121) | table_oiseau tables (8 f walk, 10 f turn) | 32 icons, 7 poses; GetBirdIcon | MATCH | |
| wasp_guepe + wasp_guepe-tourne (44, element 195-207) | table_guepe tables | 8 + 5 icons, 4 + 5 poses; GetWaspIcon | MATCH | |
| creature_creature1y + 1-5 (54, element 242-251) | table_creature walk 8 f, turn 152 f | 10 icons, 10 poses (six models); GetCreatureIcon | MATCH | |
| key1_cle1, key2_cle2, key3_cle3 (49-51, element 209-229) | table_cle1-3, 12 f, phase/3 | 7 icons, 7 poses each | MATCH | |
| secretexit_cle (21, element 122-128) | table_cle, 12 f | 7 icons, 7 poses | MATCH | |
| lollipop_sucette (26, element 136-143) | table_power, 8 f, phase/2 | 8 icons, 8 poses | MATCH | |
| invert_panneau-invert (40, element 187-194) | table_invert, 20 f, phase/2 | 8 icons, 6 poses | MATCH | |
| angel_blupi-ange (Clear2 soul, element 230-241) | VoyageDraw rising 300 px (Decor::BlupiDead) | 12 icons, 6 poses, alpha 0.5, rises 300 px (GalaxyEggbertGame.cpp:7288-7312, OR-0047) | MATCH | |
| glu168_glu (34 goo particle and Blupi's Glu death, element 168-171) | table_glu, 25 f | 4 icons, 4 poses; also the puddle (BlupiGlueModel :2813) | MATCH | |
| tentacle_tenta01-17 (53, explo 70-86) | table_tentacule (45, phase/2) | 17 icons, one model per icon | MATCH | |
| conveyor311_chenille (47/48 lifts, tile 311-316) | table_chenille / table_chenillei, phase % 6 | 6 icons, 6 poses; ConveyorLiftModel (:3419) | MATCH | tile 311-316 itself is static in the original too |
| bridge364_grotte-fragile1 (52, 364-372) | table_bridge, 157 f (shake 365/366, crumble 367-372, retract) | 9 icons, 8 poses; InteractionSystem.cpp:1660 holds the 157-entry table, the terrain model draws the cell | MATCH | |
| fan_ventillo-h (tiles 126-137) | table_decor_ventillo*, 3 f, m_time/2 | 3 poses turned about Z for the four directions (BlockModels.hpp kFanModelBases) | MATCH | |
| crusher317_aplatisseur (317-323) | table_decor_ecraseur, 10 f, m_time/3 | 7 poses, ResolveBlockAnimationIconAt | MATCH | |
| saw378_scie (378-383) | table_decor_scie, 6 f, per-cell offset | 3 poses x mirror, per-cell offset; stopped saw 379 static | MATCH | |
| temp324_temporaire (324-329) | table_decor_temp, 20 f incl. two -1 | 5 poses; -1 draws nothing | MATCH | |
| spring211_ressort (211, 209-213) | table_ressort[(m_time/2 + i*7) % 8] (Decor.cpp:926-929) | 5 poses; SpringFrameIcon, offset (x+z)*7 instead of the column's i*7 | MATCH | phase offset differs trivially |
| switch384_interrupteur (384/385) | static tile swap (see section 3) | 2 poses, 0.00 s and 0.05 s | MATCH | |

### 2b. Several icons on one pose

| Model / family | Original animation (source) | Galaxy implementation | Verdict | Proposed fix / note |
|---|---|---|---|---|
| dynamite_dynamite (55 pickup, 56 fuse, element 252-255) | 55: icon 252 static; 56: table_dynamitef, 100 f, fuse flicker | 4 icons, one pose; the fuse's sparks are a decal changing with the icon (dynamiteSparks_, :7244) | MATCH | |
| decoy_bombe, decoygreen_bombe (200-203, blupi/blupi1 257-262) | 257 + phase % 6 | 6 icons, 6 yaws each | MATCH | |
| electro238_electro (tile 238-242, also ObjectType31's frames 238-243) | tile static; ObjectType31 table_charge 6 f, phase/2 | 5 icons, one pose, painted sparks as a decal | UNKNOWN | ObjectType31 shows 238-243, the model has no icon 243 (that frame is a billboard) and is the electrode's model; whether a charge pickup should show it was not looked at. Never placed in a shipped level (category B). |
| drip404_goutte04 | table_decor_goutte, 48 f | 407 and 408 on one pose; DripShrink scales the smallest drops (BlockModels.hpp) | MATCH | |
| sign174/worldsign158/worlddone166/treasure421 (signs, treasure doors) | static icons; digits/pictures painted | one pose per icon, painting as decal | MATCH (original static) | |

### 2c. Statics, effect families and overlays

| Model / family | Original animation (source) | Galaxy implementation | Verdict | Proposed fix / note |
|---|---|---|---|---|
| 84 single-icon models: the BigDecor trees, palms, houses, machines, toys, columns, vase, palace pieces, statue (explo 16-89), the girder/pillars/posts/barrels/rails/support models, the teleporters 330-333, the doors 334-336, the bar 138/202, the emitter 304, the pickups drink/ammo/projectile/helicopter, the creatures' one-frame models 1-5 | one fixed icon, never animated in the original (BigDecor except 203 is static, Decor.cpp:709-733; tiles draw as stored, Decor.cpp:1010-1130 lists the only animated icons) | one pose | STATIC_BY_DESIGN (original static too) | |
| Pickup effects, shield orb and trails, explosions 8-11, 90-93, splashes 14/15/35/98-100, pollution 36, clear 37, goo, magic/shield/tresor tracks 27/57/39, invert bursts 41/42, electric arc 38, shield pickup 25, charge sparks (explo/element effect icons) | table-driven sprites (tables transcribed in ObjectIcons.cpp) | billboards animated by the same tables; no Ray Dream scene exists for them (render_representation.csv VIS-0524-0526, decision 2026-10-24) | STATIC_BY_DESIGN (animated 2D by decision) | models would need new art |
| Blupi's own overlays: shield orb (table_shield_blupi 16 f), shield sparkles (table_shieldloop), power stars (table_magicloop), drink bubbles (table_drinkoffset 0/7/22) | Decor.cpp:787-801, 868-880 | billboards, 2 ticks a frame, 3 bubbles rising 3 px a tick (GalaxyEggbertGame.cpp:7867-7960) | MATCH | |
| Door opening (ObjectType22): the door's sprite slides up one cell in 50 ticks | Decor.cpp:11650-11690 | OpenDoorAt spawns the slide object (InteractionSystem.cpp:408-421); it draws through ObjectSprite and so as the door's model where one exists (334-336, 421-439) | MATCH (code read, not seen live) | |
| Lightning bolt 305-307 | drawn at random frame while BlitzActif (Decor.cpp:1079-1085) | generated 3D bolt, frames by turn instead of random (BlockDefinitionRegistry.cpp:171-178) | MATCH | order not random (cosmetic) |

## 3. Tiles

The original animates exactly these tile icons in `Decor::Build` (Decor.cpp:1010-1130, 724-733,
926-929); every other tile is drawn as stored.

| Model / family | Original animation (source) | Galaxy implementation | Verdict | Proposed fix / note |
|---|---|---|---|---|
| 68 lava (+69-72) | table_decor_lave, 8 f, (i*13+j*7+m_time/2) % 8 (Decor.cpp:1029) | LavaSurface, a wave height-field cycled by the same frames; divisor 2 with the same cell offsets (BlockDefinitionRegistry.cpp:180, :347-352) | MATCH | |
| 373 spike trap | piege1 16 f at m_time/4 while Blupi is focused, piege2 4 f at /2 otherwise (Decor.cpp:1032-1037) | spikeFrames divisor 4 and the 4-frame snap when not focused (BlockDefinitionRegistry.cpp:181, :353-357) | MATCH | |
| 404/410 slime drip | table_decor_goutte, 48 f, m_time/2, per-cell offset, -1 gaps (Decor.cpp:1039-1048) | dripFrames 48 f divisor 2, per-cell offset; model + DripShrink | MATCH | |
| 317 crusher | table_decor_ecraseur 10 f, m_time/3, no cell offset (:1055) | crusherFrames divisor 3, no offset; model | MATCH | |
| 378 saw / 379 stopped saw | table_decor_scie 6 f, per-cell offset; a stored 379 stands still (:1060, ActiveSwitch) | sawFrames divisor 1 with offset; SawStopped = 379 only | MATCH | |
| 324 temporary saucer | table_decor_temp 20 f, m_time/4 (:1065) | tempFrames divisor 4; model | MATCH | |
| 92 / 91 water | eau1 6 f at /3 with offset; eau2 6 f at 3 + (17i+13j)%3 (:1070-1078) | water1/water2 with the same divisors and offsets | MATCH | cube textured with the frames (no scene) |
| 305 lightning | random 305-307, only while BlitzActif | cycled 305-307 on even ticks of the first half of 100 | MATCH | order |
| 110/114/118/122 wind streaks | table_decor_ventg/d/h/b, 4 f, m_time/1 (:1085-1102) | per-tick frames, two crossed plates (TerrainRenderer.hpp:273, OR-0065) | MATCH | |
| 126/129/132/135 fans | 3 f at m_time/2 (:1104-1122) | fan model poses, divisor 2 (BlockDefinitionRegistry.cpp:189-192) | MATCH | |
| 211 spring | table_ressort, m_time/2 + i*7 (:926-929) | SpringFrameIcon | MATCH | |
| 203 marine (BigDecor) | table_marine 11 f at m_time/3 (:724) | Marine animation, bigDecorMarineMesh (GalaxyEggbertGame.cpp:8171-8215) | MATCH | |
| 384/385 switch | no animation: the tile is swapped (Decor.cpp:7134-7136) | block type swap, model pose by icon, no blending (section 3) | MATCH | |
| 311-316 conveyor tile, 364-372 bridge tile, 330-333 teleporters, 334-336 doors, signs, bars | drawn as stored; only objects 47/48/52 animate them | models per icon (kStillModelIcons), static tile | MATCH | |

## 4. The switch machine 384/385

**Original.** `Decor::ActiveSwitch` (Decor.cpp:7134-7147): `ModifDecor(pos, bState ? 384 : 385)` swaps the tile
icon at once (`ModifDecor`, 7925-7933, does nothing else for these icons), plays sound 77 (on) or 76 (off),
then scans 41 cells along X and swaps every saw tile 379 to 378 (on) or 378 to 379 (off), so the saws start
or stop turning; Blupi plays his `Switch` action (10 ticks, Decor.cpp:5535). The tile is drawn by its own
pass (Decor.cpp:759-762) with no table. The sheet has just the two icons (lever up with a green lamp, lever
down with a red one). Nothing else in the original animates: no object is spawned, no in-between frames, no
voyage.

**Galaxy.** `avatars3d/objects/switch384_interrupteur/icons.txt` maps icon 384 to pose time 0.00 s and 385 to
0.05 s of the model's `Frames` clip (IoU 1.000 each). `TryActivateSwitch` (WorldRuntime.cpp:455) changes the block
type 384 to 385 (or back), and turns the saws of its row within 20 cells along X (OR-0058); the model pass draws the cell's own icon
through `drawModel`, which samples `Frames` at that single time with `loop=false`
(GalaxyEggbertGame.cpp:7138-7140, still-icon branch near 7384): the lever and lamps change in one frame,
exactly as the original, plus Blupi's Switch clip (0.5 s). OR-0081 (commit 4bbfae47, today) additionally
turns the panel toward Blupi so the two states can be read. A smooth lever movement would be Galaxy's own
addition. Possible confusion for the owner: a switch rebuilds the terrain (documented ~180 ms hitch under
xvfb, owner_reports.md line 343), which can read as lag between the key and the change; not measured here.
Verdict: MATCH.

## 5. Ranked fix list

| # | Fix | Size | Why first |
|---|---|---|---|
| 1 | Jump / JumpSkate: end the state into Air after 3 ticks (or hold the clip's last frame like Air/Down, `GetAnimHoldFrame`), BlupiController.cpp:2861, 2949 | small | every jump, every player |
| 2 | Spin the parked skateboard by `kSkate` (a roll about its long axis from the sprite's width), GalaxyEggbertGame.cpp:7028-7040 | small | campaign_count 63; the only pickup whose original animation is lost |
| 3 | Trigger `TakeDynamite` (and its 18-tick freeze) on the ObjectType55 pickup, InteractionSystem.cpp:2229 and the host's pickup event; refresh golden traces | small | campaign_count 39; a finished clip sits unused |
| 4 | DONE 2026-11-07 (BD-0084): the original draws Hide (276-284) through the life-loss window; Galaxy plays the `Hide` clip there now | small | was the only unknown about a built clip |
| 5 | Look at ObjectType31's frames 238-243 (electrode model, no pose for 243) | small | never placed in a shipped level |
| 6 | Turn clips for vehicles/swim/air from the `Turn*` icon tables | large | by design today; only if the owner wants the original's swing |
| 7 | 2026-11-07: the lightning's frames come in shuffled bags of 305-307 (a reproducible stand-in for the original's `m_random->Next(305, 308)`; every bolt cell still shows the same frame at a time, the original draws each cell's own). Kept: the spring's phase (x+z)*7, the 3D form of the original's column i*7 (with x alone, springs in a row along Z would bounce together); the fish head (BB3D-032: every hypothesis tested, reopen only if the owner names it; D11 was closed without it) | small | negligible |

## 6. What could not be verified

- Nothing was run: whether each pose looks right at the icon it is chosen for (IoU figures are in
  `plans/inventory/model_parity.md`), whether Jump loops visibly (item 1 is arithmetic on
  `kJumpSpeed`/`kGravity`), whether the door slide object draws as the door's model live.
- Colour, shading and the clips' pose correctness against the Ray Dream sources (`speedyblupi-data`
  was out of scope).
- Original conditions of Turn while walking (Decor.cpp:3359-3380, 3417), Hide's drawing, the spring bounce
  setting Air in vehicles (Decor.cpp:2886-2898), jeep/tank behaviour after a fall.
- Frame rates other than the original's 20 Hz reference (`Config::ScaleDiv`/`ScaleTime`) are taken as
  the ledger took them.
- 2026-11-07, process note: another agent's commit 4a81c194 swept the first version of the helper script
  into a world commit (it was untracked when they ran `git add`); this commit updates it.
