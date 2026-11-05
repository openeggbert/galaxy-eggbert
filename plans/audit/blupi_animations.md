# Blupi's animations: Mobile Eggbert's actions against Galaxy's 3D clips (2026-11-05)

The owner asked (2026-11-05) whether every animation of Blupi in Mobile Eggbert is in Galaxy, in 3D.
Every `BlupiAction` (`include/GalaxyEggbert/Def/BlupiAction.hpp`, the original's enum) was checked
against where the original sets it (`Decor.cpp`: `m_blupiAction = ...`, `BlupiDead(...)`) and the
clips of Blupi's 3D model (`avatars3d/blupi/clips.txt`, built from the Ray Dream scenes by
`tools/rd4_to_gltf/build_clip_library.py`).

**Result: every animation the original plays has its 3D clip**, but for these, handled otherwise:

| Action | Set in Decor.cpp | Galaxy |
|---|---|---|
| Turn in the air (`TurnAir`) and in the vehicles (`TurnHelico`, `TurnJeep`, `TurnTank`, `TurnSkate`, `TurnOver`, `TurnNage`, `TurnSurf`, `TurnSuspend`) | yes | the 2D sprites' turn between facing left and right; in 3D the model turns smoothly (the walk's `Turn` has its clip) |
| `Clear2` (a death where he vanishes) | yes (Decor.cpp:2756) | no frame to show: the original's icon is -1 (`kClear2Frames`) |
| `Teleporte` | yes | the clip `Teleporting` |
| `None` | yes | no animation |
| `Set`, `Electro`, `StopJumph`, `Recedeq`, `Advanceq`, `Clear5`-`Clear8` | never set (only tested) | not needed |

The clips Galaxy has (one per line of `clips.txt`):

`March` `Jump` `Air` `Down` `Up` `StopEcrase` `MarchEcrase` `Balloon` `Teleporting` `StopHelico` `MarchHelico` `StopJeep` `MarchJeep` `StopTank` `MarchTank` `StopSkate` `MarchSkate` `JumpSkate` `AirSkate` `SlowdownSkate` `StopOver` `MarchOver` `StopNage` `MarchNage` `StopSurf` `MarchSurf` `StopSuspend` `MarchSuspend` `JumpSuspend` `Non` `Hide` `Push` `StopPop` `Pop` `Win` `Turn` `Vertigo` `Advance` `Recede` `JumpAie` `Ouf1a` `Ouf1b` `Ouf3` `Ouf4` `Ouf2` `Ouf5` `StopMarch` `StopJump` `Bye` `Switch` `FireTank` `HelicoGlu` `TakeDynamite` `PutDynamite` `TakeSkate` `DeposeSkate` `Mockery` `Mockeryi` `Mockeryp` `Stop` `DeathLocked.Clear1` `DeathLocked.Clear3` `DeathLocked.Clear4` `DeathLocked.Glu` `DeathLocked.Drown` `PickupBusy.Sucette` `PickupBusy.Drink` `PickupBusy.Charge` 
