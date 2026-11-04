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

From Decor.cpp's drawing of Blupi (782-818), found by the tables Galaxy did not cite: the shield's
orb and sparkles (table_shield_blupi, table_shieldloop), the power's stars (table_magicloop) and the
Cloud's three sparks (explo.png 48-53) drawn on him while the power lasts, flashing every other pair of
ticks once the count is down to 25, were not drawn (only the trails left behind him were).
FIXED_AND_VERIFIED 2026-11-04 (`GalaxyEggbertGame::BlupiAuraShown`; camera-facing sprites a little
behind him, as the original draws them before him; live in probe worlds 987-989,
`plans/evidence/power-auras.png`). The drink's rising bubbles over the bottle (table_drinkeffect /
table_drinkoffset, Decor.cpp:882-895: three bubbles rising 3 px a tick for 50 ticks, fading) are drawn
too since 2026-11-04 (the fade in five steps; probe world 986, `plans/evidence/drink-bubbles.png`).

## Tables.cpp

Every `Tables::table_*` (101) checked for a use in Galaxy (2026-11-04). Cited by name or covered under
another name: the creatures' right/turn tables (ObjectIcons.cpp names them as "left/right/turn2l/turn2r"),
the animated tiles' tables (`BlockDefinitionRegistry`, `TerrainAnimDivisor.hpp`: water, crusher, trap,
fans and wind), the invert sign's (its model's frames). Not used before and now done: the powers' auras
(`table_shield_blupi`, `table_shieldloop`, `table_magicloop`), the drink's bubbles (`table_drinkeffect`,
`table_drinkoffset`), the water's speeds (`table_vitesse_nage`, `table_vitesse_surf`, BD-0058).

The yellow gauge (`m_jauges[1]`) shows the shared count for every timed state in the original --
the powers, Invert, the balloon, the squash (Decor.cpp:5574, 5852, 6023, 6046); Galaxy showed it for
the powers only. FIXED_AND_VERIFIED 2026-11-04 (live: an invert sign taken in probe world 985,
`plans/evidence/invert-gauge.png`).
