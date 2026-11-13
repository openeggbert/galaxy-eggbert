# Brief: the cheats' keyboard on the screen (the owner, 2026-11-13)

## What the owner asked

During play, when the player **holds the mouse button or a finger in the middle of the screen**,
within a circle around the centre, for **at least 2 seconds**, a keyboard appears on the screen on
which he can type cheats (a phone or a tablet has no keyboard: today the typed cheats, `back`
included, are out of reach there). "Something like it is already in Mobile Eggbert."

"Holding the mouse" means the left button held down (as a finger), never a cursor merely resting
there: with keyboard play the cursor often sits in the middle and must not open anything.

## What Mobile Eggbert does (MODERN build only; read only)

`../mobile-eggbert/src/WindowsPhoneSpeedyBlupi/InputPad.cpp:44-55, 274-330, 456-575` (and its Draw):
a virtual keyboard, opened by holding the **top-left corner** (20 % x 10 % of the larger screen
dimension) for **one second**; a hold opens it once (it must be released before it counts again).
Rows: F5-F8 left and F12 right on top, then QWERTY letters in three rows (the second and third
shifted by half and one key), and a close key at the top right. While it is visible **every**
touch/click goes to it and none reaches the game's buttons; a tapped key is injected as a key
press for that frame (`virtualKeysPressedThisFrame`), handled exactly as a physical key, so the
typed-cheat matcher sees its letters. The game keeps running underneath (no pause).

Galaxy follows that, except where the owner said otherwise: **the centre of the screen, a circle,
2 seconds** (the top-left corner holds Galaxy's cheats' list and notices).

## Design

- **Where it lives.** Engine-independent logic in `src/GalaxyEggbert/Game/` (InputPad or a small
  class beside it): the hold detector, the layout, the hit-testing, the drawing (reuse the style
  and drawing of `ScreenKeyboard`/`DrawPanel` where it fits; a separate layout is fine, the
  name-entry `ScreenKeyboard` itself must keep working unchanged for the galaxy/demo/nickname
  screens). The host only feeds it and forwards what it produces.
- **Activation.** Play phase only, no fade, not paused, not while the help, the F12 cheat menu or
  another overlay is up, not in a demo playback, not in the editor. Cheats compiled in
  (`kCheatsEnabled`) and not locked for the run (`honest`); with only `fairplay`'s lock it still
  opens (typing `fairplay` unlocks). A press inside a circle at the screen's centre (radius about
  10-12 % of the smaller viewport side; pick and document) that **stays inside it** for 120 ticks
  (2 s), from the mouse's left button or from any one touch, **not** a press that an on-screen play
  control claimed (`inputPadClaimedMouse` / `UpdatePlayTouches`). One hold opens it once; the
  pointer must lift before a new hold counts, and the press that opened it does not type a key.
  The camera drag-look shares the same press: a still hold does not turn the camera, so nothing
  needs suppressing before the 2 s; once open the press must not keep dragging the camera.
- **While open.** Every touch and left click goes to the keyboard (none reaches the play controls
  or the camera drag), as the original. A key fires once per press (on press or on release; say
  which, avoid double letters from a held finger). Keys: the letters a-z as the original's QWERTY
  rows; the top row with Galaxy's equivalents of the original's function keys -- check what F4-F8
  and F12 do in Galaxy (`GalaxyEggbertGameUpdate.cpp`, speeds and the F12 cheat menu) and offer the
  ones that exist; a close key. Physical keyboard and gamepad keep working. Escape/Pause or the
  phase leaving Play closes it. The game keeps running underneath (as the original).
- **Gamepad (the owner asked for it too, 2026-11-13).** Every gamepad button is taken in play
  (`GamepadInput.hpp` `GamepadKeys()`: A jump, B/X action, RT fire, RB back up, LB crouch, Y look up,
  Start/Back pause) except the **left trigger** and the right stick. Opening: **hold LT for 2 s**
  (120 ticks; the same once-per-hold rule as the pointer). The gamepad reaches the game only as the
  keyboard keys `GamepadKeys()` maps it to (that is what `ResumeLog` records), so give LT its own
  `GamepadKey` bit mapped to a keyboard key that nothing else uses (check the whole host: not F9,
  which is CNA's debug context loss, not Tab or F-keys the demo filters, not a key used in play or
  menus; say which) and detect the hold from that recorded key, so a replay reproduces it. A
  physical press of that key may then open it too; that is fine. While open: the d-pad / left stick
  (and the keyboard's arrows) move a highlighted key (the existing `FocusNavigator`, as the
  nickname's `ScreenKeyboard` is navigated), A (and Enter) presses it, B closes, LT held again
  closes it too if simple; none of these reach Blupi while it is open (A must not jump, the stick
  must not walk). Draw the highlight visibly. The control help (F1 / the controls' help screen) gets
  one line for it if there is a gamepad section.
- **Letters go the same way as typed ones.** They must reach `InputPad::UpdateTypedGhostCheat` as
  ordered letters, so every typed cheat (including `back`, `fairplay`, `help`) works, the
  gameplay-letter suppression is respected, and the F-keys act as their physical keys.
- **Determinism (important).** `ResumeLog` records each tick's raw input (touches, mouse, buttons,
  viewport, ordered cheat letters) and `back`/Continue replay it (`plans/codex/back_cheat.md`). The
  keyboard must therefore be driven **inside the recorded tick from `tickInput_`**, not from live
  device state outside it, so that replaying a log reproduces the keyboard opening, the keys
  tapped and the cheats they typed. Its state (hold counter, visible, pressed key) must be reset by
  `ResetMissionInput()` so a log's start and a replay's start match. Add it to the in-process check
  signature (`ResumeCheckSignature()` in `GalaxyEggbertGameDemo.cpp`) if it is not derivable.
- **Drawing.** Over the play view, above the HUD, readable on a phone (keys large enough to hit with
  a finger at 360 px viewport height); the original draws it at the top across the width. Keep the
  top-left cheats' list readable or draw over it -- choose and say. A label for the close key: an
  existing translated word (`Tr(...)`, look in `Translations*.cpp`, e.g. "Close"/"Cancel") or a
  symbol the fonts have; no new strings unless you add them to every language file.
- **Docs.** `CHANGELOG.md` (`### Added`), a short `CURRENT.md` entry, and a row in
  `plans/audit/behavior_differences.md` recording the owner's difference from the original
  (centre and 2 s instead of the top-left corner and 1 s). The `help` list may mention it in one
  line if that list has room. Do not edit `handoff.md`.

## Verification

- Pure-logic tests in the existing verifiers (`tools/VerifyGEInputPad.cpp` or a new one registered
  in CMakeLists.txt like its siblings): no opening before 120 ticks, opening at 120; leaving the
  circle resets; a press claimed by a play control never opens it; one hold opens once; a tap on a
  key yields exactly that letter once; the close key closes; while open no touch reaches the play
  controls; the reset clears it. The gamepad: LT's key held 119 ticks does not open, 120 does;
  the d-pad moves the highlight, A types the highlighted letter once, B closes; while open A and the
  stick give Blupi nothing; `GamepadKeys()` maps LT to its key.
- A replay check: a log in which the keyboard is opened and a cheat is typed on it (e.g. `zoom`),
  then `back` or `GALAXY_RESUME_SELFTEST`: equal. If the autoplay scripts cannot press the pointer,
  add a minimal autoplay command for a held/tapped pointer at screen coordinates
  (`src/GalaxyEggbert/Game/Autoplay.hpp`, documented in its header, covered by `VerifyAutoplay`).
- Build only in the existing tree: `export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv;
  cmake --build build-cna-opengl3 --parallel 4` (never more than 4 jobs, no new build directories,
  nothing in /tmp), then `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: all pass.
  Golden frames must stay unchanged (the keyboard is not shown in them).
- Live: under Xvfb, silent (`SDL_AUDIODRIVER=dummy`), hold the mouse in the centre for 2 s, type a
  cheat on the keyboard, close it; and the gamepad path (SDL's virtual gamepad was used before in
  this repo, see CURRENT.md, or an autoplay `hold` of LT's key and the arrows): open with LT, type a
  cheat with the d-pad and A; screenshots in `build-probe/cheat-keyboard/` (closed, open, the
  cheat active), and one at a phone-like size (e.g. 800x360). Runs under `AUTOPLAY_RUN` or a
  directory inside `build-probe/cheat-keyboard/`; remove the run directories when done.

## Rules

Do not commit or push. Do not modify anything outside this repository (`../cna`, `../cna-engine`,
`../mobile-eggbert` are read only). Another agent session may be working in this repository at the
same time: touch only the files this task needs and do not revert changes you did not make.
Report: the files changed, the radius and key-firing choices, how determinism is kept, the test
and replay results, the screenshots' paths, and anything left open.
