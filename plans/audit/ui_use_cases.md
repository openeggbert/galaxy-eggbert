# The original's screens and buttons (Game1.cpp, InputPad.cpp) against Galaxy's

The gameplay ledger (`gameplay_use_cases.csv`) enumerates Decor.cpp; this one covers the rest of the
original's flow: `Def::Phase` (13) and `Def::ButtonGlyph` (44), from `mobile-eggbert/include/
WindowsPhoneSpeedyBlupi/Def.hpp` and their handling in `Game1.cpp` / `InputPad.cpp` (2026-11-04).

Every phase is named and handled in Galaxy (`Def::GamePhase`). Of the buttons, 32 carry the same name
in Galaxy's source; the 12 that do not:

| Button | Original | Galaxy | Status |
|---|---|---|---|
| InitGamerA/B/C | the three gamer slots on the main menu (Game1.cpp:282, SetGamer) | `InitInput::gamerSelected` -> `SaveData::SetSelectedGamer` (the main menu's A/B/C buttons, pad icons 4-6/16-18) | CONFIRMED_MATCH (other names) |
| Cheat12/21/22/31 | four of the six invisible gesture zones (`cheatGeste`, 10 taps) that open the cheat menu | `InputPad::UpdateCheatGesture`, the sequence {12,22,32,12,11,21,22,21,31,32} | CONFIRMED_MATCH (other names) |
| Cheat6 | the cheat menu's "simulate trial mode" | none | NOT_APPLICABLE (no store, no trial mode) |
| TrialBuy, TrialCancel | the Windows Phone trial's purchase screen | none | NOT_APPLICABLE (no store) |
| RankingContinue | the Xbox Live ranking screen's return | none | NOT_APPLICABLE (no ranking service) |
| PlayDown | Down while held, pad icon 23; shown only with the accelerometer on, where the stick is hidden (InputPad.cpp:163, 943, 1754) | the crouch button beside Jump, always (Galaxy's stick walks forward and back, so crouching had no touch input at all: only Left Shift and the gamepad's LB) | FIXED_AND_VERIFIED 2026-11-04 (`VerifyGEInputPad`; live `plans/evidence/touch-crouch-button.png`) |

Also from InputPad.cpp: the Setup's accelerometer sensitivity slider (`accelSlider`, GameData data[7],
0..100, 50 by default; drawn and live only while the accelerometer is on, InputPad.cpp:250, 932, 1172)
was missing (the sensitivity was a fixed 50). Now in the Accelerometer row, saved as
`accelSensitivity=`, read by the tilt's dead zone. FIXED_AND_VERIFIED 2026-11-04 (`VerifyGEInputPad`;
live `plans/evidence/setup-accel-sensitivity.png`). The tilt itself was not tried on a device.
