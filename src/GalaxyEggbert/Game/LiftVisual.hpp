#pragma once

#include <array>

namespace GalaxyEggbert::Game
{
    // How a platform lift (ObjectType1, object-m.png icon 29) is built in 3D: the tile is a
    // full-width bar (rows 0-11) over a funnel-shaped stem (rows 12-35, cols 23-40 at its
    // narrowest), and rows 36-63 are transparent. Drawn as one cube with that tile, the
    // transparent part showed as a black box (2026-10-25); drawn as the bar and the stem it is
    // a thin platform whose top stays where the cube's was, so Blupi still stands on it.
    //
    // Sizes are in blocks, offsets from the lift's cell centre; u/v are tile-local (0..1).
    struct LiftVisualPart
    {
        float centerYOffset;
        float width;     // X and Z
        float height;
        float u0, v0, u1, v1;
    };

    inline constexpr float kLiftTileRows = 64.0f;
    inline constexpr float kLiftBarRows = 12.0f;
    inline constexpr float kLiftStemRows = 24.0f;
    inline constexpr float kLiftStemFirstCol = 23.0f;
    inline constexpr float kLiftStemCols = 18.0f;

    [[nodiscard]] constexpr std::array<LiftVisualPart, 2> LiftVisualParts() noexcept
    {
        constexpr float bar = kLiftBarRows / kLiftTileRows;
        constexpr float stem = kLiftStemRows / kLiftTileRows;
        return {{
            {0.5f - bar * 0.5f, 1.0f, bar, 0.0f, 0.0f, 1.0f, bar},
            {0.5f - bar - stem * 0.5f, kLiftStemCols / kLiftTileRows, stem,
             kLiftStemFirstCol / kLiftTileRows, bar, (kLiftStemFirstCol + kLiftStemCols) / kLiftTileRows, bar + stem},
        }};
    }

    // A lift carries its own icon, any decor tile of object-m (the original's levels use 29 most, then
    // 361, 387, 340, 153, 215 ...), and is drawn as that tile moving (owner 2026-11-06: "draw the lift
    // as the tile"). How its look is drawn:
    //  - Model:      the tile has a Ray Dream model (the terrain draws it as one; 29 is techno29.rd4)
    //  - BarAndStem: tile 29 without its model: the bar over the funnel stem (LiftVisualParts)
    //  - TileCube:   any other tile: a cube wearing the tile's faces, as the terrain's cube would
    enum class LiftDrawPath
    {
        Model,
        BarAndStem,
        TileCube,
    };

    inline constexpr int kLiftTileIcon = 29;

    [[nodiscard]] constexpr LiftDrawPath ChooseLiftDrawPath(int icon, bool tileHasModel) noexcept
    {
        if (tileHasModel)
        {
            return LiftDrawPath::Model;
        }
        return icon == kLiftTileIcon ? LiftDrawPath::BarAndStem : LiftDrawPath::TileCube;
    }
}
