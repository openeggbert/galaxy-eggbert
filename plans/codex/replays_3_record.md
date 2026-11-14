# Replays 3/3 — Pause > Record saves into "My recordings" (Save under a name / Discard)

The owner (2026-11-14): "Pause > Record a replay; when it ends, Save under a name or Discard. The
ten demo positions stay separate." Parts 1 (format 2: the world, version and date in the file) and
2 (the "My recordings" screen over `SharedDemoDir()`) are committed before this; read their briefs
(`plans/codex/replays_1_format.md`, `replays_2_screen.md`) and commits (`git log -6 --stat`).

Read first: `AGENTS.md`, `CLAUDE.md`, `plans/codex/README.md`, then in
`src/GalaxyEggbertCNA/GalaxyEggbertGameDemo.cpp`: `StartPlayerRecording`, `BeginDemoRecording`,
`FinishDemoRecording`, `EndPlayerRecording`, `deferredMission_` and every caller of
`FinishDemoRecording` / `EndPlayerRecording` (Update.cpp, Missions.cpp, GalaxyEggbertGame.cpp's
exit and deactivation paths), the Pause screen's Record button (`InputPad::UpdatePause`,
`PauseRecordRect`, `CanRecordDemo`), the naming panel (`StartDemoNaming`, `ScreenKeyboard`), and
`plans/audit/owner_questions.md` Q-017 (kept: a pause ends the recording).

## What to build

- **Pause > Record** starts the recording at once: the mission again from its start at the normal
  speed, exactly as `StartPlayerRecording` does today, but with no position — the positions' screen
  is no longer shown from Pause. The "REC" notice as today. Record is offered where it is today
  (`CanRecordDemo`: not in a network match, a play-test, a demo, a replay, an autoplay).
- The recording ends as today: leaving Play (Pause included, Q-017), a mission change, ten minutes
  (`kDemoMaxTicks`), the application's exit.
- When it ends (except the application's exit): the recording is serialized at that moment (its end
  signature at that tick, format 2 with its world, the version and the UTC date: part 1) and held in
  memory, and a **"Save the recording"** panel is shown over the screen it ended on: its name (the
  naming panel's keyboard on the screen and the real keyboard, `kDemoNameMax`, prefilled with a
  readable automatic name such as "Mission 11" / the galaxy mission's name), and two buttons
  **Save** and **Discard** (Enter = Save, Escape = Discard after a second press or a "Discard the
  recording?" confirmation — a lost recording cannot come back; pick one and say which). Gamepad:
  the arrows + A over the keys and the two buttons, as the naming panel allows today.
  - **Save**: written atomically into `SharedDemoDir()` as `<name with '-' for spaces>.gdemo`,
    numbered when taken (reuse part 2's helper), the notice "Saved in My recordings: <name>."
  - **Discard**: nothing written; the notice "The recording was discarded."
  - Ended by Pause: the panel over the pause screen, the pause screen after it. Ended by a mission
    change (exit reached, a death that leaves Play, ...): the panel first; the waiting mission
    (`deferredMission_`) loads after Save / Discard, as it waits for the write today.
  - The application's exit while recording: saved automatically under the automatic name (no panel
    can be shown). Deactivation (Android's background, a lost focus on the desktop) while the panel
    is open: save it automatically under the name typed so far (or the automatic one) and close the
    panel — an unwanted file can be deleted, a lost recording cannot be made again.
- The ten positions are never written by Pause > Record any more; a recording reaches a position
  only through My recordings > Menu demo (part 2). The main menu's Demos screen: a click on a
  position in its plain mode now says "Recordings are made in a mission with Pause > Record and
  kept in My recordings."; its Name / Original / Export / My recordings keep working.
- The developer's `--record-demo` (tools/record_demo.sh) is unchanged: it still writes its file
  directly, without the world, without a panel.
- Every new or changed Tr/TrMark/TrFormat string in all 19 `Translations*.cpp`, translated properly.
- `handoff.md`/`CURRENT.md` are Claude's to update; write your findings into `report.md`.

## Verification (all required)

- Unit tests where the logic is pure (the automatic name, the save name numbering if not already
  covered by part 2, the panel's hit tests and layout at 640x480 / 800x480 / 800x360 in
  `VerifyGEInputPad.cpp`: nothing overlaps, all on the screen).
- Build `build-cna-opengl3 --parallel 4` (CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv); `ctest
  --test-dir build-cna-opengl3 -j4 --output-on-failure` all pass; `tools/verify_demos.sh
  build-cna-opengl3` ten `equal`; `tools/record_demo.sh` still works (record a throwaway demo into
  `build-probe/replays-3/`, verify it with `tools/verify_demos.sh build-cna-opengl3 <that file>`).
- Live (`tools/qa_live.sh`, `QA_LIVE_RUN=build-probe/replays-3/run`): Pause > Record in mission 11,
  play ~5 s, Pause -> the panel -> type a name -> Save -> the file in `demos/shared/` (format 2 with
  the world) -> back to Pause -> Continue playing; again -> Discard -> no file; a recording ended by
  reaching the exit / dying (pick a mission where that is quick, or a probe world via
  `QA_LIVE_WORLDS`) -> the panel before the next mission; then main menu > Demos > My recordings
  lists the saved one and plays it to the end. Screenshots + `report.md` in
  `build-probe/replays-3/`.
- Do not commit, do not push. End with a summary: files changed, choices made, what was not done.
