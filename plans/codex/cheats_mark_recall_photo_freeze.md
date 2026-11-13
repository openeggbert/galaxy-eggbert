# Brief A: cheats `mark`, `recall`, `photo`, `freeze` and the cheats' help in two pages (the owner, 2026-11-13)

The owner asked for seven new MODERN typed cheats (not in the original, like `ghost`, `zoom`,
`back`) -- `mark`, `recall`, `photo`, `freecam`, `xray`, `hitbox`, `freeze` -- and for the cheats'
help screen (typed `help` / `cheats`) to have two pages the player switches between, since one page
is too full now. This brief is the first half: **the two pages, `mark`, `recall`, `photo`,
`freeze`**. `freecam`, `xray`, `hitbox` follow in a second brief; leave room for them on page two.

Read first: `plans/codex/back_cheat.md` (the rewind through the resume replay; `RewindBack()`,
`BeginResume(log, inProcess)`, `resumeLocalCheats_`, `ResumeCheckSignature()`,
`GALAXY_BACK_CHECKPOINTS`), `src/GalaxyEggbertCNA/GalaxyEggbertGame.cpp` `ApplyTypedCheat()` /
`TurnCheatsOff()`, the typed-cheat handling in `GalaxyEggbertGameUpdate.cpp` (`UpdateScreenPlay`),
`InputPad.cpp` `TypedCheatWords()` and `DrawControlsHelp()`, `ControlInput.hpp` `ControlsHelp`.

## The cheats

Every new word: an entry in `TypedCheatWords()` with a one-line English description (the cheat list
stays English, Q-023), a `TypedCheat` value marked MODERN, matching that does not collide with
another word (check prefixes/suffixes as `VerifyGEInputPad` does for `back`). Persistent toggles
appear in the active-cheats list top left (as `fps`), are turned off by `fairplay`/`honest`
(`TurnCheatsOff`), and are part of `resumeLocalCheats_` (captured at the log's start, restored for an
in-process replay) and of `ResumeCheckSignature()`, so `back`, `recall` and the self-tests stay
exact.

### `mark` and `recall`

- `mark` remembers this moment of the current world: the mission log's tick count
  (`resumeLog_.TickCount()` after this tick is appended). Typing it again moves the mark. Notice
  `mark` (ShowDemoNotice, as `back -5 s`).
- `recall` goes back to the marked moment through the same replay as `back`: generalise
  `RewindBack()` into a rewind to a target tick (`DropLastTicks(TickCount() - target)`), used by both.
  Notice `recall` when the replay finishes (the progress screen as for `back`). The mark stays: the
  player can recall again and again.
- Same conditions as `back` for both (single-player Play, a recording log, not in a replay, demo,
  demo recording, editor play-test, network match, fade or exit). Neither acts during a replay: the
  mark is the host's memory, not part of the simulation, so replaying a log with `mark` typed in it
  leaves the remembered tick alone.
- The mark belongs to this world's timeline: cleared by a new world (`StartResumeLog()`), by a
  Continue from the file, and when a `back`/`recall` drops the ticks it lies in (target < mark),
  then `recall` says `recall: no mark` and does nothing. Without a mark the same notice.

### `photo`

A toggle that hides everything drawn over the world in Play: the HUD (lives, treasures, keys,
counters), the on-screen touch controls, the active-cheats list and notices, the training hints,
the compass, the `position` axes and readings, the fps reading, the debug panel. The world and
Blupi stay as they are. **The cheats' keyboard (centre hold / LT) and the help still draw**, so a
phone or gamepad player can type `photo` again; Pause and the other screens are unaffected.
Display only: no effect on the simulation. It is listed as `photo` in the active list (hidden while
it is on, naturally).

### `freeze`

A toggle: every MoveObject stops its own motion and animation -- enemies and their patrols and AI,
lifts and moving platforms, projectiles, explosions in progress, doors sliding, falling objects --
while Blupi moves freely. What Blupi does to objects still happens (picking up, pushing a crate,
riding, a pickup's voyage to the HUD), and contact rules are unchanged (a frozen enemy or shot still
hurts). The world's tile animations (lava, water, lightning) keep their timing: only objects freeze.
Find the single place(s) where objects advance (`InteractionSystem::Update`, `AdvancePatrolStep`,
the phase/time advance) and gate them; do not scatter checks. Deterministic (toggled by recorded
letters inside the tick). Refused in a network match (the others' worlds would differ), with one
stdout line, as `back`'s refusals.

## The help in two pages

`ControlsHelp`/`DrawControlsHelp(cheats=true)`: two pages, shown as `1/2`, `2/2` with arrows at the
top or bottom of the panel.
- Page 1 -- "Speedy Blupi / Mobile Eggbert": the original's words (`opendoors` ... `weelkeys`,
  `xnjttjpo`) and Mobile Eggbert's own MODERN ones (`quick`, `ghost`, `zoom`, `debug`, `cheats`;
  check `../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/InputPad.cpp:700-745`, read only).
- Page 2 -- "Galaxy Eggbert": the rest (`help`, `compass`, `fairplay`, `fps`, `position`, `honest`,
  `back`) and the new ones, with room for `freecam`, `xray`, `hitbox` (brief B).
  Derive the split from a field of `TypedCheatWord` (its origin), not from a hard-coded index.
- Switching: Left/Right arrows and A/D, PageUp/PageDown, the gamepad's d-pad left/right and LB/RB
  (they arrive as keys, `GamepadInput.hpp`), and a click/touch on the page arrows. Opening the
  cheats' help starts on page 1; F1's controls help is unchanged. Closing as today. The help is
  driven inside the tick from recorded input (as now), so it stays deterministic.
- Layout: the 640x480 reference panel; each page must fit with readable sizes (no text over the
  page arrows or the close button) at 800x480 and 800x360.

## Verification

- `VerifyGEInputPad`: every word fires once and has a description; no collisions; the page split
  (every word on exactly one page, page 1 = original + Mobile Eggbert); page switching by each input.
- Rewind probes as `tools/autoplay/probes/back_*.txt` with `GALAXY_BACK_CHECKPOINTS`: `mark`, play
  on, `recall` -> equal to the marked tick; `recall` twice; `mark`, `back` to before it, `recall` ->
  `recall: no mark` and nothing changes; `freeze` toggled then `back`/`recall` across it -> equal;
  `photo` toggled then `back` -> equal (its flag restored). A `freeze` probe that shows a patrolling
  enemy or a lift standing still for N ticks while Blupi walks, then moving again after `freeze`.
- `GALAXY_RESUME_SELFTEST` equal with `freeze` and `photo` typed during the run (mission 012).
- Build only in the existing tree: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4` (never more than 4 jobs, no new build directories,
  nothing in /tmp, no compiler cache of your own), then
  `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: all pass, goldens unchanged.
- Screenshots in `build-probe/cheats-a/`: both help pages (800x480 and 800x360), `photo` on and
  off in the same view, a frozen enemy/lift. Silent runs (`SDL_AUDIODRIVER=dummy`) under
  `AUTOPLAY_RUN=build-probe/cheats-a-run`; remove run directories when done.
- Docs: `CHANGELOG.md` (`### Added`), a short `CURRENT.md` entry. Do not edit `handoff.md`.

## Rules

Do not commit or push. Do not modify anything outside this repository (`../cna`, `../cna-engine`,
`../mobile-eggbert` are read only). Another agent session may be working in this repository at the
same time: touch only the files this task needs and do not revert changes you did not make.
Report: the files changed, where freeze gates the objects and what it covers, how photo decides
what to hide, the mark's semantics as implemented, the tests and probes with results, the
screenshots' paths, and anything left open.
