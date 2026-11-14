# Brief D: cheats `step`, `practice`, `replay`, `warp` (the owner, 2026-11-14)

The owner asked for six more MODERN typed cheats: `step`, `perf`, `cascades`, `practice`, `replay`,
`warp`. This brief is the first part: **`step`, `practice`, `replay`, `warp`** (they touch the
timeline and the flow of play). `perf` and `cascades` follow in brief E
(`plans/codex/cheats_perf_cascades.md`); leave room for them on help page 2.

Read first, and follow their conventions exactly:
- `plans/codex/back_cheat.md`, `cheats_mark_recall_photo_freeze.md`, `cheats_freecam_xray_hitbox.md`,
  `cheats_restart_snap_here_trail_ids_sounds_chunks.md` and their code (commits 2ab10f89, 7f3a1561,
  584fbddf, 3bb31d46): `ResumeLog`, `RewindToTick()`, `BeginResume(log, inProcess)`,
  `TimelineCheatRefusal()`, `resumeLocalCheats_`, `ResumeCheckSignature()`, the optional start
  records (`OBJECT_CHEATS`, `DISPLAY_CHEATS`, `DIAGNOSTIC_CHEATS`), the freecam hold gate in
  `UpdateTick`, `GALAXY_BACK_CHECKPOINTS` / `GALAXY_RESUME_SELFTEST` probes in
  `tools/autoplay/probes/`, `CheatKeyboard.hpp` (`cecde12c`, `fdb43082`).
- Every new word: `TypedCheatWords()` entry with an English one-line description, origin "Galaxy
  Eggbert" (help page 2), a MODERN `TypedCheat`, no prefix/suffix collisions (`VerifyGEInputPad`).
  Persistent toggles: in the active-cheats list, off with `fairplay`/`honest` (`TurnCheatsOff`), in
  `resumeLocalCheats_` and `ResumeCheckSignature()` (typed letters are replayed by
  back/recall/restart/Continue: a toggle missing from the captured start state is flipped twice).

## `step`

A toggle. While on, the simulation holds (as `freecam`'s gate in `UpdateTick`: nothing of the world
steps) **except** on a tick in which the step key is newly pressed: that tick runs normally, with
the keys held in it (hold Right and press the step key: he walks one tick to the right). Holding the
step key for more than half a second steps every tick at a reduced rate (e.g. 10 ticks a second) --
say what you chose. The step key is **F2** (free in play: check nothing reads it), read from the
recorded tick input so a replay reproduces the stepping exactly. Add F2 to the cheats' keyboard in
the gap of its top row between Tab and F12 (`CheatKeyboard::Layout`), so touch and gamepad players
can step too; route it like the keyboard's other F-keys. The camera's drag-look, Pause, help, typed
cheats and the keyboard keep working while held. Shown in the active list as `step` with the world
log's current tick (`step 1234`). Off on a new world? Decide (it is a tool; keeping it across a door
seems right) and say. Refused in a network match. With `freecam` on, freecam's hold wins.

## `practice`

A toggle. While on, a death does not cost a life: at the moment Blupi dies (find the single place
where a death is decided and the life is taken -- `InteractionSystem`/the host's death handling,
`UpdateTickEvents`), the game instead rewinds 5 seconds (300 ticks) before that moment through
`RewindToTick()`, with the notice `practice -5 s`; less than 5 s into the world goes back to its
start. No death animation, no Lost screen, no life lost. The same conditions as `back`
(`TimelineCheatRefusal()`): in a network match, a demo, a recording or a play-test practice does not
act and deaths are ordinary. Never during a replay (a log never contains a death that practice
rewound, since the rewind drops it). Make sure a death that is unavoidable within 5 s cannot loop
forever without the player regaining control (he regains it at T-300 each time; say what happens).

## `replay`

An instant replay of the last 10 seconds: the game goes back 600 ticks (fast, as `back`, with the
progress screen if long) and then **plays the dropped 600 ticks visibly at normal speed** from the
original log's tail, live input ignored, then hands control back at exactly the moment `replay` was
typed (the same state: deterministic replay; prove it with a checkpoint). While it plays: a
`REPLAY` label on screen, sounds play, the camera is the recorded one. A key or tap (Escape, A,
Enter, a click) skips the rest: the remaining ticks run fast and invisibly to the same end. The ticks
replayed visibly are appended to the log as they play, so the log ends identical to the original.
During it, side-effect cheats in the replayed letters (`snap`, `back`, `mark`, `recall`, `restart`,
`replay`, `warp`, `practice`'s rewind) must not act -- extend the existing replay guards (one flag
for "a replay is running", fast or visible, not scattered checks). Same conditions as `back`.

## `warp`

Opens a panel listing the worlds the player can go to directly: the campaign's missions and hubs
(the files in `worlds3d/` that the campaign uses; in a player's galaxy, that galaxy's worlds),
grouped by world (hub 10: 11-18, ...), with their numbers (and names if the game has them -- do not
invent names). Chosen with mouse/touch, the arrows/d-pad and A/Enter (the existing
`DrawPanel`/`FocusNavigator` panels, as the galaxy and demo screens), Escape/B closes. The game holds
while it is open (as help). Choosing one loads that mission as entering it through its door would
(`LoadMission`, the arrival place as usual) -- **without changing the player's progress** (doors,
saved mission, lives stay as they are; say how you made sure nothing is saved). Refused in a network
match, a demo, a recording, a play-test. The panel is driven inside the tick from recorded input
(as help), so back/Continue replays of the time before a warp stay exact; a warp starts a new
world log as any mission start does.

## Verification

- `VerifyGEInputPad`: the four words fire once, have descriptions, page 2, no collisions; page 2
  still fits at 800x480 and 800x360 (reflow if needed); F2 in the keyboard's layout. Pure-logic
  tests where possible (step gate decisions from key edges and the hold rate; the warp list built
  from a set of world files; replay's tail bookkeeping).
- Probes in `tools/autoplay/probes/` with `GALAXY_BACK_CHECKPOINTS` / `GALAXY_RESUME_SELFTEST`
  (mission 012 unless a probe needs another): `step` with held keys and N presses then off -> the
  state equals N world ticks; `practice` with a death (see `death_once.txt`, mission 999 with
  `--spawn 41,3,5,0`) -> lives unchanged, state equal to 300 ticks before the death; `replay` ->
  after it the state equals the state when `replay` was typed, and the log's tick count equals the
  original; `replay` skipped halfway -> same end; `warp` from mission 012 to another -> that world
  loaded, savedata unchanged byte for byte. Each also with a `back` or self-test across it.
- Build only in the existing tree: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4` (never more than 4 jobs, no new build directories,
  nothing in /tmp, no compiler cache of your own), then
  `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: all 136+ pass; golden frames and
  the golden trace unchanged.
- Screenshots in `build-probe/cheats-d/`: `step` with its tick in the list, the `REPLAY` label
  mid-replay, the warp panel at 800x480 and 800x360, help page 2. Silent runs
  (`SDL_AUDIODRIVER=dummy`) under `AUTOPLAY_RUN=build-probe/cheats-d-run`; remove run directories
  when done.
- Docs: `CHANGELOG.md` (`### Added`), a short `CURRENT.md` entry. Do not edit `handoff.md`.

## Rules

Do not commit or push. Do not modify anything outside this repository (`../cna`, `../cna-engine`,
`../mobile-eggbert` are read only). Another agent session may be working in this repository at the
same time: touch only the files this task needs and do not revert changes you did not make. Never
add AI attribution anywhere. Report at the end: the files changed, where each cheat hooks in, the
choices left to you above, the tests and probes with their results, the screenshots' paths, and
anything left open. Write the report also to `build-probe/cheats-d/report.md`.
