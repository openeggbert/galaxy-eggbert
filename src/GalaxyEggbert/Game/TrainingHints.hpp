#pragma once

#include <GalaxyEggbert/HintZoneRecord.hpp>

#include <string_view>
#include <vector>

namespace GalaxyEggbert::Game
{
    // Real mobile-eggbert training-hint lookup (plan.md HUD-024), verified
    // directly against Decor.cpp:1258-1310 (Decor::DrawInfo's training-hint
    // branch) and Tables.cpp's table_training1..4 (transcribed with
    // explicit user approval, 2026-07-13). Missions 11-14 only -- every
    // other mission has no training hints at all (Decor::DrawInfo's own
    // `array == nullptr` early-out).
    //
    // gridX/gridZ are RAW grid coordinates (0..99), the same convention
    // every other MoveObject/tile placement uses this session -- real
    // mobile-eggbert X maps to gridX, real Y maps to gridZ (the standard
    // 2D-Y-to-3D-Z convention already established throughout this project).
    // treasuresCollected/inAnyVehicle/hasDynamite drive the real per-record
    // action-flag gate (Decor::IsDisplayInfo): a non-negative flag means
    // "show only when treasuresCollected exactly equals this value" (real
    // m_nbTresor == tableTresor); -1 is unconditional; -2/-3 mean "not in
    // any vehicle" / "in any vehicle" (real source only distinguishes Jeep
    // from Helicopter/Skateboard/Tank, but collapses to the same "in ANY
    // vehicle" result either way -- see this file's own .cpp comment);
    // -4/-5 mean "not carrying dynamite" / "carrying dynamite".
    //
    // Returns an empty view if no record's rect+gate matches the given position/
    // state, OR if the first matching record's real text happens to be
    // empty (a handful of real table_training1 slots are genuinely blank --
    // matches the real source's own "matched, but nothing to draw" case,
    // which still stops the scan rather than falling through to a later
    // record).
    //
    // The texts are the original's English strings with their inline pad
    // pictograms (bytes 0-13, text.png's glyphs 0-13: the wheel's eight
    // directions, Jump, Action, Down, the tilted phone; byte 0 included, hence
    // std::string_view); @p accelerometer picks the accelerometer variant, as
    // the original adds 10000 to the text's id (Decor.cpp:1287-1291; BD-0064).
    [[nodiscard]] std::string_view FindTrainingHint(int mission, int gridX, int gridZ,
                                                    int treasuresCollected,
                                                    bool inAnyVehicle, bool hasDynamite,
                                                    bool accelerometer = false) noexcept;

    // The same lookup for a 3D world that places its hints itself
    // (HintZoneRecord.hpp, 2026-10-25): the zones containing Blupi's cell
    // stand in for the records' 2D rects, scanned in the table's record order
    // whatever order @p zones has; the first whose
    // record's gate is open wins (and, as above, an empty text still stops the
    // scan). A record index the mission's table does not have is skipped.
    [[nodiscard]] std::string_view FindTrainingHintInZones(int mission, const std::vector<HintZoneRecord>& zones,
                                                           int gridX, int gridY, int gridZ, int treasuresCollected,
                                                           bool inAnyVehicle, bool hasDynamite,
                                                           bool accelerometer = false) noexcept;
}
