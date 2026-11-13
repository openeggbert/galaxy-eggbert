# Brief: the typed cheat `back` -- the game 5 seconds back (the owner, 2026-11-13)

## The owner's decisions

- A new typed cheat, the word **`back`**: the game goes back **5 seconds** (300 ticks of 1/60 s).
  Typing it again goes another 5 s back (no menu of lengths).
- While the game rebuilds the earlier moment, something on screen must show that it is working, so
  the player does not think the game froze. The same indication for **Continue**'s replay of an
  interrupted mission (it uses the same replay and today also shows nothing).
- It is a MODERN cheat (not in the original), like `ghost`, `zoom`, `fps`, `position`, `honest`.

## The mechanism (decided: reuse the resume replay, no state snapshots)

The game already records every tick's raw input since the world's start (`ResumeLog`,
`src/GalaxyEggbert/Game/ResumeLog.hpp`; `StartResumeLog()`, `BeginResume()`, `ReplayBatch()`,
`CheckResumeSelftest()` in `src/GalaxyEggbertCNA/GalaxyEggbertGameDemo.cpp`; the append in
`GalaxyEggbertGameUpdate.cpp` around `resumeLog_.Append(tickInput_)`). Replay is deterministic:
`GALAXY_RESUME_SELFTEST=<tick>` reloads in-process and compares `ResumeSignature()`; 77/78 campaign
missions report equal. Measured on 2026-11-13: 13 000 ticks replay in 0.23 s here (mission 025).

`back` = a copy of the current log with its last 300 ticks dropped, then `BeginResume(copy)`.

### 1. `ResumeLog::DropLastTicks(std::uint64_t n)` (engine-independent)

Removes the last `n` ticks from the run-length encoded `runs` (shorten the last run, remove runs
that become empty, across as many runs as needed); `n >= TickCount()` leaves no runs. Returns the
number of ticks removed. Cover it in `tools/VerifyAutoplay.cpp`: inside one run, exactly at a run
boundary, across several runs, `n = 0`, `n > TickCount()`, `TickCount()` afterwards, and a
Serialize/Parse round trip of a trimmed (non-empty) log.

### 2. The word (`src/GalaxyEggbert/Game/InputPad.hpp/.cpp`)

`TypedCheat::Back`, word `"back"`, help line in the same style as the others, e.g.
`"Go back 5 seconds (again: further)"` (the cheat help is English only). Mark it MODERN in the enum
comment. The matcher takes the longest word the buffer ends with; no other word ends in or starts
with these letters, check that this stays true. The fairplay/honest lock applies as to every word.

### 3. The host (`GalaxyEggbertGame`)

- `ApplyTypedCheat(TC::Back)`: only when the rewind is possible -- cheats enabled, phase Play, no
  fade-out, `recordingResume_ && !missionStartPending_`, not `replayingResume_`, not `demoActive_`,
  not `demoRecording_`/`demoRecordArmed_`, not `editorPlayTestActive_`, not `NetMatchActive()`, not
  after the exit was reached / Lost. Then it only sets a pending flag (as `xmissionPending_`).
  Otherwise nothing happens (one stdout line saying why). **During a replay it must never act**
  (a `back` that was refused live and stayed in the log must stay a no-op when replayed).
- Perform it in `Update()` **after** this tick was appended to `resumeLog_` (so the tick that
  completed the word is among the 300 dropped): copy the log, `DropLastTicks(300)`,
  `BeginResume(std::move(copy))`. Fewer than 300 ticks in the world: back to the world's start.
  `BeginResume` validates the world file before changing anything; if it refuses (the file changed
  on disk), the game goes on unchanged and a notice says it was not possible. It also deletes
  resume.txt in that case; that is acceptable (rewritten at the next deactivation) -- say if not.
- **State that the replay does not rebuild in-process.** `BeginResume()` restores what
  `ResumeStart` records (quick, opendoors, showsecret, compass, debug, the cheat lock, camera mode,
  speed, lives, place, seed, settings), then the replay re-applies every typed letter. Anything a
  typed cheat or the F12 cheat menu toggles that is NOT reset by `BeginResume()`/`LoadMission()`
  would be toggled a second time on top of its current value: e.g. `cheatZoomStep_`, `cheatFps_`,
  the `position` toggle, `activeCheats_`, Blupi's SuperBlupi (`megablupi`), ghost, shield/vehicle
  if they survive a load. Audit every member `ApplyCheat()`, `ApplyTypedCheat()` and the typed-cheat
  handling in `GalaxyEggbertGameUpdate.cpp` (~3530-3670) change, and which of them survive
  `LoadMission()`. Capture those values in memory in `StartResumeLog()` and put them back before an
  in-process replay starts -- for `back` **and** for the in-process `GALAXY_RESUME_SELFTEST` (so the
  self-test proves it). A Continue in a fresh process keeps today's behaviour. Also make sure the
  typed-cheat buffer (`typedCheatBuffer_` and its companions in `InputPad`) is the same at the log's
  start and at the replay's start (e.g. `ResetMissionInput()` clears it on both sides); say what you
  chose.
- After a `back` replay finishes: the notice `back -5 s` (`ShowDemoNotice`, drawn in the cheats'
  list for 4 s; cheat words are untranslated).
- Sound: the replay is already muted. Check what happens to the music (`UpdateMusic()` in
  `BeginResume`): if the same world's track restarts from its beginning, keep it playing instead,
  if that is simple; report either way.

### 4. The progress screen (back and Continue)

Today `Update()` runs `ReplayBatch()` (600 ticks per Update) and calls `SuppressDraw()`, and
`Draw()` returns at once while `replayingResume_`: the last frame stays on screen.

- Record the real time the replay began (steady clock) and its total ticks. For the first **0.25 s**
  keep suppressing the draw (a short replay finishes without anything flashing). After that, draw
  only the replay screen: the Galaxy loading screen's look (`InputPad::DrawGalaxyLoading(...,
  overTitleScene=false)`: the logo, "Loading" -- already translated in every language -- and the
  gauge), with the gauge filled by the **real** fraction replayed (ticks done / total) instead of the
  wait table's time curve. Add an optional progress parameter or a sibling function; the start-up
  loading screen must not change. Clear the screen first; draw nothing of the world (it is
  mid-replay).
- Make sure `Draw()` is really reached during a long replay with CNA's fixed time step. If one
  Update of 600 ticks could take much longer than a frame on a slower machine, bound the batch by
  time as well (e.g. stop after ~12 ms, at least one tick); the result does not depend on batching.
  Report what you chose.

### 5. Docs

`CHANGELOG.md` under `## [Unreleased]` (`### Added`), a short entry in `CURRENT.md`. Do not edit
`handoff.md` (the reviewer does).

## Verification

- Build only in the existing tree: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --target GalaxyEggbertCNA --parallel 4` and the verifier targets
  you touch (never more than 4 jobs, no new build directories, nothing in /tmp), then
  `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: everything must pass.
- Autoplay probes in `tools/autoplay/probes/` (run with `tools/autoplay.sh`; set
  `AUTOPLAY_RUN=build-probe/back-cheat-run` so other runs are not disturbed; silent by default):
  1. walk a known route, `type back`, and prove the state after the replay equals the state 300
     ticks before the word was completed (e.g. `trace` the positions every tick and compare the
     exact floats; or add an env-gated self-check that stores `ResumeSignature()` at that tick and
     compares after the replay -- as `GALAXY_RESUME_SELFTEST` does). Print the result.
  2. `back` twice in a row (10 s).
  3. `back` less than 5 s into the world (back at the start).
  4. a toggle cheat typed during the mission (`zoom`, `megablupi`, `ghost`) and then `back`: the
     toggles keep the value they had at the moment that is restored (not flipped twice).
  5. a death within the last 5 s (`mortal`, see `death_once.txt`): after `back` the life is back.
- The existing resume self-tests still report equal (missions 012 and 025, as in
  `plans/audit/behavior_differences.md` BD-0077).
- The progress screen seen: make a replay long enough to show it (e.g. a long log in mission 025,
  or an env knob that slows the replay for the capture only) and save a screenshot of it under
  `build-probe/back-cheat/`; describe it.
- Delete `build-probe/back-cheat-run/` when done (keep `build-probe/back-cheat/` screenshots).

## Rules

Do not commit or push. Do not modify anything outside this repository (`../cna`, `../cna-engine`,
`../mobile-eggbert` are read only). Another agent session may be working in this repository at the
same time: touch only the files this task needs and do not revert changes you did not make. Report:
the files changed, the state you capture/restore and why, the batching choice, the test and probe
results, the screenshot's path, and anything left open.
