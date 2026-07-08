#include "GEDirectionalCubeTiles.hpp"

#include <GalaxyEggbert/BlockTypes.hpp>

namespace GalaxyEggbert::CNA
{
    namespace
    {
        // A small sample of the tile's OWN texture, taken near its top or
        // bottom edge -- used to approximate a DirectionalCube's "flat
        // fallback color" face without a second, vertex-color shader path
        // (see GEDirectionalCubeTiles.hpp's header comment for why). The
        // questionnaire answers themselves justify this: every named
        // fallback color ("modrý odstín", "hnědý odstín", ...) is described
        // per-icon, not from a fixed palette -- icon 2's answer spells it
        // out directly ("modrý odstín, stejný jako pozadí ikony" = "blue,
        // same as the icon's own background"). Sampling near the top edge
        // for a face specified as "shora" (top) and near the bottom edge for
        // "zdola" (bottom), rather than one fixed corner for both, also
        // reproduces icons where those two colors are genuinely different
        // (e.g. icon 193: gray top / beige bottom) instead of forcing them
        // to match.
        Easy3D::UvRect SwatchUv(const Easy3D::UvRect& tile, float vCenterFrac)
        {
            constexpr float kHalfSize = 0.06f;
            const float uMid = (tile.U0 + tile.U1) * 0.5f;
            const float uHalf = (tile.U1 - tile.U0) * kHalfSize;
            const float vSpan = tile.V1 - tile.V0;
            const float vCenter = tile.V0 + vSpan * vCenterFrac;
            const float vHalf = vSpan * kHalfSize;
            return Easy3D::UvRect{uMid - uHalf, vCenter - vHalf, uMid + uHalf, vCenter + vHalf};
        }

        constexpr float kTopSwatchV = 0.08f;
        constexpr float kBottomSwatchV = 0.92f;

        // Every icon below answered "krychle, textura na (všech/4) bočních
        // stranách" -- all 4 side faces always show the tile's real texture;
        // only top/bottom vary, and only between "flat fallback color"
        // (TopColor/BottomColor = true, approximated via SwatchUv) and
        // "open" (false -- a real hole, like icon 200's). Source: direct
        // user Q&A in mobile-eggbert-reference/
        // questionnaire-all-remaining-tiles.md (round 2) and
        // questionnaire-unused-tiles.md (round 3), confirmed 2026-07-08 --
        // see those files for each icon's exact original wording. Icons
        // needing per-placement face rotation, alpha, or a missing texture
        // asset are deliberately excluded (see the .hpp comment) -- this is
        // not the full ~100-icon DirectionalCube set, only the unambiguous
        // symmetric subset.
        struct SymmetricEntry
        {
            int Icon;
            bool TopColor;    // false = open
            bool BottomColor; // false = open
        };

        constexpr SymmetricEntry kSymmetricEntries[] = {
            // shora + zdola both flat color
            {2, true, true}, {22, true, true}, {23, true, true}, {24, true, true},
            {50, true, true}, {52, true, true}, {86, true, true},
            {193, true, true}, {194, true, true}, {195, true, true}, {196, true, true}, {197, true, true},
            {224, true, true}, {225, true, true}, {226, true, true}, {227, true, true}, {228, true, true},
            {232, true, true}, {283, true, true},
            // shora flat color, zdola open
            {25, true, false}, {26, true, false}, {27, true, false}, {29, true, false},
            {44, true, false}, {45, true, false}, {46, true, false}, {47, true, false},
            {51, true, false}, {87, true, false}, {88, true, false}, {89, true, false}, {90, true, false},
            {154, true, false}, {155, true, false},
            {250, true, false}, {251, true, false}, {252, true, false}, {253, true, false},
            {254, true, false}, {255, true, false}, {256, true, false}, {257, true, false},
            {258, true, false}, {259, true, false}, {260, true, false},
            {364, true, false}, {365, true, false}, {366, true, false},
        };
    }

    bool TryGetDirectionalCubeFaces(int icon, const Easy3D::UvRect& tileUv,
                                    Easy3D::DirectionalCubeFace (&outFaces)[6])
    {
        using Easy3D::CubeFace;

        if (icon == GalaxyEggbert::BlockTypes::Platform)
        {
            // Icon 200 ("Platform"): confirmed by round-1 Q&A
            // (questionnaire-unidentified-tiles.md) and re-confirmed in
            // 02-tiles.md's icon 200 entry -- a passable grate/grid, textured
            // on all 4 side faces, top and bottom genuinely open (a real
            // hole, not a fallback color -- see BlockTypes.hpp's Platform
            // comment).
            for (auto& face : outFaces)
            {
                face.Visible = true;
                face.Uv = tileUv;
            }
            outFaces[static_cast<int>(CubeFace::PosY)].Visible = false;
            outFaces[static_cast<int>(CubeFace::NegY)].Visible = false;
            return true;
        }

        for (const auto& entry : kSymmetricEntries)
        {
            if (entry.Icon != icon)
            {
                continue;
            }

            for (auto& face : outFaces)
            {
                face.Visible = true;
                face.Uv = tileUv;
            }

            auto& top = outFaces[static_cast<int>(CubeFace::PosY)];
            top.Visible = entry.TopColor;
            top.Uv = entry.TopColor ? SwatchUv(tileUv, kTopSwatchV) : top.Uv;

            auto& bottom = outFaces[static_cast<int>(CubeFace::NegY)];
            bottom.Visible = entry.BottomColor;
            bottom.Uv = entry.BottomColor ? SwatchUv(tileUv, kBottomSwatchV) : bottom.Uv;

            return true;
        }

        return false;
    }
}
