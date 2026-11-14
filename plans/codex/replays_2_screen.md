# Replays 2/3 — the "My recordings" screen

The owner (2026-11-14): a screen listing the player's collection of recordings with Play, Rename,
Delete and Export, and separately "use as the menu demo -> position". Part 1
(`replays_1_format.md`, committed before this) made recordings carry their world, version and date
and play from their own world copy. Part 3 (`replays_3_record.md`) will make Pause > Record save
into this collection; do NOT do part 3 here.

Read first: `AGENTS.md`, `CLAUDE.md`, `plans/codex/README.md`, then part 1's brief and its commit
(`git log -3 --stat`), `src/GalaxyEggbertCNA/GalaxyEggbertGameDemo.cpp` (the demo positions'
screen: `UpdateDemoSlotsScreen`, `DrawDemoSlotsScreen`, `HandleDemoSlotClick`, `ListDemoImports`,
`ToggleDemoImport`, `StartDemoNaming`/`FinishDemoNaming`, `DemoSlotsFocusTargets`, `StartDemoFile`,
`EndOfDemo`, `StopDemo`), `src/GalaxyEggbert/Game/InputPad.hpp/.cpp` (`UpdateDemoSlots`,
`DrawDemoSlots`, `DemoSlotRect`, `kDemoListRows`), `src/GalaxyEggbert/Game/DemoReplay.hpp/.cpp`,
`src/GalaxyEggbert/Editor/PlatformFiles.hpp`, `FocusNavigator.hpp`, `ScreenKeyboard.hpp`.

## The collection

The collection is the folder that already exists for shared demos, `SharedDemoDir()`
(`demos/shared/` beside savedata.txt, `/save/demos/shared/` on the web): earlier exports and
imports are already there, so they appear at once. It is shared by players A, B and C, as the ten
positions are. In the UI it is called "My recordings".

## The screen

- Opened from the main menu's Demos screen (the ten positions, phase Init): its Import button
  becomes **My recordings** (Import's job — copying a file into a position — moves into the
  screen's "Menu demo" action). Escape / Cancel / gamepad B leave it a step at a time (an open
  naming or position choice first), back to the positions.
- A list of the collection's `.gdemo` files, newest first, `kDemoListRows` (7) a page with the page
  arrows as Import's list has; each row: the name (the title, the file name when the title is the
  automatic one), what it holds (mission N / a galaxy's, duration), the version and date when the
  file has them, and its state when it is not simply playable: "plays from its own copy of the
  world", "older version: may play differently", or why it cannot play (corrupt, world missing).
- A row clicked / tapped / chosen with the arrows + A is **selected** (drawn lit); a bar of buttons
  under the list acts on the selected row: **Play**, **Rename**, **Delete**, **Export**, **Menu
  demo**. On the web also **Add file** (the browser's picker, `RequestUpload(SharedDemoDir(),
  ".gdemo")`, as Import does today); elsewhere the empty list's message says which folder to put
  `.gdemo` files in. With nothing selected the buttons say "choose a recording first".
- **Play**: plays the file once with the demo machinery (the player's settings set aside and given
  back, any new press stops it, deactivation stops it, saving suppressed), then returns to this
  screen with the same row selected — not the next menu demo and not the main menu. A file of
  another simulation revision plays best effort (part 1's opt-in) after the message "Recorded with
  an older version: it may play differently."; when its end differs from the recording, the screen
  says "It played differently than it was recorded." A file that cannot play says why and does
  not start.
- **Rename**: the existing naming panel (keyboard on the screen + the real keyboard, `kDemoNameMax`,
  `SanitizeDemoName`), prefilled with its current name; OK / Enter keeps it: the title in the file
  (`RenameDemo`) AND the file renamed to `<name with '-' for spaces>.gdemo`, numbered when taken
  (`name-2.gdemo` ...), so an export carries the name; Cancel / Escape leaves it.
- **Delete**: two steps (the button, then "Delete <name>? Press Delete again." and Delete again; any
  other action disarms it), then the file is removed and the list refreshed.
- **Export**: on the web `OfferDownload(file, its file name)`; elsewhere the message gives the
  file's full path ("It is the file <path>."): on the desktop that folder is the player's own; on
  Android it is private to the app (say nothing more; a share sheet is a later step).
- **Menu demo**: shows the ten positions (the title "Menu demo: choose the position for <name>");
  a position chosen gets a copy (`ImportDemo`), then back to the list with "Position N plays <name>
  now."; refused with a message for a file of another revision ("A recording of an older version
  cannot be a menu demo.") or one that cannot play.
- Gamepad / keyboard: the arrows move over rows and buttons (`FocusNavigator`, as the positions'
  screen does: `DemoSlotsFocusTargets`), A chooses. Touch: rows and buttons at least as tall as the
  positions' buttons. Layout checked at 640x480, 800x480 and 800x360 (phones): nothing overlaps,
  everything on the screen, long names shrink or are cut with "..." rather than run over.
- Every new or changed Tr/TrMark/TrFormat string in all 19 `Translations*.cpp` (VerifyLocalization),
  translated properly per language.

Keep the code where its kind already lives: geometry/hit tests/drawing in InputPad (pure,
testable), file operations in DemoReplay (rename-with-file, list rows' data), the flow in
GalaxyEggbertGameDemo.cpp. The Demos screen's other modes (record from Pause, Export / Name /
Original of a position) keep working.

## Verification (all required)

- `tools/VerifyGEInputPad.cpp`: the screen's layout (rows, buttons, arrows) at the three sizes: no
  two controls overlap, all on the screen, each control's hit test answers its own rectangle.
- `tools/VerifyDemoReplay.cpp`: rename with the file (numbering when taken, the title inside
  changed, the rest of the file byte-identical, the checksum valid), the list order, delete.
- Build `build-cna-opengl3` with `--parallel 4` (CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv);
  `ctest --test-dir build-cna-opengl3 -j4 --output-on-failure`: all pass; `tools/verify_demos.sh
  build-cna-opengl3`: ten `equal`.
- Live (`tools/qa_live.sh`, its own run folder `QA_LIVE_RUN=build-probe/replays-2/run`, two or three
  recordings put into its `demos/shared/` first, one of them an older-revision copy you make by
  editing a copy's revision line and fixing its footer with the game's own serializer or a small
  helper): the screen at 800x480 and 800x360, a row selected, Play to the end and back to the
  screen, a press stopping Play, Rename (file and title), Delete (both steps), Export (the path
  message), Menu demo into position 4 then the main menu's Demo playing it, the older one's warning,
  the arrows + Enter path. Screenshots and `report.md` in `build-probe/replays-2/`.
- The web build if its tree exists (`build-cna-webgl2/`, see handoff.md for how it is built): it
  must still compile and link; say so if you could not run it.
- Do not commit, do not push. End with a summary: files changed, choices made, what was not done.
