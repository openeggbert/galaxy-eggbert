You work in the git worktree /rv/data/development/github.com/openeggbert/galaxy-eggbert-localization (branch `localization` of Galaxy Eggbert, a C++ 3D remake on the CNA framework). Read AGENTS.md / CLAUDE.md here and in the parent directory first and follow them strictly.

OWNER'S REQUEST (2026-11-12): "the accelerometer in the Setup should be hidden when the device has no accelerometer. CNA can also simulate an accelerometer on desktop, but that would have to be supported in Galaxy Eggbert's code - do not do that."

Today the Setup always shows the Accelerometer toggle and, while it is on, its Sensitivity slider (src/GalaxyEggbert/Game/InputPad.cpp: SetupAccelRect, the label at ~2662, the slider "which 2" at ~242; the in-game PlaySetup shares that code). The game asks CNA whether a sensor exists only when the option is on and play needs it (src/GalaxyEggbertCNA/GalaxyEggbertGame.cpp AccelTurnInput, Microsoft::Devices::Sensors::Accelerometer::getIsSupportedProperty(), cached in accelSupported_/accelSupportProbed_).

DO:
1. Ask Accelerometer::getIsSupportedProperty() once, early (when the game starts, or the first time the Setup opens; it asks the platform each time, so cache it), independent of the saved option, and tell InputPad whether the device has one.
2. Without an accelerometer: the Accelerometer toggle and its Sensitivity slider are not drawn, cannot be clicked or touched, and keyboard/gamepad focus (FocusNavigator targets of the Setup and the in-game Setup) skips them. Leave the rest of the Setup's layout where it is (no reflow), unless the empty slot looks clearly wrong; then say what you did.
3. With an accelerometer (Android phones): exactly as today.
4. A saved `accel` option that is on, on a device without the sensor: keep the saved value (the same save may move to a phone); the game already ignores it there.
5. Do NOT add any accelerometer simulation, keyboard emulation or CNA configuration for one. Read CNA (sibling repo /rv/data/development/github.com/libcna/cna, READ ONLY, never modify it) only to report what getIsSupportedProperty() returns on Linux desktop, Windows, the web and Android, and whether any CNA simulation could make it return true on desktop without Galaxy asking for it.

VERIFY:
- Build: export CCACHE_DIR=/rv/cnaccache CCACHE_BASEDIR=/rv; cmake --build build-cna-opengl3 --parallel 6 (only this existing tree; never create build dirs; never /tmp).
- ctest --test-dir build-cna-opengl3 -j4 --output-on-failure: all must pass. If golden frames change (tools/verify_golden_frames.sh under xvfb-run), report it and do not re-baseline.
- A screenshot of the Setup on this Linux desktop (no accelerometer) before and after, under xvfb: tools/qa_live.sh with QA_LIVE_RUN=build-probe/accel/run and a step file (wait ~25 s for the menu, click the gear at (55,425) in the 800x480 window, wait 3 s, shot); also the in-game Setup if reachable (pause in a mission: tools/qa_live.sh ... --mission 11, Escape, the pause's Setup button). Save before/after PNGs under build-probe/accel/. The game log should say the accelerometer is not present.
- Keyboard focus in the Setup (arrow keys) must not land on the hidden slots: show it in a screenshot or a log.
Do not touch any connected Android device (the reviewer tests on the phone). Do not commit. No AI attribution. When done, print a summary: changed files, what CNA reports per platform, the evidence.
