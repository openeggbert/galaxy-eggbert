#pragma once

#include <cstdint>

namespace GalaxyEggbert::Game
{
    // The hanging bar in 3D (owner 2026-11-02). The original shows Blupi hanging in profile, facing
    // the way he moves along the bar, both hands raised (Tables::table_blupi StopSuspend 31,
    // MarchSuspend 32, JumpSuspend 34: icons 156-158, 143-146, 165-166). Its Ray Dream poses
    // (Blupi/suspendu*.rd4, with no bar in the scenes) hold the hands side by side across him, 0.35
    // of a cell apart: from the original's side view one pipe, in 3D two. So a bar cell is two
    // parallel pipes (Decor/barre.rd4's model twice) along its row, one under each hand, and Blupi
    // hangs on the row's centre line facing along it.
    //
    // A cell's row: along X or Z by its bar neighbours at its height (a lone cell runs along X), or a
    // field where it has neighbours on both axes (Galaxy's bar ceilings), where he moves freely.
    enum class BarRow : std::uint8_t
    {
        AlongX,
        AlongZ,
        Field,
    };

    [[nodiscard]] constexpr BarRow ClassifyBarRow(bool barOnX, bool barOnZ) noexcept
    {
        return barOnX && barOnZ ? BarRow::Field : (barOnZ ? BarRow::AlongZ : BarRow::AlongX);
    }

    // Each pipe's distance from the row's centre line (cells): half the hands' spread in the
    // two-handed poses (the palms of StopSuspend's icons 156-158, 0.351 apart at the model's scale).
    inline constexpr float kBarPipeOffset = 0.175f;

    // How far Blupi's model is drawn to his left (cells, negative to his right) while hanging, by the
    // icon showing: his right palm onto his right pipe. Measured on the clip library the shipped
    // hanging clips were exported from (build_clip_library.py, the palm joint "Paume" in each icon's
    // frame, scaled as the game draws him). In the two-handed poses that
    // centres both hands on their pipes; in StopSuspend's pause, hanging by his right hand while he
    // looks round (162-164, 151 and 143 on the way), it puts his body under that pipe, as hanging
    // by one hand would.
    [[nodiscard]] constexpr float HangingModelShift(int icon) noexcept
    {
        switch (icon)
        {
        case 143: return -0.083f;
        case 144: return -0.031f;
        case 145: return -0.052f;
        case 146: return -0.141f;
        case 151: return -0.107f;
        case 162:
        case 163: return -0.193f;
        case 164: return -0.192f;
        case 165: return -0.038f;
        case 166: return -0.043f;
        default: return -0.034f; // 156-158, the still hang
        }
    }
}
