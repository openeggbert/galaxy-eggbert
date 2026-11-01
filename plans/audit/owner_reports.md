# Owner reports (OR-NNNN)

The owner's own findings, recorded as given (translated from Czech), each to be reproduced against
the reference and fixed or answered. The owner stresses these come from a spot check only: the audit
must be deep and complete, not limited to this list. Status values as in
`../GALAXY_EGGBERT_FIDELITY_AUDIT.md` §2. Newest batch first.

## 2026-11-01, second batch (after the first fixes of the day)

| ID | Report | First notes | Status |
|---|---|---|---|
| OR-0001 | The maximum number of eggs (lives) does not match Mobile Eggbert. In Mobile Eggbert Blupi still lives after the last egg is used; in Galaxy that is game over. The eggs at the bottom of the screen probably show the lives remaining, not the total. | Confirmed (Decor.cpp:6379-6396, 10176, 1185-1194): the row shows `m_nbVies`, the lives in reserve; a death with one or more spends one (it flies to him), only a death with none is game over. Galaxy ended the game when the row held one. Fixed: `LoseLife`/`ResolveDeathLock`; `VerifyInteractionSystem`. The maximum (MAX_EGG_COUNT, 10) already matched. | FIXED_AND_VERIFIED |
| OR-0002 | Blupi's farewell (Bye) when going to another world freezes and ends at once. | D10 fixed the Bye clip earlier; check the state's length and the world change timing | UNREVIEWED |
| OR-0003 | Getting off the skateboard shows two skateboards: one already put down and the one he is putting down. | the deposited object appears at once while DeposeSkate still draws the board in his hands | UNREVIEWED |
| OR-0004 | The teleporters (alpha, beta, ...) — reported without detail. | look at them live (models, arrival, labels) and ask what is wrong | UNREVIEWED |
| OR-0005 | Wasps, fish, birds and others move too fast, not like Mobile Eggbert. | patrol speeds (`AdvancePatrolStep`, the record's ticks), the wasp's/follower's homing speeds vs Decor.cpp | UNREVIEWED |
| OR-0006 | A world with moving platforms that move too fast to jump across. | BD-0006 fixed 31 lifts (024, 061, 062, 073, 113) on 2026-11-01; ask which world if it persists after that build | FIXED (to confirm) |
| OR-0007 | Cheats do not work, e.g. "givecopter" does nothing; probably none work. | the cheat input path (typed words) vs Decor/Game1 cheat list | UNREVIEWED |
| OR-0008 | The window title says "Galaxy Eggbert (CNA)": " (CNA)" must go; the executable should be GalaxyEggbert, not GalaxyEggbertCNA. | title string; CMake target/output name (scripts, packaging, CI, docs refer to it) | UNREVIEWED |
| OR-0009 | New goal: a completely different loading screen (today it is Mobile Eggbert's). | Galaxy's own, from its assets (as the title screen, `title.vwr`) | UNREVIEWED |
| OR-0010 | The Setup menu: only its first screen works; "Automatic zoom on action" does nothing (not implemented), nor does the accelerometer. | the original's Setup screens and options | UNREVIEWED |
| OR-0011 | Make it playable on mobile devices (Android), with the accelerometer if realistic. | on-screen pad exists (MENU-021..024); Android build (`ANDROID.md`) | UNREVIEWED |
| OR-0012 | The 3D editor's menu looks like a prototype: make it look normal. | editor palette/toolbar UI | UNREVIEWED |
| OR-0013 | Nothing is animated in the 3D editor; in blupi_recon every animatable element and object is animated. | editor preview/terrain animation while editing | UNREVIEWED |
| OR-0014 | Each world gets one of 10 background music tracks (the Speedy Eggbert MIDI, converted to WAV), or none; set in the 3D editor; the owner leaves the choice for the existing worlds to me. | a world music field (`.vwr` metadata record), editor UI, audio playback, the MIDI sources | UNREVIEWED |
| OR-0015 | The death animations are mostly wrong, e.g. Blupi cheers, and a mine icon flies from him instead of an egg. | The flying icon: confirmed and fixed — the life-loss voyage drew icon 48 from element.png (the hanging mine); the original flies icon 48 of the Blupi sheet, the lives row's small Blupi (Decor.cpp:6391). The death clips (Blupi cheering) still to check. | REPRODUCED_DEFECT (icon fixed; clips open) |
| OR-0016 | After the invincibility drink, instead of Mobile Eggbert's transparent Blupi there is a small Blupi spinning. | the Hide state's look (Drink: Hide/transparency) | UNREVIEWED |
| OR-0017 | Walking along a wall close to it, part of Blupi is inside the wall. | his collision is a point; the model has width (BD-0005 suspected) | UNREVIEWED |
| OR-0018 | On a moving platform a wrong animation plays, as if Blupi were falling. | onGround while riding a lift (RideLift) vs the anim state | UNREVIEWED |
| OR-0019 | Flying the helicopter into a block's ceiling should wreck it; nothing happens. | the original's helicopter crash rule | UNREVIEWED |
| OR-0020 | In the helicopter in the air (not at the floor), pressing Space leaves the helicopter hanging in the air and Blupi falls; Mobile Eggbert probably does not behave so. | the original's dismount rule for the helicopter (only on the ground?) | UNREVIEWED |
| OR-0021 | Pulling a crate back sometimes does not work. | D60's pull: hold detection, Blupi's room behind, `CratePush` | UNREVIEWED |
| OR-0022 | Pulling a crate, if all keys are released Blupi keeps pulling it by himself for another second. | D60's committed move (M3D-0001): the owner rejects it; the original stops at once | REPRODUCED_DEFECT (by design of M3D-0001) |
| OR-0023 | In Mobile Eggbert a crate can be pulled onto another crate so that part of it overhangs; in Galaxy it cannot. "You skimped on comparing the crate logic." | M3D-0001's whole-cell moves and destination-cell support; the original's crates rest at any pixel and need only a 20 px corner support | REPRODUCED_DEFECT (by design of M3D-0001) |

Consequences already clear: OR-0022/0023 overturn M3D-0001 (the owner wants the original's
continuous crates: stopping anywhere, overhanging up to the original's corner rule). The crate
module has to move to continuous positions with box collision against cells and crates.
