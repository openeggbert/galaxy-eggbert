#pragma once

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
    // Returns nullptr if no record's rect+gate matches the given position/
    // state, OR if the first matching record's real text happens to be
    // empty (a handful of real table_training1 slots are genuinely blank --
    // matches the real source's own "matched, but nothing to draw" case,
    // which still stops the scan rather than falling through to a later
    // record).
    //
    // Inline control-glyph placeholders in the real strings (button-icon
    // pictograms Text::DrawChar renders inline, e.g. the D-pad/Jump/Action
    // icons) are NOT modeled -- this engine's text renderer only supports
    // the printable ASCII glyph range (see Hud.hpp's own class comment).
    // Replaced with a short bracketed label instead of being silently
    // dropped ([Move]/[Jump]/[Action]), a documented simplification, not a
    // fidelity claim about the exact original button icons.
    [[nodiscard]] const char* FindTrainingHint(int mission, int gridX, int gridZ,
                                               int treasuresCollected,
                                               bool inAnyVehicle, bool hasDynamite) noexcept;
}
