# Speedy Blupi: how its demos are recorded in the game (research, 2026-11-08)

Owner's question (2026-11-08): how could Speedy Blupi's attract demos be recorded in the game
itself, by some trick, and what should Galaxy Eggbert offer instead. Source: the blupi_recon
decompilation, `src/event.cpp` (retail Speedy Blupi; line numbers of 2026-11-08), confirmed by
`port/docs/ARCHITECTURE.md` §6.

## What the original does

- **Recording is automatic, by world number.** `CEvent::ChangePhase` (`src/event.cpp:2094-2104`):
  entering `WM_PHASE_PLAY` while not playing a demo, in a world numbered 200-219, calls
  `DemoRecStart()`; leaving Play (any other phase) calls `DemoRecStop()`. `GetWorld()`
  (`:1968`) is the private world's number in the Construction screens (`m_bPrivate`), the
  mission otherwise.
- `DemoRecStart()` (`:3666`): a buffer of `MAXDEMO` (10 000, `def.h:33`) `DemoEvent {short time;
  short input}`, `InitRandom()` (srand), `SetTime(0)`, speed 1. While it records, "REC"
  (`TX_DEMOREC`, `:1365-1370`) is drawn at (10, 10); every change of `m_keyPress` (the KEY_* bit
  mask) is stored with the tick it happened on (`DemoRecEvent`). The mouse is not recorded.
- `DemoRecStop()` (`:3682`) writes `data\demoNNN.blp`, NNN being the world's number: a 204-byte
  `DemoHeader` (majRev 1) and the events.
- Playback (`DemoPlayStart`, `:3712`): `data\demo%.3d.blp` for `m_demoNumber + 200`, the world of
  the same number (`Decor::Read(gamer, 200 + n, FALSE)`: the official folder), music, srand, time
  0, speed 1, then `DemoStep` (`:3784`) feeds the key masks. `DemoStep` starts demo 200 after
  `DEF_TIME_DEMO` (1000) ticks in Init (`:3791`), then 201, ... until a file is missing, then 200
  again.

## The trick: the XMISSION cheat ("xnjttjpo")

Worlds 200-219 cannot be reached in normal play. The cheat table (`:53-77`) begins with
`"XMISSION"`, commented `(xnjttjpo)`: the matcher (`:4228-4230`) adds one to every letter of
cheat 0 after the first, so it is typed **x-n-j-t-t-j-p-o**. It toggles
`m_bBuildOfficialMissions` (`:4236-4243`, listed top-left as XMISSION while on), and with it on:

- the Construction screens edit the **official** missions instead of the player's private
  ones: `Decor::Read/Write/DeleteMission(..., !m_bBuildOfficialMissions)` (`:2230`, `:2321`,
  `:3083`) read and write `data\worldNNN.blp`, not the user's folder;
- the world selector goes up to 219 instead of 19 (`:2421-2422`, `WM_NEXT` `:4724-4741`; Shift
  steps by ten), and the editor's eighth decor menu (`WM_DECOR8`) is shown (`:2397`).

So: type `xnjttjpo`, open Construction, step to world 200-219 (build or edit it there: it is
written as the official `data\world2NN.blp`), play it from its info screen, and leave Play. The
game has written `data\demo2NN.blp`, and the attract mode plays it from then on, in the order of
the numbers. Nothing else is needed; nothing tells the player that this records.

## What Galaxy Eggbert does instead (BD-0116 - BD-0119)

- **Recording a demo, openly** (BD-0116): the pause screen's Record button picks one of the ten
  demo positions, restarts the mission from its start and records it ("Rec" top left, as the
  original's REC). The recording ends when the mission is left (won, lost, Home, Back) or
  after ten minutes, and it is stored in that position. It replaces the shipped demo there for
  this computer's players, the original is kept and restored by the same screen. Recording does not
  change how the game plays: progress is saved as always.
- **Ten shipped demos** (BD-0117): `Content/demos/demo001.gdemo` to `demo010.gdemo`.
- **xnjttjpo** (BD-0118): the cheat keeps its word and its meaning, editing the official
  worlds, but goes straight there: typed in play, it opens the 3D editor on the world being
  played. Saving writes the shipped world file, as XMISSION wrote `data\worldNNN.blp`. The
  editor's Stop returns to the mission, started again with the edits.
- **Each player's own galaxy** (BD-0119): the original's private worlds (Construction), as a
  whole campaign. Each of the three players builds a galaxy: a main hub with world signs, world
  hubs with mission signs and doors, the missions. It is played with the campaign's own hub
  rules and has its own saved progress.
